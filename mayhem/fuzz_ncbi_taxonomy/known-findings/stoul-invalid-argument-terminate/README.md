Uncaught `std::invalid_argument` from `std::stoul` in the NCBITaxonomy constructor when a nodes.dmp/names.dmp field is not numeric; the process terminates.

Reproduce: `/mayhem/fuzz_ncbi_taxonomy input` (built by `mayhem/build.sh`; ASan+UBSan halting).
