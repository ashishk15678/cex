#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "solana/transaction.h"
#include "solana/base64.h"
#include "net/solana_rpc.h"
#include "net/solana_ws.h"
#include "net/websockets.h"

int main() {
    printf("Starting Solana Prediction Market CEX Client...\n");

    // 1. Transaction Serialization Example
    uint8_t instruction_data[10] = {1, 1}; // 1 = PlaceBet, 1 = Option A
    uint64_t amount = 500;
    memcpy(instruction_data + 2, &amount, sizeof(uint64_t));

    SolInstruction ix = {
        .program_id_index = 1,
        .account_indices_len = 1,
        .account_indices = (uint8_t[]){0},
        .data_len = 10,
        .data = instruction_data
    };

    SolMessage msg;
    memset(&msg, 0, sizeof(msg));
    msg.header.num_required_signatures = 1;
    msg.instructions_len = 1;
    msg.instructions = &ix;

    uint8_t serialized_msg[512];
    size_t msg_len = serialize_message(&msg, serialized_msg, sizeof(serialized_msg));
    printf("Successfully serialized transaction message: %zu bytes\n", msg_len);

    // 2. WebSocket Subscription Example using local net/websockets.h
    printf("Connecting to Solana WS...\n");
    // Connects to local or a proxy ws since TLS requires WSS natively not supported by raw TCP read/write
    int ws_fd = solana_ws_connect("api.devnet.solana.com", 8900); 
    if (ws_fd >= 0) {
        printf("Connected. Subscribing to account...\n");
        // Subscribe to a dummy prediction market account key
        solana_ws_subscribe(ws_fd, "11111111111111111111111111111111");

        // Read the first few frames
        Frame f;
        if (ws_read_frame(ws_fd, &f) == 0) {
            if (f.payload) {
                printf("Received WS Message: %s\n", f.payload);
            }
            ws_free_frame(&f);
        }
        close(ws_fd);
    } else {
        printf("Could not connect to WS (Note: standard RPCs might block port 8900 without WSS)\n");
    }

    return EXIT_SUCCESS;
}