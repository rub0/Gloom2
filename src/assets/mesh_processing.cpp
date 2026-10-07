#include <gloom/assets/mesh_processing.hpp>

#include <meshoptimizer.h>
#include <mikktspace.h>

#include <string.h>
#include <math.h>

namespace gloom::assets {
namespace {

struct TangentContext {
    const ImportedPrimitive* primitive{};
    std::vector<std::array<float, 4>> corner_tangents;
};

[[nodiscard]] const ImportedVertex& corner_vertex(const SMikkTSpaceContext* context, const int face, const int corner) {
    const TangentContext& data = *static_cast<const TangentContext*>(context->m_pUserData);
    return data.primitive->vertices[data.primitive->indices[static_cast<size_t>(face) * 3U + static_cast<size_t>(corner)]];
}

int face_count(const SMikkTSpaceContext* context) {
    const TangentContext& data = *static_cast<const TangentContext*>(context->m_pUserData);
    return static_cast<int>(data.primitive->indices.size() / 3U);
}

int vertices_per_face(const SMikkTSpaceContext*, int) {
    return 3;
}

void position(const SMikkTSpaceContext* context, float output[], int face, int corner) {
    memcpy(output, corner_vertex(context, face, corner).position.data(), 3 * sizeof(float));
}

void normal(const SMikkTSpaceContext* context, float output[], int face, int corner) {
    memcpy(output, corner_vertex(context, face, corner).normal.data(), 3 * sizeof(float));
}

void texture_coordinate(const SMikkTSpaceContext* context, float output[], int face, int corner) {
    memcpy(output, corner_vertex(context, face, corner).texture_coordinate.data(), 2 * sizeof(float));
}

void tangent(const SMikkTSpaceContext* context, const float value[], const float orientation, int face, int corner) {
    TangentContext& data = *static_cast<TangentContext*>(context->m_pUserData);
    data.corner_tangents[static_cast<size_t>(face) * 3U + static_cast<size_t>(corner)] = {value[0], value[1], value[2], orientation};
}

[[nodiscard]] bool finite_vertex(const ImportedVertex& vertex) {
    for (float value : vertex.position)
        if (!isfinite(value))
            return false;
    for (float value : vertex.normal)
        if (!isfinite(value))
            return false;
    for (float value : vertex.texture_coordinate)
        if (!isfinite(value))
            return false;
    for (float value : vertex.texture_coordinate_1)
        if (!isfinite(value))
            return false;
    return true;
}

void calculate_bounds(ImportedPrimitive& primitive) {
    std::array<float, 3> minimum = primitive.vertices.front().position;
    std::array<float, 3> maximum = minimum;
    for (const ImportedVertex& vertex : primitive.vertices) {
        for (size_t axis = 0; axis < 3; ++axis) {
            minimum[axis] = fminf(minimum[axis], vertex.position[axis]);
            maximum[axis] = fmaxf(maximum[axis], vertex.position[axis]);
        }
    }
    for (size_t axis = 0; axis < 3; ++axis) {
        primitive.bounds_center[axis] = (minimum[axis] + maximum[axis]) * 0.5F;
    }
    float radius_squared = 0.0F;
    for (const ImportedVertex& vertex : primitive.vertices) {
        float distance_squared = 0.0F;
        for (size_t axis = 0; axis < 3; ++axis) {
            const float difference = vertex.position[axis] - primitive.bounds_center[axis];
            distance_squared += difference * difference;
        }
        radius_squared = fmaxf(radius_squared, distance_squared);
    }
    primitive.bounds_radius = sqrtf(radius_squared);
}

void generate_normals(ImportedPrimitive& primitive) {
    // glTF without NORMAL uses flat shading. Split corners before assigning
    // face normals so hard edges cannot be averaged across a shared vertex.
    std::vector<ImportedVertex> corners;
    corners.reserve(primitive.indices.size());
    for (size_t triangle = 0; triangle < primitive.indices.size(); triangle += 3U) {
        const uint32 first = primitive.indices[triangle];
        const uint32 second = primitive.indices[triangle + 1U];
        const uint32 third = primitive.indices[triangle + 2U];
        const std::array<float, 3>& a = primitive.vertices[first].position;
        const std::array<float, 3>& b = primitive.vertices[second].position;
        const std::array<float, 3>& c = primitive.vertices[third].position;
        const std::array edge_ab{b[0] - a[0], b[1] - a[1], b[2] - a[2]};
        const std::array edge_ac{c[0] - a[0], c[1] - a[1], c[2] - a[2]};
        std::array normal{edge_ab[1] * edge_ac[2] - edge_ab[2] * edge_ac[1], edge_ab[2] * edge_ac[0] - edge_ab[0] * edge_ac[2],
            edge_ab[0] * edge_ac[1] - edge_ab[1] * edge_ac[0]};
        const float magnitude = sqrtf(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
        if (magnitude > 1.0e-7F) {
            for (float& value : normal) {
                value /= magnitude;
            }
        } else {
            normal = {0.0F, 1.0F, 0.0F};
        }
        for (uint32 index : {first, second, third}) {
            ImportedVertex vertex = primitive.vertices[index];
            vertex.normal = normal;
            corners.push_back(vertex);
        }
    }
    primitive.vertices = std::move(corners);
    for (size_t i = 0; i < primitive.indices.size(); ++i)
        primitive.indices[i] = static_cast<uint32>(i);
}

[[nodiscard]] std::array<float, 4> fallback_tangent(const ImportedVertex& vertex) {
    const std::array<float, 3>& normal = vertex.normal;
    const std::array reference = fabsf(normal[1]) < 0.999F ? std::array{0.0F, 1.0F, 0.0F} : std::array{1.0F, 0.0F, 0.0F};
    std::array tangent{reference[1] * normal[2] - reference[2] * normal[1], reference[2] * normal[0] - reference[0] * normal[2],
        reference[0] * normal[1] - reference[1] * normal[0]};
    const float magnitude = sqrtf(tangent[0] * tangent[0] + tangent[1] * tangent[1] + tangent[2] * tangent[2]);
    for (float& value : tangent) {
        value /= magnitude;
    }
    return {tangent[0], tangent[1], tangent[2], 1.0F};
}

} // namespace

std::expected<MeshProcessingMetrics, std::string> process_imported_primitive(ImportedPrimitive& primitive, const MeshProcessingSettings settings) {
    MeshProcessingMetrics metrics{.source_vertices = primitive.vertices.size(), .source_indices = primitive.indices.size()};
    if (primitive.vertices.empty() || primitive.indices.empty() || primitive.indices.size() % 3U != 0)
        return std::unexpected{"Mesh processing requires valid indexed triangles"};
    for (uint32 index : primitive.indices)
        if (index >= primitive.vertices.size())
            return std::unexpected{"Mesh index exceeds its vertex storage"};
    for (const ImportedVertex& vertex : primitive.vertices)
        if (!finite_vertex(vertex))
            return std::unexpected{"Mesh vertex is not finite"};

    if (!settings.source_normals) {
        generate_normals(primitive);
    }

    TangentContext tangent_data{.primitive = &primitive, .corner_tangents = std::vector<std::array<float, 4>>(primitive.indices.size())};
    SMikkTSpaceInterface callbacks{
        .m_getNumFaces = face_count,
        .m_getNumVerticesOfFace = vertices_per_face,
        .m_getPosition = position,
        .m_getNormal = normal,
        .m_getTexCoord = texture_coordinate,
        .m_setTSpaceBasic = tangent,
        .m_setTSpace = nullptr,
    };
    const SMikkTSpaceContext context{.m_pInterface = &callbacks, .m_pUserData = &tangent_data};
    const bool generated_tangents = settings.source_texture_coordinates && genTangSpaceDefault(&context) != 0;
    if (!generated_tangents) {
        for (size_t corner = 0; corner < primitive.indices.size(); ++corner) {
            tangent_data.corner_tangents[corner] = fallback_tangent(primitive.vertices[primitive.indices[corner]]);
        }
    } else {
        for (size_t corner = 0; corner < primitive.indices.size(); ++corner) {
            const std::array<float, 4>& value = tangent_data.corner_tangents[corner];
            const float magnitude_squared = value[0] * value[0] + value[1] * value[1] + value[2] * value[2];
            if (!isfinite(value[0]) || !isfinite(value[1]) || !isfinite(value[2]) || !isfinite(value[3]) || magnitude_squared < 1.0e-10F) {
                tangent_data.corner_tangents[corner] = fallback_tangent(primitive.vertices[primitive.indices[corner]]);
            }
        }
    }

    std::vector<ImportedVertex> corners(primitive.indices.size());
    for (size_t index = 0; index < corners.size(); ++index) {
        corners[index] = primitive.vertices[primitive.indices[index]];
        corners[index].tangent = tangent_data.corner_tangents[index];
    }
    std::vector<unsigned int> remap(corners.size());
    const size_t unique_vertices = meshopt_generateVertexRemap(remap.data(), nullptr, corners.size(), corners.data(), corners.size(), sizeof(ImportedVertex));
    std::vector<ImportedVertex> optimized_vertices(unique_vertices);
    std::vector<uint32> optimized_indices(corners.size());
    meshopt_remapVertexBuffer(optimized_vertices.data(), corners.data(), corners.size(), sizeof(ImportedVertex), remap.data());
    meshopt_remapIndexBuffer(optimized_indices.data(), nullptr, corners.size(), remap.data());
    meshopt_optimizeVertexCache(optimized_indices.data(), optimized_indices.data(), optimized_indices.size(), optimized_vertices.size());
    meshopt_optimizeOverdraw(optimized_indices.data(), optimized_indices.data(), optimized_indices.size(), optimized_vertices.front().position.data(),
        optimized_vertices.size(), sizeof(ImportedVertex), 1.05F);
    const size_t fetched_vertices = meshopt_optimizeVertexFetch(optimized_vertices.data(), optimized_indices.data(), optimized_indices.size(),
        optimized_vertices.data(), optimized_vertices.size(), sizeof(ImportedVertex));
    optimized_vertices.resize(fetched_vertices);
    primitive.vertices = std::move(optimized_vertices);
    primitive.indices = std::move(optimized_indices);
    calculate_bounds(primitive);

    primitive.lod_indices.clear();
    primitive.lod_indices.reserve(2);
    for (const float ratio : {0.5F, 0.2F}) {
        const size_t requested = (static_cast<size_t>(primitive.indices.size() * ratio) / 3U) * 3U;
        const size_t target = requested < 3 ? 3 : requested;
        std::vector<uint32> simplified(primitive.indices.size());
        float result_error = 0.0F;
        const size_t count = meshopt_simplify(simplified.data(), primitive.indices.data(), primitive.indices.size(), primitive.vertices.front().position.data(),
            primitive.vertices.size(), sizeof(ImportedVertex), target, 1.0e-2F, 0, &result_error);
        if (count >= 3U && count < primitive.indices.size() && (primitive.lod_indices.empty() || count < primitive.lod_indices.back().size())) {
            simplified.resize(count);
            meshopt_optimizeVertexCache(simplified.data(), simplified.data(), simplified.size(), primitive.vertices.size());
            metrics.lod_indices += simplified.size();
            primitive.lod_indices.push_back(std::move(simplified));
        }
    }
    metrics.optimized_vertices = primitive.vertices.size();
    return metrics;
}

} // namespace gloom::assets
