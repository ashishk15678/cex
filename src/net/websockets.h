// SPDX-License-Identifier: MIT
// SPDX FileCopyRightText : Ashish Kumar <15678ashishk@gmail.com>

// Single Header Library Pattern
// All files in this project use a simple technique
// to define the interface and implementation separately.
//
// The interface is defined in this header file, and the implementation
// is provided by `#define FILE_NAME_H_IMPL`

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WEBSOCKETS_H
#define WEBSOCKETS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef enum {
    WS_OPCODE_CONT  = 0x0,
    WS_OPCODE_TEXT  = 0x1,
    WS_OPCODE_BIN   = 0x2,
    WS_OPCODE_CLOSE = 0x8,
    WS_OPCODE_PING  = 0x9,
    WS_OPCODE_PONG  = 0xA
} WsOpcode;

typedef struct {
    int fd;
    bool is_open;
} WebSocket;

typedef struct {
    bool fin;
    WsOpcode opcode;
    bool masked;
    uint8_t mask[4];
    uint64_t payload_len;
    uint8_t* payload;
} Frame;

void handle_websocket_upgrade(int client_fd, const char* client_key);
void ws_send_text(int client_fd, const char* json_payload);
int ws_read_frame(int client_fd, Frame* out_frame);
void ws_free_frame(Frame* frame);

#endif // WEBSOCKETS_H

#ifdef WEBSOCKETS_H_IMPL

#include "websockets.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <arpa/inet.h>

void handle_websocket_upgrade(int client_fd, const char* client_key) {
    if (!client_key) return;

    const char* magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    char combined[256];
    snprintf(combined, sizeof(combined), "%s%s", client_key, magic);

    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1((unsigned char*)combined, strlen(combined), hash);

    // Base64 encode the hash
    // EVP_EncodeBlock returns the number of bytes encoded, excluding the NUL terminator
    char b64[100];
    EVP_EncodeBlock((unsigned char*)b64, hash, SHA_DIGEST_LENGTH);

    char response[512];
    snprintf(response, sizeof(response),
             "HTTP/1.1 101 Switching Protocols\r\n"
             "Upgrade: websocket\r\n"
             "Connection: Upgrade\r\n"
             "Sec-WebSocket-Accept: %s\r\n"
             "Sec-WebSocket-Protocol: cex-protocol\r\n\r\n", b64);

    write(client_fd, response, strlen(response));

}

void ws_send_text(int client_fd, const char* json_payload) {
    if (!json_payload) return;

    size_t len = strlen(json_payload);
    unsigned char header[10];
    int header_len = 0;

    header[0] = 0x81; // FIN bit set, TEXT frame (0x01)

    if (len <= 125) {
        header[1] = len;
        header_len = 2;
    } else if (len <= 65535) {
        header[1] = 126;
        header[2] = (len >> 8) & 0xFF;
        header[3] = len & 0xFF;
        header_len = 4;
    } else {
        header[1] = 127;
        for (int i = 0; i < 8; i++) {
            header[2 + i] = (len >> ((7 - i) * 8)) & 0xFF;
        }
        header_len = 10;
    }

    write(client_fd, header, header_len);
    write(client_fd, json_payload, len);
}

int ws_read_frame(int client_fd, Frame* out_frame) {
    if (!out_frame) return -1;
    memset(out_frame, 0, sizeof(Frame));

    unsigned char header[2];
    ssize_t bytes_read = read(client_fd, header, 2);
    if (bytes_read <= 0) return -1; // Connection closed or error

    out_frame->fin = (header[0] & 0x80) != 0;
    out_frame->opcode = (WsOpcode)(header[0] & 0x0F);
    out_frame->masked = (header[1] & 0x80) != 0;
    uint64_t payload_len = header[1] & 0x7F;

    if (payload_len == 126) {
        unsigned char ext_len[2];
        if (read(client_fd, ext_len, 2) != 2) return -1;
        payload_len = (ext_len[0] << 8) | ext_len[1];
    } else if (payload_len == 127) {
        unsigned char ext_len[8];
        if (read(client_fd, ext_len, 8) != 8) return -1;
        payload_len = 0;
        for (int i = 0; i < 8; i++) {
            payload_len = (payload_len << 8) | ext_len[i];
        }
    }
    out_frame->payload_len = payload_len;

    if (out_frame->masked) {
        if (read(client_fd, out_frame->mask, 4) != 4) return -1;
    }

    if (payload_len > 0) {
        out_frame->payload = (uint8_t*)malloc(payload_len + 1); // +1 for null termination
        if (!out_frame->payload) return -1;

        uint64_t total_read = 0;
        while (total_read < payload_len) {
            ssize_t r = read(client_fd, out_frame->payload + total_read, payload_len - total_read);
            if (r <= 0) {
                free(out_frame->payload);
                out_frame->payload = NULL;
                return -1;
            }
            total_read += r;
        }

        // Unmask the payload if necessary
        if (out_frame->masked) {
            for (uint64_t i = 0; i < payload_len; i++) {
                out_frame->payload[i] ^= out_frame->mask[i % 4];
            }
        }

        // Null-terminate just in case it's a text frame used as a C-string
        out_frame->payload[payload_len] = '\0';
    } else {
        out_frame->payload = NULL;
    }

    return 0;
}

void ws_free_frame(Frame* frame) {
    if (frame && frame->payload) {
        free(frame->payload);
        frame->payload = NULL;
    }
}

#endif

#ifdef __cplusplus
}
#endif
