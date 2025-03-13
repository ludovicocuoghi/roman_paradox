#include "EnemyAISystem.h"
#include "SpriteUtils.h"
#include "Spawner.h"
#include <cmath>
#include <algorithm>
#include <iostream>

// ----------------------------------------------------------------------
// 1) Utility Functions (unchanged)
// ----------------------------------------------------------------------

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

static bool lineIntersectsRect(const Vec2<float>& start,
                               const Vec2<float>& end,
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

// ----------------------------------------------------------------------
// 2) EnemyAISystem Constructor
// ----------------------------------------------------------------------

EnemyAISystem::EnemyAISystem(EntityManager& entityManager,
                             Spawner& spawner,
                             GameEngine& game)
    : m_entityManager(entityManager),
      m_spawner(&spawner),
      m_game(game)
{
}

// ----------------------------------------------------------------------
// 3) update() - The Main Logic
// ----------------------------------------------------------------------

void EnemyAISystem::update(float deltaTime)
{
    // Early checks: No enemies or players -> Nothing to do
    auto enemies = m_entityManager.getEntities("enemy");
    if (enemies.empty()) return;

    auto players = m_entityManager.getEntities("player");
    if (players.empty()) return;

    auto& player      = players[0];
    auto& playerTrans = player->get<CTransform>();

    // ============================================================
    // Loop Through All Enemies
    // ============================================================
    for (auto& enemy : enemies) {
        // Must have these components or skip
        if (!enemy->has<CTransform>() ||
            !enemy->has<CEnemyAI>()  ||
            !enemy->has<CAnimation>())
        {
            continue;
        }

        // Shorthand references
        auto& enemyTrans = enemy->get<CTransform>();
        auto& enemyAI    = enemy->get<CEnemyAI>();
        auto& anim       = enemy->get<CAnimation>();
        auto& enemyState = enemy->get<CState>();


        // This flag allows movement but prevents attacking
        bool skipAttack = false;

        // Skip if enemy is in defeated state
        if (enemyAI.enemyState == EnemyState::Defeated) {
            // Keep the enemy in place and don't process any further AI logic
            enemyTrans.velocity.x = 0.f;
            enemyTrans.velocity.y = 0.f;
            continue; // Skip to the next enemy
        }

        // ----------------------------------------------------
        // 1) Forced Cooldown Check
        // ----------------------------------------------------
        if (enemyAI.isInForcedCooldown) {
            enemyAI.forcedCooldownTimer -= deltaTime;
            if (enemyAI.forcedCooldownTimer <= 0.f) {
                // End forced cooldown
                enemyAI.forcedCooldownTimer = 0.f;
                enemyAI.consecutiveAttacks  = 0;
                enemyAI.isInForcedCooldown  = false;

                std::cout << "[DEBUG] Enemy " << enemy->id()
                          << " forced cooldown ended.\n";
            } else {
                // Still in forced cooldown:
                // can move, but cannot attack
                skipAttack = true;
            }
            }
            // ----------------------------------------------------
            // 2) Emperor-Specific Logic
            // ----------------------------------------------------
            if (enemyAI.enemyType == EnemyType::Emperor)
            {
                float dx = playerTrans.pos.x - enemyTrans.pos.x;
                float dy = playerTrans.pos.y - enemyTrans.pos.y;
                float distance = std::sqrt(dx*dx + dy*dy);

                float healthPercentage = 1.f;
                if (enemy->has<CHealth>()) {
                    auto& health = enemy->get<CHealth>();
                    healthPercentage = static_cast<float>(health.currentHealth) / static_cast<float>(health.maxHealth);
                }

                // Always update facing direction first (unless in final attack)
                if (enemyAI.enemyState != EnemyState::FinalAttack) {
                    enemyAI.facingDirection = (dx < 0) ? -1.0f : 1.0f;
                }

                // Check if health is below threshold to trigger final attack for Future Emperor
                if (m_game.worldType == "Future" && healthPercentage <= 0.3f && 
                    enemyAI.enemyState != EnemyState::FinalAttack && 
                    enemyAI.enemyState != EnemyState::Defeated) {
                    
                    // Initialize final attack state
                    enemyAI.enemyState = EnemyState::FinalAttack;
                    enemyAI.finalBurstTimer = 0.f;
                    enemyAI.burstCount = 0;
                    enemyAI.phaseTimer = 0.f;
                    
                    // Prevent instant death if health is very low
                    if (enemy->has<CHealth>()) {
                        auto& health = enemy->get<CHealth>();
                        if (health.currentHealth < 2) {
                            health.currentHealth = 2;
                        }
                    }
                    
                    std::cout << "[DEBUG] Emperor entering final attack state! Health: " << healthPercentage << "\n";
                }

                // Future Emperor final attack logic
                if (m_game.worldType == "Future" && enemyAI.enemyState == EnemyState::FinalAttack) {
                    // Always stop movement during final attack
                    enemyTrans.velocity.x = 0.f;
                    enemyTrans.velocity.y = 0.f;

                    // Update phase timer
                    enemyAI.phaseTimer += deltaTime;
                    
                    // Use burstCount to track phases
                    if (enemyAI.burstCount == 0) {
                        // Phase 0: Initial teleport (happens once)
                        float screenWidth = m_game.window().getSize().x;
                        float screenHeight = m_game.window().getSize().y;

                        std::cout << screenWidth << screenHeight << std::endl;
                        
                        // Position in center of screen
                        enemyTrans.pos = Vec2<float>(5000, -1000);
                        enemyAI.burstCount = 1; // Mark teleport as complete
                        
                        if (enemy->has<CAnimation>()) {
                            auto& animation = enemy->get<CAnimation>();
                            std::string standAnim = "FutureStandEmperor3";
                            if (m_game.assets().hasAnimation(standAnim)) {
                                animation.animation = m_game.assets().getAnimation(standAnim);
                            }
                        }
                        
                        std::cout << "[DEBUG] TELEPORT COMPLETE: Emperor teleported to center position\n";
                    }
                    else if (enemyAI.burstCount == 1 && enemyAI.phaseTimer >= 1.0f) {
                        // Phase 1: Fire the black hole after charging
                        enemyAI.burstCount = 2; // Mark as fired
                        enemyAI.phaseTimer = 0.f; // Reset timer for death countdown
                        
                        // Get player position
                        Vec2<float> playerPos;
                        auto players = m_entityManager.getEntities("player");
                        if (!players.empty() && players[0]->has<CTransform>()) {
                            playerPos = players[0]->get<CTransform>().pos;
                        } else {
                            playerPos = Vec2<float>(m_game.window().getSize().x * 0.5f, 
                                                m_game.window().getSize().y * 0.5f);
                        }
                        
                        // Calculate direction to player
                        Vec2<float> direction = playerPos - enemyTrans.pos;
                        float dist = std::sqrt(direction.x * direction.x + direction.y * direction.y);
                        
                        // Normalize direction
                        if (dist > 0) {
                            direction.x /= dist;
                            direction.y /= dist;
                        }
                        
                        // Spawn central black hole
                        std::string animName = "AlienBlackHoleRedBig";
                        if (m_game.assets().hasAnimation(animName)) {
                            // Create massive black hole
                            auto massiveBlackHole = m_entityManager.addEntity("emperorBlackHole");
                            massiveBlackHole->add<CTransform>(enemyTrans.pos);
                            massiveBlackHole->add<CLifeSpan>(30.0f);
                            massiveBlackHole->add<CState>(std::to_string(enemy->id()));
                            
                            // Animation setup
                            auto& blackHoleAnim = m_game.assets().getAnimation(animName);
                            massiveBlackHole->add<CAnimation>(blackHoleAnim, true);
                            
                            // Configure size
                            sf::Vector2i animSize = blackHoleAnim.getSize();
                            Vec2<float> boxSize(animSize.x * 3.1, animSize.y * 3.1);
                            Vec2<float> halfSize(boxSize.x * 0.5f, boxSize.y * 0.5f);
                            
                            // Scale the sprite - increase size for final attack
                            auto& sprite = massiveBlackHole->get<CAnimation>().animation.getMutableSprite();
                            float scale_int = 8.5f;
                            sprite.setScale(scale_int, scale_int);
                            massiveBlackHole->add<CBoundingBox>(boxSize, halfSize);
                            
                            // Set velocity
                            float blackHoleSpeed = EMPEROR_RADIAL_BULLETS_SPEED * 0.5f;
                            massiveBlackHole->get<CTransform>().velocity = Vec2<float>(
                                direction.x * blackHoleSpeed,
                                direction.y * blackHoleSpeed
                            );
                            
                            std::cout << "[DEBUG] FIRING PHASE: Emperor launched massive black hole attack!\n";
                        } else {
                            std::cout << "[ERROR] Missing animation: " << animName << " for black hole!\n";
                        }
                    }
                    else if (enemyAI.burstCount == 2 && enemyAI.phaseTimer >= 4.0f) {
                        // Phase 2: Kill the Emperor after waiting period
                        if (enemy->has<CHealth>()) {
                            auto& health = enemy->get<CHealth>();
                            health.currentHealth = 0;
                        }
                        
                        enemyAI.enemyState = EnemyState::Defeated;
                        
                        std::cout << "[DEBUG] DEATH PHASE: Emperor defeated after final attack!\n";
                    }
                    
                    // Skip the rest of the logic when in final attack mode
                    return;
                }
                // Ancient Emperor final attack logic
                else if (m_game.worldType != "Future" && healthPercentage <= 0.1f) {
                    // ANCIENT EMPEROR FINAL ATTACK - Original implementation
                    if (enemyAI.enemyState != EnemyState::FinalAttack) {
                        enemyAI.enemyState = EnemyState::FinalAttack;
                        enemyAI.finalBurstTimer = 0.f;
                        enemyAI.burstCount = 0;
                        
                        std::cout << "[DEBUG] Ancient Emperor entering final attack state!\n";
                    }
                    
                    enemyTrans.velocity.x = 0.f;
                    enemyTrans.velocity.y = 0.f;
                    
                    enemyAI.finalBurstTimer += deltaTime;
                    if (enemyAI.finalBurstTimer >= 0.2f) {
                        enemyAI.finalBurstTimer = 0.f;
                        
                        // Ancient Emperor uses swords
                        m_spawner->spawnEmperorSwordsRadial(
                            enemy,
                            EMPEROR_RADIAL_SWORDS_COUNT * 2,
                            EMPEROR_RADIAL_SWORDS_RADIUS,
                            EMPEROR_RADIAL_SWORDS_SPEED
                        );
                        
                        enemyAI.burstCount++;
                        
                        if (enemyAI.burstCount >= 12) {
                            // Clear all projectiles
                            auto enemySwords = m_entityManager.getEntities("EmperorSword");
                            for (auto& sword : enemySwords) {
                                sword->destroy();
                            }
                            
                            float tileSize = 96;
                            enemyTrans.pos.x -= enemyAI.facingDirection * 4 * tileSize;
                            
                            // Final attack is different based on world type
                            const int swordsperburst = EMPEROR_RADIAL_SWORDS_COUNT*2;
                            const float radius = 100.f;
                            const float speed = 500.f;
                            const float baseStopTime = 0.1f;
                            
                            for (int burst = 0; burst < 15; ++burst) {
                                float stopTimeIncrement = 0.1f + 0.1f * burst;
                                
                                // Ancient Emperor uses sword armor
                                m_spawner->spawnEmperorSwordArmorRadial(
                                    enemy, 
                                    swordsperburst, 
                                    radius, 
                                    speed, 
                                    baseStopTime, 
                                    stopTimeIncrement
                                );
                            }
                            
                            if (enemy->has<CHealth>()) {
                                auto& health = enemy->get<CHealth>();
                                health.currentHealth = 0;
                            }
                            
                            enemyAI.enemyState = EnemyState::Defeated;
                            enemyTrans.velocity.x = 0.f;
                            enemyTrans.velocity.y = 0.f;
                        }
                    }
                    return;
                } // Simple movement logic - approach if too far, back away if too close
        if (enemyAI.enemyState != EnemyState::FinalAttack && 
            enemyAI.enemyState != EnemyState::Defeated) {
            
            const float MIN_DISTANCE = 350.f;
            const float MAX_DISTANCE = 500.f;
            const float MOVEMENT_SPEED = 100.f;
            
            // Prevent falling below a certain height
            const float MIN_HEIGHT = 100.f; // Minimum height from the bottom of the screen
            float groundLevel = m_game.window().getSize().y - MIN_HEIGHT;
            
            // If Emperor is falling below minimum height, push back up
            if (enemyTrans.pos.y > groundLevel) {
                enemyTrans.pos.y = groundLevel;
                enemyTrans.velocity.y = -50.f; // Small upward velocity to push back up
                std::cout << "[DEBUG] Emperor hit minimum height boundary, repositioning\n";
            }
            
            // Horizontal movement logic
            if (distance < MIN_DISTANCE) {
                // Too close - back away
                enemyTrans.velocity.x = -enemyAI.facingDirection * MOVEMENT_SPEED;
                
                // Use Run animation
                if (enemy->has<CAnimation>()) {
                    auto& animation = enemy->get<CAnimation>();
                    std::string runAnim;
                    
                    if (m_game.worldType == "Future") {
                        if (healthPercentage > 0.7f) {
                            runAnim = "FutureRunEmperor";
                        } else if (healthPercentage <= 0.7f && healthPercentage > 0.3f) {
                            runAnim = "FutureRunEmperor2";
                        } else {
                            runAnim = "FutureRunEmperor3";
                        }
                        
                        if (m_game.assets().hasAnimation(runAnim)) {
                            animation.animation = m_game.assets().getAnimation(runAnim);
                        }
                    }
                }
            }
            else if (distance > MAX_DISTANCE) {
                // Too far - move closer
                enemyTrans.velocity.x = enemyAI.facingDirection * MOVEMENT_SPEED;
                
                // Use Run animation
                if (enemy->has<CAnimation>()) {
                    auto& animation = enemy->get<CAnimation>();
                    std::string runAnim;
                    
                    if (m_game.worldType == "Future") {
                        if (healthPercentage > 0.7f) {
                            runAnim = "FutureRunEmperor";
                        } else if (healthPercentage <= 0.7f && healthPercentage > 0.3f) {
                            runAnim = "FutureRunEmperor2";
                        } else {
                            runAnim = "FutureRunEmperor3";
                        }
                        
                        if (m_game.assets().hasAnimation(runAnim)) {
                            animation.animation = m_game.assets().getAnimation(runAnim);
                        }
                    }
                }
            }
            else {
                // At proper distance - stand still
                enemyTrans.velocity.x = 0.f;
                
                // Use Stand animation when not moving
                if (enemy->has<CAnimation>() && std::abs(enemyTrans.velocity.x) < 0.1f) {
                    auto& animation = enemy->get<CAnimation>();
                    std::string standAnim;
                    
                    if (m_game.worldType == "Future") {
                        if (healthPercentage > 0.7f) {
                            standAnim = "FutureStandEmperor";
                        } else if (healthPercentage <= 0.7f && healthPercentage > 0.3f) {
                            standAnim = "FutureStandEmperor2";
                        } else {
                            standAnim = "FutureStandEmperor3";
                        }
                        
                        if (m_game.assets().hasAnimation(standAnim)) {
                            animation.animation = m_game.assets().getAnimation(standAnim);
                        }
                    }
                }
            }
            
            // Add floating behavior to keep Emperor airborne
            // This will make the Emperor hover and prevent getting stuck below ground level
            float floatingHeight = 300.f; // Target height from the top of the screen
            float targetY = floatingHeight; 
            float heightDifference = targetY - enemyTrans.pos.y;
            
            // Apply a gentle force to maintain target height
            float verticalAdjustmentForce = heightDifference * 0.1f;
            enemyTrans.velocity.y = verticalAdjustmentForce;
            
            // Cap vertical velocity to prevent extreme movements
            if (enemyTrans.velocity.y > 100.f) enemyTrans.velocity.y = 100.f;
            if (enemyTrans.velocity.y < -100.f) enemyTrans.velocity.y = -100.f;

            // Update phase based on healthPercentage (only for Future world)
            if (m_game.worldType == "Future" && enemy->has<CBossPhase>()) {
                auto& bossPhase = enemy->get<CBossPhase>().phase;
                if (healthPercentage > 0.7f && bossPhase != BossPhase::Phase1) {
                    bossPhase = BossPhase::Phase1;
                } else if (healthPercentage <= 0.7f && healthPercentage > 0.3f && bossPhase != BossPhase::Phase2) {
                    bossPhase = BossPhase::Phase2;
                } else if (healthPercentage <= 0.3f && healthPercentage > 0.2f && bossPhase != BossPhase::Phase3) {
                    bossPhase = BossPhase::Phase3;
                }
            }

            // Normal attacks based on health percentage
            if (enemyAI.enemyState != EnemyState::FinalAttack) {
                if (m_game.worldType == "Future") {
                    // FUTURE EMPEROR ATTACKS
                    // Phase 1: Continuous firing for 3 seconds, then 10-second cooldown
                    if (healthPercentage > 0.7f) {
                        // Track total time in current state (either firing or cooldown)
                        enemyAI.phaseTimer += deltaTime;
                        
                        if (!enemyAI.burstCooldownActive) {
                            // FIRING PHASE - lasts for 3 seconds
                            
                            // Fire a radial burst every 0.2 seconds
                            enemyAI.radialAttackTimer += deltaTime;
                            if (enemyAI.radialAttackTimer >= 0.2f) {
                                enemyAI.radialAttackTimer = 0.f;
                                
                                // Spawn a single radial burst
                                m_spawner->spawnEmperorBulletsRadial(
                                    enemy,
                                    EMPEROR_RADIAL_BULLETS_COUNT,
                                    EMPEROR_RADIAL_BULLETS_RADIUS,
                                    EMPEROR_RADIAL_BULLETS_SPEED,
                                    "Fast" // Blue bullets
                                );
                            }
                            
                            // After 3 seconds of firing, switch to cooldown mode
                            if (enemyAI.phaseTimer >= 3.0f) {
                                enemyAI.phaseTimer = 0.f;
                                enemyAI.burstCooldownActive = true;
                                enemyAI.radialAttackTimer = 0.f; // Reset the attack timer
                                std::cout << "[DEBUG] Emperor Phase 1: 3-second firing complete, entering cooldown\n";
                            }
                        } else {
                            // COOLDOWN PHASE - lasts for 10 seconds
                            
                            // After 10 seconds of cooldown, switch back to firing mode
                            if (enemyAI.phaseTimer >= 10.0f) {
                                enemyAI.phaseTimer = 0.f;
                                enemyAI.burstCooldownActive = false;
                                std::cout << "[DEBUG] Emperor Phase 1: 10-second cooldown ended, resuming firing\n";
                            }
                        }
                    }
                    // Phase 2: Stronger bullets with longer firing period and shorter cooldown (70-50% health)
                    else if (healthPercentage <= 0.8f && healthPercentage > 0.6f) {
                        // Track total time in current state
                        enemyAI.phaseTimer += deltaTime;
                        
                        // Track black hole timer
                        enemyAI.blackHoleTimer += deltaTime;
                        
                        if (!enemyAI.burstCooldownActive) {
                            // FIRING PHASE - lasts for 4 seconds
                            
                            // Fire a radial burst every 0.3 seconds
                            enemyAI.radialAttackTimer += deltaTime;
                            if (enemyAI.radialAttackTimer >= 0.3f) {
                                enemyAI.radialAttackTimer = 0.f;
                                
                                // Spawn a radial burst with more bullets
                                m_spawner->spawnEmperorBulletsRadial(
                                    enemy,
                                    EMPEROR_RADIAL_BULLETS_COUNT * 1.5, // 50% more bullets
                                    EMPEROR_RADIAL_BULLETS_RADIUS,
                                    EMPEROR_RADIAL_BULLETS_SPEED,
                                    "Emperor" 
                                );
                            }
                            
                            // After 4 seconds of firing, switch to cooldown mode
                            if (enemyAI.phaseTimer >= 4.0f) {
                                enemyAI.phaseTimer = 0.f;
                                enemyAI.burstCooldownActive = true;
                                enemyAI.radialAttackTimer = 0.f;
                                std::cout << "[DEBUG] Emperor Phase 2: 4-second firing complete, entering cooldown\n";
                            }
                        } else {
                            // COOLDOWN PHASE - lasts for 8 seconds
                            
                            // After 8 seconds of cooldown, switch back to firing mode
                            if (enemyAI.phaseTimer >= 8.0f) {
                                enemyAI.phaseTimer = 0.f;
                                enemyAI.burstCooldownActive = false;
                                std::cout << "[DEBUG] Emperor Phase 2: 8-second cooldown ended, resuming firing\n";
                            }
                        }
                        
                        // BLACK HOLE ATTACK: 2 directions (0 and 180 degrees) every 30 seconds
                        if (enemyAI.blackHoleTimer >= 30.0f) {
                            enemyAI.blackHoleTimer = 0.f;
                            
                            // Manual spawning for 2 black holes in opposite directions
                            auto& eTrans = enemy->get<CTransform>();
                            float centerX = eTrans.pos.x;
                            float centerY = eTrans.pos.y;
                            float radius = EMPEROR_RADIAL_BULLETS_RADIUS * 0.8f;
                            float blackHoleSpeed = EMPEROR_RADIAL_BULLETS_SPEED * 0.3f;
                            
                            // Spawn black hole at 0 degrees (right)
                            {
                                float angleDeg = 0.f;
                                float angleRad = angleDeg * 3.1415926535f / 180.f;
                                float offsetX = std::cos(angleRad) * radius;
                                float offsetY = std::sin(angleRad) * radius;
                                Vec2<float> spawnPos(centerX + offsetX, centerY + offsetY);
                                
                                auto blackHole = m_entityManager.addEntity("emperorBlackHole");
                                blackHole->add<CTransform>(spawnPos);
                                blackHole->add<CLifeSpan>(5.0f);
                                blackHole->add<CState>(std::to_string(enemy->id()));
                                
                                std::string animName = "AlienBlackHoleAttack";
                                if (m_game.assets().hasAnimation(animName)) {
                                    auto& blackHoleAnim = m_game.assets().getAnimation(animName);
                                    blackHole->add<CAnimation>(blackHoleAnim, true);
                                    
                                    sf::Vector2i animSize = blackHoleAnim.getSize();
                                    Vec2<float> boxSize(animSize.x, animSize.y);
                                    Vec2<float> halfSize(boxSize.x * 0.5f, boxSize.y * 0.5f);
                                    blackHole->add<CBoundingBox>(boxSize, halfSize);
                                }
                                
                                float vx = std::cos(angleRad) * blackHoleSpeed;
                                float vy = std::sin(angleRad) * blackHoleSpeed;
                                blackHole->get<CTransform>().velocity = Vec2<float>(vx, vy);
                            }
                            
                            // Spawn black hole at 180 degrees (left)
                            {
                                float angleDeg = 180.f;
                                float angleRad = angleDeg * 3.1415926535f / 180.f;
                                float offsetX = std::cos(angleRad) * radius;
                                float offsetY = std::sin(angleRad) * radius;
                                Vec2<float> spawnPos(centerX + offsetX, centerY + offsetY);
                                
                                auto blackHole = m_entityManager.addEntity("emperorBlackHole");
                                blackHole->add<CTransform>(spawnPos);
                                blackHole->add<CLifeSpan>(5.0f);
                                blackHole->add<CState>(std::to_string(enemy->id()));
                                
                                std::string animName = "AlienBlackHoleAttack";
                                if (m_game.assets().hasAnimation(animName)) {
                                    auto& blackHoleAnim = m_game.assets().getAnimation(animName);
                                    blackHole->add<CAnimation>(blackHoleAnim, true);
                                    
                                    sf::Vector2i animSize = blackHoleAnim.getSize();
                                    Vec2<float> boxSize(animSize.x, animSize.y);
                                    Vec2<float> halfSize(boxSize.x * 0.5f, boxSize.y * 0.5f);
                                    blackHole->add<CBoundingBox>(boxSize, halfSize);
                                }
                                
                                float vx = std::cos(angleRad) * blackHoleSpeed;
                                float vy = std::sin(angleRad) * blackHoleSpeed;
                                blackHole->get<CTransform>().velocity = Vec2<float>(vx, vy);
                            }
                            
                            std::cout << "[DEBUG] Emperor Phase 2 (70-50%): Black hole attack - 2 directions\n";
                        }
                    }
                    // Phase 3: Alternating bullet types with increased firing rate (50-30% health)
                    else if (healthPercentage <= 0.6f && healthPercentage > 0.5f) {
                        // Track total time in current state
                        enemyAI.phaseTimer += deltaTime;
                        
                        // Track black hole timer
                        enemyAI.blackHoleTimer += deltaTime;
                        
                        if (!enemyAI.burstCooldownActive) {
                            // FIRING PHASE - lasts for 5 seconds
                            
                            // Fire a radial burst every 0.25 seconds
                            enemyAI.radialAttackTimer += deltaTime;
                            if (enemyAI.radialAttackTimer >= 0.25f) {
                                enemyAI.radialAttackTimer = 0.f;
                                
                                // Alternate between Elite (black) and Strong (red) bullets
                                std::string bulletType = (enemyAI.burstCount % 2 == 0) ? "Elite" : "Strong";
                                enemyAI.burstCount++; // Just used for alternating bullet types
                                
                                m_spawner->spawnEmperorBulletsRadial(
                                    enemy,
                                    EMPEROR_RADIAL_BULLETS_COUNT * 1.8, // 80% more bullets
                                    EMPEROR_RADIAL_BULLETS_RADIUS,
                                    EMPEROR_RADIAL_BULLETS_SPEED * 1.1f, // 10% faster bullets
                                    bulletType
                                );
                            }
                            
                            // After 5 seconds of firing, switch to cooldown mode
                            if (enemyAI.phaseTimer >= 5.0f) {
                                enemyAI.phaseTimer = 0.f;
                                enemyAI.burstCooldownActive = true;
                                enemyAI.radialAttackTimer = 0.f;
                                // Keep burstCount as is (don't reset) to maintain bullet type alternation
                                std::cout << "[DEBUG] Emperor Phase 3: 5-second firing complete, entering cooldown\n";
                            }
                        } else {
                            // COOLDOWN PHASE - lasts for 6 seconds
                            
                            // After 6 seconds of cooldown, switch back to firing mode
                            if (enemyAI.phaseTimer >= 6.0f) {
                                enemyAI.phaseTimer = 0.f;
                                enemyAI.burstCooldownActive = false;
                                std::cout << "[DEBUG] Emperor Phase 3: 6-second cooldown ended, resuming firing\n";
                            }
                        }
                        
                        // BLACK HOLE ATTACK: 6 directions every 10 seconds
                        if (enemyAI.blackHoleTimer >= 10.0f) {
                            enemyAI.blackHoleTimer = 0.f;
                            
                            // Spawn 6 black holes in different directions
                            auto& eTrans = enemy->get<CTransform>();
                            float centerX = eTrans.pos.x;
                            float centerY = eTrans.pos.y;
                            float radius = EMPEROR_RADIAL_BULLETS_RADIUS * 0.8f;
                            float blackHoleSpeed = EMPEROR_RADIAL_BULLETS_SPEED * 0.4f;
                            int blackHoleCount = 4;
                            
                            // Generate a random angle offset for this burst (between 0 and 36 degrees)
                            float randomAngleOffset = (std::rand() % 36);
                            
                            for (int i = 0; i < blackHoleCount; i++) {
                                // Calculate angle for radial distribution
                                float angleDeg = (360.f / blackHoleCount) * i + randomAngleOffset;
                                float angleRad = angleDeg * 3.1415926535f / 180.f;
                                
                                // Calculate position offset
                                float offsetX = std::cos(angleRad) * radius;
                                float offsetY = std::sin(angleRad) * radius;
                                Vec2<float> spawnPos(centerX + offsetX, centerY + offsetY);
                                
                                // Create black hole entity
                                auto blackHole = m_entityManager.addEntity("emperorBlackHole");
                                blackHole->add<CTransform>(spawnPos);
                                blackHole->add<CLifeSpan>(5.0f);
                                blackHole->add<CState>(std::to_string(enemy->id()));
                                
                                // Set animation
                                std::string animName = "AlienBlackHoleAttack";
                                if (m_game.assets().hasAnimation(animName)) {
                                    auto& blackHoleAnim = m_game.assets().getAnimation(animName);
                                    blackHole->add<CAnimation>(blackHoleAnim, true);
                                    
                                    sf::Vector2i animSize = blackHoleAnim.getSize();
                                    Vec2<float> boxSize(animSize.x, animSize.y);
                                    Vec2<float> halfSize(boxSize.x * 0.5f, boxSize.y * 0.5f);
                                    blackHole->add<CBoundingBox>(boxSize, halfSize);
                                }
                                
                                // Set velocity
                                float vx = std::cos(angleRad) * blackHoleSpeed;
                                float vy = std::sin(angleRad) * blackHoleSpeed;
                                blackHole->get<CTransform>().velocity = Vec2<float>(vx, vy);
                            }
                            
                            std::cout << "[DEBUG] Emperor Phase 3 (50-30%): Black hole attack - 6 directions\n";
                        }
                    }
                    // Phase 4: Black hole attacks every 10 seconds (50-30% health)
                    else if (healthPercentage <= 0.5f && healthPercentage > 0.3f) {
                        // Track black hole timer
                        enemyAI.blackHoleTimer += deltaTime;
                        
                        // MASSIVE BLACK HOLE ATTACK TARGETED AT PLAYER: Every 10 seconds
                        if (enemyAI.blackHoleTimer >= 5.0f) {
                            enemyAI.blackHoleTimer = 0.f;
                            
                            // Get player position
                            Vec2<float> playerPos;
                            auto players = m_entityManager.getEntities("player");
                            if (!players.empty() && players[0]->has<CTransform>()) {
                                playerPos = players[0]->get<CTransform>().pos;
                            } else {
                                playerPos = Vec2<float>(m_game.window().getSize().x * 0.5f, 
                                                    m_game.window().getSize().y * 0.5f);
                            }
                            
                            // Calculate direction to player
                            auto& enemyTrans = enemy->get<CTransform>();
                            Vec2<float> direction = playerPos - enemyTrans.pos;
                            float dist = std::sqrt(direction.x * direction.x + direction.y * direction.y);
                            
                            // Normalize direction
                            if (dist > 0) {
                                direction.x /= dist;
                                direction.y /= dist;
                            }
                            
                            // Spawn central black hole aimed at player
                            std::string animName = "AlienBlackHoleAttack";
                            if (m_game.assets().hasAnimation(animName)) {
                                // Create massive black hole
                                auto massiveBlackHole = m_entityManager.addEntity("emperorBlackHole");
                                massiveBlackHole->add<CTransform>(enemyTrans.pos);
                                massiveBlackHole->add<CLifeSpan>(10.0f);
                                massiveBlackHole->add<CState>(std::to_string(enemy->id()));
                                
                                // Animation setup
                                auto& blackHoleAnim = m_game.assets().getAnimation(animName);
                                massiveBlackHole->add<CAnimation>(blackHoleAnim, true);
                                
                                // Configure size
                                sf::Vector2i animSize = blackHoleAnim.getSize();
                                Vec2<float> boxSize(animSize.x * 3, animSize.y * 3);
                                Vec2<float> halfSize(boxSize.x * 0.5f, boxSize.y * 0.5f);
                                
                                // Scale the sprite
                                auto& sprite = massiveBlackHole->get<CAnimation>().animation.getMutableSprite();
                                float scale_int = 10.0f;
                                sprite.setScale(scale_int, scale_int);
                                massiveBlackHole->add<CBoundingBox>(boxSize, halfSize);
                                
                                // Set velocity - slow but steady towards player
                                float blackHoleSpeed = EMPEROR_RADIAL_BULLETS_SPEED * 0.3f;
                                massiveBlackHole->get<CTransform>().velocity = Vec2<float>(
                                    direction.x * blackHoleSpeed,
                                    direction.y * blackHoleSpeed
                                );
                                
                                std::cout << "[DEBUG] Emperor Phase 4 (50-30%): Launched massive black hole attack at player!\n";
                            } else {
                                std::cout << "[ERROR] Missing animation: " << animName << " for black hole!\n";
                            }
                        }
                    }
                } else {
                    // ANCIENT EMPEROR ATTACKS
                    if (healthPercentage > 0.7f) {
                        enemyAI.radialAttackTimer += deltaTime;
                        if (enemyAI.radialAttackTimer >= 4.f) {
                            enemyAI.radialAttackTimer = 0.f;
                            m_spawner->spawnEmperorSwordsRadial(
                                enemy,
                                EMPEROR_RADIAL_SWORDS_COUNT,
                                EMPEROR_RADIAL_SWORDS_RADIUS,
                                EMPEROR_RADIAL_SWORDS_SPEED
                            );
                        }
                    }
                    else if (healthPercentage <= 0.7f && healthPercentage > 0.5f) {
                        enemyAI.radialAttackTimer += deltaTime;
                        if (enemyAI.radialAttackTimer >= 3.f) {
                            enemyAI.radialAttackTimer = 0.f;
                            m_spawner->spawnEmperorSwordsRadial(
                                enemy,
                                EMPEROR_RADIAL_SWORDS_COUNT * 2,
                                EMPEROR_RADIAL_SWORDS_RADIUS,
                                EMPEROR_RADIAL_SWORDS_SPEED
                            );
                        }
                    }
                    else if (healthPercentage <= 0.5f) {
                        if (!enemyAI.burstCooldownActive) {
                            float attackInterval = 3.0f; // Default attack interval
                            
                            if (distance > 600.f) {
                                attackInterval = 0.8f; // Fast attacks when player is far away
                            } else if (distance > 50.f && distance < 600.f) {
                                attackInterval = 2.5f; // Medium speed attacks at medium distance
                            }
                            
                            enemyAI.radialAttackTimer += deltaTime;
                            if (enemyAI.radialAttackTimer >= attackInterval) {
                                enemyAI.radialAttackTimer = 0.f;
                                m_spawner->spawnEmperorSwordsRadial(
                                    enemy,
                                    EMPEROR_RADIAL_SWORDS_COUNT,
                                    EMPEROR_RADIAL_SWORDS_RADIUS,
                                    EMPEROR_RADIAL_SWORDS_SPEED
                                );
                                enemyAI.burstCount++;
                            }
                        }
                        else {
                            enemyAI.burstCooldownTimer += deltaTime;
                            if (enemyAI.burstCooldownTimer >= 5.f) {
                                enemyAI.burstCooldownActive = false;
                            }
                        }
                    }
                }

                // Handle melee attack for both emperor types
                if (distance < 100.f && !enemyAI.swordSpawned) {
                    if (m_game.worldType == "Future") {
                        // Future Emperor shoots regular bullets at close range
                        for (int i = 0; i < 3; ++i) {
                            m_spawner->spawnEnemyBullet(enemy);
                        }
                    } else {
                        // Ancient Emperor spawns swords
                        for (int i = 0; i < 3; ++i) {
                            m_spawner->spawnEmperorSwordOffset(enemy);
                        }
                    }
                    enemyAI.swordSpawned = true;
                    }
                }
            }
        }
        // ----------------------------------------------------
        // 3) Knockback Handling
        // ----------------------------------------------------
        if (enemyAI.enemyState == EnemyState::Knockback) {
            std::cout << "[DEBUG] Knockback: vel.x=" << enemyTrans.velocity.x
                      << " pos.x=" << enemyTrans.pos.x
                      << " timer=" << enemyAI.knockbackTimer << "\n";

            // Destroy associated swords
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
                continue; // Skip the rest for this frame
            } else {
                enemyAI.knockbackTimer = 0.f;
                enemyAI.enemyState = EnemyState::Follow; // or Idle
            }
        }

        // ----------------------------------------------------
        // 4) Gravity & Ground Check
        // ----------------------------------------------------
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
                       : DEFAULT_GRAVITY;
            enemyTrans.velocity.y += grav * deltaTime;
            enemyTrans.velocity.y =
                std::clamp(enemyTrans.velocity.y,
                           -MAX_FALL_SPEED,
                            MAX_FALL_SPEED);
        } else {
            enemyTrans.velocity.y = 0.f;
        }

        // ----------------------------------------------------
        // 5) Distance & Visibility
        // ----------------------------------------------------
        float dx       = playerTrans.pos.x - enemyTrans.pos.x;
        float dy       = playerTrans.pos.y - enemyTrans.pos.y;
        float distance = std::sqrt(dx*dx + dy*dy);

        // If player is basically above enemy on same X, switch to Idle
        if (std::fabs(dx) < 1.0f && playerTrans.pos.y < enemyTrans.pos.y) {
            enemyAI.enemyState = EnemyState::Idle;
            enemyTrans.velocity.x = 0.f;
            continue; // Skip further logic
        }

        bool canSeePlayer = checkLineOfSight(enemyTrans.pos,
                                             playerTrans.pos,
                                             m_entityManager);

        bool playerVisible       = (distance < PLAYER_VISIBLE_DISTANCE) && canSeePlayer;
        bool playerVisible_elite = (distance < PLAYER_VISIBLE_DISTANCE) || 
                          (canSeePlayer && distance < PLAYER_VISIBLE_DISTANCE * 2);
        bool playerVisible_emperor = (distance < PLAYER_VISIBLE_DISTANCE * 100);

        bool shouldFollow = false;
        switch (enemyAI.enemyBehavior) {
            case EnemyBehavior::FollowOne:
                shouldFollow = playerVisible;
                break;
            case EnemyBehavior::FollowTwo:
                shouldFollow = playerVisible_elite;
                break;
            case EnemyBehavior::FollowThree:
                shouldFollow = true;  // Always follow, regardless of distance
                break;
            case EnemyBehavior::FollowFour:
                shouldFollow = playerVisible_emperor;
                break;
        }

        // Attack cooldown decrement
        if (enemyAI.attackCooldown > 0.f) {
            enemyAI.attackCooldown -= deltaTime;
            if (enemyAI.attackCooldown < 0.f) {
                enemyAI.attackCooldown = 0.f;
            }
        }

        // Check for tiles in front of Super enemies
        if (enemyAI.enemyType == EnemyType::Super) {
            enemyAI.tileDetected = false; // Reset flag
            
            auto& bb = enemy->get<CBoundingBox>();
            sf::FloatRect enemyRect = bb.getRect(enemyTrans.pos);
            float enemyFrontX = (enemyAI.facingDirection > 0.f)
                                ? enemyRect.left + enemyRect.width
                                : enemyRect.left;
        
            // Check for tile in front
            for (auto& tile : m_entityManager.getEntities("tile")) {
                if (!tile->has<CTransform>() || !tile->has<CBoundingBox>() || !tile->has<CAnimation>())
                    continue;
        
                auto& tileTrans = tile->get<CTransform>();
                auto& tileBB = tile->get<CBoundingBox>();
                auto& tileAnim = tile->get<CAnimation>().animation;
        
                sf::FloatRect tileRect = tileBB.getRect(tileTrans.pos);
        
                // Skip black hole
                std::string animName = tileAnim.getName();
                if (animName.find("AlienBlackHoleAttack") != std::string::npos)
                    continue;
        
                bool isHorizontallyAligned =
                    (tileRect.top < enemyRect.top + enemyRect.height) &&
                    (tileRect.top + tileRect.height > enemyRect.top);
        
                bool isInFront =
                    (enemyAI.facingDirection > 0.f &&
                     tileRect.left <= enemyFrontX &&
                     tileRect.left > enemyRect.left)
                    ||
                    (enemyAI.facingDirection < 0.f &&
                     tileRect.left + tileRect.width >= enemyFrontX &&
                     tileRect.left < enemyRect.left);
        
                if (isHorizontallyAligned && isInFront) {
                    enemyAI.tileDetected = true;
                    std::cout << "[DEBUG] Enemy " << enemy->id()
                              << " detected tile in front!\n";
                    break;
                }
            }
        }

        // ----------------------------------------------------
        // 6) FOLLOW State
        // ----------------------------------------------------
        if (enemyAI.enemyState == EnemyState::Follow) {
            const float xThreshold = 5.0f; // Previene flip ripetuti

            // Esegue il rilevamento dei tile solo se il giocatore è entro 1000 pixel
            if (distance <= 100.f) {
                enemyAI.tileDetected = false; // Reset del flag
                
                auto& bb = enemy->get<CBoundingBox>();
                sf::FloatRect enemyRect = bb.getRect(enemyTrans.pos);
                float enemyFrontX = (enemyAI.facingDirection > 0.f)
                                    ? enemyRect.left + enemyRect.width
                                    : enemyRect.left;
                
                // Controlla i tile davanti al nemico
                for (auto& tile : m_entityManager.getEntities("tile")) {
                    if (!tile->has<CTransform>() || !tile->has<CBoundingBox>() || !tile->has<CAnimation>())
                        continue;
                
                    auto& tileTrans = tile->get<CTransform>();
                    auto& tileBB    = tile->get<CBoundingBox>();
                    auto& tileAnim  = tile->get<CAnimation>().animation;
                
                    sf::FloatRect tileRect = tileBB.getRect(tileTrans.pos);
                
                    // Salta il buco nero
                    std::string animName = tileAnim.getName();
                    if (animName.find("AlienBlackHoleAttack") != std::string::npos)
                        continue;
                
                    bool isHorizontallyAligned =
                        (tileRect.top < enemyRect.top + enemyRect.height) &&
                        (tileRect.top + tileRect.height > enemyRect.top);
                
                    bool isInFront =
                        (enemyAI.facingDirection > 0.f &&
                        tileRect.left <= enemyFrontX &&
                        tileRect.left > enemyRect.left)
                        ||
                        (enemyAI.facingDirection < 0.f &&
                        tileRect.left + tileRect.width >= enemyFrontX &&
                        tileRect.left < enemyRect.left);
                
                    if (isHorizontallyAligned && isInFront) {
                        enemyAI.tileDetected = true;
                        std::cout << "[DEBUG] Enemy " << enemy->id()
                                << " detected tile in front!\n";
                        break;
                    }
                }
            }
            
            // Se il nemico di tipo Super rileva un tile, passa allo stato Attack
            if (enemyAI.enemyType == EnemyType::Super &&
                enemyAI.tileDetected &&
                (enemyAI.enemyState == EnemyState::Follow || enemyAI.enemyState == EnemyState::Idle))
            {
                std::cout << "[DEBUG] EnemySuper " << enemy->id()
                        << " switching to Attack state.\n";
                enemyAI.enemyState = EnemyState::Attack;
                // Imposta l'animazione di attacco, resetta la velocità, ecc.
                if (enemy->has<CAnimation>()) {
                    std::string attackAnimName = m_game.worldType + "Hit" + "EnemySuper";
                    if (m_game.assets().hasAnimation(attackAnimName)) {
                        auto& anim = enemy->get<CAnimation>();
                        anim.animation = m_game.assets().getAnimation(attackAnimName);
                        anim.repeat = false;
                        std::cout << "[DEBUG] Setting attack animation: " << attackAnimName << std::endl;
                        if (enemyAI.facingDirection < 0) {
                            flipSpriteLeft(anim.animation.getMutableSprite());
                        } else {
                            flipSpriteRight(anim.animation.getMutableSprite());
                        }
                    } else {
                        std::cout << "[DEBUG] ERROR: Attack animation not found: " << attackAnimName << std::endl;
                    }
                }
                enemyTrans.velocity.x = 0.f;
                enemyAI.attackTimer = ATTACK_TIMER_DEFAULT;
                enemyAI.swordSpawned = false;
            }
            
            // Logica di movimento - Special logic for Future enemies
            if (m_game.worldType == "Future") {
                // Define optimal shooting range
                const float OPTIMAL_MIN_DISTANCE = 350.0f;
                const float OPTIMAL_MAX_DISTANCE = 550.0f;
                const float TOO_CLOSE_DISTANCE = 200.0f;
                
                if (std::abs(dx) > xThreshold) {
                    // Always face the player
                    enemyAI.facingDirection = (dx > 0.f) ? 1.f : -1.f;
                    
                    // Check if we can see the player (for ranged attacks)
                    bool hasLineOfSight = checkLineOfSight(enemyTrans.pos, 
                                                        playerTrans.pos, 
                                                        m_entityManager);
                    
                    // Decide how to move based on distance
                    if (distance < TOO_CLOSE_DISTANCE) {
                        // Too close - back up!
                        enemyTrans.velocity.x = -enemyAI.facingDirection * FOLLOW_MOVE_SPEED * 0.8f;
                        std::cout << "[DEBUG] Future enemy " << enemy->id() 
                                << " backing away (too close: " << distance << ")\n";
                    }
                    else if (distance < OPTIMAL_MIN_DISTANCE && hasLineOfSight) {
                        // A bit too close but has line of sight - back up slightly
                        enemyTrans.velocity.x = -enemyAI.facingDirection * FOLLOW_MOVE_SPEED * 0.5f;
                        std::cout << "[DEBUG] Future enemy " << enemy->id() 
                                << " backing to optimal range (distance: " << distance << ")\n";
                    }
                    else if (distance > OPTIMAL_MAX_DISTANCE || !hasLineOfSight) {
                        // Too far or no line of sight - approach slowly
                        enemyTrans.velocity.x = enemyAI.facingDirection * FOLLOW_MOVE_SPEED * 0.6f;
                        // std::cout << "[DEBUG] Future enemy " << enemy->id() 
                        //         << " approaching (distance: " << distance 
                        //         << ", LOS: " << (hasLineOfSight ? "yes" : "no") << ")\n";
                    }
                    else {
                        // In optimal range and has line of sight - stop moving and shoot
                        enemyTrans.velocity.x = 0.f;
                        
                        // Set animation to idle or attack animation for Future world
                        std::string idleAnimName;
                        switch (enemyAI.enemyType) {
                            case EnemyType::Fast:    idleAnimName = "FutureStandEnemyFast"; break;
                            case EnemyType::Strong:  idleAnimName = "FutureStandEnemyStrong"; break;
                            case EnemyType::Elite:   idleAnimName = "FutureStandEnemyElite"; break;
                            case EnemyType::Normal:  idleAnimName = "FutureStandEnemyNormal"; break;
                            case EnemyType::Super:   idleAnimName = "FutureStandEnemySuper"; break;
                            case EnemyType::Emperor: idleAnimName = "FutureStandEmperor"; break;
                        }
                        
                        if (m_game.assets().hasAnimation(idleAnimName) && 
                            anim.animation.getName() != idleAnimName) {
                            anim.animation = m_game.assets().getAnimation(idleAnimName);
                            if (enemyAI.facingDirection < 0) {
                                flipSpriteLeft(anim.animation.getMutableSprite());
                            } else {
                                flipSpriteRight(anim.animation.getMutableSprite());
                            }
                        }
                        
                        std::cout << "[DEBUG] Future enemy " << enemy->id() 
                                << " at optimal shooting range (distance: " << distance << ")\n";
                    }
                } else {
                    enemyTrans.velocity.x = 0.f;
                }
            }
            // Original movement logic for non-Future enemies
            else {
                if (std::abs(dx) > xThreshold) {
                    float followSpeed = FOLLOW_MOVE_SPEED;
                    
                    // Reduce speed if player is above enemy AND horizontally close
                    float horizontalProximity = 90.0f; // Adjust this value as needed
                    if (playerTrans.pos.y < enemyTrans.pos.y - 20.0f && std::abs(dx) < horizontalProximity) {
                        followSpeed *= 0.7f; 
                    }
                    
                    enemyAI.facingDirection = (dx > 0.f) ? 1.f : -1.f;
                    enemyTrans.velocity.x = enemyAI.facingDirection * followSpeed;
                } else {
                    enemyTrans.velocity.x = 0.f;
                }
            }
        }

        // ----------------------------------------------------
        // (Animation) Running State
        // ----------------------------------------------------
       
        float healthPercentage = 1.f;
        if (enemy->has<CHealth>()) {
            auto& health = enemy->get<CHealth>();
            healthPercentage = static_cast<float>(health.currentHealth) / static_cast<float>(health.maxHealth);
        }
        
        std::string runAnimName;
        if (enemyAI.enemyType == EnemyType::Emperor && m_game.worldType == "Future") {
            // Determine run animation based on enemy state (phase)
            auto& bossPhase = enemy->get<CBossPhase>().phase;
            std::string desiredAnimName;
            switch (bossPhase) {
                case BossPhase::Phase1:
                    desiredAnimName = "FutureStandEmperor";
                    break;
                case BossPhase::Phase2:
                    desiredAnimName = "FutureStandEmperor2";
                    break;
                case BossPhase::Phase3:
                    desiredAnimName = "FutureStandEmperor3";
                    break;
                default:
                    desiredAnimName = "FutureStandEmperor";
                    break;
            }
        } else {
            switch (enemyAI.enemyType) {
                case EnemyType::Fast:
                    runAnimName = m_game.worldType + "RunEnemyFast";
                    break;
                case EnemyType::Strong:
                    runAnimName = m_game.worldType + "RunEnemyStrong";
                    break;
                case EnemyType::Elite:
                    runAnimName = m_game.worldType + "RunEnemyElite";
                    break;
                case EnemyType::Normal:
                    runAnimName = m_game.worldType + "RunEnemyNormal";
                    break;
                case EnemyType::Super:
                    runAnimName = m_game.worldType + "RunEnemySuper";
                    break;
                case EnemyType::Emperor:
                    runAnimName = m_game.worldType + "RunEmperor";
                    break;
                // Emperor is handled above.
            }
        }
        
        if (anim.animation.getName() != runAnimName) {
            if (m_game.assets().hasAnimation(runAnimName)) {
                std::cout << "Setting run anim: " << runAnimName << std::endl;
                const Animation& runAnim = m_game.assets().getAnimation(runAnimName);
                anim.animation = runAnim;
                anim.animation.reset();
            } else {
                std::cout << "[ERROR] Missing animation: " << runAnimName << " for Emperor enemy!\n";
            }
        }

        // ----------------------------------------------------
        // 7) FUTURE-WORLD Ranged Attacks (Bullets)
        // ----------------------------------------------------
        // In the Future-world ranged attacks section (section 7), replace with:

        if (m_game.worldType == "Future") {
            enemyAI.shootTimer += deltaTime;
            enemyAI.superMoveTimer += deltaTime; // track super move timer

            // (A) Check super move cooldown
            if (enemyAI.superMoveTimer >= enemyAI.superMoveCooldown) {
                enemyAI.superMoveTimer = 0.f;
                enemyAI.superMoveReady = true;
            }

            // Check if we're in burst cooldown
            bool skipShooting = false;
            if (enemyAI.burstCooldownActive) {
                enemyAI.burstCooldownTimer += deltaTime;
                
                // Fixed cooldown of 2.5 seconds after each burst
                const float POST_BURST_COOLDOWN = 1.0f;
                
                if (enemyAI.burstCooldownTimer >= POST_BURST_COOLDOWN) {
                    enemyAI.burstCooldownActive = false;
                    enemyAI.burstCooldownTimer = 0.0f;
                    std::cout << "[DEBUG] Enemy " << enemy->id()
                            << " post-burst cooldown ended.\n";
                } else {
                    // Skip shooting logic while in cooldown, but don't skip movement
                    skipShooting = true;
                }
            }

            // Only process shooting if we're not in cooldown
            if (!skipShooting) {
                // (B) If not bursting
                if (!enemyAI.inBurst) {
                    bool canShoot = checkLineOfSight(enemyTrans.pos,
                                                    playerTrans.pos,
                                                    m_entityManager)
                                    && (distance >= enemyAI.minShootDistance);

                    // If super move ready + can shoot
                    if (enemyAI.superMoveReady && canShoot) {
                        enemyAI.superMoveReady = false;
                        std::cout << "[DEBUG] Enemy " << enemy->id()
                                << " uses SUPER MOVE!\n";

                        // Use superBulletCount from CEnemyAI instead of hardcoded value
                        int superBullets = enemyState.superBulletCount;
                        float angleRange = 30.f; // spread angle

                        for (int i = 0; i < superBullets; ++i) {
                            auto bullet = m_spawner->spawnEnemyBullet(enemy);
                            if (bullet && bullet->has<CTransform>()) {
                                auto& bulletTrans = bullet->get<CTransform>();
                                float step  = angleRange / (superBullets - 1);
                                float angle = -angleRange * 0.5f + step * i;
                                bulletTrans.rotate(angle);
                            }
                        }
                        
                        // Activate cooldown after super move too
                        enemyAI.burstCooldownActive = true;
                        enemyAI.burstCooldownTimer = 0.0f;
                    }
                    // Otherwise do normal burst
                    else if (canShoot &&
                            enemyAI.shootTimer >= enemyAI.shootCooldown)
                    {
                        enemyAI.shootTimer  = 0.f;
                        enemyAI.inBurst     = true;
                        enemyAI.bulletsShot = 0;
                        enemyAI.burstTimer  = 0.f;

                        std::cout << "[DEBUG] Enemy " << enemy->id()
                                << " starts bullet burst (one at a time).\n";
                    }
                }
                // (C) If already bursting
                else {
                    enemyAI.burstTimer += deltaTime;
                    if (enemyAI.burstTimer >= enemyAI.burstInterval) {
                        enemyAI.burstTimer = 0.f;
                        enemyAI.bulletsShot++;

                        auto bullet = m_spawner->spawnEnemyBullet(enemy);
                        
                        std::cout << "[DEBUG] Enemy " << enemy->id()
                                << " fires bullet #" << enemyAI.bulletsShot << "\n";

                        // Slight random angle
                        if (bullet && bullet->has<CTransform>()) {
                            auto& bulletTrans = bullet->get<CTransform>();
                            float angleOffset = -3.f + static_cast<float>(rand() % 6);
                            bulletTrans.rotate(angleOffset);
                        }

                        // Use bulletBurstCount from CEnemyAI instead of bulletCount
                        if (enemyAI.bulletsShot >= enemyState.bulletBurstCount) {
                            enemyAI.inBurst = false;
                            
                            // Activate post-burst cooldown
                            enemyAI.burstCooldownActive = true;
                            enemyAI.burstCooldownTimer = 0.0f;
                            
                            std::cout << "[DEBUG] Burst finished for enemy "
                                    << enemy->id() << ". Starting post-burst cooldown.\n";
                        }
                    }
                }
            } // end of shooting logic
        }

        // ----------------------------------------------------
        // 8) Melee Attack (Non-Future)
        // ----------------------------------------------------
        float currentAttackRange =
            (enemyAI.enemyType == EnemyType::Emperor)
            ? EMPEROR_ATTACK_RANGE
            : ATTACK_RANGE;
        
        bool shouldAttack = (shouldFollow && distance < currentAttackRange)
                   ||
                   (enemyAI.enemyType == EnemyType::Super && 
                    enemyAI.enemyState == EnemyState::BlockedByTile)
                   || 
                   (enemyAI.enemyType == EnemyType::Super && 
                    enemyAI.tileDetected);  // Add this condition
        
        // If skipAttack is true, we skip the logic below
        if (!skipAttack) {
            if (shouldAttack &&
                enemyAI.attackCooldown <= 0.f &&
                enemyAI.enemyState != EnemyState::Attack)
            {
                // (A) Increase consecutiveAttacks
                enemyAI.consecutiveAttacks++;
                std::cout << "[DEBUG] Enemy " << enemy->id()
                          << " consecutiveAttacks = "
                          << enemyAI.consecutiveAttacks << "\n";
        
                // (B) If reached limit -> forced cooldown
                // Use maxConsecutiveSwordAttacks from CEnemyAI
                if (enemyAI.consecutiveAttacks >= enemyState.maxConsecutiveSwordAttacks) {
                    enemyAI.isInForcedCooldown  = true;
                    enemyAI.forcedCooldownTimer = enemyAI.forcedCooldownDuration;
                    std::cout << "[DEBUG] Enemy " << enemy->id()
                              << " forced cooldown started after "
                              << enemyAI.consecutiveAttacks << " attacks.\n";
                    // We let it move still
                }
                else {
                    // (C) Normal sword attack
                    if (m_game.worldType != "Future") {
                        enemyAI.enemyState   = EnemyState::Attack;
                        enemyAI.attackTimer  = ATTACK_TIMER_DEFAULT;
                        enemyAI.swordSpawned = false;
                        std::cout << "[DEBUG] Enemy " << enemy->id()
                                  << " entering Attack state (cooldown="
                                  << enemyAI.attackCooldown << ")\n";
                    }
                }
            }
            // If still following, ensure state = Follow
            else if (shouldFollow &&
                     enemyAI.enemyState != EnemyState::Attack)
            {
                enemyAI.enemyState = EnemyState::Follow;
            }
            else {
                // If player not visible ->Idle
                if (!playerVisible) {
                    if (enemyAI.enemyState == EnemyState::Follow ||
                        enemyAI.enemyState == EnemyState::Attack)
                    {
                        enemyAI.enemyState = EnemyState::Idle;
                    }
                }
            }
        }
        else {
            // If skipAttack == true, no attacking allowed,
            // but we still set follow/idle
            if (shouldFollow &&
                enemyAI.enemyState != EnemyState::Attack)
            {
                enemyAI.enemyState = EnemyState::Follow;
            }
        }

        // ----------------------------------------------------
        // 9) Handling Attack State (Melee)
        // ----------------------------------------------------
        if (enemyAI.enemyState == EnemyState::Attack) {
            enemyTrans.velocity.x  = 0.f;
            enemyAI.attackTimer   -= deltaTime;

            // Choose base anim
            std::string baseAnimName;
            switch (enemyAI.enemyType) {
                case EnemyType::Normal:  baseAnimName = "EnemyNormal";   break;
                case EnemyType::Fast:    baseAnimName = "EnemyFast";     break;
                case EnemyType::Strong:  baseAnimName = "EnemyStrong";   break;
                case EnemyType::Elite:   baseAnimName = "EnemyElite";    break;
                case EnemyType::Super:   baseAnimName = "EnemySuper";    break;
                case EnemyType::Emperor: baseAnimName = "Emperor";       break;
            }

            // Attack animation
            std::string attackAnimName = m_game.worldType + "Hit" + baseAnimName;
            if (anim.animation.getName() != attackAnimName) {
                // std::cout << "[DEBUG] Enemy " << enemy->id()
                //           << " entering Attack animation: "
                //           << attackAnimName << "\n";
                anim.animation = m_game.assets().getAnimation(attackAnimName);
                anim.animation.reset();
                anim.repeat = false;
            }

            // Spawn sword at correct time
            if (!enemyAI.swordSpawned &&
                enemyAI.attackTimer <= SWORD_SPAWN_THRESHOLD)
            {
                // Emperor spawns sword differently
                if (enemyAI.enemyType == EnemyType::Emperor) {
                    float dx       = playerTrans.pos.x - enemyTrans.pos.x;
                    float distance = std::fabs(dx);

                    if (distance < 100.f) {
                        std::cout << "[DEBUG] Emperor spawns static sword (close)!\n";
                        m_spawner->spawnEmperorSwordOffset(enemy);
                    } else {
                        std::cout << "[DEBUG] Emperor spawns horizontal sword!\n";
                        auto sword = m_spawner->spawnEmperorSwordOffset(enemy);
                        if (sword->has<CTransform>()) {
                            auto& swTrans = sword->get<CTransform>();
                            float dirX = (dx > 0.f) ? 1.f : -1.f;
                            swTrans.velocity = Vec2<float>(200.f * dirX, 0.f);
                        }
                    }
                }
                else if (m_game.worldType != "Future") {
                    // Normal sword for non-future
                    m_spawner->spawnEnemySword(enemy);
                }
                enemyAI.swordSpawned = true;

                std::cout << "[DEBUG] Enemy " << enemy->id()
                          << " spawning sword at AttackTimer: "
                          << enemyAI.attackTimer << "\n";
            }

            // End Attack
            if (enemyAI.attackTimer <= 0.f) {
                enemyAI.attackCooldown = ATTACK_COOLDOWN;
                enemyAI.enemyState     = EnemyState::Follow; // Switch back
                enemyAI.attackTimer    = 0.f;

                std::cout << "[DEBUG] Enemy " << enemy->id()
                          << " Attack finished. Switching to Follow.\n";
            }
        }

        // ----------------------------------------------------
        // 10) Idle State
        // ----------------------------------------------------
        if (enemyAI.enemyState == EnemyState::Idle) {
            enemyTrans.velocity.x = 0.f;
            anim.animation.reset();
        }

        // ----------------------------------------------------
        // 11) Update Position + Flip
        // ----------------------------------------------------
        enemyTrans.pos.x += enemyTrans.velocity.x * deltaTime;
        enemyTrans.pos.y += enemyTrans.velocity.y * deltaTime;

        if (enemyAI.facingDirection < 0.f) {
            flipSpriteLeft(anim.animation.getMutableSprite());
        } else {
            flipSpriteRight(anim.animation.getMutableSprite());
        }
    } // End for (enemies)
}
