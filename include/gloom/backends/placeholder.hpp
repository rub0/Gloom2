#pragma once

#include <gloom/core/subsystem.hpp>

#include <string>

namespace gloom::backends {

class PlaceholderSubsystem final : public core::Subsystem {
public:
    explicit PlaceholderSubsystem(std::string name);

    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] core::SubsystemState state() const noexcept override;

    void start() override;
    void tick(double delta_seconds) override;
    void stop() noexcept override;

private:
    std::string name_;
    core::SubsystemState state_{core::SubsystemState::stopped};
};

} // namespace gloom::backends

