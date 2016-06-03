#!/bin/sh

set -e

for file in "$@"; do
    if ! grep -q '^// Local Variables:$' "$file"; then
        rm -f "$file.new"
        cat "$file" | sed '$a\' > "$file.new"
        MODE=c++
        if [ "$file" != "${file%.cl}" ]; then
            MODE=c
        fi
        cat >> "$file.new" <<EOF

// Local Variables:
// mode: $MODE
// tab-width: 4
// c-basic-offset: 4
// End:
EOF
        mv "$file.new" "$file"
    fi
done
