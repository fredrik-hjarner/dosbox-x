// // If I am unsure I can validate against the new and the old at the same time
// // to see if they match.
// // Have in array sorted by most specific to least specific,
// // so that linear address of will/can match only one and that it is always the most specific.
// // Sorting by specificity is the same as sorting by size.
// namespace AddrRanges {
//     constexpr AddrRange VSYNC =                 AddrRange("WAIT_FOR_VSYNC", Addr(0x565A, 0x134A), Addr(0x565A, 0x134D));

//     constexpr AddrRange GRAPHICS_SEGMENT_565A = AddrRange("GFX_NOT_IN_EXE_1", Addr(0x565A, 0x0000), Addr(0x565A, 0xFFFF));
//     constexpr AddrRange GRAPHICS_SEGMENT_5677 = AddrRange("GFX_NOT_IN_EXE_2", Addr(0x5677, 0x0000), Addr(0x5677, 0xFFFF));
//     constexpr AddrRange GRAPHICS_SEGMENT_5FBE = AddrRange("GFX_NOT_IN_EXE_3", Addr(0x5FBE, 0x0000), Addr(0x5FBE, 0xFFFF));

//     constexpr AddrRange EMS_WINDOW_1 =          AddrRange("EMS_WINDOW_1", Addr(0xE0000, 0xE3FFF));
//     constexpr AddrRange EMS_WINDOW_2 =          AddrRange("EMS_WINDOW_2", Addr(0xE4000, 0xE7FFF));
//     constexpr AddrRange EMS_WINDOW_3 =          AddrRange("EMS_WINDOW_3", Addr(0xE8000, 0xEBFFF));
//     constexpr AddrRange EMS_WINDOW_4 =          AddrRange("EMS_WINDOW_4", Addr(0xEC000, 0xEFFFF));
//     // static assert that these together should be 64k
//     static_assert(
//         EMS_WINDOW_1.size + EMS_WINDOW_2.size + EMS_WINDOW_3.size + EMS_WINDOW_4.size == 0x10000,
//         "EMS windows should cover 64k"
//     );

//     constexpr AddrRange VIDEO_BIOS =            AddrRange("VIDEO_BIOS", Addr(0xC0000, 0xC7FFF));
//     constexpr AddrRange MOTHERBOARD_BIOS =      AddrRange("MOTHERBOARD_BIOS", Addr(0xF0000, 0xFFFFF));
// }