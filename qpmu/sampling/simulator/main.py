from sampling import AnalogSignal, ADC, TimeSyncedSamples
import argparse
import math


def main():
    parser = argparse.ArgumentParser(description="Run the ADC simulation")
    parser.add_argument("--headless", action="store_true", help="Run without GUI")
    parser.add_argument("--sampling-rate", "-fs", type=int, default=1200, help="Sampling rate in Hz")
    parser.add_argument("--bits", "-b", type=int, default=8, help="ADC resolution in bits")
    parser.add_argument("--snr", "-snr", type=float, default=1, help="Signal-to-noise ratio")
    parser.add_argument("--frequency", "-f", type=float, default=1, help="Signal frequency in Hz")
    parser.add_argument("--voltage", "-v", type=float, default=1, help="Maximum voltage in volts")
    parser.add_argument("--current", "-i", type=float, default=1, help="Maximum current in amperes")
    parser.add_argument("--phasediff", "-pd", type=float, default=0, help="V-I phase difference in degrees")
    parser.add_argument("--host", type=str, default="localhost", help="Host address of recipient socket")
    parser.add_argument("--port", type=int, default=5000, help="Port number of recipient socket")
    parser.add_argument("--socket", type=str, default="udp", help="Socket type (tcp or udp)")
    parser.add_argument("--binary", action="store_true", help="Send binary data")
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
            amplitude=args.current,
            phase_rad=math.radians(0 + args.phasediff),
        ),
        # IB
        AnalogSignal(
            frequency_hz=args.frequency,
            amplitude=args.current,
            phase_rad=math.radians(120 + args.phasediff),
        ),
        # IC
        AnalogSignal(
            frequency_hz=args.frequency,
            amplitude=args.current,
            phase_rad=math.radians(240 + args.phasediff),
        ),
    ]

    adc = ADC(
        resolution_bits=args.bits,
        sampling_rate_hz=args.sampling_rate,
        signals=signals,
    )

    if args.headless:
        for sample in TimeSyncedSamples(adc):
            print(sample)

    else:
        import gui
