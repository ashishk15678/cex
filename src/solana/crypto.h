#ifndef SOLANA_CRYPTO_H
#define SOLANA_CRYPTO_H

#include <stdint.h>
#include <stddef.h>
#include <openssl/evp.h>

#define SOLANA_PUBKEY_SIZE 32
#define SOLANA_SIGNATURE_SIZE 64

int solana_sign_message(const uint8_t *priv_key_seed, const uint8_t *message, size_t msg_len, uint8_t *signature);

#endif // SOLANA_CRYPTO_H