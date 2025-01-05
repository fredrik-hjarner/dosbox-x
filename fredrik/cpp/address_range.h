// struct AddrRange {
//     uint32_t startLinear;
//     uint32_t endLinear;
//     uint32_t size;
//     std::string tag;

//     // Constructor from two addresses
//     constexpr AddrRange(std::string tag_, uint32_t startLinear_, uint32_t endLinear_) 
//         : tag(tag_), startLinear(startLinear_), endLinear(endLinear_), size(endLinear_ - startLinear_ + 1) {}

//     // Check if an address falls within this range
//     constexpr bool contains(uint32_t linearAddress) const {
//         return linearAddress >= startLinear && linearAddress <= endLinear;
//     }
// };
