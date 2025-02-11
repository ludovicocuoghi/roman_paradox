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

    // Mapping tile types to their corresponding animation names
    std::unordered_map<std::string, std::string> tileAnimations = {
        {"Ground", "Ground"},
        {"Brick", "Brick"},
        {"Box1", "Box1"},
        {"Box2", "Box2"},
        {"PipeTall", "PipeTall"},
        {"PipeShort", "PipeShort"},
        {"Pipe", "Pipe"},
        {"Pole", "Pole"},
        {"PoleTop", "PoleTop"},
        {"BushSmall", "BushSmall"},
        {"BushTall", "BushTall"},
        {"CloudSmall", "CloudSmall"},
        {"CloudBig", "CloudBig"},
        {"TreasureBoxAnim", "TreasureBoxAnim"},
        {"TreasureBoxHit", "TreasureBoxHit"},
        {"Grape", "Grape"},
        {"Coin", "Coin"}
    };

    while (file >> type)
    {
        if (type == "Tile") {
            if (!(file >> assetType >> x >> y)) {
                std::cerr << "[WARNING] Incomplete Tile entry. Skipping.\n";
                continue;
            }

            float realX = x * GRID_SIZE + (GRID_SIZE * 0.5f);
            float realY = windowHeight - (y * GRID_SIZE) - (GRID_SIZE * 0.5f);

            // Correct positioning for pipes
            if (assetType == "PipeTall") {
                realY += GRID_SIZE / 2 * 3;
                realX += GRID_SIZE / 2;
            }
            else if (assetType == "PipeShort") {
                realY += GRID_SIZE / 2;
                realX += GRID_SIZE / 2;
            }
            else if (assetType == "Pipe") {
                realY += GRID_SIZE / 2 * 2;
                realX += GRID_SIZE / 2;
            }

            auto tile = entityManager.addEntity("tile");

            // Load the correct animation based on assetType
            if (tileAnimations.find(assetType) != tileAnimations.end()) {
                if (m_game.assets().hasAnimation(tileAnimations[assetType])) {
                    const Animation& anim = m_game.assets().getAnimation(tileAnimations[assetType]);
                    tile->add<CAnimation>(anim, true);
                    std::cout << "[DEBUG] Loaded Tile: " << assetType
                              << " at (" << x << ", " << y << ")" << std::endl;
                    Vec2<float> bboxSize(static_cast<float>(anim.getSize().x),
                                         static_cast<float>(anim.getSize().y));
                    tile->add<CBoundingBox>(bboxSize, bboxSize * 0.5f);
                    if (tileAnimations[assetType] == "TreasureBoxAnim") {
                        tile->add<CState>("inactive");
                    }
                } else {
                    std::cerr << "[WARNING] Missing animation for tile: " << assetType << std::endl;
                }
            } else {
                std::cerr << "[WARNING] Unknown tile type: " << assetType << std::endl;
            }

            tile->add<CTransform>(Vec2<float>(realX, realY));
        }
        else if (type == "Dec") {  // Decorations
            if (!(file >> assetType >> x >> y)) {
                std::cerr << "[WARNING] Incomplete Decoration entry. Skipping.\n";
                continue;
            }

            float realX = x * GRID_SIZE + (GRID_SIZE * 0.5f);
            float realY = windowHeight - (y * GRID_SIZE) - (GRID_SIZE * 0.5f);
            auto decor = entityManager.addEntity("decoration");

            if (m_game.assets().hasAnimation(assetType)) {
                const Animation& anim = m_game.assets().getAnimation(assetType);
                // Position correction if necessary
                if (assetType == "BushTall") {
                    realY += GRID_SIZE / 2 * 3;
                }

                decor->add<CAnimation>(anim, true);
                decor->add<CTransform>(Vec2<float>(realX, realY));

                std::cout << "[DEBUG] Loaded Decoration: " << assetType
                          << " at (" << x << ", " << y << ")" << std::endl;
            } else {
                std::cerr << "[WARNING] Missing animation for decoration: " << assetType << std::endl;
            }
        }
        else if (type == "Player") {
            if (!(file >> x >> y)) {
                std::cerr << "[WARNING] Incomplete Player entry. Skipping.\n";
                continue;
            }

            float realX = x * GRID_SIZE + (GRID_SIZE * 0.5f);
            float realY = windowHeight - (y * GRID_SIZE) - (GRID_SIZE * 0.5f);
            auto player = entityManager.addEntity("player");

            if (m_game.assets().hasAnimation("PlayerStand")) {
                const Animation& anim = m_game.assets().getAnimation("PlayerStand");
                player->add<CAnimation>(anim, true);
                player->add<CTransform>(Vec2<float>(realX, realY));
                Vec2<float> bboxSize(PLAYER_BB_SIZE, PLAYER_BB_SIZE);
                player->add<CBoundingBox>(bboxSize, bboxSize * 0.5f);
                player->add<CGravity>(GRAVITY_VAL);
                player->add<CState>("idle");  // Only one argument is needed

                player->add<CState>("idle");  // Just pass the string, nothing else
                auto& state = player->get<CState>();

                // Set additional properties manually
                state.isInvincible = false;
                state.invincibilityTimer = 0.0f;
                state.isJumping = false;
                state.jumpTime = 0.0f;
                state.knockbackTimer = 0.0f;
                state.onGround = false;
                state.attackTime = 0.0f;
                state.attackCooldown = 0.0f;
                
                player->add<CHealth>(10);
                playerSpawned = true;
                std::cout << "[DEBUG] Player Spawned at (" << x << ", " << y << ")" << std::endl;
            } else {
                std::cerr << "[ERROR] Missing PlayerStand animation!" << std::endl;
            }
        }
        else if (type == "Enemy") {
            std::string enemyTypeStr;
            int enemyX, enemyY, px1, py1, px2, py2;
            std::vector<Vec2<float>> patrolPoints;

            // Read enemy type, spawn position, and patrol points
            if (!(file >> enemyTypeStr >> enemyX >> enemyY >> px1 >> py1 >> px2 >> py2)) {
                std::cerr << "[WARNING] Incomplete Enemy entry. Skipping.\n";
                continue;
            }

            // Convert grid coordinates to world coordinates (center of the cell)
            float realX = enemyX * GRID_SIZE + (GRID_SIZE * 0.5f);
            float realY = windowHeight - (enemyY * GRID_SIZE) - (GRID_SIZE * 0.5f);

            auto enemy = entityManager.addEntity("enemy");

            // Convert enemyTypeStr to an EnemyType
            EnemyType enemyType;
            if (enemyTypeStr == "EnemyFast")
                enemyType = EnemyType::Fast;
            else if (enemyTypeStr == "EnemyStrong")
                enemyType = EnemyType::Strong;
            else if (enemyTypeStr == "EnemyElite")
                enemyType = EnemyType::Elite;
            else
                enemyType = EnemyType::Normal;

            // Set speed multiplier based on enemy type
            float speedMultiplier = 1.0f;
            if (enemyType == EnemyType::Fast)
                speedMultiplier = 1.5f;
            else if (enemyType == EnemyType::Strong)
                speedMultiplier = 0.7f;
            else if (enemyType == EnemyType::Elite)
                speedMultiplier = 1.5f;

            // Configure animations based on type
            std::string runAnimName = enemyTypeStr + "_Run";
            std::string standAnimName = enemyTypeStr + "_Stand";

            std::cout << "[DEBUG] Enemy Type: " << enemyTypeStr 
                    << " | Using Animations: " << standAnimName << " / " << runAnimName << std::endl;

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

            // Remove: enemy->add<CState>("patrol", false, 0.0f, false, 0.0f, 0.0f, 0.0f); ❌

            // Add core components
            enemy->add<CTransform>(Vec2<float>(realX, realY));

            Vec2<float> enemyBBSize(PLAYER_BB_SIZE, PLAYER_BB_SIZE);
            enemy->add<CBoundingBox>(enemyBBSize, enemyBBSize * 0.5f);
            enemy->add<CGravity>(GRAVITY_VAL);
            enemy->add<CHealth>(10);

            // Convert patrol points to world coordinates
            patrolPoints.push_back(Vec2<float>(px1 * GRID_SIZE + (GRID_SIZE * 0.5f),
                                            windowHeight - (py1 * GRID_SIZE) - (GRID_SIZE * 0.5f)));
            patrolPoints.push_back(Vec2<float>(px2 * GRID_SIZE + (GRID_SIZE * 0.5f),
                                            windowHeight - (py2 * GRID_SIZE) - (GRID_SIZE * 0.5f)));

            // Determine AI behavior: FollowOne (sees player) or FollowTwo (remembers player)
            EnemyBehavior behavior = (enemyType == EnemyType::Elite) ? EnemyBehavior::FollowTwo : EnemyBehavior::FollowOne;

            // Initialize enemy AI component with type & behavior
            enemy->add<CEnemyAI>(enemyType, behavior);

            // Set patrol points (if relevant)
            enemy->get<CEnemyAI>().patrolPoints = patrolPoints;
            enemy->get<CEnemyAI>().currentPatrolIndex = 0;
            enemy->get<CEnemyAI>().speedMultiplier = speedMultiplier;

            // Set enemy to Idle (default) or Follow if in range of the player
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
