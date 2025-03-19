#pragma once

#include "Scene.h"
#include <string>
#include <vector>

enum class StoryType {
    INTRO,
    ENDING
};

class Scene_StoryText : public Scene {
public:
    Scene_StoryText(GameEngine& game, StoryType type);
    ~Scene_StoryText() = default;

    void update(float deltaTime) override;
    void sRender() override;
    void sDoAction(const Action& action) override;

private:
    void loadStoryText(StoryType type);
    void renderStoryText();

    StoryType m_type;
    std::vector<std::string> m_storyLines;
    std::string m_continueText;
    float m_textFadeTime;
    float m_textFadeTimer;
    bool m_fadingIn;
};
