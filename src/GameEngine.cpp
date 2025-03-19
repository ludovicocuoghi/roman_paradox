#include "GameEngine.h"
#include "Scene_Menu.h"
#include "Scene_StoryText.h"
#include "Scene_Play.h"
#include <iostream>
#include "imgui.h"
#include "imgui-SFML.h"
#include <filesystem> 

// Constructor
GameEngine::GameEngine(const std::string& path) {
    m_window.create(sf::VideoMode(1920, 1080), "Game Window");
    m_window.setFramerateLimit(100);

    // Load assets globally
    m_assets.loadFromFile(path);

    //  Set the default camera view
    m_cameraView = m_window.getDefaultView();

    //  Define level transitions
    m_levelConnections = {
        {"alien_rome_level_1.txt", "alien_rome_level_2.txt"},
        {"alien_rome_level_2.txt", "ancient_rome_level_1_day.txt"},
        {"ancient_rome_level_1_day.txt", "ancient_rome_level_2_sunset.txt"},
        {"ancient_rome_level_2_sunset.txt", "ancient_rome_level_3_night.txt"},
        {"ancient_rome_level_3_night.txt", "ancient_rome_level_4_emperor_room.txt"},
        {"ancient_rome_level_4_emperor_room.txt", "ancient_rome_level_5_day_v2.txt"},
        {"ancient_rome_level_5_day_v2.txt", "future_rome_level_1.txt"},
        {"future_rome_level_1.txt", "future_rome_level_2.txt"},
        {"future_rome_level_2.txt", "future_rome_level_3.txt"},
        {"future_rome_level_3.txt", "future_rome_level_4_emperor_room.txt"},
        {"future_rome_level_4_emperor_room.txt", "future_rome_level_5_day_v2.txt"},
    };

    // Register the story scenes
    m_scenes["INTRO"] = nullptr;  // Will be properly created when needed
    m_scenes["ENDING"] = nullptr;  // Will be properly created when needed

    // Start in menu
    changeScene("MENU", std::make_shared<Scene_Menu>(*this));
}

//  Retrieve the current scene
std::shared_ptr<Scene> GameEngine::getCurrentScene() {
    return m_currentScene;
}

//  Get the next level path
std::string GameEngine::getNextLevelPath() {
    if (m_currentLevel.empty()) {
        std::cerr << "[ERROR] m_currentLevel is empty when requesting next level!\n";
        return "./bin/levels/ancient_rome_level_1_day.txt"; // Default fallback
    }

    std::string levelFile = m_currentLevel.substr(m_currentLevel.find_last_of("/\\") + 1);

    if (m_levelConnections.find(levelFile) != m_levelConnections.end()) {
        std::string nextLevel = m_levelConnections[levelFile];
        std::cout << "[DEBUG] Next level found: " << nextLevel << std::endl;
        return "./bin/levels/" + nextLevel;
    }

    std::cerr << "[ERROR] No next level mapping for: " << levelFile << std::endl;
    return "./bin/levels/ancient_rome_level_1_day.txt"; // Default if unknown
}

void GameEngine::setCurrentLevel(const std::string& levelPath) {
    if (!levelPath.empty()) {
        m_currentLevel = levelPath;
        std::cout << "[DEBUG] GameEngine current level set to: " << m_currentLevel << std::endl;
    }
}

const std::string& GameEngine::getCurrentLevel() const {
    return m_currentLevel;
}

//  Load a new level and update the current level
void GameEngine::loadLevel(const std::string& levelPath) {
    if (levelPath.empty()) {
        std::cerr << "[ERROR] Attempted to load an empty level path!\n";
        return;
    }

    m_currentLevel = levelPath;  //  Ensure current level is stored
    std::cout << "[DEBUG] Loading Level: " << m_currentLevel << std::endl;

    // Set the world type based on the level name
    std::string levelFile = levelPath.substr(levelPath.find_last_of("/\\") + 1);
    if (levelFile.find("ancient") != std::string::npos) {
        worldType = "Ancient";
    } else if (levelFile.find("future") != std::string::npos) {
        worldType = "Future";
    } else {
        worldType = "Normal";
    }

    changeScene("PLAY", std::static_pointer_cast<Scene>(std::make_shared<Scene_Play>(*this, m_currentLevel)));
}

// Check if the game is running
bool GameEngine::isRunning() const {
    return m_running && m_window.isOpen();
}

// Run the game loop
void GameEngine::run() {
    while (isRunning()) {
        update();
    }
}

// Update the current scene
void GameEngine::update() {
    sUserInput();

    if (m_currentScene) {
        float deltaTime = getDeltaTime();

        while (hasActions()) {
            Action action = popAction();
            m_currentScene->sDoAction(action);
        }

        m_currentScene->update(deltaTime);
        m_currentScene->sRender();

        //  Process pending level change AFTER update cycle
        if (!m_pendingLevelChange.empty()) {
            std::string nextLevel = m_pendingLevelChange;
            m_pendingLevelChange = "";
            loadLevel(nextLevel);
        }
        
        // Check if we need to show the ending when the final level is completed
        if (m_showEndingScreen) {
            m_showEndingScreen = false;
            worldType = "Normal"; // Reset to Normal world type
            changeScene("ENDING", std::make_shared<Scene_StoryText>(*this, StoryType::ENDING));
        }
    }
}

// Handle user input properly
void GameEngine::sUserInput() {
    sf::Event event;
    while (m_window.pollEvent(event)) {
        // Forward events to ImGui only if the current scene uses it
        if (m_currentScene && m_currentScene->usesImGui() && ImGui::GetCurrentContext() != nullptr)
            ImGui::SFML::ProcessEvent(event);
        
        if (event.type == sf::Event::Closed) {
            stop();
        }
    
        m_window.setKeyRepeatEnabled(false);
    
        if (event.type == sf::Event::KeyPressed || event.type == sf::Event::KeyReleased) {
            if (m_currentScene) {
                for (const auto& [key, actionName] : m_currentScene->getActionMap()) {
                    if (event.key.code == key) {
                        std::string actionType = (event.type == sf::Event::KeyPressed) ? "START" : "END";
                        m_actionQueue.push(Action(actionName, actionType));
                        std::cout << "[DEBUG] Action queued: " << actionName << " | Type: " << actionType << "\n";
                    }
                }
            }
        }
    }
}

//  Change the active scene properly
void GameEngine::changeScene(const std::string& sceneName, std::shared_ptr<Scene> scene) {
    if (!scene) {
        // Special handling for predefined scenes
        if (sceneName == "INTRO") {
            scene = std::make_shared<Scene_StoryText>(*this, StoryType::INTRO);
        } else if (sceneName == "ENDING") {
            scene = std::make_shared<Scene_StoryText>(*this, StoryType::ENDING);
        } else {
            std::cerr << "[ERROR] Tried to switch to NULL scene: " << sceneName << std::endl;
            return;
        }
    }

    std::cout << "[DEBUG] Switching Scene to: " << sceneName << std::endl;

    clearActions();
    
    // Store the new scene properly
    m_scenes[sceneName] = scene;
    m_currentScene = scene;

    if (!m_currentScene) {
        std::cerr << "[ERROR] m_currentScene is NULL after switching to: " << sceneName << std::endl;
    }
}

// Add this to GameEngine.h
void GameEngine::restartLevel(const std::string& levelPath) {
    std::cout << "[DEBUG] Restarting level: " << levelPath << std::endl;
    changeScene("PLAY", std::static_pointer_cast<Scene>(std::make_shared<Scene_Play>(*this, m_currentLevel)));
}

// Stops the game engine properly
void GameEngine::stop() {
    m_running = false;
    m_window.close();
}

// Clears all queued actions
void GameEngine::clearActions() {
    while (!m_actionQueue.empty()) {
        m_actionQueue.pop();
    }
    std::cout << "[DEBUG] Cleared all actions.\n";
}

// Check if there are pending actions
bool GameEngine::hasActions() const {
    return !m_actionQueue.empty();
}

// Get the next queued action
Action GameEngine::popAction() {
    if (!m_actionQueue.empty()) {
        Action action = m_actionQueue.front();
        m_actionQueue.pop();
        return action;
    }
    return Action("", "NONE");
}

// Access the game window
sf::RenderWindow& GameEngine::window() {
    return m_window;
}

// Get delta time
float GameEngine::getDeltaTime() {
    return m_clock.restart().asSeconds();
}

// Access assets
Assets& GameEngine::assets() {
    return m_assets;
}

//  NEW: Set the camera view
void GameEngine::setCameraView(const sf::View& view) {
    m_cameraView = view;
    m_window.setView(m_cameraView);
}

//  NEW: Get the camera view
sf::View& GameEngine::getCameraView() {
    return m_cameraView;
}

void GameEngine::scheduleLevelChange(const std::string& levelPath) {
    // Check if we're in the final level and trying to go to another level
    std::string currentLevelFile = m_currentLevel.substr(m_currentLevel.find_last_of("/\\") + 1);
    
    if (currentLevelFile == "future_rome_level_5_day_v2.txt") {
        // Player completed the final level, show ending
        std::cout << "[DEBUG] Final level completed, showing ending screen" << std::endl;
        m_showEndingScreen = true;
        return;
    }
    
    // Normal level transition
    m_pendingLevelChange = levelPath;
}


// New method to trigger the ending screen
void GameEngine::showEnding() {
    m_showEndingScreen = true;
}

// Check if this level is the final one
bool GameEngine::isFinalLevel(const std::string& levelPath) {
    if (levelPath.empty()) return false;
    
    std::string levelFile = levelPath.substr(levelPath.find_last_of("/\\") + 1);
    return levelFile == "future_rome_level_5_day_v2.txt";
}
