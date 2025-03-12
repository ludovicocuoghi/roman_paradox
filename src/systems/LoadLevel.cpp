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
                    //realY += LoadLevel::GRID_SIZE * LoadLevel::BLACKHOLE_OFFSET_MULTIPLIER;
                    //realX += LoadLevel::GRID_SIZE * LoadLevel::BLACKHOLE_OFFSET_MULTIPLIER;
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
            tile->add<CTileTouched>(false);
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
            if (assetType == "GoldPipeTall" || "PipeTall")
                realY += LoadLevel::GRID_SIZE * LoadLevel::PIPETALL_REALY_OFFSET_MULTIPLIER;
            if (assetType == "EnemyGrave" || assetType == "EmperorGrave")
                realY -= 96.f * 1.5 - 13;
            if (assetType == "LevelDoor")
            {
                realY += LoadLevel::GRID_SIZE * 0.5;
            }

            std::string fullAssetName =  m_game.worldType + assetType;
            if (m_game.assets().hasAnimation(fullAssetName))
            {
                const Animation& anim = m_game.assets().getAnimation(fullAssetName);
                if (assetType.find("BushSmall") != std::string::npos)
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
                player->add<CPlayerEquipment>();
        
                auto& state = player->get<CState>();
                
                // -------------------
                // Original initialization
                // -------------------
                state.isInvincible = false;
                state.invincibilityTimer = 0.0f;
                state.isJumping = false;
                state.jumpTime = 0.0f;
                state.knockbackTimer = 0.0f;
                state.onGround = false;
                state.attackTime = 1.0f;
                state.attackCooldown = LoadLevel::PLAYER_ATTACK_COOLDOWN; 
                state.defenseTimer = 3.0f;
                state.shieldStamina = LoadLevel::PLAYER_SHIELD_STAMINA;

                std::cout << levelPath << std::endl;
                state.shieldStamina = (levelPath.find("ancient_rome_level_4_emperor_room") != std::string::npos) 
                ? LoadLevel::PLAYER_SHIELD_STAMINA * 2 
                : LoadLevel::PLAYER_SHIELD_STAMINA;

                state.maxshieldStamina = state.shieldStamina;
                
                // For the player's health
                player->add<CHealth>(PLAYER_HEALTH, PLAYER_HEALTH);
                playerSpawned = true;
        
                // Existing bullet/sword parameters
                state.bulletDamage         = LoadLevel::PLAYER_BULLET_DAMAGE;
                state.bulletBurstCount         = LoadLevel::PLAYER_BULLET_BURST_COUNT;
                state.maxConsecutiveSwordAttacks = LoadLevel::PLAYER_MAX_CONSECUTIVE_SWORD_ATTACKS;
                state.superBulletCount         = LoadLevel::PLAYER_SUPER_BULLET_COUNT;
                state.superBulletDamage        = LoadLevel::PLAYER_SUPER_BULLET_DAMAGE;
        
                // -------------------
                // NEW: Initialize separate cooldowns + burst fields
                // -------------------

                player->add<CAmmo>(LoadLevel::PLAYER_BULLET_COUNT, LoadLevel::PLAYER_BULLET_COUNT);
                
                // If you want bulletCooldown to start at 0, you can set it:
                state.bulletCooldown    = 0.f;
                state.bulletCooldownMax = LoadLevel::PLAYER_BULLET_COOLDOWN; 
                // e.g., 0.2f or 0.5f depending on your design
        
                state.swordCooldown     = 0.f; 
                state.swordCooldownMax  = LoadLevel::PLAYER_SWORD_COOLDOWN;
                // e.g., 0.3f so we can't spam sword instantly
        
                // Burst logic
                state.inBurst           = false;
                state.burstTimer        = 0.f;
                state.burstFireTimer    = 0.f;
                state.burstDuration     = LoadLevel::PLAYER_BULLET_BURSTDURATION;  
                state.burstInterval     = LoadLevel::PLAYER_BULLET_BURSTINTERVAL;
                state.bulletsShot       = 0;
        
                // Super move logic
                state.superBulletTimer    = 0.f;
                state.superBulletCooldown = LoadLevel::PLAYER_SUPER_BULLET_COOLDOWN;
                // e.g., 6.f or something else
                state.superMoveReady      = false;
        
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
            int enemyX, enemyY;
            if (!(file >> enemyTypeStr >> enemyX >> enemyY))
            {
                std::cerr << "[WARNING] Incomplete Enemy entry. Skipping.\n";
                continue;
            }
            enemyIndex++;
            std::string enemyID = enemyTypeStr + "_" + std::to_string(enemyIndex);
            auto enemy = entityManager.addEntity("enemy");
            enemy->add<CUniqueID>(enemyID);
            
            // Convert the enemyTypeStr to the proper EnemyType enum
            EnemyType enemyType;
            if (enemyTypeStr == "Fast" || enemyTypeStr == "EnemyFast") {
                enemyType = EnemyType::Fast;
            }
            else if (enemyTypeStr == "Normal" || enemyTypeStr == "EnemyNormal") {
                enemyType = EnemyType::Normal;
            }
            else if (enemyTypeStr == "Strong" || enemyTypeStr == "EnemyStrong") {
                enemyType = EnemyType::Strong;
            }
            else if (enemyTypeStr == "Elite" || enemyTypeStr == "EnemyElite") {
                enemyType = EnemyType::Elite;
            }
            else if (enemyTypeStr == "Emperor" || enemyTypeStr == "EnemyEmperor") {
                enemyType = EnemyType::Emperor;
            }
            else if (enemyTypeStr == "Super" || enemyTypeStr == "EnemySuper") {
                enemyType = EnemyType::Super;
            }
            else {
                std::cerr << "[WARNING] Unknown enemy type: " << enemyTypeStr << ". Defaulting to Normal.\n";
                enemyType = EnemyType::Normal;
            }
            
            // Now add the components based on the properly set enemyType
            enemy->add<CEnemyAI>();
            enemy->add<CHealth>();
            
            if (enemyType == EnemyType::Fast) {
                enemy->get<CEnemyAI>().enemyType = EnemyType::Fast;
                enemy->get<CEnemyAI>().damage = LoadLevel::ENEMY_FAST_DAMAGE;
                enemy->get<CState>().bulletDamage = LoadLevel::BULLET_DAMAGE_ENEMY_FAST;
                enemy->get<CEnemyAI>().speedMultiplier = LoadLevel::ENEMY_FAST_SPEED_MULTIPLIER;
                enemy->get<CState>().maxConsecutiveSwordAttacks = LoadLevel::ENEMY_FAST_MAX_CONSECUTIVE_SWORD_ATTACKS;
                enemy->get<CState>().bulletBurstCount = LoadLevel::ENEMY_FAST_BULLET_BURST_COUNT;
                enemy->get<CState>().superBulletCount = LoadLevel::ENEMY_FAST_SUPER_BULLET_COUNT;
                enemy->get<CState>().superBulletDamage = LoadLevel::ENEMY_FAST_SUPER_BULLET_DAMAGE;
                enemy->get<CEnemyAI>().enemyBehavior = EnemyBehavior::FollowOne;
                enemy->get<CHealth>().maxHealth = LoadLevel::ENEMY_FAST_HEALTH;
                enemy->get<CHealth>().currentHealth = LoadLevel::ENEMY_FAST_HEALTH;
            }
            else if (enemyType == EnemyType::Normal) {
                enemy->get<CEnemyAI>().enemyType = EnemyType::Normal;
                enemy->get<CEnemyAI>().damage = LoadLevel::ENEMY_NORMAL_DAMAGE;
                enemy->get<CState>().bulletDamage = LoadLevel::BULLET_DAMAGE_ENEMY_NORMAL;
                enemy->get<CEnemyAI>().speedMultiplier = LoadLevel::ENEMY_NORMAL_SPEED_MULTIPLIER;
                enemy->get<CState>().maxConsecutiveSwordAttacks = LoadLevel::ENEMY_NORMAL_MAX_CONSECUTIVE_SWORD_ATTACKS;
                enemy->get<CState>().bulletBurstCount = LoadLevel::ENEMY_NORMAL_BULLET_BURST_COUNT;
                enemy->get<CState>().superBulletCount = LoadLevel::ENEMY_NORMAL_SUPER_BULLET_COUNT;
                enemy->get<CState>().superBulletDamage = LoadLevel::ENEMY_NORMAL_SUPER_BULLET_DAMAGE;
                enemy->get<CEnemyAI>().enemyBehavior = EnemyBehavior::FollowOne;
                enemy->get<CHealth>().maxHealth = LoadLevel::ENEMY_NORMAL_HEALTH;
                enemy->get<CHealth>().currentHealth = LoadLevel::ENEMY_NORMAL_HEALTH;
            }
            else if (enemyType == EnemyType::Strong) {
                enemy->get<CEnemyAI>().enemyType = EnemyType::Strong;
                enemy->get<CEnemyAI>().damage = LoadLevel::ENEMY_STRONG_DAMAGE;
                enemy->get<CState>().bulletDamage = LoadLevel::BULLET_DAMAGE_ENEMY_STRONG;
                enemy->get<CEnemyAI>().speedMultiplier = LoadLevel::ENEMY_STRONG_SPEED_MULTIPLIER;
                enemy->get<CEnemyAI>().forcedCooldownDuration = 3.f;
                enemy->get<CState>().maxConsecutiveSwordAttacks = LoadLevel::ENEMY_STRONG_MAX_CONSECUTIVE_SWORD_ATTACKS;
                enemy->get<CState>().bulletBurstCount = LoadLevel::ENEMY_STRONG_BULLET_BURST_COUNT;
                enemy->get<CState>().superBulletCount = LoadLevel::ENEMY_STRONG_SUPER_BULLET_COUNT;
                enemy->get<CState>().superBulletDamage = LoadLevel::ENEMY_STRONG_SUPER_BULLET_DAMAGE;
                enemy->get<CEnemyAI>().enemyBehavior = EnemyBehavior::FollowOne;
                enemy->get<CHealth>().maxHealth = LoadLevel::ENEMY_STRONG_HEALTH;
                enemy->get<CHealth>().currentHealth = LoadLevel::ENEMY_STRONG_HEALTH;
            }
            else if (enemyType == EnemyType::Elite) {
                enemy->get<CEnemyAI>().enemyType = EnemyType::Elite;
                enemy->get<CEnemyAI>().damage = LoadLevel::ENEMY_ELITE_DAMAGE;
                enemy->get<CState>().bulletDamage = LoadLevel::BULLET_DAMAGE_ENEMY_ELITE;
                enemy->get<CEnemyAI>().speedMultiplier = LoadLevel::ENEMY_ELITE_SPEED_MULTIPLIER;
                enemy->get<CState>().maxConsecutiveSwordAttacks = LoadLevel::ENEMY_ELITE_MAX_CONSECUTIVE_SWORD_ATTACKS;
                enemy->get<CState>().bulletBurstCount = LoadLevel::ENEMY_ELITE_BULLET_BURST_COUNT;
                enemy->get<CState>().superBulletCount = LoadLevel::ENEMY_ELITE_SUPER_BULLET_COUNT;
                enemy->get<CState>().superBulletDamage = LoadLevel::ENEMY_ELITE_SUPER_BULLET_DAMAGE;
                enemy->get<CEnemyAI>().enemyBehavior = EnemyBehavior::FollowTwo;
                enemy->get<CHealth>().maxHealth = LoadLevel::ENEMY_ELITE_HEALTH;
                enemy->get<CHealth>().currentHealth = LoadLevel::ENEMY_ELITE_HEALTH;
            }
            else if (enemyType == EnemyType::Emperor) {
                enemy->get<CEnemyAI>().enemyType = EnemyType::Emperor;
                enemy->get<CEnemyAI>().damage = LoadLevel::ENEMY_EMPEROR_DAMAGE;
                enemy->get<CEnemyAI>().radialAttackDamage = LoadLevel::EMPEROR_RADIAL_SWORD_DAMAGE;
                enemy->get<CState>().bulletDamage = LoadLevel::BULLET_DAMAGE_ENEMY_EMPEROR;
                enemy->get<CEnemyAI>().speedMultiplier = LoadLevel::ENEMY_EMPEROR_SPEED_MULTIPLIER;
                enemy->get<CState>().maxConsecutiveSwordAttacks = LoadLevel::ENEMY_EMPEROR_MAX_CONSECUTIVE_SWORD_ATTACKS;
                enemy->get<CState>().bulletBurstCount = LoadLevel::ENEMY_EMPEROR_BULLET_BURST_COUNT;
                enemy->get<CState>().superBulletCount = LoadLevel::ENEMY_EMPEROR_SUPER_BULLET_COUNT;
                enemy->get<CState>().superBulletDamage = LoadLevel::ENEMY_EMPEROR_SUPER_BULLET_DAMAGE;
                enemy->get<CEnemyAI>().enemyBehavior = EnemyBehavior::FollowFour;
                // In the Emperor's initialization section or where you set up its health:
                if (m_game.worldType == "Future") {
                    // Future Emperor has 3x health
                    enemy->get<CHealth>().maxHealth = LoadLevel::ENEMY_EMPEROR_HEALTH * 2;
                    enemy->get<CHealth>().currentHealth = LoadLevel::ENEMY_EMPEROR_HEALTH * 2;
                } else {
                    // Normal health for other worlds
                    enemy->get<CHealth>().maxHealth = LoadLevel::ENEMY_EMPEROR_HEALTH;
                    enemy->get<CHealth>().currentHealth = LoadLevel::ENEMY_EMPEROR_HEALTH;
                }
            } else if (enemyType == EnemyType::Super) {
                enemy->get<CEnemyAI>().enemyType = EnemyType::Super;
                enemy->get<CEnemyAI>().damage = LoadLevel::ENEMY_SUPER_DAMAGE;
                enemy->get<CState>().bulletDamage = LoadLevel::BULLET_DAMAGE_ENEMY_SUPER;
                enemy->get<CEnemyAI>().speedMultiplier = LoadLevel::ENEMY_SUPER_SPEED_MULTIPLIER;
                enemy->get<CState>().maxConsecutiveSwordAttacks = LoadLevel::ENEMY_SUPER_MAX_CONSECUTIVE_SWORD_ATTACKS;
                enemy->get<CState>().bulletBurstCount = LoadLevel::ENEMY_SUPER_BULLET_BURST_COUNT;
                enemy->get<CState>().superBulletCount = LoadLevel::ENEMY_SUPER_SUPER_BULLET_COUNT;
                enemy->get<CState>().superBulletDamage = LoadLevel::ENEMY_SUPER_SUPER_BULLET_DAMAGE;
                enemy->get<CEnemyAI>().enemyBehavior = EnemyBehavior::FollowTwo;
                enemy->get<CHealth>().maxHealth = LoadLevel::ENEMY_SUPER_HEALTH;
                enemy->get<CHealth>().currentHealth = LoadLevel::ENEMY_SUPER_HEALTH;
            }

            if (m_game.worldType == "Future") {
                enemy->get<CEnemyAI>().speedMultiplier = enemy->get<CEnemyAI>().speedMultiplier * 0.5;
                enemy->add<CBossPhase>();
            }
            
            std::string runAnimName, standAnimName;
            if (enemyTypeStr == "Emperor")
            {
                standAnimName = m_game.worldType + "StandOldRomeEmperor";
                runAnimName = m_game.worldType + "RunOldRomeEmperor";
            }
            else
            {
                standAnimName = m_game.worldType + "StandAnim" + enemyTypeStr;
                runAnimName = m_game.worldType + "Run" + enemyTypeStr;
                std::cout << "[DEBUG] Stand Animation: " << standAnimName << std::endl;
                std::cout << "[DEBUG] Run Animation: " << runAnimName << std::endl;
            }
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
            
            std::cout << "[DEBUG] Enemy Damage: " << enemy->get<CEnemyAI>().damage
                     << " | Bullet Damage: " << enemy->get<CState>().bulletDamage << std::endl;
            std::cout << "[DEBUG] Spawned " << enemyTypeStr << " Enemy at ("
                     << enemyX << ", " << enemyY << ")" << std::endl;
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
