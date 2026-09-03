#pragma once

#include <string_view>

namespace gloom::core {

enum class SubsystemState {
    stopped,
    running,
};

class Subsystem {
public:
    virtual ~Subsystem() = default;

    [[nodiscard]] virtual std::string_view name() const noexcept = 0;
    [[nodiscard]] virtual SubsystemState state() const noexcept = 0;

    virtual void start() = 0;
    virtual void tick(double delta_seconds) = 0;
    virtual void stop() noexcept = 0;
};

} // namespace gloom::core

