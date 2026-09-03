#include <gloom/gameplay/match_lobby.hpp>

#include <iostream>
#include <stdexcept>

namespace {

void expect(const bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error{message};
    }
}

void test_identity_and_wire_contract() {
    const gloom::gameplay::SlicePlayerIdentity identity{.account_id = 42,
                                                        .display_name = "Nyx-42"};
    const auto credential = gloom::gameplay::encode_slice_credential("secret", identity);
    const auto decoded = gloom::gameplay::decode_slice_credential(credential, "secret");
    expect(decoded && decoded->account_id == identity.account_id &&
               decoded->display_name == identity.display_name,
           "Development identity credential did not round trip");
    expect(!gloom::gameplay::decode_slice_credential(credential, "wrong"),
           "Credential accepted the wrong development secret");
    const auto provider = gloom::gameplay::make_development_identity_provider("secret");
    const auto verified = provider->verify(credential);
    expect(verified && *verified == identity && !provider->verify("opaque"),
           "Development identity provider did not enforce its credential boundary");
    expect(!gloom::gameplay::valid_development_identity({.account_id = 3,
                                                         .display_name = "bad name"}),
           "Identity accepted a display name outside the wire policy");

    gloom::gameplay::SliceLobbyState state{
        .revision = 9,
        .phase = gloom::gameplay::SliceMatchPhase::active,
        .players = {{.entity = 1, .identity = identity, .connected = true, .ready = true},
                    {.entity = 2,
                     .identity = {.account_id = 84, .display_name = "Reaper"},
                     .connected = true,
                     .ready = true}},
    };
    const auto envelope = gloom::network::decode_message(
        gloom::network::encode_message(gloom::gameplay::encode_lobby_state(state)));
    expect(envelope.has_value(), "Protocol v8 rejected a lobby state envelope");
    const auto decoded_state = gloom::gameplay::decode_lobby_state(*envelope);
    expect(decoded_state && decoded_state->revision == 9 &&
               decoded_state->phase == gloom::gameplay::SliceMatchPhase::active &&
               decoded_state->players.size() == 2 &&
               decoded_state->players[1].identity.display_name == "Reaper" &&
               decoded_state->players[1].selection ==
                   gloom::gameplay::SlicePlayerSelection{},
           "Lobby state wire round trip changed player state");
    const auto ready = gloom::gameplay::decode_lobby_ready(
        gloom::gameplay::encode_lobby_ready(true));
    expect(ready && *ready, "Lobby ready command did not round trip");
    const auto selection = gloom::gameplay::decode_lobby_selection(
        gloom::gameplay::encode_lobby_selection({}));
    expect(selection && *selection == gloom::gameplay::SlicePlayerSelection{},
           "Lobby selection command did not round trip");
    const auto guard_selection = gloom::gameplay::decode_lobby_selection(
        gloom::gameplay::encode_lobby_selection(
            {.ability = gloom::gameplay::SliceAbility::guard}));
    expect(guard_selection && guard_selection->ability == gloom::gameplay::SliceAbility::guard,
           "Lobby Guard selection command did not round trip");
    const gloom::gameplay::SlicePlayerSelection berserker_selection{
        .character = gloom::gameplay::SliceCharacter::berserker,
        .ability = gloom::gameplay::SliceAbility::none};
    const auto decoded_berserker = gloom::gameplay::decode_lobby_selection(
        gloom::gameplay::encode_lobby_selection(berserker_selection));
    expect(decoded_berserker && *decoded_berserker == berserker_selection,
           "Lobby Berserker selection command did not round trip");
    expect(!gloom::gameplay::valid_slice_selection(
               {.character = gloom::gameplay::SliceCharacter::berserker,
                .ability = gloom::gameplay::SliceAbility::bite}) &&
               !gloom::gameplay::valid_slice_selection(
                   {.character = gloom::gameplay::SliceCharacter::berserker,
                    .ability = gloom::gameplay::SliceAbility::guard}),
           "Berserker accepted an unaudited Hound ability");
    auto invalid_selection = gloom::gameplay::encode_lobby_ready(true);
    invalid_selection.payload = {std::byte{1}, std::byte{7}, std::byte{0}, std::byte{0}};
    expect(!gloom::gameplay::decode_lobby_selection(invalid_selection),
           "Lobby accepted an unsupported character selection");
}

void test_ready_start_reconnect_and_abandonment() {
    gloom::gameplay::SliceMatchLobby lobby({
        .required_players = 2,
        .reconnect_grace_ticks = 3,
        .validate_identity = gloom::gameplay::valid_development_identity,
    });
    const gloom::gameplay::SlicePlayerIdentity first{.account_id = 1,
                                                     .display_name = "First"};
    const gloom::gameplay::SlicePlayerIdentity second{.account_id = 2,
                                                      .display_name = "Second"};
    expect(lobby.admit(1, first, 0).has_value() &&
               lobby.admit(2, second, 0).has_value(),
           "Lobby did not admit two valid identities");
    expect(lobby.state().phase == gloom::gameplay::SliceMatchPhase::waiting &&
               !lobby.accepts_gameplay(1),
           "Lobby allowed gameplay before both players were ready");
    expect(lobby.set_selection(1, {}) && lobby.state().players[0].selection ==
                                              gloom::gameplay::SlicePlayerSelection{},
           "Lobby did not accept the supported pre-match selection");
    expect(lobby.set_ready(1, true) && !lobby.accepts_gameplay(1),
           "First ready player started the match alone");
    const gloom::gameplay::SlicePlayerSelection reaper_only{
        .ability = gloom::gameplay::SliceAbility::none};
    expect(lobby.set_selection(1, reaper_only) &&
               lobby.state().players[0].selection == reaper_only &&
               !lobby.state().players[0].ready && lobby.set_ready(1, true),
           "Changing loadout did not clear and restore the ready gate");
    expect(lobby.set_ready(2, true) &&
               lobby.state().phase == gloom::gameplay::SliceMatchPhase::active &&
               lobby.accepts_gameplay(1) && lobby.accepts_gameplay(2),
           "Two ready players did not start the match");

    lobby.disconnected(2, 10);
    expect(!lobby.accepts_gameplay(2) && !lobby.tick(13),
           "Lobby ended the match before reconnect grace elapsed");
    expect(lobby.admit(2, second, 13).has_value() && lobby.accepts_gameplay(2),
           "Lobby did not restore a matching identity within reconnect grace");
    lobby.disconnected(2, 20);
    expect(lobby.tick(24) &&
               lobby.state().phase == gloom::gameplay::SliceMatchPhase::completed &&
               lobby.state().end_reason ==
                   gloom::gameplay::SliceMatchEndReason::abandonment &&
               lobby.state().winner_entity == 1 && !lobby.accepts_gameplay(1),
           "Lobby did not award an authoritative abandonment win");
}

void test_duplicate_and_resume_identity_rejection() {
    gloom::gameplay::SliceMatchLobby lobby({
        .required_players = 2,
        .reconnect_grace_ticks = 10,
        .validate_identity = gloom::gameplay::valid_development_identity,
    });
    const gloom::gameplay::SlicePlayerIdentity identity{.account_id = 7,
                                                        .display_name = "Seven"};
    expect(lobby.admit(1, identity, 0).has_value(), "Initial identity was rejected");
    expect(!lobby.admit(2, identity, 0), "Duplicate account occupied two lobby slots");
    lobby.disconnected(1, 1);
    expect(!lobby.admit(1, {.account_id = 7, .display_name = "Impostor"}, 2),
           "Resume accepted an identity that changed its display name");
}

} // namespace

int main() try {
    test_identity_and_wire_contract();
    test_ready_start_reconnect_and_abandonment();
    test_duplicate_and_resume_identity_rejection();
    std::cout << "Gloom match lobby tests completed successfully.\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << "Match lobby test failure: " << error.what() << '\n';
    return 1;
}
