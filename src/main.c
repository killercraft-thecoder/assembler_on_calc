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
uint16_t stored_count = 0;  // number of lines read
uint16_t capacity = 0;      // allocated capacity

typedef struct
{
    char name[LABEL_NAME_LEN];
    uint24_t address;
} Label;

Label labels[MAX_LABELS];
uint8_t label_count = 0;

#define MIN_ALLOC_SIZE 1024 // 1 KB minimum allocation

#define MAX_INCLUDE_DEPTH 8

// helper struct for read result
typedef struct
{
    char **lines;
    uint16_t count;
} LinesResult;

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

// Read an AppVar into a LinesResult. Caller must free lines and each line.
static LinesResult read_appvar_lines(const char *name)
{
    LinesResult res = {NULL, 0};
    ti_var_t f = ti_Open(name, "r");
    if (!f)
        return res;
    uint8_t ch;
    char buf[256];
    uint16_t pos = 0;
    uint16_t count = 0;
    uint16_t cap = 0;

    while (ti_Read(&ch, 1, 1, f) == 1)
    {
        if (ch == '\n' || ch == '\r')
        {
            if (pos > 0)
            {
                buf[pos] = '\0';
                if (count >= cap)
                {
                    size_t newcap = cap + (MIN_ALLOC_SIZE / sizeof(char *));
                    char **newmem = realloc(res.lines, newcap * sizeof(char *));
                    if (!newmem)
                    { // cleanup and return empty
                        for (uint16_t i = 0; i < count; i++)
                            free(res.lines[i]);
                        free(res.lines);
                        res.lines = NULL;
                        res.count = 0;
                        ti_Close(f);
                        return res;
                    }
                    res.lines = newmem;
                    cap = newcap;
                }
                size_t len = strlen(buf) + 1;
                res.lines[count] = malloc(len);
                if (!res.lines[count])
                {
                    for (uint16_t i = 0; i < count; i++)
                        free(res.lines[i]);
                    free(res.lines);
                    res.lines = NULL;
                    res.count = 0;
                    ti_Close(f);
                    return res;
                }
                memcpy(res.lines[count], buf, len);
                count++;
                pos = 0;
            }
        }
        else if (pos < sizeof(buf) - 1)
        {
            buf[pos++] = ch;
        }
    }
    // last line if no newline at EOF
    if (pos > 0)
    {
        buf[pos] = '\0';
        if (count >= cap)
        {
            size_t newcap = cap + (MIN_ALLOC_SIZE / sizeof(char *));
            char **newmem = realloc(res.lines, newcap * sizeof(char *));
            if (!newmem)
            {
                for (uint16_t i = 0; i < count; i++)
                    free(res.lines[i]);
                free(res.lines);
                res.lines = NULL;
                res.count = 0;
                ti_Close(f);
                return res;
            }
            res.lines = newmem;
            cap = newcap;
        }
        size_t len = strlen(buf) + 1;
        res.lines[count] = malloc(len);
        if (!res.lines[count])
        {
            for (uint16_t i = 0; i < count; i++)
                free(res.lines[i]);
            free(res.lines);
            res.lines = NULL;
            res.count = 0;
            ti_Close(f);
            return res;
        }
        memcpy(res.lines[count], buf, len);
        count++;
    }

    ti_Close(f);
    res.count = count;
    return res;
}

// parse include directive line and extract filename (returns malloc'd string or NULL)
static char *parse_include_filename(const char *line)
{
    char tmp[256];
    strncpy(tmp, line, sizeof(tmp));
    tmp[255] = '\0';
    trim(tmp);
    // accept: INCLUDE "NAME" or .include "NAME" or INCLUDE NAME
    char *p = tmp;
    char *tok = strtok(p, " \t");
    if (!tok)
        return NULL;
    if (strcasecmp(tok, "INCLUDE") != 0 && strcasecmp(tok, ".include") != 0)
        return NULL;
    char *rest = strtok(NULL, "");
    if (!rest)
        return NULL;
    trim(rest);
    // strip quotes if present
    if (rest[0] == '"' || rest[0] == '\'')
    {
        char quote = rest[0];
        size_t L = strlen(rest);
        if (L >= 2 && rest[L - 1] == quote)
        {
            rest[L - 1] = '\0';
            rest++;
        }
        else
        {
            // malformed
            return NULL;
        }
    }
    // copy name
    char *name = malloc(strlen(rest) + 1);
    if (name)
        strcpy(name, rest);
    return name;
}

// process includes in stored_lines; aborts on error by printing message and returning false
static bool process_includes(void)
{
    // simple stack to detect recursion and depth
    char *include_stack[MAX_INCLUDE_DEPTH];
    int depth = 0;

    // We'll iterate through stored_lines and build a new array in-place
    uint16_t i = 0;
    while (i < stored_count)
    {
        char *line = stored_lines[i];
        char tmp[256];
        strncpy(tmp, line, sizeof(tmp));
        tmp[255] = '\0';
        trim(tmp);
        // detect include
        char *fname = parse_include_filename(tmp);
        if (!fname)
        {
            i++;
            continue;
        } // not an include, keep it
        // check recursion depth
        if (depth >= MAX_INCLUDE_DEPTH)
        {
            os_PutStrFull("Include depth exceeded\n");
            free(fname);
            return false;
        }
        // check for cycles by name
        bool cycle = false;
        for (int s = 0; s < depth; s++)
        {
            if (strcmp(include_stack[s], fname) == 0)
            {
                cycle = true;
                break;
            }
        }
        if (cycle)
        {
            os_PutStrFull("Include cycle detected\n");
            free(fname);
            return false;
        }
        // read included file
        LinesResult inc = read_appvar_lines(fname);
        if (!inc.lines || inc.count == 0)
        {
            os_PutStrFull("Include file not found or empty\n");
            free(fname);
            return false;
        }
        // splice: replace stored_lines[i] with inc.lines[0..n-1]
        // compute new required capacity
        uint32_t new_count = stored_count - 1 + inc.count; // remove 1 include line, add inc.count
        if (new_count > capacity)
        {
            // grow capacity in same chunk style
            size_t needed = new_count;
            size_t newcap = capacity;
            while (newcap < needed)
                newcap += (MIN_ALLOC_SIZE / sizeof(char *));
            char **newmem = realloc(stored_lines, newcap * sizeof(char *));
            if (!newmem)
            {
                os_PutStrFull("Memory error expanding includes\n");
                // cleanup inc
                for (uint16_t k = 0; k < inc.count; k++)
                    free(inc.lines[k]);
                free(inc.lines);
                free(fname);
                return false;
            }
            stored_lines = newmem;
            capacity = newcap;
        }
        // shift tail down/up to make room or close gap
        if (inc.count > 1)
        {
            // move tail up
            for (uint16_t t = stored_count; t > i + 1; t--)
            {
                stored_lines[t - 1 + (inc.count - 1)] = stored_lines[t - 1];
            }
        }
        else if (inc.count == 0)
        {
            // nothing to insert, just remove line
            for (uint16_t t = i; t < stored_count - 1; t++)
                stored_lines[t] = stored_lines[t + 1];
            stored_count--;
            free(fname);
            continue;
        }
        else if (inc.count == 1)
        {
            // replace in place
            free(stored_lines[i]);
            stored_lines[i] = inc.lines[0];
            free(inc.lines);
            free(fname);
            // continue scanning after this line
            i++;
            continue;
        }
        // insert inc.lines into stored_lines at position i
        // free the include line memory
        free(stored_lines[i]);
        // copy inc.lines into stored_lines[i..i+inc.count-1]
        for (uint16_t k = 0; k < inc.count; k++)
        {
            stored_lines[i + k] = inc.lines[k];
        }
        // update stored_count
        stored_count = new_count;
        // free inc.lines container (lines themselves moved)
        free(inc.lines);
        free(fname);
        // continue scanning after the inserted block
        i += inc.count;
    }
    return true;
}

void assemble_line(const char *line, uint24_t *pc, bool pass2, uint24_t line_number)
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
                        char errbuf[32];
                        snprintf(errbuf, sizeof(errbuf), "Undefined label at line:%u\n", (unsigned)line_number);
                        os_PutStrFull(errbuf);
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
        char errbuf[32];
        snprintf(errbuf, sizeof(errbuf), "Unknown instruction:%s at line:%u\n", first, (unsigned)line_number);
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
            char errbuf[32];
            snprintf(errbuf, sizeof(errbuf), "Missing operand at line:%u\n", (unsigned)line_number);
            os_PutStrFull(errbuf);
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
                    char errbuf[32];
                    snprintf(errbuf, sizeof(errbuf), "Undefined label at line:%u\n", (unsigned)line_number);
                    os_PutStrFull(errbuf);
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
    while (!os_GetCSC())
    {
    };
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
        while (!os_GetCSC())
        {
        };
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

    process_includes(); // new function that expands INCLUDE/.include directives

    // --- Pass 1: collect labels ---
    pc = 0;
    for (uint16_t i = 0; i < stored_count; i++)
    {
        assemble_line(stored_lines[i], &pc, false, i);
    }

    // --- Pass 2: emit code ---
    pc = 0;
    for (uint16_t i = 0; i < stored_count; i++)
    {
        assemble_line(stored_lines[i], &pc, true, i);
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
