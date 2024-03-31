#include <cstdlib>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <iostream>
#include <iomanip>
#include <array>

#include "phasorestimator.h"
#include "defs.h"

using std::cout;

void readSample(ADCSample &sample, int sockfd, char *buffer, size_t buffer_size)
{
    int n = read(sockfd, buffer, buffer_size);
    if (n < 0) {
        std::cerr << "Error reading from socket" << '\n';
    }

    cout << buffer << '\n';

    // Parse buffer
    int index = 0;
    sample[index] = 0;
    bool flag_reading_number = false;

    for (int i = 0; i < n; ++i) {
        if (buffer[i] == '=') {
            flag_reading_number = true;
            continue;
        }
        if (flag_reading_number) {
            if (isdigit(buffer[i])) {
                sample[index] = sample[index] * 10 + (buffer[i] - '0');
            } else if (buffer[i] == ',') {
                flag_reading_number = false;
                ++index;
                if (index == (int)sample.size())
                    break;
                sample[index] = 0;
            }
        }
    }
}

int main(int argc, char const *argv[])
{
    // ===
    // Read in command line arguments
    // ===

    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <adc_port> <phasor_port>" << '\n';
        return -1;
    }

    const int adc_port = atoi(argv[1]);
    const int phasor_port = atoi(argv[2]);
    constexpr int buffer_size = 1024;

    // ===
    // Create a TCP server to write phasor measurements to
    // ===

    int phasor_sockfd;
    struct sockaddr_in phasor_sockaddr;
    socklen_t phasor_sockaddr_len = sizeof(phasor_sockaddr);

    // Create TCP socket
    if ((phasor_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation failed" << '\n';
    }

    // Configure server address
    phasor_sockaddr.sin_family = AF_INET;
    phasor_sockaddr.sin_addr.s_addr = INADDR_ANY;
    phasor_sockaddr.sin_port = htons(phasor_port);

    // Bind socket to server address
    if (bind(phasor_sockfd, (const struct sockaddr *)&phasor_sockaddr, sizeof(phasor_sockaddr))
        < 0) {
        std::cerr << "Error binding socket" << '\n';
        return -1;
    }

    // Listen for incoming connections
    if (listen(phasor_sockfd, 3) < 0) {
        std::cerr << "Error listening for incoming connections" << '\n';
        return -1;
    }

    // Accept incoming connection
    int phasor_client;
    if ((phasor_client =
                 accept(phasor_sockfd, (struct sockaddr *)&phasor_sockaddr, &phasor_sockaddr_len))
        < 0) {
        std::cerr << "Error accepting incoming connection" << '\n';
        return -1;
    }

    // ===
    // Connect to the TCP server of ADC data
    // ===

    int adc_sockfd;
    struct sockaddr_in adc_sockaddr;
    socklen_t adc_sockaddr_len = sizeof(adc_sockaddr);

    // Create TCP socket
    if ((adc_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation failed" << '\n';
    }

    // Configure server address
    adc_sockaddr.sin_family = AF_INET;
    adc_sockaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    adc_sockaddr.sin_port = htons(adc_port);

    // Connect to server
    if (connect(adc_sockfd, (struct sockaddr *)&adc_sockaddr, adc_sockaddr_len) < 0) {
        std::cerr << "Connection to server failed" << '\n';
        return -1;
    }

    // ===
    // Read ADC data and write to phasor server
    // ===

    char buffer[buffer_size];
    ADCSample samples[n_samples];
    Result results[n_samples];
    PhasorEstimator estimator(n_samples);
    int index = 0;

    cout << std::fixed << std::setprecision(3);
    while (true) {
        readSample(samples[index], adc_sockfd, buffer, buffer_size);

        // Estimate phasors

        // seq_no
        results[index].seq_no = samples[index][0];

        // phasors
        for (int i = 0; i < n_signals; ++i) {
            float max_value = 0;
            for (int j = 0; j < n_samples; ++j) {
                max_value = std::max(max_value, (float)(samples[j][1 + i]));
            }
            auto half_max_value = max_value / 2;
            for (int j = 0, k = index + 1; j < n_samples; ++j, k = (k + 1) % n_samples) {
                estimator.set(i, j, (float)(samples[k][1 + i]) - half_max_value);
            }
        }
        // for (int j = 0; j < n_samples; ++j) {
        //     for (int i = 0; i < n_signals; ++i) {
        //         cout << std::right << std::setw(10) << estimator.get(i, j) << " ";
        //     }
        //     cout << '\n';
        // }
        // for (int i = 0; i < n_signals; ++i) {
        //     results[index].phasors[i] = estimator.phasorEstimation(i);
        // }

        // frequency
        {
            float sum = 0; // sum of (delta(phase) / delta(time)) for all signals and samples
            for (int i = 0; i < n_signals; ++i) {
                int j = index;
                int k = (index - 1 + n_samples) % n_samples;
                auto phase_diff =
                        std::abs(std::arg(results[j].phasors[i]) - std::arg(results[k].phasors[i]));
                sum += phase_diff / samples[j][8];
            }
            auto avg = sum / n_signals;
            results[index].frequency =
                    avg * (1e6 / (2 * M_PI)); // `* 1e6` to convert from micros to s
        }

        cout << "Seq No: " << results[index].seq_no << '\n';
        cout << "Frequency: " << results[index].frequency << '\n';
        for (int i = 0; i < n_signals; ++i) {
            cout << "Phasor " << i << ": " << results[index].phasors[i]
                 << " == " << std::abs(results[index].phasors[i]) << " < "
                 << (std::arg(results[index].phasors[i]) * 180 / M_PI) << '\n';
        }
        cout << "---\n";

        // Write to phasor server
        write(phasor_client, &results[index], sizeof(Result));

        index = (index + 1) % n_samples;
    }

    close(phasor_sockfd);
    close(adc_sockfd);
    return 0;
}
