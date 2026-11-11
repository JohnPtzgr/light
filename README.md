# Light Programming language

## The One-Sentence Definition

> **Light is a graduated systems language where transparency, safety, speed, efficiency, and flexibility are the default, enabled by the declarative nature.**

---

## The Core Philosophy

| Principle | Meaning |
|-----------|---------|
| **No compromises** | Speed, safety, flexibility, simplicity coexist |
| **Gradual certainty** | Start simple, add rigor as needed |
| **Explicit costs** | Every abstraction has a visible cost |
| **Structure → Safety → Optimisation** | Restrictions enable performance |
| **Transparency** | What you see is what you get |

---

## The Language

### Syntax
- Configurable, multi-dialect (Python-like, Rust-like, C-like, custom)
- Keywords usable as field names
- Right-to-left prefix pipelines (contiguous, `[a-zA-Z]`, 52 prefixes)
- String literals already wide (`wchar32_t`-like); prefixes transform *contents*, not types

### Types
- Primitives, bit-precise (`i4`, `f10e5`), algebraic (product/sum/union/intersection)
- Refinement types (`where . > 0`)
- Generic values, type packs, metatypes
- Three-level equality: `=~` (shape), `==` (full), `is` (identity)

### Memory
- Per-context pools
- **`let`** → N CT-known stacks, u32 offsets, extendable top element,
     size-customizable pools (S0), support variable affinity to allow S0 > 4GiB, local
- **`let!`** → heap, u32 offsets, size-customizable pools (S1), support variable affinity to allow S1 > 4GiB, local
- **`shared let`** → M CT-known stacks, u64 offsets, extendable top element, can be shared
- **`shared let!`** → heap, u64 offsets, shared
- Pool granularity fixed upon creation, hence per async context (default 4 GB, compiler flag)
- No dangling pointers, no GC
- Minimal overhead

### References
- **Static** (`&T`, `uN ref'pool[T]`) → compile-time, zero-cost
- **Shared** (`shared T`) → ARC, direct deref, own pool
- **Runtime** (`&'r T`) → certificate, 2 derefs, escape hatch
- **Conversions**: static→runtime (certificate), runtime→static (forbidden), shared→static (invalidate)

### Concurrency
- **`commit`** → exclusive spinlock, atomic
- **`freeze`** → read-only snapshot, joinable
- **`freeze after`** → barrier + segments + write token
- **`yield`** → cooperative, outside critical sections
- **Atomics** → lock-free
- **Wait-free by default** (work-stealing + dependency checking)
- **Deadlock-free by default** (one lock at a time)

### Async
- Backend-agnostic (`coroutine`, `thread`, `fiber`, `process`)
- **A→S bridge**: async calls sync via interception at IO boundary
- **S→A bridge**: sync blocks, async runs on await
- **One word**: `await` turns legacy C into concurrent code
- OS calls intercepted; custom IO needs custom interceptor

### Errors
- Five levels: compiler proof, `T?`, `T!`, exceptions, panic
- Operators: `?`, `!`, `!!`, `??`
- Mandatory `@throws` / `@panics`
- `Error` struct: code + message + source + backtrace + metadata

### Effects & Determinism
- Effects: input, inside state, output, outside state
- Tracks memory, concurrency, determinism, IO, etc.
- Supports user-provided effects
- ND only if ND value escapes local context (conservative)
- Determinism enables optimisation (CSE, memoisation, parallelisation)

### Metaprogramming
- `constexpr`, `@quote`, generic values, CTIO
- Generic values (compile-time maps)
- Decorators: `@If`, `@cast`, `@effect`
- Axioms + simple SAT (linear arithmetic; non-linear needs axioms)

### Ergonomics and Runtime
- Generics enable minimal boilerplate
- `var` provides python-style runtime typing
- `autoref` adds an escape hatch for complex reference topology, through scoped GC
- Reflection (:type) and `dyntype` offer runtime type support if necessary
- `eval()` and `run()` for sandboxed code execution in RT
- All necessary types and data structures like arrays, dictionaries, sets, strings, tuples are built-in

### Verification
- Refinement types (`where`)
- Axioms (user-defined, recursive, scoped, assumed true, spot-checked)
- SAT-solver (linear only, literal × variable)
- WCET/BCET from ACM
- `-fsafety-critical` mode (deterministic, stronger verification, mandatory WCET)

### Interop
- C+- dialect (GCC extensions + generics, total back-compatibility with C11)
- `extern C` (full C+- code inclusion, auto binds to light)

### Build & Tooling
- Three-stage parser (lexer-1 → lexer-2 → parser)
- Universal AST, `.lth` headers, `.ltmeta` cache
- Per-module compilation, parallel, incremental
- Smart linker, binaries with optimizations enabled by linking on a target machine
- Configurable frontend, runtime dialect switching
- `eval` / `venv` with config files
