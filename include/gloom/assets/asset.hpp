#pragma once
#include <gloom/core/array.hpp>

#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>
#include <string_view>
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
    VirtualPath(std::string normalized, size_t mount_length);

    std::string normalized_;
    size_t mount_length_{0};
};

struct AssetId {
    uint64 value{0};
    [[nodiscard]] bool operator==(const AssetId&) const noexcept = default;
};

struct AssetIdHash {
    [[nodiscard]] size_t operator()(AssetId id) const noexcept;
};

enum class AssetType : uint8 {
    binary = 1,
    scene = 2,
    mesh = 3,
    material = 4,
    texture = 5,
};

[[nodiscard]] AssetId make_asset_id(const VirtualPath& path, AssetType type) noexcept;
[[nodiscard]] uint64 fingerprint(Span<const std::byte> bytes) noexcept;

class VirtualFileSystem final {
  public:
    [[nodiscard]] std::expected<void, std::string> mount(std::string_view name, const std::filesystem::path& root);
    [[nodiscard]] std::expected<std::filesystem::path, std::string> resolve(const VirtualPath& path) const;
    [[nodiscard]] std::expected<std::vector<std::byte>, std::string> read(const VirtualPath& path) const;
    [[nodiscard]] std::expected<void, std::string> write(const VirtualPath& path, Span<const std::byte> bytes) const;

  private:
    struct Mount {
        std::string name;
        std::filesystem::path root;
    };
    // A few mounts, initialized once. Comparing views avoids a temporary string per resolve.
    std::vector<Mount> mounts_;
};

struct AssetRecord {
    AssetId id;
    AssetType type{AssetType::binary};
    VirtualPath source;
    VirtualPath cooked;
    uint64 source_fingerprint{0};
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
    uint64 source_fingerprint{0};
    std::vector<AssetId> dependencies;
    std::vector<std::byte> payload;
};

inline constexpr uint32 cooked_asset_version = 1;

[[nodiscard]] std::vector<std::byte> encode_cooked_asset(const CookedAsset& asset);
// Encoder requires a valid owned asset; malformed external bytes are checked by the decoder.
[[nodiscard]] std::expected<CookedAsset, std::string> decode_cooked_asset(Span<const std::byte> encoded);

} // namespace gloom::assets
