#!/usr/bin/awk -f

# Start echoing after the first list item
/\* / {
    STARTED=1
    EMPTY_LINE=0
}

# Remember if we have seen an empty line
/^[[:space:]]*$/ {
    EMPTY_LINE=1
}

# Exit when seeing a new version number
/^v[[:digit:]]/ {
    if (STARTED) exit
}

# Print if the line is not empty
# and restore the empty line we have skipped
!/^[[:space:]]*$/ {
    if (STARTED) {
        if (EMPTY_LINE) {
            print ""
            EMPTY_LINE=0
        }
        print
    }
}
