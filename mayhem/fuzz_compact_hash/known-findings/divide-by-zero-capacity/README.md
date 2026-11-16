Division by zero at `src/compact_hash.h:359`: a header with capacity 0 reaches the modulo in the probe.

Reproduce: `/mayhem/fuzz_compact_hash input` (built by `mayhem/build.sh`; ASan+UBSan halting).
