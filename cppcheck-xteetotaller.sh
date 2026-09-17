#!/bin/sh

cppcheck --enable=all --std=c99 --platform=unix64 \
        -DINT64_MAX="(9223372036854775807L)" -DINTPTR_MAX=INT64_MAX \
        --library=motif \
        --suppress=missingIncludeSystem \
        src/
