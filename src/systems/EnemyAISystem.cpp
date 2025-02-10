#include "EnemyAISystem.h"
#include "SpriteUtils.h" // For flipSpriteLeft and flipSpriteRight
#include <cmath>
#include <algorithm>
#include <iostream>

EnemyAISystem::EnemyAISystem(GameEngine& game, EntityManager& entityManager)
    : m_game(game), m_entityManager(entityManager)
{
}

void EnemyAISystem::update(float deltaTime) {
    auto enemies = m_entityManager.getEntities("enemy");
    for (auto& enemy : enemies) {
        if (!enemy->has<CTransform>() || !enemy->has<CState>() || !enemy->has<CEnemyAI>())
            continue;

        auto& enemyTrans = enemy->get<CTransform>();
        auto& state = enemy->get<CState>();
        auto& enemyAI = enemy->get<CEnemyAI>();

        // Handle knockback
        if (state.state == "knockback" && state.knockbackTimer > 0.f) {
            enemyTrans.velocity.x *= 0.95f;
            enemyTrans.pos.x += enemyTrans.velocity.x * deltaTime;
            enemyTrans.pos.y += enemyTrans.velocity.y * deltaTime;
            continue;
        }

        // Apply gravity
        bool isOnGround = false;
        sf::FloatRect enemyRect = enemy->get<CBoundingBox>().getRect(enemyTrans.pos);
        for (auto& tile : m_entityManager.getEntities("tile")) {
            if (!tile->has<CTransform>() || !tile->has<CBoundingBox>())
                continue;
            auto& tileTrans = tile->get<CTransform>();
            auto& tileBB = tile->get<CBoundingBox>();
            sf::FloatRect tileRect = tileBB.getRect(tileTrans.pos);
            if (enemyRect.intersects(tileRect)) {
                isOnGround = true;
                break;
            }
        }
        if (!isOnGround) {
            enemyTrans.velocity.y += enemy->get<CGravity>().gravity * deltaTime;
            enemyTrans.velocity.y = std::clamp(enemyTrans.velocity.y, -MAX_FALL_SPEED, MAX_FALL_SPEED);
        } else {
            enemyTrans.velocity.y = 0.f;
        }

        // Ignore patrol if chasing/attacking
        if (enemyAI.enemyState == EnemyState::Chase || enemyAI.enemyState == EnemyState::Attack)
            continue;

        // Patrol Movement
        if (!enemyAI.patrolPoints.empty()) {
            Vec2<float> patrolTarget = enemyAI.patrolPoints[enemyAI.currentPatrolIndex];
            float dx = patrolTarget.x - enemyTrans.pos.x;
            float dy = patrolTarget.y - enemyTrans.pos.y;
            float distance = std::sqrt(dx * dx + dy * dy);

            // Debug output for movement
            std::cout << "[DEBUG] Enemy " << enemy->id()
                      << " | Pos: (" << enemyTrans.pos.x << ", " << enemyTrans.pos.y
                      << ") | Target: (" << patrolTarget.x << ", " << patrolTarget.y
                      << ") | Distance: " << distance << std::endl;

            // Check horizontal distance instead of full Euclidean distance
            if (std::abs(dx) < 30.f) {
                if (enemyAI.patrolWaitTime <= 0.f) {
                    std::cout << "[DEBUG] Enemy " << enemy->id()
                              << " REACHED patrol point! Waiting before switching...\n";

                    enemyAI.patrolWaitTime = 0.05f;  // 1-second pause before switching points
                    enemyAI.currentPatrolIndex = (enemyAI.currentPatrolIndex + 1) % enemyAI.patrolPoints.size();
                    enemyTrans.velocity.x = 0;  // Stop movement while waiting
                } else {
                    enemyAI.patrolWaitTime -= deltaTime;
                    enemyTrans.velocity.x = 0;
                }
            } else {
                // Move towards the patrol target horizontally
                float patrolSpeed = 100.f * enemyAI.speedMultiplier;
                enemyTrans.velocity.x = (dx > 0) ? patrolSpeed : -patrolSpeed;
                enemyAI.facingDirection = (dx > 0) ? 1.f : -1.f;
            }
        }

        // Apply movement
        enemyTrans.pos.x += enemyTrans.velocity.x * deltaTime;
        enemyTrans.pos.y += enemyTrans.velocity.y * deltaTime;

        // Fix sprite flipping issue
        if (enemy->has<CAnimation>()) {
            auto& anim = enemy->get<CAnimation>();
            if (enemyAI.facingDirection < 0)
                flipSpriteLeft(anim.animation.getMutableSprite());
            else if (enemyAI.facingDirection > 0)
                flipSpriteRight(anim.animation.getMutableSprite());
        }
    }
}
