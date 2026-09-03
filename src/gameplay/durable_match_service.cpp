#include <gloom/gameplay/match_discovery.hpp>
#include <gloom/gameplay/match_service_wire.hpp>

#include <fstream>
#include <iomanip>
#include <map>
#include <mutex>
#include <optional>
#include <sstream>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#endif

namespace gloom::gameplay {
namespace {

class DurableMatchService final : public SliceMatchService {
public:
    explicit DurableMatchService(std::filesystem::path file) : file_{std::move(file)} {}
    ~DurableMatchService() override {
#ifdef _WIN32
        if (lock_ != INVALID_HANDLE_VALUE) CloseHandle(lock_);
#else
        if (lock_ >= 0) close(lock_);
#endif
    }

    std::expected<void, std::string> load() {
        std::scoped_lock lock{mutex_};
        if (!file_.parent_path().empty()) std::filesystem::create_directories(file_.parent_path());
        auto lock_path = file_; lock_path += ".lock";
#ifdef _WIN32
        lock_ = CreateFileW(lock_path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                            OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (lock_ == INVALID_HANDLE_VALUE) return std::unexpected{"Match store is locked or inaccessible"};
#else
        lock_ = open(lock_path.c_str(), O_CREAT | O_RDWR, 0600);
        if (lock_ < 0 || flock(lock_, LOCK_EX | LOCK_NB) != 0)
            return std::unexpected{"Match store is locked or inaccessible"};
#endif
        if (!std::filesystem::exists(file_)) return save();
        if (std::filesystem::file_size(file_) > 4 * 1024 * 1024)
            return std::unexpected{"Durable match store is too large"};
        std::ifstream input{file_};
        if (!input) return std::unexpected{"Could not open durable match store"};
        std::string header;
        if (!std::getline(input, header) || header != "gloom-match-store-v1")
            return std::unexpected{"Unsupported durable match store header"};
        std::map<std::string, SliceStoredMatch, std::less<>> loaded;
        std::string line;
        while (std::getline(input, line)) {
            if (line.empty()) continue;
            std::istringstream row{line};
            SliceStoredMatch item;
            unsigned protocol = 0, phase = 0;
            auto& a = item.advertisement;
            if (!(row >> std::quoted(a.match_id) >> std::quoted(a.instance_id) >>
                  std::quoted(a.display_name) >> std::quoted(a.endpoint) >> protocol >>
                  a.player_count >> a.capacity >> phase >> a.revision >>
                  item.expires_at_ms >> std::quoted(item.mutation_id)) ||
                protocol > 65'535 || phase > 2) {
                return std::unexpected{"Corrupt durable match store record"};
            }
            a.protocol_version = static_cast<std::uint16_t>(protocol);
            a.phase = static_cast<SliceMatchPhase>(phase);
            row >> std::ws;
            if (!row.eof() || !validate_stored_match(item) || loaded.contains(a.match_id))
                return std::unexpected{"Corrupt durable match store record"};
            const auto id = a.match_id;
            loaded.emplace(id, std::move(item));
        }
        if (!input.eof()) return std::unexpected{"Could not read complete durable match store"};
        input.close(); // Windows cannot atomically replace the file while this reader owns it.
        records_ = std::move(loaded);
        return save(); // Readiness includes a successful durable write, not only a readable file.
    }

    std::expected<void, std::string> store(SliceStoredMatch item,
                                            const std::uint64_t now_ms) override {
        if (auto valid = validate_stored_match(item); !valid) return valid;
        std::scoped_lock lock{mutex_};
        const auto found = records_.find(item.advertisement.match_id);
        if (found != records_.end() && found->second.mutation_id == item.mutation_id) {
            return found->second == item ? std::expected<void, std::string>{}
                                         : std::unexpected{"Mutation id payload changed"};
        }
        if (found != records_.end() && found->second.expires_at_ms > now_ms) {
            if (found->second.advertisement.instance_id != item.advertisement.instance_id)
                return std::unexpected{"Match advertisement is owned by another instance"};
            if (found->second.advertisement.revision >= item.advertisement.revision)
                return std::unexpected{"Match advertisement revision is stale"};
        }
        const auto key = item.advertisement.match_id;
        if (found == records_.end() && records_.size() >= 1024)
            return std::unexpected{"Match store capacity reached"};
        const auto previous = found == records_.end() ? std::optional<SliceStoredMatch>{}
                                                       : std::optional<SliceStoredMatch>{found->second};
        records_.insert_or_assign(key, std::move(item));
        if (auto saved = save(); !saved) {
            if (previous) records_.insert_or_assign(key, std::move(*previous));
            else records_.erase(key);
            return saved;
        }
        return {};
    }

    std::expected<bool, std::string> erase(const std::string_view id,
                                            const std::string_view instance) override {
        std::scoped_lock lock{mutex_};
        const auto found = records_.find(std::string{id});
        if (found == records_.end() || found->second.advertisement.instance_id != instance)
            return false;
        auto previous = found->second;
        records_.erase(found);
        if (auto saved = save(); !saved) {
            records_.insert_or_assign(previous.advertisement.match_id, std::move(previous));
            return std::unexpected{saved.error()};
        }
        return true;
    }

    std::expected<std::vector<SliceStoredMatch>, std::string> list() const override {
        std::scoped_lock lock{mutex_};
        std::vector<SliceStoredMatch> result;
        for (const auto& [id, item] : records_) { static_cast<void>(id); result.push_back(item); }
        return result;
    }
    std::expected<SliceStoredMatch, std::string> get(const std::string_view id) const override {
        std::scoped_lock lock{mutex_};
        const auto found = records_.find(std::string{id});
        if (found == records_.end()) return std::unexpected{"Match is not stored"};
        return found->second;
    }

private:
    std::expected<void, std::string> save() const {
        std::error_code error;
        if (const auto parent = file_.parent_path(); !parent.empty())
            std::filesystem::create_directories(parent, error);
        if (error) return std::unexpected{"Could not create match-store directory"};
        auto temporary = file_; temporary += ".tmp";
        {
            std::ofstream output{temporary, std::ios::trunc};
            if (!output) return std::unexpected{"Could not write durable match store"};
            output << "gloom-match-store-v1\n";
            for (const auto& [id, item] : records_) {
                static_cast<void>(id); const auto& a = item.advertisement;
                output << std::quoted(a.match_id) << ' ' << std::quoted(a.instance_id) << ' '
                       << std::quoted(a.display_name) << ' ' << std::quoted(a.endpoint) << ' '
                       << a.protocol_version << ' ' << a.player_count << ' ' << a.capacity << ' '
                       << static_cast<unsigned>(a.phase) << ' ' << a.revision << ' '
                       << item.expires_at_ms << ' ' << std::quoted(item.mutation_id) << '\n';
            }
            output.flush();
            if (!output) return std::unexpected{"Could not flush durable match store"};
        }
#ifdef _WIN32
        const auto pending = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr,
                                         OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (pending == INVALID_HANDLE_VALUE) return std::unexpected{"Could not flush match store to disk"};
        const auto flushed = FlushFileBuffers(pending);
        CloseHandle(pending);
        if (!flushed) return std::unexpected{"Could not flush match store to disk"};
        if (!MoveFileExW(temporary.c_str(), file_.c_str(),
                         MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            return std::unexpected{"Could not atomically replace durable match store"};
#else
        std::filesystem::rename(temporary, file_, error);
        if (error) return std::unexpected{"Could not atomically replace durable match store"};
#endif
        return {};
    }

    std::filesystem::path file_;
#ifdef _WIN32
    HANDLE lock_{INVALID_HANDLE_VALUE};
#else
    int lock_{-1};
#endif
    mutable std::mutex mutex_;
    std::map<std::string, SliceStoredMatch, std::less<>> records_;
};

} // namespace

std::expected<std::shared_ptr<SliceMatchService>, std::string>
make_durable_match_service(const std::filesystem::path& storage_file) try {
    if (storage_file.empty()) return std::unexpected{"Durable match store path is empty"};
    auto service = std::make_shared<DurableMatchService>(storage_file);
    if (auto loaded = service->load(); !loaded) return std::unexpected{loaded.error()};
    return service;
} catch (const std::exception&) {
    return std::unexpected{"Could not initialize durable match store"};
}

} // namespace gloom::gameplay
