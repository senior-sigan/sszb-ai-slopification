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
# Keep stdin open so nc doesn't half-close the socket
echo "Running script: $SCRIPT"
REPORT=$( (echo "SCRIPT $SCRIPT"; sleep 120) | nc localhost $PORT 2>/dev/null )

# Wait a moment for game to process QUIT
sleep 1

# Kill game if still running
kill $GAME_PID 2>/dev/null || true
wait $GAME_PID 2>/dev/null || true

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
