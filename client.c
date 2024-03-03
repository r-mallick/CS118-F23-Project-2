#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "utils.h"

int receive_ack(int sockfd, struct sockaddr *server_addr, socklen_t addr_size, struct packet *pkt) {
    ssize_t bytes_received = recvfrom(sockfd, pkt, sizeof(struct packet), 0, server_addr, &addr_size);
    if (bytes_received < 0) {
        perror("Error receiving acknowledgment");
        return -1;
    }
    printf("Received acknowledgment with ACK number %d\n", pkt->ack_num);
    return 0;
}
int send_packet(int sockfd, struct sockaddr *server_addr, socklen_t addr_size, struct packet *pkt) {
    ssize_t bytes_sent = sendto(sockfd, pkt, sizeof(struct packet), 0, server_addr, addr_size);
    if (bytes_sent < 0) {
        perror("Error sending packet");
        return -1;
    }
    printf("Sent packet with sequence number %d\n", pkt->seq_num);
    return 0;
}

int main(int argc, char *argv[]) {
    int listen_sockfd, send_sockfd;
    struct sockaddr_in client_addr, server_addr_to, server_addr_from;
    socklen_t addr_size = sizeof(server_addr_to);
    struct timeval tv;
    struct packet pkt;
    struct packet ack_pkt;
    char buffer[PAYLOAD_SIZE];
    unsigned short seq_num = 0;
    unsigned short ack_num = 0;
    char last = 0;
    char ack = 0;

    // read filename from command line argument
    if (argc != 2) {
        printf("Usage: ./client <filename>\n");
        return 1;
    }
    char *filename = argv[1];

    // Create a UDP socket for listening
    listen_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (listen_sockfd < 0) {
        perror("Could not create listen socket");
        return 1;
    }

    // Create a UDP socket for sending
    send_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (send_sockfd < 0) {
        perror("Could not create send socket");
        return 1;
    }

    // Configure the server address structure to which we will send data
    memset(&server_addr_to, 0, sizeof(server_addr_to));
    server_addr_to.sin_family = AF_INET;
    server_addr_to.sin_port = htons(SERVER_PORT_TO);
    server_addr_to.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Configure the client address structure
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(CLIENT_PORT);
    client_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind the listen socket to the client address
    if (bind(listen_sockfd, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
        perror("Bind failed");
        close(listen_sockfd);
        return 1;
    }

    // Open file for reading
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        perror("Error opening file");
        close(listen_sockfd);
        close(send_sockfd);
        return 1;
    }

    // TODO: Read from file, and initiate reliable data transfer to the server

    while (!feof(fp)) {
        // Read data from file
        size_t bytes_read = fread(buffer, 1, PAYLOAD_SIZE, fp);
        if (bytes_read == 0) {
            if (feof(fp))
                break; // End of file reached
            else {
                perror("Error reading from file");
                break;
            }
        }

        // Create packet
        build_packet(pkt, seqnum, acknum, last, ack, bytes_read, buffer);
        memset(&pkt, 0, sizeof(pkt));
        pkt.seqnum = seq_num;
        pkt.acknum = ack_num;
        memcpy(pkt.payload, buffer, bytes_read);

        // Send packet
        if (send_packet(send_sockfd, (struct sockaddr *)&server_addr_to, addr_size, &pkt) < 0) {
            fclose(fp);
            close(listen_sockfd);
            close(send_sockfd);
            return 1;
        }

        // Receive acknowledgment
        struct packet ack_pkt;
        if (receive_ack(listen_sockfd, (struct sockaddr *)&server_addr_from, addr_size, &ack_pkt) < 0) {
            fclose(fp);
            close(listen_sockfd);
            close(send_sockfd);
            return 1;
        }

        // Handle acknowledgment
        if (ack_pkt.acknum == seq_num) {
            // Acknowledgment received for the sent packet
            printf("Acknowledgment received for sequence number %d\n", seq_num);
            seq_num++; // Update sequence number for next packet
        } else {
            // Acknowledgment received is not for the sent packet
            printf("Invalid acknowledgment received\n");
            // Handle retransmission here if necessary
        }
        // Wait for acknowledgment
        // You need to implement acknowledgment handling here
        // Receive an acknowledgment from the server
        // Ensure the acknowledgment corresponds to the sent packet
        // If acknowledgment received is correct, update sequence number and continue sending
        // Otherwise, resend the packet
        seq_num++;
        
    }
    fclose(fp);
    close(listen_sockfd);
    close(send_sockfd);
    return 0;
}

