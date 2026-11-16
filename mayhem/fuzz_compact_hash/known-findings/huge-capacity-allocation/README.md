ASan allocation-size-too-big in `CompactHashTable::LoadTable` (`src/compact_hash.h:318`, the `posix_memalign` for the table): the header `capacity` is multiplied by the cell size and allocated without any check against the file size.

Same root cause as the other `hash.k2d` findings here: the header fields (`capacity`, `key_bits`, `value_bits`) are never validated by the read path.

Reproduce: `/mayhem/fuzz_compact_hash input` (built by `mayhem/build.sh`; ASan+UBSan halting).
