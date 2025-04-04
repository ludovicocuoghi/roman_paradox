#include "CollisionSystem.h"
#include "Physics.hpp"
#include "ResourcePath.h"
#include <iostream>
#include <algorithm>
#include <limits>
#include <random>
#include <SFML/Graphics.hpp>

CollisionSystem::CollisionSystem(EntityManager& entityManager, GameEngine& game, Spawner* spawner, int& score, const std::string& levelPath)
    : m_entityManager(entityManager), m_game(game), m_spawner(spawner), m_score(score), m_levelPath(levelPath) {}

void CollisionSystem::updateCollisions() {
    if (m_score >=100) {
        for (auto& player : m_entityManager.getEntities("player")) {
            auto& health = player->get<CHealth>();
            health.heal(health.maxHealth);
        m_score -=100;
        }
    }
    handlePlayerTileCollisions();
    handleEnemyTileCollisions();
    handlePlayerEnemyCollisions();
    handleEnemyEnemyCollisions();
    handleSwordCollisions();
    handleBlackHoleTileCollisions();
    handleMassiveBlackHoleCollisions();
    handleBulletPlayerCollisions();
    handlePlayerBulletCollisions();
    handlePlayerCollectibleCollisions();
}

// Player - Tile
void CollisionSystem::handlePlayerTileCollisions() {
    for (auto& player : m_entityManager.getEntities("player")) {

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

        // Store if we picked up armor 
        [[maybe_unused]]bool pickedUpArmor = false;

        std::shared_ptr<Entity> tileToDestroy = nullptr;

        // Check collision with each tile
        for (auto& tile : m_entityManager.getEntities("tile")) {
            if (!tile->has<CTransform>() || !tile->has<CBoundingBox>() || !tile->has<CAnimation>())
                continue;

            auto& tileTransform = tile->get<CTransform>();
            auto& tileBB        = tile->get<CBoundingBox>();
            auto& tileAnim      = tile->get<CAnimation>().animation;

            sf::FloatRect tRect = tileBB.getRect(tileTransform.pos);

            // If no intersection, skip
            if (!pRect.intersects(tRect))
                continue;

            std::string animName = tileAnim.getName();
            if (animName == "AlienBlackHoleAttack") {
                if (player->has<CHealth>()) {
                    auto& health = player->get<CHealth>();
            
                    // Force health to 0
                    health.currentHealth = 0;
                    
                    // Return to stop further collision checks
                    return;
                }
            }

            // Potential next-level logic
            std::string nextLevelPath = "";

            std::string worldLevelDoor       = m_game.worldType + "LevelDoor";  
            std::string worldLevelDoorGold   = m_game.worldType + "LevelDoorGold";
            std::string worldLevelBlackHole  = m_game.worldType + "BlackHoleRedBig";

            // If tile is a LevelDoor/BlackHole
            if (animName == worldLevelDoor || animName == worldLevelDoorGold || animName == worldLevelBlackHole) {
                m_game.scheduleLevelChange(m_game.getNextLevelPath());
                return;
            }
            // If tile is FutureArmor, mark it for destruction AFTER overlap resolution
            if (animName == "FutureArmor") {
                
                if (player->has<CPlayerEquipment>()) {
                    player->get<CPlayerEquipment>().hasFutureArmor = true;
                }
                
                if (player->has<CHealth>()) {
                    // Heal player to max health
                    auto& health = player->get<CHealth>();

                    player->get<CHealth>().maxHealth = health.maxHealth * 1.5;
                    health.heal(health.maxHealth); 
                }
                
                pickedUpArmor = true;
                tileToDestroy = tile;  // We'll destroy it after we do overlap resolution
            }

            // Next-level path check
            if (!nextLevelPath.empty()) {
                
                std::string resourcePath = getResourcePath("levels");
                if (nextLevelPath == resourcePath + "/") {
                    std::cerr << "[ERROR] Invalid next level path (empty filename)!\n";
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
                    transform.pos.x -= overlapX; // Player is left, push left
                } else {
                    transform.pos.x += overlapX; // Player is right, push right
                }
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
                            // std::cout << "[DEBUG] " << animName << " broken from below!\n";
                        } 
                        else if (animName == m_game.worldType + "Treasure") {
                            auto& tileState = tile->get<CState>();
                            if (tileState.state == "inactive") {
                                tileState.state = "activated";
                                std::string treasureHitAnim = m_game.worldType + "TreasureHit";

                                if (m_game.assets().hasAnimation(treasureHitAnim)) {
                                    tile->get<CAnimation>().animation = m_game.assets().getAnimation(treasureHitAnim);
                                    tile->get<CAnimation>().repeat = false;
                                    // std::cout << "[DEBUG] " << animName << " hit from below.\n";
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

        // AFTER checking all tiles, destroy any tile we flagged
        if (tileToDestroy) {
            tileToDestroy->destroy();
        }

        // Update player state if not attacking
        if (state.state != "attack" && state.state != "defense") {
            if (state.onGround) {
                state.state = (std::abs(velocity.x) > PLAYER_RUN_VELOCITY_THRESHOLD)
                              ? "run" : "idle";
            } else {
                state.state = "air";
            }
        }
    }
}

void CollisionSystem::handleMassiveBlackHoleCollisions() {
    for (auto& massiveBlackHole : m_entityManager.getEntities("emperorMassiveBlackHole")) {
        if (!massiveBlackHole->has<CTransform>() || !massiveBlackHole->has<CBoundingBox>()) 
            continue;

        auto& blackHoleTrans = massiveBlackHole->get<CTransform>();
        auto& blackHoleBB = massiveBlackHole->get<CBoundingBox>();
        sf::FloatRect blackHoleRect = blackHoleBB.getRect(blackHoleTrans.pos);

        // destroy ALL tiles it passes through
        for (auto& tile : m_entityManager.getEntities("tile")) {
            if (!tile->has<CTransform>() || !tile->has<CBoundingBox>())
                continue;

            auto& tileTrans = tile->get<CTransform>();
            auto& tileBB = tile->get<CBoundingBox>();
            sf::FloatRect tileRect = tileBB.getRect(tileTrans.pos);

            // Using regular collision for simplicity
            if (blackHoleRect.intersects(tileRect)) {
                auto& tileAnim      = tile->get<CAnimation>().animation;
                std::string animName = tileAnim.getName();
                m_spawner->createBlockFragments(tileTrans.pos, animName);
                tile->destroy();
            }
        }
        
        // Instant death on player contact
        for (auto& player : m_entityManager.getEntities("player")) {
            if (!player->has<CTransform>() || !player->has<CBoundingBox>())
                continue;

            auto& playerTrans = player->get<CTransform>();
            auto& playerBB = player->get<CBoundingBox>();
            sf::FloatRect playerRect = playerBB.getRect(playerTrans.pos);

            if (blackHoleRect.intersects(playerRect)) {
                // Instant kill
                if (player->has<CHealth>()) {
                    player->get<CHealth>().currentHealth = 0;
                }
            }
        }
    }
}

void CollisionSystem::handleEnemyTileCollisions() {
    for (auto& enemy : m_entityManager.getEntities("enemy")) {
        if (!enemy->has<CTransform>() || !enemy->has<CBoundingBox>()) continue;

        auto& transform = enemy->get<CTransform>();
        auto& velocity  = transform.velocity;
        auto& enemyBB   = enemy->get<CBoundingBox>();
        sf::FloatRect eRect = enemyBB.getRect(transform.pos);

        bool onGround = false;
        if (enemy->has<CState>()) {
            enemy->get<CState>().onGround = false;
        }

        // Check if enemy is a citizen
        bool isCitizen = false;
        if (enemy->has<CEnemyAI>()) {
            isCitizen = enemy->get<CEnemyAI>().enemyType == EnemyType::Citizen;
        }

        // Detect if an EnemySuper has a tile in front
        bool tileInFront = false;

        for (auto& tile : m_entityManager.getEntities("tile")) {
            if (!tile->has<CTransform>() || !tile->has<CBoundingBox>() || !tile->has<CAnimation>()) continue;

            auto& tileTrans = tile->get<CTransform>();
            auto& tileBB    = tile->get<CBoundingBox>();
            auto& tileAnim  = tile->get<CAnimation>().animation;
            sf::FloatRect tRect = tileBB.getRect(tileTrans.pos);

            // Skip if no intersection
            if (!eRect.intersects(tRect)) continue;

            std::string animName = tileAnim.getName();

            // If tile is a black hole and enemy is a citizen, kill the citizen
            if (animName.find("BlackHole") != std::string::npos) {
                if (isCitizen) {
                    // Kill the citizen if it touches a black hole
                    if (enemy->has<CHealth>()) {
                        enemy->get<CHealth>().currentHealth = 0;
                    } else {
                        // If no health component, just destroy the entity
                        enemy->destroy();
                    }
                    break; // Stop checking other tiles for this enemy
                }
                
                // For non-citizens, let them pass through black holes
                continue;
            }

            // Check if tile is in front of EnemySuper (not just intersecting)
            if (enemy->has<CEnemyAI>()) {
                auto& enemyAI = enemy->get<CEnemyAI>();
                if (enemyAI.enemyType == EnemyType::Super) {
                    float enemyFrontX = (enemyAI.facingDirection > 0.f) 
                                        ? eRect.left + eRect.width 
                                        : eRect.left;

                    bool isHorizontallyAligned =
                        (tRect.top < eRect.top + eRect.height) &&
                        (tRect.top + tRect.height > eRect.top);

                    bool isInFront =
                        (enemyAI.facingDirection > 0.f && tRect.left <= enemyFrontX && tRect.left > eRect.left) ||
                        (enemyAI.facingDirection < 0.f && tRect.left + tRect.width >= enemyFrontX && tRect.left < eRect.left);

                    if (isHorizontallyAligned && isInFront) {
                        tileInFront = true;
                    }
                }
            }

            // Normal collision resolution for all enemies
            float overlapX = std::min(eRect.left + eRect.width, tRect.left + tRect.width)
                           - std::max(eRect.left, tRect.left);
            float overlapY = std::min(eRect.top + eRect.height, tRect.top + tRect.height)
                           - std::max(eRect.top, tRect.top);

            if (overlapX < overlapY) {
                // Horizontal push
                transform.pos.x += (transform.pos.x < tileTrans.pos.x) ? -overlapX : overlapX;
                velocity.x = 0.f;
            } else {
                // Vertical push
                if (transform.pos.y < tileTrans.pos.y) {
                    transform.pos.y -= overlapY;
                    velocity.y = 0.f;
                    onGround = true;
                } else {
                    transform.pos.y += overlapY;
                    velocity.y = 0.f;
                }
            }

            // Update bounding rect in case we moved the enemy
            eRect = enemyBB.getRect(transform.pos);
        }

        // Store tile detection in CEnemyAI for AI to use
        if (enemy->has<CEnemyAI>()) {
            enemy->get<CEnemyAI>().tileDetected = tileInFront;
        }

        // Update onGround if enemy has a CState
        if (enemy->has<CState>()) {
            enemy->get<CState>().onGround = onGround;
        }
    }
}

void CollisionSystem::handleEnemyEnemyCollisions() {

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
void CollisionSystem::handlePlayerEnemyCollisions() {
    for (auto& enemy : m_entityManager.getEntities("enemy")) {
        // Skip if missing required components
        if (!enemy->has<CTransform>() || !enemy->has<CBoundingBox>())
            continue;

        // Skip enemy if in attack state
        if (enemy->has<CEnemyAI>()) {
            auto& enemyAI = enemy->get<CEnemyAI>();
            if (enemyAI.enemyState == EnemyState::Attack)
                continue;
            if (enemyAI.enemyType == EnemyType::Citizen)
                continue;
        }

        auto& enemyTrans = enemy->get<CTransform>();
        auto& enemyBB    = enemy->get<CBoundingBox>();
        sf::FloatRect enemyRect = enemyBB.getRect(enemyTrans.pos);

        for (auto& player : m_entityManager.getEntities("player")) {
            if (!player->has<CTransform>() || !player->has<CBoundingBox>() || !player->has<CState>())
                continue;
                
            auto& pTrans = player->get<CTransform>();
            auto& pBB    = player->get<CBoundingBox>();
            auto& pState = player->get<CState>();
            sf::FloatRect playerRect = pBB.getRect(pTrans.pos);
            
            if (enemyRect.intersects(playerRect)) {
                // Compute overlaps along X and Y
                float overlapX = std::min(enemyRect.left + enemyRect.width, playerRect.left + playerRect.width)
                                 - std::max(enemyRect.left, playerRect.left);
                float overlapY = std::min(enemyRect.top + enemyRect.height, playerRect.top + playerRect.height)
                                 - std::max(enemyRect.top, playerRect.top);

                const float MIN_VERTICAL_SEPARATION = 5.f;
                float separation = std::max(overlapY * COLLISION_SEPARATION_FACTOR, MIN_VERTICAL_SEPARATION);
                
                // Adjusted bounce speeds
                const float playerVerticalBounceSpeed = 250.f;
                
                // Check if player is moving downward (falling or jumping down)
                bool playerFalling = pTrans.velocity.y > 0;

                // If player is jumping on enemy from above
                if (playerFalling && pTrans.pos.y < enemyTrans.pos.y && overlapY < overlapX) {
                    // Player is landing on enemy from above
                    pTrans.pos.y -= separation;
                    pTrans.velocity.y = -playerVerticalBounceSpeed; // Controlled upward bounce for player
                    
                    // Set a small invincibility period for the player
                    pState.isInvincible = true;
                    pState.invincibilityTimer = 0.2f;
                    
                    // Optional: Give the player a "bounce" feeling
                    pState.isJumping = true;
                    pState.jumpTime = 0.1f; // Short jump time to maintain control
                    
                    continue; // Skip other collision handling for this interaction
                }
                
                // Horizontal collision - prevent player from going through
                if (overlapX <= overlapY) {
                    // Make player back away by 20 pixels
                    auto& enemyAI = enemy->get<CEnemyAI>();
                    if (enemyAI.enemyType == EnemyType::Emperor) {
                        float backDirection = (enemyTrans.pos.x < pTrans.pos.x) ? 1.0f : -1.0f;
                        pTrans.pos.x += 20.0f * backDirection;
                    }
                } 
                // Other vertical collision cases
                else {
                    // If enemy is above player
                    if (enemyTrans.pos.y < pTrans.pos.y) {
                        pTrans.pos.y = enemyRect.top + enemyRect.height + 1.0f;
                        pTrans.velocity.y = 0;
                    } 
                    // Enemy is below player
                    else {
                        pTrans.pos.y = enemyRect.top - pBB.halfSize.y - 1.0f;
                        pTrans.velocity.y = 0;
                    }
                }
            }
        }
    }
}

void CollisionSystem::handlePlayerBulletCollisions() {
    for (auto& bullet : m_entityManager.getEntities("playerBullet")) {
        if (!bullet->has<CTransform>() || !bullet->has<CBoundingBox>())
            continue;

        auto& bulletTrans = bullet->get<CTransform>();
        auto& bulletBB    = bullet->get<CBoundingBox>();
        sf::FloatRect bulletRect = bulletBB.getRect(bulletTrans.pos);

        // Destroy bullet if it hits a tile
        for (auto& tile : m_entityManager.getEntities("tile")) {
            if (!tile->has<CTransform>() || !tile->has<CBoundingBox>() || !tile->has<CAnimation>())
                continue;

            auto& tileTrans = tile->get<CTransform>();
            auto& tileBB    = tile->get<CBoundingBox>();
            auto& tileAnim  = tile->get<CAnimation>().animation;
            sf::FloatRect tileRect = tileBB.getRect(tileTrans.pos);

            if (!bulletRect.intersects(tileRect))
                continue;

            bullet->destroy();

            // If the tile is a "Box", destroy it
            std::string animName = tileAnim.getName();
            if (animName.find("Box") != std::string::npos) {
                // If you want to spawn items or fragments:
                m_spawner->createBlockFragments(tileTrans.pos, animName);
                m_spawner->spawnItem(tileTrans.pos, animName);
                tile->destroy();
            }
            break; // Stop checking after first tile collision
        }

        // Destroy bullet if it hits an enemy
        for (auto& enemy : m_entityManager.getEntities("enemy")) {
            if (!enemy->has<CTransform>() || !enemy->has<CBoundingBox>())
                continue;

            auto& enemyTrans = enemy->get<CTransform>();
            auto& enemyBB    = enemy->get<CBoundingBox>();
            sf::FloatRect enemyRect = enemyBB.getRect(enemyTrans.pos);

            if (!bulletRect.intersects(enemyRect))
                continue; 


            if (enemy->has<CHealth>()) {
                // Get bullet damage from player's CState
                float bulletDamage = 5.0f; // Default damage if we can't find the player
                
                // Find the player and get their bullet damage
                auto players = m_entityManager.getEntities("player");
                if (!players.empty() && players.front()->has<CState>()) {
                    bulletDamage = players.front()->get<CState>().bulletDamage;
                }
                
                // Apply damage to enemy
                auto& health = enemy->get<CHealth>();
                int damageToApply = static_cast<int>(bulletDamage);
                health.takeDamage(damageToApply);
                
            }
            
            bullet->destroy();
            break; 
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

        // Destroy bullet if it hits a tile
        // Check for bullet collision with tiles
        for (auto& tile : m_entityManager.getEntities("tile")) {
            if (!tile->has<CTransform>() || !tile->has<CBoundingBox>())
                continue;

            auto& tileTrans = tile->get<CTransform>();
            auto& tileBB    = tile->get<CBoundingBox>();
            sf::FloatRect tileRect = tileBB.getRect(tileTrans.pos);

            bool isSuper2Bullet = false;
            if (bullet->has<CState>()) {
                std::string parentId = bullet->get<CState>().state;
                for (auto& enemy : m_entityManager.getEntities("enemy")) {
                    if (std::to_string(enemy->id()) == parentId && 
                        enemy->has<CEnemyAI>() && 
                        enemy->get<CEnemyAI>().enemyType == EnemyType::Super2) {
                        isSuper2Bullet = true;
                        break;
                    }
                }
            }

            if (bulletRect.intersects(tileRect)) {
                if (isSuper2Bullet) {
                    // Super2 bullets destroy tiles
                    auto& tileAnim      = tile->get<CAnimation>().animation;
                    std::string animName = tileAnim.getName();
                    m_spawner->createBlockFragments(tileTrans.pos, animName);
                    tile->destroy();
                } else {
                    // Regular bullets get destroyed by tiles
                    bullet->destroy();
                }
                break;
            }
        }
        // Check for bullet collision with citizens
        for (auto& citizen : m_entityManager.getEntities("enemy")) {
            // Skip if not a citizen
            if (!citizen->has<CEnemyAI>() || 
                citizen->get<CEnemyAI>().enemyType != EnemyType::Citizen ||
                !citizen->has<CTransform>() || !citizen->has<CBoundingBox>())
                continue;
            
            auto& citizenTrans = citizen->get<CTransform>();
            auto& citizenBB = citizen->get<CBoundingBox>();
            sf::FloatRect citizenRect = citizenBB.getRect(citizenTrans.pos);
            
            if (bulletRect.intersects(citizenRect)) {
                // Kill the citizen
                if (citizen->has<CHealth>()) {
                    auto& health = citizen->get<CHealth>();
                    health.currentHealth = 0;
                } else {
                    citizen->destroy();
                }
                
                // Also destroy the bullet (unless it's a Super2 bullet)
                bool isSuper2Bullet = false;
                if (bullet->has<CState>()) {
                    std::string parentId = bullet->get<CState>().state;
                    for (auto& enemy : m_entityManager.getEntities("enemy")) {
                        if (std::to_string(enemy->id()) == parentId && 
                            enemy->has<CEnemyAI>() && 
                            enemy->get<CEnemyAI>().enemyType == EnemyType::Super2) {
                            isSuper2Bullet = true;
                            break;
                        }
                    }
                }
                
                if (!isSuper2Bullet) {
                    bullet->destroy();
                }
                break;
            }
        }

        // Check for bullet collision with player
        for (auto& player : m_entityManager.getEntities("player")) {
            if (!player->has<CTransform>() || !player->has<CBoundingBox>())
                continue;

            auto& playerTrans = player->get<CTransform>();
            auto& playerBB    = player->get<CBoundingBox>();
            sf::FloatRect playerRect = playerBB.getRect(playerTrans.pos);

            if (!bulletRect.intersects(playerRect))
                continue;

            // If player is invincible, ignore bullet
            if (player->has<CState>()) {
                auto& st = player->get<CState>();
                if (st.state == "defense") {
                    bullet->destroy();
                    break;
                }
            }

            // Apply damage to player
            if (player->has<CHealth>()) {
                auto& health = player->get<CHealth>();
                if (health.invulnerabilityTimer <= 0.f) {
                    // Default damage if we can't determine the source
                    int bulletDamage = 10;
                    
                    // Get the enemy ID from the bullet's state
                    if (bullet->has<CState>()) {
                        std::string enemyIdStr = bullet->get<CState>().state;
                        
                    // Try to convert the string to a numeric ID
                    int enemyId = std::stoi(enemyIdStr);
                    
                    // Find the enemy with this ID
                    for (auto& enemy : m_entityManager.getEntities("enemy")) {
                        if (enemy->id() == static_cast<size_t>(enemyId) && enemy->has<CEnemyAI>()) {
                            // Get damage from the original enemy
                            bulletDamage = enemy->get<CState>().bulletDamage;

                        // Apply 0.6 multiplier if the enemy is Emperor
                        if (enemy->get<CEnemyAI>().enemyType == EnemyType::Emperor) {
                            bulletDamage = static_cast<int>(bulletDamage * 0.6f);
                        }
                            break;
                        }
                    }
                    }
                    // Apply FutureArmor protection if relevant
                    bool hasFutureArmor = false;
                    if (player->has<CPlayerEquipment>()) {
                        hasFutureArmor = player->get<CPlayerEquipment>().hasFutureArmor;
                    }
                    
                    if (!hasFutureArmor) {
                        bulletDamage = static_cast<int>(bulletDamage * 1.5f);
                        // std::cout << "[DEBUG] Player without FutureArmor => bulletDamage x1.5 => " 
                        //           << bulletDamage << "\n";
                    }
                    
                    // Apply the damage
                    health.takeDamage(bulletDamage);
                    health.invulnerabilityTimer = PLAYER_HIT_INVULNERABILITY_TIME;
                    // std::cout << "[DEBUG] Player hit by bullet! Damage: " 
                    //           << bulletDamage << " Health: " << health.currentHealth << "\n";
                } else {
                    // std::cout << "[DEBUG] Player already invincible, ignoring bullet.\n";
                }
            }

            bullet->destroy();
            break; 
        }
    }
}
void CollisionSystem::handleBlackHoleTileCollisions() {
    for (auto& blackHole : m_entityManager.getEntities("emperorBlackHole")) {
        if (!blackHole->has<CTransform>() || !blackHole->has<CBoundingBox>()) 
            continue;

        auto& blackHoleTrans = blackHole->get<CTransform>();
        auto& blackHoleBB = blackHole->get<CBoundingBox>();
        sf::FloatRect blackHoleRect = blackHoleBB.getRect(blackHoleTrans.pos);

        // Check for collisions with tiles
        for (auto& tile : m_entityManager.getEntities("tile")) {
            if (!tile->has<CTransform>() || !tile->has<CBoundingBox>())
                continue;

            // Skip collision detection if tile is PipeTall or LevelDoor
            if (tile->has<CAnimation>()) {
                std::string tileAnimName = tile->get<CAnimation>().animation.getName();
                if (tileAnimName == "PipeTall" || tileAnimName == "LevelDoor") {
                    continue; // Skip this tile
                }
            }

            auto& tileTrans = tile->get<CTransform>();
            auto& tileBB = tile->get<CBoundingBox>();
            sf::FloatRect tileRect = tileBB.getRect(tileTrans.pos);

            if (blackHoleRect.intersects(tileRect)) {
                // Get the black hole's animation name to determine its type
                std::string blackHoleAnimName = blackHole->get<CAnimation>().animation.getName();
                
                // Protect tiles beyond x=3744 only in the emperor room level
                bool protectedTile = (m_levelPath.find("future_rome_level_4_emperor_room") != std::string::npos && (tileTrans.pos.x < 400 || tileTrans.pos.x > 3600 ||  tileTrans.pos.y <-550));
                if (protectedTile) {
                    continue;
                }
                
                // std::cout << "[DEBUG] Black hole destroyed a tile at position (" 
                //           << tileTrans.pos.x << "," << tileTrans.pos.y << ")\n";
                
                auto& tileAnim = tile->get<CAnimation>().animation;
                std::string animName = tileAnim.getName();
                m_spawner->createBlockFragments(tileTrans.pos, animName);
                tile->destroy();
                
                break; // Move to the next black hole after handling one tile collision
            }
        }
        
        // Also check for player collision
        for (auto& player : m_entityManager.getEntities("player")) {
            if (!player->has<CTransform>() || !player->has<CBoundingBox>())
                continue;

            auto& playerTrans = player->get<CTransform>();
            auto& playerBB = player->get<CBoundingBox>();
            sf::FloatRect playerRect = playerBB.getRect(playerTrans.pos);

            if (blackHoleRect.intersects(playerRect)) {
                // When player collides with black hole, instant death
                if (player->has<CHealth>()) {
                    auto& health = player->get<CHealth>();
                    
                    // Force health to 0 for instant death
                    health.currentHealth = 0;
                }

                // Destroy the black hole after hitting player
                blackHole->destroy();
                break;
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
                // std::cout << "[DEBUG] Player sword hit tile!\n";
                std::string animName = tileAnim.getName();
                if (animName.find("Box") != std::string::npos) {
                    m_spawner->createBlockFragments(tileTransform.pos, animName);
                    m_spawner->spawnItem(tileTransform.pos, animName);
                    tile->destroy();
                    // std::cout << "[DEBUG] " << animName << " broken by player's sword!\n";
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
                // std::cout << "[DEBUG] Player sword hit enemy!\n";
                
                // Direct check for Emperor
                bool isEmperor = enemy->has<CEnemyAI>() && 
                                enemy->get<CEnemyAI>().enemyType == EnemyType::Emperor;
                
                // Apply damage with protection for Emperor
                if (enemy->has<CHealth>()) {
                    auto& health = enemy->get<CHealth>();
                    if (health.invulnerabilityTimer > 0.f) {
                        continue; // Skip damage if currently invincible
                    }
                    int damage = PLAYER_SWORD_DAMAGE;
                    
                    // Reduce damage in Future world without FutureArmor
                    if (m_game.worldType == "Future" || (m_game.worldType == "Alien" && enemy->has<CEnemyAI>() && enemy->get<CEnemyAI>().enemyType == EnemyType::Fast)) {
                        auto players = m_entityManager.getEntities("player");
                        if (!players.empty() && players.front()->has<CPlayerEquipment>()) {
                            if (!players.front()->get<CPlayerEquipment>().hasFutureArmor) {
                                damage = static_cast<int>(damage / 3.f);
                                // std::cout << "[DEBUG] Future world + no FutureArmor: sword damage reduced to " 
                                //         << damage << "\n";
                            }
                        }
                    }
                    
                    // For Emperor, ensure health never goes below 1
                    // APPLY damage and invincibility differently if Emperor
                    if (isEmperor) {
                        int newHealth = health.currentHealth - damage;
                        if (newHealth < 1) newHealth = 1;
                        health.currentHealth = newHealth;
                        health.invulnerabilityTimer = 1.f; // Short invincibility
                        // std::cout << "[DEBUG] Emperor took damage. New Health: " << health.currentHealth << "\n";
                    } else {
                        health.takeDamage(damage);
                        health.invulnerabilityTimer = PLAYER_SWORD_INVULNERABILITY_TIME;
                        if (!health.isAlive()) {
                            enemy->destroy();
                        }
                    }
                }
            }
        }
    }
    
    // Enemy sword collisions
    for (auto& enemySword : m_entityManager.getEntities("enemySword")) {
        if (!enemySword->has<CTransform>() || !enemySword->has<CBoundingBox>())
            continue;

        auto& swTrans  = enemySword->get<CTransform>();
        auto& swBB     = enemySword->get<CBoundingBox>();
        sf::FloatRect swordRect = swBB.getRect(swTrans.pos);

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
                    // std::cout << "[DEBUG] Super enemy sword destroyed a tile!\n";
                    shouldDestroyTile = true;  // Super enemy destroys any tile
                } else if (tileAnim.getName().find("Box") != std::string::npos) {
                    // std::cout << "[DEBUG] Non-super enemy sword destroyed a tile containing 'Box'!\n";
                    shouldDestroyTile = true;  // Other enemies destroy only tiles containing "Box"
                }
            }

            if (shouldDestroyTile) {
                // Create block fragments & spawn item
                std::string animName = tileAnim.getName();
                // std::cout << "[DEBUG] Spawning black hole!!!\n";
                m_spawner->createBlockFragments(tileTransform.pos, animName);
                tile->destroy();  // Destroy tile
            }

            enemySword->destroy(); // Destroy the sword on impact
            break; // Exit loop after handling collision
        }
        
        //Collisions with Player
        for (auto& player : m_entityManager.getEntities("player")) {
            if (!player->has<CTransform>() || !player->has<CBoundingBox>())
                continue;
            
            auto& playerTrans = player->get<CTransform>();
            auto& playerBB    = player->get<CBoundingBox>();
            sf::FloatRect playerRect = playerBB.getRect(playerTrans.pos);

            // If player is defending, ignore damage
            if (player->has<CState>()) {
                auto& st = player->get<CState>();
                if (st.state == "defense") {
                    //std::cout << "[DEBUG] Player in defense, ignoring Emperor sword damage.\n";
                    break;
                }
            }
            if (swordRect.intersects(playerRect)) {
                if (player->has<CHealth>()) {
                    auto& health = player->get<CHealth>();
                    health.takeDamage(enemySword->get<CEnemyAI>().damage);
                    // std::cout << "[DEBUG] Enemy sword hit player! Damage: " << enemySword->get<CEnemyAI>().damage << "\n";
                }
                enemySword->destroy(); // Destroy sword after hit
                break; 
            }
        }
        
        // Enemy sword vs other enemies
        for (auto& otherEnemy : m_entityManager.getEntities("enemy")) {
            if (!otherEnemy->has<CTransform>() || !otherEnemy->has<CBoundingBox>() || otherEnemy->get<CEnemyAI>().enemyType == EnemyType::Super)
                continue;
            
            auto& enemyTrans = otherEnemy->get<CTransform>();
            auto& enemyBB = otherEnemy->get<CBoundingBox>();
            sf::FloatRect enemyRect = enemyBB.getRect(enemyTrans.pos);
            
            if (swordRect.intersects(enemyRect)) {
                // Direct check for Emperor
                bool isEmperor = otherEnemy->has<CEnemyAI>() && 
                                otherEnemy->get<CEnemyAI>().enemyType == EnemyType::Emperor;
                
                if (isEmperor) {
                    // std::cout << "[DEBUG] Emperor is immortal - limiting sword damage!\n";
                }

                // Get the enemy that created this sword
                int creatorId = -1;
                if (enemySword->has<CState>()) {
                    try {
                        creatorId = std::stoi(enemySword->get<CState>().state);
                    } catch (...) {
                        // Invalid ID format, continue
                    }
                }
                
                // Don't let enemy's own sword hit itself
                if (static_cast<size_t>(creatorId) != otherEnemy->id()) {
                    // std::cout << "[DEBUG] Enemy sword hit another enemy! Sword: " 
                    //         << enemySword->id() << " Hit Enemy: " << otherEnemy->id() << "\n";
                    
                    // Apply damage if needed
                    if (otherEnemy->has<CHealth>() && enemySword->has<CEnemyAI>()) {
                        auto& health = otherEnemy->get<CHealth>();
                        auto& swordAI = enemySword->get<CEnemyAI>();
                        
                        // Check if Emperor and protect health
                        if (isEmperor) {
                            int newHealth = health.currentHealth - swordAI.damage;
                            if (newHealth < 1) newHealth = 1;
                            health.currentHealth = newHealth;
                        } else {
                            health.takeDamage(swordAI.damage);
                            if (!health.isAlive()) {
                                otherEnemy->destroy();
                            }
                        }
                    }
                    
                    // Destroy the sword after hitting another enemy
                    enemySword->destroy();
                    break; // Exit this enemy loop
                }
            }
        }
    }

    // Emperor sword collisions
    for (auto& empSword : m_entityManager.getEntities("EmperorSword")) {
        // Check for required components
        if (!empSword->has<CTransform>() || !empSword->has<CBoundingBox>())
            continue;

        auto& swTrans = empSword->get<CTransform>();
        auto& swBB    = empSword->get<CBoundingBox>();

        sf::FloatRect swordRect = swBB.getRect(swTrans.pos);

        // EmperorSword vs Player
        for (auto& player : m_entityManager.getEntities("player")) {
            if (!player->has<CTransform>() || !player->has<CBoundingBox>())
                continue;

            auto& pTrans = player->get<CTransform>();
            auto& pBB    = player->get<CBoundingBox>();
            sf::FloatRect playerRect = pBB.getRect(pTrans.pos);

            if (!swordRect.intersects(playerRect))
                continue;

            // If player is defending, ignore damage
            if (player->has<CState>()) {
                auto& st = player->get<CState>();
                if (st.state == "defense") {
                    //std::cout << "[DEBUG] Player in defense, ignoring Emperor sword damage.\n";
                    break;
                }
            }

            // Damage to player
            if (player->has<CHealth>()) {
                auto& health = player->get<CHealth>();
                int empSwordDamage = 10; // default
                if (empSword->has<CEnemyAI>()) {
                    empSwordDamage = empSword->get<CEnemyAI>().damage;
                }
                health.takeDamage(empSwordDamage);
                health.invulnerabilityTimer = PLAYER_HIT_INVULNERABILITY_TIME;
                // std::cout << "[DEBUG] Player hit by emperor sword! Damage: " 
                //         << empSwordDamage 
                //         << " Health: " << health.currentHealth << "\n";
            }

            // Destroy sword on impact
            empSword->destroy();
            break; // Exit player loop
        }

        // EmperorSword vs tile
        for (auto& tile : m_entityManager.getEntities("tile")) {
            if (!tile->has<CTransform>() || !tile->has<CBoundingBox>())
                continue;

            auto& tileTransform = tile->get<CTransform>();
            auto& tileBB        = tile->get<CBoundingBox>();
            sf::FloatRect tileRect = tileBB.getRect(tileTransform.pos);

            if (!swordRect.intersects(tileRect))
                continue;

            empSword->destroy();
            break; 
        }
    }

    // Radial Emperor Armor sword collisions 
    for (auto& empSword : m_entityManager.getEntities("EmperorSwordArmor")) {
        // Check for required components
        if (!empSword->has<CTransform>() || !empSword->has<CBoundingBox>())
            continue;

        auto& swTrans = empSword->get<CTransform>();
        auto& swBB    = empSword->get<CBoundingBox>();

        sf::FloatRect swordRect = swBB.getRect(swTrans.pos);
        // EmperorArmorSwordRadial vs Player
        for (auto& player : m_entityManager.getEntities("player")) {
            if (!player->has<CTransform>() || !player->has<CBoundingBox>())
                continue;

            auto& pTrans = player->get<CTransform>();
            auto& pBB    = player->get<CBoundingBox>();
            sf::FloatRect playerRect = pBB.getRect(pTrans.pos);

            if (!swordRect.intersects(playerRect))
                continue;

            // If player is defending, ignore damage
            if (player->has<CState>()) {
                auto& st = player->get<CState>();
                if (st.state == "defense") {
                    //std::cout << "[DEBUG] Player in defense, ignoring Emperor sword damage.\n";
                    break;
                }
            }

            // Damage to player
            if (player->has<CHealth>()) {
                auto& health = player->get<CHealth>();
                int empSwordDamage = 1; // default
                health.takeDamage(empSwordDamage);
                health.invulnerabilityTimer = PLAYER_HIT_INVULNERABILITY_TIME;
                // std::cout << "[DEBUG] Player hit by emperor sword! Damage: " 
                //         << empSwordDamage 
                //         << " Health: " << health.currentHealth << "\n";
            }
        }
    }
    
    // Radial Emperor sword collisions 
    for (auto& empSword : m_entityManager.getEntities("EmperorSwordRadial")) {
        // Check for required components
        if (!empSword->has<CTransform>() || !empSword->has<CBoundingBox>())
            continue;

        auto& swTrans = empSword->get<CTransform>();
        auto& swBB    = empSword->get<CBoundingBox>();

        sf::FloatRect swordRect = swBB.getRect(swTrans.pos);

        // EmperorSwordRadial vs Player
        for (auto& player : m_entityManager.getEntities("player")) {
            if (!player->has<CTransform>() || !player->has<CBoundingBox>())
                continue;

            auto& pTrans = player->get<CTransform>();
            auto& pBB    = player->get<CBoundingBox>();
            sf::FloatRect playerRect = pBB.getRect(pTrans.pos);

            if (!swordRect.intersects(playerRect))
                continue;

            // If player is defending, ignore damage
            if (player->has<CState>()) {
                auto& st = player->get<CState>();
                if (st.state == "defense") {
                    //std::cout << "[DEBUG] Player in defense, ignoring Emperor sword damage.\n";
                    break;
                }
            }

            // Damage to player
            if (player->has<CHealth>()) {
                auto& health = player->get<CHealth>();
                int empSwordDamage = 10; // default
                if (empSword->has<CEnemyAI>()) {
                    empSwordDamage = empSword->get<CEnemyAI>().radialAttackDamage;
                }
                health.takeDamage(empSwordDamage);
                health.invulnerabilityTimer = PLAYER_HIT_INVULNERABILITY_TIME;
                // std::cout << "[DEBUG] Player hit by emperor sword! Damage: " 
                //         << empSwordDamage 
                //        << " Health: " << health.currentHealth << "\n";
            }

            // Destroy sword on impact
            empSword->destroy();
            break; // Exit player loop
        }

        // EmperorSwordRadial vs tile
        for (auto& tile : m_entityManager.getEntities("tile")) {
            if (!tile->has<CTransform>() || !tile->has<CBoundingBox>())
                continue;

            auto& tileTransform = tile->get<CTransform>();
            auto& tileBB        = tile->get<CBoundingBox>();
            sf::FloatRect tileRect = tileBB.getRect(tileTransform.pos);

            if (!swordRect.intersects(tileRect))
                continue;

            empSword->destroy();
            break; 
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
                    } else if (itemType.find("Big") != std::string::npos) {
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

                else if (itemType.find("Chicken") != std::string::npos) {
                    auto& playerState = player->get<CState>();
                
                    if (itemType.find("Small") != std::string::npos) {
                        // Aggiunge 5 secondi alla stamina dello scudo
                        playerState.shieldStamina += CollisionSystem::SMALLCHICKEN_POINTS;
                        // std::cout << "[DEBUG] Player picked up ChickenSmall: +5s shield stamina.\n";
                    } 
                    else if (itemType.find("Big") != std::string::npos) {
                        // Aggiunge 10 secondi alla stamina dello scudo
                        playerState.shieldStamina += CollisionSystem::BIGCHICKEN_POINTS;
                        // std::cout << "[DEBUG] Player picked up ChickenBig: +10s shield stamina.\n";
                    }
                
                    // Se la stamina supera il limite, clamp al massimo
                    if (playerState.shieldStamina > playerState.maxshieldStamina) {
                        playerState.shieldStamina = playerState.maxshieldStamina;
                    }
                }
            
                item->destroy();
            }
        }
    }
}
