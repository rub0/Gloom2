#include <gloom/assets/asset.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace gloom::assets {
namespace {

constexpr std::uint64_t fnv_offset = 14'695'981'039'346'656'037ULL;
constexpr std::uint64_t fnv_prime = 1'099'511'628'211ULL;
constexpr std::array<std::byte, 8> cooked_magic{
    std::byte{'G'}, std::byte{'L'}, std::byte{'O'}, std::byte{'O'},
    std::byte{'M'}, std::byte{'A'}, std::byte{'S'}, std::byte{0}};
constexpr std::size_t cooked_fixed_header_size = 52;
constexpr std::size_t maximum_asset_size = 1024ULL * 1024ULL * 1024ULL;
constexpr std::size_t maximum_dependencies = 65'536;

[[nodiscard]] bool valid_mount_name(const std::string_view name) noexcept {
  return name.size() >= 2 &&
         std::ranges::all_of(name, [](const unsigned char character) {
           return std::islower(character) != 0 ||
                  std::isdigit(character) != 0 || character == '_' ||
                  character == '-';
         });
}

[[nodiscard]] bool valid_type(const AssetType type) noexcept {
  return type >= AssetType::binary && type <= AssetType::texture;
}

[[nodiscard]] std::uint64_t
hash_bytes(const std::span<const std::byte> bytes,
           std::uint64_t hash = fnv_offset) noexcept {
  for (const std::byte value : bytes) {
    hash ^= std::to_integer<std::uint8_t>(value);
    hash *= fnv_prime;
  }
  return hash;
}

[[nodiscard]] bool is_within(const std::filesystem::path &root,
                             const std::filesystem::path &candidate) {
  auto root_part = root.begin();
  auto candidate_part = candidate.begin();
  for (; root_part != root.end(); ++root_part, ++candidate_part) {
    if (candidate_part == candidate.end() || *candidate_part != *root_part) {
      return false;
    }
  }
  return true;
}

template <typename Integer>
void append_integer(std::vector<std::byte> &output, const Integer value) {
  static_assert(std::is_unsigned_v<Integer>);
  for (std::size_t index = 0; index < sizeof(Integer); ++index) {
    output.push_back(static_cast<std::byte>(value >> (index * 8)));
  }
}

template <typename Integer>
[[nodiscard]] Integer read_integer(const std::span<const std::byte> input,
                                   std::size_t &offset) {
  static_assert(std::is_unsigned_v<Integer>);
  Integer value = 0;
  for (std::size_t index = 0; index < sizeof(Integer); ++index) {
    value |=
        static_cast<Integer>(std::to_integer<unsigned int>(input[offset++]))
        << (index * 8);
  }
  return value;
}

} // namespace

VirtualPath::VirtualPath(std::string normalized, const std::size_t mount_length)
    : normalized_{std::move(normalized)}, mount_length_{mount_length} {}

std::expected<VirtualPath, std::string>
VirtualPath::parse(const std::string_view path) {
  if (path.empty() || path.find('\0') != std::string_view::npos) {
    return std::unexpected{"Virtual path is empty or contains a null byte"};
  }
  std::string normalized{path};
  std::ranges::replace(normalized, '\\', '/');
  const std::size_t separator = normalized.find(":/");
  if (separator == std::string::npos ||
      normalized.find(':', separator + 1) != std::string::npos) {
    return std::unexpected{"Virtual path must use mount:/relative syntax"};
  }
  std::string mount_name = normalized.substr(0, separator);
  std::ranges::transform(mount_name, mount_name.begin(),
                         [](const unsigned char character) {
                           return static_cast<char>(std::tolower(character));
                         });
  if (!valid_mount_name(mount_name)) {
    return std::unexpected{"Virtual path mount name is invalid"};
  }

  std::string relative;
  std::size_t begin = separator + 2;
  while (begin <= normalized.size()) {
    const std::size_t end = normalized.find('/', begin);
    const std::string_view part{
        normalized.data() + begin,
        (end == std::string::npos ? normalized.size() : end) - begin};
    if (part == "..") {
      return std::unexpected{"Virtual path cannot leave its mount"};
    }
    if (!part.empty() && part != ".") {
      if (!relative.empty()) {
        relative.push_back('/');
      }
      relative.append(part);
    }
    if (end == std::string::npos) {
      break;
    }
    begin = end + 1;
  }
  if (relative.empty()) {
    return std::unexpected{"Virtual path must identify a file"};
  }
  return VirtualPath{mount_name + ":/" + relative, mount_name.size()};
}

std::string_view VirtualPath::string() const noexcept { return normalized_; }

std::string_view VirtualPath::mount() const noexcept {
  return std::string_view{normalized_}.substr(0, mount_length_);
}

std::string_view VirtualPath::relative() const noexcept {
  return std::string_view{normalized_}.substr(mount_length_ + 2);
}

std::size_t AssetIdHash::operator()(const AssetId id) const noexcept {
  return static_cast<std::size_t>(id.value ^ (id.value >> 32));
}

AssetId make_asset_id(const VirtualPath &path, const AssetType type) noexcept {
  std::uint64_t hash = fnv_offset;
  hash ^= static_cast<std::uint8_t>(type);
  hash *= fnv_prime;
  hash = hash_bytes(
      std::as_bytes(std::span{path.string().data(), path.string().size()}),
      hash);
  return {hash == 0 ? 1 : hash};
}

std::uint64_t fingerprint(const std::span<const std::byte> bytes) noexcept {
  return hash_bytes(bytes);
}

void VirtualFileSystem::mount(const std::string_view name,
                              const std::filesystem::path &root) {
  std::string normalized_name{name};
  std::ranges::transform(normalized_name, normalized_name.begin(),
                         [](const unsigned char character) {
                           return static_cast<char>(std::tolower(character));
                         });
  std::error_code error;
  const auto canonical_root = std::filesystem::weakly_canonical(root, error);
  if (!valid_mount_name(normalized_name) || error || canonical_root.empty() ||
      !std::filesystem::is_directory(canonical_root, error) || error ||
      mounts_.contains(normalized_name)) {
    throw std::invalid_argument{
        "Virtual filesystem mount is invalid or duplicate"};
  }
  mounts_.emplace(std::move(normalized_name), canonical_root);
}

std::expected<std::filesystem::path, std::string>
VirtualFileSystem::resolve(const VirtualPath &path) const {
  const auto found = mounts_.find(std::string{path.mount()});
  if (found == mounts_.end()) {
    return std::unexpected{"Virtual path uses an unmounted root"};
  }
  std::error_code error;
  const auto candidate = std::filesystem::weakly_canonical(
      found->second / std::filesystem::path{path.relative()}, error);
  if (error || !is_within(found->second, candidate)) {
    return std::unexpected{"Resolved asset path escapes its mount"};
  }
  return candidate;
}

std::expected<std::vector<std::byte>, std::string>
VirtualFileSystem::read(const VirtualPath &path) const {
  const auto resolved = resolve(path);
  if (!resolved) {
    return std::unexpected{resolved.error()};
  }
  std::error_code error;
  const std::uintmax_t size = std::filesystem::file_size(*resolved, error);
  if (error || size > maximum_asset_size ||
      size > static_cast<std::uintmax_t>(
                 std::numeric_limits<std::size_t>::max())) {
    return std::unexpected{"Asset file is missing or exceeds the size limit"};
  }
  std::ifstream stream{*resolved, std::ios::binary};
  if (!stream) {
    return std::unexpected{"Asset file could not be opened"};
  }
  std::vector<std::byte> bytes(static_cast<std::size_t>(size));
  stream.read(reinterpret_cast<char *>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
  if (!stream && !bytes.empty()) {
    return std::unexpected{"Asset file could not be read completely"};
  }
  return bytes;
}

std::expected<void, std::string>
VirtualFileSystem::write(const VirtualPath &path,
                         const std::span<const std::byte> bytes) const {
  if (bytes.size() > maximum_asset_size) {
    return std::unexpected{"Asset output exceeds the size limit"};
  }
  const auto resolved = resolve(path);
  if (!resolved) {
    return std::unexpected{resolved.error()};
  }
  std::error_code error;
  std::filesystem::create_directories(resolved->parent_path(), error);
  if (error) {
    return std::unexpected{"Asset output directory could not be created"};
  }
  std::ofstream stream{*resolved, std::ios::binary | std::ios::trunc};
  if (!stream) {
    return std::unexpected{"Asset output could not be opened"};
  }
  stream.write(reinterpret_cast<const char *>(bytes.data()),
               static_cast<std::streamsize>(bytes.size()));
  if (!stream) {
    return std::unexpected{"Asset output could not be written completely"};
  }
  return {};
}

void AssetCatalog::add(AssetRecord record) {
  if (record.id.value == 0 || !valid_type(record.type) ||
      record.id != make_asset_id(record.source, record.type) ||
      std::ranges::find(record.dependencies, record.id) !=
          record.dependencies.end() ||
      records_.contains(record.id)) {
    throw std::invalid_argument{"Asset catalog record is invalid or duplicate"};
  }
  std::ranges::sort(record.dependencies, {}, &AssetId::value);
  if (std::ranges::adjacent_find(record.dependencies) !=
      record.dependencies.end()) {
    throw std::invalid_argument{
        "Asset catalog dependencies contain duplicates"};
  }
  records_.emplace(record.id, std::move(record));
}

void AssetCatalog::upsert(AssetRecord record) {
  if (record.id.value == 0 || !valid_type(record.type) ||
      record.id != make_asset_id(record.source, record.type) ||
      std::ranges::find(record.dependencies, record.id) != record.dependencies.end()) {
    throw std::invalid_argument{"Asset catalog replacement record is invalid"};
  }
  std::ranges::sort(record.dependencies, {}, &AssetId::value);
  if (std::ranges::adjacent_find(record.dependencies) != record.dependencies.end()) {
    throw std::invalid_argument{"Asset catalog dependencies contain duplicates"};
  }
  records_.insert_or_assign(record.id, std::move(record));
}

const AssetRecord *AssetCatalog::find(const AssetId id) const noexcept {
  const auto found = records_.find(id);
  return found == records_.end() ? nullptr : &found->second;
}

std::expected<std::vector<AssetId>, std::string>
AssetCatalog::dependency_order() const {
  enum class Visit : std::uint8_t { visiting, complete };
  std::unordered_map<AssetId, Visit, AssetIdHash> visits;
  std::vector<AssetId> order;
  order.reserve(records_.size());
  const auto visit = [&](const auto &self,
                         const AssetId id) -> std::expected<void, std::string> {
    if (const auto found = visits.find(id); found != visits.end()) {
      return found->second == Visit::complete
                 ? std::expected<void, std::string>{}
                 : std::unexpected{"Asset dependency graph contains a cycle"};
    }
    const AssetRecord *record = find(id);
    if (record == nullptr) {
      return std::unexpected{
          "Asset dependency graph references a missing asset"};
    }
    visits.emplace(id, Visit::visiting);
    for (const AssetId dependency : record->dependencies) {
      if (auto result = self(self, dependency); !result) {
        return result;
      }
    }
    visits[id] = Visit::complete;
    order.push_back(id);
    return {};
  };
  for (const auto &[id, record] : records_) {
    static_cast<void>(record);
    if (auto result = visit(visit, id); !result) {
      return std::unexpected{result.error()};
    }
  }
  return order;
}

std::size_t AssetCatalog::size() const noexcept { return records_.size(); }

std::vector<std::byte> encode_cooked_asset(const CookedAsset &asset) {
  if (asset.id.value == 0 || !valid_type(asset.type) ||
      asset.dependencies.size() > maximum_dependencies ||
      asset.payload.size() > maximum_asset_size) {
    throw std::invalid_argument{"Cooked asset is invalid"};
  }
  std::vector<std::byte> encoded;
  encoded.reserve(cooked_fixed_header_size +
                  asset.dependencies.size() * sizeof(std::uint64_t) +
                  asset.payload.size());
  encoded.insert(encoded.end(), cooked_magic.begin(), cooked_magic.end());
  append_integer(encoded, cooked_asset_version);
  append_integer(encoded, static_cast<std::uint8_t>(asset.type));
  append_integer(encoded, std::uint8_t{0});
  append_integer(encoded, std::uint16_t{0});
  append_integer(encoded, asset.id.value);
  append_integer(encoded, asset.source_fingerprint);
  append_integer(encoded, fingerprint(asset.payload));
  append_integer(encoded,
                 static_cast<std::uint32_t>(asset.dependencies.size()));
  append_integer(encoded, static_cast<std::uint64_t>(asset.payload.size()));
  for (const AssetId dependency : asset.dependencies) {
    append_integer(encoded, dependency.value);
  }
  encoded.insert(encoded.end(), asset.payload.begin(), asset.payload.end());
  return encoded;
}

std::expected<CookedAsset, std::string>
decode_cooked_asset(const std::span<const std::byte> encoded) {
  if (encoded.size() < cooked_fixed_header_size ||
      !std::ranges::equal(cooked_magic, encoded.first(cooked_magic.size()))) {
    return std::unexpected{"Cooked asset header is invalid"};
  }
  std::size_t offset = cooked_magic.size();
  const auto version = read_integer<std::uint32_t>(encoded, offset);
  const auto type =
      static_cast<AssetType>(read_integer<std::uint8_t>(encoded, offset));
  const auto reserved_byte = read_integer<std::uint8_t>(encoded, offset);
  const auto reserved_word = read_integer<std::uint16_t>(encoded, offset);
  CookedAsset asset;
  asset.type = type;
  asset.id.value = read_integer<std::uint64_t>(encoded, offset);
  asset.source_fingerprint = read_integer<std::uint64_t>(encoded, offset);
  const auto payload_fingerprint = read_integer<std::uint64_t>(encoded, offset);
  const std::size_t dependency_count =
      read_integer<std::uint32_t>(encoded, offset);
  const std::uint64_t payload_size =
      read_integer<std::uint64_t>(encoded, offset);
  if (version != cooked_asset_version || !valid_type(type) ||
      asset.id.value == 0 || reserved_byte != 0 || reserved_word != 0 ||
      dependency_count > maximum_dependencies ||
      payload_size > maximum_asset_size ||
      dependency_count > (encoded.size() - offset) / sizeof(std::uint64_t)) {
    return std::unexpected{"Cooked asset metadata is invalid"};
  }
  const std::size_t dependency_bytes = dependency_count * sizeof(std::uint64_t);
  if (payload_size != encoded.size() - offset - dependency_bytes) {
    return std::unexpected{"Cooked asset size does not match its header"};
  }
  asset.dependencies.reserve(dependency_count);
  for (std::size_t index = 0; index < dependency_count; ++index) {
    const AssetId dependency{read_integer<std::uint64_t>(encoded, offset)};
    if (dependency.value == 0 || dependency == asset.id) {
      return std::unexpected{"Cooked asset dependency is invalid"};
    }
    asset.dependencies.push_back(dependency);
  }
  if (!std::ranges::is_sorted(asset.dependencies, {}, &AssetId::value) ||
      std::ranges::adjacent_find(asset.dependencies) !=
          asset.dependencies.end()) {
    return std::unexpected{"Cooked asset dependencies are not canonical"};
  }
  asset.payload.assign(encoded.begin() + static_cast<std::ptrdiff_t>(offset),
                       encoded.end());
  if (fingerprint(asset.payload) != payload_fingerprint) {
    return std::unexpected{"Cooked asset payload checksum failed"};
  }
  return asset;
}

} // namespace gloom::assets
