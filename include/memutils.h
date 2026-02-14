#pragma once

#include <stddef.h>
#include <stdint.h>

void* memcpy_u8(uint8_t* dest, const uint8_t* src, size_t n);
void* memset_u8(uint8_t* s, int c, size_t n);
