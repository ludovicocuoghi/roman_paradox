#pragma once
#include "GameEngine.h"
#include "EntityManager.hpp"
#include <string>
#include <unordered_map>

class LoadLevel {
public:
    // Costanti per la griglia
    static constexpr float GRID_SIZE = 96.f;
    static constexpr float HALF_GRID = GRID_SIZE * 0.5f;

    // Multiplicatori per gli offset dei pipe

    static constexpr float PIPE_REALX_OFFSET_MULTIPLIER = 1.f;        // GRID_SIZE * 0.5


    static constexpr float PIPETALL_REALY_OFFSET_MULTIPLIER = 1.5f;   // GRID_SIZE * 1.5
    static constexpr float PIPE_REALY_OFFSET_MULTIPLIER = 0.5f;        // GRID_SIZE * 1.0
    static constexpr float PIPEBROKEN_REALY_OFFSET_MULTIPLIER= 1.f;    
    static constexpr float LEVELDOOR_REALY_OFFSET_MULTIPLIER = 2.f;
    static constexpr float EMPEROR_REALY_OFFSET_MULTIPLIER = 1.f;

    static constexpr float EMPEROR_BB_WIDTH = 160.f;
    static constexpr float EMPEROR_BB_HEIGHT = 250.f;
    // Parametri per il player
    static constexpr float PLAYER_HEALTH = 50.f;
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

    LoadLevel(GameEngine& game);

    // Carica il livello dal file specificato, ripristinando l'EntityManager
    void load(const std::string& levelPath, EntityManager& entityManager);

private:
    GameEngine& m_game;
};
