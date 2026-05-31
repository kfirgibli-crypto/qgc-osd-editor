#!/usr/bin/env bash
# Serve the prototype on localhost:8000 so file:// quirks don't bite.
set -e
cd "$(dirname "$0")/../prototype"
echo "Serving on http://localhost:8000 — open in any browser."
echo "Press Ctrl+C to stop."
python3 -m http.server 8000
