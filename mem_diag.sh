#!/usr/bin/env bash
set -euo pipefail

OUT_DIR="mem_diag_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$OUT_DIR"

echo "[INFO] Output dir: $OUT_DIR"
echo "[INFO] Command: $*"

(
  exec "$@"
) > "$OUT_DIR/stdout.log" 2> "$OUT_DIR/stderr.log" &

PID=$!
echo "[INFO] PID: $PID"

{
  echo "time_ms VmRSS_kB VmSize_kB VmData_kB Threads voluntary_ctxt_switches nonvoluntary_ctxt_switches"
  while kill -0 "$PID" 2>/dev/null; do
    if [[ -r "/proc/$PID/status" ]]; then
      now=$(date +%s%3N)
      awk -v now="$now" '
        /VmRSS:/ {rss=$2}
        /VmSize:/ {vms=$2}
        /VmData:/ {data=$2}
        /Threads:/ {thr=$2}
        /voluntary_ctxt_switches:/ {vcsw=$2}
        /nonvoluntary_ctxt_switches:/ {nvcsw=$2}
        END {print now, rss+0, vms+0, data+0, thr+0, vcsw+0, nvcsw+0}
      ' /proc/$PID/status
    fi
    sleep 0.002
  done
} > "$OUT_DIR/proc_status.tsv" &

MON_PID=$!

{
  while kill -0 "$PID" 2>/dev/null; do
    if [[ -r "/proc/$PID/smaps_rollup" ]]; then
      echo "===== time $(date +%s%3N) ====="
      cat "/proc/$PID/smaps_rollup"
    fi
    sleep 0.01
  done
} > "$OUT_DIR/smaps_rollup.log" &

SMAPS_PID=$!

/usr/bin/time -v -p -o "$OUT_DIR/time.log" tail --pid="$PID" -f /dev/null || true

kill "$MON_PID" "$SMAPS_PID" 2>/dev/null || true

echo "[INFO] Finished."
cat "$OUT_DIR/time.log"
echo "[INFO] Peak sampled VmRSS:"
awk 'NR>1 {if($2>m)m=$2} END{print m " kB"}' "$OUT_DIR/proc_status.tsv"
