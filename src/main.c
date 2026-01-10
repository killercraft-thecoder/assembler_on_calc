#include <tice.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <fileioc.h>
#include <stdlib.h> // for atoi()
#include "opcodes.h"
#include "linker.h"
#include "version.h"
#include <stdint.h>
#include <stdbool.h>
#include <ti/error.h> // for ERR_MEMORY constant

#define MAX_LABELS 64
#define LABEL_NAME_LEN 16

#ifdef __INTELLISENSE__
typedef unsigned long uint24_t;
#define true 1
#define false 0
#endif

char **stored_lines = NULL; // pointer to array of char*
uint16_t  stored_count = 0;  // number of lines read
uint16_t capacity = 0;      // allocated capacity

typedef struct
{
    char name[LABEL_NAME_LEN];
    uint24_t address;
} Label;

Label labels[MAX_LABELS];
uint8_t label_count = 0;

#define MIN_ALLOC_SIZE 1024 // 1 KB minimum allocation

void trim(char *str)
{
    size_t len = strlen(str);
    while (len > 0 && isspace(str[len - 1]))
    {
        str[--len] = '\0';
    }
}

void add_label(const char *name, uint24_t address)
{
    if (label_count < MAX_LABELS)
    {
        strncpy(labels[label_count].name, name, LABEL_NAME_LEN);
        labels[label_count].name[LABEL_NAME_LEN - 1] = '\0';
        labels[label_count].address = address;
        label_count++;
    }
}

int find_label(const char *name, uint24_t *out_addr)
{
    for (uint8_t i = 0; i < label_count; i++)
    {
        if (strcmp(labels[i].name, name) == 0)
        {
            *out_addr = labels[i].address;
            return 1;
        }
    }
    return 0;
}

void assemble_line(const char *line, uint24_t *pc, bool pass2)
{
    char *line_copy = line;
    trim(line_copy);

    if (line_copy[0] == '\0')
        return;

    char *first = strtok(line_copy, " ");
    if (!first)
        return;

    // Label definition
    size_t len = strlen(first);
    if (first[len - 1] == ':')
    {
        first[len - 1] = '\0';
        if (!pass2)
            add_label(first, *pc);
        first = strtok(NULL, " ");
        if (!first)
            return;
    }

    // --- Handle Comments ---
    if (first[0] == ';')
    {
        return;
    }

    // --- Handle .db directive ---
    if (strcasecmp(first, ".db") == 0 || strcasecmp(first, "db") == 0)
    {
        char *arg;
        while ((arg = strtok(NULL, ",")) != NULL)
        {
            trim(arg);
            if (arg[0] == '"' || arg[0] == '\'')
            {
                // String literal
                char quote = arg[0];
                char *p = arg + 1;
                while (*p && *p != quote)
                {
                    if (pass2)
                        linker_emit((uint8_t *)p, 1);
                    (*pc)++;
                    p++;
                }
            }
            else
            {
                // Numeric literal
                unsigned long value = strtoul(arg, NULL, 0);
                if (pass2)
                {
                    uint8_t b = (uint8_t)value;
                    linker_emit(&b, 1);
                }
                (*pc)++;
            }
        }
        return;
    }

    // --- Handle .dw directive ---
    if (strcasecmp(first, ".dw") == 0 || strcasecmp(first, "dw") == 0)
    {
        char *arg;
        while ((arg = strtok(NULL, ",")) != NULL)
        {
            trim(arg);
            unsigned long value;

            // Label reference
            if (isalpha((unsigned char)arg[0]))
            {
                uint24_t addr;
                if (!find_label(arg, &addr))
                {
                    if (pass2)
                    {
                        os_PutStrFull("Undefined label\n");
                        return;
                    }
                    addr = 0; // placeholder in pass1
                }
                value = addr;
            }
            else
            {
                value = strtoul(arg, NULL, 0);
            }

            if (pass2)
            {
                uint8_t bytes[2];
                bytes[0] = value & 0xFF;        // low byte
                bytes[1] = (value >> 8) & 0xFF; // high byte
                linker_emit(bytes, 2);
            }
            (*pc) += 2;
        }
        return;
    }

    // --- Normal instruction handling ---
    const Instruction *inst = lookup_instruction(first);
    if (!inst)
    {
        os_PutStrFull("Unknown instruction\n");
        return;
    }

    uint8_t buffer[8];
    memcpy(buffer, inst->opcode, inst->length);

    if (inst->type != OP_NOARG &&
        (inst->type == OP_IMM8 || inst->type == OP_IMM16 || inst->type == OP_IMM24))
    {
        char *arg_str = strtok(NULL, " ,");
        if (!arg_str)
        {
            os_PutStrFull("Missing operand\n");
            return;
        }

        unsigned long value;
        if (isalpha((unsigned char)arg_str[0]))
        {
            uint24_t addr;
            if (!find_label(arg_str, &addr))
            {
                if (pass2)
                {
                    os_PutStrFull("Undefined label\n");
                    return;
                }
                addr = 0;
            }
            value = addr;
        }
        else
        {
            value = strtoul(arg_str, NULL, 0);
        }

        if (inst->type == OP_IMM8)
        {
            buffer[inst->length - 1] = (uint8_t)value;
        }
        else if (inst->type == OP_IMM16)
        {
            buffer[inst->length - 2] = value & 0xFF;
            buffer[inst->length - 1] = (value >> 8) & 0xFF;
        }
        else if (inst->type == OP_IMM24)
        {
            buffer[inst->length - 3] = value & 0xFF;
            buffer[inst->length - 2] = (value >> 8) & 0xFF;
            buffer[inst->length - 1] = (value >> 16) & 0xFF;
        }
    }

    if (pass2)
    {
        if (!linker_emit(buffer, inst->length))
        {
            os_PutStrFull("Code buffer full\n");
        }
    }

    *pc += inst->length;
}

void print_version(void)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "ON-CALC ASSEMBLER %d.%d ", VER_MAJOR, VER_MINOR);
    os_PutStrFull(buf);
    os_NewLine();
    delay(10);
    while (!os_GetCSC()) {};
}

int main(void)
{
    ti_var_t file;
    stored_lines = NULL;
    uint8_t ch;
    uint8_t pos = 0;
    uint24_t pc = 0;
    stored_count = 0;
    capacity = 0;
    char line_buf[256]; // temp buffer for reading a line

    os_ClrHome();
    print_version();
    linker_reset();

    // Open the file (AppVar named "ASRC")
    file = ti_Open("ASRC", "r");
    if (!file)
    {
        os_PutStrFull("File not found");
        os_NewLine();
        while (!os_GetCSC()) {};
        return 0;
    }

    // --- Pass 0: read lines into dynamically growing array ---
    while (ti_Read(&ch, 1, 1, file) == 1)
    {
        if (ch == '\n' || ch == '\r')
        {
            if (pos > 0)
            {
                line_buf[pos] = '\0';

                // Allocate more space if needed
                if (stored_count >= capacity)
                {
                    // Grow capacity in chunks of at least MIN_ALLOC_SIZE
                    size_t new_cap = capacity + (MIN_ALLOC_SIZE / sizeof(char *));
                    char **new_mem = realloc(stored_lines, new_cap * sizeof(char *));
                    if (!new_mem)
                    {
                        os_ThrowError(OS_E_MEMORY); // Shows ERR:MEMORY and halts
                        return 0;
                    }
                    stored_lines = new_mem;
                    capacity = new_cap;
                }

                // Allocate space for this line
                size_t len = strlen(line_buf) + 1;
                stored_lines[stored_count] = malloc(len);
                if (!stored_lines[stored_count])
                {
                    os_ThrowError(OS_E_MEMORY);
                    return 0;
                }
                memcpy(stored_lines[stored_count], line_buf, len);
                stored_count++;

                pos = 0;
            }
        }
        else if (pos < sizeof(line_buf) - 1)
        {
            line_buf[pos++] = ch;
        }
    }

    // Handle last line if no newline at EOF
    if (pos > 0)
    {
        line_buf[pos] = '\0';
        if (stored_count >= capacity)
        {
            size_t new_cap = capacity + (MIN_ALLOC_SIZE / sizeof(char *));
            char **new_mem = realloc(stored_lines, new_cap * sizeof(char *));
            if (!new_mem)
            {
                os_ThrowError(OS_E_MEMORY);
                return 0;
            }
            stored_lines = new_mem;
            capacity = new_cap;
        }
        size_t len = strlen(line_buf) + 1;
        stored_lines[stored_count] = malloc(len);
        if (!stored_lines[stored_count])
        {
            os_ThrowError(OS_E_MEMORY);
            return 0;
        }
        memcpy(stored_lines[stored_count], line_buf, len);
        stored_count++;
    }

    ti_Close(file);

    // --- Pass 1: collect labels ---
    pc = 0;
    for (uint16_t i = 0; i < stored_count; i++)
    {
        assemble_line(stored_lines[i], &pc, false);
    }

    // --- Pass 2: emit code ---
    pc = 0;
    for (uint16_t i = 0; i < stored_count; i++)
    {
        assemble_line(stored_lines[i], &pc, true);
    }

    os_PutStrFull("Build complete");
    os_NewLine();
    delay(100);
    os_PutStrFull("Collecting Memory...");
    os_NewLine();
    // --- Cleanup ---
    for (uint16_t i = 0; i < stored_count; i++)
        free(stored_lines[i]);
    free(stored_lines);
    os_PutStrFull("Collected Memory.");
    os_NewLine();
    delay(10);
    os_PutStrFull("Launching Program...");
    os_NewLine();
    delay(10);
    linker_run();

    return 0;
}
