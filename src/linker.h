#ifndef LINKER_H
#define LINKER_H

#include <stdint.h>

#define CODE_START 0xD000
#define CODE_MAX_SIZE 8192

void linker_reset();
int linker_emit(const uint8_t *bytes, uint8_t length);
void linker_run();

#endif