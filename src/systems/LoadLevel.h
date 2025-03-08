#pragma once
#include "GameEngine.h"
#include "EntityManager.hpp"
#include <string>
#include <unordered_map>

class LoadLevel {
public:
    // =======================================
    // Grid and positioning constants
    // =======================================
    static constexpr float GRID_SIZE = 96.f;
    static constexpr float HALF_GRID = GRID_SIZE * 0.5f;

    // Offset multipliers
    static constexpr float PIPE_REALX_OFFSET_MULTIPLIER = 1.f;
    static constexpr float PIPE_REALY_OFFSET_MULTIPLIER = 0.5f;
    static constexpr float PIPETALL_REALY_OFFSET_MULTIPLIER = 1.5f;
    static constexpr float PIPEBROKEN_REALY_OFFSET_MULTIPLIER = 1.f;
    static constexpr float LEVELDOOR_REALY_OFFSET_MULTIPLIER = 2.f;
    static constexpr float BLACKHOLE_OFFSET_MULTIPLIER = 2.f;
    static constexpr float EMPEROR_REALY_OFFSET_MULTIPLIER = 1.f;

    // Physics constants
    static constexpr float GRAVITY_VAL = 1000.f;

    // =======================================
    // Player constants
    // =======================================
    static constexpr float PLAYER_BB_SIZE = 80.f;
    static constexpr float PLAYER_HEALTH = 150.f;
    
    // Player attack parameters
    static constexpr float PLAYER_ATTACK_COOLDOWN = 0.5f;
    static constexpr float PLAYER_SWORD_COOLDOWN = 0.5f;
    static constexpr int   PLAYER_MAX_CONSECUTIVE_SWORD_ATTACKS = 3;
    
    // Player shield
    static constexpr float PLAYER_SHIELD_STAMINA = 3.f;
    
    // Player bullet parameters
    static constexpr int   PLAYER_BULLET_COUNT = 50;
    static constexpr int   PLAYER_BULLET_DAMAGE = 2;
    static constexpr float PLAYER_BULLET_COOLDOWN = 0.1f;
    static constexpr int   PLAYER_BULLET_BURST_COUNT = 10;
    static constexpr float PLAYER_BULLET_BURSTDURATION = 3.f;
    static constexpr float PLAYER_BULLET_BURSTINTERVAL = 0.2f;
    
    // Player super bullet parameters
    static constexpr int   PLAYER_SUPER_BULLET_COUNT = 50;
    static constexpr float PLAYER_SUPER_BULLET_DAMAGE = 1.f;
    static constexpr float PLAYER_SUPER_BULLET_COOLDOWN = 15.f;

    // =======================================
    // Enemy base stats
    // =======================================
    
    // Normal enemy
    static constexpr float ENEMY_NORMAL_SPEED_MULTIPLIER = 1.5f;
    static constexpr int   ENEMY_NORMAL_HEALTH = 40;
    static constexpr int   ENEMY_NORMAL_DAMAGE = 10;
    
    // Fast enemy
    static constexpr float ENEMY_FAST_SPEED_MULTIPLIER = 2.f;
    static constexpr int   ENEMY_FAST_HEALTH = 30;
    static constexpr int   ENEMY_FAST_DAMAGE = 8;
    
    // Strong enemy
    static constexpr float ENEMY_STRONG_SPEED_MULTIPLIER = 1.3f;
    static constexpr int   ENEMY_STRONG_HEALTH = 50;
    static constexpr int   ENEMY_STRONG_DAMAGE = 15;
    
    // Elite enemy
    static constexpr float ENEMY_ELITE_SPEED_MULTIPLIER = 1.8f;
    static constexpr int   ENEMY_ELITE_HEALTH = 50;
    static constexpr int   ENEMY_ELITE_DAMAGE = 20;
    
    // Super enemy
    static constexpr float ENEMY_SUPER_SPEED_MULTIPLIER = 2.0f;
    static constexpr int   ENEMY_SUPER_HEALTH = 50;
    static constexpr int   ENEMY_SUPER_DAMAGE = 20;
    
    // Emperor enemy
    static constexpr float ENEMY_EMPEROR_SPEED_MULTIPLIER = 0.6f;
    static constexpr int   ENEMY_EMPEROR_HEALTH = 100;
    static constexpr int   ENEMY_EMPEROR_DAMAGE = 10;
    static constexpr float EMPEROR_BB_WIDTH = 160.f;
    static constexpr float EMPEROR_BB_HEIGHT = 250.f;
    static constexpr float EMPEROR_RADIAL_SWORD_DAMAGE = 0.5f;

    // =======================================
    // Enemy bullet damage
    // =======================================
    static constexpr int   BULLET_DAMAGE_PLAYER = 5;
    static constexpr int   BULLET_DAMAGE_ENEMY_NORMAL = 4;
    static constexpr int   BULLET_DAMAGE_ENEMY_FAST = 3;
    static constexpr int   BULLET_DAMAGE_ENEMY_STRONG = 6;
    static constexpr int   BULLET_DAMAGE_ENEMY_ELITE = 5;
    static constexpr int   BULLET_DAMAGE_ENEMY_SUPER = 8;
    static constexpr int   BULLET_DAMAGE_ENEMY_EMPEROR = 7;

    // =======================================
    // Enemy attack parameters
    // =======================================
    
    // Normal enemy attack params
    static constexpr int   ENEMY_NORMAL_MAX_CONSECUTIVE_SWORD_ATTACKS = 5;
    static constexpr int   ENEMY_NORMAL_BULLET_BURST_COUNT = 5;
    static constexpr int   ENEMY_NORMAL_SUPER_BULLET_COUNT = 8;
    static constexpr float ENEMY_NORMAL_SUPER_BULLET_DAMAGE = 3.f;
    
    // Fast enemy attack params
    static constexpr int   ENEMY_FAST_MAX_CONSECUTIVE_SWORD_ATTACKS = 4;
    static constexpr int   ENEMY_FAST_BULLET_BURST_COUNT = 6;
    static constexpr int   ENEMY_FAST_SUPER_BULLET_COUNT = 4;
    static constexpr float ENEMY_FAST_SUPER_BULLET_DAMAGE = 5.f;
    
    // Strong enemy attack params
    static constexpr int   ENEMY_STRONG_MAX_CONSECUTIVE_SWORD_ATTACKS = 6;
    static constexpr int   ENEMY_STRONG_BULLET_BURST_COUNT = 8;
    static constexpr int   ENEMY_STRONG_SUPER_BULLET_COUNT = 8;
    static constexpr float ENEMY_STRONG_SUPER_BULLET_DAMAGE = 8.f;
    
    // Elite enemy attack params
    static constexpr int   ENEMY_ELITE_MAX_CONSECUTIVE_SWORD_ATTACKS = 7;
    static constexpr int   ENEMY_ELITE_BULLET_BURST_COUNT = 10;
    static constexpr int   ENEMY_ELITE_SUPER_BULLET_COUNT = 8;
    static constexpr float ENEMY_ELITE_SUPER_BULLET_DAMAGE = 8.f;
    
    // Super enemy attack params
    static constexpr int   ENEMY_SUPER_MAX_CONSECUTIVE_SWORD_ATTACKS = 10;
    static constexpr int   ENEMY_SUPER_BULLET_BURST_COUNT = 12;
    static constexpr int   ENEMY_SUPER_SUPER_BULLET_COUNT = 10;
    static constexpr float ENEMY_SUPER_SUPER_BULLET_DAMAGE = 25.f;
    
    // Emperor enemy attack params
    static constexpr int   ENEMY_EMPEROR_MAX_CONSECUTIVE_SWORD_ATTACKS = 15;
    static constexpr int   ENEMY_EMPEROR_BULLET_BURST_COUNT = 6;
    static constexpr int   ENEMY_EMPEROR_SUPER_BULLET_COUNT = 3;
    static constexpr float ENEMY_EMPEROR_SUPER_BULLET_DAMAGE = 8.f;

    // Constructor and methods
    LoadLevel(GameEngine& game);

    // Loads the level from the specified file, resetting the EntityManager
    void load(const std::string& levelPath, EntityManager& entityManager);

private:
    GameEngine& m_game;
};
