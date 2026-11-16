UBSan signed integer overflow at `src/compact_hash.h:29` from unvalidated header bit counts.

Reproduce: `/mayhem/fuzz_compact_hash input` (built by `mayhem/build.sh`; ASan+UBSan halting).
