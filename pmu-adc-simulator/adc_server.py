# A simple server that sends data from the ADC simulator to a connected client.

import socket
import os
from adc_simulator import ADC

HOST = os.getenv("PMU_ADC_HOST", '-')
PORT = os.getenv("PMU_ADC_PORT", '-')


if __name__ == "__main__":
    if HOST == '-' or PORT == '-':
        print("Please set the PMU_ADC_HOST and PMU_ADC_PORT environment variables.")
        exit(1)

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server_socket:
        server_socket.bind((HOST, int(PORT)))
        server_socket.listen(2)

        print(f'QPMU ADC server listening on {HOST}:{PORT}...')

        adc = ADC()

        conn, addr = server_socket.accept()

        with conn:
            print("Connection from: " + str(addr))

            for sample in adc.samples():
                data = str(sample)

                print("Sending data: " + data)
                conn.send(data.encode())
