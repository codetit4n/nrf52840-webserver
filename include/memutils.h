#pragma once

#include <stddef.h>
#include <stdint.h>

void* mem_cpy(void* dest, const void* src, size_t n);
void* mem_set(void* s, int c, size_t n);
int mem_cmp(const void* s1, const void* s2, size_t n);
