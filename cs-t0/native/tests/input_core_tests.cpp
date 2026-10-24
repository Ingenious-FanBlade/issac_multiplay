#include "isaac_cs_t0/input_core.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <limits>

namespace {

int g_input_failures = 0;

#define INPUT_CHECK(expression)                                                                    \
    do                                                                                             \
    {                                                                                              \
        if (!(expression))                                                                         \
        {                                                                                          \
            std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expression);           \
            ++g_input_failures;                                                                    \
        }                                                                                          \
    } while (false)

using namespace isaac_cs_t0::input;

std::array<std::uint8_t, 16> Session(std::uint8_t marker = 1)
{
    std::array<std::uint8_t, 16> session{};
    session[0] = marker;
    return session;
}

InputCmdDraftV1 Command(std::uint64_t sequence, std::uint64_t target,
                        const InputStateDraftV1& state = {})
{
    InputCmdDraftV1 command{};
    command.session_id = Session();
    command.client_id = 7;
    command.sequence = sequence;
    command.target_tick = target;
    command.state = state;
    return command;
}

void TestPrimitiveActionClassification()
{
    std::array<PrimitiveBinding, kMaximumLogicalActionCount> bindings{};
    for (auto& binding : bindings)
        binding.fill(kUnboundPrimitive);
    bindings[0][0] = 0x41;
    bindings[4][0] = 0x107;
    bindings[9][0] = 0x20;
    bindings[12][0] = 0x1B;
    bindings[14][0] = 0x20;
    bindings[20][0] = 0x107;

    auto result = ClassifyPrimitiveAction(0x41, bindings.data(), bindings.size());
    INPUT_CHECK(result.classification == PrimitiveActionClass::UniqueGameplay);
    INPUT_CHECK(result.action == 0);
    result = ClassifyPrimitiveAction(0x107, bindings.data(), bindings.size());
    INPUT_CHECK(result.classification == PrimitiveActionClass::AmbiguousOrOutsideGameplay);
    INPUT_CHECK(result.action == UINT32_MAX);
    result = ClassifyPrimitiveAction(0x20, bindings.data(), bindings.size());
    INPUT_CHECK(result.classification == PrimitiveActionClass::AmbiguousOrOutsideGameplay);
    result = ClassifyPrimitiveAction(0x1B, bindings.data(), bindings.size());
    INPUT_CHECK(result.classification == PrimitiveActionClass::AmbiguousOrOutsideGameplay);
    result = ClassifyPrimitiveAction(0xDEAD, bindings.data(), bindings.size());
    INPUT_CHECK(result.classification == PrimitiveActionClass::NoMatch);
    result = ClassifyPrimitiveAction(kUnboundPrimitive, bindings.data(), bindings.size());
    INPUT_CHECK(result.classification == PrimitiveActionClass::NoMatch);
    result = ClassifyPrimitiveAction(0x41, bindings.data(), bindings.size() + 1);
    INPUT_CHECK(result.classification == PrimitiveActionClass::InvalidLayout);
    result = ClassifyPrimitiveAction(0x41, nullptr, 1);
    INPUT_CHECK(result.classification == PrimitiveActionClass::InvalidLayout);

    std::array<PrimitiveBinding, 1> duplicate_slot{{
        {0x41, 0x41, kUnboundPrimitive, kUnboundPrimitive},
    }};
    result = ClassifyPrimitiveAction(0x41, duplicate_slot.data(), duplicate_slot.size());
    INPUT_CHECK(result.classification == PrimitiveActionClass::UniqueGameplay);
    INPUT_CHECK(result.action == 0);
}

void TestCanonicalCodecAndNormalization()
{
    static_assert(sizeof(InputStateDraftV1) == 24);
    static_assert(sizeof(InputCmdDraftV1) == 64);
    InputStateDraftV1 neutral{};
    EncodedInputStateV1 bytes{};
    INPUT_CHECK(EncodeInputState(neutral, kBaseValidEdgeMask, bytes) == CodecError::None);
    INPUT_CHECK(bytes[0] == 1 && bytes[1] == 0);
    INPUT_CHECK(bytes[2] == 24 && bytes[3] == 0);
    for (std::size_t index = 4; index < bytes.size(); ++index)
        INPUT_CHECK(bytes[index] == 0);

    InputStateDraftV1 decoded{};
    INPUT_CHECK(DecodeInputState(bytes.data(), bytes.size(), kBaseValidEdgeMask, decoded) ==
                CodecError::None);
    INPUT_CHECK(decoded.version == neutral.version && decoded.edge_flags == 0);

    RawInputSample raw{};
    raw.move_x = 1.0F;
    raw.aim_y = -1.0F;
    raw.held_flags = kHeldShoot;
    raw.edge_flags = kEdgeBomb | kEdgeDrop;
    InputStateDraftV1 state{};
    INPUT_CHECK(CanonicalizeRaw(raw, kBaseValidEdgeMask, state) == CodecError::None);
    INPUT_CHECK(state.move_x == kAxisMaximum && state.move_y == 0);
    INPUT_CHECK(state.aim_x == 0 && state.aim_y == -kAxisMaximum);
    INPUT_CHECK(state.held_flags == kHeldShoot);
    INPUT_CHECK(state.edge_flags == (kEdgeBomb | kEdgeDrop));

    raw = {};
    raw.move_x = 1.0F;
    raw.move_y = 1.0F;
    INPUT_CHECK(CanonicalizeRaw(raw, kBaseValidEdgeMask, state) == CodecError::None);
    INPUT_CHECK(state.move_x == 23169 && state.move_y == 23169);

    raw = {};
    raw.move_x = static_cast<float>(kRadialDeadZone) / static_cast<float>(kAxisMaximum);
    INPUT_CHECK(CanonicalizeRaw(raw, kBaseValidEdgeMask, state) == CodecError::None);
    INPUT_CHECK(state.move_x == 0 && state.move_y == 0);
    raw.move_x = 0.5F;
    INPUT_CHECK(CanonicalizeRaw(raw, kBaseValidEdgeMask, state) == CodecError::None);
    INPUT_CHECK(state.move_x == 14042);
    raw.move_x = -0.5F;
    INPUT_CHECK(CanonicalizeRaw(raw, kBaseValidEdgeMask, state) == CodecError::None);
    INPUT_CHECK(state.move_x == -14042);

    raw.move_x = (std::numeric_limits<float>::infinity)();
    INPUT_CHECK(CanonicalizeRaw(raw, kBaseValidEdgeMask, state) == CodecError::NonFiniteRaw);
    raw.move_x = (std::numeric_limits<float>::quiet_NaN)();
    INPUT_CHECK(CanonicalizeRaw(raw, kBaseValidEdgeMask, state) == CodecError::NonFiniteRaw);
    raw.move_x = 2.0F;
    INPUT_CHECK(CanonicalizeRaw(raw, kBaseValidEdgeMask, state) == CodecError::None);
    INPUT_CHECK(state.move_x == kAxisMaximum && state.move_y == 0);
    raw.move_x = -2.0F;
    INPUT_CHECK(CanonicalizeRaw(raw, kBaseValidEdgeMask, state) == CodecError::None);
    INPUT_CHECK(state.move_x == -kAxisMaximum && state.move_y == 0);

    raw = {};
    raw.aim_x = 1.0F;
    INPUT_CHECK(CanonicalizeRaw(raw, kBaseValidEdgeMask, state) ==
                CodecError::ShootAimMismatch);
    raw = {};
    raw.held_flags = kHeldShoot;
    INPUT_CHECK(CanonicalizeRaw(raw, kBaseValidEdgeMask, state) ==
                CodecError::ShootAimMismatch);
}

void TestStrictDecoder()
{
    InputStateDraftV1 state{};
    EncodedInputStateV1 bytes{};
    INPUT_CHECK(EncodeInputState(state, kBaseValidEdgeMask, bytes) == CodecError::None);
    InputStateDraftV1 decoded{};

    auto mutated = bytes;
    mutated[0] = 2;
    INPUT_CHECK(DecodeInputState(mutated.data(), mutated.size(), kBaseValidEdgeMask, decoded) ==
                CodecError::InvalidVersion);
    mutated = bytes;
    mutated[2] = 23;
    INPUT_CHECK(DecodeInputState(mutated.data(), mutated.size(), kBaseValidEdgeMask, decoded) ==
                CodecError::InvalidLength);
    mutated = bytes;
    mutated[4] = 2;
    INPUT_CHECK(DecodeInputState(mutated.data(), mutated.size(), kBaseValidEdgeMask, decoded) ==
                CodecError::UnknownHeldFlags);
    mutated = bytes;
    mutated[8] = 0x10;
    INPUT_CHECK(DecodeInputState(mutated.data(), mutated.size(), kBaseValidEdgeMask, decoded) ==
                CodecError::UnknownEdgeFlags);
    INPUT_CHECK(DecodeInputState(mutated.data(), mutated.size(), UINT32_MAX, decoded) ==
                CodecError::UnknownEdgeFlags);
    mutated = bytes;
    mutated[20] = 1;
    INPUT_CHECK(DecodeInputState(mutated.data(), mutated.size(), kBaseValidEdgeMask, decoded) ==
                CodecError::ReservedNonzero);
    mutated = bytes;
    mutated[12] = 0;
    mutated[13] = 0x80;
    INPUT_CHECK(DecodeInputState(mutated.data(), mutated.size(), kBaseValidEdgeMask, decoded) ==
                CodecError::AxisOutOfRange);

    state.move_x = 30000;
    state.move_y = 30000;
    INPUT_CHECK(ValidateInputState(state, kBaseValidEdgeMask) == CodecError::VectorOutOfRange);
    state = {};
    state.held_flags = kHeldShoot;
    INPUT_CHECK(ValidateInputState(state, kBaseValidEdgeMask) == CodecError::ShootAimMismatch);
    INPUT_CHECK(DecodeInputState(bytes.data(), bytes.size() - 1, kBaseValidEdgeMask, decoded) ==
                CodecError::InvalidSize);
}

void TestFrozenGoldenBytes()
{
    InputStateDraftV1 positive{};
    positive.held_flags = kHeldShoot;
    positive.edge_flags = kBaseValidEdgeMask;
    positive.move_x = 4097;
    positive.aim_x = 32767;
    EncodedInputStateV1 bytes{};
    INPUT_CHECK(EncodeInputState(positive, kBaseValidEdgeMask, bytes) == CodecError::None);
    constexpr EncodedInputStateV1 expected_positive{{
        0x01, 0x00, 0x18, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x0F, 0x00, 0x00, 0x00, 0x01, 0x10, 0x00, 0x00,
        0xFF, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    }};
    INPUT_CHECK(bytes == expected_positive);

    RawInputSample diagonal{};
    diagonal.held_flags = kHeldShoot;
    diagonal.move_x = -1.0F;
    diagonal.move_y = 1.0F;
    diagonal.aim_x = 1.0F;
    diagonal.aim_y = -1.0F;
    InputStateDraftV1 state{};
    INPUT_CHECK(CanonicalizeRaw(diagonal, kBaseValidEdgeMask, state) == CodecError::None);
    INPUT_CHECK(EncodeInputState(state, kBaseValidEdgeMask, bytes) == CodecError::None);
    constexpr EncodedInputStateV1 expected_diagonal{{
        0x01, 0x00, 0x18, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x7F, 0xA5, 0x81, 0x5A,
        0x81, 0x5A, 0x7F, 0xA5, 0x00, 0x00, 0x00, 0x00,
    }};
    INPUT_CHECK(bytes == expected_diagonal);
}

void TestCommandCodecAndLoopback()
{
    InputStateDraftV1 state{};
    state.edge_flags = kEdgeActiveItem;
    InputCmdDraftV1 command = Command(5, 12, state);
    EncodedInputCmdV1 encoded{};
    INPUT_CHECK(EncodeInputCommand(command, kBaseValidEdgeMask, encoded) == CodecError::None);
    InputCmdDraftV1 decoded{};
    INPUT_CHECK(DecodeInputCommand(encoded.data(), encoded.size(), kBaseValidEdgeMask, decoded) ==
                CodecError::None);
    INPUT_CHECK(decoded.session_id == command.session_id && decoded.client_id == 7 &&
                decoded.sequence == 5 && decoded.target_tick == 12 &&
                decoded.state.edge_flags == kEdgeActiveItem);

    LoopbackTransport transport;
    const EncodedInputCmdV1 original = encoded;
    INPUT_CHECK(transport.Send(encoded.data(), encoded.size()) == TransportSendResult::Sent);
    encoded.fill(0xA5);
    EncodedInputCmdV1 drained{};
    INPUT_CHECK(transport.Drain(&drained, 1, 1) == 1);
    INPUT_CHECK(drained == original);
    INPUT_CHECK(transport.Empty());
    INPUT_CHECK(transport.Send(original.data(), original.size() - 1) ==
                TransportSendResult::InvalidSize);

    for (std::size_t index = 0; index < kLoopbackTransportCapacity; ++index)
        INPUT_CHECK(transport.Send(original.data(), original.size()) == TransportSendResult::Sent);
    INPUT_CHECK(transport.Send(original.data(), original.size()) == TransportSendResult::Full);
    std::array<EncodedInputCmdV1, kDrainBudgetPerTick + 4> batch{};
    INPUT_CHECK(transport.Drain(batch.data(), batch.size()) == kDrainBudgetPerTick);
    INPUT_CHECK(transport.Size() == kLoopbackTransportCapacity - kDrainBudgetPerTick);
    transport.Clear();
    INPUT_CHECK(transport.Empty());
}

void TestSequenceTargetAndRetirement()
{
    auto* queue = new InputQueue(Session(), 7, kBaseValidEdgeMask);
    const InputCmdDraftV1 first = Command(1, 102);
    AcceptResult accepted = queue->Accept(first, 100, TickPhase::DrainBeforeResolve);
    INPUT_CHECK(accepted.decision == AcceptDecision::Accepted && accepted.sequence_gap == 0);
    AcceptResult duplicate = queue->Accept(first, 100, TickPhase::DrainBeforeResolve);
    INPUT_CHECK(duplicate.decision == AcceptDecision::Duplicate && queue->Size() == 1);

    InputCmdDraftV1 gap = Command(4, 103);
    accepted = queue->Accept(gap, 100, TickPhase::DrainBeforeResolve);
    INPUT_CHECK(accepted.decision == AcceptDecision::Accepted && accepted.sequence_gap == 2);
    INPUT_CHECK(queue->Accept(Command(3, 103), 100, TickPhase::DrainBeforeResolve).reason ==
                RejectReason::StaleSequence);
    INPUT_CHECK(queue->Accept(Command(5, 99), 100, TickPhase::DrainBeforeResolve).reason ==
                RejectReason::TargetExpired);
    INPUT_CHECK(queue->Accept(Command(5, 100), 100, TickPhase::Resolved).reason ==
                RejectReason::TargetExpired);
    INPUT_CHECK(queue->Accept(Command(5, 165), 100, TickPhase::DrainBeforeResolve).reason ==
                RejectReason::TargetTooFar);
    INPUT_CHECK(queue->LastAcceptedSequence() == 4);

    InputCmdDraftV1 invalid = Command(5, 104);
    invalid.state.reserved = 1;
    const AcceptResult invalid_result =
        queue->Accept(invalid, 100, TickPhase::DrainBeforeResolve);
    INPUT_CHECK(invalid_result.reason == RejectReason::InvalidCanonical &&
                invalid_result.codec_error == CodecError::ReservedNonzero);
    INPUT_CHECK(queue->LastAcceptedSequence() == 4);
    delete queue;

    queue = new InputQueue(Session(), 7, kBaseValidEdgeMask);
    INPUT_CHECK(queue->Accept(first, 100, TickPhase::DrainBeforeResolve).decision ==
                AcceptDecision::Accepted);
    InputCmdDraftV1 conflict = first;
    conflict.target_tick = 103;
    const AcceptResult conflict_result =
        queue->Accept(conflict, 100, TickPhase::DrainBeforeResolve);
    INPUT_CHECK(conflict_result.decision == AcceptDecision::FatalConflict &&
                conflict_result.reason == RejectReason::SequenceConflict && queue->Retired() &&
                queue->Size() == 1);
    INPUT_CHECK(!queue->Resolve(102).resolved);
    queue->Clear();
    INPUT_CHECK(queue->Size() == 0);
    delete queue;
}

void TestQueueCapacityResolveAndDuplicateAfterApply()
{
    auto* queue = new InputQueue(Session(), 7, kBaseValidEdgeMask);
    for (std::uint64_t sequence = 1; sequence <= kInputQueueCapacity; ++sequence)
    {
        const AcceptResult result =
            queue->Accept(Command(sequence, 102), 100, TickPhase::DrainBeforeResolve);
        INPUT_CHECK(result.decision == AcceptDecision::Accepted);
    }
    const InputCmdDraftV1 overflow = Command(kInputQueueCapacity + 1, 102);
    INPUT_CHECK(queue->Accept(overflow, 100, TickPhase::DrainBeforeResolve).reason ==
                RejectReason::QueueFull);
    INPUT_CHECK(queue->Accept(Command(1, 102), 100, TickPhase::DrainBeforeResolve).decision ==
                AcceptDecision::Duplicate);
    INPUT_CHECK(queue->LastAcceptedSequence() == kInputQueueCapacity);
    const ResolveResult resolved = queue->Resolve(102);
    INPUT_CHECK(resolved.resolved && resolved.has_input);
    INPUT_CHECK(resolved.effective.provenance_count == kInputQueueCapacity);
    INPUT_CHECK(queue->Size() == 0);
    INPUT_CHECK(queue->Accept(overflow, 102, TickPhase::DrainBeforeResolve).decision ==
                AcceptDecision::Accepted);
    INPUT_CHECK(queue->Accept(Command(1, 102), 102, TickPhase::DrainBeforeResolve).decision ==
                AcceptDecision::Duplicate);
    delete queue;
}

void TestTargetBoundariesAndNoLateApply()
{
    auto* queue = new InputQueue(Session(), 7, kBaseValidEdgeMask);
    INPUT_CHECK(queue->Accept(Command(1, 100), 100, TickPhase::DrainBeforeResolve).decision ==
                AcceptDecision::Accepted);
    INPUT_CHECK(queue->Accept(Command(2, 164), 100, TickPhase::DrainBeforeResolve).decision ==
                AcceptDecision::Accepted);
    INPUT_CHECK(queue->Accept(Command(3, 165), 100, TickPhase::DrainBeforeResolve).reason ==
                RejectReason::TargetTooFar);
    const ResolveResult current = queue->Resolve(100);
    INPUT_CHECK(current.resolved && current.has_input && current.expired_commands == 0);
    const ResolveResult late = queue->Resolve(165);
    INPUT_CHECK(late.resolved && !late.has_input && late.expired_commands == 1);
    INPUT_CHECK(queue->Size() == 0);
    delete queue;
}

void TestEdgePlanningCollisionAndMerge()
{
    EdgeTargetPlanner planner(kBaseValidEdgeMask);
    InputStateDraftV1 bomb{};
    bomb.edge_flags = kEdgeBomb;
    std::uint64_t target = 0;
    INPUT_CHECK(planner.Plan(10, bomb, target) == EdgePlanResult::Planned && target == 12);
    INPUT_CHECK(planner.Plan(10, bomb, target) == EdgePlanResult::Planned && target == 13);
    InputStateDraftV1 item{};
    item.edge_flags = kEdgeActiveItem;
    INPUT_CHECK(planner.Plan(10, item, target) == EdgePlanResult::Planned && target == 12);
    InputStateDraftV1 simultaneous{};
    simultaneous.edge_flags = kEdgeBomb | kEdgeDrop;
    INPUT_CHECK(planner.Plan(10, simultaneous, target) == EdgePlanResult::Planned && target == 14);

    auto* queue = new InputQueue(Session(), 7, kBaseValidEdgeMask);
    InputStateDraftV1 left{};
    left.move_x = -1000;
    InputStateDraftV1 right{};
    right.move_x = 1000;
    InputStateDraftV1 right_bomb = right;
    right_bomb.edge_flags = kEdgeBomb;
    InputStateDraftV1 right_item = right;
    right_item.edge_flags = kEdgeActiveItem;
    INPUT_CHECK(queue->Accept(Command(1, 22, left), 20, TickPhase::DrainBeforeResolve).decision ==
                AcceptDecision::Accepted);
    INPUT_CHECK(queue->Accept(Command(2, 22, right_bomb), 20,
                              TickPhase::DrainBeforeResolve).decision == AcceptDecision::Accepted);
    INPUT_CHECK(queue->Accept(Command(3, 22, right_item), 20,
                              TickPhase::DrainBeforeResolve).decision == AcceptDecision::Accepted);
    InputStateDraftV1 second_bomb = right;
    second_bomb.edge_flags = kEdgeBomb;
    const AcceptResult collision =
        queue->Accept(Command(4, 22, second_bomb), 20, TickPhase::DrainBeforeResolve);
    INPUT_CHECK(collision.reason == RejectReason::EdgeTargetConflict &&
                queue->LastAcceptedSequence() == 3);
    const ResolveResult resolved = queue->Resolve(22);
    INPUT_CHECK(resolved.has_input && resolved.effective.provenance_count == 3);
    INPUT_CHECK(resolved.effective.state.move_x == 1000);
    INPUT_CHECK(resolved.effective.state.edge_flags == (kEdgeBomb | kEdgeActiveItem));
    INPUT_CHECK(!queue->Resolve(22).resolved);
    delete queue;
}

void TestWrongIdentityAndSequenceBounds()
{
    auto* queue = new InputQueue(Session(), 7, kBaseValidEdgeMask);
    InputCmdDraftV1 command = Command(1, 2);
    command.session_id = Session(2);
    INPUT_CHECK(queue->Accept(command, 0, TickPhase::DrainBeforeResolve).reason ==
                RejectReason::WrongSession);
    command = Command(1, 2);
    command.client_id = 8;
    INPUT_CHECK(queue->Accept(command, 0, TickPhase::DrainBeforeResolve).reason ==
                RejectReason::WrongClient);
    command = Command(0, 2);
    INPUT_CHECK(queue->Accept(command, 0, TickPhase::DrainBeforeResolve).reason ==
                RejectReason::InvalidSequence);
    command = Command((std::numeric_limits<std::uint64_t>::max)(), 2);
    INPUT_CHECK(queue->Accept(command, 0, TickPhase::DrainBeforeResolve).reason ==
                RejectReason::InvalidSequence);
    delete queue;

    EdgeTargetPlanner planner(kBaseValidEdgeMask);
    std::uint64_t target = 0;
    InputStateDraftV1 neutral{};
    INPUT_CHECK(planner.Plan((std::numeric_limits<std::uint64_t>::max)() - 1, neutral, target) ==
                EdgePlanResult::TargetOverflow);
}

} // namespace

int RunInputCoreTests()
{
    TestPrimitiveActionClassification();
    TestCanonicalCodecAndNormalization();
    TestStrictDecoder();
    TestFrozenGoldenBytes();
    TestCommandCodecAndLoopback();
    TestSequenceTargetAndRetirement();
    TestQueueCapacityResolveAndDuplicateAfterApply();
    TestTargetBoundariesAndNoLateApply();
    TestEdgePlanningCollisionAndMerge();
    TestWrongIdentityAndSequenceBounds();
    if (g_input_failures == 0)
        std::puts("PASS T0-04 input core harness");
    return g_input_failures;
}
