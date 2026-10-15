#include "../src/game.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

using namespace mountain;
int checks = 0;
void check(bool condition, const std::string& message) {
    ++checks;
    if (!condition) { std::cerr << "FAILED: " << message << '\n'; std::exit(1); }
}
void advance(Game& game, int frames, Input input = {}) {
    for (int i = 0; i < frames; ++i) game.step(1.0f / 120, input);
}

void rockTests() {
    const Rect box{100, 100, playerWidth, playerHeight};
    check(sweptCircleTouches({119, 0}, {119, 250}, 17, box), "sweep catches a rock crossing the entire player");
    check(sweptCircleTouches({0, 125}, {250, 125}, 17, box), "sweep catches relative horizontal movement");
    check(!sweptCircleTouches({80, 0}, {80, 250}, 17, box), "sweep allows a near miss");
    check(!sweptCircleTouches({90, 90}, {90, 90}, 10, box), "rounded corners do not cause false hits");
    check(sweptCircleTouches({90, 100}, {90, 100}, 10, box), "stationary tangent counts as contact");
    check(sweptCircleTouches({70, 80}, {120, 130}, 10, box), "diagonal corner sweep detects contact");

    Game game;
    game.reset(); game.rockClock = 0;
    game.step(1.0f / 120, {});
    check(game.rocks.size() == 1, "expired spawn clock creates one rock");
    Rock spawned = game.rocks.front();
    check(std::abs(spawned.pos.y - (game.player.y - 430)) < 0.01f, "rock starts above the player");
    check(spawned.warning > 1 && spawned.speed == 0, "fresh rock has a full warning and no speed");
    check(spawned.radius >= 17 && spawned.radius <= 25, "spawn radius stays in range");
    advance(game, 30, {1, false});
    check(game.rocks.front().pos.x == spawned.pos.x, "warned target stays locked while the player dodges");
    check(game.rocks.front().pos.y == spawned.pos.y && game.lives == 3, "warning freezes fall and cannot hurt");
    float clock = game.rockClock, warning = game.rocks.front().warning;
    game.mode = Mode::Paused;
    advance(game, 120);
    check(game.rockClock == clock && game.rocks.front().warning == warning, "pause freezes spawn clock and countdown");

    Game early(7), late(7);
    early.reset(); late.reset();
    late.checkpointIndex = static_cast<int>(late.platforms.size()) - 1;
    early.rockClock = late.rockClock = 0;
    early.step(1.0f / 120, {1, false}); late.step(1.0f / 120, {1, false});
    check(early.rockDifficulty() == 0 && late.rockDifficulty() == 1, "difficulty follows checkpoint progress");
    check(std::abs(early.rocks.front().warningDuration - 1.1f) < 0.001f &&
          std::abs(late.rocks.front().warningDuration - 0.8f) < 0.001f, "warning duration ramps within fair limits");
    check(early.rockClock >= 2.2f && early.rockClock <= 3 &&
          late.rockClock >= 1.55f && late.rockClock <= 2.35f, "drop intervals stay in difficulty bounds");
    check(std::abs(early.rockClock - late.rockClock - 0.65f) < 0.001f, "summit drops are more frequent");
    check(early.rocks.front().pos.x == late.rocks.front().pos.x, "same seed and input reproduce targeting");

    game.reset(); game.rockClock = 0;
    for (std::size_t i = 0; i < maxRocks; ++i) game.rocks.push_back({{1000, -1000}, 0, 10, 20});
    game.step(1.0f / 120, {});
    check(game.rocks.size() == maxRocks && game.rockClock > 0, "active budget prevents an extra spawn and retries later");
    game.rocks.clear(); game.rockClock = 0;
    game.step(1.0f / 120, {});
    check(game.rocks.size() == 1, "spawning resumes when a slot opens");

    Game whole, split;
    whole.reset(); split.reset(); whole.rockClock = split.rockClock = 9999;
    whole.rocks.push_back({{300, 0}, 0, 0.01f, 20, 0.01f});
    split.rocks = whole.rocks;
    whole.step(1.0f / 30, {}); advance(split, 4);
    const float activeTime = 1.0f / 30 - 0.01f;
    check(whole.rocks.front().warning == 0, "countdown clamps to zero on release");
    check(std::abs(whole.rocks.front().speed - rockGravity * activeTime) < 0.001f,
          "only the remainder of the release step accelerates");
    check(std::abs(whole.rocks.front().pos.y - 0.5f * rockGravity * activeTime * activeTime) < 0.001f,
          "release step integrates only active fall time");
    check(std::abs(whole.rocks.front().pos.y - split.rocks.front().pos.y) < 0.001f &&
          std::abs(whole.rocks.front().speed - split.rocks.front().speed) < 0.001f,
          "rock fall and warning release match across step sizes");

    whole.rocks = {{{300, -10000}, rockMaxSpeed - 1, 0, 20}};
    split.rocks = whole.rocks;
    whole.step(1.0f / 30, {}); advance(split, 4);
    check(whole.rocks.front().speed == rockMaxSpeed, "fall speed clamps at terminal velocity");
    check(std::abs(whole.rocks.front().pos.y - split.rocks.front().pos.y) < 0.002f,
          "terminal speed transition matches across step sizes");
    advance(whole, 360);
    check(whole.rocks.front().speed == rockMaxSpeed, "long falls never accelerate beyond the speed cap");

    game.reset(); game.rockClock = 9999;
    game.rocks.push_back({{600, 450}, rockMaxSpeed, 0, 20});
    check(game.rockLandingY(game.rocks.front()) == 500, "drop prediction selects the first platform below");
    advance(game, 12);
    check(game.rocks.empty() && !game.particles.empty(), "rock shatters into particles on platform impact");
    check(game.lives == 3, "distant impact does not damage player");

    game.reset(); game.rockClock = 9999;
    game.player = {600, 550}; game.grounded = false;
    game.rocks.push_back({{619, 480}, rockMaxSpeed, 0, 20});
    game.step(1.0f / 30, {});
    check(game.rocks.empty() && game.lives == 3, "platform stops rock before it reaches a player below");

    game.reset(); game.rockClock = 9999;
    Rock edge{{430, 450}, rockMaxSpeed, 0, 20};
    check(std::abs(game.rockLandingY(edge) - (590 - std::sqrt(300.0f))) < 0.001f,
          "rock radius catches a platform edge");
    Rock gap{{450, 600}, rockMaxSpeed, 0, 20};
    check(!std::isfinite(game.rockLandingY(gap)), "gap trajectory has no false platform impact");
    game.rocks.push_back(gap); advance(game, 12);
    check(game.rocks.size() == 1 && game.rocks.front().pos.y > 660, "rocks fall freely through gaps");
    game.rocks.front().pos.y = game.player.y + 851;
    game.step(1.0f / 120, {});
    check(game.rocks.empty(), "out-of-view rocks are cleaned up");

    game.reset(); game.invincible = 1;
    game.player.y = game.platforms.front().box.y + 600;
    game.rocks.push_back({{game.player.x + 19, game.player.y + 25}, 0, 0, 20});
    game.step(1.0f / 120, {});
    check(game.lives == 2 && game.player.y == game.checkpoint.y,
          "simultaneous rock overlap cannot suppress fall damage during invincibility");
    check(game.rocks.empty() && game.rockClock == 2 && game.invincible == 2,
          "respawn clears hazards and grants a safe restart window");

    game.reset();
    check(game.rocks.empty() && game.particles.empty() && game.rockClock == 2.8f && game.rockDifficulty() == 0,
          "restart resets hazards, difficulty, debris, and initial delay");
}

int main() {
    rockTests();
    Game game;
    check(game.mode == Mode::Title, "starts at title");
    game.reset();
    check(game.lives == 3 && game.score == 0 && game.grounded, "clean start");
    advance(game, 60, {1, false});
    check(std::abs(game.player.x - 262.5f) < 0.1f, "frame-rate-independent movement");
    float startY = game.player.y;
    game.step(1.0f / 120, {0, true});
    check(game.velocity.y < 0 && game.jumps == 1 && game.player.y < startY, "first jump");
    advance(game, 10);
    game.step(1.0f / 120, {0, true});
    check(game.jumps == 2 && game.velocity.y < -600, "double jump");
    advance(game, 5);
    float before = game.velocity.y;
    game.step(1.0f / 120, {0, true});
    check(game.jumps == 2 && game.velocity.y > before, "cannot triple jump");
    advance(game, 180);
    check(game.grounded && game.jumps == 0 && std::abs(game.player.y - startY) < 0.1f, "landing resets jumps");

    game.reset();
    auto& b = game.bananas.front();
    game.player = {b.pos.x - 19, b.pos.y - 25};
    game.step(1.0f / 120, {});
    check(game.score == 1 && b.eaten, "banana collection");
    game.step(1.0f / 120, {});
    check(game.score == 1, "banana only counted once");

    game.reset();
    game.rocks.push_back({{game.player.x + 19, game.player.y + 25}, 0, 0, 20});
    game.step(1.0f / 120, {});
    check(game.lives == 2 && game.invincible > 0 && game.rocks.empty(), "rock damage and safe respawn");
    game.damage();
    check(game.lives == 2, "invincibility blocks repeated damage");
    game.player.y = game.platforms[0].box.y + 600;
    game.step(1.0f / 120, {});
    check(game.lives == 1, "fall still costs a life during invincibility");
    game.invincible = 0; game.damage();
    check(game.mode == Mode::Lost && game.lives == 0, "three hits end game");

    game.reset(); game.mode = Mode::Paused;
    Vec old = game.player;
    advance(game, 120, {1, true});
    check(game.player.x == old.x && game.player.y == old.y && game.elapsed == 0, "pause freezes simulation");

    game.reset();
    game.rocks.push_back({{game.player.x + 19, game.player.y + 25}, 0, 0.95f, 20});
    float rockY = game.rocks[0].pos.y;
    advance(game, 60);
    check(game.lives == 3 && game.rocks[0].pos.y == rockY, "rock warning precedes movement and damage");

    // Traverse the actual course with ordinary inputs, not teleports.
    game.reset(); game.rockClock = 9999;
    for (std::size_t i = 1; i < game.platforms.size(); ++i) {
        auto destination = game.platforms[i].box;
        for (int frame = 0; frame < 500; ++frame) {
            auto current = game.platforms[static_cast<std::size_t>(game.checkpointIndex)].box;
            bool jump = game.grounded && game.player.x + playerWidth >= current.x + current.w - 40;
            float direction = game.player.x < destination.x + 70 ? 1.0f : 0.0f;
            game.step(1.0f / 120, {direction, jump});
            if (game.grounded && game.checkpointIndex == static_cast<int>(i)) break;
        }
        check(game.checkpointIndex == static_cast<int>(i), "platform " + std::to_string(i) + " is reachable");
        check(game.lives == 3, "course traversal without falls");
    }
    advance(game, 100, {1, false});
    check(game.mode == Mode::Won, "summit triggers victory");
    check(game.score > 0, "natural traversal collects bananas");
    game.reset();
    check(game.mode == Mode::Playing && game.score == 0 && game.lives == 3 && game.rocks.empty(), "restart resets game");
    std::cout << "PASS: " << checks << " gameplay checks\n";
}
