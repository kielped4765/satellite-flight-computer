# Start from official Python 3.13 minimal image (the slim addition)
FROM python:3.13-slim

# Set working directory for all subsequent commands
WORKDIR /app

# Update package list and install tkinter for GUI support
RUN apt-get update && apt-get install -y \
    python3-tk \
    xvfb \
    x11vnc \
    procps \
    novnc

# Install Python dependencies before copying code (layer caching)
RUN pip install matplotlib pyserial pytest



# Copy only the ground station folder which includes gui.py and telemetry_parser.py
COPY ground_station/ /app/ground_station/

COPY docker/groundstation/start.sh /app/start.sh

RUN chmod +x /app/start.sh

# Expose VNC port for browser connection
EXPOSE 5900

# Start the ground station GUI when container runs
CMD ["/app/start.sh"]