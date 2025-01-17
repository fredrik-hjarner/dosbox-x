#include "globals_and_consts.h"
#include "debug_socket.h"
#include "overlays2.h"
#include "overlays.h"
// #include "address.h"
// #include "address_range.h"
// #include "address_ranges.h"
#include "get_cpu_instruction_line_string.h"

// No. Dont add any more includes!!!
// Ignore that these imports are incomplete or look weird. It is correct.


static std::set<uint64_t> uniqueAddresses;

inline bool IsVsync(uint16_t segment, uint32_t offset) {
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

inline bool IsGraphics565A(uint16_t segment) {
	return segment == 0x565A; // This code includes the loop waiting for vertical trace.
}
inline bool IsGraphics5677(uint16_t segment) {
	return segment == 0x5677;
}
inline bool IsGraphics5FBE(uint16_t segment) {
	return segment == 0x5FBE;
}

inline bool IsEmsWindow1(uint16_t segment) {
	return (segment >= 0xE000 && segment <= 0xE3FF);
}
inline bool IsEmsWindow2(uint16_t segment) {
	return (segment >= 0xE400 && segment <= 0xE7FF);
}
inline bool IsEmsWindow3(uint16_t segment) {
	return (segment >= 0xE800 && segment <= 0xEBFF);
}
inline bool IsEmsWindow4(uint16_t segment) {
	return (segment >= 0xEC00 && segment <= 0xEFFF);
}

inline bool IsVideoBIOS(uint16_t segment) {
	return (segment >= 0xC000 && segment <= 0xC7FF);
}

inline bool IsMotherboardBIOS(uint16_t segment) {
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

// When this is 1 it's very very slow, so I added this buffer to speed up things.
// Things are not outputted immediately, but are collected in buffer for a short while.
#ifdef AUTO_DISASSEMBLER_MODE
    int BUFFER_FLUSH_SIZE = 30;
#else
    int BUFFER_FLUSH_SIZE = 1;
#endif
int bufferCount = 0;
std::stringstream buffer;

bool have_reached_entry_point = false;

// TODO: Refactor this is getting messy. I could have more small helper functions.
static void LogInstruction2(uint16_t segValue, uint32_t eipValue, ofstream& out) {
    // this is only so that we will not be in a state where no new instructions are logged.
    // that might otherwise happen, let's say, if we only log unique addresses and there
    // are no unique addresses for a long while, then there might be sat 15 instructions
    // in the buffer that will only displayed in we successfully trigger new unique addresses.
    // When we log all instuctions and not just new instructions then that problem would not
    // exist though.
    bufferCount++; // one central place to increment bufferCount.

    if (!have_reached_entry_point) {
        // 0823:0000 is the entry point
        if (segValue == 0x0823 && eipValue == 0x0000) {
            have_reached_entry_point = true;
        } else {
            // Skip logging/tracing until we've reached the entry point.
            return;
        }
    }

    // In the disassembly I want all instructions.
    // Not sure I want to even skip in "unique address" mode since
    // vsync has probably been called thousands of times.
    // if(!autoDisassemblerMode && SkipSendingInstruction(segValue, eipValue)) {
    //     return;
    // }

    bool is_in_stub = Overlays::is_stub_segment(segValue);
    if(is_in_stub) {
        // TODO: I only have offset here to do logging. remove it.
        Overlays::enter_stub(
            segValue
            // eipValue
        );
    }
    bool is_in_overlay = Overlays::is_overlay_segment(segValue);
    // TODO: This does not look too good.
    // I'm not happy about the passing of stub_segment into GetCpuInstructionLineString,
    // that part sucks.
    uint16_t stub_segment = 0;
    if(is_in_overlay) {
        // Note: So currently, I only log the overlay segment
        // when I enter the segment via 3xxx segment.
        // so it is not logged any other time, i.e. only entering the segment will be logged.
        Overlays::map_overlay(segValue);
        // TODO: I only have offset here to do logging. remove it.
        stub_segment = Overlays::get_stub(
            segValue
            // eipValue
        );
    }
    // TODO: yea I really need to start to clean up the code, Im adding
    // logs n stuf all over the place.
    if(previous_stub_segment != stub_segment) {
        // we entered a new overlay segment
        // std::cerr
        //     // print segment:offset first
        //     << std::hex << std::setfill('0') << std::setw(4) << previous_segment << ":"
        //     << std::setfill('0') << std::setw(4) << previous_offset
        //     << "->"
        //     << std::hex << std::setfill('0') << std::setw(4) << segValue << ":"
        //     << std::setfill('0') << std::setw(4) << eipValue

        //     << "   o." << std::hex << std::setfill('0') << std::setw(4) << previous_stub_segment
        //     << "->o."
        //     << std::hex << std::setfill('0') << std::setw(4) << stub_segment
            
        //     << std::endl;
        previous_stub_segment = stub_segment;
    }

    uint64_t maybeUniqueAddress = 0;
    if(is_in_overlay) {
        // add some number that can never be a valid address
        // FFFF:FFFF is the last address in the memory map which is 0x10FFEF
        // so a larger number than that to differentiate an overlay from a resident address.
        // oooh. not only that I need stub_segment to be unique too so I need to shift it more.
        // now max number would look like this 100FFFF0FFFF
        // (observe segment and offset does not overlap).
        maybeUniqueAddress = ((stub_segment << 0x14) + reg_eip) + 0x100000000000;
    } else {
        maybeUniqueAddress = (SegValue(cs) << 4u) + reg_eip;
    }

    // Only add to buffer if this is a new address
    // if (uniqueAddresses.insert(address).second || true) { // TODO: The true is just a hack. I wanted to test to output it all.
    if (uniqueAddresses.insert(maybeUniqueAddress).second) {
    // if (true) {
        buffer
            << GetCpuInstructionLineString(segValue, eipValue, stub_segment)
            << GetLabelForAddress(segValue, eipValue)
            << "\n";
    }

    if (bufferCount >= BUFFER_FLUSH_SIZE && buffer.tellp() > 0) {
        std::string bufferAsString = buffer.str();

#ifdef AUTO_DISASSEMBLER_MODE
        if (!cpuLogFile.is_open()) {
            cpuLogFile.open("auto_disassembly.txt");
        }
        out << bufferAsString; // print to file instead of pipe
#else
        // Write to socket if connected
        debugSocket.write(bufferAsString);
#endif

        buffer.str("");
        buffer.clear();
        bufferCount = 0;
    }

    // remember to save the previous segment and offset.
    previous_segment = segValue;
    // previous_offset = eipValue;

#ifdef SAVE_INSTRUCTIONS_HISTORY
    previous_instruction = GetCpuInstructionLineString(segValue, eipValue, stub_segment);
#endif
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
- Make a IsResidentCode function, i.e. that the address is not in any overlay.
  - I could prolly then map each memory location to an address in the KRONDOR.EXE.
    - That would be helpful because that might aide me to find the code in IDA.
    - I probably want to write the file address in the log too because for overlays any other kind of address is meaningless (right?).
- Add `if (reg_esi > 0xFFFF || reg_edi > 0xFFFF) {` but for SP and BP too so I am sure they never get larger than 0xFFFF.
   - because that's my assumption but it can be wrong.
- Ignore instructions until we're gotten into the KRONDOR code.
- TODO: The overlay stuff and so on must be optimized so that the game does not run slower.
- TODO: I suspect that there are stuff in 4xxx that gets classified as not overlay in log, but that are overlays.
- TODO: Error: No stub found for overlay 4476
    - Error: No stub found for overlay 4494:0247
        - Entered stub 305c:0020
          Removed old mapping for stub 305c from overlay 4984
          Created new mapping for stub 305c at overlay 4984
          Error: No stub found for overlay 4494:0247
 - learn how to set a breakpoint via code, I can bp on error for example.
 - clean up and speed up the code. I think the auto-disassembler mode works fully now.
 - make sure that the overlay stuff works by doing my longer time check (in Overlay2),
   so I'm 100% sure it works. 







*/