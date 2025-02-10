#include "CollisionSystem.h"
#include "Physics.hpp"
#include <iostream>
#include <algorithm>
#include <limits>
#include <SFML/Graphics.hpp>

CollisionSystem::CollisionSystem(EntityManager& entityManager, GameEngine& game, Spawner* spawner)
    : m_entityManager(entityManager), m_game(game), m_spawner(spawner) {}

void CollisionSystem::updateCollisions() {
    handlePlayerTileCollisions();
    handleEnemyTileCollisions();
    handlePlayerEnemyCollisions();
    handleSwordCollisions();
}

// ----------------------------------
// 🏃 PLAYER - TILE COLLISIONS
// ----------------------------------
void CollisionSystem::handlePlayerTileCollisions() {
    for (auto& player : m_entityManager.getEntities("player")) {
        auto& transform = player->get<CTransform>();
        auto& velocity  = transform.velocity;
        auto& playerBB  = player->get<CBoundingBox>();
        auto& state     = player->get<CState>();

        // Calculate the player's rectangle based on its current position.
        sf::FloatRect pRect = playerBB.getRect(transform.pos);
        bool wasOnGround = state.onGround;
        state.onGround = false;  // Reset; will be set if a ground collision is detected.
        float minOverlapY = std::numeric_limits<float>::max();

        // Iterate over all tiles.
        for (auto& tile : m_entityManager.getEntities("tile")) {
            auto& tileTransform = tile->get<CTransform>();
            auto& tileBB = tile->get<CBoundingBox>();
            sf::FloatRect tRect = tileBB.getRect(tileTransform.pos);

            // If there is no intersection, skip this tile.
            if (!pRect.intersects(tRect))
                continue;

            // Calculate horizontal and vertical overlaps.
            float overlapX = std::min(pRect.left + pRect.width, tRect.left + tRect.width) -
                             std::max(pRect.left, tRect.left);
            float overlapY = std::min(pRect.top + pRect.height, tRect.top + tRect.height) -
                             std::max(pRect.top, tRect.top);

            if (overlapX < overlapY) {
                // Horizontal collision: adjust the horizontal position.
                transform.pos.x += (transform.pos.x < tileTransform.pos.x) ? -overlapX : overlapX;
                velocity.x = 0.f;
            } else {
                if (velocity.y > 0) {
                    // Player is falling: if the bottom of the player exceeds the tile's top, resolve landing.
                    if ((pRect.top + pRect.height) > tRect.top && overlapY < minOverlapY) {
                        minOverlapY = overlapY;
                        transform.pos.y -= overlapY;  // Move the player upward.
                        velocity.y = 0.f;
                        state.onGround = true;
                    }
                } else if (velocity.y < 0) {
                    // Player is moving upward: if the top of the player's rectangle is below
                    // the bottom edge of the tile, then a head collision occurred.
                    if (pRect.top > (tRect.top + tRect.height) && overlapY < minOverlapY) {
                        minOverlapY = overlapY;
                        // Check if this tile is a treasure box that hasn't been triggered.
                        if (tile->has<CAnimation>() &&
                            tile->get<CAnimation>().animation.getName() == "TreasureBoxAnim") {
                            if (!tile->has<CState>() || tile->get<CState>().state != "inactive") {
                                // Trigger the treasure box event:
                                // 1. Change the animation to "TreasureBoxHit".
                                tile->get<CAnimation>().animation = m_game.assets().getAnimation("TreasureBoxHit");
                                // 2. Mark the box as inactive.
                                if (tile->has<CState>())
                                    tile->get<CState>().state = "inactive";
                                else
                                    tile->add<CState>("inactive");
                                // 3. Spawn an item at the tile's position.
                                m_spawner->spawnItem(tileTransform.pos, "TreasureItem");
                            }
                        }
                        transform.pos.y += overlapY;  // Move the player downward to resolve the collision.
                        velocity.y = 0.f;
                    }
                }
            }
        }

        if (state.onGround && !wasOnGround) {
            std::cout << "[DEBUG] Player landed! Jump reset.\n";
            state.state = "idle";
        }
    }
}
// ----------------------------------
// 👹 ENEMY - TILE COLLISIONS
// ----------------------------------
void CollisionSystem::handleEnemyTileCollisions() {
    for (auto& enemy : m_entityManager.getEntities("enemy")) {
        auto& transform = enemy->get<CTransform>();
        auto& enemyBB = enemy->get<CBoundingBox>();
        sf::FloatRect eRect = enemyBB.getRect(transform.pos);

        bool onGround = false;
        float minOverlapY = std::numeric_limits<float>::max();

        for (auto& tile : m_entityManager.getEntities("tile")) {
            auto& tileTransform = tile->get<CTransform>();
            auto& tileBB = tile->get<CBoundingBox>();
            sf::FloatRect tRect = tileBB.getRect(tileTransform.pos);

            if (!eRect.intersects(tRect))
                continue;

            float overlapX = std::min(eRect.left + eRect.width, tRect.left + tRect.width) -
                             std::max(eRect.left, tRect.left);
            float overlapY = std::min(eRect.top + eRect.height, tRect.top + tRect.height) -
                             std::max(eRect.top, tRect.top);

            if (overlapX < overlapY) {
                transform.pos.x += (transform.pos.x < tileTransform.pos.x) ? -overlapX : overlapX;
                transform.velocity.x = 0.f;
            } else {
                if (transform.velocity.y > 0) {
                    if ((eRect.top + eRect.height) > tRect.top && overlapY < minOverlapY) {
                        minOverlapY = overlapY;
                        transform.pos.y -= overlapY;
                        transform.velocity.y = 0.f;
                        onGround = true;
                    }
                } else if (transform.velocity.y < 0) {
                    if (eRect.top < (tRect.top + tRect.height) && overlapY < minOverlapY) {
                        minOverlapY = overlapY;
                        transform.pos.y += overlapY;
                        transform.velocity.y = 0.f;
                    }
                }
            }
        }

        if (onGround)
            std::cout << "[DEBUG] Enemy landed at: (" << transform.pos.x << ", " << transform.pos.y << ")\n";
    }
}

// ----------------------------------
// ⚔️ PLAYER - ENEMY COLLISIONS
// ----------------------------------
void CollisionSystem::handlePlayerEnemyCollisions() {
    for (auto& enemy : m_entityManager.getEntities("enemy")) {
        auto& enemyTrans = enemy->get<CTransform>();
        auto& enemyBB = enemy->get<CBoundingBox>();
        sf::FloatRect enemyRect = enemyBB.getRect(enemyTrans.pos);

        for (auto& player : m_entityManager.getEntities("player")) {
            auto& pTrans = player->get<CTransform>();
            auto& pBB = player->get<CBoundingBox>();
            sf::FloatRect playerRect = pBB.getRect(pTrans.pos);

            if (enemyRect.intersects(playerRect)) {
                auto& playerState = player->get<CState>();

                if (playerState.state == "knockback")
                    continue;

                player->get<CHealth>().takeDamage(1);
                Vec2<float> hitDirection = { (enemyTrans.pos.x < pTrans.pos.x) ? 1.f : -1.f, 0 };

                Physics::Forces::ApplyKnockback(player, hitDirection, 1.0f);
                playerState.state = "knockback";
                std::cout << "[DEBUG] Player hit by enemy! Knocked back hard.\n";
            }
        }
    }
}

// ----------------------------------
// 🗡️ SWORD - ENEMY COLLISIONS
// ----------------------------------
void CollisionSystem::handleSwordCollisions() {
    for (auto& sword : m_entityManager.getEntities("sword")) {
        auto& swTrans = sword->get<CTransform>();
        auto& swBB = sword->get<CBoundingBox>();
        sf::FloatRect swordRect = swBB.getRect(swTrans.pos);

        for (auto& enemy : m_entityManager.getEntities("enemy")) {
            auto& enemyTrans = enemy->get<CTransform>();
            auto& enemyBB = enemy->get<CBoundingBox>();
            auto& enemyState = enemy->get<CState>();
            sf::FloatRect enemyRect = enemyBB.getRect(enemyTrans.pos);

            if (swordRect.intersects(enemyRect)) {
                if (enemy->has<CHealth>()) {
                    auto& health = enemy->get<CHealth>();
                    if (health.invulnerabilityTimer > 0.f)
                        continue;
                }

                float attackDirection = (swTrans.pos.x < enemyTrans.pos.x) ? 1.f : -1.f;
                Vec2<float> hitDirection = { attackDirection, -0.5f };

                Physics::Forces::ApplyKnockback(enemy, hitDirection, 1.0f);
                enemyState.state = "knockback";

                if (enemy->has<CHealth>()) {
                    auto& health = enemy->get<CHealth>();
                    health.takeDamage(1);
                    health.invulnerabilityTimer = 0.5f;
                    std::cout << "[DEBUG] Enemy hit by sword! Knocked back. Health: " 
                              << health.currentHealth << "\n";
                }
            }
        }
    }
}
