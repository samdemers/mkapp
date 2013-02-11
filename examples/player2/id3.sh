#!/bin/sh
# Read file names on the standard input and update id3 information.
#

while read FILE; do
    
    TITLE=$(id3 -l -R "$FILE" | grep Title | cut -d ' ' -f 2-)
    ARTIST=$(id3 -l -R "$FILE" | grep Artist | cut -d ' ' -f 2-)
    
    echo "set_artist(\"$ARTIST\");"
    echo "set_title(\"$TITLE\");"

done
