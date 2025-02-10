#pragma once

#include "Scene.h"
#include <vector>
#include <string>

class Scene_Menu : public Scene {
private:
    std::vector<std::string> m_menuStrings;
    size_t m_selectedMenuIndex = 0;

    void loadLevelOptions();

public:
    Scene_Menu(GameEngine& game);
    ~Scene_Menu() override = default;

    void update(float deltaTime) override;
    void sRender() override;
    void sDoAction(const Action& action) override;
};
