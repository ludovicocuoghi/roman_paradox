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
                player->add<CState>("idle", false, 0.0f, false, 0.0f, 0.0f, 0.0f);
                player->add<CHealth>(10);
                playerSpawned = true;
                std::cout << "[DEBUG] Player Spawned at (" << x << ", " << y << ")" << std::endl;
            } else {
                std::cerr << "[ERROR] Missing PlayerStand animation!" << std::endl;
            }
        }
        else if (type == "Enemy") {
            std::string enemyTypeStr;
            int enemyX, enemyY;
            std::vector<Vec2<float>> patrolPoints;

            if (!(file >> enemyTypeStr >> enemyX >> enemyY)) {
                std::cerr << "[WARNING] Incomplete Enemy entry. Skipping.\n";
                continue;
            }

            float realX = enemyX * GRID_SIZE + (GRID_SIZE * 0.5f);
            float realY = windowHeight - (enemyY * GRID_SIZE) - (GRID_SIZE * 0.5f);

            auto enemy = entityManager.addEntity("enemy");

            // Extract actual enemy type from string (removing "Enemy" prefix)
            std::string animationPrefix;
            if (enemyTypeStr == "EnemyFast") {
                animationPrefix = "EnemyFast";
            } else if (enemyTypeStr == "EnemyStrong") {  
                animationPrefix = "EnemyStrong";
            } else if (enemyTypeStr == "EnemyElite") {
                animationPrefix = "EnemyElite";
            } else if (enemyTypeStr == "EnemyNormal") {
                animationPrefix = "EnemyNormal";
            } else {
                std::cerr << "[WARNING] Unknown enemy type: " << enemyTypeStr << " defaulting to Normal." << std::endl;
                animationPrefix = "EnemyNormal";
            }

            // Assign animations
            std::string runAnimName = animationPrefix + "_Run";
            std::string standAnimName = animationPrefix + "_Stand";

            std::cout << "[DEBUG] Enemy Type: " << enemyTypeStr << " | Using Animations: " << standAnimName << " / " << runAnimName << std::endl;

            if (m_game.assets().hasAnimation(runAnimName)) {
                const Animation& anim = m_game.assets().getAnimation(runAnimName);
                enemy->add<CAnimation>(anim, true);
            } else if (m_game.assets().hasAnimation(standAnimName)) {
                std::cerr << "[WARNING] Missing " << runAnimName << " animation, falling back to " << standAnimName << std::endl;
                const Animation& anim = m_game.assets().getAnimation(standAnimName);
                enemy->add<CAnimation>(anim, true);
            } else {
                std::cerr << "[ERROR] Missing animations for " << enemyTypeStr << " enemy!" << std::endl;
            }

            // Assign state (Separate "if" conditions for easy future modifications)
            if (enemyTypeStr == "EnemyFast") {
                enemy->add<CState>("patrol", false, 0.0f, false, 0.0f, 0.0f, 0.0f);
            }

            if (enemyTypeStr == "EnemyStrong") {
                enemy->add<CState>("idle", false, 0.0f, false, 0.0f, 0.0f, 0.0f);
            }

            if (enemyTypeStr == "EnemyElite") {
                enemy->add<CState>("idle", false, 0.0f, false, 0.0f, 0.0f, 0.0f);
            }

            if (enemyTypeStr == "EnemyNormal") {
                enemy->add<CState>("idle", false, 0.0f, false, 0.0f, 0.0f, 0.0f);
            }

            enemy->add<CTransform>(Vec2<float>(realX, realY));

            // Bounding box size (same for all enemies)
            Vec2<float> enemyBBSize(PLAYER_BB_SIZE, PLAYER_BB_SIZE);
            enemy->add<CBoundingBox>(enemyBBSize, enemyBBSize * 0.5f);

            enemy->add<CGravity>(GRAVITY_VAL);
            enemy->add<CHealth>(10);

            // Read patrol points
            int px, py;
            while (file >> px >> py) {
                float patrolX = px * GRID_SIZE + (GRID_SIZE * 0.5f);
                float patrolY = windowHeight - (py * GRID_SIZE) - (GRID_SIZE * 0.5f);
                patrolPoints.push_back(Vec2<float>(patrolX, patrolY));
            }
            if (patrolPoints.empty()) {
                patrolPoints.push_back(Vec2<float>(realX, realY));
            }

            // Determine enemy type enum
            EnemyType enemyType;
            if (enemyTypeStr == "EnemyFast")
                enemyType = EnemyType::Fast;
            else if (enemyTypeStr == "EnemyStrong")
                enemyType = EnemyType::Strong;
            else if (enemyTypeStr == "EnemyElite")
                enemyType = EnemyType::Elite;
            else
                enemyType = EnemyType::Normal;

            enemy->add<CEnemyAI>(enemyType);
            enemy->get<CEnemyAI>().patrolPoints = patrolPoints;
            enemy->get<CEnemyAI>().currentPatrolIndex = 0;

            std::cout << "[DEBUG] Spawned " << enemyTypeStr << " Enemy at (" << enemyX << ", " << enemyY << ")" << std::endl;
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
