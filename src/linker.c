#include "linker.h"
#include <tice.h>
#include <fileioc.h>
#include <string.h>

#ifdef __INTELLISENSE__
typedef unsigned long uint24_t;
#define true 1
#define false 0
#endif

static uint8_t *code_ptr = (uint8_t*)CODE_START;
static size_t code_size = 0; // Track how many bytes were emitted

void linker_reset(void) {
    code_ptr = (uint8_t*)CODE_START;
    code_size = 0;
}

int linker_emit(const uint8_t *bytes, uint8_t length) {
    if ((code_ptr + length) > (uint8_t*)(CODE_START + CODE_MAX_SIZE)) {
        return 0; // Overflow
    }
    for (uint8_t i = 0; i < length; i++) {
        *code_ptr++ = bytes[i];
    }
    code_size += length;
    return 1;
}

static void linker_save_to_vat(const char *name) {
    // Delete any existing program with this name
    ti_Delete(name);

    // Create a new program in RAM
    ti_var_t slot = ti_Open(name, "w");
    if (!slot) return; // Failed to create

    // Write the assembled code into the VAT program
    ti_Write((void*)CODE_START, 1, code_size, slot);

    // Optionally archive it
    ti_SetArchiveStatus(true, slot);

    ti_Close(slot);
}

void linker_run(void) {
    // Save to VAT before running
    linker_save_to_vat("BUILT");

    // Jump to assembled code in RAM
    ((void(*)())CODE_START)();
}