#include "transaction.h"
#include <string.h>

size_t encode_compact_u16(uint8_t *out, uint16_t value) {
    size_t len = 0;
    while (value >= 0x80) {
        out[len++] = (value & 0x7F) | 0x80;
        value >>= 7;
    }
    out[len++] = (uint8_t)value;
    return len;
}

size_t serialize_message(const SolMessage *msg, uint8_t *out, size_t max_len) {
    size_t offset = 0;

    if (offset + 3 > max_len) return 0;
    out[offset++] = msg->header.num_required_signatures;
    out[offset++] = msg->header.num_readonly_signed_accounts;
    out[offset++] = msg->header.num_readonly_unsigned_accounts;

    offset += encode_compact_u16(out + offset, msg->account_keys_len);
    for (int i = 0; i < msg->account_keys_len; i++) {
        if (offset + 32 > max_len) return 0;
        memcpy(out + offset, msg->account_keys[i].pubkey, 32);
        offset += 32;
    }

    if (offset + 32 > max_len) return 0;
    memcpy(out + offset, msg->recent_blockhash, 32);
    offset += 32;

    offset += encode_compact_u16(out + offset, msg->instructions_len);
    for (int i = 0; i < msg->instructions_len; i++) {
        SolInstruction *ix = &msg->instructions[i];
        
        if (offset + 1 > max_len) return 0;
        out[offset++] = ix->program_id_index;

        offset += encode_compact_u16(out + offset, ix->account_indices_len);
        if (offset + ix->account_indices_len > max_len) return 0;
        memcpy(out + offset, ix->account_indices, ix->account_indices_len);
        offset += ix->account_indices_len;

        offset += encode_compact_u16(out + offset, ix->data_len);
        if (offset + ix->data_len > max_len) return 0;
        memcpy(out + offset, ix->data, ix->data_len);
        offset += ix->data_len;
    }

    return offset;
}