import os
import subprocess
import socket
import sys
import configparser

config = configparser.ConfigParser()

config_file = './qpmu.ini'

if os.getenv('QPMU_CONFIG_PATH'):
    config_file = os.getenv('QPMU_CONFIG_PATH')

if not os.path.isfile(config_file):
    print('The config file does not exist. Please create a config file named qpmu.ini \
          in the current directory, or set the QPMU_CONFIG_PATH environment variable to the \
          path of the config file.')
    exit(1)

config.read(config_file)

HOST = config['adc_address']['host']
PORT = config['adc_address']['port']

if __name__ == "__main__":
    if not HOST or not PORT:
        print('Please write the adc_address/host and adc_address/port settings in the config file')
        exit(1)

    if len(sys.argv) < 2:
        print("Please provide the ADC program path and arguments.")
        exit(1)

    adc_process = subprocess.Popen(sys.argv[1:],
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
                