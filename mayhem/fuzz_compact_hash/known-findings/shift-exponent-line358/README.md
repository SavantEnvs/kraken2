UBSan shift exponent too large at `src/compact_hash.h:358`: header fields (key bit counts) are not validated.

Reproduce: `/mayhem/fuzz_compact_hash input` (built by `mayhem/build.sh`; ASan+UBSan halting).
