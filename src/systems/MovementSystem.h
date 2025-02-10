#pragma once

#include <SFML/Graphics.hpp>
#include "EntityManager.hpp"
#include "GameEngine.h"
#include "Components.hpp"
#include "Vec2.hpp"

// Il MovementSystem si occupa di aggiornare il movimento del giocatore e della telecamera.
class MovementSystem {
public:
    MovementSystem(GameEngine& game,
                   EntityManager& entityManager,
                   sf::View& cameraView,
                   float& lastDirection);

    void update(float deltaTime);

private:
    GameEngine& m_game;
    EntityManager& m_entityManager;
    sf::View& m_cameraView;
    float& m_lastDirection;

    // Costanti di configurazione definite internamente:
    static constexpr float xSpeed = 350.f;
    static constexpr float ySpeed = 350.f;
    static constexpr float JumpAcceleration = 2000.f;
    static constexpr float MaxJumpDuration = 0.4f;
    static constexpr float MaxUpwardVelocity = -800.f;
    static constexpr float CAMERA_Y_OFFSET = -80.f;
    static constexpr float CAMERA_SMOOTHING_FACTOR = 0.1f;
    static constexpr float MAX_FALL_SPEED = 1000.f;
};
