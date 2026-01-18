#define _DEFAULT_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#define RECV_BUFFER_SIZE 16384
#define CONNECT_TIMEOUT_SEC 5

#define CLIENT_OK 0
#define CLIENT_ERR_SOCKET -1
#define CLIENT_ERR_IP -2
#define CLIENT_ERR_CONNECT -3
#define CLIENT_ERR_TIMEOUT -4
#define CLIENT_ERR_MEMORY -5
#define CLIENT_ERR_SEND -6
#define CLIENT_ERR_NOTCONN -7

static int sock_fd = -1;
static char recv_buffer[RECV_BUFFER_SIZE];
static int buffer_offset = 0;
static bool is_connected = false;

typedef void (*MessageCallback)(const char* message);
static MessageCallback g_callback = NULL;

static void set_nonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags != -1) {
        fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
    }
}

static void set_blocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags != -1) {
        fcntl(sockfd, F_SETFL, flags & ~O_NONBLOCK);
    }
}

void client_set_message_callback(MessageCallback callback) {
    g_callback = callback;
}

int client_connect(const char* ip, int port) {
    if (is_connected) {
        return CLIENT_OK;
    }

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        return CLIENT_ERR_SOCKET;
    }

    struct addrinfo hints, *addr_result;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    char port_str[6];
    snprintf(port_str, sizeof(port_str), "%d", port);

    int ret = getaddrinfo(ip, port_str, &hints, &addr_result);
    if (ret != 0 || addr_result == NULL) {
        close(sock_fd);
        sock_fd = -1;
        return CLIENT_ERR_IP;
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr = ((struct sockaddr_in*)addr_result->ai_addr)->sin_addr;
    freeaddrinfo(addr_result);

    set_nonblocking(sock_fd);

    int result_connect = connect(sock_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    if (result_connect < 0 && errno != EINPROGRESS) {
        close(sock_fd);
        sock_fd = -1;
        return CLIENT_ERR_CONNECT;
    }

    fd_set write_fds;
    FD_ZERO(&write_fds);
    FD_SET(sock_fd, &write_fds);

    struct timeval timeout;
    timeout.tv_sec = CONNECT_TIMEOUT_SEC;
    timeout.tv_usec = 0;

    int select_result = select(sock_fd + 1, NULL, &write_fds, NULL, &timeout);

    if (select_result <= 0) {

        close(sock_fd);
        sock_fd = -1;
        return CLIENT_ERR_TIMEOUT;
    }

    int so_error;
    socklen_t len = sizeof(so_error);
    getsockopt(sock_fd, SOL_SOCKET, SO_ERROR, &so_error, &len);

    if (so_error != 0) {
        close(sock_fd);
        sock_fd = -1;
        return CLIENT_ERR_CONNECT;
    }

    is_connected = true;
    buffer_offset = 0;
    memset(recv_buffer, 0, RECV_BUFFER_SIZE);

    return CLIENT_OK;
}

int client_disconnect(void) {
    if (sock_fd >= 0) {
        close(sock_fd);
        sock_fd = -1;
    }
    is_connected = false;
    buffer_offset = 0;
    return CLIENT_OK;
}

bool client_is_connected(void) {
    return is_connected;
}

int client_send_json(const char* json_str) {
    if (!is_connected || sock_fd < 0) {
        return CLIENT_ERR_NOTCONN;
    }

    size_t json_len = strlen(json_str);
    size_t total_len = json_len + 2;

    char* send_buffer = (char*)malloc(total_len);
    if (!send_buffer) {
        return CLIENT_ERR_MEMORY;
    }

    snprintf(send_buffer, total_len, "%s\n", json_str);

    size_t total_sent = 0;
    size_t to_send = json_len + 1;
    int retry_count = 0;
    const int max_retries = 100;

    while (total_sent < to_send && retry_count < max_retries) {
        ssize_t sent = send(sock_fd, send_buffer + total_sent, to_send - total_sent, MSG_NOSIGNAL);

        if (sent < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(1000);
                retry_count++;
                continue;
            }

            free(send_buffer);
            client_disconnect();
            return CLIENT_ERR_SEND;
        }
        total_sent += sent;
    }

    free(send_buffer);

    if (total_sent < to_send) {
        return CLIENT_ERR_SEND;
    }

    return CLIENT_OK;
}

int client_process_messages(void) {
    if (!is_connected || sock_fd < 0) {
        return CLIENT_ERR_NOTCONN;
    }

    int capacity = RECV_BUFFER_SIZE - buffer_offset - 1;
    if (capacity <= 0) {

        fprintf(stderr, "[Client] Warning: Buffer overflow, resetting\n");
        buffer_offset = 0;
        capacity = RECV_BUFFER_SIZE - 1;
    }

    ssize_t bytes_read = recv(sock_fd, recv_buffer + buffer_offset, capacity, 0);

    if (bytes_read > 0) {
        buffer_offset += bytes_read;
        recv_buffer[buffer_offset] = '\0';

        char* start = recv_buffer;
        char* newline;

        while ((newline = strchr(start, '\n')) != NULL) {
            *newline = '\0';

            if (g_callback && strlen(start) > 0) {
                g_callback(start);
            }

            start = newline + 1;
        }

        if (start < recv_buffer + buffer_offset) {
            int remaining = (recv_buffer + buffer_offset) - start;
            memmove(recv_buffer, start, remaining);
            buffer_offset = remaining;
        } else {
            buffer_offset = 0;
        }

        return 1;

    } else if (bytes_read == 0) {

        client_disconnect();
        return CLIENT_ERR_CONNECT;

    } else {

        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        } else {

            client_disconnect();
            return CLIENT_ERR_CONNECT;
        }
    }
}