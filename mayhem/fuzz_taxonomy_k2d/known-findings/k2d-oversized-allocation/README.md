ASan allocation-size-too-big in `Taxonomy::Init` (`src/taxonomy.cc:232`, `new char[rank_data_len_]`): `rank_data_len` read from the .k2d header is used unvalidated as the allocation size.

Same root cause as `k2d-oversized-node-count` (`:228`) and `k2d-oversized-name-length` (`:230`): the three header sizes are never checked against the file size.

Reproduce: `/mayhem/fuzz_taxonomy_k2d input` (built by `mayhem/build.sh`; ASan+UBSan halting).
