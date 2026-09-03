#pragma once

#include <gloom/core/subsystem.hpp>

#include <memory>
#include <vector>

namespace gloom::core {

class Engine final {
public:
    void add(std::unique_ptr<Subsystem> subsystem);
    void start();
    void tick(double delta_seconds);
    void stop() noexcept;

    [[nodiscard]] bool running() const noexcept;

private:
    std::vector<std::unique_ptr<Subsystem>> subsystems_;
    bool running_{false};
};

} // namespace gloom::core

