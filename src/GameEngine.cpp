#include "GameEngine.h"
#include "Scene_Menu.h"
#include <iostream>
#include "imgui.h"
#include "imgui-SFML.h"


// Constructor
// Constructor
GameEngine::GameEngine(const std::string& path) {
    m_window.create(sf::VideoMode(1280, 980), "Game Window");
    m_window.setFramerateLimit(100);

    // Carica gli asset globalmente (questo deve essere fatto una volta)
    m_assets.loadFromFile(path);

    // Cambia scena (ad esempio, menu iniziale)
    changeScene("MENU", std::make_shared<Scene_Menu>(*this));
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

        // ✅ Process all queued actions before updating the scene
        while (hasActions()) {
            Action action = popAction();
            m_currentScene->sDoAction(action);
        }

        m_currentScene->update(deltaTime);
        m_currentScene->sRender();
    }
}

// Handle user input properly
void GameEngine::sUserInput() {
    sf::Event event;
    while (m_window.pollEvent(event)) {
        // Se la scena corrente usa ImGui, inoltra l'evento a ImGui
        if (m_currentScene && m_currentScene->usesImGui())
            ImGui::SFML::ProcessEvent(event);
        
        if (event.type == sf::Event::Closed) {
            stop();
        }
        m_window.setKeyRepeatEnabled(false);
        if (event.type == sf::Event::KeyPressed) {
            if (m_currentScene) {
                for (const auto& [key, actionName] : m_currentScene->getActionMap()) {
                    if (event.key.code == key) {
                        m_actionQueue.push(Action(actionName, "START"));
                        std::cout << "[DEBUG] Action queued: " << actionName << " | Type: START\n";
                    }
                }
            }
        }
        if (event.type == sf::Event::KeyReleased) {
            if (m_currentScene) {
                for (const auto& [key, actionName] : m_currentScene->getActionMap()) {
                    if (event.key.code == key) {
                        m_actionQueue.push(Action(actionName, "END"));
                        std::cout << "[DEBUG] Action queued: " << actionName << " | Type: END\n";
                    }
                }
            }
        }
    }
}


// Change the active scene
void GameEngine::changeScene(const std::string& sceneName, std::shared_ptr<Scene> scene) {
    std::cout << "[DEBUG] Switching Scene to: " << sceneName << std::endl;
    m_actionQueue = {};  // ✅ Clear actions from previous scene
    m_scenes[sceneName] = scene;
    m_currentScene = scene;
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
