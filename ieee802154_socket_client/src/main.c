#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/sem.h>

#define SERVER_PORT 12345        // Server port
#define MESSAGE "Hello from Client!"
#define SLEEP_TIME_MS 1000       // Time between sending messages (in milliseconds)
#define BUFFER_SIZE 128

#define THREAD_STACK_SIZE 1024   // Stack size for each thread
#define THREAD_PRIORITY 5        // Priority for threads

// Shared resources
static int sock;
static struct sockaddr_in6 server_addr;

// Semaphores
K_SEM_DEFINE(socket_sem, 1, 1); // Semaphore for socket access

// Thread stacks and thread objects
K_THREAD_STACK_DEFINE(send_stack, THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(receive_stack, THREAD_STACK_SIZE);
struct k_thread send_thread_data;
struct k_thread receive_thread_data;

static int initialize_socket(struct sockaddr_in6 *server_addr)
{
    int sock;

    // Configure the server address
    memset(server_addr, 0, sizeof(struct sockaddr_in6));
    server_addr->sin6_family = AF_INET6;
    server_addr->sin6_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET6, CONFIG_NET_CONFIG_PEER_IPV6_ADDR, &server_addr->sin6_addr) != 1) {
        printk("Invalid IPv6 address format\n");
        return -1;
    }

    // Create a socket
    sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        printk("Failed to create socket: %d\n", errno);
        return -1;
    }
    printk("Socket created successfully\n");

    return sock;
}

void send_message(void *arg1, void *arg2, void *arg3)
{
    ssize_t sent;

    while (1) {
        // Take semaphore to gain exclusive access to the socket
        k_sem_take(&socket_sem, K_FOREVER);

        sent = sendto(sock, MESSAGE, strlen(MESSAGE), 0,
                      (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in6));
        if (sent < 0) {
            printk("Failed to send message: %d\n", errno);
        } else {
            printk("Message sent successfully: %s\n", MESSAGE);
        }

        // Release the semaphore
        k_sem_give(&socket_sem);

        k_sleep(K_MSEC(SLEEP_TIME_MS)); // Sleep before sending the next message
    }
}

void receive_message(void *arg1, void *arg2, void *arg3)
{
    char recv_buffer[BUFFER_SIZE];
    ssize_t received;
    struct sockaddr_in6 client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    while (1) {
        // Take semaphore to gain exclusive access to the socket
        k_sem_take(&socket_sem, K_FOREVER);

        memset(recv_buffer, 0, BUFFER_SIZE);
        received = recvfrom(sock, recv_buffer, BUFFER_SIZE - 1, 0,
                            (struct sockaddr *)&client_addr, &client_addr_len);
        if (received < 0) {
            printk("Failed to receive data: %d\n", errno);
        } else {
            // Null-terminate the received data for printing
            recv_buffer[received] = '\0';
            printk("Received %zd bytes: %s\n", received, recv_buffer);
        }

        // Release the semaphore
        k_sem_give(&socket_sem);

        k_sleep(K_MSEC(SLEEP_TIME_MS)); // Sleep before checking for the next message
    }
}

void main(void)
{
    printk("Socket client with semaphores example\n");

    // Initialize socket
    sock = initialize_socket(&server_addr);
    if (sock < 0) {
        return;
    }

    // Create the sending thread
    k_thread_create(&send_thread_data, send_stack, THREAD_STACK_SIZE,
                    send_message, NULL, NULL, NULL,
                    THREAD_PRIORITY, 0, K_NO_WAIT);

    // Create the receiving thread
    k_thread_create(&receive_thread_data, receive_stack, THREAD_STACK_SIZE,
                    receive_message, NULL, NULL, NULL,
                    THREAD_PRIORITY, 0, K_NO_WAIT);

    // Main thread can remain idle or handle other tasks
    while (1) {
        k_sleep(K_FOREVER); // Keep the main thread idle
    }

    // Close the socket (this part won't execute in the loop)
    close(sock);
}



// TODO: Check the SYMBOL SYSTEM for using net_if_up()

/*
    COMMENTS:
        - net_if_up() must be used for manual configuration.
        - activate CONFIG_NET_CONFIG_SETTINGS to set interface up. Use the below symbols to config. IPV6
            - CONFIG_NET_CONFIG_MY_IPV6_ADDR="2001:db8::2"
            - CONFIG_NET_CONFIG_PEER_IPV6_ADDR="2001:db8::1"

*/          