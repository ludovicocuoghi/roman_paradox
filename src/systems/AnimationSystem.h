#pragma once

#include "GameEngine.h"
#include "EntityManager.hpp"
#include "Components.hpp"
#include "Animation.hpp"  // Per CAnimation e Animation
#include "Vec2.hpp"
#include <string>

// La classe AnimationSystem si occupa di aggiornare le animazioni degli oggetti
// della scena, come giocatore, tile, nemici e collectable.
class AnimationSystem {
public:
    // Il costruttore riceve i riferimenti necessari:
    // - il GameEngine per accedere alle risorse (asset)
    // - l'EntityManager per iterare sulle entità della scena
    // - un riferimento a una variabile (m_lastDirection) per determinare il verso del giocatore
    AnimationSystem(GameEngine& game, EntityManager& entityManager, float& lastDirection);

    // Metodo principale che aggiorna le animazioni in base al deltaTime
    void update(float deltaTime);

private:
    GameEngine& m_game;
    EntityManager& m_entityManager;
    float& m_lastDirection;
};
