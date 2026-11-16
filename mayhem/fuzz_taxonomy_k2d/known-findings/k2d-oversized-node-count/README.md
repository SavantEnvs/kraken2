ASan allocation-size-too-big in `Taxonomy::Init` (`src/taxonomy.cc:228`, `new TaxonomyNode[node_count_]`): `node_count` read from the .k2d header is used unvalidated as the allocation size.

Same root cause as `k2d-oversized-allocation` and `k2d-oversized-name-length`: the three header sizes are never checked against the file size.

Reproduce: `/mayhem/fuzz_taxonomy_k2d input` (built by `mayhem/build.sh`; ASan+UBSan halting).
