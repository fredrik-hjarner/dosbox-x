// No. Dont add any includes!!!

// static void LogInstruction(uint16_t segValue, uint32_t eipValue,  ofstream& out) {

// 	static char empty[23] = { 32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,0 };

// 	if (cpuLogType == 3) { //Log only cs:ip.
// 		out << setw(4) << SegValue(cs) << ":" << setw(8) << reg_eip << endl;
// 		return;
// 	}

// 	PhysPt start = (PhysPt)GetAddress(segValue,eipValue);
// 	char dline[200];Bitu size;
// 	size = DasmI386(dline, start, reg_eip, cpu.code.big);
// 	char* res = empty;
// 	if (showExtend && (cpuLogType > 0) ) {
// 		res = AnalyzeInstruction(dline,false);
// 		if (!res || !(*res)) res = empty;
// 		Bitu reslen = strlen(res);
//         if (reslen < 22) {
//             memset(res + reslen, ' ', 22 - reslen);
//             res[22] = 0;
//         }
// 	}
// 	Bitu len = strlen(dline);
//     if (len < 30) {
//         memset(dline + len, ' ', 30 - len);
//         dline[30] = 0;
//     }

// 	// Get register values

// 	if(cpuLogType == 0) {
// 		out << setw(4) << SegValue(cs) << ":" << setw(4) << reg_eip << "  " << dline;
// 	} else if (cpuLogType == 1) {
// 		out << setw(4) << SegValue(cs) << ":" << setw(8) << reg_eip << "  " << dline << "  " << res;
// 	} else if (cpuLogType == 2) {
// 		char ibytes[200]="";	char tmpc[200];
// 		for (Bitu i=0; i<size; i++) {
// 			uint8_t value;
// 			if (mem_readb_checked((PhysPt)(start+i),&value)) sprintf(tmpc,"%s","?? ");
// 			else sprintf(tmpc,"%02X ",value);
// 			strcat(ibytes,tmpc);
// 		}
// 		len = strlen(ibytes);
//         if (len < 21) {
//             for (Bitu i = 0; i < 21 - len; i++) ibytes[len + i] = ' ';
//             ibytes[21] = 0;
//         }
// 		out << setw(4) << SegValue(cs) << ":" << setw(8) << reg_eip << "  " << dline << "  " << res << "  " << ibytes;
// 	}

// 	out << " EAX:" << setw(8) << reg_eax << " EBX:" << setw(8) << reg_ebx
// 	    << " ECX:" << setw(8) << reg_ecx << " EDX:" << setw(8) << reg_edx
// 	    << " ESI:" << setw(8) << reg_esi << " EDI:" << setw(8) << reg_edi
// 	    << " EBP:" << setw(8) << reg_ebp << " ESP:" << setw(8) << reg_esp
// 	    << " DS:"  << setw(4) << SegValue(ds)<< " ES:"  << setw(4) << SegValue(es);

// 	if(cpuLogType == 0) {
// 		out << " SS:"  << setw(4) << SegValue(ss) << " C"  << (get_CF()>0)  << " Z"   << (get_ZF()>0)
// 		    << " S" << (get_SF()>0) << " O"  << (get_OF()>0) << " I"  << GETFLAGBOOL(IF);
// 	} else {
// 		out << " FS:"  << setw(4) << SegValue(fs) << " GS:"  << setw(4) << SegValue(gs)
// 		    << " SS:"  << setw(4) << SegValue(ss)
// 		    << " CF:"  << (get_CF()>0)  << " ZF:"   << (get_ZF()>0)  << " SF:"  << (get_SF()>0)
// 		    << " OF:"  << (get_OF()>0)  << " AF:"   << (get_AF()>0)  << " PF:"  << (get_PF()>0)
// 		    << " IF:"  << GETFLAGBOOL(IF);
// 	}
// 	if(cpuLogType == 2) {
// 		out << " TF:" << GETFLAGBOOL(TF) << " VM:" << GETFLAGBOOL(VM) <<" FLG:" << setw(8) << reg_flags
// 		    << " CR0:" << setw(8) << cpu.cr0;
// 	}
// 	out << endl;
// }

// Ignore that these imports are incomplete or look weird. It is correct.
// #include <set> 

static std::set<std::string> uniqueAddresses;

// << EMS window 1 >> e000-e3ff
// << EMS window 2 >> e400-e7ff
// << EMS window 3 >> e800-ebff
// << EMS window 4 >> ec00-efff

static bool IsGraphics565A(uint16_t segment) {
	return segment == 0x565A;
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

// TODO: Refactor this is getting messy. I could have more small helper functions.
static void LogInstruction2(uint16_t segValue, uint32_t eipValue, ofstream& out) {
	// TODO: This is just a hack.
	if (!cpuLogFile.is_open()) {
		cpuLogFile.open("LOGCPU.TXT");
	}

	static std::stringstream buffer;
	static int bufferCount = 0;
	const int BUFFER_FLUSH_SIZE = 20;

	// Format the address
	std::stringstream addressStream;
	addressStream << std::hex << std::uppercase << std::setfill('0') 
				 << std::setw(4) << SegValue(cs) << ":"
				 << std::setw(4) << reg_eip;
	std::string address = addressStream.str();
	
	// Only add to buffer if this is a new address
	if (uniqueAddresses.insert(address).second) {
		// Get the physical address and disassemble
		PhysPt start = (PhysPt)GetAddress(segValue, eipValue);
		char dline[200];
		Bitu size = DasmI386(dline, start, reg_eip, cpu.code.big);

		// Get the instruction bytes
		std::stringstream bytesStream;
		for (Bitu i=0; i<size; i++) {
			uint8_t value;
			if (mem_readb_checked((PhysPt)(start+i), &value)) {
				bytesStream << "?? ";
			} else {
				bytesStream << std::hex << std::uppercase << std::setfill('0') 
						<< std::setw(2) << static_cast<int>(value) << " ";
			}
		}
		
		buffer << address << "    "  // 4 spaces after address
			   << bytesStream.str() 
			   << std::setfill(' ') << std::setw(30 - bytesStream.str().length()) << " "  // padding between opcodes and asm
			   << std::setw(35) << std::left << dline;  // left-aligned assembly with 30 char width

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

	std::string bufferAsString = buffer.str();
	
	if (bufferCount >= BUFFER_FLUSH_SIZE && bufferAsString.length() > 0) {
		out << bufferAsString;
		
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
- I probably should add some or all registers because then I could see for example which places in memory was fetched and updated.
- I have an idea of using named pipe.
  - dosbox-x just spits out every single instruction to a named pipe,
  - then js listens to it and it can either display it in terminal but also take commands at the same time.
    - js can have command to listen or to discard or maybe to do memory dumps too (but I dont quite know how that would work).
	  - regarding memory dumps they should get unique names, not like now in dosvox-x when LOGCPU.TXT gets overwritten.







*/