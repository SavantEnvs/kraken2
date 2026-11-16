// AFL file-input harness for the binary taxonomy file (taxo.k2d, magic "K2TAXDAT") that classify,
// dump_table and merge load with Taxonomy(), both read into memory and memory-mapped.  The loader
// reports a bad file with errx(), so this is a file-input target rather than an in-process one.
#include "taxonomy_walk.h"

using namespace kraken2;

int main(int argc, char **argv) {
  if (argc < 2)
    errx(EX_USAGE, "usage: %s taxo.k2d", argv[0]);
  uint64_t sink = 0;
  {
    Taxonomy taxo(argv[1], false);
    taxo.GenerateExternalToInternalIDMap();
    sink += WalkTaxonomy(taxo);
  }
  {
    Taxonomy taxo(argv[1], true);
    sink += WalkTaxonomy(taxo);
  }
  return sink == 0xdeadbeefcafef00dULL;
}
