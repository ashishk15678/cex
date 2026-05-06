#!/bin/bash

# Exit on error
set -e

echo "Building the On-chain Prediction Market C Program..."
make bpf

echo "Checking if Solana CLI is installed..."
if ! command -v solana &> /dev/null
then
    echo "Solana CLI could not be found. Please install it:"
    echo "sh -c \"\$(curl -sSfL https://release.solana.com/v1.16.0/install)\""
    exit
fi

echo "Deploying to Solana Devnet..."
# Ensure we are on devnet
solana config set --url devnet

# Create a dummy keypair if needed
if [ ! -f ~/.config/solana/id.json ]; then
    echo "Generating new Solana keypair..."
    solana-keygen new --no-bip39-passphrase
fi

# Airdrop some devnet SOL
echo "Airdropping 2 SOL for deployment..."
solana airdrop 2 || true

# Deploy the compiled .o file
# Note: typically Solana expects a .so shared object file for BPF.
# Since we compiled a raw .o, we need to link it. For a proper deployment,
# you'd use cargo-build-sbf, but this deploys the binary if formatted correctly.
echo "To deploy the raw BPF object, we convert it to a shared object first:"
ld.lld -z notext -shared --Bdynamic build/onchain_market.o -o build/onchain_market.so

echo "Deploying contract..."
solana program deploy build/onchain_market.so

echo "Deployment complete! Copy the Program ID above to use in the frontend."
