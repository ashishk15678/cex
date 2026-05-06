const web3 = require("@solana/web3.js");
const crypto = require("crypto");

async function main() {
  console.log("Connecting to Devnet...");
  const connection = new web3.Connection(
    web3.clusterApiUrl("devnet"),
    "confirmed",
  );

  // Generate a random keypair for testing
  const payer = web3.Keypair.generate();
  console.log("Generated Payer Address:", payer.publicKey.toBase58());

  console.log("Airdropping 1 SOL...");
  try {
    const signature = await connection.requestAirdrop(
      payer.publicKey,
      web3.LAMPORTS_PER_SOL,
    );
    await connection.confirmTransaction(signature);
    console.log("Airdrop successful.");
  } catch (e) {
    console.log(
      "Airdrop failed (might be rate limited), using existing funds if any.",
    );
  }

  // Generate a program ID representing where the C code is deployed
  // In reality, this should be the output of `solana program deploy`
  const programId = web3.Keypair.generate().publicKey;

  // Generate the market state account
  const marketAccount = web3.Keypair.generate();

  console.log("\n==================================");
  console.log("Test Configuration:");
  console.log("Program ID:", programId.toBase58());
  console.log("Market Data Account:", marketAccount.publicKey.toBase58());
  console.log("==================================\n");

  console.log(
    "To use the HTML UI, use the Program ID and Market Data Account printed above.",
  );
}

main().catch(console.error);
