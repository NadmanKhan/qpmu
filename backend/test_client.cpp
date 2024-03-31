
#include <cassert>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <iostream>
#include <array>

#include "defs.h"

int main(int argc, char const *argv[]) {

    assert(argc > 1);
    int port = atoi(argv[1]);

    int sockfd;
    struct sockaddr_in serv_addr;
    
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error" << '\n';
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    std::cout << "A\n";
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection failed" << '\n';
        return -1;
    }
    std::cout << "B\n";

    Result result;

    while (true) {
        if (recv(sockfd, &result, sizeof(Result), 0) < 0) {
            std::cerr << "Error receiving data" << '\n';
            return -1;
        }
        std::cout << "C\n";

        std::cout << "Seq No: " << result.seq_no << '\n';
        std::cout << "Frequency: " << result.frequency << '\n';
        for (int i = 0; i < n_signals; ++i) {
            std::cout << "Phasor " << i << ": " << result.phasors[i] << '\n';
        }
    }

    close(sockfd);

    return 0;
}