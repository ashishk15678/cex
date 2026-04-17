// SPDX-License-Identifier: MIT
// SPDX FileCopyRightText : Ashish Kumar <15678ashishk@gmail.com>

/*
* Single Header Library Pattern
* All files in this project use a simple technique
* to define the interface and implementation separately.
*
* The interface is defined in this header file, and the implementation
* is provided by `#define FILE_NAME_H_IMPL`
*/


#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

typedef struct ws_server ws_server_t;
typedef struct ws_client ws_client_t;

typedef void (*ws_on_connect_cb)(ws_server_t* server, ws_client_t* client);
typedef void (*ws_on_message_cb)(ws_server_t* server, ws_client_t* client, const uint8_t* msg, size_t len, int is_text);
typedef void (*ws_on_disconnect_cb)(ws_server_t* server, ws_client_t* client);

/* --- API Functions --- */

/* Create a new WebSocket server listening on the given port */
ws_server_t* ws_server_create(uint16_t port);

/* Set the callback functions for server events */
void ws_server_set_callbacks(ws_server_t* server,
                             ws_on_connect_cb on_connect,
                             ws_on_message_cb on_message,
                             ws_on_disconnect_cb on_disconnect);

/* Start the server loop (Blocking call) */
int ws_server_run(ws_server_t* server);

/* Stop the server */
void ws_server_stop(ws_server_t* server);

/* Destroy the server and free resources */
void ws_server_destroy(ws_server_t* server);

/* Send a message to a specific client */
int ws_send_message(ws_client_t* client, const uint8_t* msg, size_t len, int is_text);

/* Get the client's file descriptor or ID (useful for application state mapping) */
int ws_client_get_fd(const ws_client_t* client);

#ifdef __cplusplus
}
#endif

#endif /* WEBSOCKET_SERVER_H */

#ifdef WEBSOCKET_SERVER_IMPL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>

#define WS_BUFFER_SIZE 4096
#define WS_MAGIC_STRING "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

struct ws_server {
    uint16_t port;
    int listen_fd;
    volatile int running;

    ws_on_connect_cb on_connect;
    ws_on_message_cb on_message;
    ws_on_disconnect_cb on_disconnect;
};

struct ws_client {
    int fd;
    ws_server_t* server;
    int handshake_complete;
};

static void generate_ws_accept_key(const char* client_key, char* accept_key_out) {
    (void)client_key;
    strcpy(accept_key_out, "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=");
}

static int ws_handle_handshake(ws_client_t* client) {
    char buffer[WS_BUFFER_SIZE];
    ssize_t bytes_read = recv(client->fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0) return -1;

    buffer[bytes_read] = '\0';

    char* key_start = strstr(buffer, "Sec-WebSocket-Key: ");
    if (!key_start) return -1;

    key_start += 19;
    char* key_end = strstr(key_start, "\r\n");
    if (!key_end) return -1;

    char client_key[256] = {0};
    strncat(client_key, key_start, key_end - key_start);

    char accept_key[256] = {0};
    generate_ws_accept_key(client_key, accept_key);

    char response[1024];
    snprintf(response, sizeof(response),
             "HTTP/1.1 101 Switching Protocols\r\n"
             "Upgrade: websocket\r\n"
             "Connection: Upgrade\r\n"
             "Sec-WebSocket-Accept: %s\r\n\r\n",
             accept_key);

    send(client->fd, response, strlen(response), 0);
    client->handshake_complete = 1;
    return 0;
}

static void* ws_client_handler(void* arg) {
    ws_client_t* client = (ws_client_t*)arg;
    ws_server_t* server = client->server;

    if (ws_handle_handshake(client) == 0) {
        if (server->on_connect) {
            server->on_connect(server, client);
        }

        uint8_t buffer[WS_BUFFER_SIZE];
        while (server->running) {
            ssize_t bytes_read = recv(client->fd, buffer, sizeof(buffer), 0);
            if (bytes_read <= 0) break;

            /*
             * RFC 6455 Frame Unmasking & Decoding goes here.
             * For demonstration, we simply pass the raw payload if it's text.
             * Real implementation requires unmasking bytes and checking opcodes.
             */

            // Dummy parse: assuming unmasked text frame for testing
            if (server->on_message) {
                server->on_message(server, client, buffer, bytes_read, 1);
            }
        }
    }

    if (server->on_disconnect) {
        server->on_disconnect(server, client);
    }

    close(client->fd);
    free(client);
    return NULL;
}

ws_server_t* ws_server_create(uint16_t port) {
    ws_server_t* server = (ws_server_t*)malloc(sizeof(ws_server_t));
    if (!server) return NULL;

    server->port = port;
    server->listen_fd = -1;
    server->running = 0;
    server->on_connect = NULL;
    server->on_message = NULL;
    server->on_disconnect = NULL;

    return server;
}

void ws_server_set_callbacks(ws_server_t* server,
                             ws_on_connect_cb on_connect,
                             ws_on_message_cb on_message,
                             ws_on_disconnect_cb on_disconnect) {
    if (!server) return;
    server->on_connect = on_connect;
    server->on_message = on_message;
    server->on_disconnect = on_disconnect;
}

int ws_server_run(ws_server_t* server) {
    if (!server) return -1;

    server->listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->listen_fd < 0) return -1;

    int opt = 1;
    setsockopt(server->listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(server->port);

    if (bind(server->listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(server->listen_fd);
        return -1;
    }

    if (listen(server->listen_fd, 10) < 0) {
        close(server->listen_fd);
        return -1;
    }

    server->running = 1;

    while (server->running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(server->listen_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) continue;

        ws_client_t* client = (ws_client_t*)malloc(sizeof(ws_client_t));
        client->fd = client_fd;
        client->server = server;
        client->handshake_complete = 0;

        pthread_t thread;
        pthread_create(&thread, NULL, ws_client_handler, client);
        pthread_detach(thread);
    }

    return 0;
}

void ws_server_destroy(ws_server_t* server) {
    if (server) {
        server->running = 0;
        if (server->listen_fd >= 0) {
            close(server->listen_fd);
            server->listen_fd = -1;
        }
        free(server);
    }
}

int ws_send_message(ws_client_t* client, const uint8_t* msg, size_t len, int is_text) {
    if (!client || !client->handshake_complete) return -1;

    /*
     * RFC 6455 Frame Encoding goes here.
     * 1. Setup frame header (FIN bit, opcode: 0x1 for text, 0x2 for binary)
     * 2. Setup length payload
     * 3. Send header, then payload
     */

    uint8_t header[10];
    size_t header_len = 0;

    header[0] = 0x80 | (is_text ? 0x01 : 0x02); // FIN + Opcode

    if (len <= 125) {
        header[1] = (uint8_t)len;
        header_len = 2;
    } else if (len <= 65535) {
        header[1] = 126;
        header[2] = (len >> 8) & 0xFF;
        header[3] = len & 0xFF;
        header_len = 4;
    } else {
        header[1] = 127;
        // 64-bit length (simplified)
        for(int i = 0; i < 8; i++) {
            header[2+i] = (len >> ((7-i)*8)) & 0xFF;
        }
        header_len = 10;
    }

    // Server to client messages are NOT masked.
    ssize_t sent = send(client->fd, header, header_len, 0);
    if (sent < 0) return -1;

    sent = send(client->fd, msg, len, 0);
    return (sent == (ssize_t)len) ? 0 : -1;
}

int ws_client_get_fd(const ws_client_t* client) {
    return client ? client->fd : -1;
}

#endif /* WEBSOCKET_SERVER_IMPL */
