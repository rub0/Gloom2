#include <gloom/render/render_graph.hpp>

#include <iostream>
#include <stdexcept>

namespace {

void require(const bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error{message};
    }
}

} // namespace

int main() try {
    using namespace gloom::render;
    RenderGraph graph;
    const auto back_buffer = graph.add_resource(
        "back buffer", {.external = true, .initial_state = ResourceState::present});
    const auto depth = graph.add_resource("depth", {.format = 1, .width = 1280, .height = 720});
    const auto lighting = graph.add_resource("lighting", {.format = 2, .width = 1280, .height = 720});

    const ResourceUse depth_write[]{{depth, AccessMode::write, ResourceState::depth_write}};
    const auto prepass = graph.add_pass("depth prepass", depth_write);
    const ResourceUse shade_uses[]{
        {depth, AccessMode::read, ResourceState::shader_resource},
        {lighting, AccessMode::write, ResourceState::render_target},
    };
    const auto shade = graph.add_pass("opaque", shade_uses);
    const ResourceUse composite_uses[]{
        {lighting, AccessMode::read, ResourceState::shader_resource},
        {back_buffer, AccessMode::write, ResourceState::render_target},
    };
    const auto composite = graph.add_pass("composite", composite_uses);
    graph.add_dependency(prepass, shade);
    graph.add_dependency(shade, composite);

    const auto compiled = graph.compile();
    require(compiled.has_value(), "A valid render graph did not compile");
    require(compiled->pass_order.size() == 3, "Render graph lost passes");
    require(compiled->pass_order[0] == prepass && compiled->pass_order[2] == composite,
            "Render graph order is incorrect");
    require(compiled->lifetimes.size() == 3, "Render graph lifetimes are incomplete");
    require(compiled->barriers.size() >= 5, "Render graph did not schedule state barriers");

    RenderGraph invalid;
    const auto transient = invalid.add_resource("uninitialized", {});
    const ResourceUse read[]{{transient, AccessMode::read, ResourceState::shader_resource}};
    static_cast<void>(invalid.add_pass("bad read", read));
    require(!invalid.compile().has_value(), "Read-before-write was not rejected");

    RenderGraph cyclic;
    const auto first = cyclic.add_pass("first", {});
    const auto second = cyclic.add_pass("second", {});
    cyclic.add_dependency(first, second);
    cyclic.add_dependency(second, first);
    require(!cyclic.compile().has_value(), "Dependency cycle was not rejected");

    RenderGraph aliasing;
    const GraphResourceDesc temporary_description{
        .size = 4096, .format = 7, .width = 32, .height = 32};
    const auto temporary_a = aliasing.add_resource("temporary A", temporary_description);
    const auto temporary_b = aliasing.add_resource("temporary B", temporary_description);
    const ResourceUse write_a[]{
        {temporary_a, AccessMode::write, ResourceState::render_target}};
    const ResourceUse read_a[]{
        {temporary_a, AccessMode::read, ResourceState::shader_resource}};
    const ResourceUse write_b[]{
        {temporary_b, AccessMode::write, ResourceState::render_target}};
    const ResourceUse read_b[]{
        {temporary_b, AccessMode::read, ResourceState::shader_resource}};
    const auto produce_a = aliasing.add_pass("produce A", write_a);
    const auto consume_a = aliasing.add_pass("consume A", read_a);
    const auto produce_b = aliasing.add_pass("produce B", write_b);
    const auto consume_b = aliasing.add_pass("consume B", read_b);
    aliasing.add_dependency(produce_a, consume_a);
    aliasing.add_dependency(consume_a, produce_b);
    aliasing.add_dependency(produce_b, consume_b);
    const auto aliased = aliasing.compile();
    require(aliased.has_value(), "Transient aliasing graph did not compile");
    require(aliased->lifetimes.size() == 2 &&
                aliased->lifetimes[0].alias_slot == aliased->lifetimes[1].alias_slot,
            "Compatible non-overlapping transients did not share an alias slot");

    std::cout << "Gloom render-graph tests completed successfully.\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << "Render-graph test failure: " << error.what() << '\n';
    return 1;
}
