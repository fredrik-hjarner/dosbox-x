#!/bin/bash

# Default pipe path
PIPE_PATH="/tmp/command_pipe"

# Check if the pipe exists
if [ ! -p "$PIPE_PATH" ]; then
    echo "Error: Pipe $PIPE_PATH does not exist!"
    exit 1
fi

# Save current terminal settings
old_settings=$(stty -g)

# Setup cleanup function
cleanup() {
    stty "$old_settings"
    echo ""
    exit 0
}

# Set trap before changing terminal settings
trap cleanup INT TERM EXIT

# Configure terminal to read raw input
stty raw -echo

printf "Sending keystrokes to $PIPE_PATH (Press Ctrl+C to exit)\r\n"

# Open the pipe in non-blocking mode for writing
exec 3<>$PIPE_PATH

# Read and send characters
while true; do
    # Read a single character
    char=$(dd bs=1 count=1 2>/dev/null)
    
    # Check for Ctrl+C (ASCII code 3)
    if [ "$char" = $'\3' ]; then
        cleanup
    fi
    
    # Send the character to the pipe using the file descriptor
    echo -n "$char" >&3
    
    # Print feedback about the sent character (to stderr to avoid mixing with pipe output)
    printf "Sent: %s\r\n" "$char" >&2
done