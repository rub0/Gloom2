#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <gloom/gameplay/match_service_wire.hpp>
#include <gloom/backends/match_request_budget.hpp>
#include <gloom/backends/match_reader_grants.hpp>
#include <gloom/backends/match_reader_credential.hpp>
#include <httplib.h>
#include <openssl/pem.h>
#include <openssl/x509v3.h>
#include <chrono>
#include <cmath>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>

namespace {
void expect(bool value, const char* message) {
    if (!value) throw std::runtime_error{message};
}
std::uint64_t now_ms() {
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}
void certificate(const std::filesystem::path& cert_path, const std::filesystem::path& key_path) {
    std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> key{
        EVP_PKEY_Q_keygen(nullptr, nullptr, "RSA", 2048), EVP_PKEY_free};
    std::unique_ptr<X509, decltype(&X509_free)> cert{X509_new(), X509_free};
    expect(key && cert, "Could not create ephemeral TLS key");
    X509_set_version(cert.get(), 2);
    ASN1_INTEGER_set(X509_get_serialNumber(cert.get()), 1);
    X509_gmtime_adj(X509_getm_notBefore(cert.get()), -60);
    X509_gmtime_adj(X509_getm_notAfter(cert.get()), 3600);
    X509_set_pubkey(cert.get(), key.get());
    auto* name = X509_get_subject_name(cert.get());
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
                              reinterpret_cast<const unsigned char*>("localhost"), -1, -1, 0);
    X509_set_issuer_name(cert.get(), name);
    X509V3_CTX context{};
    X509V3_set_ctx(&context, cert.get(), cert.get(), nullptr, nullptr, 0);
    for (const auto& [nid, value] : {std::pair{NID_basic_constraints, "critical,CA:TRUE"},
                                   std::pair{NID_subject_alt_name, "DNS:localhost,IP:127.0.0.1"}}) {
        auto* ext = X509V3_EXT_conf_nid(nullptr, &context, nid, value);
        expect(ext != nullptr, "Could not create TLS extension");
        X509_add_ext(cert.get(), ext, -1);
        X509_EXTENSION_free(ext);
    }
    expect(X509_sign(cert.get(), key.get(), EVP_sha256()) > 0, "Could not sign test certificate");
    auto* cert_file = BIO_new_file(cert_path.string().c_str(), "w");
    expect(cert_file && PEM_write_bio_X509(cert_file, cert.get()) == 1, "Could not write certificate");
    BIO_free(cert_file);
    auto* key_file = BIO_new_file(key_path.string().c_str(), "w");
    expect(key_file && PEM_write_bio_PrivateKey(key_file, key.get(), nullptr, nullptr, 0, nullptr, nullptr) == 1,
           "Could not write private key");
    BIO_free(key_file);
}

struct Process {
    HANDLE process{nullptr}, output{nullptr};
    int port{0};
    Process(const std::filesystem::path& executable, const std::filesystem::path& root,
            bool startup_failure = false) {
        SECURITY_ATTRIBUTES sa{sizeof(sa), nullptr, TRUE};
        HANDLE writer = nullptr;
        expect(CreatePipe(&output, &writer, &sa, 0) != 0, "CreatePipe failed");
        SetHandleInformation(output, HANDLE_FLAG_INHERIT, 0);
        STARTUPINFOW startup{};
        startup.cb = sizeof(startup);
        startup.dwFlags = STARTF_USESTDHANDLES;
        startup.hStdOutput = startup.hStdError = writer;
        startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        const auto quote = [](const std::filesystem::path& p) { return L"\"" + p.wstring() + L"\""; };
        std::wstring command = quote(executable) + L" 127.0.0.1 0 " + quote(root / "cert.pem") + L" " +
            quote(root / "key.pem") + L" " + quote(root / "matches.store") + L" --run-for-ms 10000";
        PROCESS_INFORMATION info{};
        const auto started = CreateProcessW(nullptr, command.data(), nullptr, nullptr, TRUE,
                                             CREATE_NO_WINDOW, nullptr, nullptr, &startup, &info);
        CloseHandle(writer);
        expect(started != 0, "Could not start HTTPS executable");
        process = info.hProcess;
        CloseHandle(info.hThread);
        std::string line;
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{10};
        while (std::chrono::steady_clock::now() < deadline && line.find('\n') == std::string::npos) {
            DWORD available = 0;
            if (PeekNamedPipe(output, nullptr, 0, nullptr, &available, nullptr) && available) {
                char buffer[4096]; DWORD count = 0;
                ReadFile(output, buffer, (std::min)(available, DWORD{sizeof(buffer)}), &count, nullptr);
                line.append(buffer, count);
            } else std::this_thread::sleep_for(std::chrono::milliseconds{10});
        }
        const auto marker = line.find("127.0.0.1:");
        if (startup_failure) {
            expect(marker == std::string::npos, "Invalid credentials opened a listener");
            expect(WaitForSingleObject(process, 2000) == WAIT_OBJECT_0, "Invalid startup did not exit");
            DWORD code = 0;
            GetExitCodeProcess(process, &code);
            expect(code != 0, "Invalid startup succeeded");
            return;
        }
        if (marker == std::string::npos) {
            TerminateProcess(process, 1);
            throw std::runtime_error{"Host failed startup: " + line};
        }
        port = std::stoi(line.substr(marker + 10));
    }
    void finish() {
        expect(WaitForSingleObject(process, 10'000) == WAIT_OBJECT_0, "Host did not shut down in time");
        DWORD code = 1;
        GetExitCodeProcess(process, &code);
        expect(code == 0, "Host exited with an error");
    }
    ~Process() {
        if (process) {
            if (WaitForSingleObject(process, 0) != WAIT_OBJECT_0) TerminateProcess(process, 1);
            CloseHandle(process);
        }
        if (output) CloseHandle(output);
    }
};

void grant_policy_tests(const std::filesystem::path& root) {
    using namespace gloom::backends;
    using Grants = MatchReaderGrants;
    const auto start = Grants::Clock::time_point{};
    auto verifier = [](std::string_view token, std::uint64_t) -> std::expected<MatchVerifiedPrincipal, std::string> {
        if (token == "individual-alice-credential" || token == "rotated-alice-credential")
            return MatchVerifiedPrincipal{"alice", 100'000};
        if (token == "individual-bob-credential") return MatchVerifiedPrincipal{"bob", 100'000};
        return std::unexpected{"sensitive-provider-error-must-stay-private"};
    };
    Grants grants{verifier};
    expect(!grants.exchange("untrusted-individual-credential", 1000, start), "Unknown identity accepted");
    auto alice = grants.exchange("individual-alice-credential", 1000, start);
    auto renewal = grants.exchange("rotated-alice-credential", 1000, start);
    auto bob = grants.exchange("individual-bob-credential", 1000, start);
    expect(alice && renewal && bob && alice->token != renewal->token && alice->lifetime_ms == 60'000,
           "Grant issuance failed");
    for (int i = 0; i < 20; ++i)
        expect(grants.consume(i % 2 ? alice->token : renewal->token, start) == MatchGrantAccess::allowed,
               "Principal burst was too small");
    expect(grants.consume(renewal->token, start) == MatchGrantAccess::throttled,
           "Multiple grants multiplied principal quota");
    const auto exhausted_renewal=grants.exchange("rotated-alice-credential",1000,start);
    expect(exhausted_renewal && grants.consume(exhausted_renewal->token,start)==MatchGrantAccess::throttled,
           "Renewal reset an exhausted principal quota");
    expect(grants.consume(bob->token, start) == MatchGrantAccess::allowed, "Principal quotas were not isolated");
    expect(grants.consume(alice->token, start + std::chrono::milliseconds{199}) == MatchGrantAccess::throttled,
           "Principal refilled early");
    expect(grants.consume(alice->token, start + std::chrono::milliseconds{200}) == MatchGrantAccess::allowed,
           "Principal did not refill");
    expect(grants.consume(alice->token, start + std::chrono::milliseconds{59'999}) == MatchGrantAccess::allowed &&
           grants.consume(alice->token, start + std::chrono::milliseconds{60'000}) == MatchGrantAccess::invalid,
           "Grant expiry boundary failed");
    expect(grants.revoke("alice").has_value() && !grants.exchange("individual-alice-credential", 1000, start),
           "Revoked principal could exchange again");
    expect(grants.consume(renewal->token, start) == MatchGrantAccess::invalid, "Revocation missed another grant");

    Grants bounded{verifier};
    for (int i = 0; i < 4; ++i)
        expect(bounded.exchange("individual-alice-credential", 1000, start).has_value(), "Grant cap too small");
    const auto fifth = bounded.exchange("individual-alice-credential", 1000, start + std::chrono::seconds{2});
    expect(!fifth && fifth.error() == 429, "Active grant cap exceeded");
    auto short_lived = bounded.exchange("individual-bob-credential", 99'950, start);
    expect(short_lived && short_lived->lifetime_ms == 50 &&
           bounded.consume(short_lived->token, start + std::chrono::milliseconds{50}) == MatchGrantAccess::invalid,
           "Grant outlived trusted identity");
    Grants concurrent{verifier};
    const auto token = concurrent.exchange("individual-alice-credential", 1000, start)->token;
    std::atomic_uint successes{0};
    {
        std::vector<std::jthread> workers;
        for (int i = 0; i < 8; ++i) workers.emplace_back([&] {
            for (int j = 0; j < 50; ++j)
                if (concurrent.consume(token, start) == MatchGrantAccess::allowed) ++successes;
        });
    }
    expect(successes == 20, "Concurrent grants exceeded principal quota");
    const auto path = root / "revocation-policy.store";
    {
        Grants durable{verifier, path};
        const auto issued = durable.exchange("individual-alice-credential", 1000, start);
        std::filesystem::create_directory(path.string() + ".tmp");
        const auto failed = durable.revoke("alice");
        expect(!failed && failed.error() == 503 &&
               durable.consume(issued->token, start) == MatchGrantAccess::invalid,
               "Failed durable revocation was acknowledged or left access active");
        std::filesystem::remove(path.string() + ".tmp");
        expect(durable.revoke("alice").has_value(), "Durable revocation retry failed");
    }
    Grants restarted{verifier, path};
    expect(!restarted.exchange("individual-alice-credential", 1000, start), "Restart lost suspension");
    expect(restarted.exchange("individual-bob-credential", 1000, start).has_value(), "Suspension blocked other identity");

    Grants capacity{[](std::string_view credential, std::uint64_t) {
        return std::expected<MatchVerifiedPrincipal, std::string>{MatchVerifiedPrincipal{std::string{credential}, 100'000}};
    }};
    for (int i = 0; i < 1024; ++i)
        expect(capacity.exchange("trusted-principal-" + std::to_string(i), 1000, start).has_value(), "Principal cap too small");
    const auto excess = capacity.exchange("trusted-principal-1024", 1000, start);
    expect(!excess && excess.error() == 503, "Principal memory cap exceeded");
    std::ofstream{path, std::ios::trunc} << "corrupt\n";
    bool refused = false;
    try { Grants corrupt{verifier, path}; } catch (const std::exception&) { refused = true; }
    expect(refused, "Corrupt revocations did not fail startup");

    MatchReaderCredential cache;
    auto time = start;
    unsigned calls = 0;
    const auto clock = [&] { return time; };
    const auto acquire = [&]() -> std::expected<std::string, std::string> {
        ++calls;
        return "gr1_" + std::string(64, static_cast<char>('a' + calls)) + "\t60000";
    };
    const auto first = cache.acquire(acquire, clock);
    time += std::chrono::milliseconds{58'999};
    expect(cache.acquire(acquire, clock) == first && calls == 1, "Reader renewed before deadline");
    time += std::chrono::milliseconds{1};
    expect(cache.acquire(acquire, clock) != first && calls == 2, "Reader did not renew at deadline");
    time += std::chrono::seconds{59};
    expect(!cache.acquire([]() -> std::expected<std::string, std::string> {
        return std::unexpected{"Match service returned HTTP 401"};
    }, clock), "Reader reused expired grant on renewal failure");
    for (const auto& bad : {std::string{"malformed"}, "gr1_" + std::string(64, 'a') + "\t60001",
                           "gr1_" + std::string(64, 'a') + "\t0",
                           "gr1_" + std::string(64, 'a') + "\t60000\n",
                           "gr1_" + std::string(64, 'x') + "\t60000"}) {
        expect(!cache.acquire([&]() -> std::expected<std::string, std::string> { return bad; }, clock),
               "Malformed grant response accepted");
    }
    expect(!cache.acquire([&]() -> std::expected<std::string, std::string> {
        time += std::chrono::seconds{61}; return "gr1_" + std::string(64, 'a') + "\t60000";
    }, clock), "Slow exchange returned an already expired grant");
}
}

int main(int argc, char** argv) try {
    expect(argc == 2, "Expected host executable path");
    using Budget = gloom::backends::MatchRequestBudget;
    const auto start = Budget::Clock::time_point{};
    Budget budget{2, 2, start};
    expect(budget.consume(start) && budget.consume(start) && !budget.consume(start), "Burst limit failed");
    expect(!budget.consume(start + std::chrono::milliseconds{499}), "Budget refilled too early");
    expect(budget.consume(start + std::chrono::milliseconds{500}), "Budget did not refill");
    expect(!budget.consume(start), "Backward clock refilled budget");
    const auto later = start + std::chrono::hours{1};
    expect(budget.consume(later) && budget.consume(later) && !budget.consume(later), "Idle burst exceeded cap");
    Budget concurrent{60, 20, start};
    std::atomic_uint accepted{0};
    {
        std::vector<std::jthread> workers;
        for (int i = 0; i < 8; ++i) workers.emplace_back([&] {
            for (int request = 0; request < 100; ++request)
                if (concurrent.consume(start)) ++accepted;
        });
    }
    expect(accepted == 60, "Concurrent requests exceeded shared burst");
    const auto root = std::filesystem::temp_directory_path() /
        ("gloom-https-test-" + std::to_string(GetCurrentProcessId()) + "-" + std::to_string(now_ms()));
    std::filesystem::create_directory(root);
    grant_policy_tests(root);
    certificate(root / "cert.pem", root / "key.pem");
    // Legacy/admin environment must not grant a reader or let the host start.
    SetEnvironmentVariableW(L"GLOOM_MATCH_IDENTITY_PROVIDER", L"registry");
    SetEnvironmentVariableW(L"GLOOM_MATCH_SERVICE_TOKEN", L"test-token-only-do-not-deploy");
    SetEnvironmentVariableW(L"GLOOM_MATCH_READER_TOKEN", nullptr);
    SetEnvironmentVariableW(L"GLOOM_MATCH_PUBLISHER_TOKEN", nullptr);
    SetEnvironmentVariableW(L"GLOOM_MATCH_IDENTITIES_FILE", nullptr);
    { Process invalid{argv[1], root, true}; }
    SetEnvironmentVariableW(L"GLOOM_MATCH_READER_TOKEN", L"test-reader-only-do-not-deploy");
    { Process invalid{argv[1], root, true}; }
    SetEnvironmentVariableW(L"GLOOM_MATCH_PUBLISHER_TOKEN", L"test-token-only-do-not-deploy");
    { Process invalid{argv[1], root, true}; }
    const auto identities = root / "identities.txt";
    SetEnvironmentVariableW(L"GLOOM_MATCH_IDENTITIES_FILE", identities.c_str());
    { Process invalid{argv[1], root, true}; }
    const auto write_identities = [&](bool malformed = false, bool expiring = false) {
        std::ofstream output{identities, std::ios::binary};
        output << "GLOOM_MATCH_IDENTITIES_V1\n";
        for (const auto* id : {"alice", "bob", "charlie"}) {
            output << id << '\t' << gloom::backends::match_credential_digest(std::string{"individual-credential-"} + id)
                   << '\t' << now_ms() + 3'600'000 << '\n';
        }
        if (expiring) output << "dave\t" << gloom::backends::match_credential_digest("individual-credential-dave")
                             << '\t' << now_ms() + 500 << '\n';
        if (malformed) output << "invalid trailing record\n";
    };
    write_identities(true);
    { Process invalid{argv[1], root, true}; }
    write_identities();
    SetEnvironmentVariableW(L"GLOOM_MATCH_PUBLISHER_TOKEN", L"too-short");
    { Process invalid{argv[1], root, true}; }
    SetEnvironmentVariableW(L"GLOOM_MATCH_PUBLISHER_TOKEN", L"test-token-only-do-not-deploy");
    const httplib::Headers headers{{"Authorization", "Bearer test-token-only-do-not-deploy"}};
    httplib::Headers reader;
    const auto identity = [](std::string_view id) {
        return httplib::Headers{{"Authorization", "Bearer individual-credential-" + std::string{id}}};
    };
    const auto exchange = [&](httplib::SSLClient& client, std::string_view id) {
        const auto issued = client.Post("/reader-grants/exchange", identity(id), "", "text/plain");
        expect(issued && issued->status == 200 && issued->body.ends_with("\t60000") &&
               issued->get_header_value("Cache-Control") == "no-store", "Identity exchange failed");
        return httplib::Headers{{"Authorization", "Bearer " + issued->body.substr(0, issued->body.find('\t'))}};
    };
    gloom::gameplay::SliceStoredMatch record{
        {.match_id = "test-duel", .instance_id = "server-a", .display_name = "TLS duel",
         .endpoint = "127.0.0.1:27020"}, now_ms() + 30'000, "server-a:1"};
    auto wire = gloom::gameplay::encode_stored_match(record);
    {
        Process host{argv[1], root};
        httplib::SSLClient untrusted{"127.0.0.1", host.port};
        expect(!untrusted.Get("/health", headers), "Untrusted certificate was accepted");
        httplib::SSLClient client{"127.0.0.1", host.port};
        client.set_ca_cert_path((root / "cert.pem").string());
        reader = exchange(client, "alice");
        auto response = client.Get("/health");
        expect(response && response->status == 401, "Unauthenticated health was accepted");
        response = client.Get("/matches", identity("alice"));
        expect(response && response->status == 401, "Identity credential bypassed grant exchange");
        response = client.Get("/health", {{"Authorization", "Bearer test-reader-only-do-not-deploy"}});
        expect(response && response->status == 401, "Legacy reader token remained usable");
        response = client.Post("/reader-grants/exchange", headers, "", "text/plain");
        expect(response && response->status == 401, "Publisher used as client identity");
        response = client.Post("/reader-grants/exchange", identity("alice"), "bob", "text/plain");
        expect(response && response->status == 400, "Client selected a principal in exchange body");
        response = client.Post("/reader-grants/exchange?principal=bob", identity("alice"), "", "text/plain");
        expect(response && response->status == 400, "Client selected a principal in exchange query");
        auto duplicate = identity("alice"); duplicate.emplace("Authorization", "Bearer individual-credential-bob");
        response = client.Post("/reader-grants/exchange", duplicate, "", "text/plain");
        expect(response && response->status == 401, "Duplicate exchange authorization accepted");
        write_identities(true);
        response = client.Post("/reader-grants/exchange", identity("alice"), "", "text/plain");
        expect(response && response->status == 401, "Partially corrupt registry accepted identity");
        write_identities();
        const auto charlie = exchange(client, "charlie");
        response = client.Delete("/reader-grants/charlie", reader);
        expect(response && response->status == 403, "Reader could revoke a principal");
        std::filesystem::create_directory(root / "matches.store.revocations.tmp");
        response = client.Delete("/reader-grants/charlie", headers);
        expect(response && response->status == 503, "Non-durable revocation acknowledged over HTTPS");
        response = client.Get("/ready", headers);
        expect(response && response->status == 503, "Revocation disk failure did not fail readiness");
        std::filesystem::remove(root / "matches.store.revocations.tmp");
        response = client.Delete("/reader-grants/charlie", headers);
        expect(response && response->status == 204, "Publisher revocation failed");
        response = client.Get("/ready", headers);
        expect(response && response->status == 200, "Revocation retry did not recover readiness");
        response = client.Get("/matches", charlie);
        expect(response && response->status == 401, "Revoked grant accepted");
        response = client.Post("/reader-grants/exchange", identity("charlie"), "", "text/plain");
        expect(response && response->status == 401, "Revoked principal renewed grant");
        write_identities(false, true);
        response = client.Post("/reader-grants/exchange", identity("dave"), "", "text/plain");
        expect(response && response->status == 200, "Short identity exchange failed");
        const httplib::Headers expiring_grant{{"Authorization", "Bearer " + response->body.substr(0, response->body.find('\t'))}};
        std::this_thread::sleep_for(std::chrono::milliseconds{550});
        response = client.Get("/health", expiring_grant);
        expect(response && response->status == 401, "Expired HTTPS reader grant accepted");
        response = client.Post("/reader-grants/exchange", identity("dave"), "", "text/plain");
        expect(response && response->status == 401, "Expired identity exchanged again");
        write_identities();
        response = client.Get("/ready", headers);
        expect(response && response->status == 200, "Host not ready");
        response = client.Put("/matches/test-duel?now_ms=0", headers, wire, "text/plain");
        expect(response && response->status == 204, "Valid publication failed");
        response = client.Get("/health", reader);
        expect(response && response->status == 200, "Reader connector probe denied");
        response = client.Get("/matches", reader);
        expect(response && response->status == 200 && response->body == wire + '\n', "Reader listing failed");
        response = client.Get("/matches/test-duel", reader);
        expect(response && response->status == 200 && response->body == wire, "Reader resolution failed");
        response = client.Put("/matches/test-duel", reader, wire, "text/plain");
        expect(response && response->status == 403, "Reader can publish");
        response = client.Delete("/matches/test-duel?instance_id=server-a", reader);
        expect(response && response->status == 403, "Reader can withdraw using visible owner id");
        response = client.Get("/ready", reader);
        expect(response && response->status == 403, "Reader can inspect operator readiness");
        auto ambiguous = reader;
        ambiguous.emplace("Authorization", "Bearer test-token-only-do-not-deploy");
        response = client.Get("/health", ambiguous);
        expect(response && response->status == 401, "Duplicate authorization accepted");
        response = client.Put("/matches/test-duel", headers, wire, "text/plain");
        expect(response && response->status == 204, "Idempotent replay failed");
        auto impostor = record;
        impostor.advertisement.instance_id = "server-b";
        impostor.mutation_id = "server-b:1";
        response = client.Put("/matches/test-duel?now_ms=999999999999999", headers,
                              gloom::gameplay::encode_stored_match(impostor), "text/plain");
        expect(response && response->status == 409, "Client clock bypassed ownership");
        response = client.Put("/matches/test-duel", headers, "broken", "text/plain");
        expect(response && response->status == 400, "Malformed record accepted");
        response = client.Put("/matches/test-duel", headers, std::string(4096, 'x'), "text/plain");
        expect(response && response->status == 413, "Oversize body accepted");
        response = client.Get("/matches", headers);
        expect(response && response->status == 200 && response->body == wire + '\n', "List round trip failed");
        response = client.Delete("/matches/test-duel?instance_id=server-b", headers);
        expect(response && response->status == 404, "Wrong owner deleted record");
        auto expiring = record;
        expiring.advertisement.match_id = "expiring";
        expiring.expires_at_ms = now_ms() + 200;
        response = client.Put("/matches/expiring", headers,
                              gloom::gameplay::encode_stored_match(expiring), "text/plain");
        expect(response && response->status == 204, "Short lease failed");
        std::this_thread::sleep_for(std::chrono::milliseconds{250});
        response = client.Get("/matches/expiring", headers);
        expect(response && response->status == 404, "Expired record remained visible");
        std::filesystem::create_directory(root / "matches.store.tmp");
        record.advertisement.revision = 2;
        record.advertisement.display_name = "Updated TLS duel";
        record.mutation_id = "server-a:2";
        const auto update_wire = gloom::gameplay::encode_stored_match(record);
        response = client.Put("/matches/test-duel", headers, update_wire, "text/plain");
        expect(response && response->status == 503, "Disk failure did not reach HTTP caller");
        response = client.Get("/ready", headers);
        expect(response && response->status == 503, "Disk failure did not affect readiness");
        response = client.Get("/matches/test-duel", headers);
        expect(response && response->body == wire, "Failed disk write changed live record");
        std::filesystem::remove(root / "matches.store.tmp");
        response = client.Put("/matches/test-duel", headers, update_wire, "text/plain");
        expect(response && response->status == 204, "Recovered disk write failed");
        wire = update_wire;
        response = client.Get("/ready", headers);
        expect(response && response->status == 200, "Readiness did not recover");
        bool throttled = false;
        auto drained_at=std::chrono::steady_clock::now();
        client.set_keep_alive(true);
        for (int i = 0; i < 150; ++i) {
            drained_at=std::chrono::steady_clock::now();
            response = client.Get("/matches", reader);
            expect(response && (response->status == 200 || response->status == 429), "Unexpected reader response");
            if (response->status == 429) { throttled = true; break; }
        }
        expect(throttled && response->get_header_value("Retry-After") == "1", "Reader burst not throttled");
        const auto renewed = exchange(client, "alice");
        // TLS exchange/OS scheduling can take >200 ms, legitimately refilling
        // tokens at 5/s. Bound successes by elapsed refill; do not assume zero
        // wall time. The injected-clock check above proves the exact boundary.
        for(int accepted=0;accepted<150;++accepted) {
            response=client.Get("/matches",renewed);
            expect(response && (response->status==200 || response->status==429),"Unexpected renewed reader response");
            if(response->status==429)break;
            const auto elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-drained_at).count();
            expect(static_cast<double>(accepted+1)<=std::ceil(elapsed*5.0),"Renewal reset principal quota");
        }
        expect(response && response->status==429,"Renewed reader burst was not throttled");
        const auto bob = exchange(client, "bob");
        response = client.Get("/matches", bob);
        expect(response && response->status == 200, "Alice exhausted Bob's quota");
        response = client.Get("/ready", headers);
        expect(response && response->status == 200, "Reader exhausted publisher budget");
        std::this_thread::sleep_for(std::chrono::milliseconds{250});
        response = client.Get("/matches/test-duel", reader);
        expect(response && response->status == 200 && response->body == wire, "Reader budget did not recover");
        throttled = false;
        for (int i = 0; i < 100; ++i) {
            response = client.Get("/health");
            expect(response && (response->status == 401 || response->status == 429), "Unexpected anonymous response");
            if (response->status == 429) { throttled = true; break; }
        }
        expect(throttled, "Unauthenticated burst not throttled");
        response = client.Get("/ready", headers);
        expect(response && response->status == 200, "Anonymous traffic exhausted publisher budget");
        throttled = false;
        for (int i = 0; i < 100; ++i) {
            response = client.Post("/reader-grants/exchange", identity("unknown"), "", "text/plain");
            expect(response && (response->status == 401 || response->status == 429), "Unexpected exchange failure");
            if (response->status == 429) { throttled = true; break; }
        }
        expect(throttled && response->get_header_value("Retry-After") == "1", "Exchange budget not enforced");
        response = client.Get("/matches", bob);
        expect(response && response->status == 200, "Invalid exchange exhausted existing reader grant");
        response = client.Get("/ready", headers);
        expect(response && response->status == 200, "Invalid exchange exhausted publisher budget");
        host.finish();
    }
    {
        Process host{argv[1], root};
        httplib::SSLClient client{"127.0.0.1", host.port};
        client.set_ca_cert_path((root / "cert.pem").string());
        auto response = client.Get("/health", reader);
        expect(response && response->status == 401, "Restart accepted an old grant");
        response = client.Post("/reader-grants/exchange", identity("charlie"), "", "text/plain");
        expect(response && response->status == 401, "Restart lost principal revocation");
        reader = exchange(client, "alice");
        response = client.Get("/matches/test-duel", reader);
        expect(response && response->status == 200 && response->body == wire, "Restart lost durable record");
        const auto decoded = gloom::gameplay::decode_stored_match(response->body);
        expect(decoded && *decoded == record, "Shared client codec failed on HTTPS response");
        response = client.Delete("/matches/test-duel?instance_id=server-a", headers);
        expect(response && response->status == 204, "Owner withdrawal failed");
        response = client.Get("/matches/test-duel", headers);
        expect(response && response->status == 404, "Deleted match still visible");
        client.set_keep_alive(true);
        bool throttled = false;
        for (int i = 0; i < 100; ++i) {
            response = client.Get("/health", headers);
            expect(response && (response->status == 200 || response->status == 429), "Unexpected publisher response");
            if (response->status == 429) { throttled = true; break; }
        }
        expect(throttled, "Publisher burst not throttled");
        response = client.Get("/matches", reader);
        expect(response && response->status == 200, "Publisher exhausted reader budget");
        host.finish();
    }
    std::filesystem::remove_all(root); // Only this test's uniquely created directory.
    std::cout << "HTTPS executable integration passed\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
