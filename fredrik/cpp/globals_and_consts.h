/////////////////////////////////////////////////////
// DEFINES                                         //
/////////////////////////////////////////////////////

// TODO: Write a good comment for this.
// #define SAVE_INSTRUCTIONS_HISTORY

// AUTO_DISASSEMBLER_MODE:
//     true:
//         Intended to be used to create automatic disassembly. It strips registers and will perhaps do some other stuff.
//     false:
//         Used for "live debugging".
#define AUTO_DISASSEMBLER_MODE = true;

//////////////////////
// CONSTS           //
//////////////////////



/////////////////////////////////////////////////////
// GLOBAL VARIABLES                                //
// I am under the impression that global variables //
// would be faster than passing variables.         //
/////////////////////////////////////////////////////

// Store previous segment and offset
// will be helpful when debugging.
uint16_t previous_segment = 0;
// uint16_t previous_offset = 0;
#ifdef SAVE_INSTRUCTIONS_HISTORY
std::string previous_instruction = "";
#endif
//
uint16_t previous_stub_segment = 0;

