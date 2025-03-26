#include "AnimationSystem.h"
#include "Components.hpp"
#include <iostream>
#include <cmath>
#include "SpriteUtils.h"

AnimationSystem::AnimationSystem(GameEngine& game, EntityManager& entityManager, float& lastDirection)
    : m_game(game), m_entityManager(entityManager), m_lastDirection(lastDirection)
{
}

void AnimationSystem::update(float deltaTime) {
    // --- Player Animation ---
    for (auto& entity : m_entityManager.getEntities("player")) {
        if (!entity->has<CAnimation>() || !entity->has<CState>()) continue;
    
        auto& canim = entity->get<CAnimation>();
        auto& st    = entity->get<CState>();
        auto& trans = entity->get<CTransform>();
    
        // Check if the player has FutureArmor
        bool hasFutureArmor = false;
        if (entity->has<CPlayerEquipment>()) {
            hasFutureArmor = entity->get<CPlayerEquipment>().hasFutureArmor;
        }
    
        // Decide animation prefix: "Player" vs. "Future"
        std::string prefix = hasFutureArmor ? "Future" : "";
    
        if (st.state == "attack" || st.inBurst) {
            // Attack logic
            st.attackTime -= deltaTime;
    
            // Choose attack animation based on armor type
            std::string attackAnim = prefix + "PlayerAttack";
    
            if (m_game.assets().hasAnimation(attackAnim)) {
                if (canim.animation.getName() != attackAnim) {
                    canim.animation = m_game.assets().getAnimation(attackAnim);
                    canim.repeat = false;
    
                    // Flip based on last direction
                    if (m_lastDirection < 0)
                        flipSpriteLeft(canim.animation.getMutableSprite());
                    else
                        flipSpriteRight(canim.animation.getMutableSprite());
                }
            }
            
            // Once attackTime is done, revert to run/idle ONLY if not in burst
            if (st.attackTime <= 0.f && !st.inBurst) {
                if (std::abs(trans.velocity.x) > 1.f)
                    st.state = "run";
                else
                    st.state = "idle";
            }
        } else if (st.state == "defense") {
                std::string defenseAnim = prefix + "PlayerDefense";
                if (m_game.assets().hasAnimation(defenseAnim)) {
                    if (canim.animation.getName() != defenseAnim) {
                        canim.animation = m_game.assets().getAnimation(defenseAnim);
                        canim.repeat = true; // Loop defense animation
                        if (m_lastDirection < 0)
                            flipSpriteLeft(canim.animation.getMutableSprite());
                        else
                            flipSpriteRight(canim.animation.getMutableSprite());
                    }
                }
        } else {
            // e.g., "PlayerAir", "PlayerRun", or "PlayerStand"
            // or "FutureAir", "FutureRun", or "FutureStand"
            std::string desiredAnim;
            if (std::abs(trans.velocity.y) > 0.1f)
                desiredAnim = prefix + "PlayerAir";
            else if (std::abs(trans.velocity.x) > 1.f)
                desiredAnim = prefix + "PlayerRun";
            else
                desiredAnim = prefix + "PlayerStand";
    
            if (m_game.assets().hasAnimation(desiredAnim) &&
                canim.animation.getName() != desiredAnim)
            {
                canim.animation = m_game.assets().getAnimation(desiredAnim);
                canim.repeat = true;
                // Flip sprite
                if (m_lastDirection < 0)
                    flipSpriteLeft(canim.animation.getMutableSprite());
                else
                    flipSpriteRight(canim.animation.getMutableSprite());
            }
        }
    
        // Update current animation frame
        canim.animation.update(deltaTime);
    }

    // --- Tile Animation (e.g., Question Blocks) ---
    for (auto& entity : m_entityManager.getEntities("tile")) {
        if (!entity->has<CAnimation>()) continue;

        auto& anim  = entity->get<CAnimation>();
        auto& state = entity->get<CState>();

        if (state.state == "activated" && anim.animation.getName() == "TreasureBoxAnim") {
            if (m_game.assets().hasAnimation("TexTreasureBoxHit")) {
                anim.animation = m_game.assets().getAnimation("TexTreasureBoxHit");
                anim.repeat = false;
            }
        }
        anim.animation.update(deltaTime);
    }

    for (auto& grave : m_entityManager.getEntities("enemyGrave")) {
        if (!grave->has<CAnimation>()) continue;
        grave->get<CAnimation>().animation.update(deltaTime);
    }

    // --- Collectable Animation (e.g., Coins, Items) ---
    for (auto& entity : m_entityManager.getEntities("collectable")) {
        if (!entity->has<CAnimation>()) continue;

        auto& anim = entity->get<CAnimation>();
        anim.animation.update(deltaTime);
    }

    // --- Dec Animation ---
    for (auto& entity : m_entityManager.getEntities("decoration")) {
        if (!entity->has<CAnimation>()) continue;

        auto& anim = entity->get<CAnimation>();
        anim.animation.update(deltaTime);
    }

    // --- Enemy Animation ---
    for (auto& enemy : m_entityManager.getEntities("enemy")) {
        if (!enemy->has<CAnimation>() || !enemy->has<CEnemyAI>()) continue;
    
        auto& canim = enemy->get<CAnimation>();
        auto& ai = enemy->get<CEnemyAI>();
    
        std::string baseAnimName;
        switch (ai.enemyType) {
            case EnemyType::Fast:    baseAnimName = "EnemyFast"; break;
            case EnemyType::Strong:  baseAnimName = "EnemyStrong"; break;
            case EnemyType::Elite:   baseAnimName = "EnemyElite"; break;
            case EnemyType::Super:   baseAnimName = "EnemySuper"; break;
            case EnemyType::Emperor: baseAnimName = "Emperor"; break;
            default:                 baseAnimName = "EnemyNormal"; break;
        }

        if (enemy->has<CEnemyAI>() && enemy->get<CEnemyAI>().enemyType == EnemyType::Super) {
            canim.animation.update(deltaTime);
            continue;  // Skip to next entity
        }
    
        std::string desiredAnim;
        switch (ai.enemyState) {
            case EnemyState::Follow:    desiredAnim = m_game.worldType + "Run" + baseAnimName; break;
            case EnemyState::Attack:    desiredAnim = m_game.worldType + "Hit" + baseAnimName; break; 
            case EnemyState::Knockback: desiredAnim = m_game.worldType + "Hit" + baseAnimName; break;
            default:                    desiredAnim = m_game.worldType + "Stand" + baseAnimName; break;
        }
    
        if (m_game.assets().hasAnimation(desiredAnim) &&
            canim.animation.getName() != desiredAnim)
        {
            canim.animation = m_game.assets().getAnimation(desiredAnim);
            // Disable looping for Attack animations
            canim.repeat = (ai.enemyState != EnemyState::Attack);
            if (ai.facingDirection < 0)
                flipSpriteLeft(canim.animation.getMutableSprite());
            else
                flipSpriteRight(canim.animation.getMutableSprite());
        }
        canim.animation.update(deltaTime);
    }
}
