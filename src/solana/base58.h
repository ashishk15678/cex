#ifndef SOLANA_BASE58_H
#define SOLANA_BASE58_H

#include <stddef.h>
#include <stdint.h>

size_t b58_encode(const uint8_t *data, size_t binsz, char *b58, size_t b58sz);

#endif // SOLANA_BASE58_H