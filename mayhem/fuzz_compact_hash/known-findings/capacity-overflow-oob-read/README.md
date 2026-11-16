Out-of-bounds read in `CompactHashTable::Get` (`src/compact_hash.h:363` and the `CompactHashCell::value` read at `:29`): `capacity * sizeof(Cell)` wraps in `LoadTable`, so the file's few bytes satisfy the size check and a tiny table is allocated, while `idx = hash % capacity` indexes far outside it. Depending on the probe index, UBSan reports a pointer-overflow at `:363` or ASan reports the wild read (SEGV) at `:29`.

The same input passes the mmap path's `Capacity mismatch` check, since that check multiplies the same way.

Reproduce: `/mayhem/fuzz_compact_hash input` (built by `mayhem/build.sh`; ASan+UBSan halting).
