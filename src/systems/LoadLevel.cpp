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

    // Contatori per assegnare ID univoci a tile, decorazioni e nemici
    int tileIndex = 0;
    int decIndex  = 0;
    int enemyIndex = 0;

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

            // Incrementa il contatore tile e crea un ID unico, ad es. "Ground_1", "Brick_2", ecc.
            tileIndex++;
            std::string tileID = assetType + "_" + std::to_string(tileIndex);

            float realX = x * LoadLevel::GRID_SIZE + LoadLevel::HALF_GRID;
            float realY = windowHeight - (y * LoadLevel::GRID_SIZE) - LoadLevel::HALF_GRID;

            // Eventuali offset speciali per le pipe
            if (assetType == "PipeTall") {
                realY += LoadLevel::GRID_SIZE * LoadLevel::PIPETALL_REALY_OFFSET_MULTIPLIER;
            } else if (assetType == "PipeBroken") {
                realY += LoadLevel::GRID_SIZE * LoadLevel::PIPEBROKEN_REALY_OFFSET_MULTIPLIER;
            } else if (assetType == "Pipe") {
                realY += LoadLevel::GRID_SIZE * LoadLevel::PIPE_REALY_OFFSET_MULTIPLIER;
            }

            auto tile = entityManager.addEntity("tile");

            // Assegna l'ID univoco
            tile->add<CUniqueID>(tileID);

            // Caricamento animazione
            if (m_game.assets().hasAnimation(assetType)) {
                const Animation& anim = m_game.assets().getAnimation(assetType);
                tile->add<CAnimation>(anim, true);

                std::cout << "[DEBUG] Loaded Tile: " << assetType
                          << " at (" << x << ", " << y << ")"
                          << " with ID: " << tileID << std::endl;

                // Caso speciale: LevelDoor
                if (assetType == "LevelDoor" || assetType == "LevelDoorGold") {
                    realY += LoadLevel::GRID_SIZE * LoadLevel::LEVELDOOR_REALY_OFFSET_MULTIPLIER;
                    Vec2<float> bboxSize(96.f, 192.f);
                    Vec2<float> bboxOffset = bboxSize * 0.5f;
                    bboxOffset.y -= 96.f; // sposta il BB verso il basso
                    tile->add<CBoundingBox>(bboxSize, bboxOffset);
                } else {
                    // Bounding box di default = dimensioni dell'animazione
                    Vec2<float> bboxSize(
                        static_cast<float>(anim.getSize().x),
                        static_cast<float>(anim.getSize().y)
                    );
                    tile->add<CBoundingBox>(bboxSize, bboxSize * 0.5f);

                    // Esempio: se è una TreasureBox
                    if (assetType == "TreasureBoxAnim") {
                        tile->add<CState>("inactive");
                    }
                }
            } else {
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

            decIndex++;
            std::string decID = assetType + "_" + std::to_string(decIndex);

            float realX = x * LoadLevel::GRID_SIZE + LoadLevel::HALF_GRID;
            float realY = windowHeight - (y * LoadLevel::GRID_SIZE) - LoadLevel::HALF_GRID;

            auto decor = entityManager.addEntity("decoration");
            decor->add<CUniqueID>(decID);

            // Offset speciale per "GoldPipeTall", ecc.
            if (assetType == "GoldPipeTall") {
                realY += LoadLevel::GRID_SIZE * LoadLevel::PIPETALL_REALY_OFFSET_MULTIPLIER;
            }

            if (m_game.assets().hasAnimation(assetType)) {
                const Animation& anim = m_game.assets().getAnimation(assetType);

                // Esempio offset per "BushTall"
                if (assetType == "BushTall") {
                    realY += LoadLevel::GRID_SIZE * 1.5f;
                }

                decor->add<CAnimation>(anim, true);
                decor->add<CTransform>(Vec2<float>(realX, realY));

                std::cout << "[DEBUG] Loaded Decoration: " << assetType
                          << " at (" << x << ", " << y << ")"
                          << " with ID: " << decID << std::endl;
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

            if (!(file >> enemyTypeStr >> enemyX >> enemyY >> px1 >> py1 >> px2 >> py2)) {
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
            int enemyDamage = 3; // default

            // Decide type (puoi sostituire con la tua getEnemyType se preferisci)
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
            } else if (enemyTypeStr == "Emperor") {
                enemyType = EnemyType::Emperor;
                speedMultiplier = LoadLevel::ENEMY_EMPEROR_SPEED_MULTIPLIER; // da definire
                enemyHealth = LoadLevel::ENEMY_EMPEROR_HEALTH;               // da definire
                enemyDamage = LoadLevel::ENEMY_EMPEROR_DAMAGE;               // da definire
            } else if (enemyTypeStr == "EnemyNormal") {
                enemyType = EnemyType::Normal;
                speedMultiplier = LoadLevel::ENEMY_NORMAL_SPEED_MULTIPLIER;
                enemyHealth = LoadLevel::ENEMY_NORMAL_HEALTH;
                enemyDamage = LoadLevel::ENEMY_NORMAL_DAMAGE;
            } else {
                std::cerr << "[WARNING] Unknown enemy type: " << enemyTypeStr << std::endl;
                continue;
            }

            std::string runAnimName   = enemyTypeStr + "_Run";
            std::string standAnimName = enemyTypeStr + "_Stand";

            std::cout << "[DEBUG] Enemy Type: " << enemyTypeStr
                      << " | Using Animations: " << standAnimName << " / " << runAnimName
                      << " Health: " << enemyHealth
                      << " Damage: " << enemyDamage
                      << " ID: " << enemyID
                      <<  "general ID : " << enemy->id()  << std::endl;
            // Caricamento animazioni
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

            // Se Emperor, offset addizionale
            if (enemyType == EnemyType::Emperor) {
                realY -= LoadLevel::GRID_SIZE * LoadLevel::EMPEROR_REALY_OFFSET_MULTIPLIER;
            }

            // Trasform
            enemy->add<CTransform>(Vec2<float>(realX, realY));

            // Bounding box personalizzato per Emperor
            if (enemyType == EnemyType::Emperor) {
                Vec2<float> emperorBBSize(LoadLevel::EMPEROR_BB_WIDTH, LoadLevel::EMPEROR_BB_HEIGHT);
                enemy->add<CBoundingBox>(emperorBBSize, emperorBBSize * 0.5f);
            } else {
                Vec2<float> enemyBBSize(LoadLevel::PLAYER_BB_SIZE, LoadLevel::PLAYER_BB_SIZE);
                enemy->add<CBoundingBox>(enemyBBSize, enemyBBSize * 0.5f);
            }

            enemy->add<CGravity>(LoadLevel::GRAVITY_VAL);
            enemy->add<CHealth>(enemyHealth);

            // Assegna l'AI
            // Se vuoi Emperor con FollowTwo, aggiungi "|| enemyType == EnemyType::Emperor"
            EnemyBehavior behavior = (enemyType == EnemyType::Elite)
                                     ? EnemyBehavior::FollowTwo
                                     : EnemyBehavior::FollowOne;
            enemy->add<CEnemyAI>(enemyType, behavior);

            enemy->get<CEnemyAI>().damage = enemyDamage;
            enemy->get<CEnemyAI>().speedMultiplier = speedMultiplier;
            std::cout << "[DEBUG] Enemy Damage: " << enemyDamage << std::endl;

            // Gestione dei punti di pattuglia
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
        else {
            std::cerr << "[WARNING] Unknown entity type: " << type << std::endl;
        }
    } // end while

    if (!playerSpawned) {
        std::cerr << "[ERROR] No player entity found in level file!" << std::endl;
    }

    file.close();
}
