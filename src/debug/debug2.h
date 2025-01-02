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

static void LogInstruction2(uint16_t segValue, uint32_t eipValue, ofstream& out) {
	// TODO: This is just a hack.
	if (!cpuLogFile.is_open()) {
		cpuLogFile.open("LOGCPU.TXT");
	}

	static std::stringstream buffer;
	static int bufferCount = 0;
	const int BUFFER_FLUSH_SIZE = 20;
	
	// Get the physical address and disassemble
	PhysPt start = (PhysPt)GetAddress(segValue, eipValue);
	char dline[200];
	Bitu size = DasmI386(dline, start, reg_eip, cpu.code.big);
	
	// Format the address
	std::stringstream addressStream;
	addressStream << std::hex << std::uppercase << std::setfill('0') 
				 << std::setw(4) << SegValue(cs) << ":"
				 << std::setw(4) << reg_eip;
	std::string address = addressStream.str();

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
	
	// Only add to buffer if this is a new address
	if (uniqueAddresses.insert(address).second) {
		buffer << address << "    "  // 4 spaces after address
			   << bytesStream.str() 
			   << std::setfill(' ') << std::setw(20 - bytesStream.str().length()) << " "  // padding between opcodes and asm
			   << std::setw(30) << std::left << dline  // left-aligned assembly with 30 char width
			   << "\n";
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