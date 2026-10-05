#pragma once
#include <gloom/core/array.hpp>

#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace gloom::assets {

class VirtualPath final {
  public:
    [[nodiscard]] static std::expected<VirtualPath, std::string> parse(std::string_view path);

    [[nodiscard]] std::string_view string() const noexcept;
    [[nodiscard]] std::string_view mount() const noexcept;
    [[nodiscard]] std::string_view relative() const noexcept;
    [[nodiscard]] bool operator==(const VirtualPath&) const noexcept = default;

  private:
    VirtualPath(std::string normalized, std::size_t mount_length);

    std::string normalized_;
    std::size_t mount_length_{0};
};

struct AssetId {
    uint64 value{0};
    [[nodiscard]] bool operator==(const AssetId&) const noexcept = default;
};

struct AssetIdHash {
    [[nodiscard]] std::size_t operator()(AssetId id) const noexcept;
};

enum class AssetType : uint8 {
    binary = 1,
    scene = 2,
    mesh = 3,
    material = 4,
    texture = 5,
};

[[nodiscard]] AssetId make_asset_id(const VirtualPath& path, AssetType type) noexcept;
[[nodiscard]] std::uint64_t fingerprint(std::span<const std::byte> bytes) noexcept;

class VirtualFileSystem final {
  public:
    void mount(std::string_view name, const std::filesystem::path& root);
    [[nodiscard]] std::expected<std::filesystem::path, std::string> resolve(const VirtualPath& path) const;
    [[nodiscard]] std::expected<std::vector<std::byte>, std::string> read(const VirtualPath& path) const;
    [[nodiscard]] std::expected<void, std::string> write(const VirtualPath& path, std::span<const std::byte> bytes) const;

  private:
    std::unordered_map<std::string, std::filesystem::path> mounts_;
};

struct AssetRecord {
    AssetId id;
    AssetType type{AssetType::binary};
    VirtualPath source;
    VirtualPath cooked;
    std::uint64_t source_fingerprint{0};
    std::vector<AssetId> dependencies;
};

class AssetCatalog final {
  public:
    AssetCatalog() = default;
    ~AssetCatalog();
    AssetCatalog(const AssetCatalog&) = delete;
    AssetCatalog& operator=(const AssetCatalog&) = delete;
    AssetCatalog(AssetCatalog&&) noexcept = default;
    AssetCatalog& operator=(AssetCatalog&& other) noexcept;
    // Transfers the owned record; find pointers survive growth and upsert.
    void add(AssetRecord record);
    void upsert(AssetRecord record);
    [[nodiscard]] const AssetRecord* find(AssetId id) const noexcept;
    // External graph errors return a static message and leave order empty.
    [[nodiscard]] bool dependency_order(Array<AssetId>& order, const char*& error) const;
    [[nodiscard]] size_t size() const noexcept;

  private:
    struct Entry {
        AssetId id;
        AssetRecord* record{nullptr};
    };
    [[nodiscard]] size_t lower_bound(AssetId id) const noexcept;
    Array<Entry> records_;
};

struct CookedAsset {
    AssetId id;
    AssetType type{AssetType::binary};
    std::uint64_t source_fingerprint{0};
    std::vector<AssetId> dependencies;
    std::vector<std::byte> payload;
};

inline constexpr std::uint32_t cooked_asset_version = 1;

[[nodiscard]] std::vector<std::byte> encode_cooked_asset(const CookedAsset& asset);
[[nodiscard]] std::expected<CookedAsset, std::string> decode_cooked_asset(std::span<const std::byte> encoded);

} // namespace gloom::assets
