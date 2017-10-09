// Copyright (c) 2015 The Dogecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "arith_uint256.h"
#include "uint256.h"
#include "dogecoin.h"
#include "main.h"
#include "util.h"

bool CheckAuxPowProofOfWork(const CBlockHeader& block, const Consensus::Params& params)
{
    if (block.hashPrevBlock.IsNull()) {
        return true;
    }

    /* If there is no auxpow, just check the block hash.  */
    if (!block.auxpow) {
        if (block.nVersion.IsAuxpow())
            return error("%s : no auxpow on block with auxpow version",
                         __func__);

        if (!CheckProofOfWork(block.GetPoWHash(), block.nBits, params))
            return error("%s : non-AUX proof of work failed", __func__);

        return true;
    }

    /* We have auxpow.  Check it.  */

    if (!block.nVersion.IsAuxpow())
        return error("%s : auxpow on block with non-auxpow version", __func__);

    if (!block.auxpow->check(block.GetHash(), params))
        return error("%s : AUX POW is not valid", __func__);
    if (!CheckProofOfWork(block.auxpow->getParentBlockPoWHash(), block.nBits, params))
        return error("%s : AUX proof of work failed", __func__);

    return true;
}

