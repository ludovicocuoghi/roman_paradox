#include "EnemyAISystem.h"
#include "SpriteUtils.h"    // For flipSpriteLeft, flipSpriteRight
#include "Spawner.h"
#include <cmath>
#include <algorithm>
#include <iostream>

constexpr float MAX_FALL_SPEED_F = 600.f;

// Helper function to convert EnemyState to a string for debugging
static std::string enemyStateToString(EnemyState st) {
    switch (st) {
        case EnemyState::Follow:       return "Follow";
        case EnemyState::Attack:       return "Attack";
        case EnemyState::Knockback:    return "Knockback";
        case EnemyState::Idle:         return "Idle";
        case EnemyState::Patrol:       return "Patrol";
        case EnemyState::Recognition:  return "Recognition";
        default:                       return "Unknown";
    }
}

// Helper: Check if two line segments (p1-p2 and q1-q2) intersect.
static bool segmentsIntersect(const Vec2<float>& p1, const Vec2<float>& p2,
                              const Vec2<float>& q1, const Vec2<float>& q2)
{
    auto orientation = [](const Vec2<float>& a, const Vec2<float>& b, const Vec2<float>& c) -> float {
        return (b.y - a.y) * (c.x - b.x) - (b.x - a.x) * (c.y - b.y);
    };

    float o1 = orientation(p1, p2, q1);
    float o2 = orientation(p1, p2, q2);
    float o3 = orientation(q1, q2, p1);
    float o4 = orientation(q1, q2, p2);

    return (o1 * o2 < 0 && o3 * o4 < 0);
}

// Helper: Check if a line (start-end) intersects a rectangle.
static bool lineIntersectsRect(const Vec2<float>& start, const Vec2<float>& end,
                               const sf::FloatRect& rect)
{
    Vec2<float> topLeft     { rect.left,               rect.top };
    Vec2<float> topRight    { rect.left + rect.width,  rect.top };
    Vec2<float> bottomRight { rect.left + rect.width,  rect.top + rect.height };
    Vec2<float> bottomLeft  { rect.left,               rect.top + rect.height };

    return (segmentsIntersect(start, end, topLeft, topRight) ||
            segmentsIntersect(start, end, topRight, bottomRight) ||
            segmentsIntersect(start, end, bottomRight, bottomLeft) ||
            segmentsIntersect(start, end, bottomLeft, topLeft));
}

// Checks line-of-sight between enemy and player.
bool checkLineOfSight(const Vec2<float>& enemyCenter,
                      const Vec2<float>& playerCenter,
                      EntityManager& entityManager)
{
    for (auto& tile : entityManager.getEntities("tile")) {
        if (!tile->has<CTransform>() || !tile->has<CBoundingBox>()) continue;

        auto& tileTrans = tile->get<CTransform>();
        auto& tileBB    = tile->get<CBoundingBox>();
        sf::FloatRect tileRect = tileBB.getRect(tileTrans.pos);

        if (lineIntersectsRect(enemyCenter, playerCenter, tileRect)) {
            return false;
        }
    }
    return true;
}

EnemyAISystem::EnemyAISystem(EntityManager& entityManager, Spawner& spawner, GameEngine& game)
    : m_entityManager(entityManager), m_spawner(&spawner), m_game(game)
{
}

// Main AI Update Logic
void EnemyAISystem::update(float deltaTime)
{
    auto enemies = m_entityManager.getEntities("enemy");
    if (enemies.empty()) return;

    auto players = m_entityManager.getEntities("player");
    if (players.empty()) return;

    auto& player      = players[0];
    auto& playerTrans = player->get<CTransform>();

    for (auto& enemy : enemies) {
        // Verifica che l'entità abbia i componenti necessari
        if (!enemy->has<CTransform>() || 
            !enemy->has<CEnemyAI>()    || 
            !enemy->has<CAnimation>()) 
        {
            continue;
        }

        auto& enemyTrans = enemy->get<CTransform>();
        auto& enemyAI    = enemy->get<CEnemyAI>();
        auto& anim       = enemy->get<CAnimation>(); // Recupera il componente animazione una volta

        // Stampa di debug
        std::cout << "Enemy ID: " << enemy->id()
                  << " | State: " << enemyStateToString(enemyAI.enemyState)
                  << std::endl;

        // ========== 1) Gestione del Knockback ==========
        if (enemyAI.enemyState == EnemyState::Knockback) {
            std::cout << "Enemy ID: " << enemy->id() << " | State: Knockback - Entered" << std::endl;
            if (enemyAI.knockbackTimer > 0.f) {
                enemyTrans.velocity.x *= 0.95f;
                enemyTrans.pos.x      += enemyTrans.velocity.x * deltaTime;
                enemyTrans.pos.y      += enemyTrans.velocity.y * deltaTime;
                enemyAI.knockbackTimer -= deltaTime;
                continue;
            } else {
                enemyAI.knockbackTimer = 0.f;
                enemyAI.enemyState = EnemyState::Follow;
            }
        }

        // ========== 2) Applicazione della gravità e controllo del suolo ==========
        bool isOnGround = false;
        if (enemy->has<CBoundingBox>()) {
            auto& bb = enemy->get<CBoundingBox>();
            sf::FloatRect enemyRect = bb.getRect(enemyTrans.pos);

            for (auto& tile : m_entityManager.getEntities("tile")) {
                if (!tile->has<CTransform>() || !tile->has<CBoundingBox>()) continue;
                auto& tileTrans = tile->get<CTransform>();
                auto& tileBB    = tile->get<CBoundingBox>();
                sf::FloatRect tileRect = tileBB.getRect(tileTrans.pos);

                if (enemyRect.intersects(tileRect)) {
                    isOnGround = true;
                    break;
                }
            }
        }

        if (!isOnGround) {
            float grav = enemy->has<CGravity>() 
                         ? enemy->get<CGravity>().gravity 
                         : 800.f;
            enemyTrans.velocity.y += grav * deltaTime;
            enemyTrans.velocity.y  = std::clamp(enemyTrans.velocity.y, -MAX_FALL_SPEED_F, MAX_FALL_SPEED_F);
        } else {
            enemyTrans.velocity.y = 0.f;
        }

        // ========== 3) Controllo della linea di vista e della distanza ==========
        float dx = playerTrans.pos.x - enemyTrans.pos.x;
        float dy = playerTrans.pos.y - enemyTrans.pos.y;
        float distance = std::sqrt(dx * dx + dy * dy);

        bool canSeePlayer  = checkLineOfSight(enemyTrans.pos, playerTrans.pos, m_entityManager);
        bool playerVisible = (distance < 500.f) || canSeePlayer;

        bool shouldFollow = false;
        switch (enemyAI.enemyBehavior) {
            case EnemyBehavior::FollowOne:
                if (playerVisible) shouldFollow = true;
                break;
            case EnemyBehavior::FollowTwo:
                if (playerVisible) {
                    enemyAI.enemyState = EnemyState::Follow;
                    shouldFollow = true;
                } else if (enemyAI.enemyState == EnemyState::Follow) {
                    shouldFollow = true;
                }
                break;
        }

        // ========== 4) Logica d'attacco ==========
        const float attackRange = 100.f;
        if (shouldFollow && distance < attackRange && enemyAI.attackCooldown <= 0.f) {
            // Se non è già in stato di attacco, inizia l'attacco
            if (enemyAI.enemyState != EnemyState::Attack) {
                enemyAI.enemyState   = EnemyState::Attack;
                enemyAI.attackTimer  = 1.0f;
                enemyAI.swordSpawned = false;
            }
        } else if (shouldFollow) {
            enemyAI.enemyState = EnemyState::Follow;
        } else {
            if (enemyAI.enemyState == EnemyState::Follow || enemyAI.enemyState == EnemyState::Attack) {
                enemyAI.enemyState       = EnemyState::Recognition;
                enemyAI.recognitionTimer = 0.f;
                enemyAI.lastSeenPlayerPos = playerTrans.pos;
            } else {
                enemyAI.enemyState = EnemyState::Idle;
            }
        }

        // ========== 5) Aggiornamento dello stato ATTACK ==========
        if (enemyAI.enemyState == EnemyState::Attack) {
            // Ferma il movimento durante l'attacco
            enemyTrans.velocity.x = 0.f;
            enemyAI.attackTimer -= deltaTime;
            // Quando il timer scende sotto 0.5 sec e la spada non è ancora stata generata, chiamala
            if (!enemyAI.swordSpawned && enemyAI.attackTimer < 0.5f) {
                m_spawner->spawnEnemySword(enemy);
                enemyAI.swordSpawned = true;
            }
            // Quando l'attacco è completato, imposta un cooldown e ritorna allo stato Follow
            if (enemyAI.attackTimer <= 0.f) {
                enemyAI.attackCooldown = 1.0f;
                enemyAI.enemyState = EnemyState::Follow;
            }
        }

        // ========== 6) Logica di movimento (stato FOLLOW) ==========
        // ========== 6) Movement Logic (Follow State) ==========
        if (enemyAI.enemyState == EnemyState::Follow) {
            float moveSpeed = 100.0f;
            // Set velocity and facing direction
            if (dx > 0.f) {
                enemyTrans.velocity.x = moveSpeed;
                enemyAI.facingDirection = 1.f;
            } else if (dx < 0.f) {
                enemyTrans.velocity.x = -moveSpeed;
                enemyAI.facingDirection = -1.f;
            } else {
                enemyTrans.velocity.x = 0.f;
            }

            // Determine the correct run animation based on enemy type
            std::string runAnimName;
            switch (enemyAI.enemyType) {
                case EnemyType::Fast:   runAnimName = "EnemyFast_Run";   break;
                case EnemyType::Strong: runAnimName = "EnemyStrong_Run"; break;
                case EnemyType::Elite:  runAnimName = "EnemyElite_Run";  break;
                case EnemyType::Normal: runAnimName = "EnemyNormal_Run"; break;
            }
            
            // Check if the current animation is not the desired run animation.
            // Note: You may need a way to query the current animation's name,
            // for example, anim.animation.name().
            if (anim.animation.getName() != runAnimName) {
                const Animation& runAnim = m_game.assets().getAnimation(runAnimName);
                anim.animation = runAnim;
                anim.animation.reset();  // Reset only when switching animations
            }
        }

        // ========== 7) Logica dello stato RECOGNITION ==========
        if (enemyAI.enemyState == EnemyState::Recognition) {
            float recognitionMoveSpeed = 80.0f;
            float distanceToLastSeen = std::sqrt(
                std::pow(enemyAI.lastSeenPlayerPos.x - enemyTrans.pos.x, 2) +
                std::pow(enemyAI.lastSeenPlayerPos.y - enemyTrans.pos.y, 2)
            );

            if (distanceToLastSeen > 15.f) {
                if (enemyAI.lastSeenPlayerPos.x > enemyTrans.pos.x) {
                    enemyTrans.velocity.x = recognitionMoveSpeed;
                    enemyAI.facingDirection = 1.f;
                    anim.animation.reset();
                } else if (enemyAI.lastSeenPlayerPos.x < enemyTrans.pos.x) {
                    enemyTrans.velocity.x = -recognitionMoveSpeed;
                    enemyAI.facingDirection = -1.f;
                    anim.animation.reset();
                } else {
                    enemyTrans.velocity.x = 0.f;
                    anim.animation.reset();
                }
            } else {
                enemyTrans.velocity.x = 0.f;
                anim.animation.reset();

                if (!enemyAI.inRecognitionArea) {
                    enemyAI.inRecognitionArea = true;
                    enemyAI.recognitionTimer += deltaTime;

                    if (enemyAI.recognitionTimer >= enemyAI.maxRecognitionTime) {
                        enemyAI.recognitionTimer = 0.f;
                        enemyAI.inRecognitionArea = false;
                        if (!enemyAI.patrolPoints.empty()) {
                            enemyAI.enemyState = EnemyState::Patrol;
                        } else {
                            enemyAI.enemyState = EnemyState::Idle;
                        }
                    }
                } else {
                    enemyAI.recognitionTimer += deltaTime;

                    if (enemyAI.recognitionTimer >= enemyAI.maxRecognitionTime) {
                        enemyAI.recognitionTimer = 0.f;
                        enemyAI.inRecognitionArea = false;
                        if (!enemyAI.patrolPoints.empty()) {
                            enemyAI.enemyState = EnemyState::Patrol;
                        } else {
                            enemyAI.enemyState = EnemyState::Idle;
                        }
                    }
                }
            }
        }

        // ========== 8) Logica dello stato PATROL ==========
        if (enemyAI.enemyState == EnemyState::Patrol) {
            float patrolSpeed = 70.0f;

            if (enemyAI.patrolPoints.empty()) {
                enemyAI.enemyState = EnemyState::Idle;
            } else {
                Vec2<float> targetPatrolPoint = enemyAI.patrolPoints[enemyAI.currentPatrolIndex];
                float dxPatrol = targetPatrolPoint.x - enemyTrans.pos.x;
                float dyPatrol = targetPatrolPoint.y - enemyTrans.pos.y;
                float patrolPointDistance = std::sqrt(dxPatrol * dxPatrol + dyPatrol * dyPatrol);

                if (patrolPointDistance < 10.f) {
                    enemyTrans.velocity.x = 0.f;
                    anim.animation.reset();

                    enemyAI.patrolWaitTime += deltaTime;
                    if (enemyAI.patrolWaitTime >= 2.0f) {
                        enemyAI.patrolWaitTime = 0.f;
                        enemyAI.currentPatrolIndex = 
                            (enemyAI.currentPatrolIndex + 1) % enemyAI.patrolPoints.size();
                    }
                } else {
                    enemyAI.patrolWaitTime = 0.f;
                    if (dxPatrol > 0.f) {
                        enemyTrans.velocity.x = patrolSpeed;
                        enemyAI.facingDirection = 1.f;
                        anim.animation.reset();
                    } else if (dxPatrol < 0.f) {
                        enemyTrans.velocity.x = -patrolSpeed;
                        enemyAI.facingDirection = -1.f;
                        anim.animation.reset();
                    } else {
                        enemyTrans.velocity.x = 0.f;
                        anim.animation.reset();
                    }
                }
            }
        }

        // ========== 9) Logica dello stato IDLE ==========
        if (enemyAI.enemyState == EnemyState::Idle) {
            enemyTrans.velocity.x = 0.f;
            anim.animation.reset();
        }

        // Aggiornamento generale della posizione (per tutti gli stati in cui non è già stato applicato)
        enemyTrans.pos.x += enemyTrans.velocity.x * deltaTime;
        enemyTrans.pos.y += enemyTrans.velocity.y * deltaTime;

        // ========== 10) Flip dell'animazione ==========
        if (enemyAI.facingDirection < 0.f) {
            flipSpriteLeft(anim.animation.getMutableSprite());
        } else {
            flipSpriteRight(anim.animation.getMutableSprite());
        }
    }
}
