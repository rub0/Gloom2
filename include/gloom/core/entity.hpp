#pragma once

#include <cstddef>
#include <concepts>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <typeindex>
#include <typeinfo>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace gloom::core {

struct EntityId {
    static constexpr std::uint32_t invalid_index =
        std::numeric_limits<std::uint32_t>::max();

    std::uint32_t index{invalid_index};
    std::uint32_t generation{0};

    [[nodiscard]] constexpr bool valid() const noexcept {
        return index != invalid_index && generation != 0;
    }

    friend constexpr bool operator==(EntityId, EntityId) = default;
};

// Single-owner logical entity/component storage. Components are ordinary Gloom
// values; backend handles belong inside adapter components introduced by the
// owning subsystem rather than in this registry.
class EntityRegistry final {
public:
    [[nodiscard]] EntityId create();
    [[nodiscard]] bool destroy(EntityId entity);
    [[nodiscard]] bool alive(EntityId entity) const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;
    void clear();

    template <typename Component, typename... Arguments>
    Component& emplace(EntityId entity, Arguments&&... arguments) {
        using Value = std::remove_cvref_t<Component>;
        static_assert(std::same_as<Component, Value>,
                      "Component type must be an unqualified value type");
        require_alive(entity);
        auto& values = pool<Value>().values;
        const auto [iterator, inserted] = values.try_emplace(
            entity.index, std::forward<Arguments>(arguments)...);
        if (!inserted) {
            throw std::logic_error{"Entity already owns this component type"};
        }
        return iterator->second;
    }

    template <typename Component>
    [[nodiscard]] bool remove(EntityId entity) noexcept {
        using Value = std::remove_cvref_t<Component>;
        if (!alive(entity)) {
            return false;
        }
        auto* values = find_pool<Value>();
        return values != nullptr && values->values.erase(entity.index) != 0;
    }

    template <typename Component>
    [[nodiscard]] Component* get(EntityId entity) noexcept {
        using Value = std::remove_cvref_t<Component>;
        if (!alive(entity)) {
            return nullptr;
        }
        auto* values = find_pool<Value>();
        if (values == nullptr) {
            return nullptr;
        }
        const auto found = values->values.find(entity.index);
        return found == values->values.end() ? nullptr : &found->second;
    }

    template <typename Component>
    [[nodiscard]] const Component* get(EntityId entity) const noexcept {
        using Value = std::remove_cvref_t<Component>;
        if (!alive(entity)) {
            return nullptr;
        }
        const auto* values = find_pool<Value>();
        if (values == nullptr) {
            return nullptr;
        }
        const auto found = values->values.find(entity.index);
        return found == values->values.end() ? nullptr : &found->second;
    }

    template <typename Component>
    [[nodiscard]] bool has(EntityId entity) const noexcept {
        return get<Component>(entity) != nullptr;
    }

    template <typename Component>
    [[nodiscard]] std::size_t component_count() const noexcept {
        using Value = std::remove_cvref_t<Component>;
        const auto* values = find_pool<Value>();
        return values == nullptr ? 0 : values->values.size();
    }

private:
    struct Slot {
        std::uint32_t generation{1};
        bool alive{false};
    };

    class ComponentPoolBase {
    public:
        virtual ~ComponentPoolBase() = default;
        virtual void erase(std::uint32_t entity_index) noexcept = 0;
        virtual void clear() noexcept = 0;
    };

    template <typename Component>
    class ComponentPool final : public ComponentPoolBase {
    public:
        void erase(const std::uint32_t entity_index) noexcept override {
            values.erase(entity_index);
        }

        void clear() noexcept override { values.clear(); }

        std::unordered_map<std::uint32_t, Component> values;
    };

    void require_alive(EntityId entity) const;

    template <typename Component>
    ComponentPool<Component>& pool() {
        const std::type_index type{typeid(Component)};
        const auto found = component_pools_.find(type);
        if (found != component_pools_.end()) {
            return static_cast<ComponentPool<Component>&>(*found->second);
        }
        auto storage = std::make_unique<ComponentPool<Component>>();
        auto* result = storage.get();
        component_pools_.emplace(type, std::move(storage));
        return *result;
    }

    template <typename Component>
    [[nodiscard]] ComponentPool<Component>* find_pool() noexcept {
        const auto found = component_pools_.find(std::type_index{typeid(Component)});
        return found == component_pools_.end()
                   ? nullptr
                   : static_cast<ComponentPool<Component>*>(found->second.get());
    }

    template <typename Component>
    [[nodiscard]] const ComponentPool<Component>* find_pool() const noexcept {
        const auto found = component_pools_.find(std::type_index{typeid(Component)});
        return found == component_pools_.end()
                   ? nullptr
                   : static_cast<const ComponentPool<Component>*>(found->second.get());
    }

    std::vector<Slot> slots_;
    std::vector<std::uint32_t> free_indices_;
    std::unordered_map<std::type_index, std::unique_ptr<ComponentPoolBase>> component_pools_;
    std::size_t live_entities_{0};
};

} // namespace gloom::core
