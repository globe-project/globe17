//
// Created by digitalstorm on 10/25/18.
//

#ifndef GLOBE_BLIND_H
#define GLOBE_BLIND_H

#include <secp256k1.h>
#include <inttypes.h>
#include <vector>

#include <amount.h>

extern secp256k1_context *secp256k1_ctx_blind;

int SelectRangeProofParameters(uint64_t nValueIn, uint64_t &minValue, int &exponent, int &nBits);

int GetRangeProofInfo(const std::vector<uint8_t> &vRangeproof, int &rexp, int &rmantissa, CAmount &min_value, CAmount &max_value);

void ECC_Start_Blinding();
void ECC_Stop_Blinding();

#endif // GLOBE_BLIND_H