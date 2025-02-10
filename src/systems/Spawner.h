#pragma once

#include <string>
#include <memory>
#include "EntityManager.hpp"
#include "GameEngine.h"
#include "Vec2.hpp"

// Forward declaration dell'entità (assicurati che Entity sia definito in un header appropriato)
class Entity;

class Spawner {
public:
    // Costruttore: riceve i riferimenti necessari.
    Spawner(GameEngine& game, EntityManager& entityManager);

    // Genera la spada del giocatore.
    std::shared_ptr<Entity> spawnSword(std::shared_ptr<Entity> player);

    // Genera la spada del nemico.
    std::shared_ptr<Entity> spawnEnemySword(std::shared_ptr<Entity> enemy);

    // Genera un oggetto raccoltabile in base al tipo di tile.
    std::shared_ptr<Entity> spawnItem(const Vec2<float>& position, const std::string& tileType);

private:
    GameEngine& m_game;
    EntityManager& m_entityManager;
};
