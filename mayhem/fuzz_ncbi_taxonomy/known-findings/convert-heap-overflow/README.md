Heap-buffer-overflow write in `NCBITaxonomy::ConvertToKrakenTaxonomy` (`src/taxonomy.cc:170`): node ids from the dmp files index `taxo.nodes_` without bounds checks.

Reproduce: `/mayhem/fuzz_ncbi_taxonomy input` (built by `mayhem/build.sh`; ASan+UBSan halting).
