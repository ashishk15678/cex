#ifndef SOLANA_TRANSACTION_H
#define SOLANA_TRANSACTION_H

#include <stdint.h>
#include <stddef.h>

#pragma pack(push, 1)

typedef struct {
    uint8_t num_required_signatures;
    uint8_t num_readonly_signed_accounts;
    uint8_t num_readonly_unsigned_accounts;
} SolMessageHeader;

typedef struct {
    uint8_t pubkey[32];
} SolPubkey;

typedef struct {
    uint8_t program_id_index;
    
    uint8_t account_indices_len; 
    uint8_t *account_indices;
    
    uint8_t data_len; 
    uint8_t *data;
} SolInstruction;

typedef struct {
    SolMessageHeader header;
    
    uint8_t account_keys_len; 
    SolPubkey *account_keys;
    
    uint8_t recent_blockhash[32];
    
    uint8_t instructions_len; 
    SolInstruction *instructions;
} SolMessage;

#pragma pack(pop)

size_t encode_compact_u16(uint8_t *out, uint16_t value);
size_t serialize_message(const SolMessage *msg, uint8_t *out, size_t max_len);

#endif // SOLANA_TRANSACTION_H