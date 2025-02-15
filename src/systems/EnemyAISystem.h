#pragma once

#include "EntityManager.hpp"
#include "Spawner.h"
#include "GameEngine.h" // Make sure to include the GameEngine header

class EnemyAISystem {
public:
    // Updated constructor declaration with GameEngine parameter
    EnemyAISystem(EntityManager& entityManager, Spawner& spawner, GameEngine& game);

    static constexpr float MAX_FALL_SPEED = 600.f;
    static constexpr float KNOCKBACK_DECAY_FACTOR = 0.99f;
    static constexpr float DEFAULT_GRAVITY = 800.f;
    static constexpr float PLAYER_VISIBLE_DISTANCE = 500.f;
    static constexpr float ATTACK_RANGE = 100.f;
    static constexpr float ATTACK_TIMER_DEFAULT = 0.3f;
    static constexpr float SWORD_SPAWN_THRESHOLD = 0.7f;
    static constexpr float ATTACK_COOLDOWN = 0.4f;
    static constexpr float FOLLOW_MOVE_SPEED = 100.f;
    static constexpr float RECOGNITION_MOVE_SPEED = 80.f;
    static constexpr float RECOGNITION_DISTANCE_THRESHOLD = 15.f;
    static constexpr float PATROL_SPEED = 70.f;
    static constexpr float PATROL_DISTANCE_THRESHOLD = 10.f;
    static constexpr float PATROL_WAIT_DURATION = 2.0f;
    
    void update(float deltaTime);

private:
    EntityManager& m_entityManager;
    Spawner* m_spawner;
    GameEngine& m_game; // Added member to access assets
};
