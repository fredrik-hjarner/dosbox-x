// This function is only supposed to be responsible for adding the instruction as text,
// it has no other responsibilities.
// I want it to assume cpuLogType == 2 and showExtend == true
// otherwise it's exactly like the original LogInstruction function.
// Well actually, another difference is that this returns a string instead of writing to an ofstream that it took in as parameter.
static std::string GetCpuInstructionLineString(uint16_t segValue, uint32_t eipValue, uint16_t overlay_segment) {
    std::stringstream out;
    out << std::hex << std::noshowbase << std::setfill('0') << std::uppercase;

	static char empty[23] = { 32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,0 };

	PhysPt start = (PhysPt)GetAddress(segValue,eipValue);
	char dline[200];Bitu size;
	size = DasmI386(dline, start, reg_eip, cpu.code.big);
	char* res = empty;

    // address     dline                res               ibytes
    // 24DF:0194   push word [bp+08]    ss:[FFBA]=0100    FF 76 08
    // res will be meaningless for AUTO_DISASSEMBLER_MODE since those are runtime values.
#ifndef AUTO_DISASSEMBLER_MODE
    res = AnalyzeInstruction(dline,false);
    if (!res || !(*res)) res = empty;
    Bitu reslen = strlen(res);
    if (reslen < 22) {
        memset(res + reslen, ' ', 22 - reslen);
        res[22] = 0;
    }
#endif

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
        for (Bitu i = 0; i < 21 - len; i++) {
            ibytes[len + i] = ' ';
        }
        ibytes[21] = 0;
    }

    if(overlay_segment != 0) {
        out
            << "      ` o" << setw(4) << overlay_segment << ":" << setw(4) << reg_ip
            << "` ";
    } else {
        out
            << setw(6) << ((SegValue(cs)<<4u)+reg_ip)
            << "`  " // observe extra space.
            << setw(4) << SegValue(cs) << ":" << setw(4) << reg_ip
            << "` ";
    }

    out
        << dline
        << "` ";

    // address     dline                res               ibytes
    // 24DF:0194   push word [bp+08]    ss:[FFBA]=0100    FF 76 08
    // res will be meaningless for AUTO_DISASSEMBLER_MODE since those are runtime values.
#ifndef AUTO_DISASSEMBLER_MODE
    out
        << res
        << "` ";
#endif

    out
        << ibytes
        << "`";

#ifndef AUTO_DISASSEMBLER_MODE
    // hard exit and explode with error if reg_esi or reg_edi are more than 0xFFFF
    // TODO: yea one of the WAS larger than 0xFFFF so I need to output it as 32-bit.
    // TODO: Add such a check for BP and SP too.
    // if (reg_esi > 0xFFFF || reg_edi > 0xFFFF) {
    //     std::cerr << "\n\n*** ERROR: reg_esi or reg_edi are more than 0xFFFF ***\n\n" << std::endl;
    //     exit(1);
    // }

    out
        << " A:" << setw(8) << reg_eax
        << " B:" << setw(8) << reg_ebx
        << " C:" << setw(8) << reg_ecx
        << " D:" << setw(8) << reg_edx
        << " SI:" << setw(8) << reg_esi
        << " DI:" << setw(8) << reg_edi
        // << " SI:" << setw(4) << reg_si
        // << " DI:" << setw(4) << reg_di
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
        // << " PF:"  << (get_PF()>0);
#endif

    out
        << " IF:"  << GETFLAGBOOL(IF) // might be good to know if in interrupt.
        << "` ";

    // out
        // << " TF:" << GETFLAGBOOL(TF) // Trap flag
        // << " VM:" << GETFLAGBOOL(VM) // Virtual 8086 mode flag (386+ only). Always zero.
        // << " FLG:" << setw(8) << reg_flags // I don't know what FLG is.
        // << " CR0:" << setw(8) << cpu.cr0; // I don't know what CR is.

	// out << endl; // scrapping the endline that is added afterwards

    return out.str();
}