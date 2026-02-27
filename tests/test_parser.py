import struct, pytest, sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "ground_station"))
from telemetry_parser import parse_packet, find_packet, TLM_MAGIC, TLM_FORMAT, TLM_SIZE

def _make(magic=TLM_MAGIC, roll=10.5, pitch=-5.2, yaw=90.0,
          mode=3, fault=0, seq=1, ts=1000,
          ox=.01, oy=0.0, oz=0.05, temp=2350) -> bytes:
    """Build a valid binary packet with correct cheksum."""
    raw = struct.pack(TLM_FORMAT, magic, seq, ts,
                      roll, pitch, yaw, ox, oy, oz,
                      mode, fault, temp, 0)
    
    csum = 0
    for b in raw[:-2]: csum ^= b
    return raw[:-2] + struct.pack("<H", csum)

def test_valid_packet():
    pkt = parse_packet(_make())
    assert pkt is not None
    assert pkt["roll"] == pytest.approx(10.5, abs=1e-4)
    assert pkt["pitch"] == pytest.approx(-5.2, abs=1e-4)
    assert pkt["yaw"] == pytest.approx(90.0, abs=1e-4)
    assert pkt["mode_str"] == "NOMINAL"
    assert pkt["temperature_c"] == pytest.approx(235.0, abs=0.01)

def test_bad_magic_rejected():
    assert parse_packet(_make(magic=0xDEAD)) is None

def test_bad_checksum_rejected():
    raw = bytearray(_make())
    raw[-1] ^= 0xFF # Flip bits in checksum
    assert parse_packet(bytes(raw)) is None

def test_too_short_returns_none():
    assert parse_packet(b"\xC3\xA5\x00") is None

def test_find_packet_with_garbage_prefix():
    garbage = b"\xDE\xAD\xBE\xEF" * 3
    buf = bytearray(garbage + _make())
    pkt, consumed = find_packet(buf)
    assert pkt is not None
    assert pkt["roll"] == pytest.approx(10.5, abs=1e-4)

def test_find_packet_incomplete_returns_none():
    half = bytearray(_make()[:10]) # Only half a packet
    pkt, _ = find_packet(half)
    assert pkt is None
    