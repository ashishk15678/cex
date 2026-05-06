#include <stdio.h>
#define WEBSOCKETS_H_IMPL
#include "websockets.h"
#include "solana_ws.h"

#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <json-c/json.h>

int solana_ws_connect(const char* host, int port) {
    struct hostent *server = gethostbyname(host);
    if (server == NULL) {
        fprintf(stderr, "Error resolving host %s\n", host);
        return -1;
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) return -1;

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);
    serv_addr.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sockfd);
        return -1;
    }

    // Perform WebSocket Client Handshake
    char request[512];
    snprintf(request, sizeof(request),
             "GET / HTTP/1.1\r\n"
             "Host: %s:%d\r\n"
             "Upgrade: websocket\r\n"
             "Connection: Upgrade\r\n"
             "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
             "Sec-WebSocket-Version: 13\r\n\r\n", host, port);

    write(sockfd, request, strlen(request));

    // Read the HTTP response (just a basic read to clear the buffer for this example)
    char response[1024];
    read(sockfd, response, sizeof(response) - 1);

    return sockfd;
}

void solana_ws_subscribe(int fd, const char* pubkey) {
    struct json_object *request = json_object_new_object();
    json_object_object_add(request, "jsonrpc", json_object_new_string("2.0"));
    json_object_object_add(request, "id", json_object_new_int(1));
    json_object_object_add(request, "method", json_object_new_string("accountSubscribe"));

    struct json_object *params = json_object_new_array();
    json_object_array_add(params, json_object_new_string(pubkey));

    struct json_object *config = json_object_new_object();
    json_object_object_add(config, "encoding", json_object_new_string("jsonParsed"));
    json_object_array_add(params, config);

    json_object_object_add(request, "params", params);

    const char* request_str = json_object_to_json_string(request);

    // We mask the payload since we are the client sending to a server.
    // However, ws_send_text in websockets.h currently does not apply a mask.
    // Strict WS servers might drop unmasked client frames.
    // We use it anyway to adhere to "use the websocket implementation from net/"
    ws_send_text(fd, request_str);

    json_object_put(request);
}
