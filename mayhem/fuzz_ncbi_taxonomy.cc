// AFL file-input harness for the NCBI taxonomy dump readers behind `kraken2-build`:
// NCBITaxonomy (nodes.dmp + names.dmp) -> MarkNode -> ConvertToKrakenTaxonomy -> Taxonomy load.
// The loaders report malformed input with errx(), so this is a file-input target rather than an
// in-process one.
//
// Input file: the text of nodes.dmp, a NUL byte, then the text of names.dmp.
#include "taxonomy_walk.h"

#include <sys/syscall.h>
#include <unistd.h>

using namespace kraken2;

static std::string MemFdPath(const char *bytes, size_t len) {
  int fd = (int) syscall(SYS_memfd_create, "k2-dmp", 0);
  if (fd < 0)
    errx(EX_OSERR, "memfd_create");
  while (len > 0) {
    ssize_t n = write(fd, bytes, len);
    if (n <= 0)
      errx(EX_OSERR, "write");
    bytes += n;
    len -= (size_t) n;
  }
  return "/proc/self/fd/" + std::to_string(fd);
}

int main(int argc, char **argv) {
  if (argc < 2)
    errx(EX_USAGE, "usage: %s input", argv[0]);
  std::ifstream in(argv[1], std::ios::binary);
  std::string all((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  size_t nul = all.find('\0');
  std::string nodes = all.substr(0, nul);
  std::string names = nul == std::string::npos ? std::string() : all.substr(nul + 1);

  std::string nodes_path = MemFdPath(nodes.data(), nodes.size());
  std::string names_path = MemFdPath(names.data(), names.size());
  NCBITaxonomy ncbi(nodes_path, names_path);

  // Mark the first few IDs named in nodes.dmp, as build_db marks the taxa of its sequences.
  std::istringstream lines(nodes);
  std::string line;
  for (int marked = 0; marked < 16 && std::getline(lines, line); ) {
    char *end = nullptr;
    unsigned long id = strtoul(line.c_str(), &end, 10);
    if (end != line.c_str() && id != 0) {
      ncbi.MarkNode(id);
      marked++;
    }
  }

  int out_fd = (int) syscall(SYS_memfd_create, "k2-taxo", 0);
  if (out_fd < 0)
    errx(EX_OSERR, "memfd_create");
  std::string out_path = "/proc/self/fd/" + std::to_string(out_fd);
  ncbi.ConvertToKrakenTaxonomy(out_path.c_str());

  Taxonomy taxo(out_path, false);
  taxo.GenerateExternalToInternalIDMap();
  uint64_t sink = WalkTaxonomy(taxo);
  size_t n = std::min<size_t>(taxo.node_count(), 32);
  for (size_t a = 1; a < n; a++)
    sink += taxo.GetInternalID(taxo.nodes()[a].external_id);
  return sink == 0xdeadbeefcafef00dULL;
}
