from adc import AnalogSignal, TimestampedADC
from typing import Literal, Callable, Union, Tuple, Iterable
import argparse
import math
import os
from pathlib import Path


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Run the ADC simulation")
    parser.add_argument("--no-gui", default=False, action="store_true", help="Run without GUI")
    parser.add_argument("--sampling-rate", "-r", type=int, default=1200, help="Sampling rate in Hz")
    parser.add_argument("--bits", "-b", type=int, default=8, help="ADC resolution in bits")
    parser.add_argument("--noise", "-z", type=float, default=0.25, help="Noise")
    parser.add_argument("--frequency", "-f", type=float, default=50, help="Signal frequency in Hz")
    parser.add_argument("--voltage", "-v", type=float, default=240, help="Maximum voltage in volts")
    parser.add_argument("--current", "-i", type=float, default=10, help="Maximum current in amperes")
    parser.add_argument("--phasediff", "-p", type=float, default=15, help="V-I phase difference in degrees")
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

    try:
        if not args.no_gui:
            # TODO: Run the GUI
            pass
        
        pipe_path = os.getenv("ADC_RPMSG_FILE")
        pipe_path = Path(pipe_path).absolute()
        print(f"Pipe path: {pipe_path}")
        if pipe_path is None:
            raise ValueError("ADC_RPMSG_FILE environment variable not set")
        
        if os.path.exists(pipe_path):
            os.remove(pipe_path)
        os.mkfifo(pipe_path)
        with open(pipe_path, "wb") as pipe:
            for sample in adc.stream():
                pipe.write(bytes(sample))

        for sample in adc.stream():
            print(sample)

    except Exception as e:
        print(f"Error: {e}")

