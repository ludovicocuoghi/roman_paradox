#pragma once
#include "Scene.h"

class Scene_GameOver : public Scene {
private:
    void renderGameOverText();
public:
    Scene_GameOver(GameEngine& game);
    ~Scene_GameOver() override;
    void sRender() override;
    void update(float deltaTime) override;
    void sDoAction(const Action& action) override;
};
