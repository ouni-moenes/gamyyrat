#include "game.hpp"
#include "raylib.h"
#include "rlgl.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

namespace {
constexpr int width = 1200, height = 760;
constexpr float pi = 3.14159265f;
const Color ink{27, 56, 52, 255}, muted{91, 120, 104, 255};
const Color cream{255, 250, 229, 255}, gold{246, 189, 64, 255};
const Color green{57, 110, 72, 255}, leaf{122, 165, 86, 255};

struct Art {
    Font title{}, body{};
    float cameraX = 0, cameraY = 0, time = 0;
    Vector2 screen(float x, float y) const { return {x - cameraX, y - cameraY}; }
    void text(const std::string& s, float x, float y, float size, Color color, bool bold = false) const {
        DrawTextEx(bold ? title : body, s.c_str(), {x, y}, size, 0.6f, color);
    }
    void centered(const std::string& s, float x, float y, float size, Color color, bool bold = false) const {
        Font f = bold ? title : body;
        DrawTextEx(f, s.c_str(), {x - MeasureTextEx(f, s.c_str(), size, 0.6f).x / 2, y}, size, 0.6f, color);
    }
};

void pill(Rectangle box, Color c) { DrawRectangleRounded(box, 0.3f, 12, c); }
void line(Vector2 a, Vector2 b, float thickness, Color c) { DrawLineEx(a, b, thickness, c); }

void banana(float x, float y, float size = 1, float rotation = -0.25f) {
    auto point = [&](float px, float py) -> Vector2 {
        return {x + (px * std::cos(rotation) - py * std::sin(rotation)) * size,
                y + (px * std::sin(rotation) + py * std::cos(rotation)) * size};
    };
    float degrees = rotation * 180 / pi;
    DrawRing(point(0, -7), 11 * size, 19 * size, degrees + 8, degrees + 172, 28, Color{210, 142, 40, 255});
    DrawRing(point(0, -8), 11 * size, 17 * size, degrees + 8, degrees + 172, 28, gold);
    line(point(-16, -5), point(-19, -12), 4 * size, ink);
    line(point(16, -5), point(19, -10), 3 * size, ink);
}

void heart(float x, float y, bool full) {
    Color c = full ? Color{205, 94, 73, 255} : Color{208, 216, 192, 255};
    DrawCircleV({x - 5, y - 3}, 7, c); DrawCircleV({x + 5, y - 3}, 7, c);
    DrawTriangle({x - 11, y - 1}, {x, y + 13}, {x + 11, y - 1}, c);
}

void monkey(float x, float y, float scale, int facing, bool airborne, float gait, float alpha = 1) {
    auto p = [&](float a, float b) -> Vector2 { return {x + a * scale * facing, y + b * scale}; };
    const Color fur = Fade(Color{126, 69, 42, 255}, alpha);
    const Color dark = Fade(Color{91, 52, 36, 255}, alpha);
    const Color face = Fade(Color{237, 188, 126, 255}, alpha);
    const Color black = Fade(ink, alpha);
    auto limb = [&](float a, float b, float c, float d, float w, Color color) {
        line(p(a, b), p(c, d), w * scale, color); DrawCircleV(p(c, d), w * scale / 2, color);
    };
    std::array<Vector2, 5> tail{p(-10, 34), p(-29, 40), p(-43, 29), p(-42, 13), p(-33, 10)};
    for (std::size_t i = 1; i < tail.size(); ++i) line(tail[i - 1], tail[i], 6 * scale, dark);
    DrawCircleV(tail.back(), 3 * scale, dark);
    float stride = airborne ? 8 : std::sin(gait) * 6;
    limb(-7, 37, -10 - stride, airborne ? 47 : 53, 10, dark);
    limb(8, 37, 11 + stride, airborne ? 45 : 53, 10, fur);
    DrawEllipse(static_cast<int>(x), static_cast<int>(y + 30 * scale), 15 * scale, 20 * scale, fur);
    DrawEllipse(static_cast<int>(x + 3 * scale * facing), static_cast<int>(y + 32 * scale), 9 * scale, 13 * scale, face);
    limb(-12, 24, -22, airborne ? 13 : 35 - stride, 8, dark);
    limb(12, 24, 23, airborne ? 10 : 35 + stride, 8, fur);
    DrawCircleV(p(-17, 5), 9 * scale, fur); DrawCircleV(p(17, 5), 9 * scale, fur);
    DrawCircleV(p(-17, 5), 5 * scale, face); DrawCircleV(p(17, 5), 5 * scale, face);
    DrawCircleV(p(0, 4), 20 * scale, fur);
    DrawCircleV(p(-7, 5), 11 * scale, face); DrawCircleV(p(7, 5), 11 * scale, face);
    DrawEllipse(static_cast<int>(x), static_cast<int>(y + 12 * scale), 15 * scale, 10 * scale, face);
    DrawCircleV(p(-6, 3), 2.5f * scale, black); DrawCircleV(p(7, 3), 2.5f * scale, black);
    DrawCircleV(p(-6.8f, 2), 0.8f * scale, Fade(WHITE, alpha));
    DrawCircleV(p(6.2f, 2), 0.8f * scale, Fade(WHITE, alpha));
    DrawEllipse(static_cast<int>(x + 1 * scale * facing), static_cast<int>(y + 10 * scale), 3 * scale, 2 * scale, dark);
    line(p(-4, 15), p(0, 17), 1.5f * scale, black); line(p(0, 17), p(5, 14), 1.5f * scale, black);
    DrawTriangle(p(-9, -13), p(-1, -23), p(5, -13), fur);
    line(p(-12, 21), p(11, 21), 5 * scale, Fade(Color{219, 98, 64, 255}, alpha));
    DrawTriangle(p(8, 20), p(12, 34), p(20, 25), Fade(Color{219, 98, 64, 255}, alpha));
}

void pine(float x, float y, float size, Color c) {
    line({x, y - size * 0.2f}, {x, y + 9 * size}, 4 * size, c);
    for (int i = 0; i < 3; ++i) {
        float by = y - size * (15 - i * 13);
        float half = size * (14 + i * 5);
        DrawTriangle({x, by - size * 27}, {x - half, by + 7 * size}, {x + half, by + 7 * size}, c);
    }
}

void background(const Art& a) {
    DrawRectangleGradientV(0, 0, width, height, Color{224, 237, 208, 255}, Color{250, 242, 211, 255});
    DrawCircleV({953, 147}, 69, Color{246, 218, 143, 255});
    DrawCircleLines(953, 147, 83, Color{240, 220, 162, 255});
    for (int layer = 0; layer < 3; ++layer) {
        float parallax = 0.08f + layer * 0.1f;
        float drift = std::fmod(a.cameraX * parallax, 580.0f);
        Color c = layer == 0 ? Color{181, 203, 174, 255} : layer == 1 ? Color{147, 177, 150, 255} : Color{113, 153, 125, 255};
        for (int i = -1; i < 4; ++i) {
            float x = i * 580.0f - drift + layer * 160;
            float peak = 245.0f + layer * 80 + (i % 2) * 50 - a.cameraY * parallax * 0.3f;
            DrawTriangle({x + 255, peak}, {x - 100, 810}, {x + 640, 810}, c);
            if (layer == 0) {
                DrawTriangle({x + 255, peak}, {x + 188, peak + 111}, {x + 330, peak + 98}, Color{238, 238, 207, 255});
                DrawTriangle({x + 255, peak + 52}, {x + 214, peak + 112}, {x + 270, peak + 84}, c);
            }
        }
    }
    for (int i = 0; i < 18; ++i) {
        float x = i * 93.0f - std::fmod(a.cameraX * 0.32f, 93.0f);
        pine(x, 745 + std::sin(i * 1.9f) * 23, 1.5f + (i % 3) * 0.3f, Color{87, 133, 109, 255});
    }
    for (int i = 0; i < 4; ++i) {
        float x = std::fmod(i * 340.0f + a.time * 7, 1450.0f) - 100;
        float y = 108.0f + (i % 2) * 95;
        DrawEllipse(static_cast<int>(x), static_cast<int>(y), 57, 12, Fade(cream, 0.35f));
        DrawEllipse(static_cast<int>(x + 17), static_cast<int>(y - 8), 32, 17, Fade(cream, 0.35f));
    }
    for (int i = 0; i < 3; ++i) {
        float x = 310.0f + i * 37;
        float y = 153.0f + std::sin(i * 2.0f) * 14;
        line({x - 5, y}, {x, y + 3}, 1.5f, Fade(ink, 0.45f));
        line({x, y + 3}, {x + 5, y}, 1.5f, Fade(ink, 0.45f));
    }
}

void island(const Art& a, mountain::Rect box, int index) {
    Vector2 pos = a.screen(box.x, box.y);
    if (pos.x + box.w < -100 || pos.x > width + 100 || pos.y > height + 100) return;
    float x = pos.x, y = pos.y, w = box.w, h = box.h;
    DrawTriangle({x, y + 8}, {x + w * 0.35f, y + h}, {x + w, y + 8}, Color{135, 113, 79, 255});
    DrawTriangle({x + w * 0.35f, y + h}, {x + w * 0.85f, y + h * 0.7f}, {x + w, y + 8}, Color{112, 99, 76, 255});
    DrawRectangleRounded({x, y, w, 40}, 0.3f, 8, Color{158, 125, 80, 255});
    DrawRectangleRounded({x - 3, y - 5, w + 6, 18}, 0.6f, 10, green);
    DrawRectangleRounded({x, y - 5, w, 7}, 0.6f, 8, leaf);
    for (int j = 0; j < static_cast<int>(w / 22); ++j) {
        float px = x + j * 22 + 13;
        DrawCircleV({px, y + 15 + (j % 3) * 5.0f}, 7, green);
        if (j % 3 == 0) {
            line({px, y - 5}, {px - 3, y - 13}, 2, green);
            line({px, y - 5}, {px + 4, y - 11}, 2, green);
        }
        if (j % 4 == 0) DrawCircleV({px + 7, y + 42}, 3, Color{183, 152, 105, 255});
    }
    if (index % 3 == 1) {
        pine(x + w - 26, y - 7, 0.55f, green);
    } else if (index % 3 == 2) {
        DrawEllipse(static_cast<int>(x + 25), static_cast<int>(y - 8), 18, 11, muted);
    }
    if (index == 0) {
        line({x + 475, y - 1}, {x + 475, y - 65}, 6, Color{133, 92, 53, 255});
        pill({x + 418, y - 77, 114, 33}, Color{223, 188, 126, 255});
        a.centered("SUMMIT  >", x + 475, y - 71, 17, ink, true);
        pine(x + 225, y - 8, 1.1f, green);
    }
}

void rock(float x, float y, float radius, float angle) {
    DrawPoly({x, y}, 7, radius + 2, angle, Color{74, 83, 73, 255});
    DrawPoly({x - 2, y - 2}, 6, radius, angle, Color{120, 128, 105, 255});
    DrawTriangle({x - radius * 0.6f, y - radius * 0.2f}, {x - 3, y + radius * 0.4f},
                 {x + radius * 0.5f, y - radius * 0.6f}, Color{153, 156, 129, 255});
    line({x + 2, y + 3}, {x + radius * 0.55f, y + radius * 0.45f}, 2, Color{87, 97, 81, 255});
}

bool button(const Art& a, Rectangle box, const std::string& label, Vector2 mouse) {
    bool hover = CheckCollisionPointRec(mouse, box);
    pill({box.x, box.y + 5, box.width, box.height}, Color{39, 77, 53, 255});
    pill(box, hover ? Color{76, 133, 82, 255} : green);
    a.centered(label, box.x + box.width / 2, box.y + 15, 25, cream, true);
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void keycap(const Art& a, const std::string& label, float x, float y, float w = 36) {
    pill({x, y + 2, w, 30}, Color{190, 202, 172, 255});
    pill({x, y, w, 28}, cream);
    a.centered(label, x + w / 2, y + 5, 16, ink, true);
}

void titleScreen(Art& a, Vector2 mouse, bool& start) {
    DrawRectangleGradientH(0, 0, 740, height, Fade(cream, 0.96f), Fade(cream, 0));
    a.text("A LITTLE MONKEY. A BIG ADVENTURE.", 83, 107, 17, muted, true);
    a.text("MONKEY", 77, 151, 104, ink, true);
    a.text("MOUNTAIN", 77, 245, 104, ink, true);
    a.text("Leap higher. Snack often. Watch the sky.", 85, 366, 23, ink);
    a.text("Collect bananas and dodge falling rocks", 85, 407, 20, muted);
    a.text("on a little adventure to the summit.", 85, 436, 20, muted);
    if (button(a, {85, 493, 257, 60}, "LET'S CLIMB  >", mouse)) start = true;
    a.text("or press ENTER", 113, 570, 17, muted);
    keycap(a, "A", 85, 635); keycap(a, "D", 127, 635);
    a.text("move", 176, 641, 17, ink);
    keycap(a, "SPACE", 265, 635, 82); a.text("double-jump", 360, 641, 17, ink);
    Art stage = a; stage.cameraX = 0; stage.cameraY = 0;
    island(stage, {720, 572, 410, 210}, 1);
    DrawEllipse(925, 565, 53, 10, Fade(ink, 0.14f));
    monkey(930, 414 + std::sin(a.time * 2) * 3, 2.7f, -1, false, 0);
    banana(794, 385 + std::sin(a.time * 2 + 1) * 7, 1.65f);
    banana(864, 319 + std::sin(a.time * 2 + 2) * 7, 1.65f);
    banana(954, 291 + std::sin(a.time * 2 + 3) * 7, 1.65f);
    a.centered("THE ONLY WAY IS UP", 922, 646, 17, cream, true);
    a.text("01 / SUMMIT TRAIL", 86, 718, 13, muted, true);
    a.text("MADE OF SMALL LEAPS & BIG BANANA ENERGY", 746, 718, 13, ink, true);
}

void drawWorld(const Art& a, const mountain::Game& game) {
    for (std::size_t i = 0; i < game.platforms.size(); ++i) island(a, game.platforms[i].box, static_cast<int>(i));
    for (std::size_t i = 0; i < game.bananas.size(); ++i) {
        const auto& b = game.bananas[i];
        if (b.eaten) continue;
        Vector2 p = a.screen(b.pos.x, b.pos.y);
        if (p.x < -40 || p.x > width + 40 || p.y < -40 || p.y > height + 40) continue;
        float bob = std::sin(a.time * 3 + static_cast<float>(i)) * 4;
        DrawCircleV({p.x, p.y + bob}, 22, Fade(gold, 0.1f));
        banana(p.x, p.y + bob);
        DrawCircleV({p.x + 19, p.y - 18 + bob}, 2, cream);
    }
    const auto& last = game.platforms.back().box;
    Vector2 flag = a.screen(game.summitX() + 10, last.y);
    line({flag.x, flag.y}, {flag.x, flag.y - 114}, 5, ink);
    DrawTriangle({flag.x, flag.y - 112}, {flag.x, flag.y - 65}, {flag.x + 61, flag.y - 88}, gold);
    a.centered("SUMMIT!", flag.x, flag.y - 145, 22, ink, true);
    for (const auto& r : game.rocks) {
        Vector2 p = a.screen(r.pos.x, r.pos.y);
        if (p.x + r.radius < 0 || p.x - r.radius > width) continue;
        const Color danger{198, 86, 58, 255};
        float landing = game.rockLandingY(r);
        float groundY = std::isfinite(landing) ? landing + r.radius - a.cameraY : height - 65.0f;
        float pathTop = std::clamp(p.y + r.radius, 118.0f, height - 65.0f);
        float pathBottom = std::clamp(groundY, 118.0f, height - 65.0f);
        if (pathBottom > pathTop) {
            DrawRectangleRec({p.x - r.radius, pathTop, r.radius * 2, pathBottom - pathTop}, Fade(danger, 0.06f));
            for (float y = pathTop; y < pathBottom; y += 18)
                line({p.x, y}, {p.x, std::min(y + 8, pathBottom)}, 2, Fade(danger, 0.3f));
        }
        if (std::isfinite(landing) && groundY > 110 && groundY < height - 50)
            DrawEllipse(static_cast<int>(p.x), static_cast<int>(groundY), r.radius + 7, 6,
                        Fade(danger, r.warning > 0 ? 0.35f : 0.55f));
        if (r.warning > 0) {
            float sy = std::clamp(p.y, 150.0f, 430.0f);
            DrawCircleV({p.x, sy}, 19 + std::sin(a.time * 17) * 2, danger);
            float countdown = std::clamp(r.warning / std::max(0.001f, r.warningDuration), 0.0f, 1.0f);
            DrawRing({p.x, sy}, 24, 28, -90, -90 + 360 * countdown, 48, danger);
            a.centered("!", p.x, sy - 15, 30, cream, true);
            a.centered("ROCK!", p.x, sy - 43, 15, ink, true);
            char remaining[16];
            std::snprintf(remaining, sizeof(remaining), "%.1fs", std::ceil(r.warning * 10) / 10);
            a.centered(remaining, p.x, sy + 34, 15, ink, true);
        } else {
            line({p.x, p.y - 20}, {p.x - 2, p.y - 49}, 3, Fade(ink, 0.16f));
            rock(p.x, p.y, r.radius, a.time * 145);
        }
    }
    Vector2 p = a.screen(game.player.x + mountain::playerWidth / 2, game.player.y + 4);
    if (game.grounded) DrawEllipse(static_cast<int>(p.x), static_cast<int>(p.y + 50), 23, 5, Fade(ink, 0.18f));
    float alpha = game.invincible > 0 && static_cast<int>(a.time * 14) % 2 == 0 ? 0.35f : 1.0f;
    monkey(p.x, p.y, 0.95f, game.facing, !game.grounded, game.player.x * 0.05f, alpha);
    for (const auto& particle : game.particles) {
        Vector2 pos = a.screen(particle.pos.x, particle.pos.y);
        DrawCircleV(pos, 3, Fade(particle.golden ? gold : cream, std::min(1.0f, particle.life * 2)));
    }
}

void hud(const Art& a, const mountain::Game& game, bool mutedAudio) {
    pill({24, 23, 246, 78}, cream);
    banana(53, 61, 0.85f);
    a.text("BANANAS", 79, 36, 13, muted, true);
    a.text(std::to_string(game.score), 79, 54, 31, ink, true);
    a.text("/ " + std::to_string(game.bananas.size()), 124, 66, 15, muted);
    for (int i = 0; i < 3; ++i) heart(185.0f + i * 26, 64, i < game.lives);
    pill({403, 23, 394, 67}, cream);
    a.text("SUMMIT TRAIL", 424, 34, 13, muted, true);
    a.text(std::to_string(game.altitude()) + " m", 711, 32, 19, ink, true);
    pill({424, 61, 349, 6}, Color{217, 224, 195, 255});
    if (game.progress() > 0) pill({424, 61, std::max(6.0f, 349 * game.progress()), 6}, green);
    DrawCircleV({424 + 349 * game.progress(), 64}, 5, gold);
    pill({958, 23, 218, 53}, cream);
    a.text("P  PAUSE", 976, 41, 15, ink, true);
    a.text(mutedAudio ? "M  OFF" : "M  SOUND", 1070, 41, 14, muted, true);
    pill({308, 699, 584, 40}, Fade(cream, 0.93f));
    a.centered("A / D  MOVE     SPACE  DOUBLE-JUMP     R  RESTART", 600, 710, 15, ink, true);
    if (game.elapsed < 6) {
        pill({373, 124, 454, 44}, Fade(cream, std::min(1.0f, 6 - game.elapsed)));
        a.centered("Jump twice to reach higher ledges!", 600, 135, 19, ink);
    }
}

void overlay(Art& a, const mountain::Game& game, Vector2 mouse, bool& resume, bool& restart) {
    DrawRectangle(0, 0, width, height, Fade(ink, 0.48f));
    pill({361, 180, 478, 392}, cream);
    bool won = game.mode == mountain::Mode::Won, paused = game.mode == mountain::Mode::Paused;
    a.centered(paused ? "TAKE A BREATHER" : won ? "TOP BANANA!" : "ONE MORE LEAP?", 600, 218, 43, ink, true);
    if (won) { banana(600, 311, 2); }
    else if (paused) { monkey(600, 288, 1.2f, 1, false, 0); }
    else { rock(600, 311, 27, 25); }
    a.centered(paused ? "The mountain can wait." : won ? "You made it to the summit." : "Those rocks play rough.", 600, 362, 22, ink);
    std::string detail = paused ? "P / ESC to resume  |  R to start over" :
        std::to_string(game.score) + " bananas collected  /  " + std::to_string(static_cast<int>(game.elapsed)) + " seconds";
    a.centered(detail, 600, 401, 18, muted);
    if (button(a, {442, 455, 316, 59}, paused ? "KEEP CLIMBING  >" : "PLAY AGAIN  >", mouse)) {
        if (paused) resume = true; else restart = true;
    }
    a.centered(paused ? "press ENTER to resume" : "press ENTER to try again", 600, 535, 15, muted);
}

Sound makeTone(float frequency, float seconds, bool slide) {
    constexpr unsigned sampleRate = 22050;
    unsigned count = static_cast<unsigned>(seconds * sampleRate);
    std::vector<std::int16_t> samples(count);
    float phase = 0;
    for (unsigned i = 0; i < count; ++i) {
        float t = static_cast<float>(i) / sampleRate;
        float f = frequency * (slide ? (1 + t / seconds) : 1);
        phase += 2 * pi * f / sampleRate;
        float envelope = std::min(1.0f, t * 100) * (1 - t / seconds);
        samples[i] = static_cast<std::int16_t>(std::sin(phase) * envelope * 8500);
    }
    Wave wave{count, sampleRate, 16, 1, samples.data()};
    return LoadSoundFromWave(wave);
}
} // namespace

int main(int argc, char** argv) {
    if (argc > 0) {
        std::error_code error;
        auto path = std::filesystem::absolute(argv[0], error).parent_path();
        if (!error && std::filesystem::exists(path / "assets")) std::filesystem::current_path(path, error);
    }
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(width, height, "Monkey Mountain - A Little Summit Adventure");
    if (!IsWindowReady()) {
        std::fprintf(stderr, "Cannot open the game. An OpenGL 3.3 graphics driver is required.\n");
        return 1;
    }
    SetWindowMinSize(720, 456); SetExitKey(KEY_NULL); SetTargetFPS(60);
    RenderTexture2D canvas = LoadRenderTexture(width, height);
    SetTextureFilter(canvas.texture, TEXTURE_FILTER_BILINEAR);
    Art art;
    art.title = FileExists("assets/LilitaOne-Regular.ttf") ? LoadFontEx("assets/LilitaOne-Regular.ttf", 112, nullptr, 0) : GetFontDefault();
    art.body = FileExists("assets/Quicksand.ttf") ? LoadFontEx("assets/Quicksand.ttf", 44, nullptr, 0) : GetFontDefault();
    SetTextureFilter(art.title.texture, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(art.body.texture, TEXTURE_FILTER_BILINEAR);
    InitAudioDevice();
    bool audioReady = IsAudioDeviceReady(), mutedAudio = false;
    std::array<Sound, 3> sounds{};
    if (audioReady) {
        sounds = {makeTone(350, 0.13f, true), makeTone(740, 0.16f, true), makeTone(130, 0.25f, false)};
        for (auto sound : sounds) SetSoundVolume(sound, 0.3f);
    }
    mountain::Game game(static_cast<unsigned>(GetTime() * 10000));
    float accumulator = 0;
    bool jumpQueued = false;
    while (!WindowShouldClose()) {
        const float dt = std::min(GetFrameTime(), 0.1f);
        art.time += dt;
        float scale = std::min(static_cast<float>(GetScreenWidth()) / width, static_cast<float>(GetScreenHeight()) / height);
        float offsetX = (GetScreenWidth() - width * scale) / 2;
        float offsetY = (GetScreenHeight() - height * scale) / 2;
        Vector2 mouse{(GetMouseX() - offsetX) / scale, (GetMouseY() - offsetY) / scale};
        if (IsKeyPressed(KEY_M)) mutedAudio = !mutedAudio;
        bool enter = IsKeyPressed(KEY_ENTER);
        bool pause = IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE);
        if (game.mode == mountain::Mode::Playing && (pause || !IsWindowFocused())) {
            game.mode = mountain::Mode::Paused; accumulator = 0; jumpQueued = false;
        } else if (game.mode == mountain::Mode::Paused && (pause || enter)) game.mode = mountain::Mode::Playing;
        else if (enter && game.mode != mountain::Mode::Playing) {
            game.reset(); art.cameraX = art.cameraY = 0; accumulator = 0; jumpQueued = false;
        }
        if (IsKeyPressed(KEY_R) && game.mode != mountain::Mode::Title) {
            game.reset(); art.cameraX = art.cameraY = 0; accumulator = 0; jumpQueued = false;
        }
        if (game.mode == mountain::Mode::Playing) {
            jumpQueued = jumpQueued || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W);
            float move = static_cast<float>(IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) -
                         static_cast<float>(IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT));
            auto oldJump = game.jumpEvents, oldEat = game.eatEvents, oldHurt = game.hurtEvents;
            accumulator += dt;
            while (accumulator >= 1.0f / 120) {
                game.step(1.0f / 120, {move, jumpQueued});
                jumpQueued = false; accumulator -= 1.0f / 120;
            }
            if (audioReady && !mutedAudio) {
                if (oldJump != game.jumpEvents) PlaySound(sounds[0]);
                if (oldEat != game.eatEvents) PlaySound(sounds[1]);
                if (oldHurt != game.hurtEvents) PlaySound(sounds[2]);
            }
        }
        if (game.mode != mountain::Mode::Title) {
            float ease = 1 - std::exp(-7 * dt);
            art.cameraX += (std::max(0.0f, game.player.x - 350) - art.cameraX) * ease;
            art.cameraY += (std::min(0.0f, game.player.y - 420) - art.cameraY) * ease;
        }
        bool start = false, resume = false, restart = false;
        BeginTextureMode(canvas);
        rlSetBlendFactorsSeparate(RL_SRC_ALPHA, RL_ONE_MINUS_SRC_ALPHA, RL_ONE, RL_ONE_MINUS_SRC_ALPHA, RL_FUNC_ADD, RL_FUNC_ADD);
        BeginBlendMode(BLEND_CUSTOM_SEPARATE);
        ClearBackground(cream); background(art);
        if (game.mode == mountain::Mode::Title) titleScreen(art, mouse, start);
        else {
            drawWorld(art, game); hud(art, game, mutedAudio);
            if (game.mode != mountain::Mode::Playing) overlay(art, game, mouse, resume, restart);
        }
        EndBlendMode(); EndTextureMode();
        BeginDrawing(); ClearBackground(ink);
        DrawTexturePro(canvas.texture, {0, 0, static_cast<float>(width), -static_cast<float>(height)},
                       {offsetX, offsetY, width * scale, height * scale}, {0, 0}, 0, WHITE);
        EndDrawing();
        if (start || restart) { game.reset(); art.cameraX = art.cameraY = 0; accumulator = 0; jumpQueued = false; }
        if (resume) game.mode = mountain::Mode::Playing;
    }
    if (audioReady) { for (auto sound : sounds) UnloadSound(sound); }
    CloseAudioDevice();
    if (art.title.texture.id != GetFontDefault().texture.id) UnloadFont(art.title);
    if (art.body.texture.id != GetFontDefault().texture.id) UnloadFont(art.body);
    UnloadRenderTexture(canvas); CloseWindow();
    return 0;
}
