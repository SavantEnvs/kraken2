// Shared by the taxonomy harnesses: queries over a loaded Taxonomy.
#ifndef MAYHEM_TAXONOMY_WALK_H_
#define MAYHEM_TAXONOMY_WALK_H_

#include "../src/taxonomy.h"

// taxonomy.h: a node's parent_id "must be lower-numbered", and LowestCommonAncestor and
// IsAAncestorOfB follow parent_id until they meet, so they are only called when every node meets it.
static inline uint64_t WalkTaxonomy(const kraken2::Taxonomy &taxo) {
  size_t count = taxo.node_count();
  for (size_t i = 1; i < count; i++)
    if (taxo.nodes()[i].parent_id >= i)
      return 0;
  size_t n = std::min<size_t>(count, 32);
  uint64_t sink = 0;
  for (size_t a = 1; a < n; a++)
    for (size_t b = 1; b < n; b++)
      sink += taxo.LowestCommonAncestor(a, b) + taxo.IsAAncestorOfB(a, b);
  return sink;
}

#endif
