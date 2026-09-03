#include <gloom/render/render_graph.hpp>

#include <algorithm>
#include <deque>
#include <stdexcept>

namespace gloom::render {
namespace {

[[nodiscard]] bool writes(const AccessMode mode) noexcept {
    return mode == AccessMode::write || mode == AccessMode::read_write;
}

[[nodiscard]] bool compatible(const GraphResourceDesc& left,
                              const GraphResourceDesc& right) noexcept {
    return left.kind == right.kind && left.size == right.size && left.format == right.format &&
           left.width == right.width && left.height == right.height;
}

} // namespace

GraphResource RenderGraph::add_resource(std::string name, const GraphResourceDesc description) {
    const GraphResource result{static_cast<std::uint32_t>(resources_.size())};
    resources_.push_back({std::move(name), description});
    return result;
}

GraphPass RenderGraph::add_pass(std::string name, const std::span<const ResourceUse> resources) {
    const GraphPass result{static_cast<std::uint32_t>(passes_.size())};
    passes_.push_back({std::move(name), {resources.begin(), resources.end()}});
    return result;
}

void RenderGraph::add_dependency(const GraphPass before, const GraphPass after) {
    dependencies_.emplace_back(before, after);
}

std::expected<CompiledRenderGraph, std::string> RenderGraph::compile() const {
    std::vector<std::vector<std::uint32_t>> edges(passes_.size());
    std::vector<std::uint32_t> incoming(passes_.size());
    const auto add_edge = [&](const std::uint32_t before, const std::uint32_t after) {
        if (before == after || std::ranges::find(edges[before], after) != edges[before].end()) {
            return;
        }
        edges[before].push_back(after);
        ++incoming[after];
    };

    for (const auto [before, after] : dependencies_) {
        if (before.value >= passes_.size() || after.value >= passes_.size()) {
            return std::unexpected{"Render graph dependency references an invalid pass"};
        }
        add_edge(before.value, after.value);
    }

    struct PreviousUse {
        std::uint32_t pass;
        AccessMode access;
    };
    std::vector<std::vector<PreviousUse>> previous(resources_.size());
    for (std::uint32_t pass = 0; pass < passes_.size(); ++pass) {
        for (const auto& use : passes_[pass].resources) {
            if (use.resource.value >= resources_.size()) {
                return std::unexpected{"Render graph pass references an invalid resource"};
            }
            auto& uses = previous[use.resource.value];
            if (uses.empty() && !resources_[use.resource.value].description.external &&
                use.access == AccessMode::read) {
                return std::unexpected{"Transient resource '" +
                                       resources_[use.resource.value].name +
                                       "' is read before it is written"};
            }
            for (const auto prior : uses) {
                if (writes(prior.access) || writes(use.access)) {
                    add_edge(prior.pass, pass);
                }
            }
            uses.push_back({pass, use.access});
        }
    }

    std::deque<std::uint32_t> ready;
    for (std::uint32_t pass = 0; pass < incoming.size(); ++pass) {
        if (incoming[pass] == 0) {
            ready.push_back(pass);
        }
    }
    CompiledRenderGraph result;
    while (!ready.empty()) {
        const auto pass = ready.front();
        ready.pop_front();
        result.pass_order.push_back({pass});
        for (const auto dependent : edges[pass]) {
            if (--incoming[dependent] == 0) {
                ready.push_back(dependent);
            }
        }
    }
    if (result.pass_order.size() != passes_.size()) {
        return std::unexpected{"Render graph contains a dependency cycle"};
    }

    std::vector<std::uint32_t> order_index(passes_.size());
    for (std::uint32_t index = 0; index < result.pass_order.size(); ++index) {
        order_index[result.pass_order[index].value] = index;
    }
    for (std::uint32_t resource = 0; resource < resources_.size(); ++resource) {
        std::uint32_t first = std::numeric_limits<std::uint32_t>::max();
        std::uint32_t last = 0;
        for (std::uint32_t pass = 0; pass < passes_.size(); ++pass) {
            if (std::ranges::any_of(passes_[pass].resources, [&](const ResourceUse& use) {
                    return use.resource.value == resource;
                })) {
                first = std::min(first, order_index[pass]);
                last = std::max(last, order_index[pass]);
            }
        }
        if (first == std::numeric_limits<std::uint32_t>::max()) {
            continue;
        }
        std::uint32_t slot = resource;
        if (!resources_[resource].description.external) {
            slot = 0;
            for (;; ++slot) {
                const bool collision = std::ranges::any_of(
                    result.lifetimes, [&](const ResourceLifetime& lifetime) {
                        if (lifetime.alias_slot != slot ||
                            resources_[lifetime.resource.value].description.external) {
                            return false;
                        }
                        if (!compatible(resources_[lifetime.resource.value].description,
                                        resources_[resource].description)) {
                            return true;
                        }
                        return !(last < lifetime.first_pass || first > lifetime.last_pass);
                    });
                if (!collision) {
                    break;
                }
            }
        }
        result.lifetimes.push_back({{resource}, first, last, slot});
    }

    std::vector<ResourceState> states;
    states.reserve(resources_.size());
    for (const auto& resource : resources_) {
        states.push_back(resource.description.initial_state);
    }
    for (const auto pass : result.pass_order) {
        for (const auto& use : passes_[pass.value].resources) {
            auto& current = states[use.resource.value];
            if (current != use.state) {
                result.barriers.push_back({use.resource, current, use.state, pass});
                current = use.state;
            }
        }
    }
    return result;
}

std::string_view RenderGraph::pass_name(const GraphPass pass) const {
    if (pass.value >= passes_.size()) {
        throw std::out_of_range{"Invalid render graph pass"};
    }
    return passes_[pass.value].name;
}

std::string_view RenderGraph::resource_name(const GraphResource resource) const {
    if (resource.value >= resources_.size()) {
        throw std::out_of_range{"Invalid render graph resource"};
    }
    return resources_[resource.value].name;
}

void RenderGraph::clear() noexcept {
    resources_.clear();
    passes_.clear();
    dependencies_.clear();
}

} // namespace gloom::render
