#pragma once

#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

namespace mountain {
constexpr float playerWidth = 38.0f;
constexpr float playerHeight = 55.0f;
constexpr float gravity = 1550.0f;
constexpr float jumpSpeed = 625.0f;
constexpr float runSpeed = 285.0f;

struct Vec { float x = 0, y = 0; };
struct Rect { float x, y, w, h; };
struct Platform { Rect box; };
struct Banana { Vec pos; bool eaten = false; };
struct Rock { Vec pos; float speed = 0, warning = 0.95f, radius = 20; };
struct Particle { Vec pos, velocity; float life; bool golden; };
struct Input { float move = 0; bool jump = false; };
enum class Mode { Title, Playing, Paused, Lost, Won };

inline bool overlaps(Rect a, Rect b) {
    return a.x < b.x + b.w && a.x + a.w > b.x &&
           a.y < b.y + b.h && a.y + a.h > b.y;
}

inline bool circleTouches(Vec center, float radius, Rect box) {
    float dx = center.x - std::clamp(center.x, box.x, box.x + box.w);
    float dy = center.y - std::clamp(center.y, box.y, box.y + box.h);
    return dx * dx + dy * dy <= radius * radius;
}

class Game {
public:
    Mode mode = Mode::Title;
    Vec player{}, velocity{}, checkpoint{};
    std::vector<Platform> platforms;
    std::vector<Banana> bananas;
    std::vector<Rock> rocks;
    std::vector<Particle> particles;
    int lives = 3, score = 0, jumps = 0, facing = 1, checkpointIndex = 0;
    float elapsed = 0, invincible = 0, coyote = 0, jumpBuffer = 0;
    float rockClock = 2.8f;
    bool grounded = true;
    unsigned jumpEvents = 0, eatEvents = 0, hurtEvents = 0;

    explicit Game(unsigned seed = 42) : random(seed) { reset(); mode = Mode::Title; }

    void reset() {
        platforms.clear(); bananas.clear(); rocks.clear(); particles.clear();
        platforms.push_back({{-180, 590, 600, 170}});
        float x = 490, y = 520;
        for (int i = 0; i < 14; ++i) {
            float width = 210.0f + (i % 3) * 20.0f;
            platforms.push_back({{x, y, width, 120.0f + (i % 3) * 16.0f}});
            for (int j = 0; j < 3; ++j)
                bananas.push_back({{x + 45 + j * (width - 90) / 2, y - 53 - (j == 1 ? 28 : 0)}, false});
            x += width + 65.0f + (i % 3) * 12;
            y -= 55.0f + (i % 3) * 10;
        }
        player = {120, 590 - playerHeight}; velocity = {};
        checkpoint = player; lives = 3; score = 0; jumps = 0; facing = 1;
        elapsed = 0; invincible = 0; coyote = 0.1f; jumpBuffer = 0;
        rockClock = 2.8f; grounded = true; checkpointIndex = 0;
        jumpEvents = eatEvents = hurtEvents = 0;
        mode = Mode::Playing;
    }

    Rect playerBox() const { return {player.x, player.y, playerWidth, playerHeight}; }
    float summitX() const { const auto& p = platforms.back().box; return p.x + p.w - 60; }
    float progress() const { return std::clamp((player.x - 120) / (summitX() - 120), 0.0f, 1.0f); }
    int altitude() const { return std::max(0, static_cast<int>((535 - player.y) / 8)); }
    float range(float lo, float hi) { return std::uniform_real_distribution<float>(lo, hi)(random); }

    void burst(Vec pos, bool golden) {
        for (int i = 0; i < 12; ++i)
            particles.push_back({pos, {range(-110, 110), range(-200, -35)}, range(0.3f, 0.65f), golden});
    }

    void damage() {
        if (invincible > 0 || mode != Mode::Playing) return;
        --lives; ++hurtEvents;
        burst({player.x + 19, player.y + 25}, false);
        if (lives <= 0) { mode = Mode::Lost; return; }
        player = checkpoint; velocity = {}; grounded = true;
        jumps = 0; coyote = 0.1f; jumpBuffer = 0; invincible = 2.0f;
        rocks.clear(); rockClock = 2.0f;
    }

    void step(float dt, Input input) {
        if (mode != Mode::Playing || dt <= 0) return;
        dt = std::min(dt, 1.0f / 30.0f);
        elapsed += dt;
        invincible = std::max(0.0f, invincible - dt);
        coyote = grounded ? 0.1f : std::max(0.0f, coyote - dt);
        jumpBuffer = input.jump ? 0.13f : std::max(0.0f, jumpBuffer - dt);
        const float move = std::clamp(input.move, -1.0f, 1.0f);
        velocity.x = move * runSpeed;
        if (move != 0) facing = move > 0 ? 1 : -1;
        if (!grounded && coyote == 0 && jumps == 0) jumps = 1;
        if (jumpBuffer > 0 && (coyote > 0 || jumps < 2)) {
            if (coyote > 0) jumps = 0;
            velocity.y = -jumpSpeed; ++jumps; ++jumpEvents;
            grounded = false; coyote = 0; jumpBuffer = 0;
        }
        const float oldFeet = player.y + playerHeight;
        player.x = std::max(-130.0f, player.x + velocity.x * dt);
        velocity.y += gravity * dt;
        player.y += velocity.y * dt;
        grounded = false;
        if (velocity.y >= 0) {
            for (std::size_t i = 0; i < platforms.size(); ++i) {
                const auto& p = platforms[i].box;
                if (player.x + playerWidth > p.x && player.x < p.x + p.w &&
                    oldFeet <= p.y + 1 && player.y + playerHeight >= p.y) {
                    player.y = p.y - playerHeight; velocity.y = 0;
                    grounded = true; jumps = 0;
                    if (static_cast<int>(i) >= checkpointIndex) {
                        checkpointIndex = static_cast<int>(i);
                        checkpoint = {std::clamp(player.x, p.x + 15, p.x + p.w - playerWidth - 15), player.y};
                    }
                    break;
                }
            }
        }
        for (auto& b : bananas) {
            if (!b.eaten && circleTouches(b.pos, 19, playerBox())) {
                b.eaten = true; ++score; ++eatEvents; burst(b.pos, true);
            }
        }
        rockClock -= dt;
        if (rockClock <= 0) {
            float target = player.x + playerWidth / 2 + velocity.x * 0.5f + range(-75, 75);
            rocks.push_back({{std::max(-100.0f, target), player.y - 410}, 0, 0.95f, range(17, 25)});
            rockClock = range(1.4f, 2.3f);
        }
        bool hit = false;
        for (auto& r : rocks) {
            if (r.warning > 0) { r.warning -= dt; continue; }
            r.speed += 570 * dt; r.pos.y += r.speed * dt;
            if (circleTouches(r.pos, r.radius * 0.85f, playerBox())) hit = true;
        }
        rocks.erase(std::remove_if(rocks.begin(), rocks.end(), [&](const Rock& r) {
            return r.pos.y > player.y + 850;
        }), rocks.end());
        for (auto& p : particles) {
            p.life -= dt; p.velocity.y += 500 * dt;
            p.pos.x += p.velocity.x * dt; p.pos.y += p.velocity.y * dt;
        }
        particles.erase(std::remove_if(particles.begin(), particles.end(), [](const Particle& p) {
            return p.life <= 0;
        }), particles.end());
        if (hit || player.y > platforms[static_cast<std::size_t>(checkpointIndex)].box.y + 490) {
            // Falling must still cost a life during the post-hit grace period.
            if (!hit) invincible = 0;
            damage();
        }
        const auto& summit = platforms.back().box;
        if (mode == Mode::Playing && grounded && player.x + playerWidth >= summitX() &&
            player.x < summit.x + summit.w && checkpointIndex == static_cast<int>(platforms.size()) - 1)
            mode = Mode::Won;
    }

private:
    std::mt19937 random;
};
} // namespace mountain
