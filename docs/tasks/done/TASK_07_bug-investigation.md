# TASK_07_bug-investigation

# Input:
### Bug Report: SIGBUS (Exit Code 138) on macOS

#### 1. What Happened
CLion prints: `Debugger process finished with exit code 138 (interrupted by signal 10:SIGBUS). Rerun 'my_app'` when I try to run in debug mode.
The application crashed during execution with **Exit Code 138 (`SIGBUS`, Signal 10)** while running on macOS. This indicates a hardware-level bus error caused by a CPU attempting to access a physical memory address that is misaligned or invalid for the target type's alignment requirements (particularly strict on Apple Silicon / macOS architectures).

#### 2. Findings

Based on the assumption that `reinterpret_cast` is the problem:
```bash
▶ git grep "reinterpret_cast"
include/bullet/LinearMath/btAlignedAllocator.h:         return reinterpret_cast<pointer>(btAlignedAlloc(sizeof(value_type) * n, Alignment));
include/bullet/LinearMath/btAlignedAllocator.h:         btAlignedFree(reinterpret_cast<void*>(ptr));
src/engine/core/EventBus.h:                     return reinterpret_cast<TypeKey>(&marker);
```

* **Custom Allocator (`btAlignedAllocator.h`):** Uses `reinterpret_cast` around `btAlignedAlloc` and `btAlignedFree`. While Bullet's allocator is generally designed for alignment, custom overrides, mixing standard new/delete with aligned allocs, or incorrect casting can introduce alignment gaps.
* **Event Bus (`EventBus.h`):** Contains the following line:
```cpp
return reinterpret_cast<TypeKey>(&marker);

```


* *Risk 1 (Lifetime/Scope):* Returning the address of a stack-allocated local variable (`&marker`) is a classic undefined behavior risk if that address outlives the stack frame or is later dereferenced improperly.
* *Risk 2 (Alignment/Casting):* Casting a local variable's stack address to a `TypeKey` may violate alignment expectations depending on `TypeKey`'s size and strict alignment rules on macOS.



#### 3. Hypotheses to Test

1. **Hypothesis A (Stack Address Lifetime & Dereference):** The `TypeKey` returned from `&marker` in `EventBus.h` is being dereferenced or compared after its stack frame has popped, or it creates an unaligned pointer reference that triggers `SIGBUS` on the strict macOS CPU architecture.
2. **Hypothesis B (Memory Alignment Mismatch):** An unaligned pointer cast or custom memory layout in either the event system or physics allocator is violating CPU alignment constraints, causing an immediate hardware trap (`SIGBUS`).

## Criteria:
- Gauge whether this is the real source of the problem.

# Output:
- Create a concise report of the plan to fix it alongside this task file.