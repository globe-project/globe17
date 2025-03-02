#ifndef GLOBE_KEYUTIL_H
#define GLOBE_KEYUTIL_H

#include <vector>
#include <inttypes.h>

uint32_t BitcoinChecksum(uint8_t *p, uint32_t nBytes);
void AppendChecksum(std::vector<uint8_t> &data);
bool VerifyChecksum(const std::vector<uint8_t> &data);


#endif // GLOBE_KEYUTIL_H