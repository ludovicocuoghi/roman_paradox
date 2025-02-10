#pragma once

#include <string>
#include <memory>
#include "EntityManager.hpp"
#include "GameEngine.h"
#include "Vec2.hpp"

class Entity; // forward declaration

class Spawner {
public:
    Spawner(GameEngine& game, EntityManager& entityManager);

    // Funzioni di spawn per spade e oggetti:
    std::shared_ptr<Entity> spawnSword(std::shared_ptr<Entity> player);
    std::shared_ptr<Entity> spawnEnemySword(std::shared_ptr<Entity> enemy);
    std::shared_ptr<Entity> spawnItem(const Vec2<float>& position, const std::string& tileType);

    // Funzioni per la gestione degli effetti di rottura dei blocchi:
    void updateFragments(float deltaTime);
    void createBlockFragments(const Vec2<float>& position, const std::string & blockType);

private:
    GameEngine& m_game;
    EntityManager& m_entityManager;

    static constexpr float FRAGMENT_SIZE = 16.f;
};
