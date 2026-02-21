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
    class Format(Enum):
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
            for row in reader:
                if not all(row[name].isdigit() for name in SAMPLE_FRAME_FIELDNAMES):
                    raise ValueError("All sample frame values must be integers")

            # Validate: At least one sample frame
            f.seek(0)
            if not any(True for _ in reader):
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
                sample_vector = [
                    sf.channel_0,
                    sf.channel_1,
                    sf.channel_2,
                    sf.channel_3,
                    sf.channel_4,
                    sf.channel_5,
                ]
                b = struct.pack(struct_format, sf.timestamp_nsec, *(sample_vector * 30))
                binary_file.write(b)
                binary_file.flush()

            self._write = write_binary

        else:
            raise NotImplementedError(f"Format {self.format} not implemented")
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        if self.logging:
            print("Closing SampleFrameWriter", file=sys.stderr)
        try:
            if (
                self.path and self.file and not self.file.closed
            ):  # Only close if it's a file we opened
                self.file.close()
        except BrokenPipeError:
            pass
        return True  # Suppress exceptions


def stream_sample_frames(input_path: Path):
    sample_frames: list[SampleFrame] = []
    with open(input_path, "r") as f:
        for row in csv.DictReader(f):
            sf = SampleFrame(
                timestamp_nsec=int(row["timestamp_nsec"]),
                channel_0=int(row["channel_0"]),
                channel_1=int(row["channel_1"]),
                channel_2=int(row["channel_2"]),
                channel_3=int(row["channel_3"]),
                channel_4=int(row["channel_4"]),
                channel_5=int(row["channel_5"]),
            )
            sample_frames.append(sf)

    # Timestamps should be monotonically increasing
    if sample_frames != sorted(sample_frames, key=lambda r: r.timestamp_nsec):
        raise ValueError("Readings timestamps must be monotonically increasing")

    average_interval_nsec = (
        sample_frames[-1].timestamp_nsec - sample_frames[0].timestamp_nsec
    ) // (len(sample_frames) - 1)

    while True:
        prev_frame_timestamp_nsec = (
            sample_frames[0].timestamp_nsec - average_interval_nsec
        )
        prev_output_time_nsec = time.time_ns()

        for sf in sample_frames:
            # Calculate output time based on previous timestamps
            output_time_nsec = prev_output_time_nsec + (
                sf.timestamp_nsec - prev_frame_timestamp_nsec
            )

            # Update previous timestamps
            prev_frame_timestamp_nsec = sf.timestamp_nsec
            prev_output_time_nsec = output_time_nsec

            # Sleep if necessary
            sleep_duration_nsec = output_time_nsec - time.time_ns()
            if sleep_duration_nsec > 0:
                time.sleep(
                    (sleep_duration_nsec / 1e9) * 0.999
                )  # Convert nsec to sec, adjust for accuracy

            time_adjusted_reading = SampleFrame(**sf.__dict__)
            time_adjusted_reading.timestamp_nsec = output_time_nsec
            yield time_adjusted_reading


if __name__ == "__main__":
    config = Config()
    while True:
        with SampleFrameWriter(
            format=config.output_format,
            path=config.output_path,
            logging=config.enable_logging,
        ) as writer:
            for reading in stream_sample_frames(config.input_path):
                writer.write(reading)
