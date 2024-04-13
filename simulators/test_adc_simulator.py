import socket
import argparse


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="ADC Simulator")
    parser.add_argument("--port", type=int, default=0, help="UDP port number", required=True)
    args = parser.parse_args()

    assert args.port > 0

    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        addr = ("localhost", args.port)
        sock.bind(addr)
        print("Listening on", addr)
        while True:
            data, _ = sock.recvfrom(1024)
            print(data.decode(), end="")