#include <gloom/backends/placeholder.hpp>
#include <gloom/core/engine.hpp>
#include <gloom/platform/input.hpp>

#include <iostream>
#include <memory>
#include <stdexcept>

namespace {

void expect(const bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error{message};
    }
}

} // namespace

int main() try {
    using gloom::backends::PlaceholderSubsystem;
    using gloom::core::SubsystemState;

    const auto idle_axes = gloom::platform::movement_axes({});
    expect(idle_axes.x == 0.0F && idle_axes.z == 0.0F, "Idle input produced movement");

    const auto forward_right = gloom::platform::movement_axes(
        {.move_right = true, .move_forward = true});
    expect(forward_right.x == 1.0F && forward_right.z == -1.0F,
           "Forward/right input axes were mapped incorrectly");

    const auto cancelled = gloom::platform::movement_axes(
        {.move_left = true,
         .move_right = true,
         .move_forward = true,
         .move_backward = true});
    expect(cancelled.x == 0.0F && cancelled.z == 0.0F,
           "Opposing input directions did not cancel");

    gloom::core::Engine engine;
    auto platform = std::make_unique<PlaceholderSubsystem>("platform");
    auto renderer = std::make_unique<PlaceholderSubsystem>("renderer");
    const auto* platform_view = platform.get();
    const auto* renderer_view = renderer.get();

    engine.add(std::move(platform));
    engine.add(std::move(renderer));
    engine.start();

    expect(engine.running(), "Engine did not start");
    expect(platform_view->state() == SubsystemState::running, "Platform did not start");
    expect(renderer_view->state() == SubsystemState::running, "Renderer did not start");

    engine.tick(1.0 / 60.0);
    engine.stop();

    expect(!engine.running(), "Engine did not stop");
    expect(platform_view->state() == SubsystemState::stopped, "Platform did not stop");
    expect(renderer_view->state() == SubsystemState::stopped, "Renderer did not stop");

    std::cout << "Gloom C++ unit tests completed successfully.\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << "Test failure: " << error.what() << '\n';
    return 1;
}
