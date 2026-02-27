import tkinter as tk
from turtle import right
import serial, threading, queue
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from collections import deque
from telemetry_parser import find_packet

MAX_HISTORY = 120 # 2 minutes at 1 Hz

class GroundStation:
    def __init__(self, port="tmp/gs_port", baud=115200):
        self.port       = port
        self.baud       = baud
        self.running    = False
        self.q          = queue.Queue(maxsize=100)
        self.times      = deque(maxlen=MAX_HISTORY)
        self.rolls      = deque(maxlen=MAX_HISTORY)
        self.pitches    = deque(maxlen=MAX_HISTORY)
        self.yaws       = deque(maxlen=MAX_HISTORY)
        self.pkt_count  = 0
        self._build_ui()

    def _build_ui(self):
        self.root = tk.Tk()
        self.root.title("Satellite Ground Station")
        self.root.configure(bg="#0D1B2A")
        self.root.geometry("1200x760")

        # Header bar
        hdr = tk.Frame(self.root, bg="#0078D4", height=48)
        hdr.pack(fill=tk.X)
        tk.Label(hdr, text="SATELLITE FLIGHT COMPUTER | GROUND STATION",
                 font=("Arial",13,"bold"), bg="#0078D4", fg="white").pack(side=tk.LEFT,
                    padx=16, pady=10)
        self.status_lbl = tk.Label(hdr, text="OFFLINE",
                                   font=("Arial",12,"bold"), bg="#0078D4", fg="#FF6B6B")
        self.status_lbl.pack(side=tk.RIGHT, padx=16)

        # Layout
        main = tk.Frame(self.root, bg="#0D1B2A")
        main.pack(fill=tk.BOTH, expand=True, padx=8, pady=8)

        left = tk.Frame(main, bg="#0D1B2A", width=300)
        left.pack(side=tk.LEFT, fill=tk.Y, padx=(0, 8))

        self._card(left, "ATTITUDE (deg)",
                   ["Roll", "Pitch", "Yaw"],
                   ["roll_lbl", "pitch_lbl", "yaw_lbl"])
        self._card(left, "ANGULAR RATE (rad/s)",
                   ["Omega X","Omega Y","Omega Z"],
                   ["ox_lbl","oy_lbl","oz_lbl"])
        self._card(left, "SYSTEM",
                   ["Mode","Faults","Temp (C)","Packets"],
                   ["mode_lbl","fault_lbl","temp_lbl","pkt_lbl"]) 

        # Command panel

        cmd_frame = tk.LabelFrame(left, text="COMMAND UPLINK",
                                  fg="#0078D4", bg="#132436",
                                  font=("Arial", 10, "bold"))
        cmd_frame.pack(fill=tk.X, pady=4)
        commands = {("NOMINAL MODE", "N"), ("SAFE MODE", "S"),
                    ("DETUMBLE MODE", "D"), ("REBOOT", "R")}
        for label, byte in commands:
            tk.Button(cmd_frame, text=label,
                      command=lambda b=byte: self._send(b),
                      bg="#1B3A5C", fg="white",
                      font=("Courier", 9, "bold"),
                      relief=tk.FLAT, padx=8, pady=4
                      ).pack(fill=tk.X, padx=4, pady=2)

        # Plot panel
        right = tk.Frame(main, bg="#0D1B2A")
        right.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        
        plt.style.use("dark_background")
        self.fig, (self.ax1, self.ax2) = plt.subplots(2, 1, figsize=(7, 5))
        self.fig.patch.set_facecolor("#0D1B2A")
        for ax in (self.ax1, self.ax2):
            ax.set_facecolor("#132436")
 
        self.canvas = FigureCanvasTkAgg(self.fig, master=right)
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)
 
        self.conn_btn = tk.Button(
            self.root, text="CONNECT",
            command=self._toggle,
            bg="#107C10", fg="white",
            font=("Arial",11,"bold"), pady=6)
        self.conn_btn.pack(fill=tk.X, padx=8, pady=(0, 8))
 
    def _card(self, parent, title, labels, attr_names):
        frame = tk.LabelFrame(parent, text=title,
                              fg="#0078D4", bg="#132436",
                              font=("Arial",10,"bold"))
        frame.pack(fill=tk.X, pady=3)
        for lbl, attr in zip(labels, attr_names):
            row = tk.Frame(frame, bg="#132436")
            row.pack(fill=tk.X, padx=6)
            tk.Label(row, text=lbl+":", width=12, anchor="w",
                     bg="#132436", fg="#8899AA",
                     font=("Courier",10)).pack(side=tk.LEFT)
            val = tk.Label(row, text="--", anchor="w",
                           bg="#132436", fg="#00FF88",
                           font=("Courier",10,"bold"))
            val.pack(side=tk.LEFT)
            setattr(self, attr, val)
 
    def _toggle(self):
        if self.running:
            self.running = False
            self.conn_btn.config(text="CONNECT", bg="#107C10")
            self.status_lbl.config(text="OFFLINE", fg="#FF6B6B")
 
        else:
            self.running = True
            self.conn_btn.config(text="DISCONNECT", bg="#D83B01")
            self.status_lbl.config(text="ONLINE", fg="#00FF88")
            threading.Thread(target=self._rx, daemon=True).start()
            self._tick()

    def _rx(self):
        """Background thread: read serial bytes, find packets, push to queue."""
        try:
            ser = serial.Serial(self.port, self.baud, timeout=1)
            buf = bytearray()
            while self.running:
                data = ser.read(256)
                if data:
                    buf.extend(data)
                    pkt, consumed = find_packet(buf)
                    buf = buf[consumed:]
                    if pkt and not self.q.full():
                        self.q.put(pkt)
        except serial.SerialException as e:
            print(f"Serial error: {e}")
            self.running = False
 
    def _tick(self):
        """Main thread: drain queue, update labels and plot every 500 ms."""
        while not self.q.empty():
            self._update(self.q.get_nowait())
        if self.running:
            self.root.after(500, self._tick)
 
    def _update(self, pkt):
        self.pkt_count += 1
        self.times.append(pkt["timestamp_ms"] / 1000.0)
        self.rolls.append(pkt["roll"])
        self.pitches.append(pkt["pitch"])
        self.yaws.append(pkt["yaw"])

        self.roll_lbl.config( text=f'{ pkt["roll"]:+8.2f}')
        self.pitch_lbl.config(text=f'{ pkt["pitch"]:+8.2f}')
        self.yaw_lbl.config( text=f'{ pkt["yaw"]:+8.2f}')
        self.ox_lbl.config( text=f'{ pkt["omega_x"]:+7.4f}')
        self.oy_lbl.config( text=f'{ pkt["omega_y"]:+7.4f}')
        self.oz_lbl.config( text=f'{ pkt["omega_z"]:+7.4f}')
        self.mode_lbl.config( text=pkt["mode_str"])
        self.temp_lbl.config( text=f'{pkt["temperature_c"]:.1f}')
        self.pkt_lbl.config( text=str(self.pkt_count))
 
        fault_color = "#00FF88" if pkt["faults"] == 0 else "#FF4444"
        fault_text = "NOMINAL" if pkt["faults"] == 0 else f'0x{pkt["faults"]:02X}'
        self.fault_lbl.config(text=fault_text, fg=fault_color)
        
        xs = list(self.times)
        self.ax1.cla()
        self.ax2.cla()
        self.ax1.plot(xs, list(self.rolls), color="#FF6B6B", label="Roll")
        self.ax1.plot(xs, list(self.pitches), color="#4ECDC4", label="Pitch")
        self.ax1.plot(xs, list(self.yaws), color="#FFE66D", label="Yaw")
        self.ax1.set_title("Attitude (deg)", color="white")
        self.ax1.legend(loc="upper right", fontsize=8)
        self.ax1.set_facecolor("#132436")
        self.ax2.plot(xs, list(deque((p["omega_x"] for p in []), maxlen=MAX_HISTORY)),
                      color="#A8D8EA", label="Omega X")
        self.ax2.set_title("Angular Rate (rad/s)", color="white")
        self.ax2.set_facecolor("#132436")
        self.canvas.draw()
        
    def _send(self, cmd: str):
        """Send a command byte to the flight computer."""
        print(f"CMD -> {cmd}") # Extend: write cmd.encode() to serial
    
    def run(self):
        self.root.mainloop()
if __name__ == "__main__":
    import argparse
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", default="/tmp/gs_port")
    ap.add_argument("--baud", type=int, default=115200)
    args = ap.parse_args()
    GroundStation(args.port, args.baud).run()