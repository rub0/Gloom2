#include <gloom/core/array.hpp>
#include <gloom/render/lighting.hpp>
#include <stdio.h>
#include <stdlib.h>

using namespace gloom;

static uint64 allocations = 0;
void* operator new(size_t size) {
    ++allocations;
    return malloc(size ? size : 1);
}
void* operator new[](size_t size) {
    return ::operator new(size);
}
void operator delete(void* value) noexcept {
    free(value);
}
void operator delete[](void* value) noexcept {
    free(value);
}
void operator delete(void* value, size_t) noexcept {
    free(value);
}
void operator delete[](void* value, size_t) noexcept {
    free(value);
}

namespace {
void require(bool condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(1);
    }
}

uint32 sum(Span<const uint32> values) {
    uint32 total = 0;
    for (uint32 value : values)
        total += value;
    return total;
}

struct Resource {
    uint32* live{nullptr};
    Resource() = default;
    Resource(const Resource&) = delete;
    Resource& operator=(const Resource&) = delete;
    Resource& operator=(Resource&& other) noexcept {
        if (live)
            --*live;
        live = other.live;
        other.live = nullptr;
        return *this;
    }
    ~Resource() {
        if (live)
            --*live;
    }
};
} // namespace

int main() {
    require(sum({1, 2, 3}) == 6, "Initializer-list Span did not survive its call");
    uint32 values[]{4, 5};
    Span<uint32> mutable_view{values};
    mutable_view[0] = 6;
    require(sum(mutable_view) == 11, "Span conversion copied or lost array data");
    require(Span<const uint32>{}.begin() == Span<const uint32>{}.end(), "Empty Span has an invalid range");
    Array<uint32> array;
    array.reserve(4);
    array.resize(4);
    array[0] = 7;
    const uint64 warmed_allocations = allocations;
    const uint32* storage = array.data();
    for (uint32 frame = 0; frame < 1000; ++frame) {
        array.resize(1);
        array.reserve(4);
        array.resize(4);
        require(array[0] == 7 && array[3] == 0, "Array resize lost live values or exposed stale data");
    }
    require(allocations == warmed_allocations && array.data() == storage, "Warmed array allocated storage");
    array.reserve(9);
    require(allocations > warmed_allocations, "Allocation counter did not detect growth");
    require(array[0] == 7 && array.size() == 4 && array.capacity() >= 9, "Array growth lost values");
    uint32 live = 0;
    {
        Array<Resource> owners;
        owners.reserve(2);
        owners.resize(2);
        owners[0].live = &live;
        owners[1].live = &live;
        live = 2;
        owners.reserve(8);
        require(live == 2, "Growth copied or destroyed owned resources");
        owners.resize(1);
        require(live == 1, "Shrinking retained a removed resource");
        owners.resize(2);
        require(live == 1 && owners[1].live == nullptr, "Regrowth revived a removed resource");
    }
    require(live == 0, "Array destruction leaked owned resources");
    render::ClusteredLightingBuilder lighting;
    render::PreparedLighting result;
    const render::Camera camera{.position = {}, .target = {.z = 1}, .far_plane = 100};
    const render::PointLight lights[3]{{.range = 1000}, {.range = 1000}, {.range = 1000}};
    lighting.build(result, camera, 1, lights);
    const uint64 lighting_allocations = allocations;
    for (uint32 frame = 0; frame < 1000; ++frame)
        lighting.build(result, camera, 1, lights);
    require(allocations == lighting_allocations, "Warmed lighting allocated heap storage");
    puts("Gloom storage tests completed successfully: 1000 warmed frames, zero allocations.");
    return 0;
}
