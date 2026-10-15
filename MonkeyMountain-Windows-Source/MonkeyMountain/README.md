# Monkey Mountain

A small 2D C++17 mountain platformer built with **raylib 5.5**. Guide a scarf-wearing monkey to the summit, eat bananas, and dodge falling rocks. All scenery and characters are drawn in code; no image assets are needed.

## Play on Windows

1. Extract the ZIP completely.
2. Open **MonkeyMountain.exe**. Keep the `assets` directory beside it.
3. Press **Enter** or click **Let's Climb**.

The included executable is for **64-bit Windows** and needs an OpenGL 3.3-capable graphics driver. No compiler, raylib installation, or extra DLLs are needed to play. Sound is optional; the game still works if no audio device is available.

| Action | Key |
| --- | --- |
| Move left / right | A / D or left / right arrows |
| Jump | Space, W, or up arrow |
| Double-jump | Release and press jump again in midair |
| Pause / resume | P or Escape |
| Restart | R |
| Toggle sound effects | M |
| Start / play again / resume | Enter |
| Quit | Close the window |

Reach the flag on the last platform to win. Bananas are your score, not a gate: collect as many as you can. You have three lives. A red **!**, countdown ring, and shaded drop lane mark where a rock will fall; move out of its path. Targets lock when the warning appears, so rocks never follow your dodges. Rocks shatter on the first platform they hit. The climb gradually brings shorter warnings (1.1 to 0.8 seconds) and more frequent drops, with at most four rocks active at once. Each new platform is a checkpoint. Falling or being struck costs one life and returns you to the latest checkpoint, with a short grace period after a hit. Losing window focus automatically pauses the game.

The bundled Windows executable predates these rock-mechanism changes. Build from source below to play the updated version.

## Build from source

### Option 1: Windows with MinGW-w64

- Install a C++17-capable MinGW-w64 toolchain, such as [w64devkit](https://github.com/skeeto/w64devkit/releases), and put its `bin` directory on `PATH`.
- Download and extract the official [raylib 5.5 MinGW-w64 package](https://github.com/raysan5/raylib/releases/download/5.5/raylib-5.5_win64_mingw-w64.zip).
- Open Command Prompt in this folder and run:

```bat
set "RAYLIB_PATH=C:\path\to\raylib-5.5_win64_mingw-w64"
build-windows.bat
```

This builds the game, compiles the tests, and runs the tests. The supplied executable was built this way with GCC 15.2.0 and raylib 5.5.

### Option 2: CMake (Windows, Linux, macOS)

Requires CMake 3.20+, Git, a C++17 compiler, and the platform's graphics development libraries. CMake uses an installed raylib 5.5 package when available; otherwise it downloads and builds the pinned **5.5** release (internet access needed for the first configure).

On Debian/Ubuntu, the usual desktop prerequisites are:

```sh
sudo apt install build-essential cmake git libasound2-dev libx11-dev \
  libxrandr-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev
```

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Run `build/MonkeyMountain` on Linux/macOS, or `build/Release/MonkeyMountain.exe` with multi-config Windows generators. Assets are copied next to the executable automatically. Other platforms are supported by the source and raylib, but the supplied build was compiled on Windows only.

### Headless gameplay tests (no raylib or graphics needed)

```sh
cmake -S . -B build-tests -DMONKEY_BUILD_GAME=OFF
cmake --build build-tests --config Release
ctest --test-dir build-tests -C Release --output-on-failure
```

Or compile directly:

```sh
g++ -std=c++17 -Wall -Wextra -Wpedantic tests/game_tests.cpp -o game_tests
./game_tests
```

## Code map

- `src/game.hpp`: platform layout, fixed-step physics, banana pickup, rock warnings, checkpoints, lives, and win/lose state. Independent of the graphics library.
- `src/main.cpp`: window/input handling, vector graphics, camera, menus, and generated sound effects.
- `tests/game_tests.cpp`: movement, jump limits, landing, scoring, damage, pause, restart, and a complete input-driven traversal of the course.

To change difficulty, adjust the movement constants and platform generation in `src/game.hpp`. `spawnRock()` controls targeting and warning duration; `rockDifficulty()` scales timing by the highest checkpoint reached. Drop intervals range from 2.2–3.0 seconds at the start to 1.55–2.35 seconds at the summit. `rockGravity`, `rockMaxSpeed`, and `maxRocks` control falling physics and the active-rock budget. Platform impacts use the rock's full radius; player collisions use a forgiving 85% radius and swept collision checks.

## Verification

- MinGW batch build: passed; all 46 gameplay checks passed.
- CMake build and CTest: passed on 64-bit Windows.
- Game and test source: passed GCC `-Wall -Wextra -Wpedantic -Werror -fsyntax-only` checks.
- Graphical startup: verified with Mesa llvmpipe because the development VM has no hardware OpenGL driver. The software renderer is only used in the development preview and is not bundled in this download.
- Audio playback could not be verified because the VM has no playback device. The game handles this by running silently.
- The complete course was verified by automated input-driven simulation; manual in-window gameplay has not been tested.

If the Windows executable does not open, update your graphics driver and check OpenGL 3.3 support. The game's source can also be built on a machine with a suitable graphics environment.

## Licenses

Original game source: MIT (see `LICENSE`). raylib: zlib/libpng (see `RAYLIB-LICENSE.txt`). Lilita One and Quicksand fonts: SIL Open Font License (see the license files in `assets`).
