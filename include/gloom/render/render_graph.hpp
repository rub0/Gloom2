#pragma once

#include <cstdint>
#include <expected>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace gloom::render {

struct GraphResource {
    std::uint32_t value{std::numeric_limits<std::uint32_t>::max()};
    [[nodiscard]] bool operator==(const GraphResource&) const noexcept = default;
};

struct GraphPass {
    std::uint32_t value{std::numeric_limits<std::uint32_t>::max()};
    [[nodiscard]] bool operator==(const GraphPass&) const noexcept = default;
};

enum class ResourceKind : std::uint8_t { buffer, texture };
enum class ResourceState : std::uint8_t {
    undefined,
    copy_source,
    copy_destination,
    vertex_buffer,
    index_buffer,
    constant_buffer,
    shader_resource,
    render_target,
    depth_write,
    present,
};
enum class AccessMode : std::uint8_t { read, write, read_write };

struct GraphResourceDesc {
    ResourceKind kind{ResourceKind::texture};
    std::uint64_t size{0};
    std::uint32_t format{0};
    std::uint32_t width{1};
    std::uint32_t height{1};
    bool external{false};
    ResourceState initial_state{ResourceState::undefined};
};

struct ResourceUse {
    GraphResource resource;
    AccessMode access{AccessMode::read};
    ResourceState state{ResourceState::shader_resource};
};

struct ResourceLifetime {
    GraphResource resource;
    std::uint32_t first_pass{0};
    std::uint32_t last_pass{0};
    std::uint32_t alias_slot{0};
};

struct ResourceBarrier {
    GraphResource resource;
    ResourceState before{ResourceState::undefined};
    ResourceState after{ResourceState::undefined};
    GraphPass pass;
};

struct CompiledRenderGraph {
    std::vector<GraphPass> pass_order;
    std::vector<ResourceLifetime> lifetimes;
    std::vector<ResourceBarrier> barriers;
};

class RenderGraph final {
public:
    [[nodiscard]] GraphResource add_resource(std::string name, GraphResourceDesc description);
    [[nodiscard]] GraphPass add_pass(std::string name, std::span<const ResourceUse> resources);
    void add_dependency(GraphPass before, GraphPass after);

    [[nodiscard]] std::expected<CompiledRenderGraph, std::string> compile() const;
    [[nodiscard]] std::string_view pass_name(GraphPass pass) const;
    [[nodiscard]] std::string_view resource_name(GraphResource resource) const;
    void clear() noexcept;

private:
    struct ResourceRecord {
        std::string name;
        GraphResourceDesc description;
    };
    struct PassRecord {
        std::string name;
        std::vector<ResourceUse> resources;
    };

    std::vector<ResourceRecord> resources_;
    std::vector<PassRecord> passes_;
    std::vector<std::pair<GraphPass, GraphPass>> dependencies_;
};

} // namespace gloom::render
