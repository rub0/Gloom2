#include <gloom/gameplay/vertical_slice.hpp>
#include <gloom/gameplay/first_person.hpp>
#include <gloom/gameplay/first_person_presentation.hpp>
#include <gloom/gameplay/components.hpp>
#include <gloom/gameplay/character_presentation.hpp>
#include <gloom/gameplay/component_replication.hpp>
#include <gloom/backends/jolt_world.hpp>

#include <cmath>
#include <array>
#include <iostream>
#include <stdexcept>

namespace {

void expect(const bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error{message};
    }
}

void expect_near(const float actual, const float expected, const char* message) {
    if (std::abs(actual - expected) > 0.001F) {
        throw std::runtime_error{message};
    }
}

} // namespace

int main() try {
    using namespace gloom::gameplay;
    expect_near(legacy_default_life, 120.0F, "Legacy life changed");
    expect_near(soul_reaper_damage, 80.0F, "Soul Reaper damage changed");
    expect_near(soul_reaper_range, 15.0F, "Soul Reaper range changed");
    expect_near(static_cast<float>(soul_reaper_cooldown_seconds), 0.5F,
                "Soul Reaper cooldown changed");
    expect_near(hound_bite_damage, 40.0F, "Legacy Hound bite damage changed");
    expect_near(hound_bite_life_steal, 40.0F, "Legacy Hound life steal changed");

    constexpr auto hound_recipe = character_presentation_recipe(SliceCharacter::hound);
    constexpr auto berserker_recipe =
        character_presentation_recipe(SliceCharacter::berserker);
    expect(hound_recipe.character == SliceCharacter::hound &&
               berserker_recipe.character == SliceCharacter::berserker &&
               hound_recipe.fallback_parts.size() == 9 &&
               berserker_recipe.fallback_parts.size() == 9 &&
               hound_recipe.authored_scene_uri != berserker_recipe.authored_scene_uri &&
               hound_recipe.cooked_scene_uri != berserker_recipe.cooked_scene_uri,
           "Roster presentation recipes are incomplete or share an authored asset slot");
    expect(hound_recipe.fallback_parts[1].forward_offset > 0.5F &&
               berserker_recipe.fallback_parts[1].forward_offset == 0.0F &&
               berserker_recipe.fallback_parts[2].right_offset < -0.4F &&
               berserker_recipe.fallback_parts[3].right_offset > 0.4F,
           "Berserker fallback still uses the Hound quadruped silhouette");
    expect(soul_reaper_presentation.required_instances == 2 &&
               hound_ability_presentation.required_instances == 2 &&
               soul_reaper_presentation.source_uri !=
                   hound_ability_presentation.source_uri &&
               soul_reaper_presentation.cooked_uri !=
                   hound_ability_presentation.cooked_uri,
           "Cooked first-person presentation slots overlap or are incomplete");

    gloom::core::EntityRegistry composition_registry;
    const auto composed_hound = compose_slice_character(
        composition_registry, {.network_entity = 77,
                               .connection = 88,
                               .spawn_x = -3.0F,
                               .spawn_z = 2.0F});
    expect(has_complete_character_composition(composition_registry, composed_hound),
           "Hound factory omitted a required logical component");
    expect(composition_registry.get<AuthorityComponent>(composed_hound)->network_entity == 77 &&
               composition_registry.get<CharacterMovementComponent>(composed_hound)->spawn_x ==
                   -3.0F &&
               composition_registry.get<CharacterPhysicsComponent>(composed_hound)
                   ->provisional_kinematic_proxy == false,
           "Hound composition did not preserve authority, spawn or migrated physics policy");
    const auto composed_berserker = compose_slice_character(
        composition_registry,
        {.network_entity = 78,
         .selection = {.character = SliceCharacter::berserker,
                       .ability = SliceAbility::none}});
    expect(has_complete_character_composition(composition_registry, composed_berserker) &&
               composition_registry.get<CharacterLoadoutComponent>(composed_berserker)
                       ->selection.character == SliceCharacter::berserker,
           "Berserker did not use the complete shared character composition");
    auto* composed_transform = composition_registry.get<TransformComponent>(composed_hound);
    auto* composed_replication = composition_registry.get<ReplicationComponent>(composed_hound);
    composed_transform->position_x = 4.0F;
    composed_transform->velocity_z = -2.0F;
    composed_replication->grounded = false;
    const std::array composed_entities{composed_hound};
    const auto component_snapshot = capture_component_snapshot(
        composition_registry, composed_entities, 42, 9);
    expect(component_snapshot.entities.size() == 1 &&
               component_snapshot.entities.front().entity == 77 &&
               component_snapshot.entities.front().position_x == 4.0F &&
               !component_snapshot.entities.front().grounded,
           "Component-derived snapshot lost authority, transform or grounded state");
    composition_registry.get<AuthorityComponent>(composed_hound)->mode =
        ComponentAuthority::interpolated_remote;
    auto received_snapshot = component_snapshot;
    received_snapshot.simulation_tick = 43;
    received_snapshot.entities.front().position_x = 8.0F;
    const auto applied = apply_component_snapshot(composition_registry, composed_entities,
                                                   received_snapshot);
    expect(applied.applied == 1 && composed_transform->position_x == 8.0F &&
               composed_replication->last_received_tick == 43,
           "Remote component authority did not apply a newer snapshot");
    composition_registry.get<AuthorityComponent>(composed_hound)->mode =
        ComponentAuthority::server;
    received_snapshot.simulation_tick = 44;
    received_snapshot.entities.front().position_x = 12.0F;
    const auto rejected = apply_component_snapshot(composition_registry, composed_entities,
                                                    received_snapshot);
    expect(rejected.ignored_authoritative == 1 && composed_transform->position_x == 8.0F,
           "A received snapshot overwrote server-authoritative component state");

    SlicePresentationFeedback feedback;
    SliceSnapshot feedback_snapshot{.simulation_tick = 1};
    feedback_snapshot.opponent.entity = VerticalSliceSimulation::opponent_entity;
    feedback.observe(feedback_snapshot);
    feedback_snapshot.simulation_tick = 2;
    feedback_snapshot.opponent.life = 40.0F;
    feedback.observe(feedback_snapshot);
    expect(feedback.state().opponent_damage_remaining_seconds > 0.0F,
           "Authoritative damage did not create a presentation cue");
    feedback.advance(0.21);
    expect_near(feedback.state().opponent_damage_remaining_seconds, 0.0F,
                "Damage presentation cue did not expire");
    feedback_snapshot.simulation_tick = 3;
    feedback_snapshot.opponent.life = 0.0F;
    feedback_snapshot.opponent.alive = false;
    feedback.observe(feedback_snapshot);
    expect(feedback.state().opponent_death_remaining_seconds > 0.0F,
           "Authoritative death did not create a presentation cue");
    feedback_snapshot.simulation_tick = 4;
    feedback_snapshot.opponent.life = legacy_default_life;
    feedback_snapshot.opponent.alive = true;
    feedback.observe(feedback_snapshot);
    expect(feedback.state().opponent_respawn_remaining_seconds > 0.0F,
           "Authoritative respawn did not create a presentation cue");

    VerticalSliceSimulation simulation{false};
    expect(simulation.snapshot().network.active_sessions == 2,
           "Player and opponent were not admitted to authoritative sessions");
    expect(simulation.arena().size() >= 9, "The Factory blockout is incomplete");
    bool factory_has_lava_box = false;
    std::size_t factory_solid_count = 0;
    for (const auto& box : simulation.arena()) {
        factory_has_lava_box = factory_has_lava_box || box.material == ArenaMaterial::lava;
        if (!box.solid) {
            continue;
        }
        ++factory_solid_count;
        bool has_matching_obstacle = false;
        for (const auto& obstacle : simulation.movement_settings().static_obstacles) {
            has_matching_obstacle = has_matching_obstacle ||
                                    (std::abs(obstacle.minimum_x -
                                              (box.center_x - box.half_x)) < 0.001F &&
                                     std::abs(obstacle.maximum_x -
                                              (box.center_x + box.half_x)) < 0.001F &&
                                     std::abs(obstacle.minimum_z -
                                              (box.center_z - box.half_z)) < 0.001F &&
                                     std::abs(obstacle.maximum_z -
                                              (box.center_z + box.half_z)) < 0.001F &&
                                     std::abs(obstacle.top_y -
                                              (box.center_y + box.half_y)) < 0.001F);
        }
        expect(has_matching_obstacle,
               "A rendered solid Factory box has no authoritative obstacle");
    }
    expect(!factory_has_lava_box, "Factory lava regressed to box geometry");
    expect(simulation.surfaces().size() == 1 &&
               simulation.surfaces().front().material == ArenaMaterial::lava,
           "Factory lost its one-sided lava surface");
    expect(factory_solid_count == simulation.movement_settings().static_obstacles.size() &&
               factory_solid_count == 7,
           "Factory solid metadata and authoritative collision diverged");
    expect(simulation.snapshot().player.character == SliceCharacter::hound,
           "The slice did not spawn the Hound archetype");
    expect(simulation.snapshot().factory_lift.entity == 100,
           "Factory did not compose its replicated kinematic lift");
    const float lift_start_y = simulation.snapshot().factory_lift.position_y;
    for (std::uint32_t tick = 0; tick < 70; ++tick) simulation.tick({});
    expect(simulation.snapshot().factory_lift.position_y > lift_start_y + 0.05F &&
               simulation.snapshot().factory_lift.velocity_y > 0.0F,
           "Factory lift did not follow its deterministic tick motion");
    expect_near(simulation.snapshot().player.life, 120.0F, "Player did not spawn with legacy life");
    expect_near(simulation.snapshot().hud.life_fraction, 120.0F / 250.0F,
                "HUD did not preserve the legacy default/maximum-life relationship");

    const auto occluded_life = simulation.snapshot().opponent.life;
    simulation.tick({.aim_x = 1.0F, .fire_primary = true});
    expect_near(simulation.snapshot().opponent.life, occluded_life,
                "Central cover did not occlude the Soul Reaper ray");

    // Move around the central occluder, then hold fire. Cooldown should admit
    // exactly enough 80-damage hits to kill and the opponent must respawn after 4 s.
    for (std::uint32_t tick = 0; tick < 45; ++tick) {
        simulation.tick({.axis_z = -1.0F});
    }
    simulation.add_shield(VerticalSliceSimulation::opponent_entity, 100.0F);
    const auto aimed_input = [&simulation](const float target_height = 0.0F) {
        const auto& state = simulation.snapshot();
        const float x = state.opponent.position_x - state.player.position_x;
        const float y = state.opponent.position_y + target_height -
                        (state.player.position_y + 0.9F);
        const float z = state.opponent.position_z - state.player.position_z;
        const float inverse_length = 1.0F / std::sqrt(x * x + y * y + z * z);
        return SliceInput{.aim_x = x * inverse_length,
                          .aim_y = y * inverse_length,
                          .aim_z = z * inverse_length,
                          .fire_primary = true};
    };
    const auto before_shot = simulation.snapshot().opponent.life;
    simulation.tick(aimed_input(1.7F));
    expect(simulation.snapshot().opponent.life < before_shot,
           "Soul Reaper shot at the visible upper Hound body did not hit");
    expect(simulation.snapshot().hud.hit_marker,
           "Confirmed Soul Reaper hit did not activate the HUD hit marker");
    expect_near(simulation.snapshot().opponent.life, 96.0F,
                "Shield did not absorb 70 percent of Soul Reaper damage");
    expect_near(simulation.snapshot().opponent.shield, 44.0F,
                "Shield pool was not depleted by absorbed damage");
    const auto after_shot = simulation.snapshot().opponent.life;
    simulation.tick(aimed_input());
    expect_near(simulation.snapshot().opponent.life, after_shot, "Weapon cooldown was bypassed");
    expect(simulation.snapshot().hud.weapon_ready_fraction < 1.0F,
           "HUD did not expose weapon cooldown");

    for (std::uint32_t tick = 0; tick < 180 && simulation.snapshot().opponent.alive; ++tick) {
        simulation.tick(aimed_input());
    }
    expect(!simulation.snapshot().opponent.alive, "Opponent did not enter death state");
    expect(simulation.snapshot().player.kills == 1, "Kill was not credited");
    for (std::uint32_t tick = 0; tick < VerticalSliceSimulation::tick_rate * 4; ++tick) {
        simulation.tick({});
    }
    expect(simulation.snapshot().opponent.alive, "Opponent did not respawn after four seconds");
    expect_near(simulation.snapshot().opponent.life, legacy_default_life,
                "Respawn did not restore default life");
    const auto post_respawn_life = simulation.snapshot().opponent.life;
    for (std::uint32_t tick = 0; tick < 31; ++tick) simulation.tick({});
    simulation.tick(aimed_input(1.7F));
    expect(simulation.snapshot().opponent.life < post_respawn_life,
           "Respawned Hound could not be hit after rebuilding its physics/combat hitbox");
    expect(simulation.snapshot().network.authorized_input_batches > 0 &&
               simulation.snapshot().network.authorized_fire_commands >= 3 &&
               simulation.snapshot().network.rejected_commands == 0,
           "Vertical slice bypassed session-authorized gameplay routing");

    gloom::backends::JoltWorld factory_physics;
    factory_physics.start();
    VerticalSliceSimulation lava_simulation{VerticalSliceSettings{
        .opponent_ai_enabled = false,
        .authoritative_physics = &factory_physics}};
    for (std::uint32_t tick = 0; tick < 36; ++tick) {
        lava_simulation.tick({.axis_x = -1.0F}, {});
    }
    for (std::uint32_t tick = 0; tick < 100; ++tick) {
        lava_simulation.tick({.axis_z = 1.0F}, {});
    }
    expect(lava_simulation.snapshot().player.life < legacy_default_life,
           "Jolt lava trigger did not apply authoritative volume damage");

    VerticalSliceSimulation lethal_shots{false};
    for (std::uint32_t tick = 0; tick < 45; ++tick) {
        lethal_shots.tick({.axis_z = -1.0F});
    }
    const auto aim_at_upper_body = [&lethal_shots]() {
        const auto& state = lethal_shots.snapshot();
        const float x = state.opponent.position_x - state.player.position_x;
        const float y = state.opponent.position_y + 1.7F -
                        (state.player.position_y + 0.9F);
        const float z = state.opponent.position_z - state.player.position_z;
        const float inverse_length = 1.0F / std::sqrt(x * x + y * y + z * z);
        return SliceInput{.aim_x = x * inverse_length,
                          .aim_y = y * inverse_length,
                          .aim_z = z * inverse_length,
                          .fire_primary = true};
    };
    lethal_shots.tick(aim_at_upper_body());
    for (std::uint32_t tick = 0; tick < VerticalSliceSimulation::tick_rate / 2; ++tick) {
        lethal_shots.tick({});
    }
    lethal_shots.tick(aim_at_upper_body());
    expect(!lethal_shots.snapshot().opponent.alive &&
               lethal_shots.snapshot().player.kills == 1,
           "Two clear Soul Reaper hits did not kill and credit the attacker");

    VerticalSliceSimulation bite_simulation{false};
    for (std::uint32_t tick = 0; tick < 24; ++tick) {
        bite_simulation.tick({.axis_z = -1.0F}, {.axis_z = -1.0F});
    }
    for (std::uint32_t tick = 0; tick < 48; ++tick) {
        bite_simulation.tick({.axis_x = 1.0F, .aim_x = 1.0F},
                             {.axis_x = -1.0F, .aim_x = -1.0F});
    }
    bite_simulation.tick({.aim_x = 1.0F, .use_primary_ability = true}, {});
    expect(bite_simulation.snapshot().hud.primary_ability_active &&
               bite_simulation.snapshot().hud.primary_ability_ready_fraction < 1.0F,
           "Hound bite did not enter its replicated active/cooldown state");
    for (std::uint32_t tick = 0; tick < 30 &&
                                 bite_simulation.snapshot().opponent.life == legacy_default_life;
         ++tick) {
        bite_simulation.tick({}, {});
    }
    expect_near(bite_simulation.snapshot().opponent.life,
                legacy_default_life - hound_bite_damage,
                "Hound bite did not apply its legacy damage authoritatively");
    expect_near(bite_simulation.snapshot().player.life, 160.0F,
                "Hound bite did not apply its legacy life steal");
    expect(bite_simulation.snapshot().network.authorized_ability_commands == 1,
           "Hound bite bypassed authoritative ability routing");

    VerticalSliceSimulation box_collision{false};
    for (std::uint32_t tick = 0; tick < 120; ++tick) {
        box_collision.tick({.axis_z = 1.0F}, {});
    }
    expect(box_collision.snapshot_for(VerticalSliceSimulation::player_entity)
                   .player.position_z < 2.3F,
           "Player crossed a rendered Factory service block");

    VerticalSliceSimulation combatant_collision{false};
    for (std::uint32_t tick = 0; tick < 24; ++tick) {
        combatant_collision.tick({.axis_z = -1.0F}, {.axis_z = -1.0F});
    }
    for (std::uint32_t tick = 0; tick < 120; ++tick) {
        combatant_collision.tick({.axis_x = 1.0F}, {.axis_x = -1.0F});
    }
    const auto collision_state =
        combatant_collision.snapshot_for(VerticalSliceSimulation::player_entity);
    const float collision_x = collision_state.opponent.position_x -
                              collision_state.player.position_x;
    const float collision_z = collision_state.opponent.position_z -
                              collision_state.player.position_z;
    expect(collision_state.player.position_x < collision_state.opponent.position_x &&
               std::sqrt(collision_x * collision_x + collision_z * collision_z) >= 0.799F,
           "Authoritative combatants crossed or remained interpenetrated");
    const auto& predicted_collision_state = combatant_collision.snapshot();
    expect(std::abs(predicted_collision_state.player.position_x -
                    collision_state.player.position_x) < 0.02F &&
               std::abs(predicted_collision_state.player.position_z -
                        collision_state.player.position_z) < 0.02F,
           "Local camera prediction diverged while touching a living combatant");

    VerticalSliceSimulation presentation_alignment{false};
    for (std::uint32_t tick = 0; tick < 18; ++tick) {
        presentation_alignment.tick({}, {.axis_z = -1.0F});
    }
    const auto authoritative_alignment = presentation_alignment.snapshot_for(
        VerticalSliceSimulation::player_entity);
    expect_near(presentation_alignment.snapshot().opponent.position_x,
                authoritative_alignment.opponent.position_x,
                "Presented opponent X diverged from its authoritative hit volume");
    expect_near(presentation_alignment.snapshot().opponent.position_z,
                authoritative_alignment.opponent.position_z,
                "Presented opponent Z diverged from its authoritative hit volume");

    VerticalSliceSimulation selected_loadout{false};
    const SlicePlayerSelection reaper_only{.ability = SliceAbility::none};
    expect(selected_loadout.set_selection(VerticalSliceSimulation::player_entity,
                                          reaper_only),
           "Authoritative simulation rejected the supported alternate loadout");
    selected_loadout.tick({.aim_x = 1.0F, .use_primary_ability = true});
    const auto& selected_state = selected_loadout.snapshot();
    expect(selected_state.player.weapon == SliceWeapon::soul_reaper &&
               selected_state.player.ability == SliceAbility::none &&
               !selected_state.player.primary_ability_active &&
               selected_state.hud.primary_ability_ready_fraction == 0.0F &&
               selected_state.network.authorized_ability_commands == 0,
           "Selection-driven composition still activated Bite for the no-ability loadout");

    VerticalSliceSimulation guard_loadout{false};
    expect(guard_loadout.set_selection(
               VerticalSliceSimulation::player_entity,
               SlicePlayerSelection{.ability = SliceAbility::guard}),
           "Authoritative simulation rejected the Guard loadout");
    guard_loadout.tick({.aim_x = 1.0F, .use_primary_ability = true});
    expect_near(guard_loadout.snapshot().player.shield, hound_guard_shield,
                "Guard did not grant its authoritative shield");
    expect(guard_loadout.snapshot().player.ability == SliceAbility::guard &&
               guard_loadout.snapshot().hud.primary_ability_active &&
               guard_loadout.snapshot().hud.primary_ability_ready_fraction < 1.0F &&
               guard_loadout.snapshot().network.authorized_ability_commands == 1,
           "Guard state or cooldown was not derived from the selected ability");

    VerticalSliceSimulation cooldown_reuse{false};
    cooldown_reuse.tick({.aim_x = 1.0F, .use_primary_ability = true});
    for (std::uint32_t tick = 0; tick < 1'500; ++tick) {
        cooldown_reuse.tick({});
    }
    cooldown_reuse.tick({.aim_x = 1.0F, .use_primary_ability = true});
    expect(cooldown_reuse.snapshot().network.authorized_ability_commands == 2 &&
               cooldown_reuse.snapshot().hud.primary_ability_active,
           "Bite did not become reusable after its authoritative cooldown");

    FirstPersonController controller;
    const auto initial_forward = controller.forward();
    expect_near(initial_forward.x, 1.0F, "FPS controller did not initially face the opponent");
    const auto forward_movement = controller.movement(0.0F, -1.0F);
    expect_near(forward_movement.world_x, 1.0F,
                "FPS-relative forward movement was not transformed into world space");
    controller.look(100.0F, -50.0F, 0.01F);
    const auto turned = controller.forward();
    expect(turned.z < -0.5F && turned.y > 0.0F,
           "Mouse deltas did not update yaw and pitch");

    std::cout << "Gloom vertical-slice tests completed successfully.\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << "Vertical-slice test failure: " << error.what() << '\n';
    return 1;
}
