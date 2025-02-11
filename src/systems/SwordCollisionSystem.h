#pragma once

#include "GameEngine.h"
#include "EntityManager.hpp"
#include "Components.hpp"
#include <SFML/Graphics.hpp>

class SwordCollisionSystem {
public:
    SwordCollisionSystem(EntityManager& entityManager);

    void updateSwordCollisions();

private:
    EntityManager& m_entityManager;
};
