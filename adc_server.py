import os
import subprocess
import socket
import sys
import configparser

config = configparser.ConfigParser()

config_file = './qpmu.ini'

if os.getenv('QPMU_CONFIG'):
    config_file = os.getenv('QPMU_CONFIG')

if not os.path.isfile(config_file):
    print('Please create a qpmu.ini file.')
    exit(1)

config.read(config_file)

HOST = config['adc_address']['host']
PORT = config['adc_address']['port']

if __name__ == "__main__":
    if not HOST or not PORT:
        print('Please write the adc/host and adc/port settings in settings.ini.')
        exit(1)

    if len(sys.argv) < 2:
        print("Please provide the ADC program path and arguments.")
        exit(1)

    adc_process = subprocess.Popen(sys.argv[1:],
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)

    while 1:
        try:
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
        except:
            print("Connection error. Retrying...")
            continue