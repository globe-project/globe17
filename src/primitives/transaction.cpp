// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/transaction.h>

#include <hash.h>
#include <tinyformat.h>
#include <utilstrencodings.h>

std::string COutPoint::ToString() const
{
    return strprintf("COutPoint(%s, %u)", hash.ToString().substr(0,10), n);
}

CTxIn::CTxIn(COutPoint prevoutIn, CScript scriptSigIn, uint32_t nSequenceIn)
{
    prevout = prevoutIn;
    scriptSig = scriptSigIn;
    nSequence = nSequenceIn;
}

CTxIn::CTxIn(uint256 hashPrevTx, uint32_t nOut, CScript scriptSigIn, uint32_t nSequenceIn)
{
    prevout = COutPoint(hashPrevTx, nOut);
    scriptSig = scriptSigIn;
    nSequence = nSequenceIn;
}

std::string CTxIn::ToString() const
{
    std::string str;
    str += "CTxIn(";
    str += prevout.ToString();
    if (prevout.IsNull())
        str += strprintf(", coinbase %s", HexStr(scriptSig));
    else
        str += strprintf(", scriptSig=%s", HexStr(scriptSig).substr(0, 24));
    if (nSequence != SEQUENCE_FINAL)
        str += strprintf(", nSequence=%u", nSequence);
    str += ")";
    return str;
}

void CTxOutBase::SetValue(int64_t value)
{
    // convenience function intended for use with CTxOutStandard only
    assert(nVersion == OUTPUT_STANDARD);
    ((CTxOutStandard*) this)->nValue = value;
};

CAmount CTxOutBase::GetValue() const
{
    // convenience function intended for use with CTxOutStandard only
    /*
    switch (nVersion)
    {
        case OUTPUT_STANDARD:
            return ((CTxOutStandard*) this)->nValue;
        case OUTPUT_DATA:
            return 0;
        default:
            assert(false);
    };
    */
    assert(nVersion == OUTPUT_STANDARD);
    return ((CTxOutStandard*) this)->nValue;
};

std::string CTxOutBase::ToString() const
{
    switch (nVersion)
    {
        case OUTPUT_STANDARD:
        {
            CTxOutStandard *so = (CTxOutStandard*)this;
            return strprintf("CTxOutStandard(nValue=%d.%08d, scriptPubKey=%s)", so->nValue / COIN, so->nValue % COIN, HexStr(so->scriptPubKey).substr(0, 30));
        }
        case OUTPUT_DATA:
        {
            CTxOutData *dout = (CTxOutData*)this;
            return strprintf("CTxOutData(data=%s)", HexStr(dout->vData).substr(0, 30));
        }
        case OUTPUT_CT:
        {
            CTxOutCT *cto = (CTxOutCT*)this;
            return strprintf("CTxOutCT(data=%s, scriptPubKey=%s)", HexStr(cto->vData).substr(0, 30), HexStr(cto->scriptPubKey).substr(0, 30));
        }
        case OUTPUT_RINGCT:
        {
            CTxOutRingCT *rcto = (CTxOutRingCT*)this;
            return strprintf("CTxOutRingCT(data=%s, pk=%s)", HexStr(rcto->vData).substr(0, 30), HexStr(rcto->pk).substr(0, 30));
        }
        default:
            break;
    };
    return strprintf("CTxOutBase unknown version %d", nVersion);
}

CTxOutStandard::CTxOutStandard(const CAmount& nValueIn, CScript scriptPubKeyIn) : CTxOutBase(OUTPUT_STANDARD)
{
    nValue = nValueIn;
    scriptPubKey = scriptPubKeyIn;
};

void DeepCopy(CTxOutBaseRef &to, const CTxOutBaseRef &from)
{
    switch (from->GetType()) {
        case OUTPUT_STANDARD:
            to = MAKE_OUTPUT<CTxOutStandard>();
            *((CTxOutStandard*)to.get()) = *((CTxOutStandard*)from.get());
            break;
        case OUTPUT_CT:
            to = MAKE_OUTPUT<CTxOutCT>();
            *((CTxOutCT*)to.get()) = *((CTxOutCT*)from.get());
            break;
        case OUTPUT_RINGCT:
            to = MAKE_OUTPUT<CTxOutRingCT>();
            *((CTxOutRingCT*)to.get()) = *((CTxOutRingCT*)from.get());
            break;
        case OUTPUT_DATA:
            to = MAKE_OUTPUT<CTxOutData>();
            *((CTxOutData*)to.get()) = *((CTxOutData*)from.get());
            break;
        default:
            break;
    }
    return;
}

std::vector<CTxOutBaseRef> DeepCopy(const std::vector<CTxOutBaseRef> &from)
{
    std::vector<CTxOutBaseRef> vpout;
    vpout.resize(from.size());
    for (size_t i = 0; i < from.size(); ++i) {
        DeepCopy(vpout[i], from[i]);
    }

    return vpout;
}

CTxOut::CTxOut(const CAmount& nValueIn, CScript scriptPubKeyIn)
{
    nValue = nValueIn;
    scriptPubKey = scriptPubKeyIn;
}

std::string CTxOut::ToString() const
{
    return strprintf("CTxOut(nValue=%d.%08d, scriptPubKey=%s)", nValue / COIN, nValue % COIN, HexStr(scriptPubKey).substr(0, 30));
}

CMutableTransaction::CMutableTransaction() : nVersion(CTransaction::CURRENT_VERSION), nLockTime(0) {}
CMutableTransaction::CMutableTransaction(const CTransaction& tx) : vin(tx.vin), vout(tx.vout), vpout{DeepCopy(tx.vpout)}, nVersion(tx.nVersion), nLockTime(tx.nLockTime) {}

uint256 CMutableTransaction::GetHash() const
{
    return SerializeHash(*this, SER_GETHASH, SERIALIZE_TRANSACTION_NO_WITNESS);
}

uint256 CTransaction::ComputeHash() const
{
    return SerializeHash(*this, SER_GETHASH, SERIALIZE_TRANSACTION_NO_WITNESS);
}

uint256 CTransaction::ComputeWitnessHash() const
{
    if (!HasWitness()) {
        return hash;
    }
    return SerializeHash(*this, SER_GETHASH, 0);
}

/* For backward compatibility, the hash is initialized to 0. TODO: remove the need for this default constructor entirely. */
CTransaction::CTransaction() : vin(), vout(), vpout(), nVersion(CTransaction::CURRENT_VERSION), nLockTime(0), hash{}, m_witness_hash{} {}
CTransaction::CTransaction(const CMutableTransaction &tx) : vin(tx.vin), vout(tx.vout), vpout{DeepCopy(tx.vpout)} nVersion(tx.nVersion), nLockTime(tx.nLockTime), hash{ComputeHash()}, m_witness_hash{ComputeWitnessHash()} {}
CTransaction::CTransaction(CMutableTransaction &&tx) : vin(std::move(tx.vin)), vout(std::move(tx.vout)), vpout(std::move(tx.vpout)), nVersion(tx.nVersion), nLockTime(tx.nLockTime), hash{ComputeHash()}, m_witness_hash{ComputeWitnessHash()} {}

CAmount CTransaction::GetValueOut() const
{
    CAmount nValueOut = 0;
    for (const auto& tx_out : vout) {
        nValueOut += tx_out.nValue;
        if (!MoneyRange(tx_out.nValue) || !MoneyRange(nValueOut))
            throw std::runtime_error(std::string(__func__) + ": value out of range");
    }

    for (auto &txout : vpout)
    {
        if (!txout->IsStandardOutput())
            continue;

        CAmount nValue = txout->GetValue();
        nValueOut += txout->GetValue();
        if (!MoneyRange(nValue) || !MoneyRange(nValueOut))
            throw std::runtime_error(std::string(__func__) + ": value out of range");
    };

    return nValueOut;
}

CAmount CTransaction::GetPlainValueOut(size_t &nStandard, size_t &nCT, size_t &nRingCT) const
{
    // accumulators not cleared here intentionally
    CAmount nValueOut = 0;

    for (const auto &txout : vpout)
    {
        if (txout->IsType(OUTPUT_CT))
        {
            nCT++;
        } else
        if (txout->IsType(OUTPUT_RINGCT))
        {
            nRingCT++;
        };

        if (!txout->IsStandardOutput())
            continue;

        nStandard++;
        CAmount nValue = txout->GetValue();
        nValueOut += nValue;
        if (!MoneyRange(nValue) || !MoneyRange(nValueOut))
            throw std::runtime_error(std::string(__func__) + ": value out of range");
    };

    return nValueOut;
};

unsigned int CTransaction::GetTotalSize() const
{
    return ::GetSerializeSize(*this, SER_NETWORK, PROTOCOL_VERSION);
}

std::string CTransaction::ToString() const
{
    std::string str;
    str += strprintf("CTransaction(hash=%s, ver=%d, vin.size=%u, vout.size=%u, nLockTime=%u)\n",
        GetHash().ToString().substr(0,10),
        nVersion,
        vin.size(),
        vout.size(),
        nLockTime);
    for (const auto& tx_in : vin)
        str += "    " + tx_in.ToString() + "\n";
    for (const auto& tx_in : vin)
        str += "    " + tx_in.scriptWitness.ToString() + "\n";
    for (const auto& tx_out : vout)
        str += "    " + tx_out.ToString() + "\n";
    return str;
}

///////////////////////////////////////////////////////////// qtum
bool CTransaction::HasCreateOrCall() const{
    for(const CTxOut& v : vout){
        if(v.scriptPubKey.HasOpCreate() || v.scriptPubKey.HasOpCall()){
            return true;
        }
    }
    return false;
}



bool CTransaction::HasOpSpend() const{
    for(const CTxIn& i : vin){
        if(i.scriptSig.HasOpSpend()){
            return true;
        }
    }
    return false;
}
/////////////////////////////////////////////////////////////
