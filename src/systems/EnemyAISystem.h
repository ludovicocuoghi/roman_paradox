#pragma once

#include "EntityManager.hpp"
#include "Spawner.h"
#include "GameEngine.h"
#include "systems/DialogueSystem.h"

class EnemyAISystem {
public:
    // Updated constructor declaration with GameEngine parameter
    EnemyAISystem(EntityManager& entityManager, Spawner& spawner, GameEngine& game);

    static constexpr float MAX_FALL_SPEED = 600.f;
    static constexpr float KNOCKBACK_DECAY_FACTOR = 0.99f;
    static constexpr float DEFAULT_GRAVITY = 800.f;
    static constexpr float PLAYER_VISIBLE_DISTANCE = 800.f;
    static constexpr float ATTACK_RANGE = 100.f;
    static constexpr float EMPEROR_ATTACK_RANGE = 200.f;

    // Unified Emperor attack constants
    static constexpr int   EMPEROR_RADIAL_BULLETS_COUNT = 60;   
    static constexpr float EMPEROR_RADIAL_BULLETS_RADIUS = 120.f;
    static constexpr float EMPEROR_RADIAL_BULLETS_SPEED = 800.f;
    
    static constexpr int   EMPEROR_RADIAL_SWORDS_COUNT = 100;    
    static constexpr float EMPEROR_RADIAL_SWORDS_RADIUS = 120.f;
    static constexpr float EMPEROR_RADIAL_SWORDS_SPEED = 800.f;
    
    static constexpr float ATTACK_TIMER_DEFAULT = 0.3f;
    static constexpr float SWORD_SPAWN_THRESHOLD = 0.7f;

    static constexpr float ATTACK_COOLDOWN = 0.3f;
    static constexpr float FOLLOW_MOVE_SPEED = 290.f;
    void update(float deltaTime);
    void updateCitizens(float deltaTime, const CTransform& playerTrans);
    void processSuperEnemies(float deltaTime);
    void setDialogueSystem(std::shared_ptr<DialogueSystem> dialogueSystem) {
        m_dialogueSystem = dialogueSystem;
    }

private:
    EntityManager& m_entityManager;
    Spawner* m_spawner;
    GameEngine& m_game; 
    std::shared_ptr<DialogueSystem> m_dialogueSystem;
    std::map<std::string, bool> m_triggeredDialogues;
};
