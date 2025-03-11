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
        
            if (healthPercentage <= 0.1f) {
                if (enemyAI.enemyState != EnemyState::FinalAttack) {
                    enemyAI.enemyState = EnemyState::FinalAttack;
                    enemyAI.finalBurstTimer = 0.f;
                    enemyAI.burstCount = 0;
                }
        
                enemyTrans.velocity.x = 0.f;
                enemyTrans.velocity.y = 0.f;
        
                enemyAI.finalBurstTimer += deltaTime;
                if (enemyAI.finalBurstTimer >= 0.2f) {
                    enemyAI.finalBurstTimer = 0.f;
        
                    m_spawner->spawnEmperorSwordsRadial(enemy,
                                                        EMPEROR_RADIAL_SWORDS_COUNT * 2,
                                                        EMPEROR_RADIAL_SWORDS_RADIUS,
                                                        EMPEROR_RADIAL_SWORDS_SPEED);
                    enemyAI.burstCount++;
                    
                    if (enemyAI.burstCount >= 12) {
                        auto enemySwords = m_entityManager.getEntities("EmperorSword");
                        for (auto& sword : enemySwords) {
                            sword->destroy();
                        }
                        
                        float tileSize = 96;
                        enemyTrans.pos.x -= enemyAI.facingDirection * 4 * tileSize;
        
                        const int swordsPerBurst = 200;
                        const float radius = 100.f;
                        const float swordSpeed = 500.f;
                        const float baseStopTime = 0.1f;
        
                        for (int burst = 0; burst < 15; ++burst) {
                            float stopTimeIncrement = 0.1f + 0.1f * burst;
                            m_spawner->spawnEmperorSwordArmorRadial(enemy, swordsPerBurst, radius, swordSpeed, baseStopTime, stopTimeIncrement);
                        }
                        
                        if (enemy->has<CHealth>()) {
                            auto& health = enemy->get<CHealth>();
                            health.currentHealth = 0;
                        }
                        
                        enemyAI.enemyState = EnemyState::Defeated;
                        enemyTrans.velocity.x = 0.f;
                        enemyTrans.velocity.y = 0.f;
                        return;
                    }
                }
                return;
            }
        
            if (enemyAI.enemyState != EnemyState::FinalAttack) {
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
                else if (healthPercentage <= 50.f) {
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
        
                if (distance < 100.f && !enemyAI.swordSpawned) {
                    for (int i = 0; i < 3; ++i) {
                        m_spawner->spawnEmperorSwordOffset(enemy);
                    }
                    enemyAI.swordSpawned = true;
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
                if (animName.find("AlienBlackHoleVanish") != std::string::npos)
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
                    if (animName.find("AlienBlackHoleVanish") != std::string::npos)
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
                        std::cout << "[DEBUG] Future enemy " << enemy->id() 
                                << " approaching (distance: " << distance 
                                << ", LOS: " << (hasLineOfSight ? "yes" : "no") << ")\n";
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
        std::string runAnimName;
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
        }
        if (anim.animation.getName() != runAnimName) {
            const Animation& runAnim = m_game.assets().getAnimation(runAnimName);
            anim.animation           = runAnim;
            anim.animation.reset();
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
