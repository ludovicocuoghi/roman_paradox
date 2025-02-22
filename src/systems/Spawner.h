#pragma once
#include "GameEngine.h"
#include "EntityManager.hpp"
#include "Entity.hpp"
#include "Components.hpp"
#include "Animation.hpp"
#include "Vec2.hpp"
#include <string>
#include <memory>

class Spawner {
public:
    // Costanti per lo spawn della spada del player
    static constexpr float PLAYER_SWORD_DURATION = 0.2f;

    // Costanti per lo spawn della spada del nemico
    static constexpr float ENEMY_SWORD_OFFSET_X = 50.f;
    static constexpr float ENEMY_SWORD_OFFSET_Y = 10.f;
    static constexpr float ENEMY_SWORD_DURATION = 0.2f;

    static constexpr float EMPEROR_SWORD_OFFSET_X = 80.f;
    static constexpr float EMPEROR_SWORD_OFFSET_Y = 10.f;
    static constexpr float EMPEROR_ROTATING_SWORD_DURATION = 7.f;

    // Costanti per lo spawn degli item da box
    static constexpr int BOX_ITEM_DIST_MIN = 0;
    static constexpr int BOX_ITEM_DIST_MAX = 9;
    static constexpr int BOX_ITEM_THRESHOLD_COINBRONZE = 5;
    static constexpr int BOX_ITEM_THRESHOLD_GRAPESMALL = 9;

    // Costanti per lo spawn degli item da TreasureBox
    static constexpr int TREASURE_BOX_DIST_MIN = 0;
    static constexpr int TREASURE_BOX_DIST_MAX = 1;
    static constexpr float TREASURE_BOX_TILE_SIZE = 96.f;

    // Costante per lo scale dei collectible
    static constexpr float COLLECTABLE_SCALE_FACTOR = 0.3f;

    // Costante per il valore massimo dell'alpha (opacità)
    static constexpr float ALPHA_MAX = 255.f;

    // Costanti per la creazione dei frammenti
    static constexpr float FRAGMENT_SPREAD_SPEED = 400.f;
    static constexpr int FRAGMENT_ANGLE_MIN = 0;
    static constexpr int FRAGMENT_ANGLE_MAX = 359;
    static constexpr int FRAGMENT_ROTATION_SPEED_MIN = 0;
    static constexpr int FRAGMENT_ROTATION_SPEED_MAX = 199;
    static constexpr float FRAGMENT_SIZE = 50.f;
    static constexpr float FRAGMENT_DURATION = 0.6f;


    Spawner(GameEngine& game, EntityManager& entityManager);

    // Funzioni di spawn
    std::shared_ptr<Entity> spawnSword(std::shared_ptr<Entity> player);
    std::shared_ptr<Entity> spawnEnemySword(std::shared_ptr<Entity> enemy);
    std::shared_ptr<Entity> spawnItem(const Vec2<float>& position, const std::string& tileType);
    std::shared_ptr<Entity> spawnEmperorSwordOffset(std::shared_ptr<Entity> enemy);
    void spawnEmperorSwordsRadial(std::shared_ptr<Entity> enemy, int swordCount, float radius, float swordSpeed);

    void updateFragments(float deltaTime);
    void createBlockFragments(const Vec2<float>& position, const std::string & blockType);

private:
    GameEngine& m_game;
    EntityManager& m_entityManager;
};
