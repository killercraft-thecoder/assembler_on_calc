# ON-CALC ASSEMBLER README

A compact, two‑pass assembler for the TI‑84 Plus CE family built to run entirely on the calculator. It reads an AppVar named **ASRC**, assembles code in two passes (label collection and emission), and launches the resulting program. The assembler supports **335 instructions**, data directives, label syntax, and a small but practical feature set designed for on‑device development.

---

## Features

- **On‑calc assembly**: runs entirely on the TI‑84 Plus CE; no PC toolchain required.  
- **335 instructions supported**: full instruction table with immediate sizes (8/16/24) and no‑arg forms.  
- **Two‑pass assembly**: Pass 1 collects labels and addresses; Pass 2 emits bytes and resolves label references.  
- **Data directives**: `.db` and `.dw` for bytes and words (little‑endian).  
- **Label syntax**: `label:` definitions and label references in operands.  
- **Simple error reporting**: clear messages for unknown instructions, missing operands, undefined labels, and memory errors.  
- **Dynamic source buffer**: reads ASRC into a dynamically growing array of lines to support large sources.  
- **Linker integration**: emits bytes through a small linker layer and can run the assembled program on completion.  
- **Extensible opcode table**: instruction lookup is centralized so adding or tweaking opcodes is straightforward.

---

## Quickstart

**1. Prepare source**
- Create an AppVar named `ASRC` containing your assembly source lines. Use plain text lines; each line is treated as a separate source line.

**2. Launch assembler**
- Run the assembler program on your calculator. It reads `ASRC`, assembles it, and attempts to launch the resulting program.

**3. Typical workflow**
- Edit `ASRC` on the calculator or on a PC editor and transfer it as an AppVar. Run the assembler, fix any errors reported, and iterate.

**4. Output**
- On success the assembler prints `Build complete` and then runs the linked program. On failure it prints an error message and aborts.

---

## Language and directives

### Basic line structure
- **Label definition**: `name:` at the start of a line. Labels are collected in Pass 1 and resolved in Pass 2.  
- **Instruction**: `MNEMONIC [operand]` where `MNEMONIC` is one of the supported instructions. Operands may be registers, immediates, or labels depending on the instruction type.  
- **Comment**: a line starting with `;` is a comment and ignored.

### Data directives
- **Byte data**: `.db val1, val2, "string"`  
  - Strings are emitted as raw bytes (characters between quotes).  
  - Numeric values accept decimal or `0x` hex notation.  
  - Example: `.db 0x41, 65, "OK", 0`
- **Word data**: `.dw val1, val2`  
  - Emits 16‑bit little‑endian words (low byte first).  
  - Label references are allowed; unresolved labels are zero in Pass 1 and resolved in Pass 2.

### Immediate operands
- Instructions that accept immediates are classified as `OP_IMM8`, `OP_IMM16`, or `OP_IMM24`. The assembler writes immediate bytes into the instruction buffer in little‑endian order for multi‑byte immediates.

### Example directives and usage
```asm
start:
    LD A, #0x10
    JP loop

data_block:
    .db 0x01, 0x02, "HELLO", 0
    .dw table_addr

table_addr:
    .dw 0x1234
```

### Error messages you may see
- **Unknown instruction** — the mnemonic is not in the opcode table.  
- **Missing operand** — an instruction expected an operand but none was provided.  
- **Undefined label** — a label used as an operand was not defined by Pass 1.  
- **Code buffer full** — the linker or emission buffer ran out of space.  
- **ERR:MEMORY** — dynamic allocation failed while reading or storing lines.

---

## Examples

### Minimal program
```asm
start:
    NOP
    NOP
    RET
```

### Using labels and jumps
```asm
main:
    LD A, #5
loop:
    DEC A
    JP NZ, loop
    RET
```

### Data and label reference
```asm
msg:
    .db "HELLO", 0

main:
    LD HL, msg
    CALL print_string ; assumes program defines print_string
    RET
```

### Immediate 8/16/24 examples
```asm
    LD A, #0x12        ; OP_IMM8 example
    LDW HL, #0x1234    ; OP_IMM16 example (low then high)
    CALL #0x9D1234     ; OP_IMM24 example (24-bit address)
```

### Complex example showing labels and data
```asm
start:
    CALL init
    LD HL, message
    CALL print_string
    JP exit

init:
    ; initialization code
    RET

message:
    .db "ON-CALC ASSEMBLER", 0

exit:
    RET
```

---

## Debugging and diagnostics

**Line mapping**
- The assembler stores source lines in an array so it can report errors with the offending line content. When you see an error, the assembler prints a short message; inspect the corresponding line in `ASRC`.

**Undefined labels**
- If you get `Undefined label` during Pass 2, check for:
  - Typos in label names (case sensitive).
  - Labels defined after use (two‑pass assembly should handle forward references, but ensure the label exists).
  - Accidental whitespace or punctuation in the label token.

**Missing operands**
- Ensure instructions that require immediates or operands have them. Example: `LD A` is invalid; `LD A, #0x10` is valid.

**Memory errors**
- The calculator has limited RAM. If you see `ERR:MEMORY` or `Code buffer full`, reduce source size, remove large data tables, or free other AppVars before assembling.

**Improving diagnostics**
- Consider adding a listing or symbol map pass to print addresses and emitted bytes. This is a recommended enhancement for future versions.

---

## Limitations and roadmap

**Current limitations**
- No macro system or multi‑line preprocessor. The assembler intentionally avoids macros to keep behavior simple and predictable.  
- No `.include` or `.incbin` built in by default (can be added as a small pre‑pass).  
- No expression evaluator for arithmetic in immediates (immediates must be numeric literals or label names).  
- Fixed maximum label table size (`MAX_LABELS`) and static limits for dynamic arrays; very large projects may hit these limits.

**Planned or suggested improvements**
- **.include directive** to support modular source files and a small standard library.  
- **Symbol map / listing output** to aid debugging and cross‑reference emitted bytes to source lines.  
- **Expression evaluator** for arithmetic in operands (e.g., `CONST + 4`).  
- **Optional single‑line defines** for constants (`#define`) as a tiny preprocessor.  
- **Configurable origin directive** (`.org`) and output format options.  
- **Better error messages** with file/line mapping when includes are supported.

---

## Contributing and license

**Contributing**
- The project is designed to be simple and portable. Contributions that keep the assembler small, robust, and calculator‑friendly are welcome. Typical contributions:
  - Add or correct opcodes in the opcode table.  
  - Implement small, well‑scoped features (e.g., `.include`, `.org`, listing output).  
  - Improve error messages and diagnostics.  
  - Add sample libraries or example AppVars demonstrating common tasks.

**Coding style**
- Keep memory usage conservative. Prefer small buffers and free temporary allocations promptly. Preserve the two‑pass architecture unless a clear benefit is shown.

**License**
- This project is provided under the **MIT License**. Use, modify, and redistribute freely with attribution.

---

## Appendix Examples and tips

**How to structure a small library AppVar**
- Put reusable routines in an AppVar named `LIB_<NAME>` and include it (or copy its lines into `ASRC` until `.include` is implemented). Keep public symbols prefixed (e.g., `libio_printstr`) and document register/clobber conventions.

**Testing checklist before release**
- Assemble a set of small test programs that exercise:
  - All immediate sizes (8/16/24).  
  - `.db` and `.dw` with strings and label references.  
  - Forward and backward label references.  
  - Error conditions (unknown instruction, missing operand, undefined label).  
  - Memory stress test with many lines and data.

**Practical tips**
- Keep `ASRC` readable: short lines, consistent label naming, and comments.  
- When debugging, reduce the program to the smallest failing case and reassemble.  
- Free unused AppVars and close other apps to maximize available RAM during assembly.