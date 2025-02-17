#pragma once

#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <memory>
#include <string>
#include <queue>
#include "Assets.hpp"  
#include "Action.hpp"  

class Scene;  // Forward declaration (DO NOT include Scene_Play.h here!)

class GameEngine {
private:
    sf::RenderWindow m_window;
    std::unordered_map<std::string, std::shared_ptr<Scene>> m_scenes;
    std::shared_ptr<Scene> m_currentScene;
    bool m_running = true;
    Assets m_assets;
    sf::Clock m_clock;
    std::queue<Action> m_actionQueue;
    sf::View m_cameraView;
    int m_score = 0;
    std::unordered_map<std::string, std::string> m_levelConnections;
    std::string m_currentLevel;
    std::string m_pendingLevelChange = "";

public:
    GameEngine(const std::string& assetsPath);
    void init(const std::string& assetsPath);
    void run();
    void update();
    void sUserInput();
    
    void changeScene(const std::string& sceneName, std::shared_ptr<Scene> scene);
    void stop();

    sf::RenderWindow& window();
    bool isRunning() const;
    float getDeltaTime();
    Assets& assets();

    void clearActions();
    bool hasActions() const;
    Action popAction();

    std::shared_ptr<Scene> getCurrentScene();
    
    void setCameraView(const sf::View& view);
    sf::View& getCameraView();
    
    void loadLevel(const std::string& levelPath);
    std::string getNextLevelPath();
    void scheduleLevelChange(const std::string& levelPath);
};
