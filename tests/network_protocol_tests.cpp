#include <gloom/network/network_simulator.hpp>
#include <gloom/network/prediction_history.hpp>
#include <gloom/network/protocol.hpp>
#include <gloom/network/snapshot_buffer.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {

void expect(const bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error{message};
    }
}

[[nodiscard]] std::vector<std::byte> bytes(const std::initializer_list<unsigned int> values) {
    std::vector<std::byte> result;
    result.reserve(values.size());
    for (const auto value : values) {
        result.push_back(static_cast<std::byte>(value));
    }
    return result;
}

void test_protocol_round_trip_and_rejection() {
    const gloom::network::ProtocolMessage original{
        .kind = gloom::network::MessageKind::snapshot,
        .flags = gloom::network::MessageFlags::full_snapshot,
        .simulation_tick = 0x0102'0304'0506'0708ULL,
        .sequence = 42,
        .acknowledged_sequence = 39,
        .payload = bytes({0, 1, 127, 128, 255}),
    };
    const auto encoded = gloom::network::encode_message(original);
    expect(encoded.size() == gloom::network::protocol_header_size + original.payload.size(),
           "Protocol encoder emitted an unexpected size");
    const auto decoded = gloom::network::decode_message(encoded);
    expect(decoded.has_value(), "Valid protocol message was rejected");
    expect(decoded->kind == original.kind && decoded->flags == original.flags &&
               decoded->simulation_tick == original.simulation_tick &&
               decoded->sequence == original.sequence &&
               decoded->acknowledged_sequence == original.acknowledged_sequence &&
               decoded->payload == original.payload,
           "Protocol round trip changed message data");

    auto corrupt = encoded;
    corrupt[0] = static_cast<std::byte>('X');
    expect(!gloom::network::decode_message(corrupt), "Invalid protocol magic was accepted");

    corrupt = encoded;
    corrupt[4] = static_cast<std::byte>(0xff);
    expect(!gloom::network::decode_message(corrupt), "Unknown protocol version was accepted");

    corrupt = encoded;
    corrupt[6] = static_cast<std::byte>(0xff);
    expect(!gloom::network::decode_message(corrupt), "Unknown message kind was accepted");

    corrupt = encoded;
    corrupt.pop_back();
    expect(!gloom::network::decode_message(corrupt), "Truncated payload was accepted");
    expect(!gloom::network::decode_message(encoded, original.payload.size() - 1),
           "Configured payload limit was ignored");

    expect(gloom::network::sequence_more_recent(1, 0), "Increasing sequence was not newer");
    expect(!gloom::network::sequence_more_recent(0, 1), "Older sequence was considered newer");
    expect(gloom::network::sequence_more_recent(
               0, std::numeric_limits<std::uint32_t>::max()),
           "Sequence wraparound was not handled");
}

void test_latency_loss_duplication_and_capacity() {
    const auto payload = bytes({1, 2, 3, 4});
    gloom::network::NetworkSimulator latency{{.latency_seconds = 0.1, .random_seed = 7}};
    latency.submit({.flow = 3, .sequence = 9, .payload = payload});
    latency.advance(0.099);
    expect(!latency.receive(), "Packet arrived before configured latency");
    latency.advance(0.001);
    const auto delivered = latency.receive();
    expect(delivered && delivered->flow == 3 && delivered->sequence == 9 &&
               delivered->payload == payload,
           "Delayed packet data changed");

    gloom::network::NetworkSimulator loss{{.loss_probability = 1.0, .random_seed = 7}};
    loss.submit({.payload = payload});
    loss.advance(100.0);
    expect(!loss.receive() && loss.metrics().dropped_by_loss == 1,
           "Certain packet loss did not drop the packet");

    gloom::network::NetworkSimulator duplicate{
        {.duplicate_probability = 1.0, .random_seed = 7}};
    duplicate.submit({.sequence = 1, .payload = payload});
    expect(duplicate.receive().has_value() && duplicate.receive().has_value() &&
               !duplicate.receive().has_value(),
           "Certain duplication did not deliver exactly two copies");
    expect(duplicate.metrics().duplicated_packets == 1,
           "Duplicate counter is incorrect");

    gloom::network::NetworkSimulator capacity{{.latency_seconds = 1.0,
                                                .queue_capacity_packets = 1,
                                                .random_seed = 7}};
    capacity.submit({.sequence = 1, .payload = payload});
    capacity.submit({.sequence = 2, .payload = payload});
    expect(capacity.metrics().dropped_by_queue == 1,
           "Full simulation queue did not drop the excess packet");
}

void test_bandwidth_reordering_and_determinism() {
    const std::vector<std::byte> hundred_bytes(100, static_cast<std::byte>(1));
    gloom::network::NetworkSimulator bandwidth{
        {.bandwidth_bytes_per_second = 100, .random_seed = 11}};
    bandwidth.submit({.sequence = 1, .payload = hundred_bytes});
    bandwidth.submit({.sequence = 2, .payload = hundred_bytes});
    bandwidth.advance(0.999);
    expect(!bandwidth.receive(), "Bandwidth limit delivered a packet too early");
    bandwidth.advance(0.001);
    expect(bandwidth.receive()->sequence == 1,
           "Bandwidth queue did not preserve first transmission");
    expect(!bandwidth.receive(), "Second bandwidth-limited packet arrived with first");
    bandwidth.advance(1.0);
    expect(bandwidth.receive()->sequence == 2,
           "Second bandwidth-limited packet did not arrive on schedule");

    const gloom::network::NetworkSimulationSettings settings{
        .latency_seconds = 0.02,
        .jitter_seconds = 0.01,
        .reorder_probability = 1.0,
        .maximum_reorder_delay_seconds = 0.2,
        .random_seed = 0x1234,
    };
    gloom::network::NetworkSimulator first{settings};
    gloom::network::NetworkSimulator second{settings};
    const auto small_payload = bytes({5});
    for (std::uint64_t sequence = 0; sequence < 64; ++sequence) {
        first.submit({.sequence = sequence, .payload = small_payload});
        second.submit({.sequence = sequence, .payload = small_payload});
    }
    first.advance(1.0);
    second.advance(1.0);
    std::vector<std::uint64_t> first_order;
    std::vector<std::uint64_t> second_order;
    while (const auto packet = first.receive()) {
        first_order.push_back(packet->sequence);
    }
    while (const auto packet = second.receive()) {
        second_order.push_back(packet->sequence);
    }
    expect(first_order == second_order, "Equal seeds did not reproduce packet ordering");
    expect(!std::is_sorted(first_order.begin(), first_order.end()),
           "Reorder simulation did not change packet order");
}

void test_snapshot_interpolation() {
    gloom::network::SnapshotBuffer<double> snapshots{3};
    expect(snapshots.insert({.simulation_tick = 20, .server_time_seconds = 2.0, .state = 20.0}),
           "First snapshot was rejected");
    expect(snapshots.insert({.simulation_tick = 10, .server_time_seconds = 1.0, .state = 10.0}),
           "Out-of-order snapshot was rejected");
    expect(!snapshots.insert({.simulation_tick = 20, .server_time_seconds = 3.0, .state = 30.0}),
           "Duplicate snapshot tick was accepted");
    expect(!snapshots.sample(0.5, [](double left, double right, double alpha) {
               return left + (right - left) * alpha;
           }),
           "Snapshot buffer sampled before its oldest state");
    const auto middle = snapshots.sample(1.25, [](double left, double right, double alpha) {
        return left + (right - left) * alpha;
    });
    expect(middle && middle->mode == gloom::network::SnapshotSampleMode::interpolated &&
               middle->state == 12.5 && middle->alpha == 0.25,
           "Snapshot interpolation produced the wrong state");
    const auto held = snapshots.sample(3.0, [](double left, double right, double alpha) {
        return left + (right - left) * alpha;
    });
    expect(held && held->mode == gloom::network::SnapshotSampleMode::held &&
               held->state == 20.0,
           "Snapshot underrun did not hold the latest authoritative state");

    expect(snapshots.insert({.simulation_tick = 30, .server_time_seconds = 3.0, .state = 30.0}) &&
               snapshots.insert({.simulation_tick = 40, .server_time_seconds = 4.0, .state = 40.0}),
           "New snapshots were unexpectedly rejected");
    expect(snapshots.size() == 3 && !snapshots.sample(1.0, [](double a, double, double) { return a; }),
           "Snapshot capacity did not evict the oldest sample");
}

void test_prediction_reconciliation() {
    struct Input {
        int movement;
    };
    struct State {
        int position;
    };
    gloom::network::PredictionHistory<Input, State> history{4};
    expect(history.record({.sequence = 10, .input = {2}, .predicted_state = {2}}) &&
               history.record({.sequence = 11, .input = {3}, .predicted_state = {5}}) &&
               history.record({.sequence = 12, .input = {-1}, .predicted_state = {4}}),
           "Ordered predicted inputs were rejected");
    expect(!history.record({.sequence = 11, .input = {9}, .predicted_state = {9}}),
           "Old predicted input sequence was accepted");
    const auto reconciled = history.reconcile(State{100}, 10, [](State state, const Input input) {
        state.position += input.movement;
        return state;
    });
    expect(reconciled.position == 102 && history.pending().size() == 2 &&
               history.pending().front().predicted_state.position == 103 &&
               history.pending().back().predicted_state.position == 102,
           "Prediction reconciliation did not replay pending inputs");
}

} // namespace

int main() try {
    test_protocol_round_trip_and_rejection();
    test_latency_loss_duplication_and_capacity();
    test_bandwidth_reordering_and_determinism();
    test_snapshot_interpolation();
    test_prediction_reconciliation();
    std::cout << "Gloom network protocol and simulation tests completed successfully.\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << "Network protocol test failure: " << error.what() << '\n';
    return 1;
}
