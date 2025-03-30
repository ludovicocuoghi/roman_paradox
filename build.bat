@echo off
set PATH=C:\mingw64\bin;%PATH%
set SFML=C:\SFML
set OUTDIR=bin

if not exist %OUTDIR% mkdir %OUTDIR%

echo [INFO] Starting compilation...

:: Compile
g++ -std=c++20 -Wall -fdiagnostics-color=always ^
-I %SFML%\include ^
-I src ^
-I src\imgui ^
-I src\imgui-sfml ^
main.cpp ^
src\GameEngine.cpp src\Scene.cpp src\Scene_Play.cpp src\Scene_LevelEditor.cpp src\Scene_Menu.cpp ^
src\systems\LoadLevel.cpp src\systems\PlayRenderer.cpp src\systems\CollisionSystem.cpp src\Scene_GameOver.cpp ^
src\Assets.cpp src\systems\MovementSystem.cpp src\systems\AnimationSystem.cpp src\systems\EnemyAISystem.cpp ^
src\systems\Spawner.cpp src\systems\DialogueSystem.cpp src\Scene_StoryText.cpp src\ResourcePath.cpp ^
src\imgui\imgui.cpp ^
src\imgui\imgui_draw.cpp ^
src\imgui\imgui_tables.cpp ^
src\imgui\imgui_widgets.cpp ^
src\imgui\imgui_demo.cpp ^
src\imgui-sfml\imgui-SFML.cpp ^
resources.o ^
-L %SFML%\lib -lsfml-graphics -lsfml-window -lsfml-system -lopengl32 -lgdi32 ^
-o %OUTDIR%\rome_journey.exe

:: Show compiler error if it failed
if errorlevel 1 (
    echo.
    echo [ERROR] Compilation failed. Check above for details.
    pause
    exit /b 1
)

echo [INFO] Compilation succeeded. Copying files...

:: Copy SFML DLLs to output directory
copy /Y "%SFML%\bin\sfml-graphics-2.dll" "%OUTDIR%"
copy /Y "%SFML%\bin\sfml-window-2.dll" "%OUTDIR%"
copy /Y "%SFML%\bin\sfml-system-2.dll" "%OUTDIR%"

echo [INFO] Build complete.
