/*
 * Ex4: System Programming
 * Client Program (Simulation)
 * Creates 5 threads to connect to the server simultaneously.
 * Each thread sends a message, waits for the echo response, and prints it.
 * Usage: ./client
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1" // Localhost requirement [cite: 14]
#define PORT 8080
#define NUM_THREADS 5         // Requirement: at least 5 threads 
#define MESSAGE "hello system programming course"

/*
 * Function: client_task
 * Purpose: The behavior of a single client thread.
 */
void *client_task(void *arg) {
    long id = (long)arg;
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[1024];
    ssize_t n;

    // 1. Create Socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return NULL;
    }

    // 2. Define Server Address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    // Convert IP string to binary
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        close(sockfd);
        return NULL;
    }

    // 3. Connect to Server [cite: 27]
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sockfd);
        return NULL;
    }

    // 4. Send Message
    snprintf(buffer, sizeof(buffer), "thread %ld: %s", id, MESSAGE);
    printf("Thread %ld: Sending -> %s\n", id, buffer);
    
    if (write(sockfd, buffer, strlen(buffer)) < 0) {
        perror("Write failed");
        close(sockfd);
        return NULL;
    }

    // 5. Receive Response
    memset(buffer, 0, sizeof(buffer));
    n = read(sockfd, buffer, sizeof(buffer) - 1);
    if (n > 0) {
        printf("Thread %ld: Received -> %s\n", id, buffer);
    } else {
        printf("Thread %ld: Read failed or server closed connection\n", id);
    }

    // 6. Close Connection [cite: 28]
    close(sockfd);
    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];
    long i;

    printf("Starting client simulation with %d threads...\n", NUM_THREADS);

    // Create threads
    for (i = 0; i < NUM_THREADS; i++) {
        if (pthread_create(&threads[i], NULL, client_task, (void *)i) != 0) {
            perror("Failed to create thread");
        }
    }

    // Wait for all threads to finish
    for (i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("All client threads finished.\n");
    return 0;
}