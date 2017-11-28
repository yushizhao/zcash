// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2011 Vince Durham
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2014-2015 Daniel Kraft
// Copyright (c) 2014-2015 The Dogecoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include "auxpow.h"

#include "hash.h"
#include "primitives/transaction.h"

#include "chainparams.h"
#include "consensus/validation.h"
#include "main.h"
#include "script/script.h"
#include "util.h"

#include <algorithm>

/* ************************************************************************** */
bool
CAuxPow::check(const uint256& hashAuxBlock, const Consensus::Params& params) const
{
    if (vChainMerkleBranch.size() > 20)
        return error("Aux POW chain merkle branch too long");
    
    std::vector<unsigned char>::const_iterator pcHead =
        std::search(coinbaseTx.begin(), coinbaseTx.end(), UBEGIN(pchMergedMiningHeader), UEND(pchMergedMiningHeader));

    if (pcHead == coinbaseTx.end())
        return error("Aux POW missing MergedMiningHeader in parent coinbase");
    
    pcHead = pcHead + sizeof(pchMergedMiningHeader);
    
    int nSize;
    memcpy(&nSize, &pcHead[32], 4);
    const unsigned merkleHeight = vChainMerkleBranch.size();
    if (nSize != (1 << merkleHeight))
        return error("Aux POW merkle branch size does not match parent coinbase");
    
    int nNonce;
    memcpy(&nNonce, &pcHead[36], 4);
    
    int nChainIndex;
    nChainIndex = getExpectedIndex(nNonce, params.nAuxpowChainId, merkleHeight);

    // get chain merkle root
    const uint256 nRootHash = CBlock::CheckMerkleBranch(hashAuxBlock, vChainMerkleBranch, nChainIndex);
    std::vector<unsigned char> vchRootHash(nRootHash.begin(), nRootHash.end());
    std::reverse(vchRootHash.begin(), vchRootHash.end()); // correct endian
    // check that chain merkle root is in the coinbase
    std::vector<unsigned char>::const_iterator pc =
    std::search(pcHead, pcHead + 32, vchRootHash.begin(), vchRootHash.end());
    if (pcHead != pc)
        return error("Merged mining header is not just before chain merkle root");
    
    // Check the coinbase is in the parent block merkle tree
    if (CBlock::CheckMerkleBranch(Hash(coinbaseTx.begin(), coinbaseTx.end()), vMerkleBranch, 0) != parentBlock.hashMerkleRoot) {
        return error("Aux POW merkle root incorrect");        
    }

    return true;
}

bool
CAuxPow::check2(const uint256& hashAuxBlock, const Consensus::Params& params) const
{
    if (vChainMerkleBranch.size() > 20)
        return error("Aux POW chain merkle branch too long");
    
    std::vector<unsigned char>::const_iterator pcHead =
        std::search(coinbaseTx.begin(), coinbaseTx.end(), UBEGIN(pchMergedMiningHeader), UEND(pchMergedMiningHeader));

    if (pcHead == coinbaseTx.end())
        return error("Aux POW missing MergedMiningHeader in parent coinbase");
    
    pcHead = pcHead + sizeof(pchMergedMiningHeader);

    int nSize;
    memcpy(&nSize, &pcHead[32], 4);
    unsigned int merkleHeight = vChainMerkleBranch.size();
    
    if (nSize != (1 << merkleHeight)) {
    //subtree
        std::vector<unsigned char> vchSubRoot(subRoot.begin(), subRoot.end());
        std::vector<unsigned char>::const_iterator pcSubHead = vchSubRoot.begin();
        
        int nSubHeight;
        memcpy(&nSubHeight, &pcSubHead[20], 4);
        //reset merkleHeight
        merkleHeight = merkleHeight - nSubHeight;
        
        if (nSize != (1 << merkleHeight) || merkleHeight < 0)
            return error("nSize = %d, merkleHeight = %d, not match", nSize, merkleHeight);
        
        int nSubNonce;
        memcpy(&nSubNonce, &pcSubHead[24], 4);
        
        int nSubChainIndex;
        nSubChainIndex = getExpectedIndex(nSubNonce, params.nSubAuxpowChainId, nSubHeight);
        
        std::vector<uint256> vSubChainMerkleBranch(vChainMerkleBranch.begin() + merkleHeight, vChainMerkleBranch.end());
        std::vector<uint256> vMainChainMerkleBranch(vChainMerkleBranch.begin(), vChainMerkleBranch.begin() + merkleHeight - 1);        
        
        //get subtree root
        uint256 nSubtreeRoot256;
        uint160 nSubtreeRoot160;       
        nSubtreeRoot256 = CBlock::CheckMerkleBranch(hashAuxBlock, vSubChainMerkleBranch, nSubChainIndex);
        nSubtreeRoot160 = Hash160(nSubtreeRoot256.begin(),nSubtreeRoot256.end());
        //find subtree root
        std::vector<unsigned char> vchSubtreeRootHash(nSubtreeRoot160.begin(), nSubtreeRoot160.end());
        // std::reverse(vchRootHash.begin(), vchRootHash.end()); // correct endian
        std::vector<unsigned char>::const_iterator pcSub =
        std::search(vchSubRoot.begin(), vchSubRoot.end(), vchSubtreeRootHash.begin(), vchSubtreeRootHash.end());
        if (pcSub != vchSubRoot.begin())
            return error("subRoot do not have subtree root hash");
        
        int nNonce;
        memcpy(&nNonce, &pcHead[36], 4);
        
        int nChainIndex;
        nChainIndex = getExpectedIndex(nNonce, params.nAuxpowChainId, merkleHeight);

        // get chain merkle root
        const uint256 nRootHash = CBlock::CheckMerkleBranch(subRoot, vMainChainMerkleBranch, nChainIndex);
        std::vector<unsigned char> vchRootHash(nRootHash.begin(), nRootHash.end());
        std::reverse(vchRootHash.begin(), vchRootHash.end()); // correct endian
        // check that chain merkle root is in the coinbase
        std::vector<unsigned char>::const_iterator pc =
        std::search(pcHead, pcHead + 32, vchRootHash.begin(), vchRootHash.end());
        if (pcHead != pc)
            return error("Merged mining header is not just before chain merkle root");
        
        // Check the coinbase is in the parent block merkle tree
        if (CBlock::CheckMerkleBranch(Hash(coinbaseTx.begin(), coinbaseTx.end()), vMerkleBranch, 0) != parentBlock.hashMerkleRoot) {
            return error("Aux POW merkle root incorrect");        
        }

        return true;       
    }
    
    int nNonce;
    memcpy(&nNonce, &pcHead[36], 4);
    
    int nChainIndex;
    nChainIndex = getExpectedIndex(nNonce, params.nAuxpowChainId, merkleHeight);

    // get chain merkle root
    const uint256 nRootHash = CBlock::CheckMerkleBranch(hashAuxBlock, vChainMerkleBranch, nChainIndex);
    std::vector<unsigned char> vchRootHash(nRootHash.begin(), nRootHash.end());
    std::reverse(vchRootHash.begin(), vchRootHash.end()); // correct endian
    // check that chain merkle root is in the coinbase
    std::vector<unsigned char>::const_iterator pc =
    std::search(pcHead, pcHead + 32, vchRootHash.begin(), vchRootHash.end());
    if (pcHead != pc)
        return error("Merged mining header is not just before chain merkle root");
    
    // Check the coinbase is in the parent block merkle tree
    if (CBlock::CheckMerkleBranch(Hash(coinbaseTx.begin(), coinbaseTx.end()), vMerkleBranch, 0) != parentBlock.hashMerkleRoot) {
        return error("Aux POW merkle root incorrect");        
    }

    return true;
}

int
CAuxPow::getExpectedIndex(int nNonce, int nChainId, unsigned h)
{
    // Choose a pseudo-random slot in the chain merkle tree
    // but have it be fixed for a size/nonce/chain combination.
    //
    // This prevents the same work from being used twice for the
    // same chain while reducing the chance that two chains clash
    // for the same slot.

    unsigned rand = nNonce;
    rand = rand * 1103515245 + 12345;
    rand += nChainId;
    rand = rand * 1103515245 + 12345;

    return rand % (1 << h);
}

std::string CPureTransaction::ToHex() const 
{
    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
    ssTx << (*this);
    return HexStr(ssTx.begin(), ssTx.end());
}