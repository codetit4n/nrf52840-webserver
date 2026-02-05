#pragma once

#include <stddef.h>
#include <stdint.h>

void uarte_write(const char* text, size_t len);
void uarte_init(void);
