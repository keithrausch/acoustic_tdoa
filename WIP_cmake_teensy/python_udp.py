#!/usr/bin/env python3

import asyncio
import struct


PORT = 5000

HEADER_FORMAT = "<IHHQIHH"

HEADER_SIZE = struct.calcsize(HEADER_FORMAT)

MAGIC = 0x54455354

class UDPReceiver(asyncio.DatagramProtocol):

    def datagram_received(self, data, addr):

        if len(data) < HEADER_SIZE:
            return

        header = data[:HEADER_SIZE]

        (
            magic,
            version,
            msg_type,
            timestamp,
            sequence,
            sample_count,
            channels,
        ) = struct.unpack(
            HEADER_FORMAT,
            header
        )

        if magic != MAGIC:
            print("bad packet")
            return

        payload = data[HEADER_SIZE:]

        samples = struct.unpack(
            f"<{len(payload)//2}h",
            payload
        )

        print(
            f"{addr[0]} "
            f"seq={sequence} "
            f"time={timestamp} "
            f"samples={len(samples)}"
        )

async def main():

    loop = asyncio.get_running_loop()

    transport, _ = await loop.create_datagram_endpoint(
        UDPReceiver,
        local_addr=("0.0.0.0", PORT),
    )

    print(f"listening UDP {PORT}")

    try:
        await asyncio.Future()
    finally:
        transport.close()


asyncio.run(main())