#pragma once

#include "EntityManager.hpp"
#include "Spawner.h"
#include "GameEngine.h" // Make sure to include the GameEngine header

class EnemyAISystem {
public:
    // Updated constructor declaration with GameEngine parameter
    EnemyAISystem(EntityManager& entityManager, Spawner& spawner, GameEngine& game);
    
    void update(float deltaTime);

private:
    EntityManager& m_entityManager;
    Spawner* m_spawner;
    GameEngine& m_game; // Added member to access assets
};
