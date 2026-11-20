#!/usr/bin/env python3

import serial
import time
import sys


PORT = "/dev/ttyACM0"
BAUD = 115200


def main():
    print(f"Opening {PORT}")

    with serial.Serial(PORT, BAUD, timeout=1) as ser:
        time.sleep(2)  # wait for USB serial reset

        print("Reading counter:")

        while True:
            line = ser.readline()

            if not line:
                continue

            text = line.decode(errors="replace").strip()

            print(text)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\nStopped")
        sys.exit(0)