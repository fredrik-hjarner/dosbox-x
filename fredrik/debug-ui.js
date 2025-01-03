#!/usr/bin/env node

// TODO: Use es module instead!!!
const fs = require('fs');
const { execSync } = require('child_process');

// Configuration
const DATA_PIPE = '/tmp/dosbox_debug_pipe'; // Dosbox-x sends to this pipe.
const COMMAND_PIPE = '/tmp/command_pipe';

// Create pipes if they don't exist
console.log('Checking pipes...');
try {
    [DATA_PIPE, COMMAND_PIPE].forEach(pipe => {
        if (!fs.existsSync(pipe)) {
            execSync(`mkfifo ${pipe}`);
            console.log(`Created pipe: ${pipe}`);
        }
    });
} catch (error) {
    console.error('Error creating pipes:', error);
    process.exit(1);
}

// State
let shouldLog = true;
let dataStream = null;
let isHalted = false;
let writeStream = null;

// Pre-allocate buffer for halting
const FILL_BUFFER = Buffer.alloc(4096, 0);

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
        case 'h':
            if (!isHalted) {
                isHalted = true;
                dataStream.pause();
                
                // Fill the pipe buffer with zeros to block sender
                console.log('Halting - filling pipe buffer to block sender...');
                
                writeStream = fs.createWriteStream(DATA_PIPE, { flags: 'a' });
                let canContinue = true;
                while (canContinue) {
                    canContinue = writeStream.write(FILL_BUFFER);
                }
                
                writeStream.end();
                writeStream = null;
                console.log('Pipe buffer filled, sender should be blocked');
            }
            break;
        case 'c':
            if (isHalted) {
                isHalted = false;
                dataStream.resume();
                console.log('Continued - clearing buffer and resuming data flow');
            }
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

// Main function to set up pipe listeners
function main() {
    // Set up data pipe reader with non-blocking open
    dataStream = fs.createReadStream(DATA_PIPE, { flags: 'r' });
    console.log('Opening data pipe...');
    
    dataStream.on('ready', () => {
        console.log('Data pipe connected');
    });
    
    dataStream.on('data', (data) => {
        if (shouldLog && !isHalted) {
            // Only write non-zero data (filter out our fill bytes)
            if (data.some(byte => byte !== 0)) {
                process.stdout.write(data);
            }
        }
    });
    dataStream.on('error', (error) => {
        console.error('Error reading from data pipe:', error);
    });

    // Initialize command pipe
    setupCommandPipe();

    // Handle cleanup on exit
    process.on('SIGINT', () => {
        console.log('\nCleaning up...');
        dataStream.destroy();
        console.log('Data stream destroyed');
        commandStream.destroy();
        console.log('Command stream destroyed');
        if (writeStream) writeStream.end();
        console.log('Write stream ended');
        setTimeout(() => process.kill(process.pid, 'SIGKILL'), 100);
    });

    console.log('Pipe handler started. Commands:');
    console.log('  d - disable logging');
    console.log('  l - enable logging');
    console.log('  h - halt data stream (quick block via buffer fill)');
    console.log('  c - continue data stream');
}

main();