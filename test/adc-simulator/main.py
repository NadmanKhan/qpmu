from adc import AnalogSignal, TimestampedADC
from typing import Literal, Callable, Union, Tuple, Iterable
import argparse
import math
import os
from pathlib import Path
import sys


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Run the ADC simulation")
    parser.add_argument("--no-gui", default=False, action="store_true", help="Run without GUI")
    parser.add_argument("--sampling-rate", "-r", type=int, default=1200, help="Sampling rate in Hz")
    parser.add_argument("--bits", "-b", type=int, default=12, help="ADC resolution in bits")
    parser.add_argument("--noise", "-z", type=float, default=0.1, help="Noise")
    parser.add_argument("--frequency", "-f", type=float, default=49.5, help="Signal frequency in Hz")
    parser.add_argument("--voltage", "-v", type=float, default=240, help="Maximum voltage in volts")
    parser.add_argument("--current", "-i", type=float, default=10, help="Maximum current in amperes")
    parser.add_argument("--phasediff", "-p", type=float, default=15, help="V-I phase difference in degrees")
    parser.add_argument("--binary", "-B", default=False, action="store_true", help="Output binary data")
    parser.add_argument("--raw", "-R", default=False, action="store_true", help="Output raw data")
    parser.add_help = True

    args = parser.parse_args()

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

    while True:
        try:
            if not args.no_gui:
                # TODO: Run the GUI
                pass
            
            pipe_path = os.getenv("ADC_STREAM")

            if not pipe_path:
                if args.binary:
                    if args.raw:
                        for rawbuf in adc.stream_raw():
                            sys.stdout.buffer.write(bytes(rawbuf))
                    else:
                        for sample in adc.stream_samples():
                            sys.stdout.buffer.write(bytes(sample))
                else:
                    if args.raw:
                        for rawbuf in adc.stream_raw():
                            print(rawbuf)
                    else:
                        for sample in adc.stream_samples():
                            print(sample)
            
            else:
                pipe_path = Path(pipe_path).absolute()
                print(f"Pipe path: {pipe_path}")
                
                if os.path.exists(pipe_path):
                    os.remove(pipe_path)
                os.mkfifo(pipe_path)

                if args.binary:
                    with open(pipe_path, "wb") as pipe:
                        if args.raw:
                            for rawbuf in adc.stream_raw():
                                pipe.write(bytes(rawbuf))
                        else:
                            for sample in adc.stream_samples():
                                pipe.write(bytes(sample))
                
                else:
                    with open(pipe_path, "w") as pipe:
                        if args.raw:
                            for rawbuf in adc.stream_raw():
                                pipe.write(f"{rawbuf}\n")
                        else:
                            for sample in adc.stream_samples():
                                pipe.write(f"{sample}\n")

        except Exception as e:
            print(f"Error: {e}")

