#!/usr/bin/env bash
#
# mayhem/test.sh: runs Kraken 2's own end-to-end suite (tests/e2e.sh) against the normal-flag
# install that mayhem/build.sh left in mayhem/.oracle/install, and reports its counts as CTRF.
# The suite builds a database from the bundled viral references, classifies simulated reads and
# compares calls, reports and the k2 front end with golden files and accuracy thresholds, so a
# program that does nothing (or exits 0) fails it.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${MAYHEM_JOBS:=$(nproc)}"   # build parallelism; env-overridable, falls back to nproc (use -j"$MAYHEM_JOBS")
cd "$SRC"

# emit_ctrf <tool> <passed> <failed> [skipped] [pending] [other]
# Writes a CTRF report (file + stdout `CTRF {...}` marker) and returns non-zero iff failed>0.
emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}


INSTALL="$SRC/mayhem/.oracle/install"
if [ ! -x "$INSTALL/classify" ] || [ ! -x "$INSTALL/build_db" ]; then
  echo "oracle install missing at $INSTALL: mayhem/build.sh must build it" >&2
  emit_ctrf "kraken2-e2e" 0 1 0
  exit 1
fi

WORK="$(mktemp -d "${TMPDIR:-/tmp}/k2-e2e.XXXXXX")"
LOG="$WORK/e2e.out"
bash tests/e2e.sh "$INSTALL" "$WORK/run" > "$LOG" 2>&1
rc=$?
grep -E '^(FAIL|XPASS) ' "$LOG" | head -40
tail -3 "$LOG"

# Summary line: "Summary: P passed, F failed, X expected failures, U unexpected passes, S skipped (...)"
summary="$(grep -E '^Summary: ' "$LOG" | tail -1)"
num() { sed -n "s/.*[ :]\([0-9][0-9]*\) $1.*/\1/p" <<<"$summary" | head -1; }
passed="$(num passed)"; failed="$(num failed)"; xfail="$(num 'expected failures')"
xpass="$(num 'unexpected passes')"; skipped="$(num skipped)"
if [ -z "$summary" ] || [ -z "$passed" ] || [ -z "$failed" ]; then
  echo "no parsable Summary line from tests/e2e.sh (exit $rc)" >&2
  emit_ctrf "kraken2-e2e" 0 1 0
  exit 1
fi
failed=$(( failed + ${xpass:-0} ))
# The script's own exit status must agree with its counts, and a run that checked nothing is not a pass.
if [ "$rc" -ne 0 ] && [ "$failed" -eq 0 ]; then failed=1; fi
if [ "$passed" -lt 50 ]; then failed=$(( failed + 1 )); fi
rm -rf "$WORK"
emit_ctrf "kraken2-e2e" "$passed" "$failed" $(( ${xfail:-0} + ${skipped:-0} ))
