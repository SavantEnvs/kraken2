Stack-buffer-overflow in `std::string::find` after `line.pop_back()` calls in the NCBITaxonomy constructor (`src/taxonomy.cc` ~line 41) strip more characters than the line holds.

Reproduce: `/mayhem/fuzz_ncbi_taxonomy input` (built by `mayhem/build.sh`; ASan+UBSan halting).
