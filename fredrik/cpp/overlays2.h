// The purpose of this file:
// When the other method does not work to find out which stub
// corresponds to an overlay, we need a more thorough analysis.
// We need to go through all stub headers and see
// which stub corresponds to the overlay of interest.

// Word at place 0x10 the stub header is stored where in the overlay buffer
// the overlay is placed/loaded.
// Looks like it's zero when it's not loaded.

// Purpose:
// This class manages the dynamic mapping between fixed stub segments (0x3000-0x3FFF)
// and their corresponding overlay buffer locations (0x4000-0x4FFF) in a DOS
// real-mode environment.
//
// How Overlays Work:
// - Stub segments (0x3xxx) contain fixed code that loads overlays
// - Overlay buffers (0x4xxx) are where the actual overlay code gets loaded
// - A single stub can have its overlay loaded at different 4xxx locations over time
// - Multiple overlay buffers can exist simultaneously
// - When an overlay moves, its old location becomes invalid

#include <list>

static std::list<uint32_t> stub_header_overlay_buffer_segments = {
    // segment times 16 plus the offset of 16 that I was talking about.
    0x303A * 0x10 + 0x10,
    0x303D * 0x10 + 0x10,
    0x3041 * 0x10 + 0x10, // has zero functions
    0x3043 * 0x10 + 0x10, // has zero functions
    0x3045 * 0x10 + 0x10, // has zero functions
    0x3047 * 0x10 + 0x10,
    0x304A * 0x10 + 0x10,
    0x304E * 0x10 + 0x10,
    0x3052 * 0x10 + 0x10,
    0x3059 * 0x10 + 0x10,
    0x305C * 0x10 + 0x10,
    0x3063 * 0x10 + 0x10,
    0x3069 * 0x10 + 0x10,
    0x306C * 0x10 + 0x10,
    0x306F * 0x10 + 0x10,
    0x3072 * 0x10 + 0x10,
    0x307C * 0x10 + 0x10,
    0x3082 * 0x10 + 0x10,
    0x3085 * 0x10 + 0x10,
    0x308B * 0x10 + 0x10,
    0x3096 * 0x10 + 0x10,
    0x309A * 0x10 + 0x10,
    0x30A2 * 0x10 + 0x10,
    0x30AA * 0x10 + 0x10,
    0x30AF * 0x10 + 0x10,
    0x30B2 * 0x10 + 0x10,
    0x30B6 * 0x10 + 0x10,
    0x30BA * 0x10 + 0x10,
    0x30BD * 0x10 + 0x10,
    0x30C1 * 0x10 + 0x10,
    0x30C5 * 0x10 + 0x10,
    0x30D0 * 0x10 + 0x10,
    0x30D7 * 0x10 + 0x10,
    0x30DD * 0x10 + 0x10,
    0x30E0 * 0x10 + 0x10,
    0x30E3 * 0x10 + 0x10,
    0x30E6 * 0x10 + 0x10,
    0x30ED * 0x10 + 0x10,
    0x30F3 * 0x10 + 0x10,
    0x30F9 * 0x10 + 0x10,
    0x30FC * 0x10 + 0x10,
    0x3101 * 0x10 + 0x10,
    0x3104 * 0x10 + 0x10,
    0x3109 * 0x10 + 0x10,
    0x310D * 0x10 + 0x10,
    0x3110 * 0x10 + 0x10,
    0x3114 * 0x10 + 0x10,
    0x312A * 0x10 + 0x10,
    0x3136 * 0x10 + 0x10,
    0x3146 * 0x10 + 0x10,
    0x314D * 0x10 + 0x10,
    0x3154 * 0x10 + 0x10,
    0x315B * 0x10 + 0x10,
    0x316A * 0x10 + 0x10,
    0x316D * 0x10 + 0x10,
    0x3173 * 0x10 + 0x10,
    0x3176 * 0x10 + 0x10,
    0x317F * 0x10 + 0x10,
    0x3184 * 0x10 + 0x10,
    0x318D * 0x10 + 0x10,
    0x3192 * 0x10 + 0x10,
    0x319E * 0x10 + 0x10,
    0x31A4 * 0x10 + 0x10,
    0x31A8 * 0x10 + 0x10,
    0x31B4 * 0x10 + 0x10,
    0x31BB * 0x10 + 0x10,
    0x31C3 * 0x10 + 0x10,
    0x31D4 * 0x10 + 0x10,
    0x31DE * 0x10 + 0x10, // has zero functions
    0x31E0 * 0x10 + 0x10,
    0x31EC * 0x10 + 0x10,
    0x31F1 * 0x10 + 0x10,
    0x31F4 * 0x10 + 0x10,
    0x31F7 * 0x10 + 0x10,
    0x31FA * 0x10 + 0x10,
    0x31FD * 0x10 + 0x10,
};

class Overlays2 {
public:
    // Delete constructors/assignment to prevent instantiation
    Overlays2() = delete;
    Overlays2(const Overlays2&) = delete;
    Overlays2& operator=(const Overlays2&) = delete;

    // Helper: Check if a segment is in the overlay range (0x4000-0x4FFF)
    static bool is_overlay_segment(uint16_t segment) {
        return (segment >= 0x4000 && segment <= 0x4FFF);
    }

    static uint16_t get_stub_for_overlay(uint16_t overlay_segment) {
        // use this function to read the word mem_readw_inline

        // so loop through all stub_header_overlay_buffer_segments
        // and return the word if it's not zero.
        // if not found then stderr and process exit.
        // rename segment to stub_header_overlay_buffer_segment
        for (
            auto it = stub_header_overlay_buffer_segments.begin(); 
            it != stub_header_overlay_buffer_segments.end();
            ++it
        ) {
            uint16_t word = mem_readw_inline(*it);
            // log to std err
            // std::cerr
            //     << "Checking stub header at "
            //     << std::hex << *it << " for overlay " << std::hex
            //     << overlay_segment
            //     << " got word " << std::hex << word << std::endl;
            
            if (word == overlay_segment) {
                // std::cerr << "Found stub header at " << std::hex << *it;
                
                // Move this segment to the front of the list since it was just used
                if (it != stub_header_overlay_buffer_segments.begin()) {
                    stub_header_overlay_buffer_segments.splice(
                        stub_header_overlay_buffer_segments.begin(), 
                        stub_header_overlay_buffer_segments,
                        it
                    );
                }
                
                // take the address to the segment IN stub header and
                // convert to the segment of the stub header
                // return (segment - 0x10) / 0x10; // TODO: Maybe I can do some bitshifting here.
                return (*it - 0x10) >> 0x4;
            }
        }
        std::cerr << "No stub found for overlay " << std::hex << overlay_segment << std::endl;
        std::exit(1);
    }
};
