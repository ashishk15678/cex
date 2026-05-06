#ifndef SOLANA_RPC_H
#define SOLANA_RPC_H

#include <json-c/json.h>

struct json_object* solana_rpc_call(const char* method, struct json_object* params);

#endif // SOLANA_RPC_H