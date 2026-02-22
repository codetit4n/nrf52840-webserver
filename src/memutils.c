#include "memutils.h"

void* mem_cpy(void* dest, const void* src, size_t n) {
	uint8_t* d = (uint8_t*)dest;
	const uint8_t* s = (const uint8_t*)src;

	while (n--) {
		*d++ = *s++;
	}

	return dest;
}

void* mem_set(void* s, int c, size_t n) {
	uint8_t* p = (uint8_t*)s;

	while (n--) {
		*p++ = (uint8_t)c;
	}

	return s;
}

int mem_cmp(const void* s1, const void* s2, size_t n) {
	const uint8_t* p1 = (const uint8_t*)s1;
	const uint8_t* p2 = (const uint8_t*)s2;

	for (size_t i = 0; i < n; i++) {
		if (p1[i] != p2[i]) {
			return p1[i] - p2[i];
		}
	}

	return 0;
}
