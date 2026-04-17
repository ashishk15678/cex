// SPDX-License-Identifier: MIT
// SPDX FileCopyRightText : Ashish Kumar <15678ashishk@gmail.com>

/*
* Single Header Library Pattern
* All files in this project use a simple technique
* to define the interface and implementation separately.
*
* The interface is defined in this header file, and the implementation
* is provided by `#define FILE_NAME_H_IMPL`.
*/



#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char *method;
    char *path;
    char *query;
    char *body;
    // keeping context separate from struct fields
    char context[];
} HttpContext;

int createServer(int port) ;

#ifdef HTTP_SERVER_H_IMPL

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

typedef struct {
  char *api;
  void *(*handler)(int);
} ApiEndpoint;

void *handleApi(int client_fd) {
  char *response = "HTTP/1.1 200 OK\r\n"
                   "Content-Type: application/json\r\n"
                   "Content-Length: 22\r\n\r\n"
                   "{\"status\": \"todos ok\"}";
  write(client_fd, response, strlen(response));
}

// List of API endpoints ,
// "api" : handler
ApiEndpoint API_LIST[] = {
    {"/api/", handleApi},
};

int createServer(int port) {
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == -1) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  struct sockaddr_in server_addr = {
      .sin_family = AF_INET,
      .sin_port = htons(port),
      .sin_addr = {.s_addr = INADDR_ANY},
  };

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) ==
      -1) {
    perror("bind");
    exit(EXIT_FAILURE);
  }

  if (listen(server_fd, SOMAXCONN) == -1) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  return server_fd;
}


#endif

#ifdef __cplusplus
}
#endif
