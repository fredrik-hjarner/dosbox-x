#!/usr/bin/env node

// TODO: Use es module instead!!!
const fs = require('fs');
const { execSync } = require('child_process');
const net = require('net');

// Configuration
// Rename SOCKET_PATH to DATA_SOCKET_PATH
const SOCKET_PATH = '/tmp/debug_socket';  // New socket path
const COMMAND_PIPE = '/tmp/command_pipe';
const MAX_MESSAGE_SIZE = 1000000;  // 1MB

// Create command pipe if it doesn't exist
console.log('Checking command pipe...');
try {
    if (!fs.existsSync(COMMAND_PIPE)) {
        execSync(`mkfifo ${COMMAND_PIPE}`);
        console.log(`Created pipe: ${COMMAND_PIPE}`);
    }
} catch (error) {
    console.error('Error creating command pipe:', error);
    process.exit(1);
}

// Clean up the socket file if it exists from a previous run
if (fs.existsSync(SOCKET_PATH)) {
    fs.unlinkSync(SOCKET_PATH);
}

// State
let shouldLog = true;
let commandStream = null;  // Make this global so cleanup handler can access it

// Message parsing state - each message has a 4-byte length prefix followed by the actual message
const MESSAGE_LENGTH_SIZE = 4;  // Size of the length prefix in bytes

// Buffer to store the incoming 4-byte length prefix
let lengthBuffer = Buffer.alloc(MESSAGE_LENGTH_SIZE);
let lengthBufferOffset = 0;     // How many bytes of the length prefix we've read so far

// Buffer to store the actual message content
let messageBuffer = null;        // Allocated once we know the message length
let messageBufferOffset = 0;     // How many bytes of the message we've read so far
let expectedLength = 0;         // Total length of the message we're expecting

/**
 * Reads the 4-byte length prefix from the incoming data.
 * Once we have all 4 bytes, prepares the message buffer for the actual content.
 * 
 * @param {Buffer} data - The incoming data chunk
 * @param {number} offset - Current position in the data chunk
 * @returns {number} - New position in the data chunk
 */
function readMessageLength(data, offset) {
    // Calculate how many bytes we can copy into our length buffer
    const remainingLengthBytes = MESSAGE_LENGTH_SIZE - lengthBufferOffset;
    const availableBytes = data.length - offset;
    const bytesToCopy = Math.min(remainingLengthBytes, availableBytes);

    // Copy bytes into length buffer
    data.copy(lengthBuffer, lengthBufferOffset, offset, offset + bytesToCopy);
    lengthBufferOffset += bytesToCopy;

    // If we've got all 4 bytes of the length prefix
    if (lengthBufferOffset === MESSAGE_LENGTH_SIZE) {
        // Convert the 4 bytes to a 32-bit unsigned integer (little-endian)
        expectedLength = lengthBuffer.readUInt32LE(0);
        
        // Basic sanity check on message length
        if (expectedLength <= 0) {  // 1MB max
            console.error(`Message length too small: ${expectedLength}`);
        }
        if (expectedLength > MAX_MESSAGE_SIZE) {  // 1MB max
            console.error(`Message length too large: ${expectedLength}`);
        }

        // Prepare buffer for the actual message
        messageBuffer = Buffer.alloc(expectedLength);
        messageBufferOffset = 0;
        
        // Reset length buffer for next message
        lengthBufferOffset = 0;
    }

    return offset + bytesToCopy;
}

/**
 * Reads the actual message content into the message buffer.
 * Once we have the complete message, processes it.
 * 
 * @param {Buffer} data - The incoming data chunk
 * @param {number} offset - Current position in the data chunk
 * @returns {number} - New position in the data chunk
 */
function readMessageContent(data, offset) {
    // Calculate how many bytes we can copy into our message buffer
    const remainingMessageBytes = expectedLength - messageBufferOffset;
    const availableBytes = data.length - offset;
    const bytesToCopy = Math.min(remainingMessageBytes, availableBytes);

    // Copy bytes into message buffer
    data.copy(messageBuffer, messageBufferOffset, offset, offset + bytesToCopy);
    messageBufferOffset += bytesToCopy;

    // If we've got the complete message
    if (messageBufferOffset === expectedLength) {
        processCompleteMessage(messageBuffer);
        // Reset message buffer for next message
        messageBuffer = null;
    }

    return offset + bytesToCopy;
}

/**
 * Processes a complete message by converting it to a string and writing to stdout.
 * Messages are only written if logging is enabled.
 * 
 * @param {Buffer} buffer - The complete message buffer
 */
function processCompleteMessage(buffer) {
    // Only write to stdout if logging is enabled
    if (shouldLog) {
        const message = buffer.toString('utf8');
        process.stdout.write(message);
    }
}

// Socket server setup
const server = net.createServer((socket) => {
    console.log('DOSBox-X connected');
    
    socket.on('data', (data) => {
        // Always process messages to maintain buffer state
        // but only output them if shouldLog is true
        let offset = 0;
        while (offset < data.length) {
            // Each message starts with a 4-byte length prefix
            // Once we have the length, we read that many bytes as the message
            offset = messageBuffer === null
                ? readMessageLength(data, offset)    // Reading length prefix
                : readMessageContent(data, offset);  // Reading message content
        }
    });

    socket.on('error', (err) => {
        console.error('Socket error:', err);
    });

    socket.on('close', () => {
        console.log('DOSBox-X disconnected');
        // Reset message parsing state
        lengthBuffer = Buffer.alloc(MESSAGE_LENGTH_SIZE);
        lengthBufferOffset = 0;
        messageBuffer = null;
        messageBufferOffset = 0;
        expectedLength = 0;
    });
});

// Handle commands from the command pipe
function handleCommand(command) {
    switch (command) {
        case 'd':
            shouldLog = false;
            console.log('Logging disabled');
            break;
        case 'l':
            shouldLog = true;
            console.log('Logging enabled');
            break;
        default:
            console.log(`Unknown command: ${command}`);
    }
}

// Set up command pipe reader with non-blocking open
function setupCommandPipe() {
    const commandStream = fs.createReadStream(COMMAND_PIPE, { flags: 'r' });
    console.log('Opening command pipe...');
    
    commandStream.on('ready', () => {
        console.log('Command pipe connected');
    });

    commandStream.on('data', (data) => {
        const commands = data.toString().trim().split('');
        commands.forEach(handleCommand);
    });

    commandStream.on('error', (error) => {
        console.error('Error reading from command pipe:', error);
    });

    // When the pipe ends (writer closes), set up a new reader
    commandStream.on('end', () => {
        console.log('Command pipe ended.');
        console.log('Reconnecting to command pipe...');
        // TODO: This might cause tons of memory leaks and stuff, no?
        // Well if it works it works but it does not make sense how pipes work!
        setTimeout(setupCommandPipe, 100); // Reconnect after a short delay
    });
}

// Main function to set up pipe/socket listeners
function main() {
    // Start the socket server
    server.listen(SOCKET_PATH, () => {
        console.log(`Socket server listening on ${SOCKET_PATH}`);
    });

    server.on('error', (err) => {
        console.error('Server error:', err);
    });

    // Initialize command pipe
    setupCommandPipe();

    // Handle cleanup on exit
    process.on('SIGINT', () => {
        console.log('\nCleaning up...');

        if (commandStream) {
            commandStream.destroy();
            console.log('Command stream destroyed');
        }
        
        // socket.destroy();
        
        server.close();
        if (fs.existsSync(SOCKET_PATH)) {
            fs.unlinkSync(SOCKET_PATH);
        }
        setTimeout(() => process.kill(process.pid, 'SIGKILL'), 100); // TODO: Keep this comment!!!
        // process.exit();
    });

    console.log('Debug UI started. Commands:');
    console.log('  d - disable logging');
    console.log('  l - enable logging');
}

main();