#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include "EntityManager.hpp"
#include "GameEngine.h"
#include "Vec2.hpp"         // Assicurati di includere la definizione di Vec2
#include "Components.hpp"   // Per CTransform, CAnimation, CBoundingBox, ecc.

class CAnimation; // Forward declaration se necessario

class PlayRenderer {
public:
    // Costruttore: riceve i riferimenti necessari per il rendering
    PlayRenderer(GameEngine& game,
                 EntityManager& entityManager,
                 sf::Sprite& backgroundSprite,
                 sf::Texture& backgroundTexture,
                 sf::View& cameraView);

    // Setter per le variabili di configurazione
    void setShowGrid(bool show);
    void setShowBoundingBoxes(bool show);
    void setScore(int score);
    void setTimeOfDay(const std::string& tod);

    // Funzione principale di rendering (equivalente a sRender)
    void render();

    // Funzioni di utilità
    void drawGrid();
    void drawDebugLine(const Vec2<float>& start, const Vec2<float>& end, sf::Color color);
    void flipSpriteLeft(CAnimation& canim);
    void flipSpriteRight(CAnimation& canim);

private:
    GameEngine& m_game;
    EntityManager& m_entityManager;
    sf::Sprite& m_backgroundSprite;
    sf::Texture& m_backgroundTexture;
    sf::View& m_cameraView;

    // Variabili di configurazione per il rendering
    bool m_showGrid;
    bool m_showBoundingBoxes;
    int m_score;
    std::string m_timeofday;
};
