#pragma once

#include "EntityManager.hpp"
#include "GameEngine.h"

class SwordCollisionSystem {
public:
    // Costruttore: riceve i riferimenti necessari.
    SwordCollisionSystem(GameEngine& game, EntityManager& entityManager);

    // Aggiorna le collisioni tra spade e nemici per un dato deltaTime.
    void updateSwordCollisions(float deltaTime);

private:
    GameEngine& m_game;
    EntityManager& m_entityManager;
};
