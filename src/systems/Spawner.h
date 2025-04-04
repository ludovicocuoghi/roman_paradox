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
    // Constants for player sword spawn
    static constexpr float PLAYER_SWORD_DURATION = 0.2f;

    // Constants for enemy sword spawn
    static constexpr float ENEMY_SWORD_OFFSET_X = 60.f;
    static constexpr float ENEMY_SWORD_OFFSET_Y = 15.f;
    static constexpr float ENEMY_SWORD_DURATION = 0.3f;

    static constexpr float EMPEROR_SWORD_OFFSET_X = 80.f;
    static constexpr float EMPEROR_SWORD_OFFSET_Y = 10.f;
    static constexpr float EMPEROR_ROTATING_SWORD_DURATION = 7.f;

    // Constants for enemy projectile spawning (bullets)
    static constexpr float ENEMY_BULLET_SPEED = 900.f; // Bullet speed
    static constexpr float ENEMY_BULLET_DURATION = 3.0f; // Duration before disappearing
    static constexpr float ENEMY_BULLET_OFFSET_X = 30.0f; // Initial position relative to enemy
    static constexpr float ENEMY_BULLET_OFFSET_Y = 5.0f; // Slightly above the enemy

    // Constants for item spawning from boxes
    static constexpr int BOX_ITEM_DIST_MIN = 0;
    static constexpr int BOX_ITEM_DIST_MAX = 9;
    static constexpr int BOX_ITEM_THRESHOLD_COINBRONZE = 5;
    static constexpr int BOX_ITEM_THRESHOLD_GRAPESMALL = 9;

    // Constants for item spawning from TreasureBox
    static constexpr int TREASURE_BOX_DIST_MIN = 0;
    static constexpr int TREASURE_BOX_DIST_MAX = 1;
    static constexpr float TREASURE_BOX_TILE_SIZE = 96.f;

    // Constant for collectible scale
    static constexpr float COLLECTABLE_SCALE_FACTOR = 0.3f;

    // Constant for maximum alpha value (opacity)
    static constexpr float ALPHA_MAX = 255.f;

    // Constants for fragment creation
    static constexpr float FRAGMENT_SPREAD_SPEED = 400.f;
    static constexpr int FRAGMENT_ANGLE_MIN = 0;
    static constexpr int FRAGMENT_ANGLE_MAX = 359;
    static constexpr int FRAGMENT_ROTATION_SPEED_MIN = 0;
    static constexpr int FRAGMENT_ROTATION_SPEED_MAX = 199;
    static constexpr float FRAGMENT_SIZE = 50.f;
    static constexpr float FRAGMENT_DURATION = 0.6f;

    static constexpr float PLAYER_BULLET_SPEED      = 900.f;  // Adjust as needed
    static constexpr float PLAYER_BULLET_DURATION   = 3.0f;   // Bullet lifespan
    static constexpr float PLAYER_BULLET_OFFSET_X   = 30.f;   // X offset from player
    static constexpr float PLAYER_BULLET_OFFSET_Y   = 5.f;   // Y offset from player

    // Bullet types and damage values
    static constexpr float BULLET_BLUE_DAMAGE = 1.0f;   // Phase 1 - Basic bullet
    static constexpr float BULLET_GOLD_DAMAGE = 2.0f;   // Phase 2 - Medium bullet
    static constexpr float BULLET_RED_DAMAGE = 3.0f;    // Phase 2 - Strong bullet
    static constexpr float BULLET_BLACK_DAMAGE = 5.0f;  // Phase 3 - Most powerful bullet

    static constexpr float EMPEROR_RADIAL_BULLETS_SPEED = 800;
    static constexpr float EMPEROR_RADIAL_BULLETS_RADIUS = 100;
    static constexpr float EMPEROR_RADIAL_BULLETS_COUNT = 100;

    // Bullet speeds for different colors
    static constexpr float BULLET_BLUE_SPEED = 300.0f;
    static constexpr float BULLET_GOLD_SPEED = 350.0f;
    static constexpr float BULLET_RED_SPEED = 400.0f;
    static constexpr float BULLET_BLACK_SPEED = 450.0f;

    // Bullet visuals (if needed)
    static constexpr float BULLET_BLUE_SCALE = 1.0f;
    static constexpr float BULLET_GOLD_SCALE = 1.1f;
    static constexpr float BULLET_RED_SCALE = 1.2f;
    static constexpr float BULLET_BLACK_SCALE = 1.3f;
    static constexpr float BULLET_DURATION= 10.f;

    Spawner(GameEngine& game, EntityManager& entityManager);

    // Spawn functions
    std::shared_ptr<Entity> spawnSword(std::shared_ptr<Entity> player);
    std::shared_ptr<Entity> spawnEnemySword(std::shared_ptr<Entity> enemy);
    std::shared_ptr<Entity> spawnEnemyBullet(std::shared_ptr<Entity> enemy); // Added
    
    std::shared_ptr<Entity> spawnItem(const Vec2<float>& position, const std::string& tileType);
    std::shared_ptr<Entity> spawnEmperorSwordOffset(std::shared_ptr<Entity> enemy);
    std::shared_ptr<Entity> spawnPlayerBullet(std::shared_ptr<Entity> player);


    void spawnEmperorSwordsRadial(std::shared_ptr<Entity> enemy, int swordCount, float radius, float swordSpeed);
    void spawnEnemyGrave(const Vec2<float>& position, bool isEmperor);
    void updateGraves(float deltaTime);
    void updateFragments(float deltaTime);
    void createBlockFragments(const Vec2<float>& position, const std::string & blockType);
    
    void spawnEmperorSwordsStatic(std::shared_ptr<Entity> emperor,int swordCount,float radius,float speed);
    void spawnEmperorSwordArmorRadial(std::shared_ptr<Entity> emperor, int swordCount, float radius, float swordSpeed, float baseStopTime, float stopTimeIncrement);
    void spawnEmperorBulletsRadial(std::shared_ptr<Entity> enemy, int bulletCount, 
        float radius, float bulletSpeed, 
        const std::string& bulletType);

    // Original overload for backward compatibility
    void spawnEmperorBulletsRadial(std::shared_ptr<Entity> enemy, int bulletCount, 
        float radius, float bulletSpeed);

    void spawnEmperorBlackHoles(std::shared_ptr<Entity> enemy, int blackHoleCount, 
        float radius, float blackHoleSpeed);

private:
    GameEngine& m_game;
    EntityManager& m_entityManager;
};
