#pragma once

#include "EntityManager.hpp"
#include "GameEngine.h"
#include "Spawner.h"
#include <SFML/Graphics.hpp>

class CollisionSystem {
public:
    CollisionSystem(EntityManager& entityManager, GameEngine& game, Spawner* spawner, int& score, const std::string& levelPath);

    // Game mechanics constants
    static constexpr float PLAYER_RUN_VELOCITY_THRESHOLD = 1.f;
    static constexpr float COLLISION_SEPARATION_FACTOR = 0.5f;

    // Combat constants
    static constexpr int   PLAYER_SWORD_DAMAGE = 10;
    static constexpr float PLAYER_SWORD_INVULNERABILITY_TIME = 0.2f;
    static constexpr float ENEMY_SWORD_KNOCKBACK_STRENGTH = 1000.0f;
    static constexpr float PLAYER_HIT_INVULNERABILITY_TIME = 0.01f;
    static constexpr float PLAYER_KNOCKBACK_TIMER = 0.5f;

    // Knockback mechanics
    static constexpr int   PLAYER_SWORD_KNOCKBACK_ROLL_MIN = 1;
    static constexpr int   PLAYER_SWORD_KNOCKBACK_ROLL_MAX = 5;
    static constexpr int   PLAYER_SWORD_KNOCKBACK_ROLL_TRIGGER = 1;
    static constexpr float PLAYER_SWORD_KNOCKBACK_STRENGTH = 300.0f;
    static constexpr float PLAYER_SWORD_KNOCKBACK_Y_DIRECTION = -50.f;
    static constexpr float ENEMY_KNOCKBACK_AI_TIMER = 0.4f;
    static constexpr float ENEMY_KNOCKBACK_STATE_TIMER = 1.0f;

    // Collectible values
    static constexpr int COLLECTIBLE_SMALL_GRAPE_POINTS = 50;
    static constexpr int COLLECTIBLE_BIG_GRAPE_HEAL = 100;
    static constexpr int COLLECTIBLE_GOLD_COIN_POINTS = 101;
    static constexpr int COLLECTIBLE_SILVER_COIN_POINTS = 99;
    static constexpr int COLLECTIBLE_BRONZE_COIN_POINTS = 60;
    static constexpr float BIGCHICKEN_POINTS = 7.f;
    static constexpr float SMALLCHICKEN_POINTS = 3.f;

    // Collision handling methods
    void updateCollisions();
    void handlePlayerTileCollisions();
    void handleEnemyTileCollisions();
    void handlePlayerEnemyCollisions();
    void handleSwordCollisions();
    void handlePlayerCollectibleCollisions();
    void handleEnemyEnemyCollisions();
    void handleBulletPlayerCollisions();
    void handlePlayerBulletCollisions();
    void handleBlackHoleTileCollisions();
    void handleMassiveBlackHoleCollisions();

private:
    EntityManager& m_entityManager;
    GameEngine& m_game;
    Spawner* m_spawner;
    int& m_score;
    std::string m_levelPath;
};
