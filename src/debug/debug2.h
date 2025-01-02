static void LogInstruction2(uint16_t segValue, uint32_t eipValue, ofstream& out) {
	// TODO: This is just a hack.
	if (!cpuLogFile.is_open()) {
		cpuLogFile.open("LOGCPU.TXT");
	}

	static std::string buffer;
	static int bufferCount = 0;
	const int BUFFER_FLUSH_SIZE = 20;
	buffer += std::to_string(SegValue(cs)) + ":" + std::to_string(reg_eip) + "\n";
	bufferCount++;
	
	if (bufferCount >= BUFFER_FLUSH_SIZE) {
		out << buffer;
		buffer.clear();
		bufferCount = 0;
	}
	return;
}