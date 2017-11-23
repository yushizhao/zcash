// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2014-2015 Daniel Kraft
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_AUXPOW_H
#define BITCOIN_AUXPOW_H

#include "consensus/params.h"
#include "primitives/pureheader.h"
#include "primitives/transaction.h"
#include "serialize.h"
#include "uint256.h"

#include <vector>

class CBlock;
class CBlockIndex;

/** Header for merge-mining data in the coinbase.  */
static const unsigned char pchMergedMiningHeader[] = {0xfa, 0xbe, 'm', 'm'};

class CAuxPow
{
    
public:
    /** The merkle branch connecting the aux block to our coinbase.  */
    std::vector<uint256> vChainMerkleBranch;
    std::vector<uint256> vMerkleBranch;
    
    /** Parent block header (on which the real PoW is done).  */
    CPureBlockHeader parentBlock;
    std::vector<unsigned char> coinbaseTx;
    
    /** subtree root.  */
    uint256 subRoot;

public:
    CAuxPow(){}
    
    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void
    SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(parentBlock);
        READWRITE(coinbaseTx);        
        READWRITE(vMerkleBranch);      
        READWRITE(vChainMerkleBranch);
        READWRITE(subRoot);
    }

    /**
   * Check the auxpow, given the merge-mined block's hash and our chain ID.
   * Note that this does not verify the actual PoW on the parent block!  It
   * just confirms that all the merkle branches are valid.
   * @param hashAuxBlock Hash of the merge-mined block.
   * @param params Consensus parameters.
   * @return True if the auxpow is valid.
   */
    bool check(const uint256& hashAuxBlock, const Consensus::Params& params) const;

    /**
   * Get the parent block's hash.  This is used to verify that it
   * satisfies the PoW requirement.
   * @return The parent block hash.
   */
    inline uint256
    getParentBlockPoWHash() const
    {
        return parentBlock.GetPoWHash();
    }
    
    /**
   * Calculate the expected index in the merkle tree.  This is also used
   * for the test-suite.
   * @param nNonce The coinbase's nonce value.
   * @param nChainId The chain ID.
   * @param h The merkle block height.
   * @return The expected index for the aux hash.
   */
    static int getExpectedIndex(int nNonce, int nChainId, unsigned h);
};

#endif // BITCOIN_AUXPOW_H
