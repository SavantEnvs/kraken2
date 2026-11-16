// AFL file-input harness for the compact hash table file (hash.k2d) that classify and dump_table
// load with CompactHashTable<CompactHashCell>, both read into memory and memory-mapped, followed
// by lookups.  The loader reports a bad file with errx(), so this is a file-input target rather
// than an in-process one.
#include "../src/compact_hash.h"

using namespace kraken2;

static uint64_t Probe(CompactHashTable<CompactHashCell> &table, uint64_t seed) {
  uint64_t sink = table.size() + table.capacity();
  for (uint64_t i = 0; i < 64; i++)
    sink += table.Get(seed * 0x9e3779b97f4a7c15ULL + i * 0x1234567ULL);
  return sink;
}

int main(int argc, char **argv) {
  if (argc < 2)
    errx(EX_USAGE, "usage: %s hash.k2d", argv[0]);
  uint64_t sink = 0;
  {
    CompactHashTable<CompactHashCell> table(argv[1], false);
    sink += Probe(table, 1);
    sink += table.GetValueCounts().size();
  }
  {
    CompactHashTable<CompactHashCell> table(argv[1], true);
    sink += Probe(table, 2);
  }
  return sink == 0xdeadbeefcafef00dULL;
}
