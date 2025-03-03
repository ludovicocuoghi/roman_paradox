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

        // Nuova variabile per saltare solo la logica di attacco
        bool skipAttack = false;

        // ----------------------------------------------------
        // 1) FORCED COOLDOWN CHECK
        // ----------------------------------------------------
        if (enemyAI.isInForcedCooldown) {
            enemyAI.forcedCooldownTimer -= deltaTime;
            if (enemyAI.forcedCooldownTimer <= 0.f) {
                // Fine cooldown: reset attacchi
                enemyAI.forcedCooldownTimer = 0.f;
                enemyAI.consecutiveAttacks  = 0;
                enemyAI.isInForcedCooldown  = false;

                std::cout << "[DEBUG] Enemy " << enemy->id() 
                          << " forced cooldown ended.\n";
            } else {
                // NEMICO ANCORA IN COOLDOWN:
                // Non può attaccare, ma può muoversi
                skipAttack = true; 
            }
        }

        // ----------------------------------------------------
        // 2) LOGICA SPECIFICA PER L'EMPEROR
        // ----------------------------------------------------
        if (enemyAI.enemyType == EnemyType::Emperor) 
        {
            float dx = playerTrans.pos.x - enemyTrans.pos.x;
            float dy = playerTrans.pos.y - enemyTrans.pos.y;
            float distance = std::sqrt(dx * dx + dy * dy);

            // Fetch Emperor's health correctly
            float healthPercentage = 1.0f;
            if (enemy->has<CHealth>()) {
                auto& health = enemy->get<CHealth>();
                healthPercentage = static_cast<float>(health.currentHealth) / 
                                   static_cast<float>(health.maxHealth);
            }

            std::cout << "[DEBUG] Emperor Health: " << healthPercentage << "\n";

            // ========== FINAL BURST (10% HP) ==========
            if (healthPercentage <= 0.1f) {
                if (enemyAI.enemyState != EnemyState::FinalAttack) {
                    std::cout << "[DEBUG] Emperor entering FINAL ATTACK mode!\n";
                    enemyAI.enemyState = EnemyState::FinalAttack;
                    enemyAI.finalBurstTimer = 0.f;
                    enemyAI.burstCount = 0;
                }

                // Stop all movement
                enemyTrans.velocity.x = 0.f;
                enemyTrans.velocity.y = 0.f;

                // Execute Final Burst
                enemyAI.finalBurstTimer += deltaTime;
                if (enemyAI.finalBurstTimer >= 0.2f) {
                    enemyAI.finalBurstTimer = 0.f;
                    std::cout << "[DEBUG] Emperor FINAL ATTACK: 8x Radial Swords!\n";
                    m_spawner->spawnEmperorSwordsRadial(enemy, 
                                                        EMPEROR_RADIAL_SWORDS_COUNT * 2,
                                                        EMPEROR_RADIAL_SWORDS_RADIUS, 
                                                        EMPEROR_RADIAL_SWORDS_SPEED);

                    enemyAI.burstCount++;
                    std::cout << "[DEBUG] Emperor FINAL ATTACK: Burst #" << enemyAI.burstCount << "\n";

                    if (enemyAI.burstCount >= 25) {  
                        std::cout << "[DEBUG] Emperor FINAL ATTACK FINISHED! No more attacks.\n";
                        auto enemySwords = m_entityManager.getEntities("EmperorSword");
                        for (auto& sword : enemySwords) {
                            sword->destroy();
                        }
                        return;
                    }
                }
                return; // 🔹 No other attacks in Final Attack mode
            }

            // ========== NORMAL ATTACK PHASES ==========
            if (enemyAI.enemyState != EnemyState::FinalAttack) {
                // Radial Attack (Above 70% HP)
                if (healthPercentage > 0.7f) {
                    enemyAI.radialAttackTimer += deltaTime;
                    if (enemyAI.radialAttackTimer >= 4.f) {
                        enemyAI.radialAttackTimer = 0.f;
                        std::cout << "[DEBUG] Emperor Super Attack: Radial Swords (Normal)\n";
                        m_spawner->spawnEmperorSwordsRadial(enemy, 
                                                            EMPEROR_RADIAL_SWORDS_COUNT, 
                                                            EMPEROR_RADIAL_SWORDS_RADIUS, 
                                                            EMPEROR_RADIAL_SWORDS_SPEED);
                    }
                }
                // Enhanced Radial Attack (70% > HP > 50%)
                else if (healthPercentage <= 0.7f && healthPercentage > 0.5f) {
                    enemyAI.radialAttackTimer += deltaTime;
                    if (enemyAI.radialAttackTimer >= 3.f) {
                        enemyAI.radialAttackTimer = 0.f;
                        std::cout << "[DEBUG] Emperor Super Attack: DOUBLE Radial Swords (Enhanced)\n";
                        m_spawner->spawnEmperorSwordsRadial(enemy, 
                                                            EMPEROR_RADIAL_SWORDS_COUNT * 2, 
                                                            EMPEROR_RADIAL_SWORDS_RADIUS, 
                                                            EMPEROR_RADIAL_SWORDS_SPEED);
                    }
                }
                // Final Burst Phase (Below 50% HP, Above 10%)
                else if (healthPercentage <= 0.5f && healthPercentage > 0.1f) {
                    if (!enemyAI.burstCooldownActive) {
                        enemyAI.radialAttackTimer += deltaTime;
                        if (enemyAI.radialAttackTimer >= 0.2f) { 
                            enemyAI.radialAttackTimer = 0.f;
                            std::cout << "[DEBUG] Emperor Super Attack: QUADRUPLE Radial Swords (Final Phase)\n";
                            m_spawner->spawnEmperorSwordsRadial(enemy, 
                                                                EMPEROR_RADIAL_SWORDS_COUNT, 
                                                                EMPEROR_RADIAL_SWORDS_RADIUS, 
                                                                EMPEROR_RADIAL_SWORDS_SPEED);

                            enemyAI.burstCount++;
                            if (enemyAI.burstCount >= 20) {
                                enemyAI.burstCooldownActive = true;
                                enemyAI.burstCooldownTimer = 0.f;
                                enemyAI.burstCount = 0;
                                auto enemySwords = m_entityManager.getEntities("EmperorSword");
                                for (auto& sword : enemySwords) {
                                    sword->destroy();
                                }
                                std::cout << "[DEBUG] Emperor Super Attack: COOLDOWN STARTED (5 sec)\n";
                            }
                        }
                    }
                    else {
                        enemyAI.burstCooldownTimer += deltaTime;
                        if (enemyAI.burstCooldownTimer >= 5.f) {
                            enemyAI.burstCooldownActive = false;
                            std::cout << "[DEBUG] Emperor Super Attack: COOLDOWN ENDED\n";
                        }
                    }
                }

                // Close-range attack (distance < 100) 
                if (distance < 100.f && !enemyAI.swordSpawned) {
                    std::cout << "[DEBUG] Emperor spawns static swords (Close-range attack)\n";
                    for (int i = 0; i < 3; ++i) {
                        m_spawner->spawnEmperorSwordOffset(enemy);
                    }
                    enemyAI.swordSpawned = true;
                }
            }
        }

        // ----------------------------------------------------
        // 3) GESTIONE KNOCKBACK
        // ----------------------------------------------------
        if (enemyAI.enemyState == EnemyState::Knockback) {
            std::cout << "[DEBUG] Knockback: vel.x=" << enemyTrans.velocity.x
                      << " pos.x=" << enemyTrans.pos.x
                      << " timer=" << enemyAI.knockbackTimer << "\n";

            // Distruggi subito le spade nemiche associate all'enemy
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
                enemyAI.enemyState = EnemyState::Follow; // oppure "Idle"
            }
        }

        // ----------------------------------------------------
        // 4) GRAVITÀ / CONTROLLO A TERRA
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
            enemyTrans.velocity.y = std::clamp(enemyTrans.velocity.y, 
                                               -MAX_FALL_SPEED, MAX_FALL_SPEED);
        } else {
            enemyTrans.velocity.y = 0.f;
        }

        // ----------------------------------------------------
        // 5) DISTANZA DAL GIOCATORE E VISTA
        // ----------------------------------------------------
        float dx = playerTrans.pos.x - enemyTrans.pos.x;
        float dy = playerTrans.pos.y - enemyTrans.pos.y;
        float distance = std::sqrt(dx * dx + dy * dy);

        // Se il giocatore è praticamente sulla stessa X ma più in alto, Idle
        if (std::fabs(dx) < 1.0f && playerTrans.pos.y < enemyTrans.pos.y) {
            enemyAI.enemyState = EnemyState::Idle;
            enemyTrans.velocity.x = 0.f;
            continue;
        }

        bool canSeePlayer  = checkLineOfSight(enemyTrans.pos, playerTrans.pos, m_entityManager);
        bool playerVisible = (distance < PLAYER_VISIBLE_DISTANCE) || canSeePlayer;
        bool playerVisible_elite = (distance < PLAYER_VISIBLE_DISTANCE * 3) || canSeePlayer;

        bool shouldFollow = false;
        switch (enemyAI.enemyBehavior) {
            case EnemyBehavior::FollowOne:
                if (playerVisible) shouldFollow = true;
                break;
            case EnemyBehavior::FollowTwo:
                if (playerVisible_elite) shouldFollow = true;
                break;
        }

        // Decremento attacco se > 0
        if (enemyAI.attackCooldown > 0.f) {
            enemyAI.attackCooldown -= deltaTime;
            if (enemyAI.attackCooldown < 0.f) {
                enemyAI.attackCooldown = 0.f;
            }
        }

        // ----------------------------------------------------
        // 6) STATO FOLLOW
        // ----------------------------------------------------
        if (enemyAI.enemyState == EnemyState::Follow) {
            const float xThreshold = 5.0f; // Tolerance to avoid rapid flipping

            bool tileDetected = false;
            sf::FloatRect enemyRect = enemy->get<CBoundingBox>().getRect(enemyTrans.pos);

            for (auto& tile : m_entityManager.getEntities("tile")) {
                if (!tile->has<CTransform>() || !tile->has<CBoundingBox>()) continue;
                auto& tileTrans = tile->get<CTransform>();
                auto& tileBB    = tile->get<CBoundingBox>();
                sf::FloatRect tileRect = tileBB.getRect(tileTrans.pos);

                bool horizontallyAligned =
                    (tileRect.top < enemyRect.top + enemyRect.height) &&
                    (tileRect.top + tileRect.height > enemyRect.top);

                if (horizontallyAligned &&
                   ((enemyAI.facingDirection > 0.f && tileRect.left <= enemyRect.left + enemyRect.width &&
                     tileRect.left > enemyRect.left) ||
                    (enemyAI.facingDirection < 0.f && tileRect.left + tileRect.width >= enemyRect.left &&
                     tileRect.left < enemyRect.left)))
                {
                    tileDetected = true;
                    std::cout << "[DEBUG] tileinfront true!!!\n";
                    break;
                }
            }

            // Normal movement logic
            if (std::abs(dx) > xThreshold) {
                int follow_speed =  (m_game.worldType == "Future") 
                                      ? static_cast<int>(FOLLOW_MOVE_SPEED * 0.7f) 
                                      : FOLLOW_MOVE_SPEED;
                enemyTrans.velocity.x = (dx > 0.f) ? follow_speed : -follow_speed;
                enemyAI.facingDirection = (dx > 0.f) ? 1.f : -1.f;
            } else {
                enemyTrans.velocity.x = 0.f;
            }
        }

        // Imposta l'animazione di corsa (Run)
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
            anim.animation = runAnim;
            anim.animation.reset();
        }

        // ----------------------------------------------------
        // 7) FUTURE WORLD: ATTACCO A DISTANZA (BULLET)
        // ----------------------------------------------------
        if (m_game.worldType == "Future") {
            enemyAI.shootTimer     += deltaTime;
            enemyAI.superMoveTimer += deltaTime;  // Track time for super move
        
            // 1) Check if it's time for a super move
            if (enemyAI.superMoveTimer >= enemyAI.superMoveCooldown) {
                enemyAI.superMoveTimer = 0.f; 
                enemyAI.superMoveReady = true;
            }
        
            // 2) Se non stiamo già sparando un burst
            if (!enemyAI.inBurst) {
                bool canShoot = checkLineOfSight(enemyTrans.pos, playerTrans.pos, m_entityManager)
                                && (distance >= enemyAI.minShootDistance);

                // Se è pronto un super move
                if (enemyAI.superMoveReady && canShoot) {
                    enemyAI.superMoveReady = false;
                    std::cout << "[DEBUG] Enemy " << enemy->id() << " uses SUPER MOVE!\n";

                    int superBullets = 8; 
                    float angleRange  = 30.f; // Spread angle
                    for (int i = 0; i < superBullets; ++i) {
                        auto bullet = m_spawner->spawnEnemyBullet(enemy);
                        if (bullet->has<CTransform>()) {
                            auto& bulletTrans = bullet->get<CTransform>();
                            // Spread da -15° a +15°
                            float step  = angleRange / (superBullets - 1);
                            float angle = -angleRange * 0.5f + (step * i);
                            bulletTrans.rotate(angle);
                        }
                    }
                }
                // Altrimenti, spara in burst "normale"
                else if (canShoot && enemyAI.shootTimer >= enemyAI.shootCooldown) {
                    enemyAI.shootTimer  = 0.f;
                    enemyAI.inBurst     = true;
                    enemyAI.bulletsShot = 0;
                    enemyAI.burstTimer  = 0.f;

                    std::cout << "[DEBUG] Enemy " << enemy->id()
                              << " starts a bullet burst (one-at-a-time)!\n";
                }
            }
            else {
                // Siamo in un burst: spariamo un proiettile ogni tot secondi
                enemyAI.burstTimer += deltaTime;
                if (enemyAI.burstTimer >= enemyAI.burstInterval) {
                    enemyAI.burstTimer = 0.f;
                    enemyAI.bulletsShot++;

                    auto bullet = m_spawner->spawnEnemyBullet(enemy);
                    std::cout << "[DEBUG] Enemy " << enemy->id()
                              << " fires bullet #" << enemyAI.bulletsShot << " of burst.\n";

                    // Leggero spread
                    if (bullet->has<CTransform>()) {
                        auto& bulletTrans = bullet->get<CTransform>();
                        float angleOffset = -3.f + static_cast<float>(rand() % 6); 
                        bulletTrans.rotate(angleOffset);
                    }

                    if (enemyAI.bulletsShot >= enemyAI.bulletCount) {
                        enemyAI.inBurst = false;
                        std::cout << "[DEBUG] Burst finished for enemy " << enemy->id() << "\n";
                    }
                }
            }
        }

        // ----------------------------------------------------
        // 8) ATTACCO MELEE (NON-FUTURE)
        // ----------------------------------------------------
        float currentAttackRange = (enemyAI.enemyType == EnemyType::Emperor)
                                   ? EMPEROR_ATTACK_RANGE
                                   : ATTACK_RANGE;

        bool shouldAttack = (shouldFollow && distance < currentAttackRange) ||
                            (enemyAI.enemyType == EnemyType::Super &&
                             enemyAI.enemyState == EnemyState::BlockedByTile);

         // Se skipAttack è true, saltiamo la logica di attacco
         if (!skipAttack) 
         {
             if (shouldAttack && enemyAI.attackCooldown <= 0.f 
                 && enemyAI.enemyState != EnemyState::Attack)
             {
                 // (A) Incrementa il conteggio di attacchi
                 enemyAI.consecutiveAttacks++;
                 std::cout << "[DEBUG] Enemy " << enemy->id() 
                           << " consecutiveAttacks = "
                           << enemyAI.consecutiveAttacks << "\n";
 
                 // (B) Se raggiunge il limite, parte il forced cooldown
                 if (enemyAI.consecutiveAttacks >= enemyAI.maxAttacksBeforeCooldown) {
                     enemyAI.isInForcedCooldown  = true;
                     enemyAI.forcedCooldownTimer = enemyAI.forcedCooldownDuration;
                     std::cout << "[DEBUG] Enemy " << enemy->id() 
                               << " forced cooldown started after "
                               << enemyAI.consecutiveAttacks << " attacks.\n";
                     // Non facciamo "continue;", così può muoversi
                 }
                 else {
                     // (C) Altrimenti, esegui l'attacco
                     if (m_game.worldType == "Future") {
                         // Se il mondo è Future, saltiamo la spada
                     } else {
                         // Normale sword attack
                         enemyAI.enemyState   = EnemyState::Attack;
                         enemyAI.attackTimer  = ATTACK_TIMER_DEFAULT;
                         enemyAI.swordSpawned = false;
 
                         std::cout << "[DEBUG] Enemy " << enemy->id() 
                                   << " entering Attack state. (cooldown="
                                   << enemyAI.attackCooldown << ")\n";
                     }
                 }
             }
             else if (shouldFollow && enemyAI.enemyState != EnemyState::Attack) {
                 enemyAI.enemyState = EnemyState::Follow;
             }
             else {
                 // Se il player non è visibile, passiamo a Recognition/Idle
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
         } 
         else {
             // Se skipAttack è true, il nemico non può attaccare,
             // ma continua a fare follow / idle / etc.
             if (shouldFollow && enemyAI.enemyState != EnemyState::Attack) {
                 enemyAI.enemyState = EnemyState::Follow;
             }
         }
 

        // ----------------------------------------------------
        // 9) GESTIONE STATO ATTACK (MELEE)
        // ----------------------------------------------------
        if (enemyAI.enemyState == EnemyState::Attack) {
            enemyTrans.velocity.x = 0.f;
            enemyAI.attackTimer  -= deltaTime;

            std::string baseAnimName;
            switch (enemyAI.enemyType) {
                case EnemyType::Normal:  baseAnimName = "EnemyNormal";  break;
                case EnemyType::Fast:    baseAnimName = "EnemyFast";    break;
                case EnemyType::Strong:  baseAnimName = "EnemyStrong";  break;
                case EnemyType::Elite:   baseAnimName = "EnemyElite";   break;
                case EnemyType::Super:   baseAnimName = "EnemySuper";   break;
                case EnemyType::Emperor: baseAnimName = "Emperor";      break;
            }

            // Animazione di attacco
            std::string attackAnimName = m_game.worldType + "Hit" + baseAnimName;
            if (anim.animation.getName() != attackAnimName) {
                std::cout << "[DEBUG] Enemy " << enemy->id() 
                          << " entering Attack animation: " << attackAnimName << "\n";
                anim.animation = m_game.assets().getAnimation(attackAnimName);
                anim.animation.reset();
                anim.repeat = false; 
            }

            // Spawn sword al momento giusto
            if (!enemyAI.swordSpawned && enemyAI.attackTimer <= SWORD_SPAWN_THRESHOLD) {
                if (enemyAI.enemyType == EnemyType::Emperor) {
                    float dx = playerTrans.pos.x - enemyTrans.pos.x;
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
                    m_spawner->spawnEnemySword(enemy);
                }
                enemyAI.swordSpawned = true;
                std::cout << "[DEBUG] Enemy " << enemy->id() 
                          << " spawning sword at AttackTimer: " 
                          << enemyAI.attackTimer << "\n";
            }

            // Fine attacco
            if (enemyAI.attackTimer <= 0.f) {
                enemyAI.attackCooldown = ATTACK_COOLDOWN;
                enemyAI.enemyState     = EnemyState::Follow;
                enemyAI.attackTimer    = 0.f;
                std::cout << "[DEBUG] Enemy " << enemy->id() 
                          << " Attack finished. Setting cooldown=" 
                          << enemyAI.attackCooldown << "\n";
            }
        }

        // ----------------------------------------------------
        // 10) STATO IDLE
        // ----------------------------------------------------
        if (enemyAI.enemyState == EnemyState::Idle) {
            enemyTrans.velocity.x = 0.f;
            anim.animation.reset();
        }

        // ----------------------------------------------------
        // 11) AGGIORNAMENTO POSIZIONE E FLIP
        // ----------------------------------------------------
        enemyTrans.pos.x += enemyTrans.velocity.x * deltaTime;
        enemyTrans.pos.y += enemyTrans.velocity.y * deltaTime;

        if (enemyAI.facingDirection < 0.f) {
            flipSpriteLeft(anim.animation.getMutableSprite());
        } else {
            flipSpriteRight(anim.animation.getMutableSprite());
        }
    }
}
