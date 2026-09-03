#include <gloom/backends/placeholder.hpp>

#include <utility>

namespace gloom::backends {

PlaceholderSubsystem::PlaceholderSubsystem(std::string name) : name_{std::move(name)} {}

std::string_view PlaceholderSubsystem::name() const noexcept {
    return name_;
}

core::SubsystemState PlaceholderSubsystem::state() const noexcept {
    return state_;
}

void PlaceholderSubsystem::start() {
    state_ = core::SubsystemState::running;
}

void PlaceholderSubsystem::tick([[maybe_unused]] const double delta_seconds) {}

void PlaceholderSubsystem::stop() noexcept {
    state_ = core::SubsystemState::stopped;
}

} // namespace gloom::backends

