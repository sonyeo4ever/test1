rm -rf data
rm simpleReplay
rm traceReplay
gcc simpleReplay.c -g -o simpleReplay --static -lm
g++ traceReplay.cpp cJSON.cpp traceConfig.cpp -g -o traceReplay --static -lm
