#include <gloom/assets/asset.hpp>

#include <array>
#include <fstream>
#include <limits>

#include <type_traits>
#include <stdlib.h>
#include <string.h>
#include <utility>

namespace gloom::assets {
namespace {

constexpr uint64 fnv_offset = 14'695'981'039'346'656'037ULL;
constexpr uint64 fnv_prime = 1'099'511'628'211ULL;
constexpr std::array<std::byte, 8> cooked_magic{
    std::byte{'G'}, std::byte{'L'}, std::byte{'O'}, std::byte{'O'}, std::byte{'M'}, std::byte{'A'}, std::byte{'S'}, std::byte{0}};
constexpr size_t cooked_fixed_header_size = 52;
constexpr size_t maximum_asset_size = 1024ULL * 1024ULL * 1024ULL;
constexpr size_t maximum_dependencies = 65'536;

[[nodiscard]] bool valid_mount_name(const std::string_view name) noexcept {
    if (name.size() < 2)
        return false;
    for (unsigned char character : name)
        if (!((character >= 'a' && character <= 'z') || (character >= '0' && character <= '9') || character == '_' || character == '-'))
            return false;
    return true;
}

bool valid_path_utf8(std::string_view path) {
    for (size_t i = 0; i < path.size();) {
        uint32 code = static_cast<uint8>(path[i++]);
        if (code < 0x80)
            continue;
        uint32 continuation = 0, minimum = 0;
        if (code >= 0xC2 && code <= 0xDF) {
            continuation = 1;
            minimum = 0x80;
            code &= 0x1F;
        } else if (code >= 0xE0 && code <= 0xEF) {
            continuation = 2;
            minimum = 0x800;
            code &= 0x0F;
        } else if (code >= 0xF0 && code <= 0xF4) {
            continuation = 3;
            minimum = 0x10000;
            code &= 7;
        } else
            return false;
        if (continuation > path.size() - i)
            return false;
        while (continuation--) {
            const uint8 byte = static_cast<uint8>(path[i++]);
            if ((byte & 0xC0) != 0x80)
                return false;
            code = (code << 6) | (byte & 0x3F);
        }
        if (code < minimum || code > 0x10FFFF || (code >= 0xD800 && code <= 0xDFFF))
            return false;
    }
    return true;
}

[[nodiscard]] bool valid_type(const AssetType type) noexcept {
    return type >= AssetType::binary && type <= AssetType::texture;
}

[[nodiscard]] uint64 hash_bytes(const Span<const std::byte> bytes, uint64 hash = fnv_offset) noexcept {
    for (const std::byte value : bytes) {
        hash ^= std::to_integer<uint8>(value);
        hash *= fnv_prime;
    }
    return hash;
}

[[nodiscard]] bool is_within(const std::filesystem::path& root, const std::filesystem::path& candidate) {
    std::filesystem::path::const_iterator root_part = root.begin();
    std::filesystem::path::const_iterator candidate_part = candidate.begin();
    for (; root_part != root.end(); ++root_part, ++candidate_part) {
        if (candidate_part == candidate.end() || *candidate_part != *root_part) {
            return false;
        }
    }
    return true;
}

template <typename Integer> void append_integer(std::vector<std::byte>& output, const Integer value) {
    static_assert(std::is_unsigned_v<Integer>);
    for (size_t index = 0; index < sizeof(Integer); ++index) {
        output.push_back(static_cast<std::byte>(value >> (index * 8)));
    }
}

template <typename Integer> [[nodiscard]] Integer read_integer(const Span<const std::byte> input, size_t& offset) {
    static_assert(std::is_unsigned_v<Integer>);
    Integer value = 0;
    for (size_t index = 0; index < sizeof(Integer); ++index) {
        value |= static_cast<Integer>(std::to_integer<unsigned int>(input[offset++])) << (index * 8);
    }
    return value;
}

} // namespace

VirtualPath::VirtualPath(std::string normalized, const size_t mount_length) : normalized_{std::move(normalized)}, mount_length_{mount_length} {}

std::expected<VirtualPath, std::string> VirtualPath::parse(const std::string_view path) {
    if (path.empty() || path.find('\0') != std::string_view::npos) {
        return std::unexpected{"Virtual path is empty or contains a null byte"};
    }
    if (!valid_path_utf8(path))
        return std::unexpected{"Virtual path is not valid UTF-8"};
    std::string normalized{path};
    for (char& character : normalized)
        if (character == '\\')
            character = '/';
    const size_t separator = normalized.find(":/");
    if (separator == std::string::npos || normalized.find(':', separator + 1) != std::string::npos) {
        return std::unexpected{"Virtual path must use mount:/relative syntax"};
    }
    std::string mount_name = normalized.substr(0, separator);
    for (char& character : mount_name)
        if (character >= 'A' && character <= 'Z')
            character += 'a' - 'A';
    if (!valid_mount_name(mount_name)) {
        return std::unexpected{"Virtual path mount name is invalid"};
    }

    std::string relative;
    size_t begin = separator + 2;
    while (begin <= normalized.size()) {
        const size_t end = normalized.find('/', begin);
        const std::string_view part{normalized.data() + begin, (end == std::string::npos ? normalized.size() : end) - begin};
        if (part == "..") {
            return std::unexpected{"Virtual path cannot leave its mount"};
        }
        if (!part.empty() && part != ".") {
#ifdef _WIN32
            // Win32 trims these suffixes when opening a file; reject alternate spellings of parent components.
            if (part.back() == '.' || part.back() == ' ')
                return std::unexpected{"Virtual path component has a Win32-normalized suffix"};
#endif
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

std::string_view VirtualPath::string() const noexcept {
    return normalized_;
}

std::string_view VirtualPath::mount() const noexcept {
    return std::string_view{normalized_}.substr(0, mount_length_);
}

std::string_view VirtualPath::relative() const noexcept {
    return std::string_view{normalized_}.substr(mount_length_ + 2);
}

size_t AssetIdHash::operator()(const AssetId id) const noexcept {
    return static_cast<size_t>(id.value ^ (id.value >> 32));
}

AssetId make_asset_id(const VirtualPath& path, const AssetType type) noexcept {
    uint64 hash = fnv_offset;
    hash ^= static_cast<uint8>(type);
    hash *= fnv_prime;
    hash = hash_bytes({reinterpret_cast<const std::byte*>(path.string().data()), path.string().size()}, hash);
    return {.value = hash == 0 ? 1 : hash};
}

uint64 fingerprint(const Span<const std::byte> bytes) noexcept {
    return hash_bytes(bytes);
}

std::expected<void, std::string> VirtualFileSystem::mount(const std::string_view name, const std::filesystem::path& root) {
    std::string normalized_name{name};
    for (char& character : normalized_name)
        if (character >= 'A' && character <= 'Z')
            character += 'a' - 'A';
    std::error_code error;
    const std::filesystem::path canonical_root = std::filesystem::weakly_canonical(root, error);
    if (!valid_mount_name(normalized_name) || error || canonical_root.empty() || !std::filesystem::is_directory(canonical_root, error) || error)
        return std::unexpected{"Virtual filesystem mount name or directory is invalid"};
    for (const Mount& mounted : mounts_)
        if (mounted.name == normalized_name)
            return std::unexpected{"Virtual filesystem mount is duplicate"};
    mounts_.push_back({.name = std::move(normalized_name), .root = canonical_root});
    return {};
}

std::expected<std::filesystem::path, std::string> VirtualFileSystem::resolve(const VirtualPath& path) const {
    const Mount* found = nullptr;
    for (const Mount& mounted : mounts_)
        if (mounted.name == path.mount()) {
            found = &mounted;
            break;
        }
    if (!found) {
        return std::unexpected{"Virtual path uses an unmounted root"};
    }
    std::error_code error;
    const std::filesystem::path candidate = std::filesystem::weakly_canonical(
        found->root / std::filesystem::path{std::u8string_view{reinterpret_cast<const char8_t*>(path.relative().data()), path.relative().size()}}, error);
    if (error || !is_within(found->root, candidate)) {
        return std::unexpected{"Resolved asset path escapes its mount"};
    }
    return candidate;
}

std::expected<std::vector<std::byte>, std::string> VirtualFileSystem::read(const VirtualPath& path) const {
    const std::expected<std::filesystem::path, std::string> resolved = resolve(path);
    if (!resolved) {
        return std::unexpected{resolved.error()};
    }
    std::error_code error;
    const uint64 size = std::filesystem::file_size(*resolved, error);
    if (error || size > maximum_asset_size || size > static_cast<uint64>(std::numeric_limits<size_t>::max())) {
        return std::unexpected{"Asset file is missing or exceeds the size limit"};
    }
    std::ifstream stream{*resolved, std::ios::binary};
    if (!stream) {
        return std::unexpected{"Asset file could not be opened"};
    }
    std::vector<std::byte> bytes(static_cast<size_t>(size));
    stream.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!stream && !bytes.empty()) {
        return std::unexpected{"Asset file could not be read completely"};
    }
    return bytes;
}

std::expected<void, std::string> VirtualFileSystem::write(const VirtualPath& path, const Span<const std::byte> bytes) const {
    if (bytes.size() > maximum_asset_size) {
        return std::unexpected{"Asset output exceeds the size limit"};
    }
    const std::expected<std::filesystem::path, std::string> resolved = resolve(path);
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
    stream.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    stream.close();
    if (!stream) {
        return std::unexpected{"Asset output could not be written completely"};
    }
    return {};
}

namespace {
int compare_asset_ids(const void* a, const void* b) {
    const uint64 first = static_cast<const AssetId*>(a)->value, second = static_cast<const AssetId*>(b)->value;
    return first < second ? -1 : first > second ? 1 : 0;
}
void validate_record(AssetRecord& record) {
    assert(record.id.value && valid_type(record.type) && record.id == make_asset_id(record.source, record.type));
#ifndef NDEBUG
    for (AssetId dependency : record.dependencies)
        assert(dependency != record.id);
#endif
    if (record.dependencies.size() > 1)
        qsort(record.dependencies.data(), record.dependencies.size(), sizeof(AssetId), compare_asset_ids);
    for (size_t i = 1; i < record.dependencies.size(); ++i)
        assert(record.dependencies[i - 1] != record.dependencies[i]);
}
}

AssetCatalog::~AssetCatalog() {
    for (const Entry& entry : records_)
        delete entry.record;
}
AssetCatalog& AssetCatalog::operator=(AssetCatalog&& other) noexcept {
    if (this != &other) {
        for (const Entry& entry : records_)
            delete entry.record;
        records_ = static_cast<Array<Entry>&&>(other.records_);
    }
    return *this;
}
size_t AssetCatalog::lower_bound(AssetId id) const noexcept {
    size_t first = 0, last = records_.size();
    while (first < last) {
        const size_t middle = first + (last - first) / 2;
        if (records_[middle].id.value < id.value)
            first = middle + 1;
        else
            last = middle;
    }
    return first;
}
void AssetCatalog::add(AssetRecord record) {
    static_assert(__is_trivially_copyable(Entry));
    validate_record(record);
    const size_t position = lower_bound(record.id);
    assert(position == records_.size() || records_[position].id != record.id);
    records_.reserve(records_.size() + 1);
    records_.resize(records_.size() + 1);
    memmove(records_.data() + position + 1, records_.data() + position, (records_.size() - position - 1) * sizeof(Entry));
    records_[position] = {.id = record.id, .record = new AssetRecord(static_cast<AssetRecord&&>(record))};
}
void AssetCatalog::upsert(AssetRecord record) {
    const size_t position = lower_bound(record.id);
    if (position == records_.size() || records_[position].id != record.id) {
        add(static_cast<AssetRecord&&>(record));
        return;
    }
    validate_record(record);
    *records_[position].record = static_cast<AssetRecord&&>(record);
}
const AssetRecord* AssetCatalog::find(AssetId id) const noexcept {
    const size_t position = lower_bound(id);
    return position < records_.size() && records_[position].id == id ? records_[position].record : nullptr;
}
bool AssetCatalog::dependency_order(Array<AssetId>& order, const char*& error) const {
    order.resize(0);
    error = nullptr;
    order.reserve(records_.size());
    Array<uint8> visits;
    visits.reserve(records_.size());
    visits.resize(records_.size());
    struct Frame {
        size_t record{0}, dependency{0};
    };
    Array<Frame> stack;
    stack.reserve(records_.size());
    for (size_t root = 0; root < records_.size(); ++root) {
        if (visits[root])
            continue;
        visits[root] = 1;
        stack.push_back({.record = root});
        while (stack.size()) {
            Frame& frame = stack[stack.size() - 1];
            const AssetRecord& record = *records_[frame.record].record;
            if (frame.dependency == record.dependencies.size()) {
                visits[frame.record] = 2;
                order.push_back(record.id);
                stack.resize(stack.size() - 1);
                continue;
            }
            const AssetId dependency = record.dependencies[frame.dependency++];
            const size_t index = lower_bound(dependency);
            if (index == records_.size() || records_[index].id != dependency) {
                error = "Asset dependency graph references a missing asset";
                order.resize(0);
                return false;
            }
            if (visits[index] == 1) {
                error = "Asset dependency graph contains a cycle";
                order.resize(0);
                return false;
            }
            if (!visits[index]) {
                visits[index] = 1;
                stack.push_back({.record = index});
            }
        }
    }
    return true;
}
size_t AssetCatalog::size() const noexcept {
    return records_.size();
}

std::vector<std::byte> encode_cooked_asset(const CookedAsset& asset) {
    assert(asset.id.value && valid_type(asset.type) && asset.dependencies.size() <= maximum_dependencies && asset.payload.size() <= maximum_asset_size);
#ifndef NDEBUG
    for (size_t i = 0; i < asset.dependencies.size(); ++i)
        assert(asset.dependencies[i].value && asset.dependencies[i] != asset.id && (!i || asset.dependencies[i - 1].value < asset.dependencies[i].value));
#endif
    std::vector<std::byte> encoded;
    encoded.reserve(cooked_fixed_header_size + asset.dependencies.size() * sizeof(uint64) + asset.payload.size());
    encoded.insert(encoded.end(), cooked_magic.begin(), cooked_magic.end());
    append_integer(encoded, cooked_asset_version);
    append_integer(encoded, static_cast<uint8>(asset.type));
    append_integer(encoded, uint8{0});
    append_integer(encoded, uint16{0});
    append_integer(encoded, asset.id.value);
    append_integer(encoded, asset.source_fingerprint);
    append_integer(encoded, fingerprint(asset.payload));
    append_integer(encoded, static_cast<uint32>(asset.dependencies.size()));
    append_integer(encoded, static_cast<uint64>(asset.payload.size()));
    for (const AssetId dependency : asset.dependencies) {
        append_integer(encoded, dependency.value);
    }
    encoded.insert(encoded.end(), asset.payload.begin(), asset.payload.end());
    return encoded;
}

std::expected<CookedAsset, std::string> decode_cooked_asset(const Span<const std::byte> encoded) {
    if (encoded.size() < cooked_fixed_header_size || memcmp(cooked_magic.data(), encoded.data(), cooked_magic.size()) != 0) {
        return std::unexpected{"Cooked asset header is invalid"};
    }
    size_t offset = cooked_magic.size();
    const uint32 version = read_integer<uint32>(encoded, offset);
    const AssetType type = static_cast<AssetType>(read_integer<uint8>(encoded, offset));
    const uint8 reserved_byte = read_integer<uint8>(encoded, offset);
    const uint16 reserved_word = read_integer<uint16>(encoded, offset);
    CookedAsset asset;
    asset.type = type;
    asset.id.value = read_integer<uint64>(encoded, offset);
    asset.source_fingerprint = read_integer<uint64>(encoded, offset);
    const uint64 payload_fingerprint = read_integer<uint64>(encoded, offset);
    const size_t dependency_count = read_integer<uint32>(encoded, offset);
    const uint64 payload_size = read_integer<uint64>(encoded, offset);
    if (version != cooked_asset_version || !valid_type(type) || asset.id.value == 0 || reserved_byte != 0 || reserved_word != 0 ||
        dependency_count > maximum_dependencies || payload_size > maximum_asset_size || dependency_count > (encoded.size() - offset) / sizeof(uint64)) {
        return std::unexpected{"Cooked asset metadata is invalid"};
    }
    const size_t dependency_bytes = dependency_count * sizeof(uint64);
    if (payload_size != encoded.size() - offset - dependency_bytes) {
        return std::unexpected{"Cooked asset size does not match its header"};
    }
    asset.dependencies.reserve(dependency_count);
    for (size_t index = 0; index < dependency_count; ++index) {
        const AssetId dependency{.value = read_integer<uint64>(encoded, offset)};
        if (dependency.value == 0 || dependency == asset.id) {
            return std::unexpected{"Cooked asset dependency is invalid"};
        }
        asset.dependencies.push_back(dependency);
    }
    for (size_t i = 1; i < asset.dependencies.size(); ++i)
        if (asset.dependencies[i - 1].value >= asset.dependencies[i].value)
            return std::unexpected{"Cooked asset dependencies are not canonical"};
    if (fingerprint(encoded.subspan(offset)) != payload_fingerprint) {
        return std::unexpected{"Cooked asset payload checksum failed"};
    }
    asset.payload.assign(encoded.begin() + static_cast<std::ptrdiff_t>(offset), encoded.end());
    return asset;
}

} // namespace gloom::assets
