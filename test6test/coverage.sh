#!/usr/bin/env bash
# coverage.sh - build with gcov flags, run server/tests, collect gcov data, summarize coverage >=90%
set -euo pipefail

# Configuration
TCP_PORT=8080
UDP_PORT=8081
STREAM_SOCK=/tmp/stream.sock
DGRAM_SOCK=/tmp/dgram.sock
STORAGE_FILE=./storage.txt

# Build all executables with coverage flags
echo "Building executables with coverage flags..."
make clean
make coverage_all

# Launch server in background (preserve existing gcov files)
echo "Starting drinks_bar server with coverage..."
./drinks_bar.out -T "$TCP_PORT" -U "$UDP_PORT" -s "$STREAM_SOCK" -d "$DGRAM_SOCK" -f "$STORAGE_FILE" &
SERVER_PID=$!
sleep 1

# Test TCP ADD via atom_supplier
echo "Testing ADD commands (TCP)..."
echo -e "1\n5" | ./atom_supplier.out -h localhost -p "$TCP_PORT"
echo -e "2\n3" | ./atom_supplier.out -h localhost -p "$TCP_PORT"
echo -e "3\n7" | ./atom_supplier.out -h localhost -p "$TCP_PORT"

# Test UDP DELIVER via molecule_requester
echo "Testing DELIVER commands (UDP)..."
echo -e "1\n2" | ./molecule_requester.out -h localhost -p "$TCP_PORT"
echo -e "2\n1" | ./molecule_requester.out -h localhost -p "$TCP_PORT"
echo -e "3\n1" | ./molecule_requester.out -h localhost -p "$TCP_PORT"
echo -e "4\n2" | ./molecule_requester.out -h localhost -p "$TCP_PORT"

# Stop server
echo "Stopping server (PID $SERVER_PID)..."
kill "$SERVER_PID" || true
sleep 1

# Generate gcov reports (do not delete existing .gcov files)
echo "Generating gcov reports..."
mkdir -p coverage
gcov -o obj src/*.c > coverage/gcov_output.txt || true

# Summarize coverage
echo "Parsing coverage summary..."
TOTAL=0
COUNT=0
while IFS= read -r line; do
  if [[ "$line" =~ Lines.executed:[[:space:]]*([0-9]+\.[0-9]+)% ]]; then
    cov=${BASH_REMATCH[1]}
    TOTAL=$(awk "BEGIN {printf \"%.2f\", $TOTAL + $cov}")
    COUNT=$((COUNT + 1))
  fi
done < coverage/gcov_output.txt

if (( COUNT > 0 )); then
  AVERAGE=$(awk "BEGIN {printf \"%.2f\", $TOTAL / $COUNT}")
  echo "Average coverage across $COUNT files: $AVERAGE%"
else
  echo "No coverage data found."
  exit 1
fi

# Check threshold
if (( $(echo "$AVERAGE < 90.0" | bc -l) )); then
  echo "Coverage below 90%."
  exit 1
else
  echo "Coverage meets threshold (>=90%)."
  exit 0
fi
