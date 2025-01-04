// No. Dont add any includes!!!
// Ignore that these imports are incomplete or look weird. It is correct.




// This function is only supposed to be responsible for adding the instruction as text,
// it has no other responsibilities.
// I want it to assume cpuLogType == 2 and showExtend == true
// otherwise it's exactly like the original LogInstruction function.
// Well actually, another difference is that this returns a string instead of writing to an ofstream that it took in as parameter.
static std::string LogInstructionWithHardCodedValues(uint16_t segValue, uint32_t eipValue) {
    std::stringstream out;
    out << std::hex << std::noshowbase << std::setfill('0') << std::uppercase;

	static char empty[23] = { 32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,0 };

	PhysPt start = (PhysPt)GetAddress(segValue,eipValue);
	char dline[200];Bitu size;
	size = DasmI386(dline, start, reg_eip, cpu.code.big);
	char* res = empty;

    res = AnalyzeInstruction(dline,false);
    if (!res || !(*res)) res = empty;
    Bitu reslen = strlen(res);
    if (reslen < 22) {
        memset(res + reslen, ' ', 22 - reslen);
        res[22] = 0;
    }

	Bitu len = strlen(dline);
    if (len < 30) {
        memset(dline + len, ' ', 30 - len);
        dline[30] = 0;
    }

	// Get register values
    char ibytes[200]="";	char tmpc[200];
    for (Bitu i=0; i<size; i++) {
        uint8_t value;
        if (mem_readb_checked((PhysPt)(start+i),&value)) sprintf(tmpc,"%s","?? ");
        else sprintf(tmpc,"%02X ",value);
        strcat(ibytes,tmpc);
    }
    len = strlen(ibytes);
    if (len < 21) {
        for (Bitu i = 0; i < 21 - len; i++) ibytes[len + i] = ' ';
        ibytes[21] = 0;
    }
    out << setw(4) << SegValue(cs) << ":" << setw(4) << reg_ip << "  " << dline << "  " << res << "  " << ibytes;

	out
        << " A:" << setw(8) << reg_eax
        << " B:" << setw(8) << reg_ebx
	    << " C:" << setw(8) << reg_ecx
        << " D:" << setw(8) << reg_edx
	    << " SI:" << setw(8) << reg_esi
        << " DI:" << setw(8) << reg_edi
	    // << " EBP:" << setw(8) << reg_ebp
        // << " ESP:" << setw(8) << reg_esp
        << " BP:" << setw(4) << reg_bp // Not 100% sure I can rely on bp and sp being just within 16bits.
        << " SP:" << setw(4) << reg_sp  // Not 100% sure I can rely on bp and sp being just within 16bits.
	    << " DS:"  << setw(4) << SegValue(ds)
        << " ES:"  << setw(4) << SegValue(es);

    out
        // << " FS:"  << setw(4) << SegValue(fs) // seem to have no use
        // << " GS:"  << setw(4) << SegValue(gs) // seem to have no use
        << " SS:"  << setw(4) << SegValue(ss);
        // << " CF:"  << (get_CF()>0)
        // << " ZF:"   << (get_ZF()>0)
        // << " SF:"  << (get_SF()>0)
        // << " OF:"  << (get_OF()>0)
        // << " AF:"   << (get_AF()>0)
        // << " PF:"  << (get_PF()>0)
        // << " IF:"  << GETFLAGBOOL(IF);

    // out
        // << " TF:" << GETFLAGBOOL(TF) // Trap flag
        // << " VM:" << GETFLAGBOOL(VM) // Virtual 8086 mode flag (386+ only). Always zero.
        // << " FLG:" << setw(8) << reg_flags // I don't know what FLG is.
        // << " CR0:" << setw(8) << cpu.cr0; // I don't know what CR is.

	// out << endl; // scrapping the endline that is added afterwards

    return out.str();
}




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

static bool SkipSendingInstruction(uint16_t segment, uint32_t offset) {
    if (IsVsync(segment, offset)) {
        return true;
    }
    return false;
}

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/un.h>

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

static UnixSocketSender debugSocket;

std::stringstream buffer;
int bufferCount = 0;
int BUFFER_FLUSH_SIZE = 20;

// TODO: Refactor this is getting messy. I could have more small helper functions.
static void LogInstruction2(uint16_t segValue, uint32_t eipValue, ofstream& out) {
    if(SkipSendingInstruction(segValue, eipValue)) {
        bufferCount++; // always increment bufferCount. I can try not to in the future, dunno.
        return;
    }

    // Format the address
    std::stringstream addressStream;
    addressStream << std::hex << std::uppercase << std::setfill('0') 
                 << std::setw(4) << SegValue(cs) << ":"
                 << std::setw(4) << reg_eip;
    std::string address = addressStream.str();
    
    // Only add to buffer if this is a new address
    // if (uniqueAddresses.insert(address).second || true) { // TODO: The true is just a hack. I wanted to test to output it all.
    // if (uniqueAddresses.insert(address).second) {
    if (true) {
      
        buffer << LogInstructionWithHardCodedValues(segValue, eipValue);

        // Add BIOS indicator if in BIOS range
        if (IsMotherboardBIOS(SegValue(cs))) {
            buffer << std::setw(25) << "<< Motherboard BIOS >>";
        } else if (IsVideoBIOS(SegValue(cs))) {
            buffer << std::setw(25) << "<< Video BIOS >>";
        } else if (IsEmsWindow1(SegValue(cs))) {
            buffer << std::setw(25) << "<< EMS window 1 >>";
        } else if (IsEmsWindow2(SegValue(cs))) {
            buffer << std::setw(25) << "<< EMS window 2 >>";
        } else if (IsEmsWindow3(SegValue(cs))) {
            buffer << std::setw(25) << "<< EMS window 3 >>";
        } else if (IsEmsWindow4(SegValue(cs))) {
            buffer << std::setw(25) << "<< EMS window 4 >>";
        } else if (IsGraphics565A(SegValue(cs))) {
            buffer << std::setw(25) << "<< Graphics1 cant find in EXE >>";
        } else if (IsGraphics5677(SegValue(cs))) {
            buffer << std::setw(25) << "<< Graphics2 cant find in EXE >>";
        } else if (IsGraphics5FBE(SegValue(cs))) {
            buffer << std::setw(25) << "<< Graphics3 cant find in EXE >>";
        }
        
        buffer << "\n";
    }
    bufferCount++;

    if (bufferCount >= BUFFER_FLUSH_SIZE && buffer.tellp() > 0) {
        std::string bufferAsString = buffer.str();
        
        // Write to socket if connected
        debugSocket.write(bufferAsString);
        
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







*/