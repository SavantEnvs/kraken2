UBSan shift exponent too large at `src/compact_hash.h:29`: header key/value bit counts are not validated.

Reproduce: `/mayhem/fuzz_compact_hash input` (built by `mayhem/build.sh`; ASan+UBSan halting).
