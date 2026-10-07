#include <gloom/assets/asset_loader.hpp>
#include <string.h>

namespace gloom::assets {
void set_asset_error(Array<char>& output, const char* message) {
    const size_t size = strlen(message) + 1;
    output.reserve(size);
    output.resize(size);
    memcpy(output.data(), message, size);
}
AssetLoadHandle::~AssetLoadHandle() {
    if (owner_)
        owner_->release(ticket_);
}
AssetLoadHandle::AssetLoadHandle(const AssetLoadHandle& other) : owner_{other.owner_}, ticket_{other.ticket_} {
    if (owner_)
        owner_->retain(ticket_);
}
AssetLoadHandle& AssetLoadHandle::operator=(const AssetLoadHandle& other) {
    if (this == &other)
        return *this;
    if (owner_)
        owner_->release(ticket_);
    owner_ = other.owner_;
    ticket_ = other.ticket_;
    if (owner_)
        owner_->retain(ticket_);
    return *this;
}
AssetLoadHandle::AssetLoadHandle(AssetLoadHandle&& other) noexcept : owner_{other.owner_}, ticket_{other.ticket_} {
    other.owner_ = nullptr;
}
AssetLoadHandle& AssetLoadHandle::operator=(AssetLoadHandle&& other) noexcept {
    if (this == &other)
        return *this;
    if (owner_)
        owner_->release(ticket_);
    owner_ = other.owner_;
    ticket_ = other.ticket_;
    other.owner_ = nullptr;
    return *this;
}
bool AssetLoadHandle::ready() const {
    return owner_ && owner_->ready(ticket_);
}
const AssetLoadResult& AssetLoadHandle::get() const {
    assert(owner_);
    return owner_->result(ticket_);
}
struct AsyncAssetLoader::Slot {
    uint32 pins{1};
    bool cached{true}, pending{true};
    AssetLoadResult result;
};
AsyncAssetLoader::AsyncAssetLoader(core::JobSystem& jobs, const VirtualFileSystem& filesystem, const AssetCatalog& catalog)
    : jobs_{jobs}, filesystem_{filesystem}, catalog_{catalog}, tasks_{jobs.create_group()} {}
AsyncAssetLoader::~AsyncAssetLoader() {
    wait();
    for (Storage& storage : slots_) {
        assert(!storage.slot || storage.slot->pins == 0);
        delete storage.slot;
    }
}
size_t AsyncAssetLoader::lower_bound(AssetId id) const {
    size_t begin = 0, end = cache_.size();
    while (begin < end) {
        const size_t middle = begin + (end - begin) / 2;
        if (cache_[middle].id.value < id.value)
            begin = middle + 1;
        else
            end = middle;
    }
    return begin;
}
AsyncAssetLoader::Slot& AsyncAssetLoader::slot_locked(AssetLoadTicket ticket) const {
    assert(ticket.index < slots_.size() && slots_[ticket.index].generation == ticket.generation && slots_[ticket.index].slot);
    return *slots_[ticket.index].slot;
}
void AsyncAssetLoader::dispose_locked(uint32 index) {
    Slot& slot = *slots_[index].slot;
    if (slot.cached || slot.pending || slot.pins)
        return;
    delete slots_[index].slot;
    slots_[index].slot = nullptr;
    slots_[index].next = free_;
    free_ = index;
}
void AsyncAssetLoader::retain(AssetLoadTicket ticket) {
    AcquireSRWLockExclusive(&mutex_);
    ++slot_locked(ticket).pins;
    ReleaseSRWLockExclusive(&mutex_);
}
void AsyncAssetLoader::release(AssetLoadTicket ticket) {
    AcquireSRWLockExclusive(&mutex_);
    Slot& slot = slot_locked(ticket);
    assert(slot.pins);
    --slot.pins;
    dispose_locked(ticket.index);
    ReleaseSRWLockExclusive(&mutex_);
}
bool AsyncAssetLoader::ready(AssetLoadTicket ticket) const {
    AcquireSRWLockShared(&mutex_);
    const bool ready = !slot_locked(ticket).pending;
    ReleaseSRWLockShared(&mutex_);
    return ready;
}
const AssetLoadResult& AsyncAssetLoader::result(AssetLoadTicket ticket) const {
    AcquireSRWLockShared(&mutex_);
    const Slot& slot = slot_locked(ticket);
    assert(!slot.pending);
    const AssetLoadResult* result = &slot.result;
    ReleaseSRWLockShared(&mutex_);
    return *result;
}
void AsyncAssetLoader::publish(AssetLoadTicket ticket, AssetLoadResult result) {
    AcquireSRWLockExclusive(&mutex_);
    Slot& slot = slot_locked(ticket);
    assert(slot.pending);
    slot.result = static_cast<AssetLoadResult&&>(result);
    slot.pending = false;
    if (!slot.cached)
        ++metrics_.stale_completions;
    if (slot.result.state == AssetLoadState::ready) {
        ++metrics_.loaded;
        metrics_.bytes_loaded += slot.result.asset.payload.size();
    } else
        ++metrics_.failed;
    dispose_locked(ticket.index);
    ReleaseSRWLockExclusive(&mutex_);
}
struct AsyncAssetLoader::LoadContext {
    AsyncAssetLoader* loader;
    AssetLoadTicket ticket;
    AssetRecord record; // Owned snapshot; later upserts cannot change the job's input.
    void operator()() noexcept {
        GLOOM_PROFILE_SCOPE(::gloom::AllocationPhase::assets);
        AssetLoadResult result;
        decltype(loader->filesystem_.read(record.cooked)) encoded = loader->filesystem_.read(record.cooked);
        if (!encoded)
            result = placeholder(record.id, record.type, encoded.error().c_str());
        else {
            decltype(decode_cooked_asset(*encoded)) decoded = decode_cooked_asset(*encoded);
            if (!decoded)
                result = placeholder(record.id, record.type, decoded.error().c_str());
            else if (decoded->id != record.id || decoded->type != record.type || decoded->source_fingerprint != record.source_fingerprint ||
                     decoded->dependencies != record.dependencies)
                result = placeholder(record.id, record.type, "Cooked asset does not match its catalog record");
            else
                result = {.state = AssetLoadState::ready, .asset = static_cast<CookedAsset&&>(*decoded)};
        }
        loader->publish(ticket, static_cast<AssetLoadResult&&>(result));
    }
};
AssetLoadHandle AsyncAssetLoader::request(AssetId id) {
    GLOOM_PROFILE_SCOPE(::gloom::AllocationPhase::assets);
    AcquireSRWLockExclusive(&mutex_);
    ++metrics_.requests;
    const size_t position = lower_bound(id);
    if (position < cache_.size() && cache_[position].id == id) {
        const uint32 index = cache_[position].index;
        ++slots_[index].slot->pins;
        ++metrics_.cache_hits;
        const AssetLoadTicket ticket{.index = index, .generation = slots_[index].generation};
        ReleaseSRWLockExclusive(&mutex_);
        return AssetLoadHandle{this, ticket};
    }
    uint32 index = free_;
    if (index == 0xffffffffU) {
        assert(slots_.size() < 0xffffffffU);
        index = static_cast<uint32>(slots_.size());
        slots_.reserve(slots_.size() + 1);
        slots_.resize(slots_.size() + 1);
    } else
        free_ = slots_[index].next;
    Storage& storage = slots_[index];
    assert(storage.generation != 0xffffffffU);
    ++storage.generation;
    storage.slot = new Slot;
    const AssetLoadTicket ticket{.index = index, .generation = storage.generation};
    cache_.reserve(cache_.size() + 1);
    cache_.resize(cache_.size() + 1);
    for (size_t entry = cache_.size() - 1; entry > position; --entry)
        cache_[entry] = cache_[entry - 1];
    cache_[position] = {.id = id, .index = index};
    const AssetRecord* record = catalog_.find(id);
    LoadContext* context = record ? new LoadContext{.loader = this, .ticket = ticket, .record = *record} : nullptr;
    if (!record) {
        storage.slot->pending = false;
        storage.slot->result = placeholder(id, AssetType::binary, "Asset is absent from the catalog");
        ++metrics_.failed;
    }
    // Saturated scheduling can execute another load: never hold its publication lock.
    ReleaseSRWLockExclusive(&mutex_);
    if (context)
        jobs_.schedule(tasks_, core::JobSystem::Job{context});
    return AssetLoadHandle{this, ticket};
}
bool AsyncAssetLoader::invalidate(AssetId id) {
    AcquireSRWLockExclusive(&mutex_);
    const size_t position = lower_bound(id);
    if (position < cache_.size() && cache_[position].id == id) {
        const uint32 index = cache_[position].index;
        slots_[index].slot->cached = false;
        for (size_t entry = position + 1; entry < cache_.size(); ++entry)
            cache_[entry - 1] = cache_[entry];
        cache_.resize(cache_.size() - 1);
        ++metrics_.invalidations;
        dispose_locked(index);
    }
    ReleaseSRWLockExclusive(&mutex_);
    return true;
}
void AsyncAssetLoader::wait() {
    jobs_.wait(tasks_);
}
AssetLoaderMetrics AsyncAssetLoader::metrics() const noexcept {
    AcquireSRWLockShared(&mutex_);
    const AssetLoaderMetrics result = metrics_;
    ReleaseSRWLockShared(&mutex_);
    return result;
}
AssetLoadResult AsyncAssetLoader::placeholder(AssetId id, AssetType type, const char* error) {
    AssetLoadResult result{.asset = {.id = id, .type = type}};
    const uint8 missing[]{'M', 'I', 'S', 'S', 'I', 'N', 'G', 0};
    result.asset.payload.resize(sizeof(missing));
    memcpy(result.asset.payload.data(), missing, sizeof(missing));
    set_asset_error(result.error, error);
    return result;
}
}
