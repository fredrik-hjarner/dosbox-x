//////////////////////
// CONSTS           //
//////////////////////

// autoDisassemblerMode:
//     true:
//         Intended to be used to create automatic disassembly. It strips registers and will perhaps do some other stuff.
//     false:
//         Used for "live debugging".
constexpr bool autoDisassemblerMode = true;

/////////////////////////////////////////////////////
// GLOBAL VARIABLES                                //
// I am under the impression that global variables //
// would be faster than passing variables.         //
/////////////////////////////////////////////////////

// Store previous segment and offset
// will be helpful when debugging.
uint16_t previous_segment = 0;
uint16_t previous_offset = 0;
std::string previous_instruction = "";
//
uint16_t previous_stub_segment = 0;

