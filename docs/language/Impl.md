The SCode implementation pipeline is:

lexer -> parser -> analyzer -> code generator

which means I have a parse tree, a meaning tree (could be AST),
and VM code.

LuaJit:
- Parses directly to byte-code. Which is fast, although the code is a
  bit crufty. (I tried doing that for Gen, and things got complicated.)
- Byte code: each function has a byte-code array. Each operation is 32 bits,
  with an 8 bit opcode, 3 8 bit operands. Register based VM.
  The generated byte code is isomorphic to the syntax tree.
- Function has a line number array (8,16 or 32 bits per line number)
  mapping bc operations to line numbers. This assumes all of the code in
  a function comes from the same source file, which is true in Lua.
- During JIT, byte code is converted to SSA IR, which is optimized, then
  compiled to machine code.

JavaScriptCore:
- first implementation was a tree interpreter, which was thrown away.
- Now, parse to an AST, then compile AST to byte code, which can be interpreted.
  Direct threaded code for a register machine. But interpreter is now disabled.
  Direct threaded register code is bloated: 'add x y z' is 4 words.
- Byte code is for a register VM. (Registers are stored in slots in the stack
  frame.)
- The DataFlowGraph JIT converts byte code to an SSA-style IR, which it then
  optimizes.

ChakraCore:
- On first call to function, parse to AST then compile to bytecode,
  which is then interpreted.
- During interpretation, collect profile information: types and invocation
  counts.
- If a function or loop body is evaluated more than <threshold> times,
  invoke JIT compiler in background, which uses profile data. JIT code
  bails out to interpreter if unexpected types (not predicted by profile)
  are used.
- I see "bytecode" and "IR" representations in the source.

Guile:
- register machine byte codes

SCode requirements:
- Fast compilation + evaluation
- Tail recursion elimination. Parse tree evaluator doesn't do this unless you
  use a trampoline.
- Function values can be printed as something that's close to source code.
  Keep original source as string, or convert byte code to string.
- Run time errors & warnings print source location.
- Diagnostic API (debugger, profiler)
- Debug console:
  - Safely abort running program. This requires co-operation from the VM:
    evaluation thread needs to periodically check the abort flag.
  - Suspend running program, view stack trace.