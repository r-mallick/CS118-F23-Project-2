#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"

int main() {
    int listen_sockfd, send_sockfd;
    struct sockaddr_in server_addr, client_addr_from, client_addr_to;
    struct packet buffer;
    socklen_t addr_size = sizeof(client_addr_from);
    int expected_seq_num = 0;
    int recv_len;
    struct packet ack_pkt;

    // Create a UDP socket for sending
    send_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (send_sockfd < 0) {
        perror("Could not create send socket");
        return 1;
    }

    // Create a UDP socket for listening
    listen_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (listen_sockfd < 0) {
        perror("Could not create listen socket");
        return 1;
    }

    // Configure the server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind the listen socket to the server address
    if (bind(listen_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(listen_sockfd);
        return 1;
    }

    // Configure the client address structure to which we will send ACKs
    memset(&client_addr_to, 0, sizeof(client_addr_to));
    client_addr_to.sin_family = AF_INET;
    client_addr_to.sin_addr.s_addr = inet_addr(LOCAL_HOST);
    client_addr_to.sin_port = htons(CLIENT_PORT_TO);

    // Open the target file for writing (always write to output.txt)
    FILE *fp = fopen("output.txt", "wb");

    // TODO: Receive file from the client and save it as output.txt

    while (1) {
        // Receive data packet from client
        recv_len = recvfrom(listen_sockfd, &buffer, sizeof(buffer), 0, NULL, NULL);

        // Check if packet has the expected sequence number
        if (buffer.seqnum != expected_seq_num) {
            // Packet with unexpected sequence number, send ACK for previous packet
            ack_pkt.acknum = expected_seq_num - 1;
            sendto(send_sockfd, &ack_pkt, sizeof(ack_pkt), 0, (struct sockaddr *)&client_addr_to, sizeof(client_addr_to));
            printf("Ack sent for previous packet\n");
        } else {
            // Write payload to file
            fwrite(buffer.payload, 1, buffer.length, fp);
            printf("Payload written\n");

            // Update expected sequence number
            expected_seq_num++;

            // Send acknowledgement for the received packet
            ack_pkt.acknum = buffer.seqnum;
            sendto(send_sockfd, &ack_pkt, sizeof(ack_pkt), 0, (struct sockaddr *)&client_addr_to, sizeof(client_addr_to));
            printf("Ack sent for current packet\n");

            // Check if it is the last packet
            if (buffer.last)
                break;
        }
    }

    printf("File received successfully.\n");
    

    fclose(fp);
    close(listen_sockfd);
    close(send_sockfd);
    return 0;
}
