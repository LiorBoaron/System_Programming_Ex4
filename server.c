/*
 * Ex4: System Programming (3503820)
 * Server Program
 * Implements a TCP Echo server using threads (pthread).
 * Converts input to uppercase.
 * Uses mutex for counting active clients.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080
#define BUFFER_SIZE 4096 // Per requirement section 3
#define MAX_PENDING 10

// Global variables
int active_clients = 0;
pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * Function: write_all
 * Purpose: Ensures all bytes are written to the socket.
 * Handling partial writes is a requirement.
 */
ssize_t write_all(int sockfd, const char *buf, size_t len) {
    size_t total_written = 0;
    while (total_written < len) {
        ssize_t written = write(sockfd, buf + total_written, len - total_written);
        if (written <= 0) {
            return written; // Error or connection closed
        }
        total_written += written;
    }
    return total_written;
}

/*
 * Function: client_handler
 * Purpose: Thread function to handle a single client connection.
 * Reads data, converts to uppercase, and echoes back.
 */
void *client_handler(void *arg) {
    int newsockfd = *(int *)arg;
    free(arg); // Free memory allocated in main

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    // Critical Section: Increment client counter
    pthread_mutex_lock(&client_mutex);
    active_clients++;
    // Using write instead of printf to stick to System Calls
    char log_msg[64];
    int log_len = sprintf(log_msg, "Client connected. Active clients: %d\n", active_clients);
    write(STDOUT_FILENO, log_msg, log_len);
    pthread_mutex_unlock(&client_mutex);

    // Main Echo Loop
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        
        // Blocking read call
        bytes_read = read(newsockfd, buffer, BUFFER_SIZE);

        if (bytes_read < 0) {
            perror("Error reading from socket");
            break;
        }
        if (bytes_read == 0) {
            // Client closed connection
            break;
        }

        // Processing: Convert to Uppercase
        for (int i = 0; i < bytes_read; i++) {
            if (islower(buffer[i])) {
                buffer[i] = toupper(buffer[i]);
            }
        }

        // Send back using the robust write function
        if (write_all(newsockfd, buffer, bytes_read) < 0) {
            perror("Error writing to socket");
            break;
        }
    }

    // Critical Section: Decrement client counter
    pthread_mutex_lock(&client_mutex);
    active_clients--;
    log_len = sprintf(log_msg, "Client disconnected. Active clients: %d\n", active_clients);
    write(STDOUT_FILENO, log_msg, log_len);
    pthread_mutex_unlock(&client_mutex);

    close(newsockfd);
    return NULL;
}

int main() {
    int sockfd, *newsockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    pthread_t thread_id;

    // 1. Create Socket (TCP)
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error opening socket");
        exit(1);
    }

    // Option to reuse address (helps when restarting server quickly)
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 2. Bind structure setup
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // Listen on localhost/all interfaces
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error on binding");
        exit(1);
    }

    // 3. Listen
    if (listen(sockfd, MAX_PENDING) < 0) {
        perror("Error on listen");
        exit(1);
    }

    char start_msg[] = "Server is listening on port 8080...\n";
    write(STDOUT_FILENO, start_msg, sizeof(start_msg)-1);

    // 4. Accept Loop
    while (1) {
        client_len = sizeof(client_addr);
        newsockfd = malloc(sizeof(int)); // Allocate memory for each socket fd
        if (!newsockfd) {
            perror("Memory allocation failed");
            continue;
        }

        *newsockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
        if (*newsockfd < 0) {
            perror("Error on accept");
            free(newsockfd);
            continue;
        }

        // Create a new thread for the client
        if (pthread_create(&thread_id, NULL, client_handler, (void *)newsockfd) != 0) {
            perror("Error creating thread");
            // FIX: Close socket first, THEN free memory
            close(*newsockfd);
            free(newsockfd);
        } else {
            // Detach thread so resources are released upon completion
            pthread_detach(thread_id);
        }
    }

    close(sockfd);
    return 0;
}