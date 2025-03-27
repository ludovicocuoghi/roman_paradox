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

# Object files (convert source file names to object files in the build directory)
OBJ = $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(SRC))

# Default target
all: $(TARGET)

# Link object files into the final executable
$(TARGET): $(OBJ)
	@mkdir -p $(dir $@)
	$(CXX) $(OBJ) -o $(TARGET) $(LDFLAGS)

# Ensure asset directories exist and copy all assets
	@mkdir -p bin
	# Copy and preserve full directory structure for each resource folder
	rsync -a src/images/ bin/images/
	rsync -a src/fonts/ bin/fonts/
	rsync -a src/assets/ bin/assets/
	rsync -a src/levels/ bin/levels/

# Rule to compile source files into object files
$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -rf $(OBJ_DIR) $(TARGET) bin

# Phony targets
.PHONY: all clean
