#ifndef SOLANA_BASE64_H
#define SOLANA_BASE64_H

#include <stddef.h>
#include <stdint.h>

size_t base64_encode(const uint8_t *src, size_t len, char *out, size_t out_len);

#endif