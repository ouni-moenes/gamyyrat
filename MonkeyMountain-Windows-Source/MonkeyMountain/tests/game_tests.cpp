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

int main() {
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
