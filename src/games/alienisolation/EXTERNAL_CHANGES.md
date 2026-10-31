# External changes needed for the Alien: Isolation mod

This mod folder is self-contained except for one change outside it. Without that change the 32-bit addon crashes on
startup (access violation right after `State attached` in `ReShade.log`).

## `src/utils/platform.hpp`: alignment-aware heap allocation

**Problem:** `ProcessAllocator::allocate` and `CreateSharedObject` use plain `HeapAlloc`, which only guarantees 8-byte
alignment on x86. `gtl::parallel_hash_set` pads its sub-tables with `alignas(64)` and the compiler emits aligned SIMD
stores for them, so a misaligned block faults. Crashes seen: `phmap.hpp` (`command_action` pipeline registrations) and
`platform.hpp` `OpenProcessSharedSlot` (cross-addon shared state). x64 is unaffected (16-byte heap alignment).

**Fix:**
- `ProcessAllocator<T>::allocate/deallocate`: for `alignof(T) > MEMORY_ALLOCATION_ALIGNMENT`, over-allocate, align the
  pointer manually and store the original heap pointer in front of the block. Other types keep the plain `HeapAlloc` path.
- `CreateSharedObject` / `DeleteSharedObject`: allocate through `ProcessAllocator<T>` instead of raw `HeapAlloc`
  (still zero-filled with `std::memset`, `#include <cstring>` added).

Apply by copying the two hunks from the author's working tree, or re-implement as described above.

## Mod-side notes (inside this folder)

- Swapchain is HDR10 (`SetUseHDR10(true)`, PQ / BT.2020). The old hand-written `OnPresent` proxy was removed in favor of
  the library proxy (`swap_chain_proxy_*_shader`, `expected_constant_buffer_index = 11`).
- Tone mapper is PsychoV-30 (`renodx::tonemap::psychov::psychotm_test30`); `psycho_test17_custom.hlsli` was removed.
- UI replacement shaders now also match when the game draws to the back buffer (`HasBackBufferRenderTarget`), because
  the game keeps its 8-bit view while the draw is redirected to the float clone.
- Build with the x86 preset: `cmake --preset clang-x86`, then `cmake --build build32 --config Release --target renodx-alienisolation.addon32`.
