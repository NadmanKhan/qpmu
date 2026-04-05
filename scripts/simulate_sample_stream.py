#!/usr/bin/env python3

from __future__ import annotations

import argparse
import csv
import json
from enum import Enum
from pathlib import Path
import time
import struct
import sys
from dataclasses import dataclass
from itertools import cycle
from types import TracebackType
from typing import TextIO, BinaryIO


@dataclass
class SampleFrame:
    timestamp_nsec: int
    channel_0: int
    channel_1: int
    channel_2: int
    channel_3: int
    channel_4: int
    channel_5: int


SAMPLE_FRAME_FIELDNAMES = [
    "timestamp_nsec",
    "channel_0",
    "channel_1",
    "channel_2",
    "channel_3",
    "channel_4",
    "channel_5",
]


class Config:
    class Format(str, Enum):
        JSON = "json"
        CSV = "csv"
        BINARY = "binary"

    def __init__(self):
        parser = argparse.ArgumentParser(
            description="Samples Streamer Configuration",
        )
        parser.add_argument(
            "input_path",
            metavar="INPUT_PATH",
            type=str,
            help="Path to the CSV file containing the sample frames to stream",
        )
        parser.add_argument(
            "--output-path",
            "-o",
            type=str,
            default=None,
            help="Path to stream the sample frames to (default: stdout)",
        )
        parser.add_argument(
            "--output-format",
            "-f",
            type=str,
            choices=[fmt.value for fmt in self.Format],
            default="csv",
            help="Output format for the streamed sample frames (default: csv)",
        )
        parser.add_argument(
            "--enable-logging",
            "-l",
            action="store_true",
            help="Enable logging of the streaming process",
        )
        args = parser.parse_args()
        self.input_path = Path(args.input_path)
        self.output_path = Path(args.output_path) if args.output_path else None
        self.output_format = Config.Format(args.output_format)
        self.enable_logging = bool(args.enable_logging)

        # Validate: CSV file
        if not self.input_path.is_file():
            raise FileNotFoundError(f"Sample frames file not found: {self.input_path}")
        if not self.input_path.suffix == ".csv":
            raise ValueError("Sample frames file must be a CSV file")

        with open(self.input_path, "r") as f:
            # Validate: Correct columns, all integers
            reader = csv.DictReader(f)
            if not reader.fieldnames:
                raise ValueError("Sample frames file has no header")
            if set(reader.fieldnames) != set(SAMPLE_FRAME_FIELDNAMES):
                raise ValueError(
                    "Sample frames file must have the following columns: "
                    + ", ".join(SAMPLE_FRAME_FIELDNAMES)
                )
            row_count = 0
            for row in reader:
                row_count += 1
                for name in SAMPLE_FRAME_FIELDNAMES:
                    try:
                        int(row[name])
                    except ValueError:
                        raise ValueError(f"All sample frame values must be integers, found invalid value in column '{name}': {row[name]}")

            # Validate: At least one sample frame
            if row_count == 0:
                raise ValueError(
                    "Sample frames file must contain at least one sample frame"
                )


class SampleFrameWriter:
    def __init__(
        self,
        format: Config.Format,
        path: Path | None = None,
        logging: bool = False,
    ):
        self.path = path
        self.format = format
        self.logging = logging
        self.file: TextIO | BinaryIO | None = None

        def write_stub(sf: SampleFrame):
            pass

        self._write = write_stub

    def write(self, sf: SampleFrame):
        self._write(sf)
        if self.logging:
            print(sf, file=sys.stderr)

    def __enter__(self):
        if self.format == Config.Format.CSV:
            csv_file = self.file = (
                open(self.path, "w", newline="") if self.path else sys.stdout
            )

            csv_writer = csv.DictWriter(csv_file, fieldnames=SAMPLE_FRAME_FIELDNAMES)
            csv_writer.writeheader()

            def write_csv(sf: SampleFrame):
                csv_writer.writerow(sf.__dict__)
                csv_file.flush()

            self._write = write_csv

        elif self.format == Config.Format.JSON:
            json_file = self.file = open(self.path, "w") if self.path else sys.stdout

            def write_json(sf: SampleFrame):
                json_file.write(json.dumps(sf.__dict__) + "\n")
                json_file.flush()

            self._write = write_json

        elif self.format == Config.Format.BINARY:
            # C++ Read_Buffer expects: 1 int64 timestamp + 180 uint16 samples (30 per channel)
            struct_format = "@q180H"
            binary_file = self.file = (
                open(self.path, "wb") if self.path else sys.stdout.buffer
            )

            def write_binary(sf: SampleFrame):
                # Repeat each channel's sample 30 times to match C++ Read_Buffer layout
                sample_array = [
                    sf.channel_0,
                    sf.channel_1,
                    sf.channel_2,
                    sf.channel_3,
                    sf.channel_4,
                    sf.channel_5,
                ]
                b = struct.pack(struct_format, sf.timestamp_nsec, *(sample_array * 30))
                binary_file.write(b)
                binary_file.flush()

            self._write = write_binary

        else:
            raise NotImplementedError(f"Format {self.format} not implemented")

        return self

    def __exit__(
            self,
            exc_type: type[BaseException] | None,
            exc_value: BaseException | None,
            traceback: TracebackType | None,
        ):
        if self.logging:
            print("Closing SampleFrameWriter", file=sys.stderr)
        try:
            if (
                self.path and self.file and not self.file.closed
            ):  # Only close if it's a file we opened
                self.file.close()
        except BrokenPipeError:
            pass
        # Only suppress BrokenPipeError, allow KeyboardInterrupt and other exceptions to propagate
        return exc_type is BrokenPipeError


def stream_sample_frames(input_path: Path):
    sample_frames: list[SampleFrame] = []
    with open(input_path, "r") as f:
        for row in csv.DictReader(f):
            frame = SampleFrame(
                timestamp_nsec=int(row["timestamp_nsec"]),
                channel_0=int(row["channel_0"]),
                channel_1=int(row["channel_1"]),
                channel_2=int(row["channel_2"]),
                channel_3=int(row["channel_3"]),
                channel_4=int(row["channel_4"]),
                channel_5=int(row["channel_5"]),
            )
            sample_frames.append(frame)

    # Timestamps should be monotonically increasing
    if sample_frames != sorted(sample_frames, key=lambda r: r.timestamp_nsec):
        raise ValueError("Sample-frame timestamps must be monotonically increasing")

    # Calculate the interval between last and first frame (for wrapping)
    # Assume same interval as average interval in the data
    average_interval_nsec = (
        sample_frames[-1].timestamp_nsec - sample_frames[0].timestamp_nsec
    ) // (len(sample_frames) - 1)


    sample_frames_with_interval: list[tuple[SampleFrame, int]] = []
    for i in range(len(sample_frames)):
        sf1 = sample_frames[i]
        sf2 = sample_frames[(i + 1) % len(sample_frames)]  # Wrap to first frame after last
        interval = sf2.timestamp_nsec - sf1.timestamp_nsec
        if interval <= 0:
            raise ValueError("Sample-frame timestamps must be strictly increasing")
        sample_frames_with_interval.append((sf1, interval))
        
    # Start 0.1 second in the future
    next_output_time_nsec = time.time_ns() + 10**8

    for frame, interval in cycle(sample_frames_with_interval):
        # Sleep until it's time to output this frame
        sleep_duration_nsec = next_output_time_nsec - time.time_ns()
        if sleep_duration_nsec > 0:
            time.sleep(
                (sleep_duration_nsec / 1e9) * 0.999
            )  # Convert nsec to sec, adjust for accuracy

        # Output frame with the scheduled timestamp
        yield SampleFrame(**(frame.__dict__ | dict(timestamp_nsec=next_output_time_nsec)))

        next_output_time_nsec += interval


if __name__ == "__main__":
    config = Config()
    try:
        while True:
            with SampleFrameWriter(
                format=config.output_format,
                path=config.output_path,
                logging=config.enable_logging,
            ) as writer:
                for sf in stream_sample_frames(config.input_path):
                    writer.write(sf)
    except (KeyboardInterrupt, EOFError):
        if config.enable_logging:
            print("\nAborted by user", file=sys.stderr)
        sys.exit(0)
