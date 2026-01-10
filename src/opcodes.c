#include "opcodes.h"
#include <string.h>

// ~256 Z80-compatible instructions
const Instruction instruction_table[INSTRUCTION_COUNT] = {
    // LD r, n (8-bit immediate loads)
    {"ld a", {0x3E, 0x00}, 2, OP_IMM8},
    {"ld b", {0x06, 0x00}, 2, OP_IMM8},
    {"ld c", {0x0E, 0x00}, 2, OP_IMM8},
    {"ld d", {0x16, 0x00}, 2, OP_IMM8},
    {"ld e", {0x1E, 0x00}, 2, OP_IMM8},
    {"ld h", {0x26, 0x00}, 2, OP_IMM8},
    {"ld l", {0x2E, 0x00}, 2, OP_IMM8},

    // ALU ops with immediate
    {"add a", {0xC6, 0x00}, 2, OP_IMM8},
    {"sub", {0xD6, 0x00}, 2, OP_IMM8},
    {"and", {0xE6, 0x00}, 2, OP_IMM8},
    {"or", {0xF6, 0x00}, 2, OP_IMM8},
    {"xor", {0xEE, 0x00}, 2, OP_IMM8},

    // INC/DEC
    {"inc a", {0x3C}, 1, OP_NOARG},
    {"dec a", {0x3D}, 1, OP_NOARG},

    // Control flow
    {"jp", {0xC3, 0x00, 0x00}, 3, OP_IMM16},
    {"call", {0xCD, 0x00, 0x00}, 3, OP_IMM16},
    {"ret", {0xC9}, 1, OP_NOARG},
    {"nop", {0x00}, 1, OP_NOARG},
    {"halt", {0x76}, 1, OP_NOARG},

    // Stack ops
    {"push af", {0xF5}, 1, OP_NOARG},
    {"pop af", {0xF1}, 1, OP_NOARG},
    {"push bc", {0xC5}, 1, OP_NOARG},
    {"pop bc", {0xC1}, 1, OP_NOARG},
    {"push de", {0xD5}, 1, OP_NOARG},
    {"pop de", {0xD1}, 1, OP_NOARG},
    {"push hl", {0xE5}, 1, OP_NOARG},
    {"pop hl", {0xE1}, 1, OP_NOARG},

    // Memory ops
    {"ld (hl),n", {0x36, 0x00}, 2, OP_IMM8},
    {"ld a,(hl)", {0x7E}, 1, OP_NOARG},
    {"ld (hl),a", {0x77}, 1, OP_NOARG},

    // LD r, r (8-bit register-to-register loads)
    {"ld a,b", {0x78}, 1, OP_NOARG},
    {"ld a,c", {0x79}, 1, OP_NOARG},
    {"ld a,d", {0x7A}, 1, OP_NOARG},
    {"ld a,e", {0x7B}, 1, OP_NOARG},
    {"ld a,h", {0x7C}, 1, OP_NOARG},
    {"ld a,l", {0x7D}, 1, OP_NOARG},

    // INC/DEC for other registers
    {"inc b", {0x04}, 1, OP_NOARG},
    {"dec b", {0x05}, 1, OP_NOARG},
    {"inc c", {0x0C}, 1, OP_NOARG},
    {"dec c", {0x0D}, 1, OP_NOARG},

    // ALU ops with registers (ADD A, r)
    {"add a,b", {0x80}, 1, OP_NOARG},
    {"add a,c", {0x81}, 1, OP_NOARG},
    {"add a,d", {0x82}, 1, OP_NOARG},
    {"add a,e", {0x83}, 1, OP_NOARG},

    // ALU ops with registers (SUB, AND, OR, XOR with registers)
    {"sub b", {0x90}, 1, OP_NOARG},
    {"sub c", {0x91}, 1, OP_NOARG},
    {"and b", {0xA0}, 1, OP_NOARG},
    {"and c", {0xA1}, 1, OP_NOARG},
    {"or b", {0xB0}, 1, OP_NOARG},
    {"or c", {0xB1}, 1, OP_NOARG},
    {"xor b", {0xA8}, 1, OP_NOARG},
    {"xor c", {0xA9}, 1, OP_NOARG},

    // 16-bit register loads (immediate)
    {"ld bc,nn", {0x01, 0x00, 0x00}, 3, OP_IMM16},
    {"ld de,nn", {0x11, 0x00, 0x00}, 3, OP_IMM16},
    {"ld hl,nn", {0x21, 0x00, 0x00}, 3, OP_IMM16},
    {"ld sp,nn", {0x31, 0x00, 0x00}, 3, OP_IMM16},

    // Relative jumps
    {"jr e", {0x18, 0x00}, 2, OP_IMM8},
    {"jr nz,e", {0x20, 0x00}, 2, OP_IMM8},
    {"jr z,e", {0x28, 0x00}, 2, OP_IMM8},
    {"jr nc,e", {0x30, 0x00}, 2, OP_IMM8},
    {"jr c,e", {0x38, 0x00}, 2, OP_IMM8},

    // LD A,(rr) and LD (rr),A — common memory-indirect loads/stores
    {"ld a,(bc)", {0x0A}, 1, OP_NOARG},
    {"ld a,(de)", {0x1A}, 1, OP_NOARG},
    {"ld (bc),a", {0x02}, 1, OP_NOARG},
    {"ld (de),a", {0x12}, 1, OP_NOARG},

    // 16-bit arithmetic
    {"add hl,bc", {0x09}, 1, OP_NOARG},
    {"add hl,de", {0x19}, 1, OP_NOARG},
    {"add hl,hl", {0x29}, 1, OP_NOARG},
    {"add hl,sp", {0x39}, 1, OP_NOARG},

    // Rotate/shift accumulator
    {"rlca", {0x07}, 1, OP_NOARG},
    {"rrca", {0x0F}, 1, OP_NOARG},
    {"rla", {0x17}, 1, OP_NOARG},
    {"rra", {0x1F}, 1, OP_NOARG},

    // Compare accumulator with register
    {"cp a", {0xBF}, 1, OP_NOARG},
    {"cp b", {0xB8}, 1, OP_NOARG},
    {"cp c", {0xB9}, 1, OP_NOARG},
    {"cp d", {0xBA}, 1, OP_NOARG},

    // SBC (Subtract with Carry) - register and immediate
    {"sbc a,b", {0x98}, 1, OP_NOARG},
    {"sbc a,c", {0x99}, 1, OP_NOARG},
    {"sbc a,d", {0x9A}, 1, OP_NOARG},
    {"sbc a,e", {0x9B}, 1, OP_NOARG},
    {"sbc a,h", {0x9C}, 1, OP_NOARG},
    {"sbc a,l", {0x9D}, 1, OP_NOARG},
    {"sbc a,a", {0x9F}, 1, OP_NOARG},
    {"sbc a,n", {0xDE, 0x00}, 2, OP_IMM8},

    // INC/DEC on index registers (Z80 + eZ80)
    {"inc ix", {0xDD, 0x23}, 2, OP_NOARG},
    {"dec ix", {0xDD, 0x2B}, 2, OP_NOARG},
    {"inc iy", {0xFD, 0x23}, 2, OP_NOARG},
    {"dec iy", {0xFD, 0x2B}, 2, OP_NOARG},

    // LD SP,HL / LD SP,IX / LD SP,IY
    {"ld sp,hl", {0xF9}, 1, OP_NOARG},
    {"ld sp,ix", {0xDD, 0xF9}, 2, OP_NOARG},
    {"ld sp,iy", {0xFD, 0xF9}, 2, OP_NOARG},

    // POP/ PUSH IX / IY
    {"push ix", {0xDD, 0xE5}, 2, OP_NOARG},
    {"pop ix", {0xDD, 0xE1}, 2, OP_NOARG},
    {"push iy", {0xFD, 0xE5}, 2, OP_NOARG},
    {"pop iy", {0xFD, 0xE1}, 2, OP_NOARG},

    // Block transfer instructions
    {"ldi", {0xED, 0xA0}, 2, OP_NOARG},
    {"ldd", {0xED, 0xA8}, 2, OP_NOARG},
    {"ldir", {0xED, 0xB0}, 2, OP_NOARG},
    {"lddr", {0xED, 0xB8}, 2, OP_NOARG},

    // Block compare instructions
    {"cpi", {0xED, 0xA1}, 2, OP_NOARG},
    {"cpd", {0xED, 0xA9}, 2, OP_NOARG},
    {"cpir", {0xED, 0xB1}, 2, OP_NOARG},
    {"cpdr", {0xED, 0xB9}, 2, OP_NOARG},

    // Bit test
    {"bit 0,b", {0xCB, 0x40}, 2, OP_NOARG},
    {"bit 7,a", {0xCB, 0x7F}, 2, OP_NOARG},

    // Bit set/reset
    {"set 0,b", {0xCB, 0xC0}, 2, OP_NOARG},
    {"res 0,b", {0xCB, 0x80}, 2, OP_NOARG},

    // Conditional returns
    {"ret nz", {0xC0}, 1, OP_NOARG},
    {"ret z", {0xC8}, 1, OP_NOARG},
    {"ret nc", {0xD0}, 1, OP_NOARG},
    {"ret c", {0xD8}, 1, OP_NOARG},

    // Conditional calls
    {"call nz,nn", {0xC4, 0x00, 0x00}, 3, OP_IMM16},
    {"call z,nn", {0xCC, 0x00, 0x00}, 3, OP_IMM16},
    {"call nc,nn", {0xD4, 0x00, 0x00}, 3, OP_IMM16},
    {"call c,nn", {0xDC, 0x00, 0x00}, 3, OP_IMM16},

    // Remaining ADD A,r variants
    {"add a,h", {0x84}, 1, OP_NOARG},
    {"add a,l", {0x85}, 1, OP_NOARG},

    // Remaining SUB r variants
    {"sub d", {0x92}, 1, OP_NOARG},
    {"sub e", {0x93}, 1, OP_NOARG},
    {"sub h", {0x94}, 1, OP_NOARG},
    {"sub l", {0x95}, 1, OP_NOARG},

    // Rotate/shift on registers (CB prefix)
    {"rl b", {0xCB, 0x10}, 2, OP_NOARG},
    {"rr b", {0xCB, 0x18}, 2, OP_NOARG},
    {"sla b", {0xCB, 0x20}, 2, OP_NOARG},
    {"sra b", {0xCB, 0x28}, 2, OP_NOARG},
    {"srl b", {0xCB, 0x38}, 2, OP_NOARG},

    // More BIT/SET/RES examples
    {"bit 1,c", {0xCB, 0x49}, 2, OP_NOARG},
    {"set 1,c", {0xCB, 0xC9}, 2, OP_NOARG},
    {"res 1,c", {0xCB, 0x89}, 2, OP_NOARG},

    // Handy load/store variants
    {"ld a,(nn)", {0x3A, 0x00, 0x00}, 3, OP_IMM16},
    {"ld (nn),a", {0x32, 0x00, 0x00}, 3, OP_IMM16},

    // AND register variants
    {"and d", {0xA2}, 1, OP_NOARG},
    {"and e", {0xA3}, 1, OP_NOARG},
    {"and h", {0xA4}, 1, OP_NOARG},
    {"and l", {0xA5}, 1, OP_NOARG},

    // OR register variants
    {"or d", {0xB2}, 1, OP_NOARG},
    {"or e", {0xB3}, 1, OP_NOARG},
    {"or h", {0xB4}, 1, OP_NOARG},
    {"or l", {0xB5}, 1, OP_NOARG},

    // XOR register variants
    {"xor d", {0xAA}, 1, OP_NOARG},
    {"xor e", {0xAB}, 1, OP_NOARG},
    {"xor h", {0xAC}, 1, OP_NOARG},
    {"xor l", {0xAD}, 1, OP_NOARG},

    // CP register variants
    {"cp e", {0xBB}, 1, OP_NOARG},
    {"cp h", {0xBC}, 1, OP_NOARG},
    {"cp l", {0xBD}, 1, OP_NOARG},

    // Indexed memory loads (IX+d)
    {"ld a,(ix+0)", {0xDD, 0x7E, 0x00}, 3, OP_IMM8},
    {"ld (ix+0),a", {0xDD, 0x77, 0x00}, 3, OP_IMM8},

    // Indexed memory loads (IY+d)
    {"ld a,(iy+0)", {0xFD, 0x7E, 0x00}, 3, OP_IMM8},
    {"ld (iy+0),a", {0xFD, 0x77, 0x00}, 3, OP_IMM8},

    // Interrupt control
    {"di", {0xF3}, 1, OP_NOARG},
    {"ei", {0xFB}, 1, OP_NOARG},

    // Flag operations
    {"cpl", {0x2F}, 1, OP_NOARG},
    {"scf", {0x37}, 1, OP_NOARG},
    {"ccf", {0x3F}, 1, OP_NOARG},

    // Exchange instructions
    {"ex de,hl", {0xEB}, 1, OP_NOARG},
    {"ex af,af'", {0x08}, 1, OP_NOARG},
    {"exx", {0xD9}, 1, OP_NOARG},

    // Exchange with stack
    {"ex (sp),hl", {0xE3}, 1, OP_NOARG},
    {"ex (sp),ix", {0xDD, 0xE3}, 2, OP_NOARG},
    {"ex (sp),iy", {0xFD, 0xE3}, 2, OP_NOARG},

    // Input/Output
    {"in a,(n)", {0xDB, 0x00}, 2, OP_IMM8},
    {"out (n),a", {0xD3, 0x00}, 2, OP_IMM8},

    // Indexed arithmetic (IX+d)
    {"add a,(ix+0)", {0xDD, 0x86, 0x00}, 3, OP_IMM8},
    {"sub (ix+0)", {0xDD, 0x96, 0x00}, 3, OP_IMM8},

    // Indexed arithmetic (IY+d)
    {"add a,(iy+0)", {0xFD, 0x86, 0x00}, 3, OP_IMM8},
    {"sub (iy+0)", {0xFD, 0x96, 0x00}, 3, OP_IMM8},

    // Restart instructions
    {"rst 00h", {0xC7}, 1, OP_NOARG},
    {"rst 08h", {0xCF}, 1, OP_NOARG},
    {"rst 10h", {0xD7}, 1, OP_NOARG},
    {"rst 18h", {0xDF}, 1, OP_NOARG},

    // 24-bit load/store (ADL mode)
    {"ld hl,(nnnnnn)", {0xED, 0x6B, 0x00, 0x00, 0x00}, 5, OP_IMM24},
    {"ld (nnnnnn),hl", {0xED, 0x63, 0x00, 0x00, 0x00}, 5, OP_IMM24},
    {"ld de,(nnnnnn)", {0xED, 0x5B, 0x00, 0x00, 0x00}, 5, OP_IMM24},
    {"ld (nnnnnn),de", {0xED, 0x53, 0x00, 0x00, 0x00}, 5, OP_IMM24},

    // 24-bit stack pointer load/store
    {"ld sp,(nnnnnn)", {0xED, 0x7B, 0x00, 0x00, 0x00}, 5, OP_IMM24},
    {"ld (nnnnnn),sp", {0xED, 0x73, 0x00, 0x00, 0x00}, 5, OP_IMM24},

    // Extended arithmetic with 24-bit registers
    {"add hl,sp", {0x39}, 1, OP_NOARG}, // works in ADL as 24-bit
    {"adc hl,sp", {0xED, 0x7A}, 2, OP_NOARG},
    {"sbc hl,sp", {0xED, 0x72}, 2, OP_NOARG},

    // Indexed load/store with 24-bit displacement
    {"ld a,(ix+nn)", {0xDD, 0x7E, 0x00, 0x00}, 4, OP_IMM16},
    {"ld (ix+nn),a", {0xDD, 0x77, 0x00, 0x00}, 4, OP_IMM16},
    {"ld a,(iy+nn)", {0xFD, 0x7E, 0x00, 0x00}, 4, OP_IMM16},
    {"ld (iy+nn),a", {0xFD, 0x77, 0x00, 0x00}, 4, OP_IMM16},

    // Multiplication (eZ80 only)
    {"mlt bc", {0xED, 0x4C}, 2, OP_NOARG},
    {"mlt de", {0xED, 0x5C}, 2, OP_NOARG},
    {"mlt hl", {0xED, 0x6C}, 2, OP_NOARG},
    {"mlt sp", {0xED, 0x7C}, 2, OP_NOARG},

    // Swap bytes in register (eZ80 only)
    {"swapnib a", {0xED, 0x23}, 2, OP_NOARG},

    // 24-bit block transfer (ADL mode)
    {"ldirx", {0xED, 0xB4}, 2, OP_NOARG}, // LDIR but with IX/IY in ADL
    {"lddrx", {0xED, 0xBC}, 2, OP_NOARG}, // LDDR with IX/IY in ADL

    // 24-bit block compare (ADL mode)
    {"cpirx", {0xED, 0xB5}, 2, OP_NOARG},
    {"cpdrx", {0xED, 0xBD}, 2, OP_NOARG},

    // 24-bit immediate loads to registers
    {"ld bc,nnnnnn", {0x01, 0x00, 0x00, 0x00}, 4, OP_IMM24},
    {"ld de,nnnnnn", {0x11, 0x00, 0x00, 0x00}, 4, OP_IMM24},
    {"ld hl,nnnnnn", {0x21, 0x00, 0x00, 0x00}, 4, OP_IMM24},
    {"ld sp,nnnnnn", {0x31, 0x00, 0x00, 0x00}, 4, OP_IMM24},

    // 24-bit arithmetic with registers
    {"adc hl,bc", {0xED, 0x4A}, 2, OP_NOARG},
    {"adc hl,de", {0xED, 0x5A}, 2, OP_NOARG},
    {"adc hl,hl", {0xED, 0x6A}, 2, OP_NOARG},
    {"adc hl,sp", {0xED, 0x7A}, 2, OP_NOARG},

    // Test instructions (eZ80 only)
    {"tst a", {0xED, 0x3C}, 2, OP_NOARG},
    {"tst b", {0xED, 0x04}, 2, OP_NOARG},
    {"tst c", {0xED, 0x0C}, 2, OP_NOARG},

    // Push immediate (eZ80 only)
    {"push nn", {0xED, 0x8A, 0x00, 0x00}, 4, OP_IMM16},
    {"push nnnnnn", {0xED, 0x8B, 0x00, 0x00, 0x00}, 5, OP_IMM24},

    // More conditional jumps (absolute)
    {"jp nz,nn", {0xC2, 0x00, 0x00}, 3, OP_IMM16},
    {"jp z,nn", {0xCA, 0x00, 0x00}, 3, OP_IMM16},
    {"jp nc,nn", {0xD2, 0x00, 0x00}, 3, OP_IMM16},
    {"jp c,nn", {0xDA, 0x00, 0x00}, 3, OP_IMM16},

    // More conditional calls
    {"call po,nn", {0xE4, 0x00, 0x00}, 3, OP_IMM16},
    {"call pe,nn", {0xEC, 0x00, 0x00}, 3, OP_IMM16},
    {"call p,nn", {0xF4, 0x00, 0x00}, 3, OP_IMM16},
    {"call m,nn", {0xFC, 0x00, 0x00}, 3, OP_IMM16},

    // More conditional returns
    {"ret po", {0xE0}, 1, OP_NOARG},
    {"ret pe", {0xE8}, 1, OP_NOARG},
    {"ret p", {0xF0}, 1, OP_NOARG},
    {"ret m", {0xF8}, 1, OP_NOARG},

    // Rotate/shift accumulator (common in bit twiddling)
    {"rlca", {0x07}, 1, OP_NOARG},
    {"rrca", {0x0F}, 1, OP_NOARG},
    {"rla", {0x17}, 1, OP_NOARG},
    {"rra", {0x1F}, 1, OP_NOARG},

    // Exchange accumulator and flags with alternate set
    {"ex af,af'", {0x08}, 1, OP_NOARG},

    // Load HL from (nn) and store HL to (nn)
    {"ld hl,(nn)", {0x2A, 0x00, 0x00}, 3, OP_IMM16},
    {"ld (nn),hl", {0x22, 0x00, 0x00}, 3, OP_IMM16},

    // eZ80 LEA instructions (24-bit displacement)
    {"lea bc,ix+nn", {0xDD, 0x01, 0x00, 0x00}, 4, OP_IMM16},
    {"lea bc,iy+nn", {0xFD, 0x01, 0x00, 0x00}, 4, OP_IMM16},
    {"lea de,ix+nn", {0xDD, 0x11, 0x00, 0x00}, 4, OP_IMM16},
    {"lea de,iy+nn", {0xFD, 0x11, 0x00, 0x00}, 4, OP_IMM16},
    {"lea hl,ix+nn", {0xDD, 0x21, 0x00, 0x00}, 4, OP_IMM16},
    {"lea hl,iy+nn", {0xFD, 0x21, 0x00, 0x00}, 4, OP_IMM16},
    {"lea sp,ix+nn", {0xDD, 0x31, 0x00, 0x00}, 4, OP_IMM16},
    {"lea sp,iy+nn", {0xFD, 0x31, 0x00, 0x00}, 4, OP_IMM16},

    // eZ80 push/pop in ADL mode (24-bit regs)
    {"push bc", {0xC5}, 1, OP_NOARG},
    {"push de", {0xD5}, 1, OP_NOARG},
    {"push hl", {0xE5}, 1, OP_NOARG},
    {"push af", {0xF5}, 1, OP_NOARG},
    {"pop bc", {0xC1}, 1, OP_NOARG},
    {"pop de", {0xD1}, 1, OP_NOARG},
    {"pop hl", {0xE1}, 1, OP_NOARG},
    {"pop af", {0xF1}, 1, OP_NOARG},

    // IX/IY 24-bit load/store
    {"ld ix,nnnnnn", {0xDD, 0x21, 0x00, 0x00, 0x00}, 5, OP_IMM24},
    {"ld iy,nnnnnn", {0xFD, 0x21, 0x00, 0x00, 0x00}, 5, OP_IMM24},
    {"ld ix,(nnnnnn)", {0xDD, 0x2A, 0x00, 0x00, 0x00}, 5, OP_IMM24},
    {"ld iy,(nnnnnn)", {0xFD, 0x2A, 0x00, 0x00, 0x00}, 5, OP_IMM24},
    {"ld (nnnnnn),ix", {0xDD, 0x22, 0x00, 0x00, 0x00}, 5, OP_IMM24},
    {"ld (nnnnnn),iy", {0xFD, 0x22, 0x00, 0x00, 0x00}, 5, OP_IMM24},

    // Block I/O
    {"ini", {0xED, 0xA2}, 2, OP_NOARG},  // IN (C), (HL) then HL++, B--
    {"ind", {0xED, 0xAA}, 2, OP_NOARG},  // IN (C), (HL) then HL--, B--
    {"outi", {0xED, 0xA3}, 2, OP_NOARG}, // OUT (C), (HL) then HL++, B--
    {"outd", {0xED, 0xAB}, 2, OP_NOARG}, // OUT (C), (HL) then HL--, B--

    // Repeated block I/O
    {"inir", {0xED, 0xB2}, 2, OP_NOARG}, // Repeat INI until B=0
    {"indr", {0xED, 0xBA}, 2, OP_NOARG}, // Repeat IND until B=0
    {"otir", {0xED, 0xB3}, 2, OP_NOARG}, // Repeat OUTI until B=0
    {"otdr", {0xED, 0xBB}, 2, OP_NOARG}, // Repeat OUTD until B=0

    // Block memory search (non‑compare)
    {"ldir", {0xED, 0xB0}, 2, OP_NOARG}, // Copy HL→DE, BC bytes, forward
    {"lddr", {0xED, 0xB8}, 2, OP_NOARG}, // Copy HL→DE, BC bytes, backward

    // Block compare (already have CPIRX for eZ80, but here’s Z80 form)
    {"cpir", {0xED, 0xB1}, 2, OP_NOARG}, // Compare A with (HL), forward
    {"cpdr", {0xED, 0xB9}, 2, OP_NOARG}, // Compare A with (HL), backward

    // Negate accumulator
    {"neg", {0xED, 0x44}, 2, OP_NOARG}, // A = 0 - A

    // Load I/R to A and vice versa
    {"ld a,i", {0xED, 0x57}, 2, OP_NOARG},
    {"ld a,r", {0xED, 0x5F}, 2, OP_NOARG},
    {"ld i,a", {0xED, 0x47}, 2, OP_NOARG},
    {"ld r,a", {0xED, 0x4F}, 2, OP_NOARG},

    // Interrupt mode control
    {"im 0", {0xED, 0x46}, 2, OP_NOARG},
    {"im 1", {0xED, 0x56}, 2, OP_NOARG},
    {"im 2", {0xED, 0x5E}, 2, OP_NOARG},

    // Return from non‑maskable interrupt
    {"retn", {0xED, 0x45}, 2, OP_NOARG},

    // Return from interrupt (maskable)
    {"reti", {0xED, 0x4D}, 2, OP_NOARG},

        // --- Z80 rarities ---
    {"sll b", {0xCB, 0x30}, 2, OP_NOARG}, // Undocumented: Shift Left Logical (set bit 0)
    {"sll c", {0xCB, 0x31}, 2, OP_NOARG},
    {"sll d", {0xCB, 0x32}, 2, OP_NOARG},
    {"sll e", {0xCB, 0x33}, 2, OP_NOARG},
    {"sll h", {0xCB, 0x34}, 2, OP_NOARG},
    {"sll l", {0xCB, 0x35}, 2, OP_NOARG},
    {"sll (hl)", {0xCB, 0x36}, 2, OP_NOARG},
    {"sll a", {0xCB, 0x37}, 2, OP_NOARG},

    {"rld", {0xED, 0x6F}, 2, OP_NOARG}, // Rotate nibbles between A and (HL)
    {"rrd", {0xED, 0x67}, 2, OP_NOARG}, // Reverse rotate nibbles

    {"ld ixl,nn", {0xDD, 0x2E, 0x00}, 3, OP_IMM8}, // Low byte of IX
    {"ld ixh,nn", {0xDD, 0x26, 0x00}, 3, OP_IMM8}, // High byte of IX
    {"ld iyl,nn", {0xFD, 0x2E, 0x00}, 3, OP_IMM8}, // Low byte of IY
    {"ld iyh,nn", {0xFD, 0x26, 0x00}, 3, OP_IMM8}, // High byte of IY

    {"ld a,ixh", {0xDD, 0x7C}, 2, OP_NOARG},
    {"ld a,ixl", {0xDD, 0x7D}, 2, OP_NOARG},
    {"ld a,iyh", {0xFD, 0x7C}, 2, OP_NOARG},
    {"ld a,iyl", {0xFD, 0x7D}, 2, OP_NOARG},

    {"ld ixh,a", {0xDD, 0x67}, 2, OP_NOARG},
    {"ld ixl,a", {0xDD, 0x6F}, 2, OP_NOARG},
    {"ld iyh,a", {0xFD, 0x67}, 2, OP_NOARG},
    {"ld iyl,a", {0xFD, 0x6F}, 2, OP_NOARG},

    // --- eZ80 extras ---
    {"lea bc,sp+nn", {0xED, 0x01, 0x00, 0x00}, 4, OP_IMM16},
    {"lea de,sp+nn", {0xED, 0x11, 0x00, 0x00}, 4, OP_IMM16},
    {"lea hl,sp+nn", {0xED, 0x21, 0x00, 0x00}, 4, OP_IMM16},

    {"ld u,nnnnnn", {0xED, 0x6D, 0x00, 0x00, 0x00}, 5, OP_IMM24}, // Load 24-bit user reg
    {"ld (nnnnnn),u", {0xED, 0x65, 0x00, 0x00, 0x00}, 5, OP_IMM24},
    {"ld u,(nnnnnn)", {0xED, 0x6F, 0x00, 0x00, 0x00}, 5, OP_IMM24},

    {"push u", {0xED, 0x75}, 2, OP_NOARG},
    {"pop u",  {0xED, 0x7D}, 2, OP_NOARG},

    {"mlt ix", {0xED, 0xDC}, 2, OP_NOARG}, // Multiply IXH*IXL
    {"mlt iy", {0xED, 0xFC}, 2, OP_NOARG}, // Multiply IYH*IYL

    {"tst bc", {0xED, 0x04}, 2, OP_NOARG}, // Test BC (sets flags, no store)
    {"tst de", {0xED, 0x14}, 2, OP_NOARG},
    {"tst hl", {0xED, 0x24}, 2, OP_NOARG},
    {"tst sp", {0xED, 0x34}, 2, OP_NOARG},

    {"lddrx", {0xED, 0xBC}, 2, OP_NOARG},
    {"cpirx", {0xED, 0xB5}, 2, OP_NOARG},
    {"cpdrx", {0xED, 0xBD}, 2, OP_NOARG},

    {"sub ixh", {0xDD, 0x94}, 2, OP_NOARG},
    {"sub ixl", {0xDD, 0x95}, 2, OP_NOARG},
    {"sub iyh", {0xFD, 0x94}, 2, OP_NOARG},
    {"sub iyl", {0xFD, 0x95}, 2, OP_NOARG},

    {"and ixh", {0xDD, 0xA4}, 2, OP_NOARG},
    {"and ixl", {0xDD, 0xA5}, 2, OP_NOARG},
    {"and iyh", {0xFD, 0xA4}, 2, OP_NOARG},
    {"and iyl", {0xFD, 0xA5}, 2, OP_NOARG},

    {"or ixh",  {0xDD, 0xB4}, 2, OP_NOARG},
    {"or ixl",  {0xDD, 0xB5}, 2, OP_NOARG},
    {"or iyh",  {0xFD, 0xB4}, 2, OP_NOARG},
    {"or iyl",  {0xFD, 0xB5}, 2, OP_NOARG},

    {"xor ixh", {0xDD, 0xAC}, 2, OP_NOARG},
    {"xor ixl", {0xDD, 0xAD}, 2, OP_NOARG},
    {"xor iyh", {0xFD, 0xAC}, 2, OP_NOARG},
    {"xor iyl", {0xFD, 0xAD}, 2, OP_NOARG},

    {"cp ixh",  {0xDD, 0xBC}, 2, OP_NOARG},
    {"cp ixl",  {0xDD, 0xBD}, 2, OP_NOARG},
    {"cp iyh",  {0xFD, 0xBC}, 2, OP_NOARG},
    {"cp iyl",  {0xFD, 0xBD}, 2, OP_NOARG},

    // Indexed INC/DEC on IXH/IXL/IYH/IYL
    {"inc ixh", {0xDD, 0x24}, 2, OP_NOARG},
    {"inc ixl", {0xDD, 0x2C}, 2, OP_NOARG},
    {"inc iyh", {0xFD, 0x24}, 2, OP_NOARG},
    {"inc iyl", {0xFD, 0x2C}, 2, OP_NOARG},

    {"dec ixh", {0xDD, 0x25}, 2, OP_NOARG},
    {"dec ixl", {0xDD, 0x2D}, 2, OP_NOARG},
    {"dec iyh", {0xFD, 0x25}, 2, OP_NOARG},
    {"dec iyl", {0xFD, 0x2D}, 2, OP_NOARG},

};

// Lookup function: case-insensitive match
const Instruction *lookup_instruction(const char *mnemonic)
{
    for (int i = 0; i < INSTRUCTION_COUNT; i++)
    {
        if (strcasecmp(mnemonic, instruction_table[i].mnemonic) == 0)
        {
            return &instruction_table[i];
        }
    }
    return NULL;
}