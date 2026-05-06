#include "base64.h"

static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

size_t base64_encode(const uint8_t *src, size_t len, char *out, size_t out_len) {
    size_t i = 0, j = 0;
    uint32_t octet_a, octet_b, octet_c, triple;

    size_t expected_len = 4 * ((len + 2) / 3);
    if (out_len < expected_len + 1) return 0;

    for (i = 0; i < len;) {
        octet_a = i < len ? src[i++] : 0;
        octet_b = i < len ? src[i++] : 0;
        octet_c = i < len ? src[i++] : 0;

        triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        out[j++] = b64_table[(triple >> 3 * 6) & 0x3F];
        out[j++] = b64_table[(triple >> 2 * 6) & 0x3F];
        out[j++] = b64_table[(triple >> 1 * 6) & 0x3F];
        out[j++] = b64_table[(triple >> 0 * 6) & 0x3F];
    }

    int pad = len % 3;
    if (pad > 0) out[expected_len - 1] = '=';
    if (pad == 1) out[expected_len - 2] = '=';

    out[expected_len] = '\0';
    return expected_len;
}