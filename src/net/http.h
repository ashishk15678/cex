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



#ifdef __cplusplus
extern "C" {
#endif


typedef struct  {
    char* method;
    char* url;
    char* headers;
    char* body;
} HttpRequest;


HttpRequest createRequest(char* method, char* url, char* headers, char* body);
int sendRequest(HttpRequest request,char* buffer);

char* httpRequestToString(HttpRequest request);

#ifdef HTTP_H_IMPL

#include "http.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 8192

HttpRequest createRequest(char *method, char *url, char *extra_headers,
                          char *body) {
  char host[256] = {0};
  char *url_ptr = url;

  // skip http:// and https://
  if (strncmp(url_ptr, "http://", 7) == 0) {
    url_ptr += 7;
  } else if (strncmp(url_ptr, "https://", 8) == 0) {
    url_ptr += 8;
  }

  sscanf(url_ptr, "%255[^/]", host);

  char std_headers[1024];
  int body_len = body ? strlen(body) : 0;

  snprintf(std_headers, sizeof(std_headers),
           "Host: %s\r\n"
           "User-Agent: C-Client/1.0\r\n"
           "Connection: close\r\n"
           "Content-Length: %d\r\n"
           "%s",
           host, body_len, (extra_headers ? extra_headers : ""));

  char *final_headers = strdup(std_headers);

  return (HttpRequest){
      .method = method, .url = url, .headers = final_headers, .body = body};
}

int sendRequest(HttpRequest request,char* buffer) {
  char *request_str = httpRequestToString(request);
  struct addrinfo hints, *res;
  int fd;

  char host[256] = {0};
  char *url_ptr = request.url;

  char *port = "80";
  // skip http:// and https://
  if (strncmp(url_ptr, "http://", 7) == 0) {
    url_ptr += 7;
  } else if (strncmp(url_ptr, "https://", 8) == 0) {
    url_ptr += 8;
    port = "443";
  }

  sscanf(url_ptr, "%255[^/]", host);

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;       // IPv4
  hints.ai_socktype = SOCK_STREAM; // TCP

  if (getaddrinfo(host, port, &hints, &res) != 0) {
    fprintf(stderr, "Failed to resolve host: %s\n", host);
    free(request_str);
    return -1;
  }

  fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (fd == -1) {
    perror("Socket creation failed");
    goto cleanup_addr;
  }

  if (connect(fd, res->ai_addr, res->ai_addrlen) < 0) {
    perror("Connection failed");
    close(fd);
    goto cleanup_addr;
  }

  SSL_library_init();
  const SSL_METHOD *method = TLS_client_method();
  SSL_CTX *ctx = SSL_CTX_new(method);
  SSL *ssl = SSL_new(ctx);
  SSL_set_fd(ssl, fd);
  SSL_set_tlsext_host_name(ssl, host);

  if (SSL_connect(ssl) <= 0) {
    ERR_print_errors_fp(stderr);
  } else {
    SSL_write(ssl, request_str, strlen(request_str));

    int total_bytes = 0;
    int bytes_received;

    while ((bytes_received = SSL_read(ssl, buffer + total_bytes,
                                     BUFFER_SIZE - total_bytes - 1)) > 0) {
      total_bytes += bytes_received;

      if (total_bytes >= (int)sizeof(buffer) - 1) {
        printf("Buffer full, stopping read.\n");
        break;
      }
    }

    if (total_bytes > 0) {
        buffer[total_bytes] = '\0';
        printf("--- SERVER RESPONSE ---\n%s\n-----------------------\n", buffer);
    }

    if (bytes_received < 0) {
      int err = SSL_get_error(ssl, bytes_received);
      fprintf(stderr, "SSL read error: %d\n", err);
    }
  }

  SSL_free(ssl);
  SSL_CTX_free(ctx);
  close(fd);
  freeaddrinfo(res);
  free(request_str);
  return 0;

cleanup_addr:
  freeaddrinfo(res);
  free(request_str);

  return -1;
}

char *httpRequestToString(HttpRequest request) {
    char *path = strchr(request.url, '/');
    if (path) {
        if (*(path + 1) == '/') path = strchr(path + 2, '/');
    }
    if (!path) path = "/";
    size_t len = strlen(request.method) + strlen(path) +
                 strlen(request.headers) + strlen(request.body) + 64;
    char *str = malloc(len);
    sprintf(str, "%s %s HTTP/1.1\r\n%s\r\n\r\n%s",
            request.method, path, request.headers, request.body);
    return str;
}


#endif

#ifdef __cplusplus
}
#endif
