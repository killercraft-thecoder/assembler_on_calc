#ifndef OPCODES_H
#define OPCODES_H

#include <stdint.h>

typedef enum {
    OP_NONE,
    OP_IMM8,
    OP_IMM16,
    OP_IMM24,
    OP_NOARG
} OperandType;

typedef struct {
    const char *mnemonic;
    uint8_t opcode[5];     // Max 5 bytes
    uint8_t length;        // Number of bytes
    OperandType type;
} Instruction;

#define INSTRUCTION_COUNT 335

extern const Instruction instruction_table[INSTRUCTION_COUNT];

const Instruction *lookup_instruction(const char *mnemonic);

#endif