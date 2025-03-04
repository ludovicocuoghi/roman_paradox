#include "Scene_Play.h"
#include "Scene_Menu.h"
#include "Scene_GameOver.h"
#include "Physics.hpp"
#include "Entity.hpp"
#include "Components.hpp"
#include "Animation.hpp"
#include <iostream>
#include <fstream>
#include <random>
#include <algorithm>
#include "systems/PlayRenderer.h"
#include "systems/AnimationSystem.h"
#include <cmath>
#include "systems/CollisionSystem.h"
#include "systems/SpriteUtils.h"
#include "systems/Spawner.h"

void Scene_Play::initializeCamera()
{
    // 1) Start with the window's default view
    m_cameraView = m_game.window().getDefaultView();
    // 3) Get the background texture size (assuming m_backgroundTexture is loaded)
    sf::Vector2u bgSize = m_backgroundTexture.getSize();
    if (bgSize.x == 0 || bgSize.y == 0) {
        // Safety check: if the texture didn't load properly, just return
        return;
    }

    // 4) Center the camera on the middle of the background
    float centerX = static_cast<float>(bgSize.x) / 2.f;
    float centerY = static_cast<float>(bgSize.y) / 2.f - CAMERA_Y_OFFSET;
    m_cameraView.setCenter(centerX, centerY);

    m_cameraView.zoom(1.4f);

    // 6) Apply the view to the window
    m_game.window().setView(m_cameraView);
}
// Then call it from the constructor:
Scene_Play::Scene_Play(GameEngine& game, const std::string& levelPath)
    : Scene(game),
      m_levelPath(levelPath),
      m_entityManager(),
      m_lastDirection(1.f),
      m_animationSystem(game, m_entityManager, m_lastDirection),
      m_playRenderer(game, m_entityManager, m_backgroundSprite, m_backgroundTexture, m_cameraView, m_score),
      m_backgroundTexture(),
      m_backgroundSprite(),
      m_game(game),
      m_gameOver(false),
      m_levelLoader(game),
      m_showBoundingBoxes(false),
      m_showGrid(false),
      m_backgroundPath(""),
      m_timeofday(""),
      m_cameraView(game.window().getDefaultView()),
      m_score(0),
      m_movementSystem(game, m_entityManager, m_cameraView, m_lastDirection),
      m_spawner(game, m_entityManager),
      m_enemyAISystem(m_entityManager, m_spawner, m_game)
{
    std::cout << "[DEBUG] Scene_Play constructor: levelPath = " << levelPath << std::endl;

    if (m_levelPath.empty()) {
        std::cerr << "[ERROR] Scene_Play received an empty level path!" << std::endl;
        return;
    }

    selectBackgroundFromLevel(m_levelPath);
    std::cout << "[DEBUG] Selected background: " << m_backgroundPath << std::endl;

    if (!m_backgroundTexture.loadFromFile(m_backgroundPath)) {
        std::cerr << "[ERROR] Could not load background image: " << m_backgroundPath << std::endl;
    } else {
        m_backgroundTexture.setRepeated(true);
        m_backgroundSprite.setTexture(m_backgroundTexture);
    }

    std::cout << "[DEBUG] Initializing Camera...\n";
    initializeCamera();

    std::cout << "[DEBUG] Calling init()...\n";
    init();

    std::cout << "[DEBUG] Scene_Play initialized successfully!\n";
}

void Scene_Play::selectBackgroundFromLevel(const std::string& levelPath) {
    // Extract level name from path
    std::string levelName = levelPath.substr(levelPath.find_last_of("/\\") + 1);
    
    // Mapping of levels to backgrounds
    if (levelName == "alien_rome_level_1.txt") {
        m_backgroundPath = "src/images/Background/alien_rome/alien_rome_phase_1.png";
        m_timeofday = "ALIEN EMPIRE";
    } else if (levelName == "alien_rome_level_2.txt") {
        m_backgroundPath = "src/images/Background/alien_rome/alien_rome_phase_2.png";
        m_timeofday = "ALIEN EMPIRE";
    } else if (levelName == "ancient_rome_level_1_day.txt") {
        m_backgroundPath = "src/images/Background/ancient_rome/ancient_rome_level_1_day.png";
        m_timeofday = "ANCIENT ROME (DAY)";
    } else if (levelName == "ancient_rome_level_2_sunset.txt") {
        m_backgroundPath = "src/images/Background/ancient_rome/ancient_rome_level_2_sunset.png";
        m_timeofday = "ANCIENT ROME (SUNSET)";
    } else if (levelName == "ancient_rome_level_3_night.txt") {
        m_backgroundPath = "src/images/Background/ancient_rome/ancient_rome_level_3_night.png";
        m_timeofday = "ANCIENT ROME (NIGHT)";
    } else if (levelName == "ancient_rome_level_4_emperor_room.txt") {
        m_backgroundPath = "src/images/Background/ancient_rome/ancient_rome_level_4_emperor_room.png";
        m_timeofday = "EMPEROR ROOM";
    } else if (levelName == "future_rome_level_1.txt") {
        m_backgroundPath = "src/images/Background/future_rome/rome2.png";
        m_timeofday = "FUTURE ROME (DAY)";
    } else if (levelName == "future_rome_level_2.txt") {
        m_backgroundPath = "src/images/Background/future_rome/rome3.png";
        m_timeofday = "FUTURE ROME (NIGHT)";
    } else if (levelName == "future_rome_level_emperor_room.txt") {
        m_backgroundPath = "src/images/Background/future_rome/emperor_room.png";
        m_timeofday = "FUTURE EMPEROR ROOM";
    }  
    else {
        // Default background if level is not mapped
        m_backgroundPath = "src/images/Background/default.png";
        m_timeofday = "Unknown";
    }
}

void Scene_Play::init()
{
    std::cout << "[DEBUG] Scene_Play::init() - Start\n";

    registerCommonActions();
    registerAction(sf::Keyboard::T, "TOGGLE_TEXTURE");
    registerAction(sf::Keyboard::A, "MOVE_LEFT");
    registerAction(sf::Keyboard::D, "MOVE_RIGHT");
    registerAction(sf::Keyboard::W, "JUMP");
    registerAction(sf::Keyboard::Escape, "QUIT");
    registerAction(sf::Keyboard::Space, "ATTACK");
    registerAction(sf::Keyboard::M, "DEFENSE");
    registerAction(sf::Keyboard::G, "TOGGLE_GRID");
    registerAction(sf::Keyboard::B, "TOGGLE_BB");
    registerAction(sf::Keyboard::Enter, "SUPERMOVE");

    std::cout << "[DEBUG] Scene_Play::init() - Loading level: " << m_levelPath << std::endl;

    if (m_levelPath.empty()) {
        std::cerr << "[ERROR] Cannot load level: Level path is empty!\n";
        return;
    }

    if (!std::filesystem::exists(m_levelPath)) {
        std::cerr << "[ERROR] Level file does not exist: " << m_levelPath << std::endl;
        return;
    }

    // ✅ Ensure entity manager is clean before loading
    m_entityManager = EntityManager();

    std::cout << "[DEBUG] Scene_Play::init() - Calling m_levelLoader.load()\n";
    m_levelLoader.load(m_levelPath, m_entityManager);
    std::cout << "[DEBUG] Scene_Play::init() - Level loaded successfully!\n";
}
//
// Main Update Function
//
void Scene_Play::update(float deltaTime)
{
    if (!m_gameOver)
    {
        // 1) Update your entity manager, states, collisions, etc.
        m_entityManager.update();

        for (auto& entity : m_entityManager.getEntities()) {
            if (entity->has<CHealth>())
                entity->get<CHealth>().update(deltaTime);
            if (entity->has<CState>())
                entity->get<CState>().update(deltaTime);
        }

        sMovement(deltaTime);
        sEnemyAI(deltaTime);
        sCollision();
        sAnimation(deltaTime);
        UpdateFragments(deltaTime);
        m_spawner.updateGraves(deltaTime);
        sUpdateSword();
        sAmmoSystem(deltaTime);
        updateBurstFire(deltaTime);

        // Then do ephemeral checks, life checks, etc.
        lifeCheckEnemyDeath();
        lifeCheckPlayerDeath();
    }
    else
    {
        m_game.window().setView(m_game.window().getDefaultView());
        m_game.changeScene("GAMEOVER", std::make_shared<Scene_GameOver>(m_game));
    }

    // 3) Finally, render
    sRender();
}
// Rendering
//

void Scene_Play::sRender() {
    // Aggiorna eventuali impostazioni di rendering se necessario:
    m_playRenderer.setShowGrid(m_showGrid);
    m_playRenderer.setShowBoundingBoxes(m_showBoundingBoxes);
    m_playRenderer.setScore(m_score);
    m_playRenderer.setTimeOfDay(m_timeofday);

    // Esegui il rendering tramite PlayRenderer
    m_playRenderer.render();
}

//
// Movement and Camera Update
//
void Scene_Play::sMovement(float deltaTime) {
    m_movementSystem.update(deltaTime);
}
//
// Animation Updates
//
void Scene_Play::sAnimation(float deltaTime)
{
    m_animationSystem.update(deltaTime);
}

//
//Collision Handliong
//
void Scene_Play::sCollision() {
    CollisionSystem collisionSystem(m_entityManager, m_game, &m_spawner, m_score);
    collisionSystem.updateCollisions(); // ✅ pass dt
}
// Action Processing (Input Handling)                        

void Scene_Play::sDoAction(const Action& action)
{
    auto playerEntities = m_entityManager.getEntities("player");
    if (playerEntities.empty()) return;

    auto player = playerEntities[0];
    auto& PTrans   = player->get<CTransform>();
    auto& vel      = PTrans.velocity;  
    auto& state    = player->get<CState>();

    bool inDefense = (state.state == "defense");
    bool hasFutureArmor = player->has<CPlayerEquipment>() && 
                          player->get<CPlayerEquipment>().hasFutureArmor;

    static bool isMovingLeft  = false;
    static bool isMovingRight = false;
    

    if (action.type() == "START")
    {
        if (!inDefense)
        {
            if (action.name() == "MOVE_LEFT") {
                isMovingLeft    = true;
                isMovingRight   = false;
                PTrans.facingDirection = -1.f;
                vel.x = -xSpeed;
                if (state.state != "air") {
                    state.state = "run";
                }
            }
            else if (action.name() == "MOVE_RIGHT") {
                isMovingRight   = true;
                isMovingLeft    = false;
                PTrans.facingDirection = 1.f;
                vel.x = xSpeed;
                if (state.state != "air") {
                    state.state = "run";
                }
            }
            else if (action.name() == "JUMP") {
                if (state.onGround && 
                   (state.state == "idle" || 
                    state.state == "run" || 
                    state.state == "attack"))
                {
                    state.isJumping = true;
                    state.jumpTime  = 0.0f;
                    vel.y           = -ySpeed;
                    state.state     = "air";
                }
            }
        }

        if (action.name() == "TOGGLE_GRID") {
            m_showGrid = !m_showGrid;
        }
        else if (action.name() == "TOGGLE_BB") {
            m_showBoundingBoxes = !m_showBoundingBoxes;
        }
        else if (action.name() == "ATTACK") {
            // Check cooldowns first
            if (state.bulletCooldown > 0.f) {
                std::cout << "[DEBUG] Attack on cooldown! " << state.bulletCooldown << "s left.\n";
                return;
            }
            
            if (state.attackCooldown <= 0.f) {
                state.state = "attack";
                state.attackTime = 0.5f;
                state.attackCooldown = 0.5f;
                
                if (hasFutureArmor) {
                    // Check for ammo if the ammo system is enabled
                    if (player->has<CAmmo>()) {
                        auto& ammo = player->get<CAmmo>();
                        if (ammo.currentBullets <= 0) {
                            std::cout << "[DEBUG] Out of ammo! Cannot attack.\n";
                            return;
                        }
                        // Consume a bullet
                        ammo.currentBullets--;
                        std::cout << "[DEBUG] Bullet fired. Ammo left: " << ammo.currentBullets << "\n";
                    }
                    
                    // Fire a bullet immediately for feedback
                    m_spawner.spawnPlayerBullet(player);
                    state.bulletCooldown = 0.2f;
                    
                    // Enable burst mode so we can keep firing
                    state.inBurst        = true;
                    state.burstTimer     = 0.f;
                    state.burstFireTimer = 0.f;
                    state.bulletsShot    = 1;
                    std::cout << "[DEBUG] Burst started.\n";
                }
                else {
                    // First, destroy any existing sword
                    if (m_activeSword) {
                        m_activeSword->destroy();
                        m_activeSword = nullptr;
                    }
                    
                    // Then spawn a new sword and store the reference
                    m_activeSword = m_spawner.spawnSword(player);
                    state.bulletCooldown = 0.5f;
                    std::cout << "[DEBUG] Sword attack. Cooldown: 0.5s\n";
                }
            }
        }
        else if (action.name() == "SUPERMOVE") {
            // Super move logic
            if (state.bulletCooldown > 0.f) {
                std::cout << "[DEBUG] Super Move on cooldown! " << state.bulletCooldown << "s left.\n";
                return;
            }
            
            // Add check for super move readiness
            if (state.superBulletTimer <= 0.f && state.attackCooldown <= 0.f) {
                if (hasFutureArmor) {
                    state.state = "attack";
                    state.attackTime = 0.5f;
                    state.attackCooldown = 0.5f;
        
                    // Fire multiple bullets in a spread pattern
                    int bulletCount = state.superBulletCount;
                    float angleRange = 40.0f;
                    for (int i = 0; i < bulletCount; i++) {
                        auto bullet = m_spawner.spawnPlayerBullet(player);
                        if (bullet && bullet->has<CTransform>()) {
                            auto& bulletTrans = bullet->get<CTransform>();
                            float step = angleRange / (bulletCount - 1);
                            float angle = -angleRange * 0.5f + step * i;
                            bulletTrans.rotate(angle);
                        }
                    }
                    
                    // Set both cooldowns
                    state.bulletCooldown = 1.0f; // Normal cooldown for basic shots
                    
                    // THIS IS THE CRUCIAL PART: Reset the super move timer
                    state.superBulletTimer = state.superBulletCooldown; // Reset super move cooldown
                    state.superMoveReady = false; // No longer ready
                    
                    std::cout << "[DEBUG] Super Move! Fired " << bulletCount << " bullets. Super cooldown reset to " 
                              << state.superBulletCooldown << "s\n";
                } 
                else {
                    std::cout << "[DEBUG] No future armor, can't perform Super Move.\n";
                }
            } else if (state.superBulletTimer > 0.f) {
                // Provide feedback that super move isn't ready yet
                std::cout << "[DEBUG] Super Move not ready yet. Cooldown remaining: " 
                          << state.superBulletTimer << "s\n";
            }
        }
        else if (action.name() == "DEFENSE") {
            // Activate defense only if there's stamina left
            if (state.shieldStamina > 0.f && state.state != "defense") {
                std::cout << "[DEBUG] Defense activated.\n";
                state.state = "defense";
            }
        }
    }
    else if (action.type() == "END")
    {
        if (!inDefense) {
            if (action.name() == "MOVE_LEFT") {
                isMovingLeft = false;
                if (!isMovingRight) {
                    vel.x = 0.f;
                    if (state.state != "air") {
                        state.state = "idle";
                    }
                }
            }
            else if (action.name() == "MOVE_RIGHT") {
                isMovingRight = false;
                if (!isMovingLeft) {
                    vel.x = 0.f;
                    if (state.state != "air") {
                        state.state = "idle";
                    }
                }
            }
            else if (action.name() == "JUMP") {
                state.isJumping = false;
            }
            else if (action.name() == "ATTACK") 
            {
                // If user releases ATTACK during a burst, cancel it
                if (state.inBurst) {
                    state.inBurst        = false;
                    state.burstTimer     = 0.f;
                    state.burstFireTimer = 0.f;
                    state.bulletsShot    = 0;
                    std::cout << "[DEBUG] Burst ended by releasing ATTACK.\n";
                }
            }
        }
        else if (action.name() == "DEFENSE") {
            // End defense when defense key is released
            if (state.state == "defense") {
                state.state = "idle";
            }
        }
    }
}

void Scene_Play::sLifespan(float deltaTime)
{
    // Loop through all entities with a CLifeSpan component
    for (auto e : m_entityManager.getEntities())
    {
        if (e->has<CLifeSpan>())
        {
            // Decrease the remaining lifespan by the elapsed time
            auto& lifespan = e->get<CLifeSpan>();
            lifespan.remainingTime -= deltaTime;
            
            // If lifespan is over, destroy the entity
            if (lifespan.remainingTime <= 0)
            {
                e->destroy();
                
                // If this is a sword, you might want to log it
                if (e->tag() == "sword")
                {
                    std::cout << "[DEBUG] Sword despawned due to lifespan end.\n";
                }
            }
        }
    }
}

void Scene_Play::sUpdateSword()
{
    auto playerEntities = m_entityManager.getEntities("player");
    if (playerEntities.empty() || !m_activeSword) return;

    auto player = playerEntities[0];
    auto& state = player->get<CState>();
    
    // If the attack animation is done or player state changed, destroy the sword
    if (m_activeSword && (state.attackTime <= 0.f || state.state != "attack")) {
        m_activeSword->destroy();
        m_activeSword = nullptr;
        std::cout << "[DEBUG] Sword removed after attack finished.\n";
    }
}

void Scene_Play::sAmmoSystem(float dt)
{
    auto playerEntities = m_entityManager.getEntities("player");
    if (playerEntities.empty()) return;
    
    auto player = playerEntities[0];
    if (!player->has<CAmmo>()) return;
    
    // Only process if player has future armor
    if (!player->has<CPlayerEquipment>() || !player->get<CPlayerEquipment>().hasFutureArmor) {
        return;
    }
    
    auto& ammo = player->get<CAmmo>();
    
    // Check if we need to start reloading
    if (ammo.currentBullets <= 0 && !ammo.isReloading) {
        ammo.isReloading = true;
        ammo.currentReloadTime = 0.f;
        ammo.reloadTime = 5.0f; // 5 seconds for reload
        std::cout << "[DEBUG] Auto-reload started.\n";
    }
    
    // Handle reloading process
    if (ammo.isReloading) {
        ammo.currentReloadTime += dt;
        
        // Reload complete
        if (ammo.currentReloadTime >= ammo.reloadTime) {
            ammo.currentBullets = ammo.maxBullets;
            ammo.isReloading = false;
            std::cout << "[DEBUG] Reload complete. Bullets: " << ammo.currentBullets << "\n";
        }
    }
}

void Scene_Play::updateBurstFire(float deltaTime)
{
    // Get the player
    auto playerEntities = m_entityManager.getEntities("player");
    if (playerEntities.empty()) return;
    auto player = playerEntities[0];

    auto& state = player->get<CState>();

    // If not in a burst, do nothing
    if (!state.inBurst) return;

    // 1) Update timers
    state.burstTimer     += deltaTime;
    state.burstFireTimer += deltaTime;

    // 2) If the total burst time exceeded
    if (state.burstTimer >= state.burstDuration) {
        // End the burst
        state.inBurst        = false;
        state.burstTimer     = 0.f;
        state.burstFireTimer = 0.f;
        state.bulletsShot    = 0;
        std::cout << "[DEBUG] Burst ended (time limit reached).\n";
        return;
    }

    // 3) Check if enough time has passed to fire again
    if (state.burstFireTimer >= state.burstInterval) 
    {
        // Check if we have ammo, if ammo system is enabled
        if (player->has<CAmmo>()) {
            auto& ammo = player->get<CAmmo>();
            if (ammo.currentBullets <= 0) {
                // End the burst if out of ammo
                state.inBurst = false;
                std::cout << "[DEBUG] Burst ended - out of ammo.\n";
                return;
            }
            
            // Consume a bullet
            ammo.currentBullets--;
        }
        
        // Reset the interval timer
        state.burstFireTimer = 0.f;
        state.bulletsShot++;

        // Spawn a bullet
        m_spawner.spawnPlayerBullet(player);
        
        if (player->has<CAmmo>()) {
            auto& ammo = player->get<CAmmo>();
            std::cout << "[DEBUG] Burst bullet #" << state.bulletsShot << ", Ammo left: " << ammo.currentBullets << "\n";
        } else {
            std::cout << "[DEBUG] Burst bullet #" << state.bulletsShot << "\n";
        }
    }
}

// Wrapper: chiama il metodo updateFragments del Spawner
void Scene_Play::UpdateFragments(float deltaTime) {
    m_spawner.updateFragments(deltaTime);
}

//
// Check for Player Death
//
void Scene_Play::lifeCheckPlayerDeath() {
    auto players = m_entityManager.getEntities("player");
    if (players.empty()) {
        return;
    }

    auto& player = players.front();
    const auto& transform = player->get<CTransform>();
    const auto& health = player->get<CHealth>();

    bool isOutOfBounds = transform.pos.y > 1800;
    bool isDead = (health.currentHealth <= 0);

    if (isOutOfBounds || isDead) {
        m_gameOver = true;
        m_game.changeScene("GAMEOVER", std::make_shared<Scene_GameOver>(m_game));
    }
}

std::string Scene_Play::extractLevelName(const std::string& path) {
    auto pos = path.find_last_of("/\\");
    if (pos == std::string::npos) return path; // nessuna slash
    return path.substr(pos + 1);
}

void Scene_Play::removeTileByID(const std::string& tileID) {
    auto tiles = m_entityManager.getEntities("tile");
    for (auto& t : tiles) {
        if (t->has<CUniqueID>()) {
            auto& uid = t->get<CUniqueID>();
            if (uid.id == tileID) {
                t->destroy();
                std::cout << "[DEBUG] Tile " << tileID << " distrutta!\n";
                break; // esci dopo averla trovata
            }
        }
    }
}

void Scene_Play::lifeCheckEnemyDeath() {
    auto enemies = m_entityManager.getEntities("enemy");
    for (auto& enemy : enemies) {
        if (!enemy->has<CHealth>()) continue;

        const auto& transform = enemy->get<CTransform>();
        const auto& health    = enemy->get<CHealth>();

        bool isOutOfBounds = (transform.pos.y > 1800);
        bool isDead        = (health.currentHealth <= 0);

        if (isOutOfBounds || isDead) {
            bool isEmperor = (enemy->has<CEnemyAI>() && enemy->get<CEnemyAI>().enemyType == EnemyType::Emperor);

            std::cout << "[DEBUG] Enemy ID " << enemy->id() << " is dead! Spawning " 
                      << (isEmperor ? "Emperor" : "normal") << " grave...\n";
            

            // ✅ Spawn the correct grave type
            m_spawner.spawnEnemyGrave(transform.pos, isEmperor);

            // Handle unique enemy ID logic (e.g., tile removal)
            if (enemy->has<CUniqueID>()) {
                auto& uniqueID = enemy->get<CUniqueID>();
                std::string levelName = extractLevelName(m_levelPath);

                std::cout << "[DEBUG] Checking tile removal for enemy ID: " << uniqueID.id << "\n";
                
                if (levelName == "ancient_rome_level_1_day.txt") {
                    if (uniqueID.id == "EnemyFast_4") {
                        std::cout << "[DEBUG] Removing tile PipeTall_275\n";
                        removeTileByID("PipeTall_275");
                    }
                }
                else if (levelName == "ancient_rome_level_2_sunset.txt") {
                    if (uniqueID.id == "EnemyStrong_12") {
                        std::cout << "[DEBUG] Removing tile PipeTall_900\n";
                        removeTileByID("PipeTall_900");
                    }
                }
                else if (levelName == "ancient_rome_level_4_emperor_room.txt") {
                    if (uniqueID.id == "Emperor_1") {
                        std::cout << "[DEBUG] (Level2) Emperor defeated...\n";
                        removeTileByID("PipeTall_209");
                    }
                }
            }

            // Destroy the enemy
            enemy->destroy();
            std::cout << "[DEBUG] Enemy destroyed: ID = " << enemy->id() << "\n";
        }
    }
}

std::shared_ptr<Entity> Scene_Play::spawnSword(std::shared_ptr<Entity> player) {
    return m_spawner.spawnSword(player);
}

std::shared_ptr<Entity> Scene_Play::spawnItem(Vec2<float> position, const std::string& tileType) {
    return m_spawner.spawnItem(position, tileType);
}

std::shared_ptr<Entity> Scene_Play::spawnEnemySword(std::shared_ptr<Entity> enemy) {
    return m_spawner.spawnEnemySword(enemy);
}

void Scene_Play::sEnemyAI(float deltaTime) {
    m_enemyAISystem.update(deltaTime);
}

bool Scene_Play::isObstacleInFront(Vec2<float> enemyPos, float direction)
{
    Vec2<float> checkPos = enemyPos + Vec2<float>(direction * 50.f, 0.f);
    for (auto& tile : m_entityManager.getEntities("tile")) {
        auto& tileTrans = tile->get<CTransform>();
        sf::FloatRect tileBB = tile->get<CBoundingBox>().getRect(tileTrans.pos);

        if (tileBB.contains(checkPos.x, checkPos.y)) {
            return true; // Obstacle detected
        }
    }
    return false;
}
