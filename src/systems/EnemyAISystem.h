#pragma once

#include "EntityManager.hpp"
#include "GameEngine.h"
#include <SFML/Graphics.hpp>
#include "Components.hpp"
#include "Vec2.hpp"
#include <cmath>
#include <algorithm>

// Il modulo EnemyAISystem gestisce l'aggiornamento dell'IA dei nemici.
class EnemyAISystem {
public:
    // Costruttore: riceve i riferimenti necessari.
    EnemyAISystem(GameEngine& game, EntityManager& entityManager);

    // Aggiorna l'IA dei nemici in base a deltaTime.
    void update(float deltaTime);

private:
    GameEngine& m_game;
    EntityManager& m_entityManager;

    // Costante di configurazione per la gravità
    static constexpr float MAX_FALL_SPEED = 1000.f;
};
