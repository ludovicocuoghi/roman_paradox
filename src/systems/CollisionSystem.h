#pragma once

#include "EntityManager.hpp"
#include "GameEngine.h"
#include "Spawner.h"
#include <SFML/Graphics.hpp>
#include <iostream>

class CollisionSystem {
public:
    // Constructor declaration only (no inline definition here)
    CollisionSystem(EntityManager& entityManager, GameEngine& game, Spawner* spawner);

    void updateCollisions();

private:
    EntityManager& m_entityManager;
    GameEngine& m_game;
    Spawner* m_spawner;

    void handlePlayerTileCollisions();
    void handleEnemyTileCollisions();
    void handlePlayerEnemyCollisions();
    void handleSwordCollisions();
    void handlePlayerCollectibleCollisions();
};
