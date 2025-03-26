#pragma once
#include "GameEngine.h"
#include "EntityManager.hpp"
#include "Components.hpp"
#include <SFML/Graphics.hpp>

class MovementSystem {
public:
    // Constants for player movement
    static constexpr float MAX_JUMP_HOLD_TIME = 0.04f;        // Maximum duration to hold jump
    static constexpr float JUMP_BOOST_ACCELERATION = 11000.f;  // Additional acceleration during jump
    static constexpr float MAX_UPWARD_VELOCITY = -2000.f;      // Maximum upward velocity (negative value)
    static constexpr float GRAVITY_MULTIPLIER = 2.f;          // Gravity multiplier during fall
    static constexpr float MAX_FALL_SPEED = 9000.f;           // Maximum fall speed
    static constexpr float X_SPEED = 350.f;                    // Player's horizontal speed

    // Constants for camera movement
    void updateCamera();
    // Constants for sword positioning (which follows the player)
    static constexpr float SWORD_OFFSET_X_LEFT = -80.f;
    static constexpr float SWORD_OFFSET_X_RIGHT = 30.f;
    static constexpr float SWORD_OFFSET_Y = 10.f;
    static constexpr float ZOOM_STRENGTH = 1.0f;

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
    float m_currentZoom = 1.0f;
};
