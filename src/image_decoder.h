#pragma once
#include <stdint.h>
// Decode and scale bounded image data to 0x00RRGGBB. Caller owns returned pixels.
uint32_t* browserDecodeImage(const unsigned char* data,int bytes,int* width,int* height);
