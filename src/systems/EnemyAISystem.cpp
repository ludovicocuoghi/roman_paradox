#include "EnemyAISystem.h"
#include "SpriteUtils.h"
#include "Spawner.h"
#include <cmath>
#include <algorithm>
#include <iostream>

// Le funzioni statiche di utilità rimangono invariate
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

static bool lineIntersectsRect(const Vec2<float>& start, const Vec2<float>& end,
                               const sf::FloatRect& rect)
{
    Vec2<float> topLeft  { rect.left,               rect.top };
    Vec2<float> topRight { rect.left + rect.width,  rect.top };
    Vec2<float> botRight { rect.left + rect.width,  rect.top + rect.height };
    Vec2<float> botLeft  { rect.left,               rect.top + rect.height };

    return (segmentsIntersect(start, end, topLeft, topRight) ||
            segmentsIntersect(start, end, topRight, botRight) ||
            segmentsIntersect(start, end, botRight, botLeft) ||
            segmentsIntersect(start, end, botLeft, topLeft));
}

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

void EnemyAISystem::update(float deltaTime)
{
    auto enemies = m_entityManager.getEntities("enemy");
    if (enemies.empty()) return;

    auto players = m_entityManager.getEntities("player");
    if (players.empty()) return;

    auto& player      = players[0];
    auto& playerTrans = player->get<CTransform>();

    for (auto& enemy : enemies) {
        if (!enemy->has<CTransform>() || 
            !enemy->has<CEnemyAI>()    || 
            !enemy->has<CAnimation>()) 
        {
            continue;
        }

        auto& enemyTrans = enemy->get<CTransform>();
        auto& enemyAI    = enemy->get<CEnemyAI>();
        auto& anim       = enemy->get<CAnimation>();

        // Gestione del knockback
        if (enemyAI.enemyState == EnemyState::Knockback) {
            std::cout << "[DEBUG] Knockback: vel.x=" << enemyTrans.velocity.x
                      << " pos.x=" << enemyTrans.pos.x
                      << " timer=" << enemyAI.knockbackTimer << "\n";

            // Aggiunto: distruggi subito le spade nemiche associate all'enemy
            auto enemySwords = m_entityManager.getEntities("enemySword");
            for (auto& sword : enemySwords) {
                if (sword->has<CState>()) {
                    auto& swordState = sword->get<CState>();
                    if (swordState.state == std::to_string(enemy->id())) {
                        sword->destroy();
                    }
                }
            }

            if (enemyAI.knockbackTimer > 0.f) {
                enemyTrans.velocity.x *= KNOCKBACK_DECAY_FACTOR;
                enemyTrans.pos += enemyTrans.velocity * deltaTime;
                enemyAI.knockbackTimer -= deltaTime;
                continue;
            } else {
                enemyAI.knockbackTimer = 0.f;
                enemyAI.enemyState = EnemyState::Follow; // oppure "idle"
            }
        }

        // Gravità / Controllo a terra
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
            float grav = enemy->has<CGravity>() ? enemy->get<CGravity>().gravity : DEFAULT_GRAVITY;
            enemyTrans.velocity.y += grav * deltaTime;
            enemyTrans.velocity.y = std::clamp(enemyTrans.velocity.y, -MAX_FALL_SPEED, MAX_FALL_SPEED);
        } else {
            enemyTrans.velocity.y = 0.f;
        }

        // Calcolo della distanza / Line of Sight
        float dx = playerTrans.pos.x - enemyTrans.pos.x;
        float dy = playerTrans.pos.y - enemyTrans.pos.y;
        float distance = std::sqrt(dx * dx + dy * dy);

        // Controllo: se il giocatore è praticamente sulla stessa X e sopra il nemico, passa a Idle.
        if (std::fabs(dx) < 1.0f && playerTrans.pos.y < enemyTrans.pos.y) {
            enemyAI.enemyState = EnemyState::Idle;
            enemyTrans.velocity.x = 0.f;
            continue; // Salta l'update per questo nemico
        }

        bool canSeePlayer  = checkLineOfSight(enemyTrans.pos, playerTrans.pos, m_entityManager);
        bool playerVisible = (distance < PLAYER_VISIBLE_DISTANCE) || canSeePlayer;

        bool shouldFollow = false;
        switch (enemyAI.enemyBehavior) {
            case EnemyBehavior::FollowOne:
                if (playerVisible) shouldFollow = true;
                break;
            case EnemyBehavior::FollowTwo:
                if (playerVisible && enemyAI.enemyState != EnemyState::Attack) {
                    enemyAI.enemyState = EnemyState::Follow;
                    shouldFollow = true;
                } else if (enemyAI.enemyState == EnemyState::Follow) {
                    shouldFollow = true;
                }
                break;
        }

        // Gestione del cooldown per l'attacco
        if (enemyAI.attackCooldown > 0.f) {
            enemyAI.attackCooldown -= deltaTime;
            if (enemyAI.attackCooldown < 0.f) {
                enemyAI.attackCooldown = 0.f;
            }
        }

        // Logica d'attacco
        // Prima di controllare l'attacco
        float currentAttackRange = ATTACK_RANGE;
        if (enemyAI.enemyType == EnemyType::Emperor) {
            currentAttackRange = EMPEROR_ATTACK_RANGE; // Valore più grande
        }

        // Logica d'attacco
        if (shouldFollow && distance < currentAttackRange 
            && enemyAI.attackCooldown <= 0.f 
            && enemyAI.enemyState != EnemyState::Attack)
        {
            enemyAI.enemyState   = EnemyState::Attack;
            enemyAI.attackTimer  = ATTACK_TIMER_DEFAULT;
            enemyAI.swordSpawned = false;

            std::cout << "[DEBUG] Enemy " << enemy->id() 
                    << " entering Attack state. (cooldown=" 
                    << enemyAI.attackCooldown << ")\n";
        }
        else if (shouldFollow && enemyAI.enemyState != EnemyState::Attack) {
            enemyAI.enemyState = EnemyState::Follow;
        }
        else {
            if (!playerVisible) {
                if (enemyAI.enemyState == EnemyState::Follow 
                    || enemyAI.enemyState == EnemyState::Attack)
                {
                    enemyAI.enemyState        = EnemyState::Recognition;
                    enemyAI.recognitionTimer  = 0.f;
                    enemyAI.lastSeenPlayerPos = playerTrans.pos;
                } else {
                    enemyAI.enemyState = EnemyState::Idle;
                }
            }
        }

        // Aggiornamento durante l'attacco
        if (enemyAI.enemyState == EnemyState::Attack) {
            std::cout << "[DEBUG] Enemy " << enemy->id() 
                      << " AttackTimer: " << enemyAI.attackTimer << "\n";
            enemyTrans.velocity.x = 0.f;
            enemyAI.attackTimer -= deltaTime;

            // Spawn della spada nemica
            if (!enemyAI.swordSpawned && enemyAI.attackTimer <= SWORD_SPAWN_THRESHOLD) {
                m_spawner->spawnEnemySword(enemy);
                enemyAI.swordSpawned = true;
                std::cout << "[DEBUG] Enemy " << enemy->id() 
                          << " spawning sword at AttackTimer: " 
                          << enemyAI.attackTimer << "\n";
            }

            if (enemyAI.attackTimer <= 0.f) {
                enemyAI.attackCooldown = ATTACK_COOLDOWN;
                enemyAI.enemyState     = EnemyState::Follow;
                enemyAI.attackTimer    = 0.f;

                std::cout << "[DEBUG] Enemy " << enemy->id() 
                          << " Attack finished. Setting cooldown=" 
                          << enemyAI.attackCooldown << "\n";
            }
        }

        // Stato Follow
        if (enemyAI.enemyState == EnemyState::Follow) {
            if (dx > 0.f) {
                enemyTrans.velocity.x = FOLLOW_MOVE_SPEED;
                enemyAI.facingDirection = 1.f;
            } else if (dx < 0.f) {
                enemyTrans.velocity.x = -FOLLOW_MOVE_SPEED;
                enemyAI.facingDirection = -1.f;
            } else {
                enemyTrans.velocity.x = 0.f;
            }

            std::string runAnimName;
            switch (enemyAI.enemyType) {
                case EnemyType::Fast:   runAnimName = "EnemyFast_Run";   break;
                case EnemyType::Strong: runAnimName = "EnemyStrong_Run"; break;
                case EnemyType::Elite:  runAnimName = "EnemyElite_Run";  break;
                case EnemyType::Normal: runAnimName = "EnemyNormal_Run"; break;
                case EnemyType::Emperor: runAnimName = "Emperor_Run"; break;
            }
            if (anim.animation.getName() != runAnimName) {
                const Animation& runAnim = m_game.assets().getAnimation(runAnimName);
                anim.animation = runAnim;
                anim.animation.reset();
            }
        }

        // Stato Recognition
        if (enemyAI.enemyState == EnemyState::Recognition) {
            float recognitionMoveSpeed = RECOGNITION_MOVE_SPEED;
            float distanceToLastSeen = std::sqrt(
                std::pow(enemyAI.lastSeenPlayerPos.x - enemyTrans.pos.x, 2) +
                std::pow(enemyAI.lastSeenPlayerPos.y - enemyTrans.pos.y, 2)
            );

            if (distanceToLastSeen > RECOGNITION_DISTANCE_THRESHOLD) {
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

        // Stato Patrol
        if (enemyAI.enemyState == EnemyState::Patrol) {
            if (enemyAI.patrolPoints.empty()) {
                enemyAI.enemyState = EnemyState::Idle;
            } else {
                Vec2<float> target = enemyAI.patrolPoints[enemyAI.currentPatrolIndex];
                float dxPatrol = target.x - enemyTrans.pos.x;
                float dyPatrol = target.y - enemyTrans.pos.y;
                float patrolDist = std::sqrt(dxPatrol * dxPatrol + dyPatrol * dyPatrol);

                if (patrolDist < PATROL_DISTANCE_THRESHOLD) {
                    enemyTrans.velocity.x = 0.f;
                    anim.animation.reset();
                    enemyAI.patrolWaitTime += deltaTime;
                    if (enemyAI.patrolWaitTime >= PATROL_WAIT_DURATION) {
                        enemyAI.patrolWaitTime = 0.f;
                        enemyAI.currentPatrolIndex =
                            (enemyAI.currentPatrolIndex + 1) % enemyAI.patrolPoints.size();
                    }
                } else {
                    enemyAI.patrolWaitTime = 0.f;
                    if (dxPatrol > 0.f) {
                        enemyTrans.velocity.x = PATROL_SPEED;
                        enemyAI.facingDirection = 1.f;
                        anim.animation.reset();
                    } else if (dxPatrol < 0.f) {
                        enemyTrans.velocity.x = -PATROL_SPEED;
                        enemyAI.facingDirection = -1.f;
                        anim.animation.reset();
                    } else {
                        enemyTrans.velocity.x = 0.f;
                        anim.animation.reset();
                    }
                }
            }
        }

        // Stato Idle
        if (enemyAI.enemyState == EnemyState::Idle) {
            enemyTrans.velocity.x = 0.f;
            anim.animation.reset();
        }

        // Aggiornamento della posizione
        enemyTrans.pos.x += enemyTrans.velocity.x * deltaTime;
        enemyTrans.pos.y += enemyTrans.velocity.y * deltaTime;

        // Gestione del flip dello sprite
        if (enemyAI.facingDirection < 0.f) {
            flipSpriteLeft(anim.animation.getMutableSprite());
        } else {
            flipSpriteRight(anim.animation.getMutableSprite());
        }
    }
}
