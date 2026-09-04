# Uses our Linux version 22 for our base image
FROM ubuntu:22.04

# These are the packages we need installed. Updates first then Installs all 5 packages 
RUN apt-get update && apt-get install -y \
    gcc-arm-none-eabi \
    binutils-arm-none-eabi \
    cmake \
    make \
    ninja-build \
    qemu-system-arm \
    git

# Sets the working directory for all commands to run from here
WORKDIR /app

# Copies all of our files from our source to our container
COPY . /app

# Clone FreeRTOS directly — bypasses submodule issues
RUN git clone --depth=1 https://github.com/FreeRTOS/FreeRTOS-Kernel.git /app/FreeRTOS

# Creates a new layer on our newly created image and executes on the files
RUN cmake -B build -G "Ninja" && cmake --build build

# Executes the container by running the firmware in QEMU ARM emulator for when container starts
CMD ["qemu-system-arm", "-M", "lm3s6965evb", \
     "-kernel", "build/satellite_fc.elf", \
     "-serial", "mon:stdio", \
     "-nographic"]