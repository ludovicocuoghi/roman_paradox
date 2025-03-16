#pragma once

#include "GameEngine.h"
#include "EntityManager.hpp"
#include "Components.hpp"
#include "Animation.hpp"
#include "Vec2.hpp"
#include <string>

// The AnimationSystem class handles updating animations for all scene objects,
// including player, tiles, enemies, and collectables
class AnimationSystem {
public:
    // Constructor takes required references:
    // - GameEngine for accessing assets
    // - EntityManager for iterating scene entities
    // - lastDirection reference to determine player orientation
    AnimationSystem(GameEngine& game, EntityManager& entityManager, float& lastDirection);

    // Main method that updates animations based on deltaTime
    void update(float deltaTime);

private:
    GameEngine& m_game;
    EntityManager& m_entityManager;
    float& m_lastDirection;
};
