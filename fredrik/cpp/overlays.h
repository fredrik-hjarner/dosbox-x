// OverlayMapper.hpp
//
// A fast, memory-efficient mapper for real-mode DOS overlay segments
// 
// Purpose:
// This class manages the dynamic mapping between fixed stub segments (0x3000-0x3FFF)
// and their corresponding overlay buffer locations (0x4000-0x4FFF) in a DOS
// real-mode environment. It's designed to be extremely fast and suitable for
// time-critical code paths in DOS programs.
//
// How Overlays Work:
// - Stub segments (0x3xxx) contain fixed code that loads overlays
// - Overlay buffers (0x4xxx) are where the actual overlay code gets loaded
// - A single stub can have its overlay loaded at different 4xxx locations over time
// - Multiple overlay buffers can exist simultaneously
// - When an overlay moves, its old location becomes invalid
//
// Performance Characteristics:
// - All operations are O(1) using dual hash maps
// - No iteration through mappings is ever needed
// - Memory usage is proportional to number of active overlays
// - Suitable for environments with 70+ stubs and 200+ possible overlay locations

#include <unordered_map>
#include <iostream>
#include <cstdlib>

class Overlays {
private:
    // Primary mapping: overlay location -> stub segment
    // Used to quickly determine which stub owns a given overlay buffer
    inline static std::unordered_map<uint16_t, uint16_t> overlay_to_stub;     // 4xxx -> 3xxx

    // Reverse mapping: stub segment -> current overlay location
    // Enables instant cleanup of old mappings when a stub moves
    inline static std::unordered_map<uint16_t, uint16_t> stub_to_overlay;     // 3xxx -> 4xxx

    // Tracks the most recently entered stub segment
    // Reset to 0 after each mapping operation
    inline static uint16_t current_stub = 0;

public:
    // Delete constructors/assignment to prevent instantiation
    Overlays() = delete;
    Overlays(const Overlays&) = delete;
    Overlays& operator=(const Overlays&) = delete;

    // Record entry into a stub segment
    // This must be called before map_overlay() to establish the mapping source
    //
    // Parameters:
    //   stub_segment: Segment address in 3xxx range
    //
    // Example:
    //   Overlays::enter_stub(0x3100);  // We're entering the stub at 0x3100
    // TODO: I only have offset here to do logging. remove it.
    static void enter_stub(
        uint16_t stub_segment
        // uint16_t offset
    ) {
        // log to stderr that this was entered
        // std::cerr
        //     << "Entered stub " << std::hex << std::setfill('0') << std::setw(4) << stub_segment
        //     << ":" << std::hex << std::setfill('0') << std::setw(4) << offset << std::endl;
        current_stub = stub_segment;
    }
    
    // Map the current stub segment to an overlay buffer location
    // Must be called after enter_stub() to create the actual mapping
    //
    // This function maintains these invariants:
    // 1. Each overlay location maps to exactly one stub
    // 2. Each stub maps to exactly one overlay location
    // 3. Old mappings for a stub are automatically removed
    //
    // Parameters:
    //   overlay_segment: Segment address in 4xxx range where the overlay is loaded
    //
    // Error handling:
    //   Exits if no stub segment was set via enter_stub()
    //
    // Example sequence:
    //   Overlays::enter_stub(0x3100);    // Enter stub
    //   Overlays::map_overlay(0x4500);   // Overlay for 0x3100 is now at 0x4500
    //   Overlays::enter_stub(0x3100);    // Same stub again
    //   Overlays::map_overlay(0x4600);   // Overlay moved to 0x4600, 0x4500 mapping removed
    static void map_overlay(uint16_t overlay_segment) {
        if (current_stub == 0) {
            // TODO: It would be great with error here.
            // std::cerr << "Error: No stub segment set" << std::endl;
            // std::exit(1);
            return; // TODO: Temporary to get it to work fast.
        }

        return _map_overlay(overlay_segment, current_stub);
    }

    static void _map_overlay(uint16_t overlay_segment, uint16_t stub_segment) {
        // If this stub was mapped somewhere else, remove that mapping
        // This ensures a stub only exists at one overlay location at a time
        auto old_location = stub_to_overlay.find(stub_segment);
        if (old_location != stub_to_overlay.end()) {
            // log to stderr simply that this was removed
            // std::cerr << "Removed old mapping for stub " << std::hex << std::setfill('0') << std::setw(4) << current_stub << " from overlay " << std::hex << std::setfill('0') << std::setw(4) << old_location->second << std::endl;
            overlay_to_stub.erase(old_location->second);
        }

        // Create new mappings in both directions
        // log to stderr that this was created
        // std::cerr << "Created new mapping for stub " << std::hex << std::setfill('0') << std::setw(4) << current_stub << " at overlay " << std::hex << std::setfill('0') << std::setw(4) << overlay_segment << std::endl;
        overlay_to_stub[overlay_segment] = stub_segment;
        stub_to_overlay[stub_segment] = overlay_segment;
        stub_segment = 0;  // Reset to prevent accidental reuse
    }
    
    // Look up which stub segment corresponds to an overlay buffer location
    // This is the primary query operation during execution
    //
    // Parameters:
    //   overlay_segment: Segment address in 4xxx range to look up
    //
    // Returns:
    //   The 3xxx stub segment that owns this overlay location
    //
    // Error handling:
    //   Exits if the overlay segment isn't currently mapped
    //
    // Example:
    //   uint16_t stub = Overlays::get_stub(0x4500);  // What stub owns this overlay?
    // TODO: I only have offset here to do logging. remove it.
    static uint16_t get_stub(
        uint16_t overlay_segment
        // uint16_t offset
    ) {
        auto it = overlay_to_stub.find(overlay_segment);
        if (it == overlay_to_stub.end()) {
            // return 0; // TODO: Temporary to get it to work fast.
            // TODO: It would be great with error here.
            std::cerr
                << "Error: No stub found for overlay " << std::hex << std::setfill('0') << std::setw(4) << overlay_segment
                // << ":" << std::hex << std::setfill('0') << std::setw(4) << offset
                << std::endl;
            std::cerr
                << "Previous address: "
                << std::hex <<std::setfill('0') << std::setw(4) << previous_segment
                // << ":" << std::hex << std::setfill('0') << std::setw(4)
                // << previous_offset
                << std::endl;

#ifdef SAVE_INSTRUCTIONS_HISTORY
            std::cerr
                << "Previous instruction: \n"
                << previous_instruction << std::endl;
#endif

            // TODO: clean this up man.
            auto stub_segment = Overlays2::get_stub_for_overlay(overlay_segment);
            _map_overlay(overlay_segment, stub_segment);
            return stub_segment;
        }
        return it->second;
    }

    // Helper: Check if a segment is in the stub range (0x3000-0x3FFF)
    static bool is_stub_segment(uint16_t segment) {
        return (segment >= 0x3000 && segment <= 0x3FFF);
    }

    // Helper: Check if a segment is in the overlay range (0x4000-0x4FFF)
    static bool is_overlay_segment(uint16_t segment) {
        return (segment >= 0x4000 && segment <= 0x4FFF);
    }
};

/*
Usage Example:

    // Scenario 1: Simple mapping
    Overlays::enter_stub(0x3100);    // Enter first stub
    Overlays::map_overlay(0x4500);   // Its overlay is at 0x4500

    // Scenario 2: Multiple overlays
    Overlays::enter_stub(0x3200);    // Enter second stub
    Overlays::map_overlay(0x4600);   // Its overlay is at 0x4600

    // Scenario 3: Overlay movement
    Overlays::enter_stub(0x3100);    // Back to first stub
    Overlays::map_overlay(0x4700);   // Move its overlay to 0x4700
                                        // The 0x4500 mapping is automatically removed

    // Scenario 4: Looking up mappings
    uint16_t stub = Overlays::get_stub(0x4700);  // Returns 0x3100
    stub = Overlays::get_stub(0x4500);           // Error: No longer mapped

    // Scenario 5: Overlay reuse
    Overlays::enter_stub(0x3300);    // New stub
    Overlays::map_overlay(0x4600);   // Takes over where 0x3200's overlay was
    stub = Overlays::get_stub(0x4600);  // Returns 0x3300
*/