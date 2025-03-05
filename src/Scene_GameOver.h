#ifndef SCENE_GAMEOVER_H
#define SCENE_GAMEOVER_H

#include "Scene.h"
#include <SFML/Graphics.hpp>

class Scene_GameOver : public Scene {
private:
    void renderGameOverText();
    std::string m_levelPath;
    int m_selectedOption; // 0 = Restart Level, 1 = Return to Menu

public:
    Scene_GameOver(GameEngine& game, const std::string& levelPath = "");
    ~Scene_GameOver();

    void update(float deltaTime) override;
    void sRender() override;
    void sDoAction(const Action& action) override;
};

#endif // SCENE_GAMEOVER_H
