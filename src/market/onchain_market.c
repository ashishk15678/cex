#include <stdint.h>
#include <stddef.h>

extern uint64_t entrypoint(const uint8_t *input);

typedef struct {
    uint8_t is_initialized;
    uint8_t is_resolved;
    uint8_t winning_outcome; 
    uint64_t total_pool_a;
    uint64_t total_pool_b;
} PredictionMarketData;

enum MarketInstruction {
    InitMarket = 0,
    PlaceBet = 1,
    ResolveMarket = 2
};

uint64_t entrypoint(const uint8_t *input) {
    size_t offset = 0;

    uint64_t num_accounts = *(uint64_t *)(input + offset);
    offset += sizeof(uint64_t);

    if (num_accounts < 1) return 1; 

    offset += sizeof(uint8_t);
    
    offset += sizeof(uint8_t) * 2;
    
    offset += 32;
    
    offset += 32;

    offset += sizeof(uint64_t);

    uint64_t data_len = *(uint64_t *)(input + offset);
    offset += sizeof(uint64_t);

    uint8_t *account_data = (uint8_t *)(input + offset);
    offset += data_len;

    offset += sizeof(uint64_t);
    
    uint64_t instr_len = *(uint64_t *)(input + offset);
    offset += sizeof(uint64_t);

    if (instr_len == 0) return 2; 

    const uint8_t *instruction_data = input + offset;
    uint8_t instruction = instruction_data[0];

    if (data_len < sizeof(PredictionMarketData)) {
        return 3; 
    }

    PredictionMarketData *market = (PredictionMarketData *)account_data;

    switch (instruction) {
        case InitMarket:
            if (market->is_initialized) return 4;
            market->is_initialized = 1;
            market->is_resolved = 0;
            market->winning_outcome = 0;
            market->total_pool_a = 0;
            market->total_pool_b = 0;
            break;

        case PlaceBet:
            if (market->is_resolved) return 5;
            if (instr_len < 10) return 6; 
            
            uint8_t option = instruction_data[1];
            uint64_t amount = *(uint64_t *)(instruction_data + 2);
            
            if (option == 1) {
                market->total_pool_a += amount;
            } else if (option == 2) {
                market->total_pool_b += amount;
            } else {
                return 7; 
            }
            break;

        case ResolveMarket:
            if (market->is_resolved) return 5;
            if (instr_len < 2) return 6;

            uint8_t winner = instruction_data[1];
            market->is_resolved = 1;
            market->winning_outcome = winner;
            break;

        default:
            return 8; 
    }

    return 0; 
}