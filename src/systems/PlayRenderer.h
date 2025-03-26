#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include "EntityManager.hpp"
#include "GameEngine.h"
#include "Vec2.hpp"         // Make sure to include Vec2 definition
#include "Components.hpp"   // For CTransform, CAnimation, CBoundingBox, etc.

class CAnimation; // Forward declaration if necessary

class DialogueSystem;

class PlayRenderer {
public:
    // Constructor: receives necessary references for rendering
    PlayRenderer(GameEngine& game,
                 EntityManager& entityManager,
                 sf::Sprite& backgroundSprite,
                 sf::Texture& backgroundTexture,
                 sf::View& cameraView,
                 int& score);

    // Setters for configuration variables
    void setShowGrid(bool show);
    void setShowBoundingBoxes(bool show);
    void setScore(int score);
    void setTimeOfDay(const std::string& tod);

    // Main rendering function (equivalent to sRender)
    void render();

    // Utility functions
    void drawGrid();
    void drawDebugLine(const Vec2<float>& start, const Vec2<float>& end, sf::Color color);
    void flipSpriteLeft(CAnimation& canim);
    void flipSpriteRight(CAnimation& canim);
    void setDialogueSystem(DialogueSystem* dialogueSystem) { m_dialogueSystem = dialogueSystem; }
    void renderDialogue(DialogueSystem* dialogueSystem);

private:
    GameEngine& m_game;
    EntityManager& m_entityManager;
    sf::Sprite& m_backgroundSprite;
    sf::Texture& m_backgroundTexture;
    sf::View& m_cameraView;

    // Configuration variables for rendering
    bool m_showGrid;
    bool m_showBoundingBoxes;
    int m_score;
    std::string m_timeofday;

    DialogueSystem* m_dialogueSystem = nullptr;
};
