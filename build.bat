@echo off
set PATH=C:\mingw64\bin;%PATH%
set SFML=C:\SFML
set OUTDIR=windows_build

if not exist %OUTDIR% mkdir %OUTDIR%

echo [INFO] Starting compilation...

:: Compile
g++ -std=c++20 -Wall -fdiagnostics-color=always -static-libgcc -static-libstdc++ ^
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
-o %OUTDIR%\rome_interstellar_paradox.exe

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

:: Copy all MinGW runtime DLLs
copy /Y "C:\mingw64\bin\libgcc_*.dll" "%OUTDIR%"
copy /Y "C:\mingw64\bin\libstdc++*.dll" "%OUTDIR%"
copy /Y "C:\mingw64\bin\libwinpthread*.dll" "%OUTDIR%"
copy /Y "C:\mingw64\bin\libssp*.dll" "%OUTDIR%"

:: Copy assets with /Y to automatically respond "Yes to All"
xcopy /E /I /Y src\assets %OUTDIR%\assets
xcopy /E /I /Y src\fonts %OUTDIR%\fonts
xcopy /E /I /Y src\images %OUTDIR%\images
xcopy /E /I /Y src\levels %OUTDIR%\levels

echo [INFO] Build complete.