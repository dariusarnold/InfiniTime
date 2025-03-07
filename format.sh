#!/usr/bin/env bash

set -euo pipefail

# Add files or directories to exclude when formatting (supports globs)
EXCLUDED_FILES=(
    # The paths src/libs/* and src/FreeRTOS/* are excluded from formatting 
    # test-format.
    "./src/libs/*" 
    "./src/FreeRTOS/*"
    # Upstream uses test-format.sh in the pipeline, but it is only used
    # to check the files changed in a PR. It appears that the whole repo
    # has not yet been formatted completely.
    # These files are reformatted differently when running format.sh.
    # To avoid ceating large unneccessary diffs to upstream, avoid formatting.
    "./src/drivers/Bma421_C/bma4_defs.h" 
    "./src/drivers/Bma421_C/bma4.h" 
    "./src/drivers/Bma421_C/bma423.h" 
    "./bootloader/boot_graphics.h" 
)

SOURCE_FILES=$(find . -type f \( -iname '*.h' -o -iname '*.cpp' \) -a \( -not -path "./build*" -a -not -path "./cmake-build*" \))

formatted_count=0
skipped_count=0


for file in $SOURCE_FILES; do
    [ -e "$file" ] || continue

    for excluded in "${EXCLUDED_FILES[@]}"; do
        # We want to glob match here to support globs in EXCLUDED_FILES.
        # shellcheck disable=SC2053
        if [[ "$file" == $excluded ]]; then
            echo "Excluding $file"
            skipped_count=$((skipped_count + 1))
            continue 2  # Skip both inner and outer loop iteration to go to next file
        fi
    done

    echo Formatting "$file"
    formatted_count=$((formatted_count + 1))
    clang-format-14 --style file -i "$file"
done

echo "Files formatted: $formatted_count"
echo "Files skipped: $skipped_count"

exit 0
