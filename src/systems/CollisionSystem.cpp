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
    handleBulletPlayerCollisions();
    handlePlayerBulletCollisions();
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

        // -------------------------------------
        // We'll store if we picked up armor here
        // -------------------------------------
        bool pickedUpArmor = false;
        std::shared_ptr<Entity> tileToDestroy = nullptr;

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

            // Potential next-level logic
            std::string nextLevelPath = "";

            // Check tile's animation name
            std::string animName = tileAnim.getName();
            std::string worldLevelDoor       = m_game.worldType + "LevelDoor";  
            std::string worldLevelDoorGold   = m_game.worldType + "LevelDoorGold";
            std::string worldLevelBlackHole  = m_game.worldType + "BlackHoleRedBig";

            // 1) If tile is a LevelDoor/BlackHole
            if (animName == worldLevelDoor || animName == worldLevelDoorGold || animName == worldLevelBlackHole) {
                std::cout << "[DEBUG] Player entered " << animName << ". Scheduling level change...\n";
                m_game.scheduleLevelChange(m_game.getNextLevelPath());
                return;
            }
            // 2) If tile is FutureArmor, mark it for destruction AFTER overlap resolution
            if (animName == "FutureArmor") {
                std::cout << "[DEBUG] Player picked up Future Armor!\n";
                if (player->has<CPlayerEquipment>()) {
                    player->get<CPlayerEquipment>().hasFutureArmor = true;
                }
                pickedUpArmor = true;
                tileToDestroy = tile;  // We'll destroy it after we do overlap resolution
            }

            // 3) Next-level path check (unused in your snippet, but kept for consistency)
            if (!nextLevelPath.empty()) {
                std::cout << "[DEBUG] Transitioning to next level: " << nextLevelPath << std::endl;
                if (nextLevelPath == "./bin/levels/") {
                    std::cerr << "[ERROR] Invalid next level path (empty string)!\n";
                    return;
                }
                m_game.loadLevel(nextLevelPath);
                return;
            }

            // 4) Calculate overlap on X and Y
            float overlapX = std::min(pRect.left + pRect.width, tRect.left + tRect.width)
                           - std::max(pRect.left, tRect.left);
            float overlapY = std::min(pRect.top + pRect.height, tRect.top + tRect.height)
                           - std::max(pRect.top, tRect.top);

            // Decide which axis to fix based on which overlap is smaller
            if (overlapX < overlapY) {
                // Fix X overlap
                if (transform.pos.x < tileTransform.pos.x) {
                    transform.pos.x -= overlapX; // Player is left, push left
                } else {
                    transform.pos.x += overlapX; // Player is right, push right
                }
                velocity.x = 0.f; // Stop horizontal velocity
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
                        if (animName == m_game.worldType + "Box1" ||
                            animName == m_game.worldType + "Box2")
                        {
                            m_spawner->createBlockFragments(tileTransform.pos, animName);
                            m_spawner->spawnItem(tileTransform.pos, animName);
                            tileToDestroy = tile;
                            std::cout << "[DEBUG] " << animName << " broken from below!\n";
                        } 
                        else if (animName == m_game.worldType + "Treasure") {
                            auto& tileState = tile->get<CState>();
                            if (tileState.state == "inactive") {
                                tileState.state = "activated";
                                std::string treasureHitAnim = m_game.worldType + "TreasureHit";

                                if (m_game.assets().hasAnimation(treasureHitAnim)) {
                                    tile->get<CAnimation>().animation = m_game.assets().getAnimation(treasureHitAnim);
                                    tile->get<CAnimation>().repeat = false;
                                    std::cout << "[DEBUG] " << animName << " hit from below.\n";
                                }
                                m_spawner->spawnItem(tileTransform.pos, treasureHitAnim);
                            }
                        }
                    }
                }
            }

            // Update pRect in case we moved the player
            pRect = playerBB.getRect(transform.pos);
        } // end for tile

        // 5) AFTER checking all tiles, destroy any tile we flagged
        if (tileToDestroy) {
            tileToDestroy->destroy();
        }

        // 6) Update player state if not attacking
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

            if (tile->has<CAnimation>()) {
                std::string animName = tile->get<CAnimation>().animation.getName();
                std::string worldLevelDoor = m_game.worldType + "LevelDoor";  
            
                if (animName == worldLevelDoor) {
                    continue;
                }
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

void CollisionSystem::handlePlayerBulletCollisions() {
    for (auto& bullet : m_entityManager.getEntities("playerBullet")) {
        // Must have CTransform & CBoundingBox
        if (!bullet->has<CTransform>() || !bullet->has<CBoundingBox>())
            continue;

        auto& bulletTrans = bullet->get<CTransform>();
        auto& bulletBB    = bullet->get<CBoundingBox>();
        sf::FloatRect bulletRect = bulletBB.getRect(bulletTrans.pos);

        // 1) Destroy bullet if it hits a tile
        for (auto& tile : m_entityManager.getEntities("tile")) {
            if (!tile->has<CTransform>() || !tile->has<CBoundingBox>())
                continue;

            auto& tileTrans = tile->get<CTransform>();
            auto& tileBB    = tile->get<CBoundingBox>();
            sf::FloatRect tileRect = tileBB.getRect(tileTrans.pos);

            if (bulletRect.intersects(tileRect)) {
                std::cout << "[DEBUG] Player bullet hit a tile! Destroying bullet.\n";
                bullet->destroy();
                break; // Stop checking after first tile collision
            }
        }

        // 2) Destroy bullet if it hits an enemy
        //    and apply damage/knockback if you want
        for (auto& enemy : m_entityManager.getEntities("enemy")) {
            if (!enemy->has<CTransform>() || !enemy->has<CBoundingBox>())
                continue;

            auto& enemyTrans = enemy->get<CTransform>();
            auto& enemyBB    = enemy->get<CBoundingBox>();
            sf::FloatRect enemyRect = enemyBB.getRect(enemyTrans.pos);

            if (!bulletRect.intersects(enemyRect))
                continue; // no collision

            // We have a collision: apply damage, destroy bullet
            std::cout << "[DEBUG] Player bullet hit an enemy! Destroying bullet.\n";

            // Example: apply damage
            if (enemy->has<CHealth>()) {
                auto& health = enemy->get<CHealth>();
                const int PLAYER_BULLET_DAMAGE = 5; // or some constant
                health.takeDamage(PLAYER_BULLET_DAMAGE);

                // Optionally set invulnerabilityTimer if you want a grace period
                // health.invulnerabilityTimer = 0.2f;

                // Possibly do knockback
                // if (enemy->has<CState>()) { ... }
            }

            bullet->destroy();
            break; // Exit after first enemy hit
        }
    }
}

void CollisionSystem::handleBulletPlayerCollisions() {
    for (auto& bullet : m_entityManager.getEntities("enemyBullet")) {
        if (!bullet->has<CTransform>() || !bullet->has<CBoundingBox>()) 
            continue;

        auto& bulletTrans = bullet->get<CTransform>();
        auto& bulletBB    = bullet->get<CBoundingBox>();
        sf::FloatRect bulletRect = bulletBB.getRect(bulletTrans.pos);

        // 1) Destroy bullet if it hits a tile
        for (auto& tile : m_entityManager.getEntities("tile")) {
            if (!tile->has<CTransform>() || !tile->has<CBoundingBox>())
                continue;

            auto& tileTrans = tile->get<CTransform>();
            auto& tileBB    = tile->get<CBoundingBox>();
            sf::FloatRect tileRect = tileBB.getRect(tileTrans.pos);

            if (bulletRect.intersects(tileRect)) {
                std::cout << "[DEBUG] Bullet hit a tile! Destroying bullet.\n";
                bullet->destroy();
                break; // Stop checking after first tile hit
            }
        }

        // 2) Destroy bullet if it hits the player
        for (auto& player : m_entityManager.getEntities("player")) {
            if (!player->has<CTransform>() || !player->has<CBoundingBox>())
                continue;

            auto& playerTrans = player->get<CTransform>();
            auto& playerBB    = player->get<CBoundingBox>();
            sf::FloatRect playerRect = playerBB.getRect(playerTrans.pos);

            if (!bulletRect.intersects(playerRect))
                continue; // No collision, skip

            // Even if the player is invulnerable, bullet is destroyed
            if (player->has<CHealth>()) {
                auto& health = player->get<CHealth>();

                // Only apply damage if invulnerability is 0
                if (health.invulnerabilityTimer <= 0.f) {
                    int bulletDamage = BULLET_DAMAGE_NORMAL; // Default
                    if (bullet->has<CEnemyAI>()) {
                        auto& bulletAI = bullet->get<CEnemyAI>();
                        switch (bulletAI.enemyType) {
                            case EnemyType::Elite:
                                bulletDamage = BULLET_DAMAGE_ELITE;
                                break;
                            case EnemyType::Strong:
                                bulletDamage = BULLET_DAMAGE_STRONG;
                                break;
                            case EnemyType::Normal:
                            default:
                                bulletDamage = BULLET_DAMAGE_NORMAL;
                                break;
                        }
                    }
                    // Apply damage and knockback
                    health.takeDamage(bulletDamage);
                    health.invulnerabilityTimer = PLAYER_HIT_INVULNERABILITY_TIME;
                    std::cout << "[DEBUG] Player hit by bullet! Damage: " 
                              << bulletDamage << " Health: " << health.currentHealth << "\n";

                    // Set knockback if needed
                    if (player->has<CState>()) {
                        player->get<CState>().state = "knockback";
                        player->get<CState>().knockbackTimer = PLAYER_KNOCKBACK_TIMER;
                    }
                }
            }

            // Destroy bullet on impact, regardless of invulnerability
            bullet->destroy();
            break; // Exit after first player hit
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
                std::cout << "[DEBUG] Player sword hit tile!\n";
                std::string animName = tileAnim.getName();
                if (animName.find("Box") != std::string::npos) {
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

        // Enemy sword vs tile
        for (auto& tile : m_entityManager.getEntities("tile")) {
            if (!tile->has<CTransform>() || !tile->has<CBoundingBox>())
                continue;

            auto& tileTransform = tile->get<CTransform>();
            auto& tileBB        = tile->get<CBoundingBox>();
            auto& tileAnim      = tile->get<CAnimation>().animation;
            sf::FloatRect tileRect = tileBB.getRect(tileTransform.pos);

            if (!swordRect.intersects(tileRect))
                continue;

            bool shouldDestroyTile = false;  // Flag to determine if the tile should be destroyed

            if (enemySword->has<CEnemyAI>()) {
                auto& swordAI = enemySword->get<CEnemyAI>();

                if (swordAI.enemyType == EnemyType::Super) {
                    std::cout << "[DEBUG] Super enemy sword destroyed a tile!\n";
                    shouldDestroyTile = true;  // Super enemy destroys any tile
                } else if (tileAnim.getName().find("Box") != std::string::npos) {
                    std::cout << "[DEBUG] Non-super enemy sword destroyed a tile containing 'Box'!\n";
                    shouldDestroyTile = true;  // Other enemies destroy only tiles containing "Box"
                }
            }

            if (shouldDestroyTile) {
                // ✅ Create block fragments & spawn item
                m_spawner->createBlockFragments(tileTransform.pos, tileAnim.getName());

                tile->destroy();  // ✅ Destroy tile
            }

            enemySword->destroy(); // ✅ Destroy the sword on impact
            break; // Exit loop after handling collision
        }
    }

    for (auto& empSword : m_entityManager.getEntities("EmperorSword")) {
        // Verifichiamo che abbia CTransform e CBoundingBox
        if (!empSword->has<CTransform>() || !empSword->has<CBoundingBox>())
            continue;

        auto& swTrans = empSword->get<CTransform>();
        auto& swBB    = empSword->get<CBoundingBox>();
        
        // Se ha un CLifeSpan, puoi leggerlo o stamparlo (opzionale)
        if (empSword->has<CLifeSpan>()) {
            auto& swLS = empSword->get<CLifeSpan>();
            //std::cout << "[DEBUG] Emperor sword lifespan: " << swLS.remainingTime << "\n";
        }

        sf::FloatRect swordRect = swBB.getRect(swTrans.pos);

        // 3a) EmperorSword vs Player
        for (auto& player : m_entityManager.getEntities("player")) {
            if (!player->has<CTransform>() || !player->has<CBoundingBox>())
                continue;

            auto& pTrans = player->get<CTransform>();
            auto& pBB    = player->get<CBoundingBox>();
            sf::FloatRect playerRect = pBB.getRect(pTrans.pos);

            if (!swordRect.intersects(playerRect))
                continue; // niente collisione

            // Se il player è già in knockback, salta
            if (player->has<CState>()) {
                auto& playerState = player->get<CState>();
                if (playerState.state == "knockback")
                    continue;
            }

            // Esempio di knockback orizzontale
            float attackDirection = (swTrans.pos.x < pTrans.pos.x) ? 1.f : -1.f;
            Vec2<float> hitDirection = { attackDirection, 0.f };

            // Applica il knockback
            Physics::Forces::ApplyKnockback(player, hitDirection, EMPEROR_SWORD_KNOCKBACK_STRENGTH);

            // Danno al player
            if (player->has<CHealth>()) {
                auto& health = player->get<CHealth>();

                // Se la spada ha un CEnemyAI, usiamo "damage"
                int empSwordDamage = 10; // default
                if (empSword->has<CEnemyAI>()) {
                    empSwordDamage = empSword->get<CEnemyAI>().damage;
                }

                health.takeDamage(empSwordDamage);
                health.invulnerabilityTimer = PLAYER_HIT_INVULNERABILITY_TIME;

                std::cout << "[DEBUG] Player hit by emperor sword! Damage: " 
                          << empSwordDamage 
                          << " Health: " << health.currentHealth << "\n";
            }

            // Settiamo lo stato a knockback
            if (player->has<CState>()) {
                auto& pState = player->get<CState>();
                pState.state = "knockback";
                pState.knockbackTimer = PLAYER_KNOCKBACK_TIMER;
            }

            // Se vuoi distruggere la spada all'impatto, scommenta:
            empSword->destroy();

            break; // Esci dal loop player
        }

        // 3b) EmperorSword vs tile
        for (auto& tile : m_entityManager.getEntities("tile")) {
            if (!tile->has<CTransform>() || !tile->has<CBoundingBox>())
                continue;

            auto& tileTransform = tile->get<CTransform>();
            auto& tileBB        = tile->get<CBoundingBox>();
            sf::FloatRect tileRect = tileBB.getRect(tileTransform.pos);

            if (!swordRect.intersects(tileRect))
                continue;

            // Se vuoi distruggere la spada quando tocca un tile, fai:
            empSword->destroy();

            // Oppure, se vuoi distruggere un tile "fragile" e la spada, fai:
            // tile->destroy();
            // empSword->destroy();

            //std::cout << "[DEBUG] Emperor sword destroyed on tile collision!\n";
            break; // Esci dal loop tile
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
            
                if (itemType.find("Grape") != std::string::npos) {
                    if (itemType.find("Small") != std::string::npos) {
                        health.heal(COLLECTIBLE_SMALL_GRAPE_POINTS);
                    } else if (itemType.find("Big") != std::string::npos && player->has<CHealth>()) {
                        health.heal(COLLECTIBLE_BIG_GRAPE_HEAL);
                    }
                } 
                else if (itemType.find("Coin") != std::string::npos) {
                    if (itemType.find("Gold") != std::string::npos) {
                        m_score += COLLECTIBLE_GOLD_COIN_POINTS;
                    } else if (itemType.find("Silver") != std::string::npos) {
                        m_score += COLLECTIBLE_SILVER_COIN_POINTS;
                    } else if (itemType.find("Bronze") != std::string::npos) {
                        m_score += COLLECTIBLE_BRONZE_COIN_POINTS;
                    }
                }
            
                item->destroy();
            }
        }
    }
}
