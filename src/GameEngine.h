#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#include <map>
#include <queue>
#include <string>
#include "Assets.hpp"
#include "Action.hpp"
#include "Scene.h"

class GameEngine {
public:
    GameEngine(const std::string& path);

    // Scene management
    void changeScene(const std::string& sceneName, std::shared_ptr<Scene> scene);
    std::shared_ptr<Scene> getCurrentScene();
    

    // Level management
    void loadLevel(const std::string& levelPath);
    std::string getNextLevelPath();
    void setCurrentLevel(const std::string& levelPath);
    const std::string& getCurrentLevel() const;
    void restartLevel(const std::string& levelPath);
    void scheduleLevelChange(const std::string& levelPath);
    bool isFinalLevel(const std::string& levelPath);
    void showEnding();

    // Game loop
    bool isRunning() const;
    void run();
    void update();
    void stop();

    // Action management
    void clearActions();
    bool hasActions() const;
    Action popAction();

    // Access
    sf::RenderWindow& window();
    float getDeltaTime();
    Assets& assets();
    
    // Camera management
    void setCameraView(const sf::View& view);
    sf::View& getCameraView();
    std::string worldType;
    int universeNumber;
    int alternateUniverseNumber;
    int alternateUniverseNumber2;
    void setLanguage(const std::string& language);
    const std::string& getLanguage() const { return m_language; }
    float getScaleX() const;
    float getScaleY() const;
    sf::Vector2f getReferenceResolution() const;

private:
    void sUserInput();

    sf::RenderWindow m_window;
    sf::Clock m_clock;
    sf::View m_cameraView;
    Assets m_assets;
    
    bool m_running = true;
    bool m_showEndingScreen = false;
    
    std::shared_ptr<Scene> m_currentScene;
    std::map<std::string, std::shared_ptr<Scene>> m_scenes;
    std::queue<Action> m_actionQueue;
    
    std::string m_currentLevel;
    std::string m_pendingLevelChange;
    std::map<std::string, std::string> m_levelConnections;
    std::string m_language = "English";
    sf::Vector2f m_referenceResolution;
};
