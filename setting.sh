rm -rf data
rm simpleReplay
rm traceReplay
gcc simpleReplay.c -g -o simpleReplay --static -lm
gcc traceReplay.c cJSON.c -g -o traceReplay --static -lm
