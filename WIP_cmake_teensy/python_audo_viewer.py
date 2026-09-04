#!/usr/bin/env python3

"""
Lossless Teensy UDP audio recorder and sample-accurate viewer.

UDP packet:

struct UDPAudioBlock
{
    uint64_t sync{0};
    uint16_t msg_type{0};
    uint16_t session_id{0};
    uint16_t channel_id{0};
    uint64_t block_index{0};
    std::array<int16_t, AUDIO_BLOCK_SAMPLES> data{};
};

The recording is lossless:
    - Audio samples are stored as exact little-endian int16 values.
    - UDP block indices are stored separately.
    - Missing blocks are NOT removed from the time axis.
    - Missing blocks appear as gaps in the waveform.
    - Duplicate packets are detected.
    - Out-of-order packets are detected.

Viewer:
    - Four individual channel plots.
    - Fifth plot containing all channels.
    - All plots share the X/time axis.
    - Mouse controls only X.
    - Automatic Y scaling based on visible samples.
    - Full int16 Y range mode.
    - Shared Y scaling mode.
    - Exact sample inspection.
    - Missing blocks appear as breaks.
    - Block boundaries themselves are NOT artificially disconnected.
      If two consecutive blocks exist, the waveform is continuous across
      the boundary.
"""


import argparse
import json
import socket
import struct
import sys
import time

from dataclasses import dataclass
from datetime import datetime
from pathlib import Path

import numpy as np
import pyqtgraph as pg
from pyqtgraph.Qt import QtCore, QtWidgets


# ============================================================================
# Configuration
# ============================================================================

NCHANNELS = 4

AUDIO_BLOCK_SAMPLES = 128

DEFAULT_HOST = "0.0.0.0"
DEFAULT_PORT = 5000

INT16_MIN = -32768
INT16_MAX = 32767


# ============================================================================
# UDP packet format
# ============================================================================

PACKET_HEADER_FORMAT = "<QHHH2xQ"

PACKET_HEADER_SIZE = struct.calcsize(
    PACKET_HEADER_FORMAT
)

PACKET_DATA_SIZE = (
    AUDIO_BLOCK_SAMPLES * 2
)

PACKET_SIZE = (
    PACKET_HEADER_SIZE
    + PACKET_DATA_SIZE
)


# ============================================================================
# UDP packet
# ============================================================================

@dataclass
class UDPAudioBlock:

    sync: int
    msg_type: int
    session_id: int
    channel_id: int
    block_index: int
    data: np.ndarray


def parse_packet(packet: bytes) -> UDPAudioBlock:

    if len(packet) != PACKET_SIZE:

        raise ValueError(
            f"wrong packet size: "
            f"received {len(packet)} bytes, "
            f"expected {PACKET_SIZE}"
        )

    (
        sync,
        msg_type,
        session_id,
        channel_id,
        block_index,
    ) = struct.unpack_from(
        PACKET_HEADER_FORMAT,
        packet,
        0,
    )

    data = np.frombuffer(
        packet,
        dtype="<i2",
        offset=PACKET_HEADER_SIZE,
        count=AUDIO_BLOCK_SAMPLES,
    ).copy()

    return UDPAudioBlock(
        sync=sync,
        msg_type=msg_type,
        session_id=session_id,
        channel_id=channel_id,
        block_index=block_index,
        data=data,
    )


# ============================================================================
# Recorder
# ============================================================================

class Recorder:

    def __init__(
        self,
        host,
        port,
        duration,
        sample_rate,
        output_directory,
    ):

        self.host = host
        self.port = port
        self.duration = duration
        self.sample_rate = sample_rate
        self.output_directory = output_directory

        #
        # channel -> block_index -> samples
        #

        self.blocks = {
            channel: {}
            for channel in range(NCHANNELS)
        }

        self.packet_count = 0
        self.bad_packet_count = 0
        self.duplicate_packet_count = 0

        self.duplicate_counts = {
            channel: 0
            for channel in range(NCHANNELS)
        }

        self.out_of_order_counts = {
            channel: 0
            for channel in range(NCHANNELS)
        }

        self.last_received_block = {
            channel: None
            for channel in range(NCHANNELS)
        }

        self.sync_values = set()
        self.session_ids = set()
        self.msg_types = set()

    def record(self):

        print()
        print("UDP AUDIO RECORDER")
        print("==================")
        print(
            f"Listening on:   "
            f"{self.host}:{self.port}"
        )
        print(
            f"Duration:       "
            f"{self.duration:.3f} s"
        )
        print(
            f"Sample rate:    "
            f"{self.sample_rate:g} Hz"
        )
        print(
            f"Packet size:    "
            f"{PACKET_SIZE} bytes"
        )
        print(
            f"Samples/packet: "
            f"{AUDIO_BLOCK_SAMPLES}"
        )
        print()

        sock = socket.socket(
            socket.AF_INET,
            socket.SOCK_DGRAM,
        )

        sock.setsockopt(
            socket.SOL_SOCKET,
            socket.SO_RCVBUF,
            16 * 1024 * 1024,
        )

        sock.bind(
            (self.host, self.port)
        )

        sock.settimeout(0.1)

        print("Recording...")
        print()

        start = time.monotonic()

        try:

            while True:

                elapsed = (
                    time.monotonic()
                    - start
                )

                if elapsed >= self.duration:
                    break

                try:

                    packet, address = (
                        sock.recvfrom(65535)
                    )

                except socket.timeout:

                    continue

                try:

                    message = parse_packet(
                        packet
                    )

                except ValueError as exc:

                    self.bad_packet_count += 1

                    print(
                        f"WARNING: {exc}",
                        file=sys.stderr,
                    )

                    continue

                channel = message.channel_id

                if channel >= NCHANNELS:

                    print(
                        f"WARNING: invalid channel "
                        f"{channel}",
                        file=sys.stderr,
                    )

                    continue

                self.packet_count += 1

                self.sync_values.add(
                    message.sync
                )

                self.session_ids.add(
                    message.session_id
                )

                self.msg_types.add(
                    message.msg_type
                )

                # --------------------------------------------------------
                # Out-of-order detection
                # --------------------------------------------------------

                previous_block = (
                    self.last_received_block[
                        channel
                    ]
                )

                if (
                    previous_block is not None
                    and message.block_index
                    < previous_block
                ):

                    self.out_of_order_counts[
                        channel
                    ] += 1

                self.last_received_block[
                    channel
                ] = message.block_index

                # --------------------------------------------------------
                # Duplicate detection
                # --------------------------------------------------------

                channel_blocks = (
                    self.blocks[channel]
                )

                if (
                    message.block_index
                    in channel_blocks
                ):

                    self.duplicate_packet_count += 1

                    self.duplicate_counts[
                        channel
                    ] += 1

                    print(
                        f"WARNING: duplicate block "
                        f"channel={channel}, "
                        f"block={message.block_index}",
                        file=sys.stderr,
                    )

                #
                # Keep the most recently received copy.
                #

                channel_blocks[
                    message.block_index
                ] = message.data

                if self.packet_count % 100 == 0:

                    print(
                        f"\rReceived "
                        f"{self.packet_count:,} "
                        f"packets",
                        end="",
                        flush=True,
                    )

        except KeyboardInterrupt:

            print()
            print("Recording interrupted.")

        finally:

            sock.close()

        print()
        print()

        print(
            f"Received packets:  "
            f"{self.packet_count:,}"
        )

        print(
            f"Bad packets:       "
            f"{self.bad_packet_count:,}"
        )

        print(
            f"Duplicate packets: "
            f"{self.duplicate_packet_count:,}"
        )

        print()

        return self.save()

    def save(self):

        timestamp = datetime.now().strftime(
            "%Y-%m-%d_%H-%M-%S"
        )

        recording_directory = (
            self.output_directory
            / timestamp
        )

        recording_directory.mkdir(
            parents=True,
            exist_ok=False,
        )

        print(
            f"Saving recording to:"
        )

        print(
            f"  {recording_directory}"
        )

        metadata = {

            "format_version": 2,

            "created":
                datetime.now().isoformat(),

            "udp": {

                "host": self.host,

                "port": self.port,

                "packet_size":
                    PACKET_SIZE,

                "packet_header_size":
                    PACKET_HEADER_SIZE,
            },

            "audio": {

                "channels":
                    NCHANNELS,

                "block_samples":
                    AUDIO_BLOCK_SAMPLES,

                "sample_rate_hz":
                    self.sample_rate,

                "sample_dtype":
                    "int16",

                "endianness":
                    "little",
            },

            "protocol": {

                "sync_values": sorted(
                    int(x)
                    for x in self.sync_values
                ),

                "session_ids": sorted(
                    int(x)
                    for x in self.session_ids
                ),

                "msg_types": sorted(
                    int(x)
                    for x in self.msg_types
                ),
            },

            "recording": {

                "requested_duration_s":
                    self.duration,

                "received_packets":
                    self.packet_count,

                "bad_packets":
                    self.bad_packet_count,

                "duplicate_packets":
                    self.duplicate_packet_count,
            },

            "channels": {},
        }

        print()
        print("RECORDING QUALITY")
        print("=================")

        total_missing = 0
        total_duplicates = 0
        total_out_of_order = 0

        for channel in range(NCHANNELS):

            blocks = self.blocks[channel]

            audio_filename = (
                f"channel_{channel}.bin"
            )

            block_filename = (
                f"channel_{channel}_blocks.bin"
            )

            audio_path = (
                recording_directory
                / audio_filename
            )

            block_path = (
                recording_directory
                / block_filename
            )

            duplicate_count = (
                self.duplicate_counts[channel]
            )

            out_of_order_count = (
                self.out_of_order_counts[channel]
            )

            total_duplicates += (
                duplicate_count
            )

            total_out_of_order += (
                out_of_order_count
            )

            if not blocks:

                audio_path.touch()
                block_path.touch()

                metadata["channels"][
                    str(channel)
                ] = {

                    "audio_file":
                        audio_filename,

                    "block_index_file":
                        block_filename,

                    "block_count":
                        0,

                    "sample_count":
                        0,

                    "first_block_index":
                        None,

                    "last_block_index":
                        None,

                    "missing_block_count":
                        0,

                    "duplicate_block_count":
                        duplicate_count,

                    "out_of_order_packet_count":
                        out_of_order_count,
                }

                print(
                    f"Channel {channel}"
                )

                print(
                    "  Received blocks:    0"
                )

                print(
                    "  Missing blocks:     0"
                )

                print(
                    f"  Duplicate blocks:   "
                    f"{duplicate_count}"
                )

                print(
                    f"  Out-of-order:       "
                    f"{out_of_order_count}"
                )

                continue

            block_indices = np.asarray(
                sorted(blocks.keys()),
                dtype="<u8",
            )

            block_indices.tofile(
                block_path
            )

            with audio_path.open("wb") as f:

                for block_index in block_indices:

                    data = blocks[
                        int(block_index)
                    ]

                    data.astype(
                        "<i2",
                        copy=False,
                    ).tofile(f)

            first_block = int(
                block_indices[0]
            )

            last_block = int(
                block_indices[-1]
            )

            expected_block_count = (
                last_block
                - first_block
                + 1
            )

            received_block_count = (
                len(block_indices)
            )

            missing_block_count = (
                expected_block_count
                - received_block_count
            )

            total_missing += (
                missing_block_count
            )

            print(
                f"Channel {channel}"
            )

            print(
                f"  Received blocks:    "
                f"{received_block_count:,}"
            )

            print(
                f"  First block:        "
                f"{first_block:,}"
            )

            print(
                f"  Last block:         "
                f"{last_block:,}"
            )

            print(
                f"  Missing blocks:     "
                f"{missing_block_count:,}"
            )

            print(
                f"  Duplicate blocks:   "
                f"{duplicate_count:,}"
            )

            print(
                f"  Out-of-order:       "
                f"{out_of_order_count:,}"
            )

            if missing_block_count:

                expected = np.arange(
                    first_block,
                    last_block + 1,
                    dtype=np.uint64,
                )

                missing = np.setdiff1d(
                    expected,
                    block_indices,
                )

                preview_count = min(
                    len(missing),
                    20,
                )

                preview = ", ".join(
                    str(int(x))
                    for x in missing[
                        :preview_count
                    ]
                )

                print(
                    f"  Missing indices:    "
                    f"{preview}",
                    end="",
                )

                if len(missing) > preview_count:

                    print(
                        f" ... "
                        f"({len(missing) - preview_count:,} more)"
                    )

                else:

                    print()

            metadata["channels"][
                str(channel)
            ] = {

                "audio_file":
                    audio_filename,

                "block_index_file":
                    block_filename,

                "block_count":
                    received_block_count,

                "sample_count":
                    received_block_count
                    * AUDIO_BLOCK_SAMPLES,

                "first_block_index":
                    first_block,

                "last_block_index":
                    last_block,

                "missing_block_count":
                    missing_block_count,

                "duplicate_block_count":
                    duplicate_count,

                "out_of_order_packet_count":
                    out_of_order_count,
            }

        print()
        print("OVERALL")
        print("=======")

        print(
            f"Missing blocks:     "
            f"{total_missing:,}"
        )

        print(
            f"Duplicate blocks:   "
            f"{total_duplicates:,}"
        )

        print(
            f"Out-of-order:       "
            f"{total_out_of_order:,}"
        )

        if (
            total_missing == 0
            and total_duplicates == 0
            and total_out_of_order == 0
            and self.bad_packet_count == 0
        ):

            print()
            print(
                "✓ No packet integrity problems detected."
            )

        else:

            print()
            print(
                "⚠ Packet integrity problems were detected."
            )

        metadata_path = (
            recording_directory
            / "metadata.json"
        )

        with metadata_path.open("w") as f:

            json.dump(
                metadata,
                f,
                indent=2,
            )

        print()
        print(
            f"Metadata written to:"
        )

        print(
            f"  {metadata_path}"
        )

        return recording_directory


# ============================================================================
# Recording loader
# ============================================================================

class Recording:

    def __init__(
        self,
        directory,
    ):

        self.directory = directory

        metadata_path = (
            directory
            / "metadata.json"
        )

        if not metadata_path.exists():

            raise RuntimeError(
                f"{directory} does not contain "
                f"metadata.json"
            )

        with metadata_path.open() as f:

            self.metadata = json.load(f)

        audio_metadata = (
            self.metadata["audio"]
        )

        self.sample_rate = float(
            audio_metadata[
                "sample_rate_hz"
            ]
        )

        self.block_samples = int(
            audio_metadata.get(
                "block_samples",
                AUDIO_BLOCK_SAMPLES,
            )
        )

        self.channels = {}

        for channel in range(NCHANNELS):

            info = (
                self.metadata[
                    "channels"
                ].get(str(channel))
            )

            if info is None:
                continue

            audio_path = (
                directory
                / info["audio_file"]
            )

            block_path = (
                directory
                / info["block_index_file"]
            )

            sample_count = int(
                info["sample_count"]
            )

            block_count = int(
                info["block_count"]
            )

            if sample_count:

                samples = np.memmap(
                    audio_path,
                    dtype="<i2",
                    mode="r",
                    shape=(sample_count,),
                )

                blocks = np.memmap(
                    block_path,
                    dtype="<u8",
                    mode="r",
                    shape=(block_count,),
                )

            else:

                samples = np.empty(
                    0,
                    dtype="<i2",
                )

                blocks = np.empty(
                    0,
                    dtype="<u8",
                )

            self.channels[channel] = {

                "samples":
                    samples,

                "blocks":
                    blocks,
            }

    def samples(self, channel):

        return self.channels[
            channel
        ]["samples"]

    def block_indices(self, channel):

        return self.channels[
            channel
        ]["blocks"]

    def sample_count(self, channel):

        return len(
            self.samples(channel)
        )

    def print_summary(self):

        print()
        print("RECORDING")
        print("=========")

        print(
            f"Directory:   "
            f"{self.directory}"
        )

        print(
            f"Created:     "
            f"{self.metadata.get('created', 'unknown')}"
        )

        print(
            f"Sample rate: "
            f"{self.sample_rate:g} Hz"
        )

        print(
            f"Block size:  "
            f"{self.block_samples} samples"
        )

        print()

        for channel in range(NCHANNELS):

            info = (
                self.metadata[
                    "channels"
                ].get(str(channel))
            )

            if info is None:
                continue

            print(
                f"Channel {channel}: "
                f"{info['sample_count']:,} samples, "
                f"{info['block_count']:,} blocks, "
                f"{info['missing_block_count']:,} missing, "
                f"{info.get('duplicate_block_count', 0):,} duplicate, "
                f"{info.get('out_of_order_packet_count', 0):,} out-of-order"
            )


# ============================================================================
# Plot colors
# ============================================================================

CHANNEL_COLORS = [

    (50, 120, 220),

    (220, 70, 70),

    (50, 170, 90),

    (170, 80, 200),
]

# ============================================================================
# Waveform renderer
# ============================================================================

class WaveformView:
    """
    Renders one channel using the ORIGINAL UDP block indices.

    Important:
        The position of a block in `block_indices` is NOT its block number.

        For example:

            block_indices = [100, 101, 103, 104]

        means block 102 is missing.

    Therefore every block's X position is calculated from its actual
    block_index.

    The underlying samples are never modified.
    """

    def __init__(
        self,
        plot,
        data,
        block_indices,
        sample_rate,
        channel,
        on_sample=None,
        color=None,
    ):

        self.plot = plot
        self.data = data
        self.block_indices = block_indices
        self.sample_rate = sample_rate
        self.channel = channel
        self.on_sample = on_sample
        self.y_mode = "auto"

        if color is None:
            color = CHANNEL_COLORS[
                channel % len(CHANNEL_COLORS)
            ]

        self.curve = pg.PlotDataItem(
            pen=pg.mkPen(
                color=color,
                width=1,
            ),
            symbol=None,
            connect="finite",
        )

        self.plot.addItem(self.curve)

        self.plot.setLabel("left", "int16")
        self.plot.setLabel(
            "bottom",
            "time",
            units="s",
        )

        self.plot.showGrid(
            x=True,
            y=True,
            alpha=0.25,
        )

        self.plot.setTitle(
            f"Channel {channel}"
        )

        self.plot.setMouseEnabled(
            x=True,
            y=False,
        )

        self._last_range = None

        self.vline = pg.InfiniteLine(
            angle=90,
            movable=False,
            pen=pg.mkPen(
                color=(150, 150, 150),
                style=QtCore.Qt.PenStyle.DashLine,
            ),
        )

        self.hline = pg.InfiniteLine(
            angle=0,
            movable=False,
            pen=pg.mkPen(
                color=(150, 150, 150),
                style=QtCore.Qt.PenStyle.DashLine,
            ),
        )

        self.plot.addItem(
            self.vline,
            ignoreBounds=True,
        )

        self.plot.addItem(
            self.hline,
            ignoreBounds=True,
        )

        self.plot.scene().sigMouseMoved.connect(
            self.mouse_moved
        )

        self.update()

    # ------------------------------------------------------------------------
    # Visible time/sample range
    # ------------------------------------------------------------------------

    def visible_sample_range(self):

        x0, x1 = self.plot.viewRange()[0]

        sample0 = max(
            0,
            int(np.floor(
                x0 * self.sample_rate
            )),
        )

        sample1 = max(
            sample0,
            int(np.ceil(
                x1 * self.sample_rate
            )) + 1,
        )

        return sample0, sample1

    # ------------------------------------------------------------------------
    # Find blocks overlapping the visible sample range
    # ------------------------------------------------------------------------

    def visible_block_range(self):

        sample0, sample1 = (
            self.visible_sample_range()
        )

        if len(self.block_indices) == 0:
            return 0, 0

        first_possible_block = (
            sample0 // AUDIO_BLOCK_SAMPLES
        )

        last_possible_block = (
            (sample1 - 1)
            // AUDIO_BLOCK_SAMPLES
        )

        #
        # IMPORTANT:
        #
        # block_indices contains ACTUAL UDP block numbers.
        #
        # We therefore search for the corresponding entries rather
        # than treating the array position as the block number.
        #

        first_pos = np.searchsorted(
            self.block_indices,
            first_possible_block,
            side="left",
        )

        last_pos = np.searchsorted(
            self.block_indices,
            last_possible_block,
            side="right",
        )

        return first_pos, last_pos

    # ------------------------------------------------------------------------
    # Automatic Y scaling
    # ------------------------------------------------------------------------

    def auto_scale_y(
        self,
        sample0,
        sample1,
    ):

        if self.y_mode != "auto":
            return

        if sample1 <= sample0:
            return

        if len(self.data) == 0:
            return

        #
        # We cannot simply use data[sample0:sample1] because that would
        # ignore missing blocks.
        #
        # Instead calculate the samples that actually exist in the
        # visible time range.
        #

        first_pos, last_pos = (
            self.visible_block_range()
        )

        if last_pos <= first_pos:
            return

        values_min = None
        values_max = None

        for pos in range(
            first_pos,
            last_pos,
        ):

            block_index = int(
                self.block_indices[pos]
            )

            block_start_sample = (
                block_index
                * AUDIO_BLOCK_SAMPLES
            )

            block_end_sample = (
                block_start_sample
                + AUDIO_BLOCK_SAMPLES
            )

            #
            # Convert from timeline sample coordinates to our compact
            # stored-data coordinates.
            #
            # Because data only contains RECEIVED blocks, the position
            # of this block in the data array is `pos`.
            #

            stored_start = (
                pos * AUDIO_BLOCK_SAMPLES
            )

            stored_end = min(
                stored_start
                + AUDIO_BLOCK_SAMPLES,
                len(self.data),
            )

            if stored_end <= stored_start:
                continue

            visible_start = max(
                block_start_sample,
                sample0,
            )

            visible_end = min(
                block_end_sample,
                sample1,
            )

            if visible_end <= visible_start:
                continue

            offset_start = (
                visible_start
                - block_start_sample
            )

            offset_end = (
                visible_end
                - block_start_sample
            )

            actual_start = (
                stored_start
                + offset_start
            )

            actual_end = min(
                stored_start
                + offset_end,
                stored_end,
            )

            if actual_end <= actual_start:
                continue

            visible = np.asarray(
                self.data[
                    actual_start:actual_end
                ]
            )

            if visible.size == 0:
                continue

            local_min = int(
                np.min(visible)
            )

            local_max = int(
                np.max(visible)
            )

            if values_min is None:
                values_min = local_min
                values_max = local_max
            else:
                values_min = min(
                    values_min,
                    local_min,
                )

                values_max = max(
                    values_max,
                    local_max,
                )

        if values_min is None:
            return

        if values_min == values_max:

            margin = max(
                1,
                int(
                    max(
                        abs(values_min),
                        1,
                    )
                    * 0.05
                ),
            )

        else:

            margin = max(
                1,
                int(
                    (values_max - values_min)
                    * 0.05
                ),
            )

        self.plot.setYRange(
            values_min - margin,
            values_max + margin,
            padding=0,
        )

    # ------------------------------------------------------------------------
    # Render
    # ------------------------------------------------------------------------

    def update(self):

        sample0, sample1 = (
            self.visible_sample_range()
        )

        current_range = (
            sample0,
            sample1,
        )

        if current_range == self._last_range:
            return

        self._last_range = current_range

        if (
            sample1 <= sample0
            or len(self.data) == 0
            or len(self.block_indices) == 0
        ):

            self.curve.clear()
            return

        first_pos, last_pos = (
            self.visible_block_range()
        )

        if last_pos <= first_pos:

            self.curve.clear()
            return

        x_parts = []
        y_parts = []

        for pos in range(
            first_pos,
            last_pos,
        ):

            block_index = int(
                self.block_indices[pos]
            )

            #
            # This is the CRITICAL FIX.
            #
            # The block's position on the timeline comes from its
            # actual UDP block_index, NOT from `pos`.
            #

            block_start_sample = (
                block_index
                * AUDIO_BLOCK_SAMPLES
            )

            block_end_sample = (
                block_start_sample
                + AUDIO_BLOCK_SAMPLES
            )

            #
            # Determine which part of this block is visible.
            #

            start_sample = max(
                block_start_sample,
                sample0,
            )

            end_sample = min(
                block_end_sample,
                sample1,
            )

            if end_sample <= start_sample:
                continue

            offset_start = (
                start_sample
                - block_start_sample
            )

            offset_end = (
                end_sample
                - block_start_sample
            )

            #
            # Position of the block inside the compact received-data
            # array.
            #

            stored_start = (
                pos * AUDIO_BLOCK_SAMPLES
                + offset_start
            )

            stored_end = min(
                pos * AUDIO_BLOCK_SAMPLES
                + offset_end,
                len(self.data),
            )

            if stored_end <= stored_start:
                continue

            values = np.asarray(
                self.data[
                    stored_start:stored_end
                ]
            )

            #
            # Exact timeline position.
            #

            indices = np.arange(
                start_sample,
                start_sample + len(values),
                dtype=np.int64,
            )

            times = (
                indices
                / self.sample_rate
            )

            x_parts.append(times)
            y_parts.append(values)

            #
            # ALWAYS break at the end of a received block.
            #
            # This means we never visually connect the last sample of
            # one UDP block to the first sample of another block.
            #

            x_parts.append(
                np.asarray([np.nan])
            )

            y_parts.append(
                np.asarray([np.nan])
            )

        if not x_parts:

            self.curve.clear()
            return

        x = np.concatenate(x_parts)
        y = np.concatenate(y_parts)

        self.curve.setData(
            x,
            y,
            connect="finite",
        )

        self.auto_scale_y(
            sample0,
            sample1,
        )

    # ------------------------------------------------------------------------
    # Y scaling modes
    # ------------------------------------------------------------------------

    def set_y_mode(self, mode):

        self.y_mode = mode

        if mode == "full":

            self.plot.setYRange(
                INT16_MIN,
                INT16_MAX,
                padding=0,
            )

        elif mode == "auto":

            self._last_range = None
            self.update()

    # ------------------------------------------------------------------------
    # Mouse cursor
    # ------------------------------------------------------------------------

    def mouse_moved(self, position):

        if not self.plot.sceneBoundingRect().contains(
            position
        ):
            return

        mouse_point = (
            self.plot
            .getViewBox()
            .mapSceneToView(position)
        )

        time_s = mouse_point.x()

        sample_index = int(
            np.rint(
                time_s * self.sample_rate
            )
        )

        if (
            sample_index < 0
            or len(self.data) == 0
        ):
            return

        #
        # Determine the UDP block containing this sample.
        #

        block_index = (
            sample_index
            // AUDIO_BLOCK_SAMPLES
        )

        offset = (
            sample_index
            % AUDIO_BLOCK_SAMPLES
        )

        #
        # Search for this ACTUAL block index.
        #

        pos = np.searchsorted(
            self.block_indices,
            block_index,
        )

        if (
            pos >= len(self.block_indices)
            or int(self.block_indices[pos])
            != block_index
        ):

            #
            # This sample belongs to a missing UDP block.
            #

            self.vline.setPos(
                sample_index
                / self.sample_rate
            )

            self.hline.setVisible(False)

            if self.on_sample is not None:

                self.on_sample(
                    self.channel,
                    sample_index,
                    sample_index / self.sample_rate,
                    None,
                )

            return

        stored_index = (
            pos * AUDIO_BLOCK_SAMPLES
            + offset
        )

        if stored_index >= len(self.data):
            return

        value = int(
            self.data[stored_index]
        )

        actual_time = (
            sample_index
            / self.sample_rate
        )

        self.vline.setPos(
            actual_time
        )

        self.hline.setVisible(True)

        self.hline.setPos(
            value
        )

        if self.on_sample is not None:

            self.on_sample(
                self.channel,
                sample_index,
                actual_time,
                value,
            )


# ============================================================================
# Combined waveform
# ============================================================================

class CombinedWaveformView:

    """
    Fifth plot containing all four channels.

    Each channel is positioned using its ACTUAL UDP block_index.

    Missing blocks therefore produce genuine holes in the time axis.
    """

    def __init__(
        self,
        plot,
        recording,
        sample_rate,
    ):

        self.plot = plot
        self.recording = recording
        self.sample_rate = sample_rate

        self.curves = []
        self.y_mode = "auto"
        self._last_range = None

        self.plot.setTitle(
            "All Channels"
        )

        self.plot.setLabel(
            "left",
            "int16",
        )

        self.plot.setLabel(
            "bottom",
            "time",
            units="s",
        )

        self.plot.showGrid(
            x=True,
            y=True,
            alpha=0.25,
        )

        self.plot.setMouseEnabled(
            x=True,
            y=False,
        )

        for channel in range(
            NCHANNELS
        ):

            curve = pg.PlotDataItem(
                pen=pg.mkPen(
                    color=CHANNEL_COLORS[channel],
                    width=1,
                ),
                symbol=None,
                connect="finite",
            )

            self.plot.addItem(curve)
            self.curves.append(curve)

        self.update()

    # ------------------------------------------------------------------------
    # Visible range
    # ------------------------------------------------------------------------

    def visible_sample_range(self):

        x0, x1 = self.plot.viewRange()[0]

        sample0 = max(
            0,
            int(np.floor(
                x0 * self.sample_rate
            )),
        )

        sample1 = max(
            sample0,
            int(np.ceil(
                x1 * self.sample_rate
            )) + 1,
        )

        return sample0, sample1

    # ------------------------------------------------------------------------
    # Render
    # ------------------------------------------------------------------------

    def update(self):

        sample0, sample1 = (
            self.visible_sample_range()
        )

        current_range = (
            sample0,
            sample1,
        )

        if current_range == self._last_range:
            return

        self._last_range = current_range

        for channel in range(
            NCHANNELS
        ):

            data = self.recording.samples(
                channel
            )

            blocks = self.recording.block_indices(
                channel
            )

            curve = self.curves[channel]

            if (
                len(data) == 0
                or len(blocks) == 0
            ):

                curve.clear()
                continue

            #
            # Find the actual UDP blocks that could overlap the
            # visible time range.
            #

            first_block = (
                sample0
                // AUDIO_BLOCK_SAMPLES
            )

            last_block = (
                (sample1 - 1)
                // AUDIO_BLOCK_SAMPLES
            )

            first_pos = np.searchsorted(
                blocks,
                first_block,
                side="left",
            )

            last_pos = np.searchsorted(
                blocks,
                last_block,
                side="right",
            )

            if last_pos <= first_pos:

                curve.clear()
                continue

            x_parts = []
            y_parts = []

            for pos in range(
                first_pos,
                last_pos,
            ):

                block_index = int(
                    blocks[pos]
                )

                block_start_sample = (
                    block_index
                    * AUDIO_BLOCK_SAMPLES
                )

                block_end_sample = (
                    block_start_sample
                    + AUDIO_BLOCK_SAMPLES
                )

                start_sample = max(
                    block_start_sample,
                    sample0,
                )

                end_sample = min(
                    block_end_sample,
                    sample1,
                )

                if end_sample <= start_sample:
                    continue

                offset_start = (
                    start_sample
                    - block_start_sample
                )

                offset_end = (
                    end_sample
                    - block_start_sample
                )

                stored_start = (
                    pos * AUDIO_BLOCK_SAMPLES
                    + offset_start
                )

                stored_end = min(
                    pos * AUDIO_BLOCK_SAMPLES
                    + offset_end,
                    len(data),
                )

                if stored_end <= stored_start:
                    continue

                values = np.asarray(
                    data[
                        stored_start:stored_end
                    ]
                )

                indices = np.arange(
                    start_sample,
                    start_sample + len(values),
                    dtype=np.int64,
                )

                times = (
                    indices
                    / self.sample_rate
                )

                x_parts.append(times)
                y_parts.append(values)

                #
                # Break connection between blocks.
                #

                x_parts.append(
                    np.asarray([np.nan])
                )

                y_parts.append(
                    np.asarray([np.nan])
                )

            if not x_parts:

                curve.clear()
                continue

            curve.setData(
                np.concatenate(x_parts),
                np.concatenate(y_parts),
                connect="finite",
            )

        #
        # Combined plot Y scaling.
        #

        if self.y_mode == "auto":

            global_min = None
            global_max = None

            for channel in range(
                NCHANNELS
            ):

                data = self.recording.samples(
                    channel
                )

                blocks = self.recording.block_indices(
                    channel
                )

                if len(blocks) == 0:
                    continue

                first_block = (
                    sample0
                    // AUDIO_BLOCK_SAMPLES
                )

                last_block = (
                    (sample1 - 1)
                    // AUDIO_BLOCK_SAMPLES
                )

                first_pos = np.searchsorted(
                    blocks,
                    first_block,
                    side="left",
                )

                last_pos = np.searchsorted(
                    blocks,
                    last_block,
                    side="right",
                )

                for pos in range(
                    first_pos,
                    last_pos,
                ):

                    block_index = int(
                        blocks[pos]
                    )

                    block_start = (
                        block_index
                        * AUDIO_BLOCK_SAMPLES
                    )

                    block_end = (
                        block_start
                        + AUDIO_BLOCK_SAMPLES
                    )

                    start = max(
                        block_start,
                        sample0,
                    )

                    end = min(
                        block_end,
                        sample1,
                    )

                    if end <= start:
                        continue

                    offset_start = (
                        start - block_start
                    )

                    offset_end = (
                        end - block_start
                    )

                    stored_start = (
                        pos
                        * AUDIO_BLOCK_SAMPLES
                        + offset_start
                    )

                    stored_end = min(
                        pos
                        * AUDIO_BLOCK_SAMPLES
                        + offset_end,
                        len(data),
                    )

                    if stored_end <= stored_start:
                        continue

                    visible = np.asarray(
                        data[
                            stored_start:stored_end
                        ]
                    )

                    if visible.size == 0:
                        continue

                    ymin = int(
                        np.min(visible)
                    )

                    ymax = int(
                        np.max(visible)
                    )

                    if global_min is None:
                        global_min = ymin
                        global_max = ymax
                    else:
                        global_min = min(
                            global_min,
                            ymin,
                        )

                        global_max = max(
                            global_max,
                            ymax,
                        )

            if global_min is not None:

                if global_min == global_max:

                    margin = max(
                        1,
                        int(
                            max(
                                abs(global_min),
                                1,
                            )
                            * 0.05
                        ),
                    )

                else:

                    margin = max(
                        1,
                        int(
                            (global_max - global_min)
                            * 0.05
                        ),
                    )

                self.plot.setYRange(
                    global_min - margin,
                    global_max + margin,
                    padding=0,
                )

    # ------------------------------------------------------------------------
    # Y modes
    # ------------------------------------------------------------------------

    def set_y_mode(self, mode):

        self.y_mode = mode

        if mode == "full":

            self.plot.setYRange(
                INT16_MIN,
                INT16_MAX,
                padding=0,
            )

        elif mode == "auto":

            self._last_range = None
            self.update()

# ============================================================================
# Viewer
# ============================================================================

class RecordingViewer(
    QtWidgets.QMainWindow
):

    def __init__(
        self,
        recording,
    ):

        super().__init__()

        self.recording = recording
        self.sample_rate = recording.sample_rate

        self.y_mode = "auto"

        self.setWindowTitle(
            "Sample-Accurate UDP Audio Viewer — "
            + recording.directory.name
        )

        self.resize(
            1600,
            1100,
        )

        central = QtWidgets.QWidget()

        self.setCentralWidget(
            central
        )

        layout = QtWidgets.QVBoxLayout(
            central
        )

        self.info_label = QtWidgets.QLabel(
            "A = automatic Y    "
            "F = full int16 Y    "
            "S = shared Y"
        )

        self.info_label.setStyleSheet(
            "font-family: monospace;"
            "font-size: 14px;"
        )

        layout.addWidget(
            self.info_label
        )

        graphics_layout = (
            pg.GraphicsLayoutWidget()
        )

        layout.addWidget(
            graphics_layout
        )

        self.plots = []
        self.waveforms = []

        # --------------------------------------------------------------------
        # Four channel plots
        # --------------------------------------------------------------------

        for channel in range(
            NCHANNELS
        ):

            plot = graphics_layout.addPlot(
                row=channel,
                col=0,
            )

            self.plots.append(
                plot
            )

            waveform = WaveformView(
                plot=plot,
                data=recording.samples(channel),
                block_indices=recording.block_indices(channel),
                sample_rate=self.sample_rate,
                channel=channel,
                on_sample=self.sample_selected,
                color=CHANNEL_COLORS[channel],
            )

            self.waveforms.append(
                waveform
            )

        # --------------------------------------------------------------------
        # Combined plot
        # --------------------------------------------------------------------

        combined_plot = (
            graphics_layout.addPlot(
                row=NCHANNELS,
                col=0,
            )
        )

        self.plots.append(
            combined_plot
        )

        self.combined_waveform = (
            CombinedWaveformView(
                plot=combined_plot,
                recording=recording,
                sample_rate=self.sample_rate,
            )
        )

        # --------------------------------------------------------------------
        # Synchronize X axes
        # --------------------------------------------------------------------

        for plot in self.plots[1:]:

            plot.setXLink(
                self.plots[0]
            )

        self.plots[0].getViewBox().sigXRangeChanged.connect(
            self.range_changed
        )

        self.show_full_recording()

    # ------------------------------------------------------------------------
    # Keyboard
    # ------------------------------------------------------------------------

    def keyPressEvent(
        self,
        event,
    ):

        key = event.key()

        if key == QtCore.Qt.Key.Key_A:

            self.set_y_mode("auto")
            return

        if key == QtCore.Qt.Key.Key_F:

            self.set_y_mode("full")
            return

        if key == QtCore.Qt.Key.Key_S:

            self.set_y_mode("shared")
            return

        super().keyPressEvent(
            event
        )

    # ------------------------------------------------------------------------
    # Y scaling
    # ------------------------------------------------------------------------

    def set_y_mode(
        self,
        mode,
    ):

        self.y_mode = mode

        if mode == "auto":

            for waveform in self.waveforms:

                waveform.set_y_mode(
                    "auto"
                )

            self.combined_waveform.set_y_mode(
                "auto"
            )

            self.info_label.setText(
                "Y mode: AUTO — "
                "each channel scales to its visible samples"
            )

        elif mode == "full":

            for waveform in self.waveforms:

                waveform.set_y_mode(
                    "full"
                )

            self.combined_waveform.set_y_mode(
                "full"
            )

            self.info_label.setText(
                "Y mode: FULL INT16 — "
                "-32768 .. 32767"
            )

        elif mode == "shared":

            x0, x1 = (
                self.plots[0].viewRange()[0]
            )

            sample0 = max(
                0,
                int(
                    np.floor(
                        x0 * self.sample_rate
                    )
                ),
            )

            sample1 = max(
                sample0,
                int(
                    np.ceil(
                        x1 * self.sample_rate
                    )
                ) + 1,
            )

            global_min = None
            global_max = None

            for channel in range(
                NCHANNELS
            ):

                data = (
                    self.recording.samples(
                        channel
                    )
                )

                blocks = (
                    self.recording.block_indices(
                        channel
                    )
                )

                for block_number in range(
                    len(blocks)
                ):

                    block_index = int(
                        blocks[
                            block_number
                        ]
                    )

                    actual_start = (
                        block_index
                        * AUDIO_BLOCK_SAMPLES
                    )

                    actual_end = (
                        actual_start
                        + AUDIO_BLOCK_SAMPLES
                    )

                    start = max(
                        actual_start,
                        sample0,
                    )

                    end = min(
                        actual_end,
                        sample1,
                    )

                    if end <= start:
                        continue

                    compact_start = (
                        block_number
                        * AUDIO_BLOCK_SAMPLES
                        + (
                            start
                            - actual_start
                        )
                    )

                    compact_end = (
                        block_number
                        * AUDIO_BLOCK_SAMPLES
                        + (
                            end
                            - actual_start
                        )
                    )

                    visible = np.asarray(
                        data[
                            compact_start:compact_end
                        ]
                    )

                    if visible.size == 0:
                        continue

                    ymin = int(
                        np.min(visible)
                    )

                    ymax = int(
                        np.max(visible)
                    )

                    if global_min is None:

                        global_min = ymin
                        global_max = ymax

                    else:

                        global_min = min(
                            global_min,
                            ymin,
                        )

                        global_max = max(
                            global_max,
                            ymax,
                        )

            if global_min is not None:

                if global_min == global_max:

                    margin = max(
                        1,
                        int(
                            max(
                                abs(global_min),
                                1,
                            )
                            * 0.05
                        ),
                    )

                else:

                    margin = max(
                        1,
                        int(
                            (
                                global_max
                                - global_min
                            )
                            * 0.05
                        ),
                    )

                for plot in self.plots:

                    plot.setYRange(
                        global_min - margin,
                        global_max + margin,
                        padding=0,
                    )

            for waveform in self.waveforms:

                waveform.y_mode = "shared"

            self.combined_waveform.y_mode = (
                "shared"
            )

            self.info_label.setText(
                "Y mode: SHARED — "
                "all plots use the same visible amplitude range"
            )

    # ------------------------------------------------------------------------
    # X range changed
    # ------------------------------------------------------------------------

    def range_changed(
        self,
        *args,
    ):

        if self.y_mode == "auto":

            for waveform in self.waveforms:

                waveform._last_range = None
                waveform.update()

            self.combined_waveform._last_range = None
            self.combined_waveform.update()

        elif self.y_mode == "shared":

            self.set_y_mode(
                "shared"
            )

        else:

            for waveform in self.waveforms:

                waveform._last_range = None
                waveform.update()

            self.combined_waveform._last_range = None
            self.combined_waveform.update()

    # ------------------------------------------------------------------------
    # Full recording
    # ------------------------------------------------------------------------

    def show_full_recording(self):

        max_sample = 0

        for channel in range(
            NCHANNELS
        ):

            blocks = (
                self.recording.block_indices(
                    channel
                )
            )

            if len(blocks) == 0:
                continue

            channel_last_sample = (
                (
                    int(blocks[-1])
                    + 1
                )
                * AUDIO_BLOCK_SAMPLES
            )

            max_sample = max(
                max_sample,
                channel_last_sample,
            )

        if max_sample == 0:
            return

        duration = (
            max_sample
            / self.sample_rate
        )

        self.plots[0].setXRange(
            0,
            duration,
            padding=0,
        )

        for waveform in self.waveforms:

            waveform._last_range = None
            waveform.update()

        self.combined_waveform._last_range = None
        self.combined_waveform.update()

    # ------------------------------------------------------------------------
    # Exact sample selection
    # ------------------------------------------------------------------------
    def sample_selected(
        self,
        channel,
        sample_index,
        time_s,
        value,
    ):

        block_index = (
            sample_index
            // AUDIO_BLOCK_SAMPLES
        )

        offset = (
            sample_index
            % AUDIO_BLOCK_SAMPLES
        )

        blocks = (
            self.recording.block_indices(
                channel
            )
        )

        #
        # Search by ACTUAL UDP block index.
        #

        pos = np.searchsorted(
            blocks,
            block_index,
        )

        if (
            pos < len(blocks)
            and int(blocks[pos]) == block_index
        ):

            actual_block_index = int(
                blocks[pos]
            )

            block_text = (
                f"block={actual_block_index:,}, "
                f"offset={offset}"
            )

        else:

            block_text = (
                f"block={block_index:,} MISSING"
            )

        #
        # Show exact sample values for all channels at this timeline
        # sample position.
        #

        all_values = []

        for c in range(NCHANNELS):

            channel_data = (
                self.recording.samples(c)
            )

            channel_blocks = (
                self.recording.block_indices(c)
            )

            c_block_index = (
                sample_index
                // AUDIO_BLOCK_SAMPLES
            )

            c_offset = (
                sample_index
                % AUDIO_BLOCK_SAMPLES
            )

            c_pos = np.searchsorted(
                channel_blocks,
                c_block_index,
            )

            if (
                c_pos < len(channel_blocks)
                and int(channel_blocks[c_pos])
                == c_block_index
            ):

                stored_index = (
                    c_pos
                    * AUDIO_BLOCK_SAMPLES
                    + c_offset
                )

                if stored_index < len(
                    channel_data
                ):

                    all_values.append(
                        f"C{c}="
                        f"{int(channel_data[stored_index])}"
                    )

                else:

                    all_values.append(
                        f"C{c}=---"
                    )

            else:

                all_values.append(
                    f"C{c}=MISSING"
                )

        if value is None:

            selected_text = (
                f"selected=C{channel} "
                f"MISSING"
            )

        else:

            selected_text = (
                f"selected=C{channel} "
                f"value={value:6d}"
            )

        self.info_label.setText(
            f"sample={sample_index:,}    "
            f"time={time_s:.12f} s    "
            f"{selected_text}    "
            f"{block_text}    "
            + "    ".join(all_values)
        )

# ============================================================================
# Argument parser
# ============================================================================

def make_parser():

    parser = argparse.ArgumentParser(
        description=(
            "Lossless Teensy UDP audio "
            "recorder and sample-accurate viewer."
        )
    )

    subparsers = parser.add_subparsers(
        dest="command",
        required=True,
    )

    record_parser = subparsers.add_parser(
        "record",
        help="Record UDP audio.",
    )

    record_parser.add_argument(
        "duration",
        type=float,
    )

    record_parser.add_argument(
        "--sample-rate",
        type=float,
        default=44100,
    )

    record_parser.add_argument(
        "--host",
        default=DEFAULT_HOST,
    )

    record_parser.add_argument(
        "--port",
        type=int,
        default=DEFAULT_PORT,
    )

    record_parser.add_argument(
        "-o",
        "--output",
        type=Path,
        default=Path("recordings"),
    )

    load_parser = subparsers.add_parser(
        "load",
        help="Display an existing recording.",
    )

    load_parser.add_argument(
        "directory",
        type=Path,
    )

    load_parser.add_argument(
        "--sample-rate",
        type=float,
        default=None,
    )

    load_parser.add_argument(
        "--summary",
        action="store_true",
    )

    return parser


# ============================================================================
# Main
# ============================================================================

def main():

    parser = make_parser()

    args = parser.parse_args()

    if args.command == "record":

        if args.duration <= 0:

            parser.error(
                "duration must be positive"
            )

        if args.sample_rate <= 0:

            parser.error(
                "sample rate must be positive"
            )

        recorder = Recorder(
            host=args.host,
            port=args.port,
            duration=args.duration,
            sample_rate=args.sample_rate,
            output_directory=args.output,
        )

        recording_directory = (
            recorder.record()
        )

        print()
        print(
            "Opening recording..."
        )

        recording = Recording(
            recording_directory
        )

        recording.print_summary()

        app = (
            QtWidgets.QApplication.instance()
        )

        if app is None:

            app = QtWidgets.QApplication(
                sys.argv
            )

        pg.setConfigOptions(
            antialias=False,
        )

        viewer = RecordingViewer(
            recording
        )

        viewer.show()

        sys.exit(
            app.exec()
        )

    if args.command == "load":

        if not args.directory.exists():

            parser.error(
                f"recording does not exist: "
                f"{args.directory}"
            )

        try:

            recording = Recording(
                args.directory
            )

        except RuntimeError as exc:

            parser.error(
                str(exc)
            )

        if args.sample_rate is not None:

            recording.sample_rate = (
                args.sample_rate
            )

        recording.print_summary()

        if args.summary:
            return

        app = (
            QtWidgets.QApplication.instance()
        )

        if app is None:

            app = QtWidgets.QApplication(
                sys.argv
            )

        pg.setConfigOptions(
            antialias=False,
        )

        viewer = RecordingViewer(
            recording
        )

        viewer.show()

        sys.exit(
            app.exec()
        )


if __name__ == "__main__":

    main()
