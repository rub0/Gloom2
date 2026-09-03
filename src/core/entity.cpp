#include <gloom/core/entity.hpp>

namespace gloom::core {
namespace {

void advance_generation(std::uint32_t& generation) noexcept {
    ++generation;
    if (generation == 0) {
        generation = 1;
    }
}

} // namespace

EntityId EntityRegistry::create() {
    std::uint32_t index = 0;
    if (free_indices_.empty()) {
        if (slots_.size() >= EntityId::invalid_index) {
            throw std::overflow_error{"Entity registry exhausted its identifier space"};
        }
        index = static_cast<std::uint32_t>(slots_.size());
        slots_.push_back({});
    } else {
        index = free_indices_.back();
        free_indices_.pop_back();
    }
    auto& slot = slots_[index];
    slot.alive = true;
    ++live_entities_;
    return {.index = index, .generation = slot.generation};
}

bool EntityRegistry::destroy(const EntityId entity) {
    if (!alive(entity)) {
        return false;
    }
    free_indices_.reserve(free_indices_.size() + 1);
    for (auto& [type, components] : component_pools_) {
        static_cast<void>(type);
        components->erase(entity.index);
    }
    auto& slot = slots_[entity.index];
    slot.alive = false;
    advance_generation(slot.generation);
    free_indices_.push_back(entity.index);
    --live_entities_;
    return true;
}

bool EntityRegistry::alive(const EntityId entity) const noexcept {
    return entity.valid() && entity.index < slots_.size() &&
           slots_[entity.index].alive &&
           slots_[entity.index].generation == entity.generation;
}

std::size_t EntityRegistry::size() const noexcept { return live_entities_; }

void EntityRegistry::clear() {
    std::vector<std::uint32_t> free_indices;
    free_indices.reserve(slots_.size());
    for (std::size_t index = 0; index < slots_.size(); ++index) {
        free_indices.push_back(static_cast<std::uint32_t>(index));
    }
    for (auto& [type, components] : component_pools_) {
        static_cast<void>(type);
        components->clear();
    }
    for (std::size_t index = 0; index < slots_.size(); ++index) {
        auto& slot = slots_[index];
        if (slot.alive) {
            advance_generation(slot.generation);
        }
        slot.alive = false;
    }
    free_indices_.swap(free_indices);
    live_entities_ = 0;
}

void EntityRegistry::require_alive(const EntityId entity) const {
    if (!alive(entity)) {
        throw std::invalid_argument{"Component operation requires a live entity"};
    }
}

} // namespace gloom::core
