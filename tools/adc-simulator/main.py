#!/usr/bin/env python3

from adc import AnalogSignal, TimestampedADC
from contextlib import contextmanager
import argparse
import math
import os
from pathlib import Path
import sys
import csv

PRESAMPLED_DIR = (
    Path(__file__).parent.parent.parent / "data" / "2025-03-23" / "processed"
)


@contextmanager
def sample_writer_context(args):
    pipe_path = Path(args.pipe).absolute() if args.pipe else None
    if pipe_path:
        print(f"Pipe path: {pipe_path}")

        if os.path.exists(pipe_path):
            os.remove(pipe_path)
        os.mkfifo(pipe_path)

    if pipe_path:
        if args.binary:
            with open(pipe_path, "wb") as pipe:

                def write(s: TimestampedADC.Sample):
                    pipe.write(bytes(s))

        else:
            with open(pipe_path, "w") as pipe:

                def write(s: TimestampedADC.Sample):
                    pipe.write(f"{str(s)}\n")

    else:
        if args.binary:

            def write(s: TimestampedADC.Sample):
                sys.stdout.buffer.write(bytes(s))

        else:

            def write(s: TimestampedADC.Sample):
                sys.stdout.write(f"{str(s)}\n")

    yield write


def sample_stream(args):

    if args.presampled:
        path = PRESAMPLED_DIR / f"{args.presampled}.csv"

        if not path.exists():
            print(f"File {path} does not exist")
            sys.exit(1)

        with open(path, "r") as f:
            reader = csv.DictReader(f)
            presampled_data = [
                {
                    "time_delta": int(row["time_delta"]),
                    "ch0": int(row["ch0"]),
                    "ch1": int(row["ch1"]),
                    "ch2": int(row["ch2"]),
                    "ch3": int(row["ch3"]),
                    "ch4": int(row["ch4"]),
                    "ch5": int(row["ch5"]),
                }
                for row in reader
            ]

        yield from TimestampedADC.stream_from_presampled_data(presampled_data)

    else:
        signals = [
            # VA
            AnalogSignal(
                frequency_hz=args.frequency,
                amplitude=args.voltage,
                phase_rad=math.radians(0),
            ),
            # VB
            AnalogSignal(
                frequency_hz=args.frequency,
                amplitude=args.voltage,
                phase_rad=math.radians(120),
            ),
            # VC
            AnalogSignal(
                frequency_hz=args.frequency,
                amplitude=args.voltage,
                phase_rad=math.radians(240),
            ),
            # IA
            AnalogSignal(
                frequency_hz=args.frequency,
                amplitude=args.voltage,  # Because the signal is fed to the ADC
                phase_rad=math.radians(0 + args.phasediff),
            ),
            # IB
            AnalogSignal(
                frequency_hz=args.frequency,
                amplitude=args.voltage,  # Because the signal is fed to the ADC
                phase_rad=math.radians(120 + args.phasediff),
            ),
            # IC
            AnalogSignal(
                frequency_hz=args.frequency,
                amplitude=args.voltage,  # Because the signal is fed to the ADC
                phase_rad=math.radians(240 + args.phasediff),
            ),
        ]
        adc = TimestampedADC(
            resolution_bits=args.bits,
            sampling_rate_hz=args.sampling_rate,
            reference_voltage_v=args.voltage,
            channels=list(zip(signals, [args.noise] * len(signals))),
        )

        yield from adc.stream()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Run the ADC simulation")
    parser.add_argument(
        "--no-gui",
        default=False,
        action="store_true",
        help="Run without GUI",
    )
    parser.add_argument(
        "--sampling-rate",
        type=int,
        default=1200,
        help="Sampling rate in Hz",
    )
    parser.add_argument(
        "--bits",
        type=int,
        default=12,
        help="ADC resolution in bits",
    )
    parser.add_argument("--noise", type=float, default=0.1, help="Noise")
    parser.add_argument(
        "--frequency",
        "-f",
        type=float,
        default=49.5,
        help="Signal frequency in Hz",
    )
    parser.add_argument(
        "--voltage",
        "-v",
        type=float,
        default=240,
        help="Maximum voltage in volts",
    )
    parser.add_argument(
        "--current",
        "-i",
        type=float,
        default=10,
        help="Maximum current in amperes",
    )
    parser.add_argument(
        "--phasediff",
        "-p",
        type=float,
        default=15,
        help="V-I phase difference in degrees",
    )
    parser.add_argument(
        "--binary",
        "-b",
        default=False,
        action="store_true",
        help="Output binary data",
    )
    parser.add_argument(
        "--presampled",
        default=None,
        choices=[
            str(Path(name).with_suffix("")) for name in os.listdir(PRESAMPLED_DIR)
        ],
        type=str,
        help=f"Use presampled data from the {PRESAMPLED_DIR} directory",
    )
    parser.add_argument(
        "--pipe",
        default=None,
        type=str,
        help="Path to named pipe for output",
    )
    parser.add_help = True

    args = parser.parse_args()

    while True:
        try:
            with sample_writer_context(args) as write:
                for sample in sample_stream(args):
                    write(sample)

        except Exception as e:
            print(f"Error: {e}")
