#!/bin/bash
# Run a .sszb test script against the game
# Usage: ./tests/run_test.sh [script.sszb]
set -e

SCRIPT="${1:-tests/test_full_cycle.sszb}"
PORT=9999
GAME="./build/Game"

if [ ! -f "$GAME" ]; then
    echo "ERROR: Game not built. Run 'make build' first."
    exit 1
fi

if [ ! -f "$SCRIPT" ]; then
    echo "ERROR: Script not found: $SCRIPT"
    exit 1
fi

# Kill any existing game
pkill -f "./build/Game" 2>/dev/null || true
sleep 0.5

# Start the game in background
echo "Starting game..."
$GAME &
GAME_PID=$!

# Wait for TCP server
echo "Waiting for TCP server on port $PORT..."
for i in $(seq 1 30); do
    if nc -z localhost $PORT 2>/dev/null; then
        echo "TCP server ready."
        break
    fi
    if ! kill -0 $GAME_PID 2>/dev/null; then
        echo "ERROR: Game process died."
        exit 1
    fi
    sleep 0.2
done

# Send SCRIPT command and capture the report
# Use a background cat to keep stdin open; kill it when nc exits
echo "Running script: $SCRIPT"
exec 3< <(echo "SCRIPT $SCRIPT"; sleep 999)
nc localhost $PORT <&3 > /private/tmp/claude/test_report.txt 2>/dev/null &
NC_PID=$!

# Wait for game to finish (QUIT closes it)
wait $GAME_PID 2>/dev/null || true

# nc should exit when server closes fd; give it a moment then force-kill
sleep 0.5
kill $NC_PID 2>/dev/null || true
wait $NC_PID 2>/dev/null || true
exec 3<&-

REPORT=$(cat /private/tmp/claude/test_report.txt 2>/dev/null)

echo ""
echo "$REPORT"

# Check for failures
if echo "$REPORT" | grep -q "FAIL: 0"; then
    echo ""
    echo "=== ALL TESTS PASSED ==="
    exit 0
else
    echo ""
    echo "=== TESTS FAILED ==="
    exit 1
fi
