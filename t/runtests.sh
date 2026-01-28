#!/bin/bash

###############################################################################
# TVI Test Runner - Simplified Version
###############################################################################
# Format: <crc> <pattern>
# Example: 4f59dddb llx
###############################################################################

# Configuration
TIMEOUT=5
RESULTS_DIR="results"
TEST_EXE="../tvi"
CRC_EXE="./docrc"

# Find test executable
if [[ -x "../tvi_debug" ]]; then
    TEST_EXE="../tvi_debug"
elif [[ -x "../tvi_static" ]]; then
    TEST_EXE="../tvi_static"
elif [[ -x "../tvi" ]]; then
    TEST_EXE="../tvi"
else
    echo "Error: No TVI executable found"
    exit 1
fi
if [[ ! -x "$CRC_EXE" ]]; then
    echo "Error: No $CRC_EXE executable found"
    exit 1
fi

if [[ $# -ne 1 ]]; then
    echo "Usage: $0 <pattern_file> [<txt_file>]"
    echo "Example: $0 -               # use default pattern file"
    echo "         $0 all             # Use all pattern files"
    echo "         $0 'pattern_*.txt' # Use pattern_*.txt files"
    exit 1
fi

PATTERN_FILE="$1"
TXTDATA_FILE="textdata.txt"
[ -n "$2" ] && [ -f "$2" ] && TESTDATA_FILE="$2"

if [[ "$PATTERN_FILE" = "-" ]]; then
    if [[ -f "patterns.txt" ]]; then
        PATTERN_FILE="patterns.txt"
    else
        echo "Error: No default pattern file found (patterns.txt)"
        exit 1
    fi
elif [[ "$PATTERN_FILE" = "all" ]]; then
    PATTERN_FILE="pattern*.txt"
fi

if [[ ! -f "$TXTDATA_FILE" ]] ; then
   # --- create file textdata.txt data
   cat > "$TXTDATA_FILE" << 'EOF'
1 This is a test paragraph with multiple words.
2 Here we have some text that has several words.
3 The quick brown fox jumps over the lazy dog.
4 Testing word movements and operations.
EOF
   #
fi

# Setup results directory
mkdir -p "$RESULTS_DIR"
rm -f "$RESULTS_DIR"/test.* 2>/dev/null

echo "# --- Running TVI tests from $PATTERN_FILE using $TESTDATA_FILE"
echo "# --- Test executable: $TEST_EXE"

# Process patterns
test_count=0
test_passed=0

for file in $PATTERN_FILE; do
  echo ""
  echo "# file: $file"
  while read crc pattern info; do
    # Skip empty lines and comments
    [[ -z "$crc"    ]] && continue
    [[ "$crc" = "#" ]] && continue
    ((test_count++))
    if [[ -z "$pattern" ]]; then
        printf "%-3s %-15s %s\n" "$test_count" "INVALID_FORMAT" "skipped"
        continue
    fi

    # Prepare test file
    tempfn="$RESULTS_DIR/test.tmp_$$"
    testfn="$RESULTS_DIR/test.$test_count"
    cp "$TXTDATA_FILE" "$testfn"
    echo "#" >> "$testfn"

    echo "$pattern"|timeout $TIMEOUT $TEST_EXE -i - $TXTDATA_FILE >tempfn 2>/dev/null
    status=$?

    result="failed"
    if [[ $status -eq 0 ]]; then
        # Calculate actual CRC
        actual_crc=$(cat tempfn|$CRC_EXE "$pattern")

        if [[ "$crc" = "$actual_crc" ]]; then
           result="passed"
        fi
    else
        result="timeout"
    fi

    # Display result
    printf "%-3s %-7s %-24s" "$test_count" "$result" "$pattern"

    # Log details
    printf "# %s pat = %-12s     %-8s %-8s  %s\n" \
           "$result" "$pattern" "$crc" "${actual_crc:-TIMEOUT}" "$info" >> "$testfn"

    [[ "$result" != "timeout" ]] && cat tempfn >> "$testfn" 2>/dev/null
    rm -f tempfn

    # Format output in columns
    if [[ $((test_count % 3)) -eq 0 ]]; then
        echo ""
    else
        printf " | "
    fi
  done < "$file"
  # Final newline
  if [[ $((test_count % 3)) -ne 0 ]]; then
    echo ""
  fi
done

# Summary
echo ""
test_count=$(ls results/test.*|wc -l)
test_passed=$(cat results/test.*|grep "# passed "|wc -l)
test_failed=$(cat results/test.*|grep "# failed "|wc -l)
test_timeout=$(cat results/test.*|grep "# timeout "|wc -l)
#
echo "# Total tests = $test_count"
[ "$test_passed"  != "0" ] && echo "- passed  : $test_passed"
[ "$test_failed"  != "0" ] && echo "- failed  : $test_failed"
[ "$test_timeout" != "0" ] && echo "- timeout : $test_timeout"

[[ $test_passed = $test_count ]] && exit 0
exit 1
#EOF
