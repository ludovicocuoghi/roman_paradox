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
        state.onGround = false; // Reset; will be set if a ground collision is detected

        // Iterate over all tiles.
        for (auto& tile : m_entityManager.getEntities("tile")) {
            // Ensure tile has the necessary components
            if (!tile->has<CTransform>() || !tile->has<CBoundingBox>() || !tile->has<CAnimation>())
                continue;

            auto& tileTransform = tile->get<CTransform>();
            auto& tileBB        = tile->get<CBoundingBox>();
            auto& tileAnim      = tile->get<CAnimation>().animation;
            sf::FloatRect tRect = tileBB.getRect(tileTransform.pos);

            // If there is no intersection, skip to the next tile.
            if (!pRect.intersects(tRect))
                continue;

            // Calculate horizontal and vertical overlaps.
            float overlapX = std::min(pRect.left + pRect.width, tRect.left + tRect.width) -
                             std::max(pRect.left, tRect.left);
            float overlapY = std::min(pRect.top + pRect.height, tRect.top + tRect.height) -
                             std::max(pRect.top, tRect.top);

            if (overlapX < overlapY) {
                // Horizontal collision: adjust horizontal position.
                if (transform.pos.x < tileTransform.pos.x)
                    transform.pos.x -= overlapX;
                else
                    transform.pos.x += overlapX;
                velocity.x = 0.f;
            }
            else {
                // Vertical collision:
                if (transform.pos.y < tileTransform.pos.y) {
                    // Player is above the tile -> landing.
                    transform.pos.y -= overlapY;
                    velocity.y = 0.f;
                    state.onGround = true;
                }
                else {
                    // Player is hitting the tile from below (moving upward)
                    if (velocity.y < 0) {
                        transform.pos.y += overlapY;
                        velocity.y = 0.f;
                        std::string animName = tileAnim.getName();

                        // Only break Box1 and Box2 from below.
                        if (animName == "Box1" || animName == "Box2") {
                            // Break the box: create fragments and spawn an item.
                            m_spawner->createBlockFragments(tileTransform.pos, animName);
                            m_spawner->spawnItem(tileTransform.pos, animName);
                            tile->destroy();
                            std::cout << "[DEBUG] " << animName << " broken from below!\n";
                        }
                        // Handle question/treasure box: set inactive, change animation, and spawn item.
                        else if (animName == "TreasureBoxAnim" || animName == "QuestionAnim") {
                            auto& tileState = tile->get<CState>();
                            if (tileState.state == "inactive") {
                                tileState.state = "activated";
                                if (m_game.assets().hasAnimation("TreasureBoxHit")) {
                                    tile->get<CAnimation>().animation = m_game.assets().getAnimation("TreasureBoxHit");
                                    tile->get<CAnimation>().repeat = false;
                                    std::cout << "[DEBUG] Treasure box hit from below.\n";
                                }
                                m_spawner->spawnItem(tileTransform.pos, "TreasureBoxAnim");
                            }
                        }
                        // If the tile is a Brick, do nothing when hit from below.
                    }
                }
            }

            // Update the player's rectangle after collision resolution.
            pRect = playerBB.getRect(transform.pos);
        }

        // Update player's state based on collisions and velocity (if not attacking).
        if (state.state != "attack") {
            if (state.onGround)
                state.state = (std::abs(velocity.x) > 1.f) ? "run" : "idle";
            else
                state.state = "air";
        }
        if (state.onGround && !wasOnGround)
            std::cout << "[DEBUG] Player landed! Jump reset.\n";
    }
}

// ----------------------------------
// 👹 ENEMY - TILE COLLISIONS
// (Remains unchanged)
void CollisionSystem::handleEnemyTileCollisions() {
    for (auto& enemy : m_entityManager.getEntities("enemy")) {
        auto& transform = enemy->get<CTransform>();
        auto& enemyBB   = enemy->get<CBoundingBox>();
        sf::FloatRect eRect = enemyBB.getRect(transform.pos);

        bool onGround = false;
        float minOverlapY = std::numeric_limits<float>::max();

        for (auto& tile : m_entityManager.getEntities("tile")) {
            auto& tileTransform = tile->get<CTransform>();
            auto& tileBB        = tile->get<CBoundingBox>();
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
            }
            else {
                if (transform.velocity.y > 0) {
                    if ((eRect.top + eRect.height) > tRect.top && overlapY < minOverlapY) {
                        minOverlapY = overlapY;
                        transform.pos.y -= overlapY;
                        transform.velocity.y = 0.f;
                        onGround = true;
                    }
                }
                else if (transform.velocity.y < 0) {
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
// (Remains unchanged)
void CollisionSystem::handlePlayerEnemyCollisions() {
    for (auto& enemy : m_entityManager.getEntities("enemy")) {
        auto& enemyTrans = enemy->get<CTransform>();
        auto& enemyBB    = enemy->get<CBoundingBox>();
        sf::FloatRect enemyRect = enemyBB.getRect(enemyTrans.pos);

        for (auto& player : m_entityManager.getEntities("player")) {
            auto& pTrans = player->get<CTransform>();
            auto& pBB    = player->get<CBoundingBox>();
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
// 🗡️ SWORD - ENEMY & TILE COLLISIONS
// ----------------------------------
void CollisionSystem::handleSwordCollisions() {
    for (auto& sword : m_entityManager.getEntities("sword")) {
        if (!sword->has<CTransform>() || !sword->has<CBoundingBox>())
            continue;

        auto& swTrans = sword->get<CTransform>();
        auto& swBB    = sword->get<CBoundingBox>();
        sf::FloatRect swordRect = swBB.getRect(swTrans.pos);

        // 1) SWORD <-> TILE (e.g., to break Box1 and Box2; Brick should never break)
        for (auto& tile : m_entityManager.getEntities("tile")) {
            if (!tile->has<CTransform>() || !tile->has<CBoundingBox>() || !tile->has<CAnimation>())
                continue;

            auto& tileTransform = tile->get<CTransform>();
            auto& tileBB        = tile->get<CBoundingBox>();
            auto& tileAnim      = tile->get<CAnimation>().animation;
            sf::FloatRect tileRect = tileBB.getRect(tileTransform.pos);

            if (swordRect.intersects(tileRect)) {
                std::string animName = tileAnim.getName();
                // For sword collisions, only break Box1 and Box2 (and optionally TreasureBox/Question block)
                if (animName == "Box1" || animName == "Box2") {
                    m_spawner->createBlockFragments(tileTransform.pos, animName);
                    m_spawner->spawnItem(tileTransform.pos, animName);
                    tile->destroy();
                    std::cout << "[DEBUG] " << animName << " broken by sword!\n";
                }
                // If you want the treasure box to also be broken by the sword, you can add:
                else if (animName == "TreasureBoxAnim" || animName == "QuestionAnim") {
                    auto& tileState = tile->get<CState>();
                    if (tileState.state == "inactive") {
                        tileState.state = "activated";
                        if (m_game.assets().hasAnimation("TreasureBoxHit")) {
                            tile->get<CAnimation>().animation = m_game.assets().getAnimation("TreasureBoxHit");
                            tile->get<CAnimation>().repeat = false;
                            std::cout << "[DEBUG] Treasure box hit by sword.\n";
                        }
                        m_spawner->spawnItem(tileTransform.pos, "TreasureBoxAnim");
                    }
                }
            }
        }

        // 2) SWORD <-> ENEMY
        for (auto& enemy : m_entityManager.getEntities("enemy")) {
            if (!enemy->has<CTransform>() || !enemy->has<CBoundingBox>())
                continue;

            auto& enemyTrans = enemy->get<CTransform>();
            auto& enemyBB    = enemy->get<CBoundingBox>();
            sf::FloatRect enemyRect = enemyBB.getRect(enemyTrans.pos);

            if (swordRect.intersects(enemyRect)) {
                std::cout << "[DEBUG] Enemy took a hit from the sword!\n";
                // Here you can reduce health, apply knockback, or destroy the enemy.
                // Example:
                Physics::Forces::ApplyKnockback(enemy, hitDirection, 1.0f);
                enemy->get<CHealth>().takeDamage(10);
            }
        }
    }
}
