#!/bin/bash
# compiles engine for host and reports errors
cd "$(dirname "$0")/.."
mkdir -p /tmp/lx6obj; rm -f /tmp/lx6obj/*.o /tmp/lx6err.txt
SRCS=$(cat host/sources.txt)
for f in $SRCS; do gcc -c -O2 -g -w -DLX6_HOST -D__fp16=float -include host/lx6_host.h -Ihost/stub -Ihost -Ilxr/Inc -Ilxr/Inc/Soundengine $f -o /tmp/lx6obj/$(basename $f).o 2>>/tmp/lx6err.txt || echo FAIL $f; done
