#include "CollisionSystem.h"
#include "Physics.hpp"
#include <iostream>
#include <algorithm>
#include <limits>
#include <random>
#include <SFML/Graphics.hpp>

CollisionSystem::CollisionSystem(EntityManager& entityManager, GameEngine& game, Spawner* spawner, int& score)
    : m_entityManager(entityManager), m_game(game), m_spawner(spawner), m_score(score) {}

void CollisionSystem::updateCollisions() {
    if (m_score >=100) {
        for (auto& player : m_entityManager.getEntities("player")) {
            auto& health = player->get<CHealth>();
            health.heal(100);
        m_score -=100;
        }
    }
    handlePlayerTileCollisions();
    handleEnemyTileCollisions();
    handlePlayerEnemyCollisions();
    handleEnemyEnemyCollisions();
    handleSwordCollisions();
    handlePlayerCollectibleCollisions();
}

// Player - Tile
void CollisionSystem::handlePlayerTileCollisions() {
    for (auto& player : m_entityManager.getEntities("player")) {
        // Ensure we have the needed components
        if (!player->has<CTransform>() || !player->has<CBoundingBox>() || !player->has<CState>())
            continue;

        auto& transform = player->get<CTransform>();
        auto& velocity  = transform.velocity;
        auto& playerBB  = player->get<CBoundingBox>();
        auto& state     = player->get<CState>();

        // Get the bounding box for the player's current position
        sf::FloatRect pRect = playerBB.getRect(transform.pos);
        // Reset onGround each frame; we’ll set it to true if we land on a tile
        state.onGround = false;

        // Check collision with each tile
        for (auto& tile : m_entityManager.getEntities("tile")) {
            // Make sure the tile has the needed components
            if (!tile->has<CTransform>() || !tile->has<CBoundingBox>() || !tile->has<CAnimation>())
                continue;

            auto& tileTransform = tile->get<CTransform>();
            auto& tileBB        = tile->get<CBoundingBox>();
            auto& tileAnim      = tile->get<CAnimation>().animation;

            sf::FloatRect tRect = tileBB.getRect(tileTransform.pos);

            // If no intersection, skip
            if (!pRect.intersects(tRect))
                continue;

            std::string nextLevelPath = "";  // ✅ Store next level path instead of switching immediately

            // ✅ NEW: Check if this tile is a LevelDoor
            if (tile->has<CAnimation>() && (tile->get<CAnimation>().animation.getName() == "LevelDoor" || tile->get<CAnimation>().animation.getName() == "LevelDoorGold") ) {
                std::cout << "[DEBUG] Player entered LevelDoor. Scheduling level change...\n";
                m_game.scheduleLevelChange(m_game.getNextLevelPath());
                return;
            }
            
            // ✅ Move level transition **outside the loop** to avoid modifying entities while iterating
            if (!nextLevelPath.empty()) {
                std::cout << "[DEBUG] Transitioning to next level: " << nextLevelPath << std::endl;
            
                if (nextLevelPath == "./bin/levels/") {
                    std::cerr << "[ERROR] Invalid next level path (empty string)!\n";
                    return;
                }
            
                m_game.loadLevel(nextLevelPath);
                return;
            }
            
            // Calculate overlap on X and Y
            float overlapX = std::min(pRect.left + pRect.width, tRect.left + tRect.width)
                           - std::max(pRect.left, tRect.left);
            float overlapY = std::min(pRect.top + pRect.height, tRect.top + tRect.height)
                           - std::max(pRect.top, tRect.top);

            // Decide which axis to fix based on which overlap is smaller
            if (overlapX < overlapY) {
                // Fix X overlap
                if (transform.pos.x < tileTransform.pos.x) {
                    // Player is left of tile, push left
                    transform.pos.x -= overlapX;
                } else {
                    // Player is right of tile, push right
                    transform.pos.x += overlapX;
                }
                // Stop horizontal velocity
                velocity.x = 0.f;
            } else {
                // Fix Y overlap
                if (transform.pos.y < tileTransform.pos.y) {
                    // Player is above the tile
                    transform.pos.y -= overlapY;
                    velocity.y = 0.f;
                    state.onGround = true; // Landed on tile
                } else {
                    // Player hit the tile from below
                    if (velocity.y < 0) {
                        transform.pos.y += overlapY;
                        velocity.y = 0.f;

                        // Example: breakable or special tiles
                        std::string animName = tileAnim.getName();
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

            // Update pRect in case we moved the player
            pRect = playerBB.getRect(transform.pos);
        }

        // Update player state if not attacking
        if (state.state != "attack") {
            if (state.onGround) {
                state.state = (std::abs(velocity.x) > PLAYER_RUN_VELOCITY_THRESHOLD)
                              ? "run" : "idle";
            } else {
                state.state = "air";
            }
        }
    }
}



// Enemy - Tile
void CollisionSystem::handleEnemyTileCollisions() {
    for (auto& enemy : m_entityManager.getEntities("enemy")) {
        if (!enemy->has<CTransform>() || !enemy->has<CBoundingBox>())
            continue;

        auto& transform = enemy->get<CTransform>();
        auto& velocity  = transform.velocity;
        auto& enemyBB   = enemy->get<CBoundingBox>();

        // If you want to track onGround for enemies:
        bool onGround = false;
        if (enemy->has<CState>()) {
            enemy->get<CState>().onGround = false;
        }

        // Get bounding rect for enemy
        sf::FloatRect eRect = enemyBB.getRect(transform.pos);

        for (auto& tile : m_entityManager.getEntities("tile")) {
            if (!tile->has<CTransform>() || !tile->has<CBoundingBox>())
                continue;

            auto& tileTransform = tile->get<CTransform>();
            auto& tileBB        = tile->get<CBoundingBox>();
            sf::FloatRect tRect = tileBB.getRect(tileTransform.pos);

            if (!eRect.intersects(tRect))
                continue;

            if (tile->has<CAnimation>() && tile->get<CAnimation>().animation.getName() == "LevelDoor") {
                continue;
            }
            
            
            float overlapX = std::min(eRect.left + eRect.width, tRect.left + tRect.width)
                           - std::max(eRect.left, tRect.left);
            float overlapY = std::min(eRect.top + eRect.height, tRect.top + tRect.height)
                           - std::max(eRect.top, tRect.top);

            if (overlapX < overlapY) {
                // Fix horizontal overlap
                if (transform.pos.x < tileTransform.pos.x) {
                    transform.pos.x -= overlapX;
                } else {
                    transform.pos.x += overlapX;
                }
                velocity.x = 0.f;
            } else {
                // Fix vertical overlap
                if (transform.pos.y < tileTransform.pos.y) {
                    // Enemy above tile
                    transform.pos.y -= overlapY;
                    velocity.y = 0.f;
                    onGround = true;
                } else {
                    // Enemy below tile (jumped into it)
                    transform.pos.y += overlapY;
                    velocity.y = 0.f;
                }
            }

            // Update eRect if the enemy was moved
            eRect = enemyBB.getRect(transform.pos);
        }

        // If you track onGround for enemies
        if (enemy->has<CState>()) {
            enemy->get<CState>().onGround = onGround;
        }
    }
}

void CollisionSystem::handleEnemyEnemyCollisions() {
    // Get a list of all enemies
    auto enemies = m_entityManager.getEntities("enemy");

    // Compare every pair of enemies (i < j) so we don't repeat or compare an enemy with itself
    for (size_t i = 0; i < enemies.size(); i++) {
        for (size_t j = i + 1; j < enemies.size(); j++) {
            auto e1 = enemies[i];
            auto e2 = enemies[j];

            // Ensure both have transforms and bounding boxes
            if (!e1->has<CTransform>() || !e1->has<CBoundingBox>())
                continue;
            if (!e2->has<CTransform>() || !e2->has<CBoundingBox>())
                continue;

            auto& t1 = e1->get<CTransform>();
            auto& bb1 = e1->get<CBoundingBox>();
            sf::FloatRect r1 = bb1.getRect(t1.pos);

            auto& t2 = e2->get<CTransform>();
            auto& bb2 = e2->get<CBoundingBox>();
            sf::FloatRect r2 = bb2.getRect(t2.pos);

            // Check intersection
            if (!r1.intersects(r2))
                continue;

            // Calculate overlap on X and Y
            float overlapX = std::min(r1.left + r1.width,  r2.left + r2.width)
                           - std::max(r1.left,            r2.left);
            float overlapY = std::min(r1.top + r1.height, r2.top + r2.height)
                           - std::max(r1.top,             r2.top);

            // Decide which axis to resolve based on smaller overlap
            if (overlapX < overlapY) {
                // --- Resolve horizontally ---
                // We'll push each enemy half the overlap
                float push = overlapX * 0.5f;

                // If e1 is left of e2
                if (t1.pos.x < t2.pos.x) {
                    t1.pos.x -= push;
                    t2.pos.x += push;
                } else {
                    t1.pos.x += push;
                    t2.pos.x -= push;
                }

                // Optionally zero out their horizontal velocities so they stop sliding into each other
                if (std::abs(t1.velocity.x) > 0.f) t1.velocity.x = 0.f;
                if (std::abs(t2.velocity.x) > 0.f) t2.velocity.x = 0.f;
            }
            else {
                // --- Resolve vertically ---
                float push = overlapY * 0.5f;

                // If e1 is above e2
                if (t1.pos.y < t2.pos.y) {
                    t1.pos.y -= push;
                    t2.pos.y += push;
                } else {
                    t1.pos.y += push;
                    t2.pos.y -= push;
                }

                // Optionally zero out their vertical velocities
                if (std::abs(t1.velocity.y) > 0.f) t1.velocity.y = 0.f;
                if (std::abs(t2.velocity.y) > 0.f) t2.velocity.y = 0.f;
            }
        }
    }
}

// Player - Enemy
void CollisionSystem::handlePlayerEnemyCollisions() {
    for (auto& enemy : m_entityManager.getEntities("enemy")) {
        // Skip enemy if in attack state.
        if (enemy->has<CEnemyAI>()) {
            auto& enemyAI = enemy->get<CEnemyAI>();
            if (enemyAI.enemyState == EnemyState::Attack)
                continue;
        }
        auto& enemyTrans = enemy->get<CTransform>();
        auto& enemyBB    = enemy->get<CBoundingBox>();
        sf::FloatRect enemyRect = enemyBB.getRect(enemyTrans.pos);

        // Check enemy on-ground state.
        bool enemyOnGround = false;
        if (enemy->has<CState>())
            enemyOnGround = enemy->get<CState>().onGround;

        for (auto& player : m_entityManager.getEntities("player")) {
            auto& pTrans = player->get<CTransform>();
            auto& pBB    = player->get<CBoundingBox>();
            sf::FloatRect playerRect = pBB.getRect(pTrans.pos);

            // Check player on-ground state.
            bool playerOnGround = false;
            if (player->has<CState>())
                playerOnGround = player->get<CState>().onGround;

            if (enemyRect.intersects(playerRect)) {
                // Compute overlaps along X and Y.
                float overlapX = std::min(enemyRect.left + enemyRect.width, playerRect.left + playerRect.width)
                                 - std::max(enemyRect.left, playerRect.left);
                float overlapY = std::min(enemyRect.top + enemyRect.height, playerRect.top + playerRect.height)
                                 - std::max(enemyRect.top, playerRect.top);

                const float MIN_VERTICAL_SEPARATION = 5.f;
                float separation = std::max(overlapY * COLLISION_SEPARATION_FACTOR, MIN_VERTICAL_SEPARATION);
                const float bounceSpeed = 100.f;

                if (overlapX < overlapY) {
                    // Horizontal collision resolution (unchanged)
                    if (enemyTrans.pos.x < pTrans.pos.x) {
                        enemyTrans.pos.x -= overlapX * COLLISION_SEPARATION_FACTOR;
                        pTrans.pos.x     += overlapX * COLLISION_SEPARATION_FACTOR;
                        enemyTrans.velocity.x = -std::max(std::abs(enemyTrans.velocity.x), bounceSpeed);
                        pTrans.velocity.x     = std::max(std::abs(pTrans.velocity.x), bounceSpeed);
                    } else {
                        enemyTrans.pos.x += overlapX * COLLISION_SEPARATION_FACTOR;
                        pTrans.pos.x     -= overlapX * COLLISION_SEPARATION_FACTOR;
                        enemyTrans.velocity.x = std::max(std::abs(enemyTrans.velocity.x), bounceSpeed);
                        pTrans.velocity.x     = -std::max(std::abs(pTrans.velocity.x), bounceSpeed);
                    }
                } else {
                    // Vertical collision resolution.
                    // If enemy is above player:
                    if (enemyTrans.pos.y < pTrans.pos.y) {
                        if (playerOnGround) {
                            // Player is fixed on the ground; push enemy upward only.
                            enemyTrans.pos.y -= separation;
                            enemyTrans.velocity.y = -bounceSpeed;
                        } else {
                            // Normal resolution: enemy upward, player downward.
                            enemyTrans.pos.y -= separation;
                            pTrans.pos.y     += separation;
                            enemyTrans.velocity.y = -bounceSpeed;
                            pTrans.velocity.y     = bounceSpeed;
                        }
                    } else {
                        // Enemy is below player:
                        if (enemyOnGround) {
                            // Enemy is on ground; push player upward only.
                            pTrans.pos.y -= separation;
                            pTrans.velocity.y = -bounceSpeed;
                        } else {
                            // Normal resolution: enemy downward, player upward.
                            enemyTrans.pos.y += separation;
                            pTrans.pos.y     -= separation;
                            enemyTrans.velocity.y =  bounceSpeed;
                            pTrans.velocity.y     = -bounceSpeed;
                        }
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

                static std::random_device rd;
                static std::mt19937 gen(rd());
                std::uniform_int_distribution<int> dist(PLAYER_SWORD_KNOCKBACK_ROLL_MIN, PLAYER_SWORD_KNOCKBACK_ROLL_MAX);
                int roll = dist(gen);
                std::cout << "[DEBUG] roll = " << roll << "\n";

                if (roll == PLAYER_SWORD_KNOCKBACK_ROLL_TRIGGER) {
                    float attackDirection = (swTrans.pos.x < enemyTrans.pos.x) ? 1.f : -1.f;
                    Vec2<float> hitDirection = { attackDirection, PLAYER_SWORD_KNOCKBACK_Y_DIRECTION };

                    float finalKnockback = PLAYER_SWORD_KNOCKBACK_STRENGTH + (roll * 5.f);

                    // Applica il knockback
                    Physics::Forces::ApplyKnockback(enemy, hitDirection, finalKnockback);

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
                int enemyDamage = enemySword->get<CEnemyAI>().damage;
                health.takeDamage(enemyDamage);
                std::cout << "[DEBUG] Player hit by enemy sword! Damage: " << enemyDamage << " " << "Health: " << health.currentHealth << "\n" ;
                health.invulnerabilityTimer = PLAYER_HIT_INVULNERABILITY_TIME;
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
            auto& health = player->get<CHealth>();
            
            sf::FloatRect iRect = iBB.getRect(iTrans.pos);

            if (pRect.intersects(iRect)) {
                std::string itemType = item->get<CState>().state;
                if (itemType == "GrapeSmall") {
                    health.heal(COLLECTIBLE_SMALL_GRAPE_POINTS);
                } else if (itemType == "GrapeBig") {
                    if (player->has<CHealth>()) {
                        health.heal(COLLECTIBLE_BIG_GRAPE_HEAL);
                    }
                } else if (itemType == "CoinGold") {
                    m_score += COLLECTIBLE_GOLD_COIN_POINTS;
                } else if (itemType == "CoinSilver") {
                    m_score += COLLECTIBLE_SILVER_COIN_POINTS;
                } else if (itemType == "CoinBronze") {
                    m_score += COLLECTIBLE_BRONZE_COIN_POINTS;
                }
                item->destroy();
            }
        }
    }
}
