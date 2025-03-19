#pragma once

#include "Scene.h"
#include <SFML/Graphics.hpp>
#include <string>

class Scene_GameOver : public Scene {
private:
    std::string m_levelPath;
    int m_selectedOption;  // 0 = Restart, 1 = Level Selection

    void renderGameOverText();

public:
    Scene_GameOver(GameEngine& game, const std::string& levelPath = "");
    ~Scene_GameOver();

    void update(float deltaTime) override;
    void sRender() override;
    void sDoAction(const Action& action) override;
    void goToLevelSelection();
};
