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
    // --- Aggiornamento animazioni del giocatore ---
    for (auto& entity : m_entityManager.getEntities("player")) {
        if (!entity->has<CAnimation>() || !entity->has<CState>()) continue;

        auto& canim = entity->get<CAnimation>();
        auto& st    = entity->get<CState>();
        auto& trans = entity->get<CTransform>();

        if (st.state == "attack") {
            st.attackTime -= deltaTime;
            if (m_game.assets().hasAnimation("PlayerAttack")) {
                if (canim.animation.getName() != "PlayerAttack") {
                    canim.animation = m_game.assets().getAnimation("PlayerAttack");
                    canim.repeat = false;
                    if (m_lastDirection < 0)
                        flipSpriteLeft(canim.animation.getMutableSprite());
                    else
                        flipSpriteRight(canim.animation.getMutableSprite());
                }
            }
            if (st.attackTime <= 0.f) {
                st.state = (std::abs(trans.velocity.x) > 1.f) ? "run" : "idle";
            }
        } else {
            std::string desiredAnim;
            if (std::abs(trans.velocity.y) > 0.1f)
                desiredAnim = "PlayerAir";
            else if (std::abs(trans.velocity.x) > 1.f)
                desiredAnim = "PlayerRun";
            else
                desiredAnim = "PlayerStand";

            if (m_game.assets().hasAnimation(desiredAnim) &&
                canim.animation.getName() != desiredAnim)
            {
                canim.animation = m_game.assets().getAnimation(desiredAnim);
                canim.repeat = true;
                if (m_lastDirection < 0)
                    flipSpriteLeft(canim.animation.getMutableSprite());
                else
                    flipSpriteRight(canim.animation.getMutableSprite());
            }
        }
        canim.animation.update(deltaTime);
    }

    // --- Aggiornamento animazioni dei tile (es. blocchi con domanda) ---
    for (auto& entity : m_entityManager.getEntities("tile")) {
        if (!entity->has<CAnimation>() || !entity->has<CState>()) continue;

        auto& anim  = entity->get<CAnimation>();
        auto& state = entity->get<CState>();

        if (state.state == "activated" && anim.animation.getName() == "TreasureBoxAnim") {
            if (m_game.assets().hasAnimation("TexTreasureBoxHit")) {
                anim.animation = m_game.assets().getAnimation("TexTreasureBoxHit");
                anim.repeat = false;
                std::cout << "[DEBUG] Question block hit, animation changed.\n";
            }
        }
        anim.animation.update(deltaTime);
    }

    // --- Aggiornamento animazioni per collectable (es. monete, grappoli) ---
    for (auto& entity : m_entityManager.getEntities("collectable")) {
        if (!entity->has<CAnimation>()) continue;

        auto& anim = entity->get<CAnimation>();
        anim.animation.update(deltaTime);
    }

    // --- Aggiornamento animazioni dei nemici ---
    for (auto& enemy : m_entityManager.getEntities("enemy")) {
        if (!enemy->has<CAnimation>() || !enemy->has<CEnemyAI>()) continue;

        auto& canim = enemy->get<CAnimation>();
        auto& ai = enemy->get<CEnemyAI>();

        std::string baseAnimName;
        switch (ai.enemyType) {
            case EnemyType::Fast:   baseAnimName = "EnemyFast"; break;
            case EnemyType::Strong: baseAnimName = "EnemyStrong"; break;
            case EnemyType::Elite:  baseAnimName = "EnemyElite"; break;
            default:                baseAnimName = "EnemyNormal"; break;
        }

        std::string desiredAnim;
        switch (ai.enemyState) {
            case EnemyState::Patrol: desiredAnim = baseAnimName + "_Run"; break;
            case EnemyState::Chase:  desiredAnim = baseAnimName + "_Chase"; break;
            case EnemyState::Attack: desiredAnim = baseAnimName + "_Attack"; break;
            default:                desiredAnim = baseAnimName + "_Run"; break;
        }

        if (m_game.assets().hasAnimation(desiredAnim) &&
            canim.animation.getName() != desiredAnim)
        {
            canim.animation = m_game.assets().getAnimation(desiredAnim);
            // Se lo stato è "Attack" non eseguire il looping
            canim.repeat = (ai.enemyState != EnemyState::Attack);
            if (ai.facingDirection < 0)
                flipSpriteLeft(canim.animation.getMutableSprite());
            else
                flipSpriteRight(canim.animation.getMutableSprite());
        }
        canim.animation.update(deltaTime);
    }
}
