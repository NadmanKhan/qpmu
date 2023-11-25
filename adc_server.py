import subprocess
import socket
import os
import sys


HOST = os.getenv("PMU_ADC_HOST", '-')
PORT = os.getenv("PMU_ADC_PORT", '-')

if __name__ == "__main__":
    if HOST == '-' or PORT == '-':
        print("Please set the PMU_ADC_HOST and PMU_ADC_PORT environment variables.")
        exit(1)

    if len(sys.argv) != 2:
        print("Please provide the path to the ADC process.")
        exit(1)
    
    adc_process_path = sys.argv[1]
    adc_process = subprocess.Popen(adc_process_path,
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server_socket:
        server_socket.bind((HOST, int(PORT)))
        server_socket.listen(2)

        print(f'QPMU ADC server listening on {HOST}:{PORT}...')

        conn, addr = server_socket.accept()

        with conn:
            print("Connection from: " + str(addr))

            while True:
                line = adc_process.stdout.readline()
                if not line:
                    break

                data = line.decode().strip()

                print("Sending data: " + data)
                conn.send(data.encode())
