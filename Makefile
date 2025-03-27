# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -std=c++20 -Wall -Wextra -I/opt/homebrew/opt/sfml@2/include \
           -I./src/imgui -I./src/imgui-sfml -I./src/

# SFML library flags
LDFLAGS = -L/opt/homebrew/opt/sfml@2/lib -lsfml-graphics -lsfml-window -lsfml-system -framework OpenGL -framework CoreFoundation

# Target executable
TARGET = bin/sfml_app

# Source files
SRC = main.cpp src/GameEngine.cpp src/Scene.cpp src/Scene_Play.cpp src/Scene_LevelEditor.cpp src/Scene_Menu.cpp src/systems/LoadLevel.cpp src/systems/PlayRenderer.cpp src/systems/CollisionSystem.cpp src/Scene_GameOver.cpp \
      src/Assets.cpp src/systems/MovementSystem.cpp src/systems/AnimationSystem.cpp src/systems/EnemyAISystem.cpp src/systems/Spawner.cpp src/systems/DialogueSystem.cpp src/Scene_StoryText.cpp src/ResourcePath.cpp\
      $(wildcard src/imgui/*.cpp) $(wildcard src/imgui-sfml/*.cpp)

# Object files directory
OBJ_DIR = build

# Object files
OBJ = $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(SRC))

# Default target
all: $(TARGET)

# Link object files into the final executable
$(TARGET): $(OBJ)
	@mkdir -p $(dir $@)
	$(CXX) $(OBJ) -o $(TARGET) $(LDFLAGS)

# Clean up old resource folders and avoid macOS folder auto-renaming
	@rm -rf bin/images
	@rm -rf bin/images\ 2 bin/images\ 3 bin/images\ 4 bin/images\ 5
	@rm -rf bin/fonts
	@rm -rf bin/assets
	@rm -rf bin/levels

# Recreate only the clean asset folders
	@mkdir -p bin/images
	@mkdir -p bin/fonts
	@mkdir -p bin/assets
	@mkdir -p bin/levels

# Copy only whitelisted and valid folders
	cp -r src/images/Player      bin/images/
	cp -r src/images/Collectables bin/images/
	cp -r src/images/Tile        bin/images/
	cp -r src/images/Background  bin/images/
	cp -r src/images/Bullet      bin/images/
	cp -r src/images/Dec         bin/images/
	cp -r src/images/Enemy       bin/images/
	cp -r src/images/Portraits   bin/images/
	cp -r src/images/sword       bin/images/

	cp -r src/fonts/*    bin/fonts/
	cp -r src/assets/*   bin/assets/
	cp -r src/levels/*   bin/levels/

# Compile rule
$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean target
clean:
	rm -rf $(OBJ_DIR) $(TARGET) bin

# Phony targets
.PHONY: all clean
