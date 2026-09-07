#pragma once
#include <gloom/assets/asset.hpp>
#include <array>
#include <memory>

namespace gloom::audio {
struct Vec3 { float x{}, y{}, z{}; };
struct Listener { Vec3 position{}, forward{0,0,-1}, up{0,1,0}, velocity{}; };
enum class Bus : unsigned char { music, effects, ambient };
struct Clip {
    unsigned rate{}, channels{};
    std::vector<float> samples;
    [[nodiscard]] std::size_t frames() const { return samples.size()/channels; }
};
[[nodiscard]] std::expected<std::shared_ptr<const Clip>,std::string> decode_clip(std::span<const std::byte> bytes);
[[nodiscard]] std::expected<std::shared_ptr<const Clip>,std::string> load_clip(const assets::VirtualFileSystem&,std::string_view path);
struct VoiceDesc {
    Bus bus{Bus::effects}; Vec3 position{};
    float gain{1}, minimum_distance{7.5F}, maximum_distance{150};
    bool spatial{}, loop{};
};
using VoiceId = std::uint64_t;
struct MixMetrics { std::uint64_t started{}, dropped{}, finished{}; std::size_t active{}; };
// Main-thread mixer. Platform output owns its own queue; no callback sees this object.
class Mixer {
public:
    explicit Mixer(std::size_t limit=64);
    VoiceId play(std::shared_ptr<const Clip>,VoiceDesc={});
    void stop(VoiceId);
    void stop_scene();
    void move(VoiceId,Vec3);
    void listener(Listener);
    void volumes(float master,float music,float effects);
    void pause(bool value) { paused_=value; }
    void render(std::span<float> stereo);
    [[nodiscard]] bool playing(VoiceId) const;
    [[nodiscard]] MixMetrics metrics() const;
    [[nodiscard]] std::array<float,2> spatial_gains(const VoiceDesc&) const;
private:
    struct Voice { VoiceId id; std::shared_ptr<const Clip> clip; VoiceDesc desc; double cursor{}; };
    std::vector<Voice> voices_;
    std::size_t limit_; VoiceId next_{1}; Listener listener_;
    float master_{.7F},music_{.35F},effects_{.8F}; bool paused_{};
    MixMetrics metrics_;
};
class Output {
public:
    virtual ~Output()=default;
    virtual void pump(Mixer& mixer,double elapsed)=0;
    [[nodiscard]] virtual bool available() const noexcept=0;
    [[nodiscard]] virtual std::string diagnostic() const=0;
};
[[nodiscard]] std::unique_ptr<Output> make_null_output();
[[nodiscard]] std::unique_ptr<Output> make_sdl_output();
}
