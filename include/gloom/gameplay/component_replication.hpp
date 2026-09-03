#pragma once

#include <gloom/gameplay/components.hpp>

#include <span>
#include <stdexcept>

namespace gloom::gameplay {

struct ComponentSnapshotApplyResult {
    std::size_t applied{0};
    std::size_t ignored_authoritative{0};
    std::size_t missing{0};
};

[[nodiscard]] inline network::MovementState
movement_state_from_components(const core::EntityRegistry& registry,
                               const core::EntityId entity) {
    const auto* transform = registry.get<TransformComponent>(entity);
    const auto* authority = registry.get<AuthorityComponent>(entity);
    const auto* replication = registry.get<ReplicationComponent>(entity);
    if (transform == nullptr || authority == nullptr || replication == nullptr) {
        throw std::logic_error{"Replicated entity is missing transform, authority or replication"};
    }
    return {.entity = authority->network_entity,
            .simulation_tick = replication->simulation_tick,
            .position_x = transform->position_x,
            .position_y = transform->position_y,
            .position_z = transform->position_z,
            .velocity_x = transform->velocity_x,
            .velocity_y = transform->velocity_y,
            .velocity_z = transform->velocity_z,
            .grounded = replication->grounded};
}

[[nodiscard]] inline network::WorldSnapshot
capture_component_snapshot(const core::EntityRegistry& registry,
                           const std::span<const core::EntityId> entities,
                           const std::uint64_t simulation_tick,
                           const std::uint32_t acknowledged_input = 0) {
    network::WorldSnapshot snapshot{.simulation_tick = simulation_tick,
                                    .acknowledged_input = acknowledged_input};
    snapshot.entities.reserve(entities.size());
    for (const auto entity : entities) {
        auto state = movement_state_from_components(registry, entity);
        state.simulation_tick = simulation_tick;
        snapshot.entities.push_back(state);
    }
    return snapshot;
}

// Applies received state according to component ownership. Server-owned entities
// cannot be overwritten. A predicted owner is reconciled only when explicitly
// requested; interpolated remotes always consume newer snapshots.
[[nodiscard]] inline ComponentSnapshotApplyResult
apply_component_snapshot(core::EntityRegistry& registry,
                         const std::span<const core::EntityId> entities,
                         const network::WorldSnapshot& snapshot,
                         const bool reconcile_predicted_owner = false) {
    ComponentSnapshotApplyResult result;
    for (const auto& state : snapshot.entities) {
        core::EntityId logical;
        for (const auto candidate : entities) {
            const auto* authority = registry.get<AuthorityComponent>(candidate);
            if (authority != nullptr && authority->network_entity == state.entity) {
                logical = candidate;
                break;
            }
        }
        auto* authority = registry.get<AuthorityComponent>(logical);
        auto* transform = registry.get<TransformComponent>(logical);
        auto* replication = registry.get<ReplicationComponent>(logical);
        if (authority == nullptr || transform == nullptr || replication == nullptr) {
            ++result.missing;
            continue;
        }
        if (authority->mode == ComponentAuthority::server ||
            (authority->mode == ComponentAuthority::predicted_owner &&
             !reconcile_predicted_owner)) {
            ++result.ignored_authoritative;
            continue;
        }
        if (snapshot.simulation_tick < replication->last_received_tick) {
            continue;
        }
        *transform = {.position_x = state.position_x,
                      .position_y = state.position_y,
                      .position_z = state.position_z,
                      .velocity_x = state.velocity_x,
                      .velocity_y = state.velocity_y,
                      .velocity_z = state.velocity_z};
        replication->simulation_tick = snapshot.simulation_tick;
        replication->last_received_tick = snapshot.simulation_tick;
        replication->acknowledged_input = snapshot.acknowledged_input;
        replication->grounded = state.grounded;
        ++result.applied;
    }
    return result;
}

} // namespace gloom::gameplay
