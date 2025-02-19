#include "LoadLevel.h"
#include <fstream>
#include <iostream>

LoadLevel::LoadLevel(GameEngine& game)
    : m_game(game)
{
}

void LoadLevel::load(const std::string& levelPath, EntityManager& entityManager)
{
    // Reinitialize the EntityManager
    entityManager = EntityManager();

    std::ifstream file(levelPath);
    if (!file.is_open()) {
        std::cerr << "[ERROR] Failed to open level file: " << levelPath << std::endl;
        return;
    }

    // Retrieve the window height from the game engine
    const int windowHeight = m_game.window().getSize().y;
    bool playerSpawned = false;

    std::string type, assetType;
    int x, y;

    std::cout << "[DEBUG] Loading level from: " << levelPath << std::endl;

    while (file >> type)
    {
        // ------------------------------------------------------
        // TILE
        // ------------------------------------------------------
        if (type == "Tile") {
            if (!(file >> assetType >> x >> y)) {
                std::cerr << "[WARNING] Incomplete Tile entry. Skipping.\n";
                continue;
            }
        
            float realX = x * LoadLevel::GRID_SIZE + LoadLevel::HALF_GRID;
            float realY = windowHeight - (y * LoadLevel::GRID_SIZE) - LoadLevel::HALF_GRID;
        
            // Pipe offsets (unchanged)
            if (assetType == "PipeTall") {
                realY += LoadLevel::GRID_SIZE * LoadLevel::PIPETALL_REALY_OFFSET_MULTIPLIER;
            }
            else if (assetType == "PipeBroken") {
                realY += LoadLevel::GRID_SIZE * LoadLevel::PIPEBROKEN_REALY_OFFSET_MULTIPLIER;
            }
            else if (assetType == "Pipe") {
                realY += LoadLevel::GRID_SIZE * LoadLevel::PIPE_REALY_OFFSET_MULTIPLIER;
            }
        
            auto tile = entityManager.addEntity("tile");
        
            // Directly check if we have an animation named assetType
            if (m_game.assets().hasAnimation(assetType)) {
                const Animation& anim = m_game.assets().getAnimation(assetType);
                tile->add<CAnimation>(anim, true);
        
                std::cout << "[DEBUG] Loaded Tile: " << assetType
                          << " at (" << x << ", " << y << ")" << std::endl;
        
                // ------------------------------
                // Special case for "LevelDoor"
                // ------------------------------
                if (assetType == "LevelDoor") {
                    //Specify y offset for tile location

                    realY += LoadLevel::GRID_SIZE * LoadLevel::LEVELDOOR_REALY_OFFSET_MULTIPLIER;
                    // Force a custom bounding box (96×192), centered with y offset
                    Vec2<float> bboxSize(96.f, 192.f);
                    Vec2<float> bboxOffset = bboxSize * 0.5f;
                    if (assetType == "LevelDoor") {
                        bboxOffset.y -= 96.f; // Move bounding box down by 96 pixels
                    }
                    tile->add<CBoundingBox>(bboxSize, bboxOffset);
                } else {
                    // Default bounding box = animation size
                    Vec2<float> bboxSize(
                        static_cast<float>(anim.getSize().x),
                        static_cast<float>(anim.getSize().y)
                    );
                    tile->add<CBoundingBox>(bboxSize, bboxSize * 0.5f);
        
                    // If it's a TreasureBox, add "inactive" state
                    if (assetType == "TreasureBoxAnim") {
                        tile->add<CState>("inactive");
                    }
                    // else if (assetType == "Box1" || assetType == "Box2") { ... }
                }
            }
            else {
                std::cerr << "[WARNING] Missing animation for tile: " << assetType << std::endl;
            }
        
            tile->add<CTransform>(Vec2<float>(realX, realY));
        }
        // ------------------------------------------------------
        // DECORATION
        // ------------------------------------------------------
        else if (type == "Dec") {
            if (!(file >> assetType >> x >> y)) {
                std::cerr << "[WARNING] Incomplete Decoration entry. Skipping.\n";
                continue;
            }

            float realX = x * LoadLevel::GRID_SIZE + LoadLevel::HALF_GRID;
            float realY = windowHeight - (y * LoadLevel::GRID_SIZE) - LoadLevel::HALF_GRID;
            auto decor = entityManager.addEntity("decoration");

            // Pipe offset if it's "GoldPipeTall", etc.
            if (assetType == "GoldPipeTall") {
                realY += LoadLevel::GRID_SIZE * LoadLevel::PIPETALL_REALY_OFFSET_MULTIPLIER;
            }

            if (m_game.assets().hasAnimation(assetType)) {
                const Animation& anim = m_game.assets().getAnimation(assetType);
                // Example offset for "BushTall"
                if (assetType == "BushTall") {
                    realY += LoadLevel::GRID_SIZE * 1.5f;
                }

                decor->add<CAnimation>(anim, true);
                decor->add<CTransform>(Vec2<float>(realX, realY));

                std::cout << "[DEBUG] Loaded Decoration: " << assetType
                          << " at (" << x << ", " << y << ")" << std::endl;
            } else {
                std::cerr << "[WARNING] Missing animation for decoration: " << assetType << std::endl;
            }
        }
        // ------------------------------------------------------
        // PLAYER
        // ------------------------------------------------------
        else if (type == "Player") {
            if (!(file >> x >> y)) {
                std::cerr << "[WARNING] Incomplete Player entry. Skipping.\n";
                continue;
            }

            float realX = x * LoadLevel::GRID_SIZE + LoadLevel::HALF_GRID;
            float realY = windowHeight - (y * LoadLevel::GRID_SIZE) - LoadLevel::HALF_GRID;
            auto player = entityManager.addEntity("player");

            if (m_game.assets().hasAnimation("PlayerStand")) {
                const Animation& anim = m_game.assets().getAnimation("PlayerStand");
                player->add<CAnimation>(anim, true);
                player->add<CTransform>(Vec2<float>(realX, realY));
                Vec2<float> bboxSize(LoadLevel::PLAYER_BB_SIZE, LoadLevel::PLAYER_BB_SIZE);
                player->add<CBoundingBox>(bboxSize, bboxSize * 0.5f);
                player->add<CGravity>(LoadLevel::GRAVITY_VAL);
                player->add<CState>("idle");

                auto& state = player->get<CState>();
                state.isInvincible = false;
                state.invincibilityTimer = 0.0f;
                state.isJumping = false;
                state.jumpTime = 0.0f;
                state.knockbackTimer = 0.0f;
                state.onGround = false;
                state.attackTime = 0.0f;
                state.attackCooldown = 0.0f;
                
                player->add<CHealth>(PLAYER_HEALTH);
                playerSpawned = true;
                std::cout << "[DEBUG] Player Spawned at (" << x << ", " << y << ")" << std::endl;
            } else {
                std::cerr << "[ERROR] Missing PlayerStand animation!" << std::endl;
            }
        }
        // ------------------------------------------------------
        // ENEMY
        // ------------------------------------------------------
        else if (type == "Enemy") {
            std::string enemyTypeStr;
            int enemyX, enemyY, px1, py1, px2, py2;
            std::vector<Vec2<float>> patrolPoints;

            if (!(file >> enemyTypeStr >> enemyX >> enemyY >> px1 >> py1 >> px2 >> py2)) {
                std::cerr << "[WARNING] Incomplete Enemy entry. Skipping.\n";
                continue;
            }
            auto enemy = entityManager.addEntity("enemy");

            EnemyType enemyType;
            float speedMultiplier = 1.0f;
            int enemyHealth = 10;
            int enemyDamage = 3; // default

            // Decide type
            if (enemyTypeStr == "EnemyFast") {
                enemyType = EnemyType::Fast;
                speedMultiplier = LoadLevel::ENEMY_FAST_SPEED_MULTIPLIER;
                enemyHealth = LoadLevel::ENEMY_FAST_HEALTH;
                enemyDamage = LoadLevel::ENEMY_FAST_DAMAGE;
            } else if (enemyTypeStr == "EnemyStrong") {
                enemyType = EnemyType::Strong;
                speedMultiplier = LoadLevel::ENEMY_STRONG_SPEED_MULTIPLIER;
                enemyHealth = LoadLevel::ENEMY_STRONG_HEALTH;
                enemyDamage = LoadLevel::ENEMY_STRONG_DAMAGE;
            } else if (enemyTypeStr == "EnemyElite") {
                enemyType = EnemyType::Elite;
                speedMultiplier = LoadLevel::ENEMY_ELITE_SPEED_MULTIPLIER;
                enemyHealth = LoadLevel::ENEMY_ELITE_HEALTH;
                enemyDamage = LoadLevel::ENEMY_ELITE_DAMAGE;
            } else if  (enemyTypeStr == "Emperor") {
                enemyType = EnemyType::Emperor;
                speedMultiplier = LoadLevel::ENEMY_EMPEROR_SPEED_MULTIPLIER;
                enemyHealth = LoadLevel::ENEMY_EMPEROR_HEALTH;
                enemyDamage = LoadLevel::ENEMY_EMPEROR_DAMAGE;
            } else {
                std::cerr << "[WARNING] Unknown enemy type: " << enemyTypeStr << std::endl;
                continue;
            }

            std::string runAnimName   = enemyTypeStr + "_Run";
            std::string standAnimName = enemyTypeStr + "_Stand";

            std::cout << "[DEBUG] Enemy Type: " << enemyTypeStr
                      << " | Using Animations: " << standAnimName << " / " << runAnimName
                      << " Health: " << enemyHealth
                      << " Damage: " << enemyDamage << std::endl;

            // Try run anim, fallback to stand anim
            if (m_game.assets().hasAnimation(runAnimName)) {
                const Animation& anim = m_game.assets().getAnimation(runAnimName);
                enemy->add<CAnimation>(anim, true);
            } else if (m_game.assets().hasAnimation(standAnimName)) {
                std::cerr << "[WARNING] Missing " << runAnimName << " animation, falling back to "
                          << standAnimName << std::endl;
                const Animation& anim = m_game.assets().getAnimation(standAnimName);
                enemy->add<CAnimation>(anim, true);
            } else {
                std::cerr << "[ERROR] Missing animations for " << enemyTypeStr << " enemy!" << std::endl;
            }

            
            float realX = enemyX * LoadLevel::GRID_SIZE + LoadLevel::HALF_GRID;
            float realY = windowHeight - (enemyY * LoadLevel::GRID_SIZE) - LoadLevel::HALF_GRID;


            if (enemyType == EnemyType::Emperor) {
                realY -= LoadLevel::GRID_SIZE * LoadLevel::EMPEROR_REALY_OFFSET_MULTIPLIER;
            }
            
            // Aggiungi il componente Transform
            enemy->add<CTransform>(Vec2<float>(realX, realY));
            
            // Se Emperor, usa un bounding box più grande
            if (enemyType == EnemyType::Emperor) {
                Vec2<float> emperorBBSize(LoadLevel::EMPEROR_BB_WIDTH, LoadLevel::EMPEROR_BB_HEIGHT);
                enemy->add<CBoundingBox>(emperorBBSize, emperorBBSize * 0.5f);
            } else {
                // bounding box di default per gli altri
                Vec2<float> enemyBBSize(LoadLevel::PLAYER_BB_SIZE, LoadLevel::PLAYER_BB_SIZE);
                enemy->add<CBoundingBox>(enemyBBSize, enemyBBSize * 0.5f);
            }
            
            enemy->add<CGravity>(LoadLevel::GRAVITY_VAL);
            enemy->add<CHealth>(enemyHealth);

            enemy->add<CEnemyAI>(enemyType, (enemyType == EnemyType::Elite) ? EnemyBehavior::FollowTwo : EnemyBehavior::FollowOne);
            enemy->get<CEnemyAI>().damage = enemyDamage;
            enemy->get<CEnemyAI>().speedMultiplier = speedMultiplier;
            std::cout << "[DEBUG] Enemy Damage: " << enemyDamage << std::endl;

            // Patrol points
            patrolPoints.push_back(Vec2<float>(
                px1 * LoadLevel::GRID_SIZE + LoadLevel::HALF_GRID,
                windowHeight - (py1 * LoadLevel::GRID_SIZE) - LoadLevel::HALF_GRID
            ));
            patrolPoints.push_back(Vec2<float>(
                px2 * LoadLevel::GRID_SIZE + LoadLevel::HALF_GRID,
                windowHeight - (py2 * LoadLevel::GRID_SIZE) - LoadLevel::HALF_GRID
            ));
            enemy->get<CEnemyAI>().patrolPoints = patrolPoints;
            enemy->get<CEnemyAI>().currentPatrolIndex = 0;
            enemy->get<CEnemyAI>().enemyState = EnemyState::Idle;

            std::cout << "[DEBUG] Spawned " << enemyTypeStr << " Enemy at ("
                      << enemyX << ", " << enemyY << ") with patrol ("
                      << px1 << "," << py1 << ") <-> (" << px2 << "," << py2 << ")" << std::endl;
        }
        else {
            std::cerr << "[WARNING] Unknown entity type: " << type << std::endl;
        }
    } // end while

    if (!playerSpawned) {
        std::cerr << "[ERROR] No player entity found in level file!" << std::endl;
    }

    file.close();
}
