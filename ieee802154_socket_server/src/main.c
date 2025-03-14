#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/sem.h>

#define SERVER_PORT 12345         // Server listening port
#define BUFFER_SIZE 128           // Buffer size for incoming messages
#define RESPONSE_MESSAGE "Hello from server!"

#define THREAD_STACK_SIZE 1024    // Stack size for threads
#define THREAD_PRIORITY 5         // Priority for threads

static struct k_sem sync_sem;     // Semaphore for synchronizing receive and transmit
static char recv_buffer[BUFFER_SIZE]; // Buffer to store received messages
static struct sockaddr_in6 client_addr; // Client address
static socklen_t client_addr_len;
static int server_sock;          // Server socket

// Thread stack and thread definitions
K_THREAD_STACK_DEFINE(receive_stack, THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(transmit_stack, THREAD_STACK_SIZE);
struct k_thread receive_thread_data;
struct k_thread transmit_thread_data;

void receive_thread(void *arg1, void *arg2, void *arg3) {
    ssize_t received;

    while (1) {
        memset(recv_buffer, 0, BUFFER_SIZE);
        client_addr_len = sizeof(client_addr);

        // Wait for data from a client
        received = recvfrom(server_sock, recv_buffer, BUFFER_SIZE - 1, 0,
                            (struct sockaddr *)&client_addr, &client_addr_len);
        if (received < 0) {
            printk("Failed to receive data: %d\n", errno);
            continue;
        }

        // Null-terminate the received data for printing
        recv_buffer[received] = '\0';
        printk("Received %zd bytes from client: %s\n", received, recv_buffer);

        // Signal the transmit thread to send a response
        k_sem_give(&sync_sem);
    }
}

void transmit_thread(void *arg1, void *arg2, void *arg3) {
    ssize_t sent;

    while (1) {
        // Wait for the semaphore to be given by the receive thread
        k_sem_take(&sync_sem, K_FOREVER);

        // Respond to the client
        sent = sendto(server_sock, RESPONSE_MESSAGE, strlen(RESPONSE_MESSAGE), 0,
                      (struct sockaddr *)&client_addr, client_addr_len);
        if (sent < 0) {
            printk("Failed to send response: %d\n", errno);
        } else {
            printk("Response sent to client\n");
        }

        // Add a small delay to avoid flooding the network
        k_sleep(K_MSEC(100));
    }
}

void main(void) {
    struct sockaddr_in6 server_addr;

    printk("Starting UDP server with semaphore synchronization\n");

    // Initialize semaphore
    k_sem_init(&sync_sem, 0, 1);

    // Create a UDP socket
    server_sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (server_sock < 0) {
        printk("Failed to create socket: %d\n", errno);
        return;
    }
    printk("Socket created successfully\n");

    // Configure the server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_port = htons(SERVER_PORT);
    server_addr.sin6_addr = in6addr_any;

    // Bind the socket to the server address and port
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printk("Failed to bind socket: %d\n", errno);
        close(server_sock);
        return;
    }
    printk("Socket bound to port %d\n", SERVER_PORT);

    // Start the receive thread
    k_thread_create(&receive_thread_data, receive_stack, THREAD_STACK_SIZE,
                    receive_thread, NULL, NULL, NULL,
                    THREAD_PRIORITY, 0, K_NO_WAIT);

    // Start the transmit thread
    k_thread_create(&transmit_thread_data, transmit_stack, THREAD_STACK_SIZE,
                    transmit_thread, NULL, NULL, NULL,
                    THREAD_PRIORITY, 0, K_NO_WAIT);
}
