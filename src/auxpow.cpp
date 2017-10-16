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

/* Moved from wallet.cpp.  CMerkleTx is necessary for auxpow, independent
   of an enabled (or disabled) wallet.  Always include the code.  */

int CMerkleTx::SetMerkleBranch(const CBlock& block)
{
    AssertLockHeld(cs_main);
    CBlock blockTmp;

    // Update the tx's hashBlock
    hashBlock = block.GetHash();

    // Locate the transaction
    for (nIndex = 0; nIndex < (int)block.vtx.size(); nIndex++)
        if (block.vtx[nIndex] == *(CTransaction*)this)
            break;
    if (nIndex == (int)block.vtx.size()) {
        vMerkleBranch.clear();
        nIndex = -1;
        LogPrintf("ERROR: SetMerkleBranch(): couldn't find tx in block\n");
        return 0;
    }

    // Fill in merkle branch
    vMerkleBranch = block.GetMerkleBranch(nIndex);

    // Is the tx in a block that's in the main chain
    BlockMap::iterator mi = mapBlockIndex.find(hashBlock);
    if (mi == mapBlockIndex.end())
        return 0;
    const CBlockIndex* pindex = (*mi).second;
    if (!pindex || !chainActive.Contains(pindex))
        return 0;

    return chainActive.Height() - pindex->nHeight + 1;
}

int CMerkleTx::GetDepthInMainChainINTERNAL(const CBlockIndex*& pindexRet) const
{
    if (hashBlock.IsNull() || nIndex == -1)
        return 0;
    AssertLockHeld(cs_main);

    // Find the block it claims to be in
    BlockMap::iterator mi = mapBlockIndex.find(hashBlock);
    if (mi == mapBlockIndex.end())
        return 0;
    CBlockIndex* pindex = (*mi).second;
    if (!pindex || !chainActive.Contains(pindex))
        return 0;

    // Make sure the merkle branch connects to this block
    if (!fMerkleVerified) {
        if (CBlock::CheckMerkleBranch(GetHash(), vMerkleBranch, nIndex) != pindex->hashMerkleRoot)
            return 0;
        fMerkleVerified = true;
    }

    pindexRet = pindex;
    return chainActive.Height() - pindex->nHeight + 1;
}

int CMerkleTx::GetDepthInMainChain(const CBlockIndex*& pindexRet) const
{
    AssertLockHeld(cs_main);
    int nResult = GetDepthInMainChainINTERNAL(pindexRet);
    if (nResult == 0 && !mempool.exists(GetHash()))
        return -1; // Not in chain, not in mempool

    return nResult;
}

int CMerkleTx::GetBlocksToMaturity() const
{
    if (!IsCoinBase())
        return 0;
    return std::max(0, (COINBASE_MATURITY + 1) - GetDepthInMainChain());
}


bool CMerkleTx::AcceptToMemoryPool(bool fLimitFree, bool fRejectAbsurdFee)
{
    CValidationState state;
    return ::AcceptToMemoryPool(mempool, state, *this, fLimitFree, NULL, fRejectAbsurdFee);
}

CPureTransaction::CPureTransaction() : nVersion(CTransaction::MIN_CURRENT_VERSION), nLockTime(0) {}

uint256 CPureTransaction::GetHash() const
{
    return SerializeHash(*this);
}

std::string CPureTransaction::ToString() const
{
    std::string str;
    str += strprintf("CPureTransaction(hash=%s, ver=%d, vin.size=%u, vout.size=%u, nLockTime=%u)\n",
        GetHash().ToString().substr(0,10),
        nVersion,
        vin.size(),
        vout.size(),
        nLockTime);
    for (unsigned int i = 0; i < vin.size(); i++)
        str += "    " + vin[i].ToString() + "\n";
    for (unsigned int i = 0; i < vout.size(); i++)
        str += "    " + vout[i].ToString() + "\n";
    return str;
}

/* ************************************************************************** */

bool
CAuxPow::check(const uint256& hashAuxBlock, const Consensus::Params& params) const
{
    if (nIndex != 0)
        return error("AuxPow is not a generate");

    if (vChainMerkleBranch.size() > 30)
        return error("Aux POW chain merkle branch too long");

    // Check that the chain merkle root is in the coinbase
    const uint256 nRootHash = CBlock::CheckMerkleBranch(hashAuxBlock, vChainMerkleBranch, nChainIndex);
    std::vector<unsigned char> vchRootHash(nRootHash.begin(), nRootHash.end());
    std::reverse(vchRootHash.begin(), vchRootHash.end()); // correct endian
    
    // Check that we are in the parent block merkle tree
    if (CBlock::CheckMerkleBranch(coinbaseTx.GetHash(), vMerkleBranch, nIndex) != parentBlock.hashMerkleRoot)
        return error("Aux POW merkle root incorrect");

    const CScript script = coinbaseTx.vin[0].scriptSig;

    CScript::const_iterator pcHead =
        std::search(script.begin(), script.end(), UBEGIN(pchMergedMiningHeader), UEND(pchMergedMiningHeader));

    if (pcHead == script.end())
        return error("Aux POW missing MergedMiningHeader in parent coinbase");
    
    CScript::const_iterator pc =
        std::search(script.begin(), script.end(), vchRootHash.begin(), vchRootHash.end());

    if (pc == script.end())
        return error("Aux POW missing chain merkle root in parent coinbase");

    if (pcHead + sizeof(pchMergedMiningHeader) != pc)
        return error("Merged mining header is not just before chain merkle root");    
    
    return true;
}
