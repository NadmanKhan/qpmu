# A simple client to test the ADC server.

import socket
from adc_server import HOST, PORT


if __name__ == "__main__":
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as client_socket:
        client_socket.connect((HOST, PORT))

        while True:
            data = client_socket.recv(1024)

            if not data:
                break

            print("Received data: " + data.decode())
