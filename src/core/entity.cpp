#include <gloom/core/entity.hpp>

namespace gloom::core {
namespace {
void advance_generation(uint32& generation) noexcept {
    if (++generation == 0)
        generation = 1;
}
}
EntityRegistry::~EntityRegistry() {
    for (Pool& pool : pools_)
        if (pool.clear)
            pool.clear(pool, true);
}
void EntityRegistry::reserve(size_t entities) {
    assert(entities < EntityId::invalid_index);
    slots_.reserve(entities);
    free_indices_.reserve(slots_.capacity());
}
EntityId EntityRegistry::create() {
    GLOOM_PROFILE_SCOPE(::gloom::AllocationPhase::entities);
    uint32 index;
    if (free_indices_.size()) {
        index = free_indices_[free_indices_.size() - 1];
        free_indices_.resize(free_indices_.size() - 1);
    } else {
        assert(slots_.size() < EntityId::invalid_index);
        index = static_cast<uint32>(slots_.size());
        if (slots_.size() == slots_.capacity())
            reserve(slots_.size() + 1);
        slots_.push_back({});
    }
    slots_[index].alive = true;
    ++live_entities_;
    GLOOM_PROFILE_CAPACITY("entity_slots", slots_.size(), slots_.capacity(), sizeof(Slot));
    return {.index = index, .generation = slots_[index].generation};
}
bool EntityRegistry::destroy(EntityId entity) noexcept {
    GLOOM_PROFILE_SCOPE(::gloom::AllocationPhase::entities);
    if (!alive(entity))
        return false;
    for (Pool& pool : pools_)
        if (pool.erase)
            pool.erase(pool, entity.index);
    slots_[entity.index].alive = false;
    advance_generation(slots_[entity.index].generation);
    free_indices_.push_back(entity.index);
    --live_entities_;
    return true;
}
bool EntityRegistry::alive(EntityId entity) const noexcept {
    return entity.valid() && entity.index < slots_.size() && slots_[entity.index].alive && slots_[entity.index].generation == entity.generation;
}
size_t EntityRegistry::size() const noexcept {
    return live_entities_;
}
void EntityRegistry::clear() noexcept {
    GLOOM_PROFILE_SCOPE(::gloom::AllocationPhase::entities);
    for (Pool& pool : pools_)
        if (pool.clear)
            pool.clear(pool, false);
    free_indices_.resize(0);
    for (uint32 index = 0; index < slots_.size(); ++index) {
        if (slots_[index].alive)
            advance_generation(slots_[index].generation);
        slots_[index].alive = false;
        free_indices_.push_back(index);
    }
    live_entities_ = 0;
}
} // namespace gloom::core
