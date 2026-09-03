#include <gloom/core/entity.hpp>

#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

struct TransformComponent {
    float x{0.0F};
    float y{0.0F};
    float z{0.0F};
};

struct HealthComponent {
    float current{100.0F};
};

struct NameComponent {
    explicit NameComponent(std::string value) : value{std::move(value)} {}
    std::string value;
};

void expect(const bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error{message};
    }
}

template <typename Function>
void expect_throws(Function&& function, const char* message) {
    try {
        function();
    } catch (const std::exception&) {
        return;
    }
    throw std::runtime_error{message};
}

} // namespace

int main() try {
    gloom::core::EntityRegistry entities;
    const auto lava = entities.create();
    const auto hound = entities.create();
    expect(lava.valid() && hound.valid() && lava != hound && entities.size() == 2,
           "Entity registry did not create distinct live entities");

    auto& transform = entities.emplace<TransformComponent>(lava, 1.0F, -0.5F, 7.4F);
    entities.emplace<NameComponent>(lava, "FactoryLava");
    entities.emplace<HealthComponent>(hound, 120.0F);
    transform.y = -0.6F;
    expect(entities.has<TransformComponent>(lava) &&
               entities.get<TransformComponent>(lava)->y == -0.6F &&
               entities.get<NameComponent>(lava)->value == "FactoryLava" &&
               entities.component_count<TransformComponent>() == 1,
           "Typed components were not stored or retrieved correctly");
    expect(!entities.has<HealthComponent>(lava) &&
               entities.get<HealthComponent>(lava) == nullptr,
           "Component lookup crossed entity ownership");
    expect_throws([&] { entities.emplace<TransformComponent>(lava); },
                  "Duplicate component insertion was accepted");

    expect(entities.remove<NameComponent>(lava) &&
               !entities.remove<NameComponent>(lava) &&
               !entities.has<NameComponent>(lava),
           "Component removal was not idempotent");

    const auto stale_lava = lava;
    expect(entities.destroy(lava) && !entities.destroy(lava) &&
               !entities.alive(stale_lava) &&
               entities.get<TransformComponent>(stale_lava) == nullptr &&
               entities.component_count<TransformComponent>() == 0,
           "Entity destruction did not remove components or invalidate the handle");
    const auto replacement = entities.create();
    expect(replacement.index == stale_lava.index &&
               replacement.generation != stale_lava.generation &&
               entities.alive(replacement) && !entities.alive(stale_lava),
           "Reused entity slot did not reject a stale generation");
    expect_throws([&] { entities.emplace<HealthComponent>(stale_lava); },
                  "A stale entity handle accepted a component");

    const gloom::core::EntityRegistry& read_only = entities;
    expect(read_only.get<HealthComponent>(hound) != nullptr &&
               read_only.get<HealthComponent>(hound)->current == 120.0F,
           "Const component lookup failed");

    entities.clear();
    expect(entities.size() == 0 && !entities.alive(hound) &&
               entities.component_count<HealthComponent>() == 0,
           "Registry clear did not invalidate entities and components");
    const auto after_clear = entities.create();
    expect(after_clear.valid() && !entities.alive(hound),
           "Registry reuse after clear revived a stale entity handle");

    std::cout << "Gloom entity/component tests completed successfully.\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << "Entity/component test failure: " << error.what() << '\n';
    return 1;
}
