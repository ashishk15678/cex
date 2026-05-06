#include "base58.h"
#include <string.h>

static const char b58_alphabet[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

size_t b58_encode(const uint8_t *data, size_t binsz, char *b58, size_t b58sz) {
    int zeroes = 0;
    while (zeroes < binsz && data[zeroes] == 0) {
        zeroes++;
    }

    size_t size = binsz * 138 / 100 + 1;
    uint8_t b58_bytes[size];
    memset(b58_bytes, 0, size);

    size_t length = 0;
    for (size_t i = zeroes; i < binsz; i++) {
        uint32_t carry = data[i];
        size_t j = 0;
        for (size_t k = size; k > 0 && (carry != 0 || j < length); k--, j++) {
            carry += 256 * b58_bytes[k - 1];
            b58_bytes[k - 1] = carry % 58;
            carry /= 58;
        }
        length = j;
    }

    size_t str_len = zeroes + length;
    if (b58sz <= str_len) {
        return 0;
    }

    for (int i = 0; i < zeroes; i++) {
        b58[i] = '1';
    }
    for (size_t i = 0; i < length; i++) {
        b58[zeroes + i] = b58_alphabet[b58_bytes[size - length + i]];
    }
    b58[str_len] = '\0';
    return str_len;
}