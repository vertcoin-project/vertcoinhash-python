#ifndef VERTHASH_H2_INCLUDED
#define VERTHASH_H2_INCLUDED

#include <stdint.h>

void verthash_hash(const unsigned char* blob_bytes, const size_t blob_size, const unsigned char* input, unsigned char* output);

#endif