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

    // Parametri per il player
    static constexpr float PLAYER_HEALTH = 15.f;
    static constexpr float PLAYER_BB_SIZE = 80.f;
    static constexpr float GRAVITY_VAL = 1000.f;

    // Parametri per i nemici in base al tipo:
    // EnemyFast
    static constexpr float ENEMY_FAST_SPEED_MULTIPLIER = 1.5f;
    static constexpr int   ENEMY_FAST_HEALTH = 5;
    static constexpr int   ENEMY_FAST_DAMAGE = 1;

    // EnemyStrong
    static constexpr float ENEMY_STRONG_SPEED_MULTIPLIER = 0.7f;
    static constexpr int   ENEMY_STRONG_HEALTH = 10;
    static constexpr int   ENEMY_STRONG_DAMAGE = 3;

    // EnemyElite
    static constexpr float ENEMY_ELITE_SPEED_MULTIPLIER = 1.5f;
    static constexpr int   ENEMY_ELITE_HEALTH = 15;
    static constexpr int   ENEMY_ELITE_DAMAGE = 4;

    // EnemyNormal
    static constexpr float ENEMY_NORMAL_SPEED_MULTIPLIER = 1.0f;
    static constexpr int   ENEMY_NORMAL_HEALTH = 8;
    static constexpr int   ENEMY_NORMAL_DAMAGE = 2;

    LoadLevel(GameEngine& game);

    // Carica il livello dal file specificato, ripristinando l'EntityManager
    void load(const std::string& levelPath, EntityManager& entityManager);

private:
    GameEngine& m_game;
};
