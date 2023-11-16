'''
A simple server that sends data from the ADC simulator to a connected client.
'''

import socket
import os
from adc_simulator import ADC


HOST = os.getenv("PMU_ADC_SIMULATOR_HOST", '127.0.0.1')
PORT = os.getenv("PMU_ADC_SIMULATOR_PORT", 12345)


if __name__ == "__main__":
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server_socket:
        server_socket.bind((HOST, PORT))
        server_socket.listen(2)

        adc = ADC()

        conn, addr = server_socket.accept()

        with conn:
            print("Connection from: " + str(addr))

            for sample in adc.samples():
                data = str(sample)

                print("Sending data: " + data)
                conn.send(data.encode())
