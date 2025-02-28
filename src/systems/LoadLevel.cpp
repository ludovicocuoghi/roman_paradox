#include "LoadLevel.h"
#include <fstream>
#include <iostream>
#include <string>

LoadLevel::LoadLevel(GameEngine& game)
    : m_game(game)
{
}

void LoadLevel::load(const std::string& levelPath, EntityManager& entityManager)
{
    entityManager = EntityManager();
    std::ifstream file(levelPath);
    if (!file.is_open())
    {
        std::cerr << "[ERROR] Failed to open level file: " << levelPath << std::endl;
        return;
    }
    const int windowHeight = m_game.window().getSize().y;
    bool playerSpawned = false;
    int tileIndex = 0;
    int decIndex  = 0;
    int enemyIndex = 0;
    std::string type, assetType;
    int x, y;
    std::cout << "[DEBUG] Loading level from: " << levelPath << std::endl;

    if (levelPath.find("ancient") != std::string::npos)
        m_game.worldType = "Ancient";
    else if (levelPath.find("alien") != std::string::npos)
        m_game.worldType = "Alien";
    else if (levelPath.find("future") != std::string::npos)
        m_game.worldType = "Future";

    while (file >> type)
    {
        if (type == "Tile")
        {
            if (!(file >> assetType >> x >> y))
            {
                std::cerr << "[WARNING] Incomplete Tile entry. Skipping.\n";
                continue;
            }
            tileIndex++;
            std::string tileID = assetType + "_" + std::to_string(tileIndex);
            float realX = x * LoadLevel::GRID_SIZE + LoadLevel::HALF_GRID;
            float realY = windowHeight - (y * LoadLevel::GRID_SIZE) - LoadLevel::HALF_GRID;
            if (assetType == "PipeTall")
                realY += LoadLevel::GRID_SIZE * LoadLevel::PIPETALL_REALY_OFFSET_MULTIPLIER;
            else if (assetType == "PipeBroken")
                realY += LoadLevel::GRID_SIZE * LoadLevel::PIPEBROKEN_REALY_OFFSET_MULTIPLIER;
            else if (assetType == "Pipe")
                realY += LoadLevel::GRID_SIZE * LoadLevel::PIPE_REALY_OFFSET_MULTIPLIER;
            else if (assetType == "BlackHoleRedBig")
            {
                realX += LoadLevel::GRID_SIZE * LoadLevel::BLACKHOLE_OFFSET_MULTIPLIER;
                realY += LoadLevel::GRID_SIZE * LoadLevel::BLACKHOLE_OFFSET_MULTIPLIER;
            }
            auto tile = entityManager.addEntity("tile");
            tile->add<CUniqueID>(tileID);
            std::string fullAssetName =  m_game.worldType + assetType;
            std::cout << "[DEBUG] Loaded Tile: " << assetType
                      << " at (" << x << ", " << y << ")"
                      << " with ID: " << tileID << std::endl;
            if (m_game.assets().hasAnimation(fullAssetName))
            {
                const Animation& anim = m_game.assets().getAnimation(fullAssetName);
                tile->add<CAnimation>(anim, true);
                if (assetType == "LevelDoor" || assetType == "LevelDoorGold")
                {
                    realY += LoadLevel::GRID_SIZE * LoadLevel::LEVELDOOR_REALY_OFFSET_MULTIPLIER;
                    Vec2<float> bboxSize(96.f, 192.f);
                    Vec2<float> bboxOffset = bboxSize * 0.5f;
                    bboxOffset.y -= 96.f;
                    tile->add<CBoundingBox>(bboxSize, bboxOffset);
                }
                else if (assetType == "BlackHoleRedBig")
                {
                    realY += LoadLevel::GRID_SIZE * LoadLevel::BLACKHOLE_OFFSET_MULTIPLIER;
                    Vec2<float> bboxSize(150.f, 150.f);
                    Vec2<float> bboxOffset = bboxSize * 0.5f;
                    tile->add<CBoundingBox>(bboxSize, bboxOffset);
                }
                else
                {
                    Vec2<float> bboxSize(static_cast<float>(anim.getSize().x), static_cast<float>(anim.getSize().y));
                    tile->add<CBoundingBox>(bboxSize, bboxSize * 0.5f);
                    if (assetType == "Treasure")
                        tile->add<CState>("inactive");
                }
            }
            else
            {
                std::cerr << "[WARNING] Missing animation for tile: " << assetType << std::endl;
            }
            tile->add<CTransform>(Vec2<float>(realX, realY));
        }
        else if (type == "Dec")
        {
            if (!(file >> assetType >> x >> y))
            {
                std::cerr << "[WARNING] Incomplete Decoration entry. Skipping.\n";
                continue;
            }
            decIndex++;
            std::string decID = assetType + "_" + std::to_string(decIndex);
            float realX = x * LoadLevel::GRID_SIZE + LoadLevel::HALF_GRID;
            float realY = windowHeight - (y * LoadLevel::GRID_SIZE) - LoadLevel::HALF_GRID;
            auto decor = entityManager.addEntity("decoration");
            decor->add<CUniqueID>(decID);
            if (assetType == "GoldPipeTall")
                realY += LoadLevel::GRID_SIZE * LoadLevel::PIPETALL_REALY_OFFSET_MULTIPLIER;
            if (assetType == "EnemyGrave" || assetType == "EmperorGrave")
                realY += +10;
            std::string fullAssetName =  m_game.worldType + assetType;
            if (m_game.assets().hasAnimation(fullAssetName))
            {
                const Animation& anim = m_game.assets().getAnimation(fullAssetName);
                if (assetType.find("BushTall") != std::string::npos)
                    realY += LoadLevel::GRID_SIZE * 1.5f;
                decor->add<CAnimation>(anim, true);
                decor->add<CTransform>(Vec2<float>(realX, realY));
                std::cout << "[DEBUG] Loaded Decoration: " << assetType
                          << " at (" << x << ", " << y << ")"
                          << " with ID: " << decID << std::endl;
            }
            else
            {
                std::cerr << "[WARNING] Missing animation for decoration: " << assetType << std::endl;
            }
        }
        else if (type == "Player")
        {
            if (!(file >> x >> y))
            {
                std::cerr << "[WARNING] Incomplete Player entry. Skipping.\n";
                continue;
            }
            float realX = x * LoadLevel::GRID_SIZE + LoadLevel::HALF_GRID;
            float realY = windowHeight - (y * LoadLevel::GRID_SIZE) - LoadLevel::HALF_GRID;
            auto player = entityManager.addEntity("player");
            if (m_game.assets().hasAnimation("PlayerStand"))
            {
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
            }
            else
            {
                std::cerr << "[ERROR] Missing PlayerStand animation!" << std::endl;
            }
        }
        else if (type == "Enemy")
        {
            std::string enemyTypeStr;
            int enemyX, enemyY, px1, py1, px2, py2;
            if (!(file >> enemyTypeStr >> enemyX >> enemyY >> px1 >> py1 >> px2 >> py2))
            {
                std::cerr << "[WARNING] Incomplete Enemy entry. Skipping.\n";
                continue;
            }
            enemyIndex++;
            std::string enemyID = enemyTypeStr + "_" + std::to_string(enemyIndex);
            auto enemy = entityManager.addEntity("enemy");
            enemy->add<CUniqueID>(enemyID);
            EnemyType enemyType;
            float speedMultiplier = 1.0f;
            int enemyHealth = 10;
            int enemyDamage = 3;
            if (enemyTypeStr == "EnemyFast")
            {
                enemyType = EnemyType::Fast;
                speedMultiplier = LoadLevel::ENEMY_FAST_SPEED_MULTIPLIER;
                enemyHealth = LoadLevel::ENEMY_FAST_HEALTH;
                enemyDamage = LoadLevel::ENEMY_FAST_DAMAGE;
            }
            else if (enemyTypeStr == "EnemyStrong")
            {
                enemyType = EnemyType::Strong;
                speedMultiplier = LoadLevel::ENEMY_STRONG_SPEED_MULTIPLIER;
                enemyHealth = LoadLevel::ENEMY_STRONG_HEALTH;
                enemyDamage = LoadLevel::ENEMY_STRONG_DAMAGE;
            }
            else if (enemyTypeStr == "EnemyElite")
            {
                enemyType = EnemyType::Elite;
                speedMultiplier = LoadLevel::ENEMY_ELITE_SPEED_MULTIPLIER;
                enemyHealth = LoadLevel::ENEMY_ELITE_HEALTH;
                enemyDamage = LoadLevel::ENEMY_ELITE_DAMAGE;
            }
            else if (enemyTypeStr == "Emperor")
            {
                enemyType = EnemyType::Emperor;
                speedMultiplier = LoadLevel::ENEMY_EMPEROR_SPEED_MULTIPLIER;
                enemyHealth = LoadLevel::ENEMY_EMPEROR_HEALTH;
                enemyDamage = LoadLevel::ENEMY_EMPEROR_DAMAGE;
            }
            else if (enemyTypeStr == "EnemyNormal")
            {
                enemyType = EnemyType::Normal;
                speedMultiplier = LoadLevel::ENEMY_NORMAL_SPEED_MULTIPLIER;
                enemyHealth = LoadLevel::ENEMY_NORMAL_HEALTH;
                enemyDamage = LoadLevel::ENEMY_NORMAL_DAMAGE;
            }
            else if (enemyTypeStr == "EnemySuper")
            {
                enemyType = EnemyType::Super;
                speedMultiplier = LoadLevel::ENEMY_NORMAL_SPEED_MULTIPLIER;
                enemyHealth = LoadLevel::ENEMY_NORMAL_HEALTH;
                enemyDamage = LoadLevel::ENEMY_NORMAL_DAMAGE;
            }
            else
            {
                std::cerr << "[WARNING] Unknown enemy type: " << enemyTypeStr << std::endl;
                continue;
            }
            std::string runAnimName, standAnimName;
            if (enemyTypeStr == "Emperor")
            {
                standAnimName =  m_game.worldType + "StandOldRomeEmperor";
                runAnimName =  m_game.worldType + "RunOldRomeEmperor";
            }
            else
            {
                standAnimName =  m_game.worldType + "StandAnim" + enemyTypeStr;
                runAnimName =  m_game.worldType + "Run" + enemyTypeStr;
            }
            std::cout << "[DEBUG] Enemy Type: " << enemyTypeStr
                      << " | Using Animations: " << standAnimName << " / " << runAnimName
                      << " Health: " << enemyHealth
                      << " Damage: " << enemyDamage
                      << " ID: " << enemyID << std::endl;
            if (m_game.assets().hasAnimation(runAnimName))
            {
                const Animation& anim = m_game.assets().getAnimation(runAnimName);
                enemy->add<CAnimation>(anim, true);
            }
            else if (m_game.assets().hasAnimation(standAnimName))
            {
                std::cerr << "[WARNING] Missing " << runAnimName << " animation, falling back to " << standAnimName << std::endl;
                const Animation& anim = m_game.assets().getAnimation(standAnimName);
                enemy->add<CAnimation>(anim, true);
            }
            else
            {
                std::cerr << "[ERROR] Missing animations for " << enemyTypeStr << " enemy!" << std::endl;
            }
            float realX = enemyX * LoadLevel::GRID_SIZE + LoadLevel::HALF_GRID;
            float realY = windowHeight - (enemyY * LoadLevel::GRID_SIZE) - LoadLevel::HALF_GRID;
            if (enemyType == EnemyType::Emperor)
                realY -= LoadLevel::GRID_SIZE * LoadLevel::EMPEROR_REALY_OFFSET_MULTIPLIER;
            enemy->add<CTransform>(Vec2<float>(realX, realY));
            if (enemyType == EnemyType::Emperor)
            {
                Vec2<float> emperorBBSize(LoadLevel::EMPEROR_BB_WIDTH, LoadLevel::EMPEROR_BB_HEIGHT);
                enemy->add<CBoundingBox>(emperorBBSize, emperorBBSize * 0.5f);
            }
            else
            {
                Vec2<float> enemyBBSize(LoadLevel::PLAYER_BB_SIZE, LoadLevel::PLAYER_BB_SIZE);
                enemy->add<CBoundingBox>(enemyBBSize, enemyBBSize * 0.5f);
            }
            enemy->add<CGravity>(LoadLevel::GRAVITY_VAL);
            enemy->add<CHealth>(enemyHealth);
            EnemyBehavior behavior = (enemyType == EnemyType::Elite || enemyType == EnemyType::Super ) ? EnemyBehavior::FollowTwo : EnemyBehavior::FollowOne;
            enemy->add<CEnemyAI>(enemyType, behavior);
            enemy->get<CEnemyAI>().damage = enemyDamage;
            enemy->get<CEnemyAI>().speedMultiplier = speedMultiplier;
            std::cout << "[DEBUG] Enemy Damage: " << enemyDamage << std::endl;
            std::vector<Vec2<float>> patrolPoints;
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
                      << px1 << "," << py1 << ") <-> (" << px2 << "," << py2 << ")"
                      << " ID: " << enemyID << std::endl;
        }
        else
        {
            std::cerr << "[WARNING] Unknown entity type: " << type << std::endl;
        }
    }
    if (!playerSpawned)
        std::cerr << "[ERROR] No player entity found in level file!" << std::endl;
    file.close();
}
