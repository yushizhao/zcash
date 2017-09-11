#include "/home/blockchain/leveldbview/bitcoin-master/src/dbwrapper.h"
#include "/home/blockchain/leveldbview/bitcoin-master/src/uint256.h"
#include "uint256.cpp"
#include "utilstrencodings.cpp"
#include "utilstrencodings.h"
#include "/home/blockchain/leveldbview/bitcoin-master/src/txdb.h"
#include "dbwrapper.cpp"
#include "fs.h"
#include "fs.cpp"
#include "util.h"
#include "util.cpp"
#include "utiltime.h"
#include "utiltime.cpp"
#include "random.h"
#include "random.cpp"
#include "crypto/sha512.h"
#include "crypto/sha512.cpp"
#include "crypto/sha256.h"
#include "crypto/sha256.cpp"
#include "crypto/chacha20.h"
#include "crypto/chacha20.cpp"
#include "crypto/ripemd160.h"
#include "crypto/ripemd160.cpp"
#include "crypto/hmac_sha512.h"
#include "crypto/hmac_sha512.cpp"
#include "script/script.h"
#include "script/script.cpp"
#include "support/cleanse.h"
#include "support/cleanse.cpp"
#include "support/lockedpool.h"
#include "support/lockedpool.cpp"
#include "chainparamsbase.h"
#include "chainparamsbase.cpp"
#include "/home/blockchain/leveldbview/bitcoin-master/src/leveldb/include/leveldb/db.h"
#include "/home/blockchain/leveldbview/bitcoin-master/src/primitives/transaction.h"
#include "/home/blockchain/leveldbview/bitcoin-master/src/primitives/transaction.cpp"
#include "/home/blockchain/leveldbview/bitcoin-master/src/primitives/block.h"
#include "/home/blockchain/leveldbview/bitcoin-master/src/primitives/block.cpp"
#include "/home/blockchain/leveldbview/bitcoin-master/src/txdb.h"
#include "/home/blockchain/leveldbview/bitcoin-master/src/txdb.cpp"
#include "coins.h"
#include "coins.cpp"
#include "compressor.h"
#include "compressor.cpp"
#include "pubkey.h"
#include "pubkey.cpp"
#include "secp256k1.h"
#include "secp256k1_recovery.h"
#include "hash.h"
#include "hash.cpp"
#include "script/standard.h"
#include "script/standard.cpp"
#include "base58.h"
#include "base58.cpp"
#include "chainparams.h"
#include "chainparams.cpp"
#include "consensus/merkle.h"
#include "consensus/merkle.cpp"
#include "key.h"
#include "key.cpp"

#include "/home/blockchain/leveldbview/bitcoin-master/src/secp256k1/src/secp256k1.c"
#include "/home/blockchain/leveldbview/bitcoin-master/src/secp256k1/src/modules/recovery/main_impl.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <stdint.h>
#include <string>
#include <stdlib.h> 

#include <sqlite3.h>

int main(int argc, char* argv[]) {

    CBlock genesis;
    
    const char* pszTimestamp = "Zcash0b9c4eef8b7cc417ee5001e3500984b6fea35683a7cac141a043c42064835d34";
    
    CMutableTransaction txNew;
    txNew.vin.resize(1);
    txNew.vout.resize(1+16529043);
    txNew.vin[0].scriptSig = CScript() << 520617983 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    // txNew.vout[0].nValue = 0;
    // txNew.vout[0].scriptPubKey = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
    
        /* Open LEVELDB database */
    const std::string PATH = "/btc/478558_A_1460";
	
	CDBWrapper db(PATH, 1024*64, false, false, true);
	
	static const char DB_COIN = 'C';
    static const char DB_BEST_BLOCK = 'B';
    
    COutPoint inner_key;
	CoinEntry key(&inner_key);
	Coin value;
	std::unique_ptr<CDBIterator> iter(db.NewIterator());
    iter->Seek(DB_COIN);

    while (iter->Valid()) {        
		iter->GetKey(key);
		iter->GetValue(value);
        txNew.vout[inner_key.n] = value.out;
        iter->Next();
    }
    
    genesis.vtx.push_back(txNew);
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = genesis.BuildMerkleTree();
    genesis.nTime    = 1477641360;
    genesis.nBits    = 0x1f07ffff;
    genesis.nNonce   = 0;
	uint256 genesisHash = genesis.GetHash();
    uint256 txHash = genesis.vtx[0]->GetHash();    
    std::cout << genesisHash.ToString() << "\n";
    std::cout << txHash.ToString() << "\n";
    
    std::string hbc;
	uint256 hashBestChain;
	db.Read(DB_BEST_BLOCK, hashBestChain);
	hbc = hashBestChain.ToString();
	std::cout << hbc << "\n";
    std::cout << inner_key.hash.ToString() << "\n";
    
    return 0;
}
