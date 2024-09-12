from sampling import AnalogSignal, ADC, Sample, TimeSyncedSamples
from typing import Literal, Callable, Union, Tuple
import argparse
import math
import sys
import socket

WriterFunc = Callable[[Sample], None]
WriterFuncMaker = Callable[[None], WriterFunc]


def make_writer_maker(
    adc: ADC, binary: bool = False, sockconfig: Union[Tuple[str, int, Literal["tcp", "udp"]], None] = None
) -> Union[Tuple[WriterFuncMaker, socket.socket, None]]:

    assert isinstance(adc, ADC), "ADC must be an instance of ADC"
    assert isinstance(binary, bool), "Binary must be a boolean"
    if sockconfig is not None:
        assert isinstance(sockconfig, tuple), "Sockconfig must be a tuple"
        assert len(sockconfig) == 3, "Sockconfig must have 3 elements"
        assert isinstance(sockconfig[0], str), "Host must be a string"
        assert isinstance(sockconfig[1], int), "Port must be an integer"
        assert isinstance(sockconfig[2], str), "Socktype must be a string"
        assert sockconfig[0] != "", "Host must not be empty"
        assert 0 <= sockconfig[1] <= 65535, "Port must be between 1 and 65535"
        assert sockconfig[2] in ["tcp", "udp"], "Socktype must be tcp or udp"

    if sockconfig is None:
        if binary:

            def write(sample: Sample) -> None:
                sys.stdout.buffer.write(bytes(sample))

        else:

            def write(sample: Sample) -> None:
                print(str(sample))

        return (lambda: lambda sample: write(sample)), None

    # If sockconfig is not None

    (host, port, socktype) = sockconfig
    addr = (host, port)
    tobytes: Callable[[Sample], bytes] = (
        (lambda sample: bytes(sample)) if binary else (lambda sample: str(sample).encode())
    )

    sock: socket.socket = None

    if socktype == "tcp":

        def make_writer() -> WriterFunc:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

            sock.connect(addr)

            print(f"Connecting to {host}:{port} using {socktype}", file=sys.stderr)
            print(f"Socket: {sock}", file=sys.stderr)

            def write(sample: Sample) -> None:
                try:
                    sock.sendall(tobytes(sample))
                    print(f'Sent \n"{sample}"\n to sock={sock}, addr={addr}', file=sys.stderr)
                except OSError as e:
                    print(f"Error during `sendall` on sock={sock}: {e}; addr={addr}", file=sys.stderr)

            return write

    else:  # socktype == "udp"

        def make_writer() -> WriterFunc:
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

            print(f"Connecting to {host}:{port} using {socktype}", file=sys.stderr)
            print(f"Socket: {sock}", file=sys.stderr)

            def write(sample: Sample) -> None:
                try:
                    sock.sendto(tobytes(sample), addr)
                    print(f'Sent \n"{sample}"\n to sock={sock}, addr={addr}', file=sys.stderr)
                except OSError as e:
                    print(f"Error during `sendto` on sock={sock}: {e}; addr={addr}", file=sys.stderr)

            return write

    return make_writer, sock


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Run the ADC simulation")
    parser.add_argument("--no-gui", default=False, action="store_true", help="Run without GUI")
    parser.add_argument("--sampling-rate", "-s", type=int, default=1200, help="Sampling rate in Hz")
    parser.add_argument("--bits", "-b", type=int, default=8, help="ADC resolution in bits")
    parser.add_argument("--noise", "-z", type=float, default=0.25, help="Noise")
    parser.add_argument("--frequency", "-f", type=float, default=50, help="Signal frequency in Hz")
    parser.add_argument("--voltage", "-v", type=float, default=1, help="Maximum voltage in volts")
    parser.add_argument("--current", "-i", type=float, default=1, help="Maximum current in amperes")
    parser.add_argument("--phasediff", "-p", type=float, default=15, help="V-I phase difference in degrees")
    parser.add_argument("--host", "-a", type=str, default="127.0.0.1", help="Host address of recipient socket")
    parser.add_argument("--port", "-n", type=int, default=12345, help="Port number of recipient socket")
    parser.add_argument(
        "--socktype", "-t", type=str, default="none", required=False, help="Socket type (tcp or udp or none)"
    )
    parser.add_argument("--binary", default=False, action="store_true", help="Send binary data")
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
        noises=[args.noise] * len(signals),
    )

    sockconfig = None
    if args.socktype in ["tcp", "udp"]:
        sockconfig = (args.host, args.port, args.socktype)

    writer_maker, sock = make_writer_maker(adc, args.binary, sockconfig=sockconfig)

    try:
        if args.no_gui:
            write = writer_maker()
            for sample in TimeSyncedSamples(adc):
                write(sample)

        else:
            pass

    except Exception as e:
        print(f"Error: {e}")

    finally:
        if sock is not None:
            sock.close()
