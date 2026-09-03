#pragma once

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
  [[nodiscard]] static std::expected<VirtualPath, std::string>
  parse(std::string_view path);

  [[nodiscard]] std::string_view string() const noexcept;
  [[nodiscard]] std::string_view mount() const noexcept;
  [[nodiscard]] std::string_view relative() const noexcept;
  [[nodiscard]] bool operator==(const VirtualPath &) const noexcept = default;

private:
  VirtualPath(std::string normalized, std::size_t mount_length);

  std::string normalized_;
  std::size_t mount_length_{0};
};

struct AssetId {
  std::uint64_t value{0};
  [[nodiscard]] bool operator==(const AssetId &) const noexcept = default;
};

struct AssetIdHash {
  [[nodiscard]] std::size_t operator()(AssetId id) const noexcept;
};

enum class AssetType : std::uint8_t {
  binary = 1,
  scene = 2,
  mesh = 3,
  material = 4,
  texture = 5,
};

[[nodiscard]] AssetId make_asset_id(const VirtualPath &path,
                                    AssetType type) noexcept;
[[nodiscard]] std::uint64_t
fingerprint(std::span<const std::byte> bytes) noexcept;

class VirtualFileSystem final {
public:
  void mount(std::string_view name, const std::filesystem::path &root);
  [[nodiscard]] std::expected<std::filesystem::path, std::string>
  resolve(const VirtualPath &path) const;
  [[nodiscard]] std::expected<std::vector<std::byte>, std::string>
  read(const VirtualPath &path) const;
  [[nodiscard]] std::expected<void, std::string>
  write(const VirtualPath &path, std::span<const std::byte> bytes) const;

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
  void add(AssetRecord record);
  void upsert(AssetRecord record);
  [[nodiscard]] const AssetRecord *find(AssetId id) const noexcept;
  [[nodiscard]] std::expected<std::vector<AssetId>, std::string>
  dependency_order() const;
  [[nodiscard]] std::size_t size() const noexcept;

private:
  std::unordered_map<AssetId, AssetRecord, AssetIdHash> records_;
};

struct CookedAsset {
  AssetId id;
  AssetType type{AssetType::binary};
  std::uint64_t source_fingerprint{0};
  std::vector<AssetId> dependencies;
  std::vector<std::byte> payload;
};

inline constexpr std::uint32_t cooked_asset_version = 1;

[[nodiscard]] std::vector<std::byte>
encode_cooked_asset(const CookedAsset &asset);
[[nodiscard]] std::expected<CookedAsset, std::string>
decode_cooked_asset(std::span<const std::byte> encoded);

} // namespace gloom::assets
