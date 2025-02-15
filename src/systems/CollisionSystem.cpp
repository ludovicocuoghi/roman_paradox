#include "CollisionSystem.h"
#include "Physics.hpp"
#include <iostream>
#include <algorithm>
#include <limits>
#include <random>
#include <SFML/Graphics.hpp>

CollisionSystem::CollisionSystem(EntityManager& entityManager, GameEngine& game, Spawner* spawner)
    : m_entityManager(entityManager), m_game(game), m_spawner(spawner) {}

void CollisionSystem::updateCollisions() {
    handlePlayerTileCollisions();
    handleEnemyTileCollisions();
    handlePlayerEnemyCollisions();
    handleSwordCollisions();
    handlePlayerCollectibleCollisions();
}

// Player - Tile
void CollisionSystem::handlePlayerTileCollisions() {
    for (auto& player : m_entityManager.getEntities("player")) {
        auto& transform = player->get<CTransform>();
        auto& velocity  = transform.velocity;
        auto& playerBB  = player->get<CBoundingBox>();
        auto& state     = player->get<CState>();

        sf::FloatRect pRect = playerBB.getRect(transform.pos);
        state.onGround = false;

        for (auto& tile : m_entityManager.getEntities("tile")) {
            if (!tile->has<CTransform>() || !tile->has<CBoundingBox>() || !tile->has<CAnimation>())
                continue;

            auto& tileTransform = tile->get<CTransform>();
            auto& tileBB        = tile->get<CBoundingBox>();
            auto& tileAnim      = tile->get<CAnimation>().animation;
            sf::FloatRect tRect = tileBB.getRect(tileTransform.pos);

            if (!pRect.intersects(tRect))
                continue;

            float overlapX = std::min(pRect.left + pRect.width, tRect.left + tRect.width)
                           - std::max(pRect.left, tRect.left);
            float overlapY = std::min(pRect.top + pRect.height, tRect.top + tRect.height)
                           - std::max(pRect.top, tRect.top);

            if (overlapX < overlapY) {
                // Collisione orizzontale
                if (transform.pos.x < tileTransform.pos.x)
                    transform.pos.x -= overlapX;
                else
                    transform.pos.x += overlapX;
                velocity.x = 0.f;
            } else {
                // Collisione verticale
                if (transform.pos.y < tileTransform.pos.y) {
                    transform.pos.y -= overlapY;
                    velocity.y = 0.f;
                    state.onGround = true;
                } else {
                    if (velocity.y < 0) {
                        transform.pos.y += overlapY;
                        velocity.y = 0.f;
                        std::string animName = tileAnim.getName();

                        // Breakable from below
                        if (animName == "Box1" || animName == "Box2") {
                            m_spawner->createBlockFragments(tileTransform.pos, animName);
                            m_spawner->spawnItem(tileTransform.pos, animName);
                            tile->destroy();
                            std::cout << "[DEBUG] " << animName << " broken from below!\n";
                        } else if (animName == "TreasureBoxAnim" || animName == "QuestionAnim") {
                            auto& tileState = tile->get<CState>();
                            if (tileState.state == "inactive") {
                                tileState.state = "activated";
                                if (m_game.assets().hasAnimation("TreasureBoxHit")) {
                                    tile->get<CAnimation>().animation = m_game.assets().getAnimation("TreasureBoxHit");
                                    tile->get<CAnimation>().repeat = false;
                                    std::cout << "[DEBUG] Treasure/Question box hit from below.\n";
                                }
                                m_spawner->spawnItem(tileTransform.pos, "TreasureBoxAnim");
                            }
                        }
                    }
                }
            }
            pRect = playerBB.getRect(transform.pos);
        }
        if (state.state != "attack") {
            if (state.onGround)
                state.state = (std::abs(velocity.x) > PLAYER_RUN_VELOCITY_THRESHOLD) ? "run" : "idle";
            else
                state.state = "air";
        }
    }
}

// Enemy - Tile
void CollisionSystem::handleEnemyTileCollisions() {
    for (auto& enemy : m_entityManager.getEntities("enemy")) {
        auto& transform = enemy->get<CTransform>();
        auto& enemyBB   = enemy->get<CBoundingBox>();
        sf::FloatRect eRect = enemyBB.getRect(transform.pos);

        float minOverlapY = std::numeric_limits<float>::max();

        for (auto& tile : m_entityManager.getEntities("tile")) {
            if (!tile->has<CTransform>() || !tile->has<CBoundingBox>())
                continue;

            auto& tileTransform = tile->get<CTransform>();
            auto& tileBB        = tile->get<CBoundingBox>();
            sf::FloatRect tRect = tileBB.getRect(tileTransform.pos);

            if (!eRect.intersects(tRect))
                continue;

            float overlapX = std::min(eRect.left + eRect.width, tRect.left + tRect.width)
                           - std::max(eRect.left, tRect.left);
            float overlapY = std::min(eRect.top + eRect.height, tRect.top + tRect.height)
                           - std::max(eRect.top, tRect.top);

            if (overlapX < overlapY) {
                // Collisione orizzontale
                transform.pos.x += (transform.pos.x < tileTransform.pos.x) ? -overlapX : overlapX;
                transform.velocity.x = 0.f;
            } else {
                // Collisione verticale
                if (transform.velocity.y > 0) {
                    if ((eRect.top + eRect.height) > tRect.top && overlapY < minOverlapY) {
                        minOverlapY = overlapY;
                        transform.pos.y -= overlapY;
                        transform.velocity.y = 0.f;
                    }
                } else if (transform.velocity.y < 0) {
                    if (eRect.top < (tRect.top + tRect.height) && overlapY < minOverlapY) {
                        minOverlapY = overlapY;
                        transform.pos.y += overlapY;
                        transform.velocity.y = 0.f;
                    }
                }
            }
            eRect = enemyBB.getRect(transform.pos);
        }
    }
}

// Player - Enemy
void CollisionSystem::handlePlayerEnemyCollisions() {
    for (auto& enemy : m_entityManager.getEntities("enemy")) {
        if (enemy->has<CEnemyAI>()) {
            auto& enemyAI = enemy->get<CEnemyAI>();
            if (enemyAI.enemyState == EnemyState::Attack)
                continue;
        }

        auto& enemyTrans = enemy->get<CTransform>();
        auto& enemyBB    = enemy->get<CBoundingBox>();
        sf::FloatRect enemyRect = enemyBB.getRect(enemyTrans.pos);

        for (auto& player : m_entityManager.getEntities("player")) {
            auto& pTrans = player->get<CTransform>();
            auto& pBB    = player->get<CBoundingBox>();
            sf::FloatRect playerRect = pBB.getRect(pTrans.pos);

            if (enemyRect.intersects(playerRect)) {
                float overlapX = std::min(enemyRect.left + enemyRect.width, playerRect.left + playerRect.width)
                               - std::max(enemyRect.left, playerRect.left);
                float overlapY = std::min(enemyRect.top + enemyRect.height, playerRect.top + playerRect.height)
                               - std::max(enemyRect.top, playerRect.top);

                // Separazione semplice usando il fattore definito
                if (overlapX < overlapY) {
                    if (enemyTrans.pos.x < pTrans.pos.x) {
                        enemyTrans.pos.x -= overlapX * COLLISION_SEPARATION_FACTOR;
                        pTrans.pos.x     += overlapX * COLLISION_SEPARATION_FACTOR;
                    } else {
                        enemyTrans.pos.x += overlapX * COLLISION_SEPARATION_FACTOR;
                        pTrans.pos.x     -= overlapX * COLLISION_SEPARATION_FACTOR;
                    }
                } else {
                    if (enemyTrans.pos.y < pTrans.pos.y) {
                        enemyTrans.pos.y -= overlapY * COLLISION_SEPARATION_FACTOR;
                        pTrans.pos.y     += overlapY * COLLISION_SEPARATION_FACTOR;
                    } else {
                        enemyTrans.pos.y += overlapY * COLLISION_SEPARATION_FACTOR;
                        pTrans.pos.y     -= overlapY * COLLISION_SEPARATION_FACTOR;
                    }
                }
            }
        }
    }
}

void CollisionSystem::handleSwordCollisions() {
    // Player sword
    for (auto& sword : m_entityManager.getEntities("sword")) {
        if (!sword->has<CTransform>() || !sword->has<CBoundingBox>())
            continue;

        auto& swTrans = sword->get<CTransform>();
        auto& swBB    = sword->get<CBoundingBox>();
        sf::FloatRect swordRect = swBB.getRect(swTrans.pos);

        // --- Player Sword vs Tile ---
        for (auto& tile : m_entityManager.getEntities("tile")) {
            if (!tile->has<CTransform>() || !tile->has<CBoundingBox>() || !tile->has<CAnimation>())
                continue;

            auto& tileTransform = tile->get<CTransform>();
            auto& tileBB        = tile->get<CBoundingBox>();
            auto& tileAnim      = tile->get<CAnimation>().animation;
            sf::FloatRect tileRect = tileBB.getRect(tileTransform.pos);

            if (swordRect.intersects(tileRect)) {
                std::string animName = tileAnim.getName();
                if (animName == "Box1" || animName == "Box2") {
                    m_spawner->createBlockFragments(tileTransform.pos, animName);
                    m_spawner->spawnItem(tileTransform.pos, animName);
                    tile->destroy();
                    std::cout << "[DEBUG] " << animName << " broken by player's sword!\n";
                }
            }
        }

        // --- Player Sword vs Enemy ---
        for (auto& enemy : m_entityManager.getEntities("enemy")) {
            if (!enemy->has<CTransform>() || !enemy->has<CBoundingBox>())
                continue;

            auto& enemyTrans = enemy->get<CTransform>();
            auto& enemyBB    = enemy->get<CBoundingBox>();
            sf::FloatRect enemyRect = enemyBB.getRect(enemyTrans.pos);

            if (swordRect.intersects(enemyRect)) {
                // Applica il danno
                if (enemy->has<CHealth>()) {
                    auto& health = enemy->get<CHealth>();
                    health.takeDamage(PLAYER_SWORD_DAMAGE);
                    health.invulnerabilityTimer = PLAYER_SWORD_INVULNERABILITY_TIME;
                    if (!health.isAlive()) {
                        enemy->destroy();
                        continue;
                    }
                }

                // Lancia la "moneta" (roll da 1 a 50 e trigger se uguale a 1)
                static std::random_device rd;
                static std::mt19937 gen(rd());
                std::uniform_int_distribution<int> dist(PLAYER_SWORD_KNOCKBACK_ROLL_MIN, PLAYER_SWORD_KNOCKBACK_ROLL_MAX);
                int roll = dist(gen);
                std::cout << "[DEBUG] roll = " << roll << "\n";

                if (roll == PLAYER_SWORD_KNOCKBACK_ROLL_TRIGGER) {
                    float attackDirection = (swTrans.pos.x < enemyTrans.pos.x) ? 1.f : -1.f;
                    Vec2<float> hitDirection = { attackDirection, PLAYER_SWORD_KNOCKBACK_Y_DIRECTION };

                    // Applica il knockback
                    Physics::Forces::ApplyKnockback(enemy, hitDirection, PLAYER_SWORD_KNOCKBACK_STRENGTH);

                    // Aggiorna lo stato dell'AI, se presente
                    if (enemy->has<CEnemyAI>()) {
                        auto& enemyAI = enemy->get<CEnemyAI>();
                        enemyAI.enemyState = EnemyState::Knockback;
                        enemyAI.knockbackTimer = ENEMY_KNOCKBACK_AI_TIMER;
                    }
                    // Aggiorna lo stato generico
                    if (enemy->has<CState>()) {
                        auto& state = enemy->get<CState>();
                        state.state = "knockback";
                        state.knockbackTimer = ENEMY_KNOCKBACK_STATE_TIMER;
                    }
                    std::cout << "[DEBUG] Knockback applicato al nemico!\n";
                }
            }
        }
    }

    // Enemy sword
    for (auto& enemySword : m_entityManager.getEntities("enemySword")) {
        if (!enemySword->has<CTransform>() || !enemySword->has<CBoundingBox>())
            continue;

        auto& swTrans = enemySword->get<CTransform>();
        auto& swBB    = enemySword->get<CBoundingBox>();
        auto& swLS    = enemySword->get<CLifeSpan>();
        std::cout << "[DEBUG] Enemy sword lifespan: " << swLS.remainingTime << "\n";

        sf::FloatRect swordRect = swBB.getRect(swTrans.pos);

        // Enemy sword vs player
        for (auto& player : m_entityManager.getEntities("player")) {
            if (!player->has<CTransform>() || !player->has<CBoundingBox>())
                continue;

            auto& pTrans = player->get<CTransform>();
            auto& pBB    = player->get<CBoundingBox>();
            sf::FloatRect playerRect = pBB.getRect(pTrans.pos);

            if (!swordRect.intersects(playerRect))
                continue;

            // Se il player è già in knockback, salto
            if (player->has<CState>()) {
                auto& playerState = player->get<CState>();
                if (playerState.state == "knockback")
                    continue;
            }

            // Knockback del player
            float attackDirection = (swTrans.pos.x < pTrans.pos.x) ? 1.f : -1.f;
            Vec2<float> hitDirection = { attackDirection, 0.f };
            Physics::Forces::ApplyKnockback(player, hitDirection, ENEMY_SWORD_KNOCKBACK_STRENGTH);

            // Danno al player
            if (player->has<CHealth>()) {
                auto& health = player->get<CHealth>();
                health.takeDamage(ENEMY_SWORD_DAMAGE);
                health.invulnerabilityTimer = PLAYER_HIT_INVULNERABILITY_TIME;
                std::cout << "[DEBUG] Player hit by enemy sword! Health: " << health.currentHealth << "\n";
            }
            if (player->has<CState>()) {
                player->get<CState>().state = "knockback";
                player->get<CState>().knockbackTimer = PLAYER_KNOCKBACK_TIMER;
            }
            break;
        }

        // (Optional) Enemy sword vs tile
        for (auto& tile : m_entityManager.getEntities("tile")) {
            if (!tile->has<CTransform>() || !tile->has<CBoundingBox>() || !tile->has<CAnimation>())
                continue;
            auto& tileTransform = tile->get<CTransform>();
            auto& tileBB        = tile->get<CBoundingBox>();
            auto& tileAnim      = tile->get<CAnimation>().animation;
            sf::FloatRect tileRect = tileBB.getRect(tileTransform.pos);

            if (swordRect.intersects(tileRect)) {
                std::string animName = tileAnim.getName();
                if (animName == "Box1" || animName == "Box2") {
                    m_spawner->createBlockFragments(tileTransform.pos, animName);
                    m_spawner->spawnItem(tileTransform.pos, animName);
                    tile->destroy();
                    std::cout << "[DEBUG] " << animName << " broken by enemy sword!\n";
                }
            }
        }
    }
}

// Player - Collectibles
void CollisionSystem::handlePlayerCollectibleCollisions() {
    for (auto& player : m_entityManager.getEntities("player")) {
        if (!player->has<CTransform>() || !player->has<CBoundingBox>())
            continue;
        auto& pTrans = player->get<CTransform>();
        auto& pBB    = player->get<CBoundingBox>();
        sf::FloatRect pRect = pBB.getRect(pTrans.pos);

        for (auto& item : m_entityManager.getEntities("collectable")) {
            if (!item->has<CTransform>() || !item->has<CBoundingBox>() || !item->has<CState>())
                continue;
            auto& iTrans = item->get<CTransform>();
            auto& iBB    = item->get<CBoundingBox>();
            sf::FloatRect iRect = iBB.getRect(iTrans.pos);

            if (pRect.intersects(iRect)) {
                std::string itemType = item->get<CState>().state;
                if (itemType == "GrapeSmall") {
                    std::cout << "[DEBUG] Collected small grape (+" 
                              << COLLECTIBLE_SMALL_GRAPE_POINTS << " points)!\n";
                    // Incrementa il punteggio del player
                } else if (itemType == "GrapeBig") {
                    if (player->has<CHealth>()) {
                        player->get<CHealth>().heal(COLLECTIBLE_BIG_GRAPE_HEAL);
                        std::cout << "[DEBUG] Collected big grape (+" 
                                  << COLLECTIBLE_BIG_GRAPE_HEAL << " health)!\n";
                    }
                }
                item->destroy();
            }
        }
    }
}
