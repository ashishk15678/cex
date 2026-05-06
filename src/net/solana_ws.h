#ifndef SOLANA_WS_H
#define SOLANA_WS_H

int solana_ws_connect(const char* host, int port);
void solana_ws_subscribe(int fd, const char* pubkey);

#endif