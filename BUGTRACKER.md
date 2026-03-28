# KonEngine / KonScript — Bug Tracker & Dev Plan

Last updated: 2026-03-28

---

## Applied Changes (this session)

### `src/input/input.hpp`
- Added `Esc = 256` as an alias for `Escape` in `Key::Code`
- Both `Key::Esc` and `Key::Escape` now work from C++

### `tools/KonScript/include/codegen.hpp` — `keyAliases` in `genMember()`
- `Esc` is now the canonical form — passes through the alias map untouched
- `Escape` and `ESC` both alias down to `Esc`
- In KonScript: `Key.Esc`, `Key.Escape`, and `Key.ESC` all compile to `Key::Esc`

---

## Known Bugs

### 🔴 Critical

**[BUG-01] `for-in` over a range literal emits broken C++**
- File: `codegen.hpp` → `genForIn()`
- The iterable is emitted raw via `genExpr()`. When the iterable is a `Range`
  expression (`0..10`), `genExpr()` just writes `/* range */` (see `genExpr`
  `Expr::Kind::Range` case). This means:
  ```ks
  for i: I32 in 0..10 { ... }
  ```
  compiles to:
  ```cpp
  for (const int32_t& i : /* range */) { ... }
  ```
  which is a compile error.
- **Fix needed:** `genForIn()` must detect when the iterable is a `RangeExpr`
  and emit an iota-style expansion instead:
  ```cpp
  // 0..10  (exclusive)
  for (int32_t i = 0; i < 10; i++) { ... }

  // 0..=10 (inclusive)
  for (int32_t i = 0; i <= 10; i++) { ... }
  ```
  Or emit using `<numeric>` iota + a temp vector — but that allocates. The
  direct loop expansion is cleaner and matches what the user expects.

---

**[BUG-02] `wait` statement is a no-op stub**
- File: `codegen.hpp` → `genWait()`
- Currently emits: `/* wait */ (void)(duration); // wait -- implement via coroutine scheduler`
- No actual delay happens. Any code using `wait` silently does nothing.
- **Fix needed:** Decide on the semantics before fixing:
  - If KonScript stays transpile-to-C++: emit `std::this_thread::sleep_for()`
    with the `<thread>` and `<chrono>` headers added. Works but blocks the
    game loop thread — bad for game logic.
  - Better: tie into the VM / coroutine scheduler once that exists (v0.9.0+).
  - Short term: emit a TODO comment + a `sleep_for` as a placeholder so it
    at least does something visible.

---

**[BUG-03] `spawn` expression is a stub**
- File: `codegen.hpp`, `Expr::Kind::Spawn` case
- Emits: `/* spawn */ call()`
- The `spawn` keyword is parsed and in the AST but generates meaningless output.
- **Fix needed:** Same dependency as BUG-02 — needs the VM/scheduler.
  Until then, should either be removed from the parser or emit a clear
  compile-time error instead of silently generating broken code.

---

**[BUG-04] `Print` with format strings emits broken C++**
- File: `codegen.hpp` → `genCall()`, `genIdent()`
- `Print("Score: %d", score)` is mapped to `std::cout << "Score: %d" << score << "\n"`
- This ignores the format string entirely — `%d` is printed literally.
- The correct output should be using `printf` or `std::format` / `snprintf`.
- **Fix needed:** Either:
  - Map `Print` to `printf` instead of `std::cout` (simplest, matches the
    printf-style format strings the user is already writing)
  - Or implement a real format string parser in the codegen

---

**[BUG-05] `genForIn` uses `const T&` for all loop variables**
- File: `codegen.hpp` → `genForIn()`
- Emits: `for (const int32_t& i : collection)`
- For primitive types like `I32`, `F64`, `Bool`, a `const T&` works but is
  unnecessary — `const T` is better and avoids potential dangling reference
  issues if the collection is a temporary.
- For non-range iterables (actual arrays/vectors), `const T&` is correct.
- **Fix needed:** Detect primitive types and emit `const T` rather than `const T&`.

---

### 🟡 Medium

**[BUG-06] `safe` member access (`?.`) emits `.` instead of a null check**
- File: `codegen.hpp` → `genMember()`
- The `safe` flag branch is:
  ```cpp
  if (e->safe)
      write(".");
  else
      write(".");
  ```
  Both branches write `.` — the safe access does nothing different.
- **Fix needed:** Emit an actual null guard. Since nullable types map to
  `std::optional`, the correct output for `foo?.bar` is something like:
  ```cpp
  (foo ? foo->bar : std::nullopt)
  ```
  or wrapped in a helper lambda to avoid double-evaluation.

---

**[BUG-07] `FloatLit` codegen loses precision with `std::to_string`**
- File: `codegen.hpp` → `genExpr()`, `Expr::Kind::FloatLit`
- Uses `std::to_string(value)` which defaults to 6 decimal places.
- `let x: F64 = 3.14159265358979` compiles to `3.141593` — precision lost.
- **Fix needed:** Use `std::to_string` with full precision, or better, keep
  the raw token string from the lexer and emit that directly.

---

**[BUG-08] Enum payload constructors conflict with user-defined names**
- File: `codegen.hpp` → `genEnum()`
- For enums with payloads, the codegen emits top-level `inline` constructor
  functions named after each variant (e.g. `Coin(...)`, `Health(...)`).
- These pollute the global namespace and can clash with user-defined functions
  or other enum variants with the same name.
- **Fix needed:** Wrap the constructor helpers in a namespace matching the
  enum name, or make them static members of a tag struct.

---

**[BUG-09] `genConst` emits `static constexpr` unconditionally**
- File: `codegen.hpp` → `genConst()`
- `static constexpr` is only valid at class/struct scope or namespace scope —
  not at function scope. A `const` declared inside a function body will fail
  to compile.
- **Fix needed:** Track whether generation is at top-level or inside a
  function/block, and emit `constexpr` (no `static`) at function scope.

---

**[BUG-10] `genSafeMember` (safe member on non-namespace objects) calls `genMember` which writes `.` for both safe and unsafe**
- Same root as BUG-06. Both `Member` and `SafeMember` dispatch to `genMember`,
  which does not correctly differentiate. Tracked separately because the fix
  involves the AST `safe` flag, not just the codegen path.

---

**[BUG-11] `genSwitch` with enum variants doesn't qualify case labels**
- File: `codegen.hpp` → `genSwitch()`
- For a `switch` on an `enum class` value, case labels must be fully qualified:
  `case State::Idle:` not `case Idle:`.
- The codegen emits whatever `genExpr` produces for the case value, which for
  a plain identifier (`Idle`) gives `Idle` — a compile error with `enum class`.
- **Fix needed:** When the switch expression is an enum type (detected from
  the typechecker's symbol table), qualify case labels with `EnumName::`.

---

### 🟢 Minor / Polish

**[BUG-12] `genWait` includes `<thread>` / `<chrono>` nowhere**
- Even when fixed (BUG-02), the generated file won't include the necessary
  headers. The header emission in `generate()` needs a flag to add them.

**[BUG-13] `break` / `continue` with labels emit `goto` but never emit the label targets**
- File: `codegen.hpp` → `genBreak()`, `genContinue()`
- Labelled break/continue emit `goto __break_label` but the loop generators
  (`genWhile`, `genForC`, `genForIn`, `genLoop`) never emit the corresponding
  `__break_label:` / `__continue_label:` after the loop body.
- This is a compile error whenever labelled break/continue is used.

**[BUG-14] `genForIn` label field exists in `ForInStmt` AST but is ignored in codegen**
- Related to BUG-13. The `label` field on `ForInStmt` is set by the parser
  but `genForIn()` never reads it.

**[BUG-15] The `Error` type in `codegen.hpp` uses a forward-declared `e` that doesn't exist**
- In `codegen.hpp`:
  ```cpp
  const std::vector<e>& errors() const { return m_errors; }
  ...
  std::vector<e> m_errors;
  ```
  `e` is not a type — this should be `Error`. This may be a project knowledge
  truncation artifact but worth verifying in the actual file.

---

## Roadmap — What To Work On

### Phase 1 — KonScript Stability (v0.9.0, current focus)

Priority order:

1. **Fix BUG-01** (`for-in` range expansion) — most likely to be hit by any
   real KonScript code
2. **Fix BUG-04** (`Print` format strings → `printf`) — affects every debug
   print in any KonScript program
3. **Fix BUG-11** (switch enum case qualification) — breaks any switch on
   an enum
4. **Fix BUG-06 / BUG-10** (safe member access `?.`) — currently a silent
   wrong-code generator
5. **Fix BUG-07** (float precision) — subtle data corruption in compiled output
6. **Fix BUG-09** (`static constexpr` at function scope) — compile error for
   any `const` declared inside a function
7. **Fix BUG-13 / BUG-14** (labelled break/continue) — compile error when used
8. **Fix BUG-08** (enum payload namespace pollution) — correctness + UX
9. **Fix BUG-05** (`const T&` for primitives in for-in) — minor but clean up
10. **Verify BUG-15** (Error type alias) — check actual source file

Then for v0.9.0 KonScript features:
- Add KonScript DOCS.md to the repo (`tools/KonScript/DOCS.md`)
- Update test_lexer.cpp and test_parser.cpp: `Key.Escape` → `Key.Esc`
- Add a codegen test that compiles a `.ks` file and verifies the `.cpp` output

---

### Phase 2 — Asset Pipeline (v0.9.0, alongside KonScript)

- KonPaktor is already implemented (`konpak.hpp`, CLI, Qt GUI)
- Remaining work:
  - Add `konpak` build to CI release workflow
  - Add engine-side `AssetManager` that transparently loads from `.konpak`
    or loose files without the user calling `UnpackAssets()` manually
  - KonPaktor Windows build verification (OpenSSL vs BCrypt path)

---

### Phase 3 — VM Design (post-v0.9.0, pre-editor)

This is required for the editor. Without a VM:
- The editor can't run KonScript without shelling out to g++
- Hot reload is impossible
- In-editor debugging is impossible

Design questions to settle before starting:
- **Bytecode or tree-walking?** Tree-walking is faster to build, bytecode is
  faster to run and easier to serialize. Given the editor goal, bytecode is
  the right call — the editor needs to serialize and send scripts to the
  running game.
- **Scope:** VM for KonScript only, or general engine scripting VM?
  KonScript-only is simpler. A general VM is harder but makes v0.11.0
  (editor scripting) much cleaner.
- **`wait` / `spawn` semantics:** The VM is where these get real implementations.
  `wait` = yield the coroutine for N seconds. `spawn` = start a new coroutine.
  Design the VM's coroutine model before fixing BUG-02 and BUG-03.

Suggested VM architecture:
```
KonScript source (.ks)
    → Lexer → Parser → TypeChecker → BytecodeEmitter
        → VM (register-based, ~32 registers)
            → KonEngine C++ bindings (calls into the engine)
```

The transpile-to-C++ path stays for release builds (full optimization).
The VM path is for the editor and hot-reload. Two backends, one frontend.

---

### Phase 4 — Editor MVP (v0.10.0)

- Viewport panel (render scene to an FBO, display in ImGui image)
- Hierarchy panel (tree view of scene nodes)
- Properties panel (inspect/edit node fields)
- Scene save/load (serialize scene tree to a simple text format)
- Asset browser (list files in project directory)

Depends on: VM (Phase 3) for running game scripts in-editor.

---

### Phase 5 — Editor Scripting (v0.11.0)

- Built-in code editor (ImGui + a text editor widget, or embed a minimal one)
- KonScript syntax highlighting
- In-editor compile + run via the VM
- Hot reload: re-compile changed `.ks` files and swap the VM's bytecode

---

## Key Design Notes

### `Key.Esc` change (applied)
- C++: both `Key::Esc` and `Key::Escape` work (both = 256 in the enum)
- KonScript: `Key.Esc` is the canonical form; `Key.Escape` and `Key.ESC`
  are aliases that map to `Esc` via the codegen alias table
- Documented as `Esc` everywhere going forward

### VM and `wait` / `spawn`
- Don't fix BUG-02 and BUG-03 with a `sleep_for` hack
- Leave them as stubs with a clear comment, fix properly when the VM exists
- Adding `sleep_for` now would set a bad precedent and block the thread

### `Print` → `printf` (BUG-04)
- Switch the codegen mapping from `std::cout <<` to `printf`
- This means the generated file needs `#include <cstdio>` — add it to the
  header block in `generate()`
- `Print("hello\n")` → `printf("hello\n")`
- `Print("Score: %d", score)` → `printf("Score: %d", score)`
- Much cleaner than the current stdout chain
