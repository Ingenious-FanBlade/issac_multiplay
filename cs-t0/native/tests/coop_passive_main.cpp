#include <Windows.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cwchar>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <string_view>
#include <utility>

#include "isaac_cs_t0/coop_passive_ipc.hpp"
#include "isaac_cs_t0/coop_passive_runtime.hpp"
#include "isaac_cs_t0/hooks.hpp"
#include "isaac_cs_t0/sha256.hpp"

namespace {

int g_failures = 0;

#define CHECK(expression)                                                        \
    do {                                                                         \
        if (!(expression)) {                                                     \
            std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__,       \
                         #expression);                                            \
            ++g_failures;                                                        \
            return false;                                                        \
        }                                                                        \
    } while (false)

void Suite(const char* name, std::uint32_t cases, bool passed)
{
    std::printf("%s %s (%u cases)\n", passed ? "PASS" : "FAIL", name, cases);
}

std::uint8_t HexNibble(char value)
{
    if (value >= '0' && value <= '9')
        return static_cast<std::uint8_t>(value - '0');
    if (value >= 'A' && value <= 'F')
        return static_cast<std::uint8_t>(value - 'A' + 10);
    return 0xFFu;
}

std::array<std::uint8_t, 32> DigestFromHex(std::string_view value)
{
    std::array<std::uint8_t, 32> result{};
    if (value.size() != result.size() * 2)
        return result;
    for (std::size_t index = 0; index < result.size(); ++index)
    {
        const auto high = HexNibble(value[index * 2]);
        const auto low = HexNibble(value[index * 2 + 1]);
        if (high > 0xFu || low > 0xFu)
            return {};
        result[index] = static_cast<std::uint8_t>((high << 4) | low);
    }
    return result;
}

class OrderedPatchFixture {
public:
    static constexpr std::size_t kRelative = 41;
    static constexpr std::size_t kPointers = 12;
    static constexpr std::size_t kCombined = 53;

    OrderedPatchFixture()
    {
        memory = static_cast<std::uint8_t*>(
            VirtualAlloc(nullptr, 0x4000, MEM_COMMIT | MEM_RESERVE,
                         PAGE_EXECUTE_READWRITE));
        Reset();
    }

    ~OrderedPatchFixture()
    {
        if (memory)
            VirtualFree(memory, 0, MEM_RELEASE);
    }

    OrderedPatchFixture(const OrderedPatchFixture&) = delete;
    OrderedPatchFixture& operator=(const OrderedPatchFixture&) = delete;

    void Reset()
    {
        if (!memory)
            return;
        std::memset(memory, 0x90, 0x4000);
        for (std::size_t index = 0; index < relative.size(); ++index)
        {
            auto* call = memory + index * 8;
            auto* original = memory + 0x1000 + index;
            auto* replacement = memory + 0x1400 + index;
            call[0] = 0xE8;
            const auto displacement =
                static_cast<std::int32_t>(original - (call + 5));
            std::memcpy(call + 1, &displacement, sizeof(displacement));
            relative[index] = {};
            relative[index].call = call;
            std::memcpy(relative[index].expected, call, 5);
            relative[index].replacement = replacement;
        }
        auto** slots = reinterpret_cast<void**>(memory + 0x2000);
        for (std::size_t index = 0; index < pointers.size(); ++index)
        {
            slots[index] = memory + 0x2400 + index;
            pointers[index] = {&slots[index], memory + 0x2800 + index,
                               nullptr, nullptr, false, slots[index]};
        }

        std::size_t relative_index = 0;
        std::size_t pointer_index = 0;
        for (std::size_t ordinal = 0; ordinal < descriptors.size(); ++ordinal)
        {
            auto& descriptor = descriptors[ordinal];
            descriptor = {};
            descriptor.ordinal = static_cast<std::uint32_t>(ordinal);
            // The frozen composite catalog has image/import pointers at 27..38;
            // all remaining inherited and co-op slots are REL32 calls.
            if (ordinal >= 27 && ordinal <= 38)
                descriptor.pointer = &pointers[pointer_index++];
            else
                descriptor.relative = &relative[relative_index++];
        }
    }

    bool Restored() const
    {
        for (const auto& patch : relative)
            if (patch.installed ||
                std::memcmp(patch.call, patch.expected, sizeof(patch.expected)) != 0)
                return false;
        for (const auto& patch : pointers)
            if (patch.installed || *patch.slot != patch.expected_original)
                return false;
        return true;
    }

    bool InstalledPrefix(std::size_t count) const
    {
        for (std::size_t ordinal = 0; ordinal < descriptors.size(); ++ordinal)
        {
            const auto& descriptor = descriptors[ordinal];
            const bool installed = descriptor.relative ? descriptor.relative->installed
                                                       : descriptor.pointer->installed;
            if (installed != (ordinal < count))
                return false;
        }
        return true;
    }

    std::uint8_t* memory = nullptr;
    std::array<isaac_cs_t0::mode::RelativeCallPatch, kRelative> relative{};
    std::array<isaac_cs_t0::mode::PointerPatch, kPointers> pointers{};
    std::array<isaac_cs_t0::mode::OrderedPatchDescriptor, kCombined> descriptors{};
};

class TopologyFixture {
public:
    static constexpr std::size_t kMainSize = 0x00900000;
    static constexpr std::size_t kRootSize = 0x00030000;
    static constexpr std::size_t kOnlineManagerSize = 0x00050000;
    static constexpr std::size_t kPlayerSize = 0x00002000;
    static constexpr std::size_t kDeviceSize = 0x00000100;
    static constexpr std::size_t kVectorStorageSize = 0x00001000;

    static constexpr std::size_t kInputManagerRva = 0x00857B18;
    static constexpr std::size_t kGameRootSlotRva = 0x00871678;
    static constexpr std::size_t kOnlineManagerSlotRva = 0x0087169C;
    static constexpr std::size_t kNetworkBackendSlotRva = 0x00879A58;
    static constexpr std::size_t kPlayerManagerOffset = 0x0001BAA8;
    static constexpr std::size_t kRunActiveOffset = 0x0002658A;
    static constexpr std::size_t kHudOffset = 0x0001DA04;
    static constexpr std::size_t kHudStride = 0x000006DC;
    static constexpr std::size_t kOnlineRosterOffset = 0x0004B3D8;
    static constexpr std::size_t kPlayerVtableRva = 0x0076BDD0;
    static constexpr std::size_t kPlayerColRva = 0x007AE31C;
    static constexpr std::size_t kPlayerAssignmentOffset = 0x00001618;
    static constexpr std::size_t kPlayerOrdinalOffset = 0x0000161C;
    static constexpr std::size_t kPlayerTwinOffset = 0x00001E68;
    static constexpr std::size_t kDeviceAssignmentOffset = 0x0000000C;
    static constexpr std::size_t kKeyboardVtableRva = 0x007A1678;
    static constexpr std::size_t kGamepadVtableRva = 0x00782C80;

    TopologyFixture()
    {
        main_image = Allocate(kMainSize);
        root = Allocate(kRootSize);
        online_manager = Allocate(kOnlineManagerSize);
        player = Allocate(kPlayerSize);
        keyboard = Allocate(kDeviceSize);
        gamepad = Allocate(kDeviceSize);
        vector_storage = Allocate(kVectorStorageSize);
        Reset();
    }

    ~TopologyFixture()
    {
        for (std::size_t index = allocation_count; index > 0; --index)
            VirtualFree(allocations[index - 1], 0, MEM_RELEASE);
    }

    TopologyFixture(const TopologyFixture&) = delete;
    TopologyFixture& operator=(const TopologyFixture&) = delete;

    bool Ready() const noexcept
    {
        return main_image && root && online_manager && player && keyboard &&
               gamepad && vector_storage;
    }

    void Reset()
    {
        if (!Ready())
            return;
        std::memset(main_image, 0, kMainSize);
        std::memset(root, 0, kRootSize);
        std::memset(online_manager, 0, kOnlineManagerSize);
        std::memset(player, 0, kPlayerSize);
        std::memset(keyboard, 0, kDeviceSize);
        std::memset(gamepad, 0, kDeviceSize);
        std::memset(vector_storage, 0, kVectorStorageSize);

        *reinterpret_cast<void**>(main_image + kGameRootSlotRva) = root;
        *reinterpret_cast<void**>(main_image + kOnlineManagerSlotRva) =
            online_manager;
        *reinterpret_cast<void**>(main_image + kNetworkBackendSlotRva) = nullptr;
        *reinterpret_cast<std::uint16_t*>(root + kRunActiveOffset) = 1;

        auto** const device_entries = reinterpret_cast<void**>(vector_storage);
        device_entries[0] = keyboard;
        device_entries[1] = root + 0x10;
        device_entries[2] = gamepad;
        device_entries[3] = root + 0x20;
        SetVector(main_image + kInputManagerRva + 8, vector_storage, 2,
                  sizeof(void*) * 2);

        *reinterpret_cast<void**>(keyboard) = main_image + kKeyboardVtableRva;
        *reinterpret_cast<void**>(gamepad) = main_image + kGamepadVtableRva;
        // C0 classifies active devices by the exact frozen vtable. A device COL
        // is not part of the accepted live source schema and is deliberately
        // absent from this otherwise-valid fixture.
        *reinterpret_cast<std::int32_t*>(keyboard + kDeviceAssignmentOffset) = 7;
        *reinterpret_cast<std::int32_t*>(gamepad + kDeviceAssignmentOffset) = 8;

        auto** const player_entries =
            reinterpret_cast<void**>(vector_storage + 0x100);
        player_entries[0] = player;
        SetVector(root + kPlayerManagerOffset, vector_storage + 0x100, 1,
                  sizeof(void*));
        *reinterpret_cast<void**>(player) = main_image + kPlayerVtableRva;
        reinterpret_cast<void**>(main_image + kPlayerVtableRva)[-1] =
            main_image + kPlayerColRva;
        *reinterpret_cast<std::int32_t*>(player + kPlayerAssignmentOffset) = 7;
        *reinterpret_cast<std::uint32_t*>(player + kPlayerOrdinalOffset) = 0;
        *reinterpret_cast<void**>(player + kPlayerTwinOffset) = nullptr;

        *reinterpret_cast<void**>(root + kHudOffset) = player;
        SetVector(online_manager + kOnlineRosterOffset, nullptr, 0,
                  sizeof(void*));
    }

    std::uint8_t* MainImage() const noexcept { return main_image; }
    void* ExpectedManager() const noexcept
    {
        return root + kPlayerManagerOffset;
    }

    void InvalidateDeviceVector() noexcept
    {
        auto** const header =
            reinterpret_cast<std::uint8_t**>(main_image + kInputManagerRva + 8);
        header[1] = header[0] + 1;
    }

    void SetRunActive(bool active) noexcept
    {
        *reinterpret_cast<std::uint16_t*>(root + kRunActiveOffset) =
            active ? 1 : 0;
    }

    void ClearDeviceReference(std::size_t index) noexcept
    {
        reinterpret_cast<void**>(vector_storage)[index * 2 + 1] = nullptr;
    }

    void SetUnknownDeviceVtable(std::size_t index) noexcept
    {
        auto* const object = index == 0 ? keyboard : gamepad;
        *reinterpret_cast<void**>(object) = main_image + 0x1000;
    }

    void SetPlayerAssignment(std::int32_t assignment) noexcept
    {
        *reinterpret_cast<std::int32_t*>(player + kPlayerAssignmentOffset) =
            assignment;
    }

    void SetDeviceAssignment(std::size_t index,
                             std::int32_t assignment) noexcept
    {
        auto* const object = index == 0 ? keyboard : gamepad;
        *reinterpret_cast<std::int32_t*>(object + kDeviceAssignmentOffset) =
            assignment;
    }

    void SetPlayerOrdinal(std::uint32_t ordinal) noexcept
    {
        *reinterpret_cast<std::uint32_t*>(player + kPlayerOrdinalOffset) = ordinal;
    }

    void ClearPlayerEntry() noexcept
    {
        *reinterpret_cast<void**>(vector_storage + 0x100) = nullptr;
    }

    void SetUnknownPlayerVtable() noexcept
    {
        *reinterpret_cast<void**>(player) = main_image + 0x1000;
    }

    void ClearPlayerCol() noexcept
    {
        reinterpret_cast<void**>(main_image + kPlayerVtableRva)[-1] = nullptr;
    }

    void SetTwinOrPossessor() noexcept
    {
        *reinterpret_cast<void**>(player + kPlayerTwinOffset) = root;
    }

    void SetHud(std::size_t index, void* value) noexcept
    {
        *reinterpret_cast<void**>(root + kHudOffset + index * kHudStride) = value;
    }

    void SetOnlineBackendPresent() noexcept
    {
        *reinterpret_cast<void**>(main_image + kNetworkBackendSlotRva) = root;
    }

    void SetOnlineRosterPresent() noexcept
    {
        auto* const roster = vector_storage + 0x200;
        *reinterpret_cast<void**>(roster) = player;
        SetVector(online_manager + kOnlineRosterOffset, roster, 1,
                  sizeof(void*));
    }

    void SetInvalidOnlineManager() noexcept
    {
        *reinterpret_cast<void**>(main_image + kOnlineManagerSlotRva) =
            reinterpret_cast<void*>(1);
    }

    void InvalidateOnlineRoster() noexcept
    {
        auto** const header = reinterpret_cast<std::uint8_t**>(
            online_manager + kOnlineRosterOffset);
        header[0] = vector_storage + 1;
        header[1] = vector_storage + 1;
        header[2] = vector_storage + 1;
    }

    void SetOnlineFrame(std::uint32_t frame) noexcept
    {
        *reinterpret_cast<std::uint32_t*>(online_manager + 0x0004B3E4) = frame;
    }

    void SetThreePlayers() noexcept
    {
        SetVector(root + kPlayerManagerOffset, vector_storage + 0x100, 3,
                  sizeof(void*));
    }

private:
    std::uint8_t* Allocate(std::size_t size)
    {
        auto* const value = static_cast<std::uint8_t*>(
            VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
        if (value && allocation_count < allocations.size())
            allocations[allocation_count++] = value;
        return value;
    }

    static void SetVector(std::uint8_t* header, std::uint8_t* begin,
                          std::size_t count, std::size_t stride) noexcept
    {
        auto** const slots = reinterpret_cast<std::uint8_t**>(header);
        slots[0] = begin;
        slots[1] = begin ? begin + count * stride : nullptr;
        slots[2] = slots[1];
    }

    std::array<void*, 8> allocations{};
    std::size_t allocation_count = 0;
    std::uint8_t* main_image = nullptr;
    std::uint8_t* root = nullptr;
    std::uint8_t* online_manager = nullptr;
    std::uint8_t* player = nullptr;
    std::uint8_t* keyboard = nullptr;
    std::uint8_t* gamepad = nullptr;
    std::uint8_t* vector_storage = nullptr;
};

bool ExpectTopology(
    TopologyFixture& fixture, void* expected_manager,
    std::uint32_t expected_classification,
    isaac_cs_t0::coop_passive::detail::TopologyDiagnostic expected_diagnostic,
    std::size_t expected_players, std::size_t expected_devices)
{
    isaac_cs_t0::coop_passive::detail::TopologyProbeResult result{};
    CHECK(isaac_cs_t0::coop_passive::detail::RunTopologyProbe(
        fixture.MainImage(), expected_manager, result));
    CHECK(result.classification == expected_classification);
    CHECK(result.diagnostic == expected_diagnostic);
    CHECK(result.player_count == expected_players);
    CHECK(result.device_count == expected_devices);
    return true;
}

bool TestFrozenSinglePlayerTopology()
{
    using isaac_cs_t0::coop_passive::detail::TopologyDiagnostic;
    TopologyFixture fixture;
    CHECK(fixture.Ready());

    // This is the exact shape frozen by the C0 OFFLINE_SINGLE source: one
    // keyboard-bound Player, a second unassigned gamepad diagnostic, one
    // primary HUD entry, empty twin HUD entries, and an empty online roster.
    CHECK(ExpectTopology(fixture, fixture.ExpectedManager(), 0,
                         TopologyDiagnostic::None, 1, 2));

    fixture.Reset();
    fixture.SetRunActive(false);
    CHECK(ExpectTopology(fixture, fixture.ExpectedManager(), 1,
                         TopologyDiagnostic::RunNotActive, 0, 0));

    fixture.Reset();
    CHECK(ExpectTopology(fixture, fixture.MainImage(), 3,
                         TopologyDiagnostic::ManagerOwnerMismatch, 0, 0));

    fixture.Reset();
    fixture.SetOnlineBackendPresent();
    CHECK(ExpectTopology(fixture, fixture.ExpectedManager(), 3,
                         TopologyDiagnostic::OnlineBackendPresent, 0, 0));

    fixture.Reset();
    fixture.SetOnlineRosterPresent();
    CHECK(ExpectTopology(fixture, fixture.ExpectedManager(), 3,
                         TopologyDiagnostic::OnlineRosterPresent, 0, 0));

    fixture.Reset();
    fixture.SetInvalidOnlineManager();
    CHECK(ExpectTopology(fixture, fixture.ExpectedManager(), 3,
                         TopologyDiagnostic::InvalidOnlineManager, 0, 0));

    fixture.Reset();
    fixture.InvalidateOnlineRoster();
    CHECK(ExpectTopology(fixture, fixture.ExpectedManager(), 1,
                         TopologyDiagnostic::OnlineRosterHeaderInvalid, 0, 0));

    fixture.Reset();
    fixture.SetOnlineFrame(1);
    CHECK(ExpectTopology(fixture, fixture.ExpectedManager(), 3,
                         TopologyDiagnostic::OfflineOnlineFrameNonzero, 0, 0));

    fixture.Reset();
    fixture.InvalidateDeviceVector();
    CHECK(ExpectTopology(fixture, fixture.ExpectedManager(), 1,
                         TopologyDiagnostic::DeviceVectorInvalid, 0, 0));

    fixture.Reset();
    fixture.ClearDeviceReference(1);
    CHECK(ExpectTopology(fixture, fixture.ExpectedManager(), 1,
                         TopologyDiagnostic::InvalidDeviceEntry, 0, 2));

    fixture.Reset();
    fixture.SetUnknownDeviceVtable(1);
    CHECK(ExpectTopology(fixture, fixture.ExpectedManager(), 3,
                         TopologyDiagnostic::UnknownDeviceVtable, 0, 2));

    fixture.Reset();
    fixture.ClearPlayerEntry();
    CHECK(ExpectTopology(fixture, fixture.ExpectedManager(), 3,
                         TopologyDiagnostic::InvalidPlayerEntry, 1, 2));

    fixture.Reset();
    fixture.SetUnknownPlayerVtable();
    CHECK(ExpectTopology(fixture, fixture.ExpectedManager(), 3,
                         TopologyDiagnostic::UnknownPlayerVtable, 1, 2));

    fixture.Reset();
    fixture.ClearPlayerCol();
    CHECK(ExpectTopology(fixture, fixture.ExpectedManager(), 3,
                         TopologyDiagnostic::UnknownPlayerCol, 1, 2));

    fixture.Reset();
    fixture.SetTwinOrPossessor();
    CHECK(ExpectTopology(fixture, fixture.ExpectedManager(), 3,
                         TopologyDiagnostic::TwinOrPossessor, 1, 2));

    fixture.Reset();
    fixture.SetThreePlayers();
    CHECK(ExpectTopology(fixture, fixture.ExpectedManager(), 3,
                         TopologyDiagnostic::UnexpectedThirdPlayer, 3, 2));

    // Relationship fields can legitimately be between manager-owned writes
    // during initialization. They are diagnostic transient drops, never the
    // permanent TopologyAmbiguous error observed by live-01.
    fixture.Reset();
    fixture.SetPlayerOrdinal(1);
    CHECK(ExpectTopology(fixture, fixture.ExpectedManager(), 1,
                         TopologyDiagnostic::PlayerOrdinalMismatch, 1, 2));

    fixture.Reset();
    fixture.SetPlayerAssignment(-1);
    CHECK(ExpectTopology(fixture, fixture.ExpectedManager(), 1,
                         TopologyDiagnostic::PlayerAssignmentPending, 1, 2));

    fixture.Reset();
    fixture.SetPlayerAssignment(99);
    CHECK(ExpectTopology(fixture, fixture.ExpectedManager(), 1,
                         TopologyDiagnostic::AssignmentNotUnique, 1, 2));

    fixture.Reset();
    fixture.SetDeviceAssignment(1, 7);
    CHECK(ExpectTopology(fixture, fixture.ExpectedManager(), 1,
                         TopologyDiagnostic::AssignmentNotUnique, 1, 2));

    fixture.Reset();
    fixture.SetHud(0, nullptr);
    CHECK(ExpectTopology(fixture, fixture.ExpectedManager(), 1,
                         TopologyDiagnostic::HudMismatch, 1, 2));

    fixture.Reset();
    fixture.SetHud(4, fixture.MainImage());
    CHECK(ExpectTopology(fixture, fixture.ExpectedManager(), 1,
                         TopologyDiagnostic::HudMismatch, 1, 2));

    for (std::uint32_t raw = 1; raw <= 25; ++raw)
    {
        isaac_cs_t0::coop_passive::detail::TopologyTransitionProbe transition{};
        CHECK(isaac_cs_t0::coop_passive::detail::RunInvalidTopologyTransitionProbe(
            static_cast<TopologyDiagnostic>(raw), transition));
        CHECK(transition.unbound_stays_unbound);
        CHECK(transition.bound_becomes_unbound);
        CHECK(transition.first_valid_is_candidate);
        CHECK(transition.second_valid_is_bound);
    }
    isaac_cs_t0::coop_passive::detail::TopologyTransitionProbe rejected{};
    CHECK(!isaac_cs_t0::coop_passive::detail::RunInvalidTopologyTransitionProbe(
        TopologyDiagnostic::None, rejected));
    CHECK(!isaac_cs_t0::coop_passive::detail::RunInvalidTopologyTransitionProbe(
        static_cast<TopologyDiagnostic>(26), rejected));
    return true;
}

bool TestTopologyDiagnosticDigestContract()
{
    using isaac_cs_t0::coop_passive::detail::TopologyDiagnostic;
    using isaac_cs_t0::coop_passive::detail::TopologyDiagnosticDigest;

    // These vectors were independently generated as
    // SHA256("IsaacCsT0-CoopPassive-TopologyDiagnostic-v1" || uint32_le(code)).
    // Keeping the complete codebook here makes byte order, domain, stability,
    // and cross-language decoding part of the Native test contract.
    constexpr std::array<std::string_view, 25> kExpected{{
        "2C09A4A5D95FDC19252D9302FDD8A5F99C023B562F55843CA8D9417E533742B3",
        "728E8A073D0071DC0EF19F528D657309EB1770ED76EBE742F94DC1E664D767BB",
        "070AC84FE78800A813F68B14CD53837B09982983191DC61A6C914C8C4BF78760",
        "ED59D11B1A98A10AD89E85BD6E65C3EDA88BADA575D34BFD031E570CA7015EEC",
        "3B8BD7F299CD381A18EA52B7706903D883521A53C890A1D3F10DF52E68D82F09",
        "9C2D14B4B02274CB0A50EF80B30C6D93C1ECDB4AAD28D8679FD346CCB9378768",
        "F7C5EE9AFEA9C3658CABFA7B28DDF96FF16C8A28516BDBF94023F84A98BAE394",
        "1511132C1115BEB0F04ACB8C5B4B48D06D45C589597126AFE1A1DD822140085E",
        "A96D8FDAFCCBDE83CD8FA6058D9B304818C995ACD4A701338CD956C0762DE488",
        "37B1DFBD7503D2E49F1AE3881DEC25BB78CCA6F976CB6135BCCD5C973D5769D8",
        "254C7D2E5DBBF343EC0F4AC56D69CC39D87F9AF1D4C178B2F7F8C0B0D7054280",
        "6C459BCB675893F849B885D8BB6444E9E629F34E9A95C1C159C8BF20F13D1304",
        "25AEE064704A69C204B634763CF6B5444EAC7959B0B4D08BE6DD982DB9F28C3E",
        "FA613608761287D335DC9092B733E7894BB443440E2633565130EBC0E116EA8E",
        "FD1CBBFE33694ED0672BD15CAA917DB7320C4BD94F30AE7705C0AB89EC8CCD54",
        "813746922991DE8E14F3E96DF8C18A4F5F9508D8CAE818E7FFC310226C4BBFF8",
        "5D3A66832B42B3B2EAE787D16E0C12AF3EA5687243CF75F19AF089D13F0E0FCC",
        "C6DF94CEF7B3E5AAAE10863234C1B073222503ABB5DAA8043FD1723BA658CE05",
        "A0B07D872F01EEDB072D66CC5A96E6021287CC0C513223E794446960FD8CC13B",
        "8A266195D6DD253260A8D126308BE1ECA8D3C69492B9E070BE672F18F7A2446C",
        "65DD1A7725708E1FBFB979D7BDB559FE508FD6A049069037DD3F6E7A74A4341E",
        "10278C2713B0C0D108C26743516C9A86EA30A92EC87892FC08955FD09904B4B9",
        "5FDB056A88B410B512BA0B1340F945428E878B90628D103F566C7DA70757B5D8",
        "B09DA408BB01A5C08F5037D934B8F9D278578E200A05F8AA300E6A3A4BA9260C",
        "B00A9DDA5DDD97B3D2E499213E975B2907EA8C2327A775D3F0BD64B846DE923C",
    }};
    constexpr char kDomain[] =
        "IsaacCsT0-CoopPassive-TopologyDiagnostic-v1";
    constexpr std::array<std::uint8_t, 32> kZero{};
    std::array<std::array<std::uint8_t, 32>, 25> observed{};

    CHECK(TopologyDiagnosticDigest(TopologyDiagnostic::None) == kZero);
    CHECK(TopologyDiagnosticDigest(static_cast<TopologyDiagnostic>(26)) == kZero);
    CHECK(TopologyDiagnosticDigest(static_cast<TopologyDiagnostic>(UINT32_MAX)) ==
          kZero);
    for (std::uint32_t raw = 1;
         raw <= static_cast<std::uint32_t>(kExpected.size()); ++raw)
    {
        const std::array<std::uint8_t, 4> little_endian{{
            static_cast<std::uint8_t>(raw & 0xFFu),
            static_cast<std::uint8_t>((raw >> 8) & 0xFFu),
            static_cast<std::uint8_t>((raw >> 16) & 0xFFu),
            static_cast<std::uint8_t>((raw >> 24) & 0xFFu),
        }};
        const auto independently_encoded = isaac_cs_t0::mode::Sha256Pair(
            kDomain, sizeof(kDomain) - 1, little_endian.data(),
            little_endian.size());
        const auto expected = DigestFromHex(kExpected[raw - 1]);
        const auto actual =
            TopologyDiagnosticDigest(static_cast<TopologyDiagnostic>(raw));
        CHECK(expected != kZero);
        CHECK(independently_encoded == expected);
        CHECK(actual == expected);
        for (std::uint32_t prior = 1; prior < raw; ++prior)
            CHECK(actual != observed[prior - 1]);
        observed[raw - 1] = actual;
    }
    return true;
}

bool IsExactManagerSentinel(
    const isaac_cs_t0::coop_passive::detail::ManagerCallProbe& call)
{
    return call.original_calls == 1 && call.expected_return == 0xC0DEC045u &&
           call.observed_return == 0xC0DEC045u;
}

bool TestManagerDiagnosticAndOriginalOnceContract()
{
    using isaac_cs_t0::coop_passive::EventKind;
    using isaac_cs_t0::coop_passive::detail::ManagerDiagnosticProbe;
    using isaac_cs_t0::coop_passive::detail::RunManagerDiagnosticProbe;
    using isaac_cs_t0::coop_passive::detail::TopologyDiagnostic;
    using isaac_cs_t0::coop_passive::detail::TopologyDiagnosticDigest;

    constexpr std::array<std::uint32_t, 7> kNormalKinds{{
        static_cast<std::uint32_t>(EventKind::ManagerPassBegin),
        static_cast<std::uint32_t>(EventKind::ManagerPassEnd),
        static_cast<std::uint32_t>(EventKind::BindingCandidate),
        static_cast<std::uint32_t>(EventKind::ManagerPassBegin),
        static_cast<std::uint32_t>(EventKind::ManagerPassEnd),
        static_cast<std::uint32_t>(EventKind::BindingBound),
        static_cast<std::uint32_t>(EventKind::LuaVectorEquivalent),
    }};
    constexpr std::array<std::uint32_t, 2> kRejectedKinds{{
        static_cast<std::uint32_t>(EventKind::ManagerPassBegin),
        static_cast<std::uint32_t>(EventKind::ManagerPassEnd),
    }};
    constexpr std::array<std::uint8_t, 32> kZero{};

    ManagerDiagnosticProbe representative{};
    CHECK(RunManagerDiagnosticProbe(TopologyDiagnostic::RunNotActive,
                                    representative));
    CHECK(IsExactManagerSentinel(representative.normal_first));
    CHECK(IsExactManagerSentinel(representative.normal_second));
    CHECK(representative.normal_event_count ==
          static_cast<std::uint32_t>(kNormalKinds.size()));
    CHECK(representative.normal_event_kinds == kNormalKinds);
    CHECK(representative.first_end_digest != kZero);
    CHECK(representative.first_end_digest == representative.candidate_digest);
    CHECK(representative.first_end_digest == representative.second_end_digest);
    CHECK(representative.first_end_digest == representative.bound_digest);
    for (std::uint32_t raw = 1; raw <= 25; ++raw)
        CHECK(representative.first_end_digest != TopologyDiagnosticDigest(
                                                      static_cast<TopologyDiagnostic>(raw)));

    CHECK(IsExactManagerSentinel(representative.observation_disabled));
    CHECK(representative.observation_disabled_event_count == 0);
    CHECK(IsExactManagerSentinel(representative.wrong_thread));
    CHECK(representative.wrong_thread_event_count == 1);
    CHECK(representative.wrong_thread_last_event_kind ==
          static_cast<std::uint32_t>(EventKind::Failure));
    CHECK(IsExactManagerSentinel(representative.topology_read_exception));
    CHECK(representative.topology_read_exception_event_count == 2);
    CHECK(representative.topology_read_exception_digest ==
          TopologyDiagnosticDigest(TopologyDiagnostic::AccessViolation));
    CHECK(IsExactManagerSentinel(representative.ring_publish_failure));
    CHECK(representative.ring_publish_failed);

    for (std::uint32_t raw = 1; raw <= 25; ++raw)
    {
        const auto diagnostic = static_cast<TopologyDiagnostic>(raw);
        ManagerDiagnosticProbe rejected{};
        CHECK(RunManagerDiagnosticProbe(diagnostic, rejected));
        CHECK(rejected.rejected_diagnostic == diagnostic);
        CHECK(IsExactManagerSentinel(rejected.rejected));
        CHECK(rejected.rejected_event_count ==
              static_cast<std::uint32_t>(kRejectedKinds.size()));
        CHECK(rejected.rejected_event_kinds == kRejectedKinds);
        CHECK(rejected.expected_rejected_digest ==
              TopologyDiagnosticDigest(diagnostic));
        CHECK(rejected.rejected_digest == rejected.expected_rejected_digest);
        CHECK(rejected.rejected_digest != kZero);
    }

    ManagerDiagnosticProbe invalid{};
    CHECK(!RunManagerDiagnosticProbe(TopologyDiagnostic::None, invalid));
    CHECK(!RunManagerDiagnosticProbe(static_cast<TopologyDiagnostic>(26), invalid));
    return true;
}

bool TestBackendScreenAndRouteWitnessContract()
{
    using isaac_cs_t0::coop_passive::EventKind;
    using isaac_cs_t0::coop_passive::kNoOrdinal;
    using isaac_cs_t0::coop_passive::detail::BackendRouteWitnessProbe;
    using isaac_cs_t0::coop_passive::detail::BackendScreenCaseProbe;
    using isaac_cs_t0::coop_passive::detail::RunBackendRouteWitnessProbe;
    using isaac_cs_t0::coop_passive::detail::TopologyDiagnostic;

    BackendRouteWitnessProbe probe{};
    CHECK(RunBackendRouteWitnessProbe(probe));

    const std::array<const BackendScreenCaseProbe*, 23> cases{{
        &probe.null_backend_without_route,
        &probe.known_backend_without_route,
        &probe.known_backend_exact_route,
        &probe.misaligned_backend,
        &probe.unreadable_backend,
        &probe.unknown_backend_vtable,
        &probe.roster_begin_only,
        &probe.roster_end_only,
        &probe.roster_capacity_only,
        &probe.allocated_empty_roster,
        &probe.nonempty_roster,
        &probe.online_frame_present,
        &probe.partial_route,
        &probe.out_of_order_route,
        &probe.stale_generation,
        &probe.cross_generation_witness,
        &probe.possessor_route,
        &probe.exit_route,
        &probe.unrelated_assignment,
        &probe.assignment_pending,
        &probe.player_ordinal_mismatch,
        &probe.hud_mismatch,
        &probe.intervening_rejected_pass,
    }};
    const std::array<std::uint32_t, 23> expected_manager_calls{{
        2, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3,
    }};
    for (std::size_t index = 0; index < cases.size(); ++index)
    {
        CHECK(cases[index]->manager_original_calls ==
              expected_manager_calls[index]);
        CHECK(cases[index]->expected_return != 0);
        CHECK(cases[index]->observed_return == cases[index]->expected_return);
    }

    const auto expect_screen = [&](const BackendScreenCaseProbe& value,
                                   std::uint32_t classification,
                                   TopologyDiagnostic diagnostic) noexcept {
        return value.classification == classification &&
               value.diagnostic == diagnostic &&
               value.manager_end_classification == classification &&
               value.manager_end_diagnostic == diagnostic;
    };
    const auto expect_rejected = [&](const BackendScreenCaseProbe& value,
                                     std::uint32_t classification,
                                     TopologyDiagnostic diagnostic) noexcept {
        return expect_screen(value, classification, diagnostic) &&
               !value.route_authorized && value.remains_unbound &&
               value.binding_candidate_count == 0 &&
               value.binding_bound_count == 0 &&
               value.terminal_failure_count == 0;
    };

    // The existing null-backend C0 route remains valid without an ordinary
    // route witness and still requires two consecutive coherent manager ends.
    CHECK(expect_screen(probe.null_backend_without_route, 0,
                        TopologyDiagnostic::None));
    CHECK(!probe.null_backend_without_route.route_authorized);
    CHECK(!probe.null_backend_without_route.remains_unbound);
    CHECK(probe.null_backend_without_route.binding_candidate_count == 2);
    CHECK(probe.null_backend_without_route.binding_bound_count == 2);
    CHECK(probe.null_backend_without_route.terminal_failure_count == 0);
    CHECK(probe.null_backend_without_route.generation_before == 1);
    CHECK(probe.null_backend_without_route.generation_after == 1);

    CHECK(expect_rejected(probe.known_backend_without_route, 3,
                          TopologyDiagnostic::OnlineBackendPresent));
    CHECK(probe.known_backend_without_route.generation_before == 1);
    CHECK(probe.known_backend_without_route.generation_after == 1);

    // A known Steam backend is admitted only by the complete route in one
    // manager transaction. The assignment does not invalidate a second time.
    CHECK(expect_screen(probe.known_backend_exact_route, 0,
                        TopologyDiagnostic::None));
    CHECK(probe.known_backend_exact_route.route_authorized);
    CHECK(!probe.known_backend_exact_route.remains_unbound);
    CHECK(probe.known_backend_exact_route.spawn_original_calls == 1);
    CHECK(probe.known_backend_exact_route.assignment_original_calls == 1);
    CHECK(probe.known_backend_exact_route.hud_original_calls == 1);
    CHECK(probe.known_backend_exact_route.binding_candidate_count == 2);
    CHECK(probe.known_backend_exact_route.binding_bound_count == 2);
    CHECK(probe.known_backend_exact_route.terminal_failure_count == 0);
    CHECK(probe.exact_generation_before == 1);
    CHECK(probe.exact_generation_after_spawn == 2);
    CHECK(probe.exact_generation_after_assignment == 2);
    CHECK(probe.known_backend_exact_route.generation_after == 2);

    constexpr std::array<std::uint32_t, 18> kExactKinds{{
        static_cast<std::uint32_t>(EventKind::ManagerPassBegin),
        static_cast<std::uint32_t>(EventKind::BindingUnbound),
        static_cast<std::uint32_t>(EventKind::BindingUnbound),
        static_cast<std::uint32_t>(EventKind::OrdinarySpawnBegin),
        static_cast<std::uint32_t>(EventKind::OrdinarySpawnEnd),
        static_cast<std::uint32_t>(EventKind::OrdinaryAssignmentBegin),
        static_cast<std::uint32_t>(EventKind::OrdinaryAssignmentEnd),
        static_cast<std::uint32_t>(EventKind::JoinHudBegin),
        static_cast<std::uint32_t>(EventKind::JoinHudEnd),
        static_cast<std::uint32_t>(EventKind::ManagerPassEnd),
        static_cast<std::uint32_t>(EventKind::BindingCandidate),
        static_cast<std::uint32_t>(EventKind::BindingCandidate),
        static_cast<std::uint32_t>(EventKind::ManagerPassBegin),
        static_cast<std::uint32_t>(EventKind::ManagerPassEnd),
        static_cast<std::uint32_t>(EventKind::BindingBound),
        static_cast<std::uint32_t>(EventKind::LuaVectorEquivalent),
        static_cast<std::uint32_t>(EventKind::BindingBound),
        static_cast<std::uint32_t>(EventKind::LuaVectorEquivalent),
    }};
    constexpr std::array<std::uint32_t, 18> kExactOrdinals{{
        kNoOrdinal, 0,          1,          kNoOrdinal, kNoOrdinal,
        kNoOrdinal, kNoOrdinal, kNoOrdinal, kNoOrdinal, kNoOrdinal,
        0,          1,          kNoOrdinal, kNoOrdinal, 0,
        0,          1,          1,
    }};
    constexpr std::array<std::uint32_t, 18> kExactEndpoints{{
        0, 1, 2, 0, 0, 0, 0, 0, 0, 0, 1, 2, 0, 0, 1, 1, 2, 2,
    }};
    CHECK(probe.exact_event_count == kExactKinds.size());
    for (std::size_t index = 0; index < kExactKinds.size(); ++index)
    {
        CHECK(probe.exact_event_kinds[index] == kExactKinds[index]);
        CHECK(probe.exact_event_local_ordinals[index] ==
              kExactOrdinals[index]);
        CHECK(probe.exact_event_endpoints[index] == kExactEndpoints[index]);
        CHECK(probe.exact_event_generations[index] == (index == 0 ? 1u : 2u));
    }

    for (const auto* value : {&probe.misaligned_backend,
                              &probe.unreadable_backend,
                              &probe.unknown_backend_vtable,
                              &probe.roster_begin_only,
                              &probe.roster_end_only,
                              &probe.roster_capacity_only,
                              &probe.allocated_empty_roster,
                              &probe.nonempty_roster,
                              &probe.online_frame_present})
    {
        CHECK(value->spawn_original_calls == 1);
        CHECK(value->assignment_original_calls == 1);
        CHECK(value->hud_original_calls == 1);
    }
    CHECK(expect_rejected(probe.misaligned_backend, 3,
                          TopologyDiagnostic::OnlineBackendPresent));
    CHECK(expect_rejected(probe.unreadable_backend, 2,
                          TopologyDiagnostic::AccessViolation));
    CHECK(expect_rejected(probe.unknown_backend_vtable, 3,
                          TopologyDiagnostic::OnlineBackendPresent));
    for (const auto* value : {&probe.roster_begin_only,
                              &probe.roster_end_only,
                              &probe.roster_capacity_only})
        CHECK(expect_rejected(*value, 1,
                              TopologyDiagnostic::OnlineRosterHeaderInvalid));
    for (const auto* value : {&probe.allocated_empty_roster,
                              &probe.nonempty_roster})
        CHECK(expect_rejected(*value, 3,
                              TopologyDiagnostic::OnlineRosterPresent));
    CHECK(expect_rejected(probe.online_frame_present, 3,
                          TopologyDiagnostic::OfflineOnlineFrameNonzero));

    for (const auto* value : {&probe.partial_route,
                              &probe.out_of_order_route,
                              &probe.stale_generation,
                              &probe.cross_generation_witness,
                              &probe.exit_route,
                              &probe.unrelated_assignment})
        CHECK(expect_rejected(*value, 3,
                              TopologyDiagnostic::OnlineBackendPresent));
    CHECK(probe.partial_route.spawn_original_calls == 1);
    CHECK(probe.partial_route.assignment_original_calls == 0);
    CHECK(probe.partial_route.hud_original_calls == 0);
    CHECK(probe.out_of_order_route.spawn_original_calls == 1);
    CHECK(probe.out_of_order_route.assignment_original_calls == 1);
    CHECK(probe.out_of_order_route.hud_original_calls == 1);
    CHECK(probe.stale_generation.spawn_original_calls == 1);
    CHECK(probe.stale_generation.assignment_original_calls == 1);
    CHECK(probe.stale_generation.hud_original_calls == 1);
    CHECK(probe.stale_generation.generation_after == 3);
    CHECK(probe.cross_generation_witness.spawn_original_calls == 1);
    CHECK(probe.cross_generation_witness.assignment_original_calls == 2);
    CHECK(probe.cross_generation_witness.hud_original_calls == 1);
    CHECK(probe.cross_generation_witness.generation_after == 3);
    CHECK(probe.exit_route.exit_original_calls == 1);
    CHECK(probe.exit_route.generation_after == 2);
    CHECK(probe.unrelated_assignment.assignment_original_calls == 1);
    CHECK(probe.unrelated_assignment.generation_after == 2);

    // The actual possessor wrapper remains the pre-existing stop sentinel: it
    // calls original once, never authorizes the route, and emits one Failure.
    CHECK(expect_screen(probe.possessor_route, 3,
                        TopologyDiagnostic::OnlineBackendPresent));
    CHECK(probe.possessor_route.possessor_spawn_original_calls == 1);
    CHECK(probe.possessor_route.spawn_original_calls == 0);
    CHECK(!probe.possessor_route.route_authorized);
    CHECK(probe.possessor_route.remains_unbound);
    CHECK(probe.possessor_route.binding_candidate_count == 0);
    CHECK(probe.possessor_route.binding_bound_count == 0);
    CHECK(probe.possessor_route.terminal_failure_count == 1);

    // A valid wrapper witness cannot weaken topology relationship checks. Each
    // rejected pass revokes the witness, leaves both ordinals unbound, and does
    // not publish a terminal failure.
    for (const auto* value : {&probe.assignment_pending,
                              &probe.player_ordinal_mismatch,
                              &probe.hud_mismatch})
    {
        CHECK(!value->route_authorized);
        CHECK(value->remains_unbound);
        CHECK(value->spawn_original_calls == 1);
        CHECK(value->assignment_original_calls == 1);
        CHECK(value->hud_original_calls == 1);
        CHECK(value->binding_candidate_count == 0);
        CHECK(value->binding_bound_count == 0);
        CHECK(value->terminal_failure_count == 0);
    }
    CHECK(expect_screen(probe.assignment_pending, 1,
                        TopologyDiagnostic::PlayerAssignmentPending));
    CHECK(expect_screen(probe.player_ordinal_mismatch, 1,
                        TopologyDiagnostic::PlayerOrdinalMismatch));
    CHECK(expect_screen(probe.hud_mismatch, 1,
                        TopologyDiagnostic::HudMismatch));

    // An invalid pass between two valid samples drops the first candidate and
    // revokes the backend witness. The third pass cannot bind or recandidate
    // without a fresh complete ordinary route in that manager transaction.
    CHECK(probe.intervening_rejected_pass.classification == 3);
    CHECK(probe.intervening_rejected_pass.diagnostic ==
          TopologyDiagnostic::OnlineBackendPresent);
    CHECK(probe.intervening_rejected_pass.manager_end_classification == 1);
    CHECK(probe.intervening_rejected_pass.manager_end_diagnostic ==
          TopologyDiagnostic::PlayerAssignmentPending);
    CHECK(!probe.intervening_rejected_pass.route_authorized);
    CHECK(probe.intervening_rejected_pass.remains_unbound);
    CHECK(probe.intervening_rejected_pass.spawn_original_calls == 1);
    CHECK(probe.intervening_rejected_pass.assignment_original_calls == 1);
    CHECK(probe.intervening_rejected_pass.hud_original_calls == 1);
    CHECK(probe.intervening_rejected_pass.binding_candidate_count == 2);
    CHECK(probe.intervening_rejected_pass.binding_bound_count == 0);
    CHECK(probe.intervening_rejected_pass.terminal_failure_count == 0);
    return true;
}

std::uint32_t Rel32Target(std::uint32_t call_rva,
                          const std::array<std::uint8_t, 5>& bytes)
{
    std::int32_t displacement = 0;
    std::memcpy(&displacement, bytes.data() + 1, sizeof(displacement));
    return static_cast<std::uint32_t>(
        static_cast<std::int64_t>(call_rva) + 5 + displacement);
}

bool TestApprovedLocalCoopCallsiteGraph()
{
    // Exact target-image slices frozen by coop-static-inspection-v1 and the
    // routes/roles frozen by coop-static-graph-v1. This deliberately stops at
    // the approved local-coop branch boundary: the separate upstream
    // controller-assignment candidate remains UNKNOWN and is not inferred here.
    struct FrozenCall {
        std::uint32_t rva;
        std::array<std::uint8_t, 5> bytes;
        std::uint32_t target;
    };
    constexpr std::array<FrozenCall, 8> kCalls{{
        {0x002FADA7u, {{0xE8, 0x94, 0xFE, 0x2B, 0x00}}, 0x005BAC40u},
        {0x005BB415u, {{0xE8, 0xB6, 0xE8, 0xFF, 0xFF}}, 0x005B9CD0u},
        {0x005BAFFAu, {{0xE8, 0xD1, 0xEC, 0xFF, 0xFF}}, 0x005B9CD0u},
        {0x005BB22Au, {{0xE8, 0xD1, 0xED, 0xFF, 0xFF}}, 0x005BA000u},
        {0x005BB424u, {{0xE8, 0x27, 0xB0, 0xDE, 0xFF}}, 0x003A6450u},
        {0x005BB004u, {{0xE8, 0x47, 0xB4, 0xDE, 0xFF}}, 0x003A6450u},
        {0x005BB4FDu, {{0xE8, 0x1E, 0xD1, 0xFE, 0xFF}}, 0x005A8620u},
        {0x005BA752u, {{0xE8, 0xC9, 0xDE, 0xFE, 0xFF}}, 0x005A8620u},
    }};
    for (const auto& call : kCalls)
    {
        CHECK(call.bytes[0] == 0xE8u);
        const auto resolved = Rel32Target(call.rva, call.bytes);
        if (resolved != call.target)
            std::fprintf(stderr,
                         "REL32 mismatch at %08X: resolved %08X expected %08X\n",
                         call.rva, resolved, call.target);
        CHECK(resolved == call.target);
    }

    // The approved online guard is a JNZ rel32 at offset 18. A non-empty
    // NetInputDevice vector jumps to 0x5BB574 and therefore skips the complete
    // local body 0x5BAC85..0x5BB56E.
    constexpr std::uint32_t kGuardRva = 0x005BAC6Du;
    constexpr std::array<std::uint8_t, 24> kGuard{{
        0x8B, 0x15, 0x9C, 0x16, 0xC7, 0x00, 0x8B, 0x82,
        0xD8, 0xB3, 0x04, 0x00, 0x3B, 0x82, 0xDC, 0xB3,
        0x04, 0x00, 0x0F, 0x85, 0xEF, 0x08, 0x00, 0x00,
    }};
    std::int32_t guard_displacement = 0;
    std::memcpy(&guard_displacement, kGuard.data() + 20,
                sizeof(guard_displacement));
    CHECK(kGuard[18] == 0x0Fu && kGuard[19] == 0x85u);
    CHECK(static_cast<std::uint32_t>(
              static_cast<std::int64_t>(kGuardRva) + kGuard.size() +
              guard_displacement) == 0x005BB574u);
    static_assert(0x005BAC85u < 0x005BAFFAu &&
                  0x005BB4FDu < 0x005BB56Eu);

    // Approved branch separation: ordinary join has the unique ordered
    // spawn->assignment->HUD chain, possessor has only its two stop sentinels,
    // and exit has its distinct retire->HUD chain.
    CHECK(kCalls[1].rva < kCalls[4].rva && kCalls[4].rva < kCalls[6].rva);
    CHECK(kCalls[2].rva < kCalls[5].rva &&
          kCalls[2].target == kCalls[1].target &&
          kCalls[5].target == kCalls[4].target);
    CHECK(kCalls[3].target < kCalls[7].rva &&
          kCalls[3].target == 0x005BA000u &&
          kCalls[3].target != kCalls[1].target &&
          kCalls[7].target == kCalls[6].target);
    return true;
}

bool TestIpcAbi()
{
    using namespace isaac_cs_t0::coop_passive;
    static_assert(sizeof(void*) == 4);
    static_assert(kCombinedSlotCount == 53);
    static_assert(kCompleteHookMask == 0x001FFFFFFFFFFFFFull);
    static_assert(alignof(NativePassiveEvent) == 8);
    static_assert(alignof(SharedCoopPassiveBlock) == 8);
    static_assert(sizeof(NativePassiveEvent) == kNativeEventSize);
    static_assert(sizeof(SharedCoopPassiveBlock) == kSharedBlockSize);
    static_assert(offsetof(NativePassiveEvent, published_sequence) == 0);
    static_assert(offsetof(SharedCoopPassiveBlock, events) == kEventsOffset);
    return true;
}

bool TestOrdered53SlotTransaction()
{
    using namespace isaac_cs_t0::mode;
    OrderedPatchFixture fixture;
    CHECK(fixture.memory != nullptr);
    SetPatchTestFault(PatchTestFaultStage::None, UINT32_MAX);
    CHECK(InstallOrderedPatchTransaction(fixture.descriptors.data(),
                                         fixture.descriptors.size()));
    CHECK(fixture.InstalledPrefix(OrderedPatchFixture::kCombined));
    CHECK(RestoreOrderedPatchTransaction(fixture.descriptors.data(),
                                         fixture.descriptors.size()));
    CHECK(fixture.Restored());

    for (std::uint32_t ordinal = 0;
         ordinal < OrderedPatchFixture::kCombined; ++ordinal)
    {
        fixture.Reset();
        const bool pointer = ordinal >= 27 && ordinal <= 38;
        SetPatchTestFault(pointer ? PatchTestFaultStage::PreparePointer
                                  : PatchTestFaultStage::PrepareRelative,
                          ordinal);
        PatchTransactionFailure failure{};
        CHECK(!InstallOrderedPatchTransaction(fixture.descriptors.data(),
                                              fixture.descriptors.size(), &failure));
        CHECK(failure.index == ordinal);
        CHECK(fixture.Restored());

        fixture.Reset();
        SetPatchTestFault(pointer ? PatchTestFaultStage::ApplyPointer
                                  : PatchTestFaultStage::ApplyRelative,
                          ordinal);
        CHECK(!InstallOrderedPatchTransaction(fixture.descriptors.data(),
                                              fixture.descriptors.size(), &failure));
        CHECK(failure.index == ordinal);
        CHECK(fixture.Restored());

        fixture.Reset();
        SetPatchTestFault(PatchTestFaultStage::Protect, ordinal);
        CHECK(!InstallOrderedPatchTransaction(fixture.descriptors.data(),
                                              fixture.descriptors.size(), &failure));
        CHECK(failure.index == ordinal);
        CHECK(fixture.Restored());

        fixture.Reset();
        SetPatchTestFault(PatchTestFaultStage::None, UINT32_MAX);
        CHECK(InstallOrderedPatchTransaction(fixture.descriptors.data(),
                                             fixture.descriptors.size()));
        SetPatchTestFault(pointer ? PatchTestFaultStage::RestorePointer
                                  : PatchTestFaultStage::RestoreRelative,
                          ordinal);
        CHECK(!RestoreOrderedPatchTransaction(fixture.descriptors.data(),
                                              fixture.descriptors.size()));
        SetPatchTestFault(PatchTestFaultStage::None, UINT32_MAX);
        CHECK(RestoreOrderedPatchTransaction(fixture.descriptors.data(),
                                             fixture.descriptors.size()));
        CHECK(fixture.Restored());
    }

    for (const auto stage : {PatchTestFaultStage::ThreadSnapshot,
                             PatchTestFaultStage::ThreadSuspend})
    {
        fixture.Reset();
        SetPatchTestFault(stage, 0);
        PatchTransactionFailure failure{};
        CHECK(!InstallOrderedPatchTransaction(fixture.descriptors.data(),
                                              fixture.descriptors.size(), &failure));
        CHECK(fixture.Restored());
    }

    fixture.Reset();
    std::swap(fixture.descriptors[7], fixture.descriptors[8]);
    CHECK(!InstallOrderedPatchTransaction(fixture.descriptors.data(),
                                          fixture.descriptors.size()));
    CHECK(fixture.Restored());
    fixture.Reset();
    fixture.descriptors[52].ordinal = 51;
    CHECK(!InstallOrderedPatchTransaction(fixture.descriptors.data(),
                                          fixture.descriptors.size()));
    CHECK(fixture.Restored());

    fixture.Reset();
    SetPatchTestFaultPair(PatchTestFaultStage::ApplyRelative, 52,
                          PatchTestFaultStage::RollbackRelative, 51);
    PatchTransactionFailure rollback_failure{};
    CHECK(!InstallOrderedPatchTransaction(fixture.descriptors.data(),
                                          fixture.descriptors.size(),
                                          &rollback_failure));
    CHECK(rollback_failure.index == 51);
    SetPatchTestFault(PatchTestFaultStage::None, UINT32_MAX);
    CHECK(RestoreOrderedPatchTransaction(fixture.descriptors.data(),
                                         fixture.descriptors.size()));
    CHECK(fixture.Restored());
    return true;
}

bool TestOriginalOnceAndBindingContract()
{
    using namespace isaac_cs_t0::coop_passive::detail;
    AbiOriginalOnceProbe abi{};
    CHECK(RunAbiOriginalOnceProbe(abi));
    for (const auto original_calls : abi.original_calls)
        CHECK(original_calls == 1);
    CHECK(abi.AllOriginalsCalledExactlyOnce());
    CHECK(abi.ArgumentsAndReturnsPreserved());
    CHECK(abi.ObservationFailureStillPassedThrough());
    CHECK(abi.PossessorStoppedWithoutSuppressingOriginal());

    BindingMachineProbe binding{};
    CHECK(RunBindingMachineProbe(binding));
    CHECK(binding.two_complete_passes_required);
    CHECK(binding.endpoint_ordinals_frozen);
    CHECK(binding.duplicate_and_third_ordinal_rejected);
    CHECK(binding.lifecycle_invalidates_before_rebind);
    CHECK(binding.old_generation_never_revived);
    CHECK(binding.menu_teardown_retires_stable_and_temporary_layers);
    CHECK(binding.invalid_manager_pass_unbinds_without_retirement);
    return true;
}

int PublishCrossLanguage(int count, wchar_t** arguments)
{
    if (count != 5)
        return 2;
    return isaac_cs_t0::coop_passive::detail::PublishCrossLanguageFixture(
        arguments[2], arguments[3], arguments[4]);
}

} // namespace

int wmain(int count, wchar_t** arguments)
{
    if (count > 1 && std::wcscmp(arguments[1],
                                 L"--publish-coop-passive-cross-language") == 0)
        return PublishCrossLanguage(count, arguments);

    const bool abi = TestIpcAbi();
    Suite("coop-passive-ipc-abi", 8, abi);
    const bool transaction = TestOrdered53SlotTransaction();
    Suite("coop-passive-53-slot-transaction", 221, transaction);
    const bool runtime = TestOriginalOnceAndBindingContract();
    Suite("coop-passive-runtime-contract", 34, runtime);
    const bool topology = TestFrozenSinglePlayerTopology();
    Suite("coop-passive-topology-regression", 49, topology);
    const bool diagnostic = TestTopologyDiagnosticDigestContract();
    Suite("coop-passive-topology-diagnostic-digest", 28, diagnostic);
    const bool manager_diagnostic =
        TestManagerDiagnosticAndOriginalOnceContract();
    Suite("coop-passive-manager-diagnostic", 32, manager_diagnostic);
    const bool backend_screen = TestBackendScreenAndRouteWitnessContract();
    Suite("coop-passive-backend-screen-route-witness", 23, backend_screen);
    const bool callsites = TestApprovedLocalCoopCallsiteGraph();
    Suite("coop-passive-approved-callsite-graph", 12, callsites);
    if (g_failures == 0)
        std::puts("PASS CS-T0-05-03 passive Native harness (407 cases)");
    return g_failures == 0 ? 0 : 1;
}
