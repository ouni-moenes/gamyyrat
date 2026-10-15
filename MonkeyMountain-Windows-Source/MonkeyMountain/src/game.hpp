#pragma once

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <vector>

namespace mountain {
constexpr float playerWidth = 38.0f;
constexpr float playerHeight = 55.0f;
constexpr float gravity = 1550.0f;
constexpr float jumpSpeed = 625.0f;
constexpr float runSpeed = 285.0f;
constexpr float rockGravity = 780.0f;
constexpr float rockMaxSpeed = 680.0f;
constexpr std::size_t maxRocks = 4;

struct Vec { float x = 0, y = 0; };
struct Rect { float x, y, w, h; };
struct Platform { Rect box; };
struct Banana { Vec pos; bool eaten = false; };
struct Rock {
    Vec pos;
    float speed = 0, warning = 1.1f, radius = 20, warningDuration = 1.1f;
    bool shattered = false;
};
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

inline bool segmentTouchesRect(Vec from, Vec to, Rect box) {
    float enter = 0, exit = 1;
    auto clip = [&](float start, float delta, float lo, float hi) {
        if (delta == 0) return start >= lo && start <= hi;
        float a = (lo - start) / delta, b = (hi - start) / delta;
        if (a > b) std::swap(a, b);
        enter = std::max(enter, a); exit = std::min(exit, b);
        return enter <= exit;
    };
    return clip(from.x, to.x - from.x, box.x, box.x + box.w) &&
           clip(from.y, to.y - from.y, box.y, box.y + box.h);
}

inline bool sweptCircleTouches(Vec from, Vec to, float radius, Rect box) {
    if (segmentTouchesRect(from, to, {box.x - radius, box.y, box.w + 2 * radius, box.h}) ||
        segmentTouchesRect(from, to, {box.x, box.y - radius, box.w, box.h + 2 * radius})) return true;
    Vec delta{to.x - from.x, to.y - from.y};
    float lengthSquared = delta.x * delta.x + delta.y * delta.y;
    for (Vec corner : {Vec{box.x, box.y}, Vec{box.x + box.w, box.y},
                       Vec{box.x, box.y + box.h}, Vec{box.x + box.w, box.y + box.h}}) {
        float t = lengthSquared == 0 ? 0 : std::clamp(
            ((corner.x - from.x) * delta.x + (corner.y - from.y) * delta.y) / lengthSquared, 0.0f, 1.0f);
        float dx = from.x + delta.x * t - corner.x, dy = from.y + delta.y * t - corner.y;
        if (dx * dx + dy * dy <= radius * radius) return true;
    }
    return false;
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

    float rockDifficulty() const {
        return std::clamp(static_cast<float>(checkpointIndex) / (platforms.size() - 1), 0.0f, 1.0f);
    }

    float rockLandingY(const Rock& rock) const {
        float landing = std::numeric_limits<float>::infinity();
        for (const auto& platform : platforms) {
            const auto& p = platform.box;
            float dx = rock.pos.x - std::clamp(rock.pos.x, p.x, p.x + p.w);
            if (std::abs(dx) > rock.radius) continue;
            float contact = p.y - std::sqrt(std::max(0.0f, rock.radius * rock.radius - dx * dx));
            if (contact >= rock.pos.y) landing = std::min(landing, contact);
        }
        return landing;
    }

    void spawnRock() {
        float difficulty = rockDifficulty();
        float warning = 1.1f - 0.3f * difficulty;
        float target = player.x + playerWidth / 2 + velocity.x * 0.35f + range(-90, 90);
        rocks.push_back({{std::max(-100.0f, target), player.y - 430}, 0, warning, range(17, 25), warning});
    }

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
        const Vec previousPlayer = player;
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
            if (rocks.size() < maxRocks) spawnRock();
            rockClock = range(2.2f, 3.0f) - 0.65f * rockDifficulty();
        }
        bool hit = false;
        for (auto& r : rocks) {
            float activeDt = dt;
            if (r.warning > 0) {
                float waiting = std::min(r.warning, dt);
                r.warning = std::max(0.0f, r.warning - dt);
                activeDt -= waiting;
            }
            if (activeDt <= 0) continue;
            Vec from = r.pos;
            float landing = rockLandingY(r);
            float speed = std::clamp(r.speed, 0.0f, rockMaxSpeed);
            float accelerating = std::min(activeDt, (rockMaxSpeed - speed) / rockGravity);
            r.pos.y += speed * accelerating + 0.5f * rockGravity * accelerating * accelerating +
                       rockMaxSpeed * (activeDt - accelerating);
            r.speed = std::min(rockMaxSpeed, speed + rockGravity * activeDt);
            if (r.pos.y >= landing) {
                r.pos.y = landing; r.shattered = true;
                burst({r.pos.x, r.pos.y + r.radius}, false);
            }
            // Sweep in the moving player's reference frame to avoid tunneling.
            from.x += (player.x - previousPlayer.x) * (activeDt / dt);
            from.y += (player.y - previousPlayer.y) * (activeDt / dt);
            if (sweptCircleTouches(from, r.pos, r.radius * 0.85f, playerBox())) hit = true;
        }
        rocks.erase(std::remove_if(rocks.begin(), rocks.end(), [&](const Rock& r) {
            return r.shattered || r.pos.y > player.y + 850;
        }), rocks.end());
        for (auto& p : particles) {
            p.life -= dt; p.velocity.y += 500 * dt;
            p.pos.x += p.velocity.x * dt; p.pos.y += p.velocity.y * dt;
        }
        particles.erase(std::remove_if(particles.begin(), particles.end(), [](const Particle& p) {
            return p.life <= 0;
        }), particles.end());
        bool fell = player.y > platforms[static_cast<std::size_t>(checkpointIndex)].box.y + 490;
        if (hit || fell) {
            // Falling must still cost a life during the post-hit grace period.
            if (fell) invincible = 0;
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
