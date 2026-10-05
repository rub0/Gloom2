#pragma once
#include <gloom/core/allocation_profile.hpp>
#include <gloom/core/array.hpp>
#include <gloom/core/fixed_function.hpp>

namespace gloom::core {
struct EntityId {
    static constexpr uint32 invalid_index = 0xffffffffU;
    uint32 index{invalid_index};
    uint32 generation{0};
    [[nodiscard]] constexpr bool valid() const noexcept {
        return index != invalid_index && generation != 0;
    }
    friend constexpr bool operator==(EntityId, EntityId) = default;
};
// Explicit IDs for the types used in Gloom. Each ID is unique within a registry.
enum class ComponentType : uint32 {
    damage_volume,
    transform,
    character_physics,
    health,
    shield,
    movement,
    weapon,
    ability,
    loadout,
    authority,
    replication,
    presentation,
    score,
    kinematic_motion,
    static_body,
    dynamic_body,
    kinematic_body,
    trigger,
    count
};
// Single owner, externally synchronized. Components declare static component_id.
// Unqualified value types with noexcept destructors; their addresses survive growth
// and other removals, expiring only on their remove/destroy/clear or registry destruction.
// Copy/move is disabled. Slot reuse never revives an old EntityId.
class EntityRegistry {
  public:
    EntityRegistry() = default;
    ~EntityRegistry();
    EntityRegistry(const EntityRegistry&) = delete;
    EntityRegistry& operator=(const EntityRegistry&) = delete;
    void reserve(size_t entities);
    [[nodiscard]] EntityId create();
    [[nodiscard]] bool destroy(EntityId entity) noexcept;
    [[nodiscard]] bool alive(EntityId entity) const noexcept;
    [[nodiscard]] size_t size() const noexcept;
    void clear() noexcept;
    // Live entity and absent component are preconditions. Cold pages stay for reuse.
    template <typename Component, typename... Arguments> Component& emplace(EntityId entity, Arguments&&... arguments) {
        GLOOM_PROFILE_SCOPE(::gloom::AllocationPhase::entities);
        GLOOM_PROFILE_LAYOUT(__FUNCSIG__, sizeof(Component), alignof(Component));
        static_assert(noexcept(static_cast<Component*>(nullptr)->~Component()), "Component destruction must not throw");
        assert(alive(entity));
        Pool& pool = pools_[type<Component>()];
        assert(!pool.erase || pool.erase == erase<Component>);
        pool.erase = erase<Component>;
        pool.clear = clear_pool<Component>;
        if (pool.pages.size() <= entity.index / 8) {
            pool.pages.reserve(entity.index / 8 + 1);
            pool.pages.resize(entity.index / 8 + 1);
        }
        if (!pool.pages[entity.index / 8])
            pool.pages[entity.index / 8] = new Page<Component>;
        Page<Component>& page = *static_cast<Page<Component>*>(pool.pages[entity.index / 8]);
        assert(!(page.present & (1U << (entity.index % 8))));
        Component* value = new (page.storage + (entity.index % 8) * sizeof(Component), InlineStorage::at) Component(static_cast<Arguments&&>(arguments)...);
        page.present |= 1U << (entity.index % 8);
        ++pool.count;
        return *value;
    }
    template <typename Component> [[nodiscard]] bool remove(EntityId entity) noexcept {
        if (!get<Component>(entity))
            return false;
        Pool& pool = pools_[type<Component>()];
        pool.erase(pool, entity.index);
        return true;
    }
    template <typename Component> [[nodiscard]] Component* get(EntityId entity) noexcept {
        return const_cast<Component*>(static_cast<const EntityRegistry*>(this)->get<Component>(entity));
    }
    template <typename Component> [[nodiscard]] const Component* get(EntityId entity) const noexcept {
        GLOOM_PROFILE_SCOPE(::gloom::AllocationPhase::entities);
        if (!alive(entity))
            return nullptr;
        const Pool& pool = pools_[type<Component>()];
        assert(!pool.erase || pool.erase == erase<Component>);
        if (entity.index / 8 >= pool.pages.size() || !pool.pages[entity.index / 8])
            return nullptr;
        const Page<Component>& page = *static_cast<const Page<Component>*>(pool.pages[entity.index / 8]);
        return page.present & (1U << (entity.index % 8)) ? reinterpret_cast<const Component*>(page.storage + (entity.index % 8) * sizeof(Component)) : nullptr;
    }
    template <typename Component> [[nodiscard]] bool has(EntityId entity) const noexcept {
        return get<Component>(entity) != nullptr;
    }
    template <typename Component> [[nodiscard]] size_t component_count() const noexcept {
        const Pool& pool = pools_[type<Component>()];
        assert(!pool.erase || pool.erase == erase<Component>);
        return pool.count;
    }

  private:
    struct Slot {
        uint32 generation{1};
        bool alive{false};
    };
    template <typename Component> struct Page {
        alignas(Component) uint8 storage[8 * sizeof(Component)];
        uint32 present{0};
    };
    struct Pool {
        Array<void*> pages;
        void (*erase)(Pool&, uint32) noexcept {nullptr};
        void (*clear)(Pool&, bool) noexcept {nullptr};
        size_t count{0};
    };
    template <typename Component> static constexpr size_t type() {
        static_assert(static_cast<size_t>(Component::component_id) < static_cast<size_t>(ComponentType::count), "Component ID is not registered");
        return static_cast<size_t>(Component::component_id);
    }
    template <typename Component> static void erase(Pool& pool, uint32 index) noexcept {
        if (index / 8 >= pool.pages.size() || !pool.pages[index / 8])
            return;
        Page<Component>& page = *static_cast<Page<Component>*>(pool.pages[index / 8]);
        if (!(page.present & (1U << (index % 8))))
            return;
        reinterpret_cast<Component*>(page.storage + (index % 8) * sizeof(Component))->~Component();
        page.present &= ~(1U << (index % 8));
        --pool.count;
    }
    template <typename Component> static void clear_pool(Pool& pool, bool release) noexcept {
        for (void* storage : pool.pages) {
            if (!storage)
                continue;
            Page<Component>& page = *static_cast<Page<Component>*>(storage);
            for (uint32 index = 0; index < 8; ++index)
                if (page.present & (1U << index))
                    reinterpret_cast<Component*>(page.storage + index * sizeof(Component))->~Component();
            page.present = 0;
            if (release)
                delete &page;
        }
        pool.count = 0;
    }
    Array<Slot> slots_;
    Array<uint32> free_indices_;
    Pool pools_[static_cast<size_t>(ComponentType::count)];
    size_t live_entities_{0};
};
} // namespace gloom::core
