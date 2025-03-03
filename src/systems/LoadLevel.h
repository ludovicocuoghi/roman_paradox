#pragma once
#include "GameEngine.h"
#include "EntityManager.hpp"
#include <string>
#include <unordered_map>

class LoadLevel {
public:
    // Costanti per la griglia
     // Costanti per la griglia
     static constexpr float GRID_SIZE = 96.f;
     static constexpr float HALF_GRID = GRID_SIZE * 0.5f;
 
     // Multiplicatori per gli offset dei pipe
     static constexpr float PIPE_REALX_OFFSET_MULTIPLIER = 1.f;        // GRID_SIZE * 0.5
     static constexpr float PIPETALL_REALY_OFFSET_MULTIPLIER = 1.5f;      // GRID_SIZE * 1.5
     static constexpr float PIPE_REALY_OFFSET_MULTIPLIER = 0.5f;         // GRID_SIZE * 1.0
     static constexpr float PIPEBROKEN_REALY_OFFSET_MULTIPLIER= 1.f;    
     static constexpr float LEVELDOOR_REALY_OFFSET_MULTIPLIER = 2.f;
     static constexpr float BLACKHOLE_OFFSET_MULTIPLIER = 2.f;
     static constexpr float EMPEROR_REALY_OFFSET_MULTIPLIER = 1.f;
 
     static constexpr float EMPEROR_BB_WIDTH = 160.f;
     static constexpr float EMPEROR_BB_HEIGHT = 250.f;
     // Parametri per il player
     static constexpr float PLAYER_HEALTH = 100.f;
     static constexpr float PLAYER_BB_SIZE = 80.f;
     static constexpr float GRAVITY_VAL = 1000.f;
 
     // Parametri per i nemici in base al tipo:
     // EnemyFast
     static constexpr float ENEMY_FAST_SPEED_MULTIPLIER = 1.8f;
     static constexpr int   ENEMY_FAST_HEALTH = 20;
     static constexpr int   ENEMY_FAST_DAMAGE = 5;
 
     // EnemyStrong
     static constexpr float ENEMY_STRONG_SPEED_MULTIPLIER = 1.3f;
     static constexpr int   ENEMY_STRONG_HEALTH = 30;
     static constexpr int   ENEMY_STRONG_DAMAGE = 15;
 
     // EnemyElite
     static constexpr float ENEMY_ELITE_SPEED_MULTIPLIER = 1.8f;
     static constexpr int   ENEMY_ELITE_HEALTH = 40;
     static constexpr int   ENEMY_ELITE_DAMAGE = 15;
 
     // EnemyNormal
     static constexpr float ENEMY_NORMAL_SPEED_MULTIPLIER = 1.0f;
     static constexpr int   ENEMY_NORMAL_HEALTH = 20;
     static constexpr int   ENEMY_NORMAL_DAMAGE = 8;
 
     // EnemyEmperor
     static constexpr float ENEMY_EMPEROR_SPEED_MULTIPLIER = 1.0f;
     static constexpr int   ENEMY_EMPEROR_HEALTH = 100;
     static constexpr int   ENEMY_EMPEROR_DAMAGE = 10;
     
     // EnemySuper
     static constexpr float ENEMY_SUPER_SPEED_MULTIPLIER = 2.0f;
     static constexpr int   ENEMY_SUPER_HEALTH = 50;
     static constexpr int   ENEMY_SUPER_DAMAGE = 20;
 
     // Bullet Damage
     static constexpr int   BULLET_DAMAGE_PLAYER = 5;
     static constexpr int   BULLET_DAMAGE_ENEMY_FAST = 3;
     static constexpr int   BULLET_DAMAGE_ENEMY_NORMAL = 4;
     static constexpr int   BULLET_DAMAGE_ENEMY_STRONG = 6;
     static constexpr int   BULLET_DAMAGE_ENEMY_ELITE = 5;
     static constexpr int   BULLET_DAMAGE_ENEMY_EMPEROR = 7;
     static constexpr int   BULLET_DAMAGE_ENEMY_SUPER = 8;
 
     // Parametri per attacchi con spada e burst di proiettili
     // EnemyFast
     static constexpr int   ENEMY_FAST_MAX_CONSECUTIVE_SWORD_ATTACKS = 4;
     static constexpr int   ENEMY_FAST_BULLET_BURST_COUNT = 6;
     static constexpr int   ENEMY_FAST_SUPER_BULLET_COUNT = 4;
     static constexpr float ENEMY_FAST_SUPER_BULLET_DAMAGE = 5.f;
 
     // EnemyNormal
     static constexpr int   ENEMY_NORMAL_MAX_CONSECUTIVE_SWORD_ATTACKS = 5;
     static constexpr int   ENEMY_NORMAL_BULLET_BURST_COUNT = 5;
     static constexpr int   ENEMY_NORMAL_SUPER_BULLET_COUNT = 8;
     static constexpr float ENEMY_NORMAL_SUPER_BULLET_DAMAGE = 10.f;
 
     // EnemyStrong
     static constexpr int   ENEMY_STRONG_MAX_CONSECUTIVE_SWORD_ATTACKS = 6;
     static constexpr int   ENEMY_STRONG_BULLET_BURST_COUNT = 7;
     static constexpr int   ENEMY_STRONG_SUPER_BULLET_COUNT = 8;
     static constexpr float ENEMY_STRONG_SUPER_BULLET_DAMAGE = 20.f;
 
     // EnemyElite
     static constexpr int   ENEMY_ELITE_MAX_CONSECUTIVE_SWORD_ATTACKS = 7;
     static constexpr int   ENEMY_ELITE_BULLET_BURST_COUNT = 15;
     static constexpr int   ENEMY_ELITE_SUPER_BULLET_COUNT = 8;
     static constexpr float ENEMY_ELITE_SUPER_BULLET_DAMAGE = 20.f;
 
     // EnemyEmperor
     static constexpr int   ENEMY_EMPEROR_MAX_CONSECUTIVE_SWORD_ATTACKS = 15;
     static constexpr int   ENEMY_EMPEROR_BULLET_BURST_COUNT = 6;
     static constexpr int   ENEMY_EMPEROR_SUPER_BULLET_COUNT = 3;
     static constexpr float ENEMY_EMPEROR_SUPER_BULLET_DAMAGE = 1.7f;
 
     // EnemySuper
     static constexpr int   ENEMY_SUPER_MAX_CONSECUTIVE_SWORD_ATTACKS = 10;
     static constexpr int   ENEMY_SUPER_BULLET_BURST_COUNT = 12;
     static constexpr int   ENEMY_SUPER_SUPER_BULLET_COUNT = 10;
     static constexpr float ENEMY_SUPER_SUPER_BULLET_DAMAGE = 25.f;

    LoadLevel(GameEngine& game);

    // Carica il livello dal file specificato, ripristinando l'EntityManager
    void load(const std::string& levelPath, EntityManager& entityManager);

private:
    GameEngine& m_game;
};
