#include "globals_and_consts.h"
#include "address.h"
#include "address_range.h"
#include "address_ranges.h"
#include "get_cpu_instruction_line_string.h"

// No. Dont add any more includes!!!
// Ignore that these imports are incomplete or look weird. It is correct.


static std::set<std::string> uniqueAddresses;

static bool IsVsync(uint16_t segment, uint32_t offset) {
    if (segment == 0x565A) {
        if (offset == 0x134A) {
            return true;
        }
        if (offset == 0x134B) {
            return true;
        }
        if (offset == 0x134D) {
            return true;
        }
    }
	return false;
}

static bool IsGraphics565A(uint16_t segment) {
	return segment == 0x565A; // This code includes the loop waiting for vertical trace.
}
static bool IsGraphics5677(uint16_t segment) {
	return segment == 0x5677;
}
static bool IsGraphics5FBE(uint16_t segment) {
	return segment == 0x5FBE;
}

static bool IsEmsWindow1(uint16_t segment) {
	return (segment >= 0xE000 && segment <= 0xE3FF);
}
static bool IsEmsWindow2(uint16_t segment) {
	return (segment >= 0xE400 && segment <= 0xE7FF);
}
static bool IsEmsWindow3(uint16_t segment) {
	return (segment >= 0xE800 && segment <= 0xEBFF);
}
static bool IsEmsWindow4(uint16_t segment) {
	return (segment >= 0xEC00 && segment <= 0xEFFF);
}

static bool IsVideoBIOS(uint16_t segment) {
	return (segment >= 0xC000 && segment <= 0xC7FF);
}

static bool IsMotherboardBIOS(uint16_t segment) {
	return (segment >= 0xF000 && segment <= 0xFFFF);
}

// Not in use as of now.
static bool SkipSendingInstruction(uint16_t segment, uint32_t offset) {
    if (IsVsync(segment, offset)) {
        return true;
    } else if (IsMotherboardBIOS(segment)) {
        return true;
    } else if (IsVideoBIOS(segment)) {
        return true;
    }
    return false;
}

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/un.h>

// TODO: autoDisassemblerMode does not need UnixSocketSender at all.
class UnixSocketSender {
private:
    int sock_fd;
    struct sockaddr_un addr;
    bool is_connected;
    
public:
    UnixSocketSender() : sock_fd(-1), is_connected(false) {
        // Create socket
        sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sock_fd == -1) {
            // Handle socket creation error
            std::cerr << "\n\n*** ERROR: Failed to create debug socket: " << strerror(errno) << " (errno: " << errno << ") ***\n\n" << std::endl;
            return;
        }
        
        // Setup address structure
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, "/tmp/debug_socket", sizeof(addr.sun_path) - 1);

        this->connect(); // connect immediately in constructor.
    }
    
    bool connect() {
        if (is_connected) return true;
        if (sock_fd == -1) {
            std::cerr << "\n\n*** ERROR: connect: Failed to create debug socket: " << strerror(errno) << " (errno: " << errno << ") ***\n\n" << std::endl;
            return false;
        }
        
        is_connected = (::connect(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) == 0);
        if (!is_connected) {
            std::cerr << "\n\n*** ERROR: connect: Failed to connect to debug socket: " << strerror(errno) << " (errno: " << errno << ") ***\n\n" << std::endl;
        }
        return is_connected;
    }
    
    bool write(const std::string& message) {
        if (!is_connected) {
            std::cerr << "\n\n*** ERROR: write: Failed to connect to debug socket: " << strerror(errno) << " (errno: " << errno << ") ***\n\n" << std::endl;
            return false;
        }

        // TODO: This might be faster but I don't know.
        // Just send the message directly
        std::size_t length = ::write(sock_fd, message.c_str(), message.length());
        if (length != message.length()) {
            std::cerr << "\n\n*** ERROR: write: Failed to send message: " << strerror(errno) << " (errno: " << errno << ") ***\n\n" << std::endl;
            is_connected = false;
            return false;
        }
        return true;
    }
    
    ~UnixSocketSender() {
        if (sock_fd != -1) {
            close(sock_fd);
        }
    }
};

std::string GetLabelForAddress(uint16_t segValue, uint32_t eipValue) {
    if (IsVsync(segValue, eipValue)) {
        return "WAIT_FOR_VSYNC";
    } else if (IsMotherboardBIOS(SegValue(cs))) {
        return "MOTHERBOARD_BIOS";
    } else if (IsVideoBIOS(SegValue(cs))) {
        return "VIDEO_BIOS";
    } else if (IsEmsWindow1(SegValue(cs))) {
        return "EMS window 1";
    } else if (IsEmsWindow2(SegValue(cs))) {
        return "EMS window 2";
    } else if (IsEmsWindow3(SegValue(cs))) {
        return "EMS window 3";
    } else if (IsEmsWindow4(SegValue(cs))) {
        return "EMS window 4";
    } else if (IsGraphics565A(SegValue(cs))) {
        return "Graphics1 cant find in EXE";
    } else if (IsGraphics5677(SegValue(cs))) {
        return "Graphics2 cant find in EXE";
    } else if (IsGraphics5FBE(SegValue(cs))) {
        return "Graphics3 cant find in EXE";
    }
    return "";
}

// TODO: autoDisassemblerMode does not need UnixSocketSender at all.
static UnixSocketSender debugSocket;

// When this is 1 it's very very slow, so I added this buffer to speed up things.
// Things are not outputted immediately, but are collected in buffer for a short while.
int BUFFER_FLUSH_SIZE = autoDisassemblerMode ? 30 : 1;
int bufferCount = 0;
std::stringstream buffer;

// TODO: Refactor this is getting messy. I could have more small helper functions.
static void LogInstruction2(uint16_t segValue, uint32_t eipValue, ofstream& out) {
    // this is only so that we will not be in a state where no new instructions are logged.
    // that might otherwise happen, let's say, if we only log unique addresses and there
    // are no unique addresses for a long while, then there might be sat 15 instructions
    // in the buffer that will only displayed in we successfully trigger new unique addresses.
    // When we log all instuctions and not just new instructions then that problem would not
    // exist though.
    bufferCount++; // one central place to increment bufferCount.

    // In the disassembly I want all instructions.
    // Not sure I want to even skip in "unique address" mode since
    // vsync has probably been called thousands of times.
    // if(!autoDisassemblerMode && SkipSendingInstruction(segValue, eipValue)) {
    //     return;
    // }

    // Format the address
    std::stringstream addressStream;
    addressStream << std::hex << std::uppercase << std::setfill('0') 
                 << std::setw(4) << SegValue(cs) << ":"
                 << std::setw(4) << reg_eip;
    std::string address = addressStream.str();

    // Only add to buffer if this is a new address
    // if (uniqueAddresses.insert(address).second || true) { // TODO: The true is just a hack. I wanted to test to output it all.
    if (uniqueAddresses.insert(address).second) {
    // if (true) {
        buffer
            << GetCpuInstructionLineString(segValue, eipValue, autoDisassemblerMode)
            << GetLabelForAddress(segValue, eipValue)
            << "\n";
    }

    if (bufferCount >= BUFFER_FLUSH_SIZE && buffer.tellp() > 0) {
        std::string bufferAsString = buffer.str();

        if(autoDisassemblerMode) {
            if (!cpuLogFile.is_open()) {
                cpuLogFile.open("auto_disassembly.txt");
            }
            out << bufferAsString; // print to file instead of pipe
        } else {
            // Write to socket if connected
            debugSocket.write(bufferAsString);
            
        }
        buffer.str("");
        buffer.clear();
        bufferCount = 0;
    }

}

/*
TODO:
- Log when a function was entered by checking if the previous instruction was a call (or similar).
- Log when a function when instruction is ret (or similar).
- Add many more items to my "memory map". I can add functions that I name for example.
  - I can even for certain code or functions only show like that it was called and exited and skips the rest if it's not interesting.
    - I can have som kind of toggle in my map to show or hide the code.
  - vsync loop should probably be totally hidden to not cause spam.
  - certain functions are the "accumulated" for example say some graphics function that runs 100 times in a row, then I just show 1 line.
- I probably should add some or all registers because then I could see for example which places in memory was fetched and updated.
- I have an idea of using named pipe.
  - dosbox-x just spits out every single instruction to a named pipe,
  - then js listens to it and it can either display it in terminal but also take commands at the same time.
    - js can have command to listen or to discard or maybe to do memory dumps too (but I dont quite know how that would work).
	  - regarding memory dumps they should get unique names, not like now in dosvox-x when LOGCPU.TXT gets overwritten.
- Save and reload state would be very valuable when debugging, try something then rewind change a variable or try something else
  then combine it with mem or instruction diffing to see what the difference was!!!
  Yes, dosbox-x has save state and load state!
- I could probably figure out a way to trigger a breakpoint x steps before the breakpoint
  (if the instruction is visible under in code window).
- Now that I have and Address struct I can have all the "memory ranges" be absolute addresses intead to speed things up.







*/