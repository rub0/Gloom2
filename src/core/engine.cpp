#include <gloom/core/engine.hpp>

#include <stdexcept>
#include <utility>

namespace gloom::core {

void Engine::add(std::unique_ptr<Subsystem> subsystem) {
    if (running_) {
        throw std::logic_error{"Cannot add a subsystem while the engine is running"};
    }
    if (!subsystem) {
        throw std::invalid_argument{"Cannot add a null subsystem"};
    }
    subsystems_.push_back(std::move(subsystem));
}

void Engine::start() {
    if (running_) {
        throw std::logic_error{"Engine is already running"};
    }

    std::size_t started = 0;
    try {
        for (auto& subsystem : subsystems_) {
            subsystem->start();
            ++started;
        }
        running_ = true;
    } catch (...) {
        while (started > 0) {
            subsystems_[--started]->stop();
        }
        throw;
    }
}

void Engine::tick(const double delta_seconds) {
    if (!running_) {
        throw std::logic_error{"Engine must be running before tick"};
    }
    if (delta_seconds < 0.0) {
        throw std::invalid_argument{"Delta time cannot be negative"};
    }

    for (auto& subsystem : subsystems_) {
        subsystem->tick(delta_seconds);
    }
}

void Engine::stop() noexcept {
    for (auto subsystem = subsystems_.rbegin(); subsystem != subsystems_.rend(); ++subsystem) {
        (*subsystem)->stop();
    }
    running_ = false;
}

bool Engine::running() const noexcept {
    return running_;
}

} // namespace gloom::core

