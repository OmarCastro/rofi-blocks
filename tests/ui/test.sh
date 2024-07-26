#!/usr/bin/env bash
 
cd "$(dirname "${BASH_SOURCE[0]}")"

xvfb-run -s "-screen 0 1280x720x24" ./test--xvfb.sh