#include "crypto.h"
#include <string.h>

int solana_sign_message(const uint8_t *priv_key_seed, const uint8_t *message, size_t msg_len, uint8_t *signature) {
    EVP_PKEY *pkey = EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, NULL, priv_key_seed, 32);
    if (!pkey) return -1;

    EVP_MD_CTX *md_ctx = EVP_MD_CTX_new();
    if (!md_ctx) {
        EVP_PKEY_free(pkey);
        return -1;
    }

    size_t sig_len = SOLANA_SIGNATURE_SIZE;
    int rc = -1;

    if (EVP_DigestSignInit(md_ctx, NULL, NULL, NULL, pkey) == 1) {
        if (EVP_DigestSign(md_ctx, signature, &sig_len, message, msg_len) == 1) {
            if (sig_len == SOLANA_SIGNATURE_SIZE) {
                rc = 0; 
            }
        }
    }

    EVP_MD_CTX_free(md_ctx);
    EVP_PKEY_free(pkey);
    return rc;
}