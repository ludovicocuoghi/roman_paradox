#pragma once

#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <string>
#include "Action.hpp"

class GameEngine;

class Scene {
protected:
    GameEngine& m_game;
    std::unordered_map<int, std::string> m_actionMap;
    bool m_paused = false;

public:
    Scene(GameEngine& game);
    virtual ~Scene() = default;

    void registerAction(int key, const std::string& actionName);
    void registerCommonActions();
    // Nella classe Scene (header)
    virtual bool usesImGui() const { return false; };
    
    virtual void update(float deltaTime) = 0;
    virtual void sRender() = 0;
    virtual void sDoAction(const Action& action) = 0;

    void togglePause();
    const std::unordered_map<int, std::string>& getActionMap() const { return m_actionMap; }
};
