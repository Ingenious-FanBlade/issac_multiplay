#pragma once

#include <Windows.h>

#include <array>
#include <cstdint>
#include <cstring>

namespace isaac_cs_t0::target_image {

inline constexpr std::uint32_t kExpectedTimestamp = 0x69E6E3A7u;
inline constexpr std::uint32_t kPreferredImageBase = 0x00400000u;
inline constexpr std::uint32_t kCanonicalImageSize = 0x0092C000u;
inline constexpr std::uint32_t kUnpackedImageSize = 0x008FE000u;
inline constexpr std::uint32_t kCanonicalEntryPointRva = 0x008FE310u;
inline constexpr std::uint32_t kUnpackedEntryPointRva = 0x006EFC46u;

enum class Profile : std::uint32_t {
    Canonical = 1,
    ApprovedUnpacked = 2,
    // User-directed T0-09-B diagnostic profile for the same canonical PE
    // metadata loaded at a relocated module base. This is not a new T0-01
    // baseline approval; exact hook targets remain verified before patching.
    RelocatedCanonicalExperimental = 3,
    CanonicalRuntimeRebasedExperimental = 4,
};

struct SectionExpectation final {
    std::array<char, 8> name;
    std::uint32_t virtual_size;
    std::uint32_t rva;
    std::uint32_t raw_size;
    std::uint32_t raw_offset;
    std::uint32_t characteristics;
};

inline constexpr std::array<SectionExpectation, 6> kSections{{
    {{{'.', 't', 'e', 'x', 't', 0, 0, 0}}, 0x00716134u, 0x00001000u, 0x00716200u,
      0x00000400u, 0x60000020u},
    {{{'.', 'r', 'd', 'a', 't', 'a', 0, 0}}, 0x000DF948u, 0x00718000u, 0x000DFA00u,
      0x00716600u, 0x40000040u},
    {{{'.', 'd', 'a', 't', 'a', 0, 0, 0}}, 0x000A4AA4u, 0x007F8000u, 0x00069E00u,
      0x007F6000u, 0xC0000040u},
    {{{'.', 'r', 's', 'r', 'c', 0, 0, 0}}, 0x00003B20u, 0x0089D000u, 0x00003C00u,
      0x0085FE00u, 0x40000040u},
    {{{'.', 'r', 'e', 'l', 'o', 'c', 0, 0}}, 0x0005C93Cu, 0x008A1000u, 0x0005CA00u,
      0x00863A00u, 0x42000040u},
    {{{'.', 'b', 'i', 'n', 'd', 0, 0, 0}}, 0x0002D808u, 0x008FE000u, 0x0002D808u,
      0x008C0400u, 0x60000000u},
}};

inline bool SectionMatches(const IMAGE_SECTION_HEADER& actual,
                           const SectionExpectation& expected) noexcept
{
    return std::memcmp(actual.Name, expected.name.data(), expected.name.size()) == 0 &&
           actual.Misc.VirtualSize == expected.virtual_size &&
           actual.VirtualAddress == expected.rva && actual.SizeOfRawData == expected.raw_size &&
           actual.PointerToRawData == expected.raw_offset &&
           actual.Characteristics == expected.characteristics;
}

inline bool LoadedImageBaseMatches(const std::uint8_t* image_base,
                                   std::uint32_t header_image_base,
                                   Profile profile) noexcept
{
    const auto loaded = reinterpret_cast<std::uintptr_t>(image_base);
    if (loaded > UINT32_MAX)
        return false;
    // The canonical loader preserves the preferred base. The one approved unpacked loader
    // rewrites it to the actual relocated module base. The selected identity fixes the rule.
    if (profile == Profile::Canonical)
        return header_image_base == kPreferredImageBase &&
               loaded == kPreferredImageBase;
    if (profile == Profile::ApprovedUnpacked ||
        profile == Profile::CanonicalRuntimeRebasedExperimental)
        return header_image_base == static_cast<std::uint32_t>(loaded);
    return header_image_base == kPreferredImageBase &&
           loaded != kPreferredImageBase && (loaded & 0xFFFFu) == 0;
}

inline bool IsApproved(const std::uint8_t* image_base, Profile profile) noexcept
{
    if (!image_base)
        return false;
    __try
    {
        const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(image_base);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0)
            return false;
        const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(image_base + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE ||
            nt->FileHeader.Machine != IMAGE_FILE_MACHINE_I386 ||
            nt->FileHeader.TimeDateStamp != kExpectedTimestamp ||
            nt->FileHeader.Characteristics != 0x0122u ||
            nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC ||
            !LoadedImageBaseMatches(image_base, nt->OptionalHeader.ImageBase, profile) ||
            nt->OptionalHeader.SectionAlignment != 0x1000u ||
            nt->OptionalHeader.FileAlignment != 0x0200u ||
            nt->OptionalHeader.SizeOfHeaders != 0x0400u ||
            nt->OptionalHeader.DllCharacteristics != 0x8140u)
            return false;

        const bool canonical_layout =
            (profile == Profile::Canonical ||
             profile == Profile::RelocatedCanonicalExperimental ||
             profile == Profile::CanonicalRuntimeRebasedExperimental) &&
            nt->FileHeader.NumberOfSections == 6 &&
            nt->OptionalHeader.SizeOfImage == kCanonicalImageSize &&
            nt->OptionalHeader.AddressOfEntryPoint == kCanonicalEntryPointRva;
        const bool unpacked = profile == Profile::ApprovedUnpacked &&
                              nt->FileHeader.NumberOfSections == 5 &&
                              nt->OptionalHeader.SizeOfImage == kUnpackedImageSize &&
                              nt->OptionalHeader.AddressOfEntryPoint == kUnpackedEntryPointRva;
        if (!(canonical_layout || unpacked))
            return false;
        const auto* sections = IMAGE_FIRST_SECTION(nt);
        const std::size_t count = canonical_layout ? kSections.size() : kSections.size() - 1;
        for (std::size_t index = 0; index < count; ++index)
        {
            if (!SectionMatches(sections[index], kSections[index]))
                return false;
        }
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

inline bool IsApproved(const std::uint8_t* image_base) noexcept
{
    return IsApproved(image_base, Profile::Canonical) ||
           IsApproved(image_base, Profile::ApprovedUnpacked);
}

// Cache this once during initialization, before publishing any Hook. A zero
// result leaves RVA conversion closed for an unrecognized image.
inline std::uint32_t ApprovedImageSize(const std::uint8_t* image_base) noexcept
{
    if (IsApproved(image_base, Profile::Canonical))
        return kCanonicalImageSize;
    if (IsApproved(image_base, Profile::ApprovedUnpacked))
        return kUnpackedImageSize;
    return 0;
}

// T0-09-B user-directed experimental path for the same approved canonical PE
// metadata when Windows has relocated the module. It does not bypass the
// subsequent exact hook/site verification.
inline bool IsApprovedT09(const std::uint8_t* image_base) noexcept
{
    return IsApproved(image_base) ||
           IsApproved(image_base, Profile::RelocatedCanonicalExperimental) ||
           IsApproved(image_base, Profile::CanonicalRuntimeRebasedExperimental);
}

} // namespace isaac_cs_t0::target_image
