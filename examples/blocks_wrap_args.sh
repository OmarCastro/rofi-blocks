#!/bin/bash
 
cd "$(dirname "${BASH_SOURCE[0]}")"

ARGS="$@"

rofi -modi blocks -show blocks -blocks-wrap "scripts/blocks_wrap_args.sh $ARGS"
