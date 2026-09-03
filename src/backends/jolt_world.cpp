#include <gloom/backends/jolt_world.hpp>

#include <Jolt/Jolt.h>

#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

#include <algorithm>
#include <cmath>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace gloom::backends {
namespace {

namespace object_layers {
constexpr JPH::ObjectLayer non_moving = 0;
constexpr JPH::ObjectLayer moving = 1;
constexpr JPH::ObjectLayer count = 2;
} // namespace object_layers

namespace broad_phase_layers {
const JPH::BroadPhaseLayer non_moving{0};
const JPH::BroadPhaseLayer moving{1};
constexpr std::uint32_t count = 2;
} // namespace broad_phase_layers

class BroadPhaseLayers final : public JPH::BroadPhaseLayerInterface {
public:
    [[nodiscard]] JPH::uint GetNumBroadPhaseLayers() const override {
        return broad_phase_layers::count;
    }

    [[nodiscard]] JPH::BroadPhaseLayer GetBroadPhaseLayer(const JPH::ObjectLayer layer) const override {
        if (layer == object_layers::non_moving) {
            return broad_phase_layers::non_moving;
        }
        return broad_phase_layers::moving;
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    [[nodiscard]] const char* GetBroadPhaseLayerName(const JPH::BroadPhaseLayer layer) const override {
        return layer == broad_phase_layers::non_moving ? "non-moving" : "moving";
    }
#endif
};

class ObjectLayerPairs final : public JPH::ObjectLayerPairFilter {
public:
    [[nodiscard]] bool ShouldCollide(const JPH::ObjectLayer first,
                                     const JPH::ObjectLayer second) const override {
        if (first == object_layers::non_moving) {
            return second == object_layers::moving;
        }
        return true;
    }
};

class ObjectVsBroadPhase final : public JPH::ObjectVsBroadPhaseLayerFilter {
public:
    [[nodiscard]] bool ShouldCollide(const JPH::ObjectLayer object_layer,
                                     const JPH::BroadPhaseLayer broad_phase_layer) const override {
        if (object_layer == object_layers::non_moving) {
            return broad_phase_layer == broad_phase_layers::moving;
        }
        return true;
    }
};

std::mutex runtime_mutex;
std::uint32_t runtime_users = 0;

void acquire_jolt_runtime() {
    const std::scoped_lock lock{runtime_mutex};
    if (runtime_users++ == 0) {
        JPH::RegisterDefaultAllocator();
        JPH::Factory::sInstance = new JPH::Factory{};
        JPH::RegisterTypes();
    }
}

void release_jolt_runtime() noexcept {
    const std::scoped_lock lock{runtime_mutex};
    if (--runtime_users == 0) {
        JPH::UnregisterTypes();
        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;
    }
}

class RuntimeLease final {
public:
    RuntimeLease() { acquire_jolt_runtime(); }
    ~RuntimeLease() { release_jolt_runtime(); }

    RuntimeLease(const RuntimeLease&) = delete;
    RuntimeLease& operator=(const RuntimeLease&) = delete;
};

[[nodiscard]] JPH::Vec3 to_jolt(const physics::Vec3 value) {
    return {value.x, value.y, value.z};
}

[[nodiscard]] JPH::Quat to_jolt(const physics::Quaternion value) {
    return {value.x, value.y, value.z, value.w};
}

[[nodiscard]] physics::Vec3 from_jolt(const JPH::Vec3Arg value) {
    return {value.GetX(), value.GetY(), value.GetZ()};
}

[[nodiscard]] physics::Quaternion from_jolt(const JPH::QuatArg value) {
    return {value.GetX(), value.GetY(), value.GetZ(), value.GetW()};
}

[[nodiscard]] JPH::EMotionType to_jolt(const physics::MotionType motion) {
    switch (motion) {
    case physics::MotionType::static_body:
        return JPH::EMotionType::Static;
    case physics::MotionType::dynamic:
        return JPH::EMotionType::Dynamic;
    case physics::MotionType::kinematic:
        return JPH::EMotionType::Kinematic;
    }
    throw std::invalid_argument{"Unknown Gloom motion type"};
}

[[nodiscard]] std::uint32_t resolve_worker_count(const std::uint32_t requested) {
    if (requested != 0) {
        return requested;
    }
    const auto hardware_threads = std::thread::hardware_concurrency();
    return hardware_threads > 1 ? hardware_threads - 1 : 0;
}

void validate_settings(const physics::PhysicsSettings& settings) {
    if (!std::isfinite(settings.fixed_time_step) || settings.fixed_time_step <= 0.0) {
        throw std::invalid_argument{"Physics fixed time step must be finite and positive"};
    }
    if (settings.max_sub_steps == 0 || settings.max_bodies == 0 || settings.max_body_pairs == 0 ||
        settings.max_contact_constraints == 0 || settings.temporary_allocator_bytes == 0) {
        throw std::invalid_argument{"Physics capacity settings must be greater than zero"};
    }
}

[[nodiscard]] std::uint64_t pack_entity(const core::EntityId entity) noexcept {
    if (!entity.valid()) {
        return 0;
    }
    return static_cast<std::uint64_t>(entity.generation) << 32U |
           static_cast<std::uint64_t>(entity.index);
}

[[nodiscard]] core::EntityId unpack_entity(const std::uint64_t value) noexcept {
    if (value == 0) {
        return {};
    }
    return {.index = static_cast<std::uint32_t>(value),
            .generation = static_cast<std::uint32_t>(value >> 32U)};
}

struct BodyPairKey {
    std::uint32_t first{0};
    std::uint32_t second{0};

    friend bool operator==(BodyPairKey, BodyPairKey) = default;
};

struct BodyPairHash {
    [[nodiscard]] std::size_t operator()(const BodyPairKey pair) const noexcept {
        const std::uint64_t packed = static_cast<std::uint64_t>(pair.first) << 32U |
                                     static_cast<std::uint64_t>(pair.second);
        return std::hash<std::uint64_t>{}(packed);
    }
};

class ContactCollector final : public JPH::ContactListener {
public:
    void begin_step(const std::uint64_t simulation_step) {
        const std::scoped_lock lock{mutex_};
        simulation_step_ = simulation_step;
        emitted_pairs_.clear();
    }

    void OnContactAdded(const JPH::Body& first,
                        const JPH::Body& second,
                        const JPH::ContactManifold& manifold,
                        JPH::ContactSettings&) override {
        add_contact(first, second, manifold, false);
    }

    void OnContactPersisted(const JPH::Body& first,
                            const JPH::Body& second,
                            const JPH::ContactManifold& manifold,
                            JPH::ContactSettings&) override {
        add_contact(first, second, manifold, true);
    }

    void OnContactRemoved(const JPH::SubShapeIDPair& sub_shapes) override {
        const std::scoped_lock lock{mutex_};
        const auto found = contacts_.find(sub_shapes);
        if (found == contacts_.end()) {
            return;
        }
        const Contact contact = found->second;
        contacts_.erase(found);
        const auto count = pair_contact_counts_.find(contact.pair);
        if (count == pair_contact_counts_.end()) {
            return;
        }
        if (--count->second == 0) {
            pair_contact_counts_.erase(count);
            emit(physics::TriggerEventType::exited, contact);
        }
    }

    [[nodiscard]] std::vector<physics::TriggerEvent> take_events() {
        std::vector<physics::TriggerEvent> result;
        {
            const std::scoped_lock lock{mutex_};
            result.swap(events_);
        }
        std::ranges::sort(result, [](const physics::TriggerEvent& first,
                                    const physics::TriggerEvent& second) {
            if (first.simulation_step != second.simulation_step) {
                return first.simulation_step < second.simulation_step;
            }
            if (first.trigger.index != second.trigger.index) {
                return first.trigger.index < second.trigger.index;
            }
            if (first.trigger.generation != second.trigger.generation) {
                return first.trigger.generation < second.trigger.generation;
            }
            if (first.other.index != second.other.index) {
                return first.other.index < second.other.index;
            }
            if (first.other.generation != second.other.generation) {
                return first.other.generation < second.other.generation;
            }
            if (first.trigger_body.value != second.trigger_body.value) {
                return first.trigger_body.value < second.trigger_body.value;
            }
            if (first.other_body.value != second.other_body.value) {
                return first.other_body.value < second.other_body.value;
            }
            return first.type < second.type;
        });
        return result;
    }

private:
    struct Contact {
        BodyPairKey pair;
        physics::BodyId first_body;
        physics::BodyId second_body;
        core::EntityId first_entity;
        core::EntityId second_entity;
        bool first_sensor{false};
        bool second_sensor{false};
    };

    void add_contact(const JPH::Body& first,
                     const JPH::Body& second,
                     const JPH::ContactManifold& manifold,
                     const bool persisted) {
        if (!first.IsSensor() && !second.IsSensor()) {
            return;
        }
        const JPH::SubShapeIDPair sub_shapes{first.GetID(),
                                             manifold.mSubShapeID1,
                                             second.GetID(),
                                             manifold.mSubShapeID2};
        const Contact contact{
            .pair = {.first = first.GetID().GetIndexAndSequenceNumber(),
                     .second = second.GetID().GetIndexAndSequenceNumber()},
            .first_body = {first.GetID().GetIndexAndSequenceNumber()},
            .second_body = {second.GetID().GetIndexAndSequenceNumber()},
            .first_entity = unpack_entity(first.GetUserData()),
            .second_entity = unpack_entity(second.GetUserData()),
            .first_sensor = first.IsSensor(),
            .second_sensor = second.IsSensor(),
        };

        const std::scoped_lock lock{mutex_};
        const auto [stored, inserted] = contacts_.try_emplace(sub_shapes, contact);
        static_cast<void>(stored);
        if (inserted) {
            auto& count = pair_contact_counts_[contact.pair];
            if (count++ == 0) {
                emit(physics::TriggerEventType::entered, contact);
            }
            emitted_pairs_.insert(contact.pair);
            return;
        }
        if (persisted && emitted_pairs_.insert(contact.pair).second) {
            emit(physics::TriggerEventType::stayed, contact);
        }
    }

    void emit(const physics::TriggerEventType type, const Contact& contact) {
        const auto append = [&](const core::EntityId trigger,
                                const core::EntityId other,
                                const physics::BodyId trigger_body,
                                const physics::BodyId other_body) {
            if (!trigger.valid() || !other.valid()) {
                return;
            }
            events_.push_back({.trigger = trigger,
                               .other = other,
                               .trigger_body = trigger_body,
                               .other_body = other_body,
                               .simulation_step = simulation_step_,
                               .type = type});
        };
        if (contact.first_sensor) {
            append(contact.first_entity,
                   contact.second_entity,
                   contact.first_body,
                   contact.second_body);
        }
        if (contact.second_sensor) {
            append(contact.second_entity,
                   contact.first_entity,
                   contact.second_body,
                   contact.first_body);
        }
    }

    std::mutex mutex_;
    std::unordered_map<JPH::SubShapeIDPair, Contact> contacts_;
    std::unordered_map<BodyPairKey, std::size_t, BodyPairHash> pair_contact_counts_;
    std::unordered_set<BodyPairKey, BodyPairHash> emitted_pairs_;
    std::vector<physics::TriggerEvent> events_;
    std::uint64_t simulation_step_{0};
};

class CharacterContactCollector final : public JPH::CharacterContactListener {
public:
    void begin_step(const std::uint64_t step) {
        const std::scoped_lock lock{mutex_};
        simulation_step_ = step;
    }

    void OnContactAdded(const JPH::CharacterVirtual* character,
                        const JPH::CharacterContact& contact,
                        JPH::CharacterContactSettings&) override {
        append(character, contact, physics::CharacterContactEventType::entered);
    }
    void OnContactPersisted(const JPH::CharacterVirtual* character,
                            const JPH::CharacterContact& contact,
                            JPH::CharacterContactSettings&) override {
        append(character, contact, physics::CharacterContactEventType::stayed);
    }
    void OnContactRemoved(const JPH::CharacterVirtual* character,
                          const JPH::BodyID& body,
                          const JPH::SubShapeID&) override {
        const std::scoped_lock lock{mutex_};
        const auto key = make_key(character, body);
        const auto found = active_.find(key);
        if (found == active_.end()) return;
        auto event = found->second;
        event.type = physics::CharacterContactEventType::exited;
        event.simulation_step = simulation_step_;
        events_.push_back(event);
        active_.erase(found);
    }

    [[nodiscard]] std::vector<physics::CharacterContactEvent> take_events() {
        std::vector<physics::CharacterContactEvent> result;
        { const std::scoped_lock lock{mutex_}; result.swap(events_); }
        std::ranges::sort(result, [](const auto& a, const auto& b) {
            if (a.simulation_step != b.simulation_step) return a.simulation_step < b.simulation_step;
            if (a.character.index != b.character.index) return a.character.index < b.character.index;
            if (a.other.index != b.other.index) return a.other.index < b.other.index;
            if (a.other_body.value != b.other_body.value) return a.other_body.value < b.other_body.value;
            return a.type < b.type;
        });
        return result;
    }

private:
    [[nodiscard]] static std::uint64_t make_key(const JPH::CharacterVirtual* character,
                                                const JPH::BodyID body) {
        return static_cast<std::uint64_t>(character->GetID().GetValue()) << 32U |
               body.GetIndexAndSequenceNumber();
    }
    void append(const JPH::CharacterVirtual* character,
                const JPH::CharacterContact& contact,
                const physics::CharacterContactEventType type) {
        const auto owner = unpack_entity(character->GetUserData());
        const auto other = unpack_entity(contact.mUserData);
        if (!owner.valid() || !other.valid()) return;
        physics::CharacterContactEvent event{
            .character = owner,
            .other = other,
            .other_body = {contact.mBodyB.GetIndexAndSequenceNumber()},
            .position = {static_cast<float>(contact.mPosition.GetX()),
                         static_cast<float>(contact.mPosition.GetY()),
                         static_cast<float>(contact.mPosition.GetZ())},
            .normal = from_jolt(contact.mContactNormal),
            .simulation_step = simulation_step_,
            .type = type,
        };
        const std::scoped_lock lock{mutex_};
        events_.push_back(event);
        active_[make_key(character, contact.mBodyB)] = event;
    }
    std::mutex mutex_;
    std::unordered_map<std::uint64_t, physics::CharacterContactEvent> active_;
    std::vector<physics::CharacterContactEvent> events_;
    std::uint64_t simulation_step_{0};
};

} // namespace

struct JoltWorld::Impl {
    struct CharacterRecord {
        JPH::Ref<JPH::CharacterVirtual> character;
        JPH::Vec3 desired_horizontal_velocity{JPH::Vec3::sZero()};
        JPH::CharacterVirtual::ExtendedUpdateSettings update_settings;
    };

    Impl(const physics::PhysicsSettings& settings, const std::uint32_t worker_threads)
        : temporary_allocator{settings.temporary_allocator_bytes},
          jobs{JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, static_cast<int>(worker_threads)} {
        system.Init(settings.max_bodies,
                    0,
                    settings.max_body_pairs,
                    settings.max_contact_constraints,
                    broad_phase_layer_interface,
                    object_vs_broad_phase,
                    object_layer_pairs);
        system.SetContactListener(&contact_collector);
    }

    ~Impl() {
        auto& bodies = system.GetBodyInterface();
        for (const std::uint32_t value : body_ids) {
            const JPH::BodyID id{value};
            bodies.RemoveBody(id);
            bodies.DestroyBody(id);
        }
    }

    [[nodiscard]] JPH::BodyID checked_body(const physics::BodyId body) const {
        if (!body.valid() || !body_ids.contains(body.value)) {
            throw std::invalid_argument{"Physics body handle is invalid or no longer exists"};
        }
        return JPH::BodyID{body.value};
    }

    [[nodiscard]] CharacterRecord& checked_character(const physics::CharacterId character) {
        const auto found = characters.find(character.value);
        if (!character.valid() || found == characters.end()) {
            throw std::invalid_argument{"Physics character handle is invalid or no longer exists"};
        }
        return found->second;
    }

    [[nodiscard]] const CharacterRecord& checked_character(
        const physics::CharacterId character) const {
        const auto found = characters.find(character.value);
        if (!character.valid() || found == characters.end()) {
            throw std::invalid_argument{"Physics character handle is invalid or no longer exists"};
        }
        return found->second;
    }

    RuntimeLease runtime;
    JPH::TempAllocatorImpl temporary_allocator;
    JPH::JobSystemThreadPool jobs;
    BroadPhaseLayers broad_phase_layer_interface;
    ObjectVsBroadPhase object_vs_broad_phase;
    ObjectLayerPairs object_layer_pairs;
    ContactCollector contact_collector;
    CharacterContactCollector character_contact_collector;
    JPH::PhysicsSystem system;
    std::unordered_set<std::uint32_t> body_ids;
    std::unordered_map<std::uint32_t, CharacterRecord> characters;
    std::uint32_t next_character_id{1};
};

JoltWorld::JoltWorld(physics::PhysicsSettings settings) : settings_{settings} {}

JoltWorld::~JoltWorld() {
    stop();
}

std::string_view JoltWorld::name() const noexcept {
    return "physics.jolt";
}

core::SubsystemState JoltWorld::state() const noexcept {
    return state_;
}

void JoltWorld::start() {
    if (state_ == core::SubsystemState::running) {
        throw std::logic_error{"Jolt physics world is already running"};
    }

    validate_settings(settings_);
    const std::uint32_t workers = resolve_worker_count(settings_.worker_threads);
    impl_ = std::make_unique<Impl>(settings_, workers);
    ++instance_generation_;
    if (instance_generation_ == 0) {
        instance_generation_ = 1;
    }
    stats_ = {.worker_threads = workers};
    accumulator_ = 0.0;
    state_ = core::SubsystemState::running;
}

void JoltWorld::tick(const double delta_seconds) {
    simulate(delta_seconds);
}

void JoltWorld::stop() noexcept {
    impl_.reset();
    accumulator_ = 0.0;
    stats_ = {};
    state_ = core::SubsystemState::stopped;
}

std::uint64_t JoltWorld::instance_generation() const noexcept {
    return instance_generation_;
}

void JoltWorld::simulate(const double delta_seconds) {
    require_running();
    if (!std::isfinite(delta_seconds) || delta_seconds < 0.0) {
        throw std::invalid_argument{"Physics delta time must be finite and non-negative"};
    }

    const double maximum_accumulation = settings_.fixed_time_step * settings_.max_sub_steps;
    accumulator_ = std::min(accumulator_ + delta_seconds, maximum_accumulation);
    stats_.last_sub_steps = 0;

    while (accumulator_ + 1.0e-12 >= settings_.fixed_time_step &&
           stats_.last_sub_steps < settings_.max_sub_steps) {
        const float fixed_delta = static_cast<float>(settings_.fixed_time_step);
        impl_->contact_collector.begin_step(stats_.completed_steps + 1);
        impl_->character_contact_collector.begin_step(stats_.completed_steps + 1);
        const JPH::Vec3 gravity = impl_->system.GetGravity();
        for (auto& [id, record] : impl_->characters) {
            static_cast<void>(id);
            JPH::Vec3 velocity = record.character->GetLinearVelocity();
            velocity.SetX(record.desired_horizontal_velocity.GetX());
            velocity.SetZ(record.desired_horizontal_velocity.GetZ());
            const auto ground_state = record.character->GetGroundState();
            const bool supported = ground_state == JPH::CharacterBase::EGroundState::OnGround;
            if (supported && record.character->GetLinearVelocity().GetY() <= 0.0F) {
                velocity.SetY(record.character->GetGroundVelocity().GetY());
            } else {
                velocity.SetY(record.character->GetLinearVelocity().GetY() +
                              gravity.GetY() * fixed_delta);
            }
            record.character->SetLinearVelocity(velocity);
            record.character->ExtendedUpdate(
                fixed_delta,
                gravity,
                record.update_settings,
                impl_->system.GetDefaultBroadPhaseLayerFilter(object_layers::moving),
                impl_->system.GetDefaultLayerFilter(object_layers::moving),
                {},
                {},
                impl_->temporary_allocator);
        }
        const auto update_error = impl_->system.Update(static_cast<float>(settings_.fixed_time_step),
                                                       1,
                                                       &impl_->temporary_allocator,
                                                       &impl_->jobs);
        if (update_error != JPH::EPhysicsUpdateError::None) {
            throw std::runtime_error{"Jolt reported a physics update capacity error"};
        }
        accumulator_ -= settings_.fixed_time_step;
        ++stats_.last_sub_steps;
        ++stats_.completed_steps;
    }

    stats_.interpolation_alpha = std::clamp(accumulator_ / settings_.fixed_time_step, 0.0, 1.0);
}

physics::BodyId JoltWorld::create_body(const physics::BodyDesc& description) {
    require_running();

    JPH::RefConst<JPH::Shape> shape;
    switch (description.shape.type) {
    case physics::ShapeType::triangle_mesh: {
        const auto& mesh=description.shape.triangle_mesh;
        if (!mesh || mesh->indices.empty() || mesh->indices.size()%3 ||
            description.motion!=physics::MotionType::static_body)
            throw std::invalid_argument{"Triangle collision requires a nonempty static mesh"};
        JPH::VertexList vertices;
        for (const auto& vertex : mesh->vertices) {
            if (!std::isfinite(vertex.x)||!std::isfinite(vertex.y)||!std::isfinite(vertex.z))
                throw std::invalid_argument{"Nonfinite collision vertex"};
            vertices.emplace_back(vertex.x,vertex.y,vertex.z);
        }
        JPH::IndexedTriangleList triangles;
        for (std::size_t i=0;i<mesh->indices.size();i+=3) {
            for (std::size_t j=0;j<3;++j)
                if (mesh->indices[i+j]>=vertices.size()) throw std::invalid_argument{"Invalid collision index"};
            triangles.emplace_back(mesh->indices[i],mesh->indices[i+1],mesh->indices[i+2]);
        }
        JPH::MeshShapeSettings settings{std::move(vertices),std::move(triangles)};
        const auto result=settings.Create();
        if (result.HasError()) throw std::invalid_argument{result.GetError().c_str()};
        shape=result.Get();
        break;
    }
    case physics::ShapeType::box:
        if (description.shape.half_extent.x <= 0.0F || description.shape.half_extent.y <= 0.0F ||
            description.shape.half_extent.z <= 0.0F) {
            throw std::invalid_argument{"Box half extents must be positive"};
        }
        shape = new JPH::BoxShape{to_jolt(description.shape.half_extent)};
        break;
    case physics::ShapeType::sphere:
        if (description.shape.radius <= 0.0F) {
            throw std::invalid_argument{"Sphere radius must be positive"};
        }
        shape = new JPH::SphereShape{description.shape.radius};
        break;
    }

    const JPH::ObjectLayer layer = description.motion == physics::MotionType::static_body
                                       ? object_layers::non_moving
                                       : object_layers::moving;
    JPH::BodyCreationSettings body_settings{shape,
                                            JPH::RVec3{to_jolt(description.transform.position)},
                                            to_jolt(description.transform.rotation),
                                            to_jolt(description.motion),
                                            layer};
    body_settings.mFriction = description.friction;
    body_settings.mRestitution = description.restitution;
    body_settings.mUserData = pack_entity(description.owner);
    body_settings.mIsSensor = description.sensor;

    auto& bodies = impl_->system.GetBodyInterface();
    const JPH::EActivation activation = description.motion == physics::MotionType::dynamic
                                            ? JPH::EActivation::Activate
                                            : JPH::EActivation::DontActivate;
    const JPH::BodyID id = bodies.CreateAndAddBody(body_settings, activation);
    if (id.IsInvalid()) {
        throw std::runtime_error{"Jolt could not create a physics body; capacity may be exhausted"};
    }

    const auto value = id.GetIndexAndSequenceNumber();
    impl_->body_ids.insert(value);
    return physics::BodyId{value};
}

void JoltWorld::destroy_body(const physics::BodyId body) {
    require_running();
    const JPH::BodyID id = impl_->checked_body(body);
    auto& bodies = impl_->system.GetBodyInterface();
    bodies.RemoveBody(id);
    bodies.DestroyBody(id);
    impl_->body_ids.erase(body.value);
}

physics::CharacterState JoltWorld::query_character_motion(
    const physics::CharacterDesc& description, const physics::Vec3 velocity,
    const float delta_seconds, const physics::Vec3 gravity) {
    require_running();
    if (!(delta_seconds>0.0F) || !std::isfinite(delta_seconds) ||
        !std::isfinite(description.radius) || !std::isfinite(description.cylinder_half_height) ||
        description.radius<=0 || description.cylinder_half_height<=0 ||
        !std::isfinite(description.position.x) || !std::isfinite(description.position.y) || !std::isfinite(description.position.z) ||
        !std::isfinite(velocity.x) || !std::isfinite(velocity.y) || !std::isfinite(velocity.z) ||
        !std::isfinite(gravity.x) || !std::isfinite(gravity.y) || !std::isfinite(gravity.z))
        throw std::invalid_argument{"Invalid scene movement query"};
    const JPH::RefConst<JPH::Shape> capsule=new JPH::CapsuleShape{
        description.cylinder_half_height,description.radius};
    const JPH::RefConst<JPH::Shape> standing=new JPH::RotatedTranslatedShape{
        JPH::Vec3{0,description.cylinder_half_height+description.radius,0},JPH::Quat::sIdentity(),capsule};
    JPH::CharacterVirtualSettings settings;
    settings.mShape=standing;
    settings.mMaxSlopeAngle=description.max_slope_angle_radians;
    settings.mSupportingVolume=JPH::Plane{JPH::Vec3::sAxisY(),-description.radius};
    JPH::CharacterVirtual character{&settings,JPH::RVec3{to_jolt(description.position)},
                                    JPH::Quat::sIdentity(),0,&impl_->system};
    character.SetLinearVelocity(to_jolt(velocity));
    JPH::CharacterVirtual::ExtendedUpdateSettings update;
    update.mWalkStairsStepUp={0,description.step_up_height,0};
    update.mStickToFloorStepDown=velocity.y>0 ? JPH::Vec3::sZero() : JPH::Vec3{0,-description.step_down_height,0};
    character.ExtendedUpdate(delta_seconds,to_jolt(gravity),update,
        impl_->system.GetDefaultBroadPhaseLayerFilter(object_layers::moving),
        impl_->system.GetDefaultLayerFilter(object_layers::moving),{},{},impl_->temporary_allocator);
    const auto position=character.GetPosition();
    return {.position={static_cast<float>(position.GetX()),static_cast<float>(position.GetY()),static_cast<float>(position.GetZ())},
            .velocity=from_jolt(character.GetLinearVelocity()),
            .ground_velocity=from_jolt(character.GetGroundVelocity()),
            .ground_normal=from_jolt(character.GetGroundNormal()),
            .grounded=character.GetGroundState()==JPH::CharacterBase::EGroundState::OnGround};
}

core::EntityId JoltWorld::body_owner(const physics::BodyId body) const {
    require_running();
    const JPH::BodyID id = impl_->checked_body(body);
    return unpack_entity(impl_->system.GetBodyInterface().GetUserData(id));
}

physics::Transform JoltWorld::body_transform(const physics::BodyId body) const {
    require_running();
    const JPH::BodyID id = impl_->checked_body(body);
    JPH::RVec3 position;
    JPH::Quat rotation;
    impl_->system.GetBodyInterface().GetPositionAndRotation(id, position, rotation);
    return {{static_cast<float>(position.GetX()),
             static_cast<float>(position.GetY()),
             static_cast<float>(position.GetZ())},
            from_jolt(rotation)};
}

void JoltWorld::set_body_transform(const physics::BodyId body,
                                   const physics::Transform transform) {
    require_running();
    impl_->system.GetBodyInterface().SetPositionAndRotation(
        impl_->checked_body(body), JPH::RVec3{to_jolt(transform.position)},
        to_jolt(transform.rotation), JPH::EActivation::Activate);
}

void JoltWorld::move_kinematic_body(const physics::BodyId body,
                                    const physics::Transform target,
                                    const float delta_seconds) {
    require_running();
    if (!std::isfinite(delta_seconds) || delta_seconds <= 0.0F) {
        throw std::invalid_argument{"Kinematic move delta must be finite and positive"};
    }
    const JPH::BodyID id = impl_->checked_body(body);
    if (impl_->system.GetBodyInterface().GetMotionType(id) != JPH::EMotionType::Kinematic) {
        throw std::invalid_argument{"Only kinematic bodies can use velocity-derived movement"};
    }
    impl_->system.GetBodyInterface().MoveKinematic(
        id, JPH::RVec3{to_jolt(target.position)}, to_jolt(target.rotation), delta_seconds);
}

physics::Vec3 JoltWorld::linear_velocity(const physics::BodyId body) const {
    require_running();
    return from_jolt(impl_->system.GetBodyInterface().GetLinearVelocity(impl_->checked_body(body)));
}

void JoltWorld::set_linear_velocity(const physics::BodyId body, const physics::Vec3 velocity) {
    require_running();
    impl_->system.GetBodyInterface().SetLinearVelocity(impl_->checked_body(body), to_jolt(velocity));
}

std::vector<physics::TriggerEvent> JoltWorld::take_trigger_events() {
    require_running();
    return impl_->contact_collector.take_events();
}

physics::CharacterId JoltWorld::create_character(const physics::CharacterDesc& description) {
    require_running();
    if (!std::isfinite(description.position.x) || !std::isfinite(description.position.y) ||
        !std::isfinite(description.position.z) ||
        !std::isfinite(description.radius) || description.radius <= 0.0F ||
        !std::isfinite(description.cylinder_half_height) ||
        description.cylinder_half_height <= 0.0F ||
        !std::isfinite(description.max_slope_angle_radians) ||
        description.max_slope_angle_radians <= 0.0F ||
        description.max_slope_angle_radians > 1.570796327F ||
        !std::isfinite(description.step_up_height) || description.step_up_height < 0.0F ||
        !std::isfinite(description.step_down_height) || description.step_down_height < 0.0F ||
        !std::isfinite(description.mass) || description.mass <= 0.0F ||
        !std::isfinite(description.maximum_push_force) || description.maximum_push_force < 0.0F) {
        throw std::invalid_argument{
            "Character position must be finite; dimensions and slope angle must be valid"};
    }
    const JPH::RefConst<JPH::Shape> capsule =
        new JPH::CapsuleShape{description.cylinder_half_height, description.radius};
    const JPH::RefConst<JPH::Shape> standing_shape = new JPH::RotatedTranslatedShape{
        JPH::Vec3{0.0F, description.cylinder_half_height + description.radius, 0.0F},
        JPH::Quat::sIdentity(),
        capsule};
    JPH::CharacterVirtualSettings settings;
    settings.mShape = standing_shape;
    settings.mInnerBodyShape = standing_shape;
    settings.mInnerBodyLayer = object_layers::moving;
    settings.mMaxSlopeAngle = description.max_slope_angle_radians;
    settings.mMass = description.mass;
    settings.mMaxStrength = description.maximum_push_force;
    settings.mSupportingVolume = JPH::Plane{JPH::Vec3::sAxisY(), -description.radius};
    JPH::Ref<JPH::CharacterVirtual> character = new JPH::CharacterVirtual{
        &settings,
        JPH::RVec3{to_jolt(description.position)},
        JPH::Quat::sIdentity(),
        pack_entity(description.owner),
        &impl_->system};
    character->SetListener(&impl_->character_contact_collector);
    JPH::CharacterVirtual::ExtendedUpdateSettings update_settings;
    update_settings.mWalkStairsStepUp = {0.0F, description.step_up_height, 0.0F};
    update_settings.mStickToFloorStepDown = {0.0F, -description.step_down_height, 0.0F};
    const std::uint32_t id = impl_->next_character_id++;
    impl_->characters.emplace(id, Impl::CharacterRecord{.character = std::move(character),
                                                        .update_settings = update_settings});
    return physics::CharacterId{id};
}

void JoltWorld::destroy_character(const physics::CharacterId character) {
    require_running();
    static_cast<void>(impl_->checked_character(character));
    impl_->characters.erase(character.value);
}

void JoltWorld::set_character_horizontal_velocity(const physics::CharacterId character,
                                                  const physics::Vec3 velocity) {
    require_running();
    if (!std::isfinite(velocity.x) || !std::isfinite(velocity.y) || !std::isfinite(velocity.z)) {
        throw std::invalid_argument{"Character velocity must be finite"};
    }
    auto& record = impl_->checked_character(character);
    record.desired_horizontal_velocity = {velocity.x, 0.0F, velocity.z};
}

void JoltWorld::jump_character(const physics::CharacterId character, const float jump_speed) {
    require_running();
    if (!std::isfinite(jump_speed) || jump_speed <= 0.0F) {
        throw std::invalid_argument{"Character jump speed must be finite and positive"};
    }
    auto& virtual_character = impl_->checked_character(character).character;
    if (virtual_character->GetGroundState() == JPH::CharacterBase::EGroundState::OnGround) {
        JPH::Vec3 velocity = virtual_character->GetLinearVelocity();
        velocity.SetY(jump_speed);
        virtual_character->SetLinearVelocity(velocity);
    }
}

void JoltWorld::add_character_impulse(const physics::CharacterId character,
                                      const physics::Vec3 impulse) {
    require_running();
    if (!std::isfinite(impulse.x) || !std::isfinite(impulse.y) || !std::isfinite(impulse.z)) {
        throw std::invalid_argument{"Character impulse must be finite"};
    }
    auto& record = impl_->checked_character(character);
    record.character->SetLinearVelocity(record.character->GetLinearVelocity() + to_jolt(impulse));
}

void JoltWorld::set_character_position(const physics::CharacterId character,
                                       const physics::Vec3 position) {
    require_running();
    if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z)) {
        throw std::invalid_argument{"Character position must be finite"};
    }
    impl_->checked_character(character).character->SetPosition(JPH::RVec3{to_jolt(position)});
}

physics::CharacterState JoltWorld::character_state(const physics::CharacterId character) const {
    require_running();
    const auto& virtual_character = impl_->checked_character(character).character;
    const JPH::RVec3 position = virtual_character->GetPosition();
    return {
        .position = {static_cast<float>(position.GetX()),
                     static_cast<float>(position.GetY()),
                     static_cast<float>(position.GetZ())},
        .velocity = from_jolt(virtual_character->GetLinearVelocity()),
        .ground_velocity = from_jolt(virtual_character->GetGroundVelocity()),
        .ground_normal = from_jolt(virtual_character->GetGroundNormal()),
        .grounded = virtual_character->GetGroundState() ==
                    JPH::CharacterBase::EGroundState::OnGround,
    };
}

std::vector<physics::CharacterContactEvent> JoltWorld::take_character_contact_events() {
    require_running();
    return impl_->character_contact_collector.take_events();
}

physics::SimulationStats JoltWorld::statistics() const noexcept {
    return stats_;
}

void JoltWorld::require_running() const {
    if (state_ != core::SubsystemState::running || !impl_) {
        throw std::logic_error{"Jolt physics world must be running"};
    }
}

} // namespace gloom::backends
