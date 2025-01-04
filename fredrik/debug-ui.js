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

// Socket server setup
const server = net.createServer((socket) => {
    console.log('DOSBox-X connected');
    
    socket.on('data', (data) => {
        if (shouldLog) {
            const message = data.toString('utf8');
            process.stdout.write(message);
        }
    });

    socket.on('error', (err) => {
        console.error('Socket error:', err);
    });

    socket.on('close', () => {
        console.log('DOSBox-X disconnected');
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