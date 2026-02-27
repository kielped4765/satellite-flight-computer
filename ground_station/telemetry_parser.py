import struct

TLM_MAGIC = 0xA5C3
TLM_FORMAT = "<HHIffffffBBhH" # little-endian packed — matches TelemetryPacket_t
TLM_SIZE = struct.calcsize(TLM_FORMAT) # 46 bytes

MODES = {0: "SAFE", 1: "DETUMBLE", 2: "NADIR", 3: "NORMAL"}
FIELDS = ["magic","sequence","timestamp_ms",
          "roll","pitch","yaw",
          "omega_x","omega_y","omega_z",
          "mode","faults","temperature","checksum"]

def parse_packet (data: bytes) -> dict | None:
    """Parse bytes -> dict. Returns None if magic or checksum is invalid."""
    if len(data) < TLM_SIZE:
        return None
    try:
        values = struct.unpack(TLM_FORMAT, data[:TLM_SIZE])
        pkt = dict(zip(FIELDS, values))
        if pkt["magic"] != TLM_MAGIC:
            return None
        # Verify XOR checksum
        csum = 0
        for byte in data[:TLM_SIZE - 2]:
            csum ^= byte
        if csum != pkt["checksum"]:
            return None
        pkt["temperature_c"] = pkt["temperature"] / 10.0
        pkt["mode_str"]      = MODES.get(pkt["mode"], "UNKNOWN")
        return pkt
    except struct.error:
        return None
    
def find_packet(buf: bytearray) -> tuple:
    """Scan a bytearray for a valid packet.
    Retrusn (packet_dict_or_None, bytes_consumed)
    Caller should remove the consumed bytes from buf."""
    magic_bytes = struct.pack("<H", TLM_MAGIC)
    idx = buf.find(magic_bytes)
    if idx < 0:
        return None, len(buf) # discard all
    pkt = parse_packet(bytes(buf[idx:]))
    if pkt:
        return pkt, idx + TLM_SIZE # Good packet
    return None, idx + 1           # Bad packet, skip magic and try again

