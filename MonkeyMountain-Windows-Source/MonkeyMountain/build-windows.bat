@echo off
setlocal
cd /d "%~dp0"
if "%RAYLIB_PATH%"=="" (
    echo Set RAYLIB_PATH to the raylib 5.5 MinGW package folder first.
    echo Example: set "RAYLIB_PATH=C:\raylib-5.5_win64_mingw-w64"
    exit /b 1
)
where g++ >nul 2>nul
if errorlevel 1 (
    echo Add your MinGW-w64 compiler's bin folder to PATH first.
    exit /b 1
)
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -I"%RAYLIB_PATH%\include" src\main.cpp -L"%RAYLIB_PATH%\lib" -lraylib -lopengl32 -lgdi32 -lwinmm -static -mwindows -o MonkeyMountain.exe
if errorlevel 1 exit /b 1
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic tests\game_tests.cpp -o game_tests.exe
if errorlevel 1 exit /b 1
game_tests.exe
if errorlevel 1 exit /b 1
echo Build and gameplay checks passed. Run MonkeyMountain.exe.
