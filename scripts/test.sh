#!/usr/bin/env bash
# Run all JS-side tests. Used by `npm test` and CI.
set -e
cd "$(dirname "$0")/.."
echo "=== Tier 1: data layer ==="
node prototype/tests/test-osd-params.js
echo ""
echo "=== Inline parity ==="
node prototype/tests/test-inline-parity.js
echo ""
echo "All JS tests passed."
