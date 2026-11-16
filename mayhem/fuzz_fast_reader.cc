// libFuzzer harness for Kraken 2's sequence block reader and the code that consumes its records
// (FastReader -> quality masking -> MinimizerScanner / TranslateToAllFrames), driven the way
// classify.cc drives them.
//
// Input layout: 4 control bytes, then the FASTA/FASTQ payload.
//   data[0] bit0: FASTQ rather than FASTA; bit1: interleaved pairs (record_multiple 2);
//           bit2: pull with LoadRecords instead of LoadBlock; bit3: translated (protein) search
//   data[1]: how many leading bytes go into the stream cursor's carry (the rest stay in the fd)
//   data[2]: block size, in bytes
//   data[3]: minimizer length l in the low 5 bits (1..31 bases, 1..15 residues), (k - l) / 2 in the high 3
#include "../src/aa_translate.h"
#include "../src/fast_reader.h"
#include "../src/mmscanner.h"

#include <sys/syscall.h>
#include <unistd.h>

using namespace kraken2;

namespace {
int MakeMemFd(const uint8_t *data, size_t size) {
  int fd = (int) syscall(SYS_memfd_create, "k2-input", 0);
  if (fd < 0)
    return -1;
  size_t done = 0;
  while (done < size) {
    ssize_t n = write(fd, data + done, size - done);
    if (n <= 0) {
      close(fd);
      return -1;
    }
    done += (size_t) n;
  }
  lseek(fd, 0, SEEK_SET);
  return fd;
}
}  // namespace

extern "C" int LLVMFuzzerInitialize(int *, char ***) {
  initLookUpTables();
  return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  if (size < 4)
    return -1;
  const uint8_t flags = data[0];
  const size_t carry_len = std::min<size_t>(data[1], size - 4);
  const size_t block_bytes = (size_t) data[2] + 1;
  const bool protein = (flags & 8) != 0;
  // l stays within MinimizerScanner's own limit (l bases x 2 bits or l residues x 4 bits in 63 bits).
  const ssize_t l = 1 + (data[3] & 0x1f) % (protein ? 15 : 31);
  const ssize_t k = l + 2 * (data[3] >> 5);

  const uint8_t *payload = data + 4;
  const size_t payload_len = size - 4;
  int fd = MakeMemFd(payload + carry_len, payload_len - carry_len);
  if (fd < 0)
    return -1;

  StreamCursor cur;
  cur.format = (flags & 1) ? FORMAT_FASTQ : FORMAT_FASTA;
  cur.carry.assign(payload, payload + carry_len);

  // Every load resets the reader's buffers; only its cumulative fault counter survives an input, and
  // it is read as a delta.  Reusing it saves allocating its 16 MiB buffer on every execution.
  static FastReader reader;
  const size_t faults_before = reader.fault_count();
  MinimizerScanner scanner(k, l, DEFAULT_SPACED_SEED_MASK, ! protein);
  std::vector<std::string> frames(6);
  const size_t multiple = (flags & 2) ? 2 : 1;
  volatile uint64_t sink = 0;

  for (;;) {
    bool more = (flags & 4) ? reader.LoadRecords(fd, cur, multiple * 3)
                            : reader.LoadBlock(fd, cur, block_bytes, multiple);
    if (! more)
      break;
    reader.Parse();
    for (size_t i = 0; i < reader.size(); i++) {
      SeqView &v = reader.at(i);
      for (uint32_t j = 0; j < v.header_len; j++)
        sink = sink + (unsigned char) v.header[j];
      for (uint32_t j = 0; j < v.comment_len; j++)
        sink = sink + (unsigned char) v.comment[j];
      if (v.quals && v.quals_len == v.seq_len) {
        char *s = v.seq_mutable();
        for (uint32_t j = 0; j < v.seq_len; j++)
          if (v.quals[j] - '!' < 20)
            s[j] = 'x';
      }
      if (protein) {
        TranslateToAllFrames(v.seq, v.seq_len, frames);
        for (auto &frame : frames) {
          scanner.LoadSequence(frame);
          while (uint64_t *m = scanner.NextMinimizer())
            sink = sink + *m + scanner.is_ambiguous();
        }
      }
      else {
        scanner.LoadSequence(v.seq, v.seq_len);
        while (uint64_t *m = scanner.NextMinimizer())
          sink = sink + *m + scanner.is_ambiguous();
      }
    }
    if (cur.eof)
      break;
  }
  sink = sink + reader.fault_count() - faults_before + cur.error.size();
  close(fd);
  return 0;
}
