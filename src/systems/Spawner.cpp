#include "Spawner.h"
#include "SpriteUtils.h"
#include <iostream>
#include <random>
#include <cstdlib> 
#include <ctime>  

Spawner::Spawner(GameEngine& game, EntityManager& entityManager)
    : m_game(game), m_entityManager(entityManager)
{
}

// Spawn della spada del player
std::shared_ptr<Entity> Spawner::spawnSword(std::shared_ptr<Entity> player) {
    auto sword = m_entityManager.addEntity("sword");
    auto& pTrans = player->get<CTransform>();
    sword->add<CTransform>(pTrans.pos);
    sword->add<CLifeSpan>(PLAYER_SWORD_DURATION); // Durata della spada del player

    if (m_game.assets().hasAnimation("Sword")) {
        auto& swordAnim = m_game.assets().getAnimation("Sword");
        sword->add<CAnimation>(swordAnim, true);
        sf::Vector2i animSize = swordAnim.getSize();
        float w = static_cast<float>(animSize.x);
        float h = static_cast<float>(animSize.y);
        Vec2<float> boxSize(w, h);
        Vec2<float> halfSize(0.f, h * 0.5f);
        sword->add<CBoundingBox>(boxSize, halfSize);
    } else {
        std::cerr << "[ERROR] Missing Sword animation!\n";
    }
    return sword;
}
std::shared_ptr<Entity> Spawner::spawnPlayerBullet(std::shared_ptr<Entity> player) {
    // 1) Create the bullet entity
    auto bullet = m_entityManager.addEntity("playerBullet");

    // 2) Get player's transform to figure out where to spawn the bullet
    if (!player->has<CTransform>()) {
        std::cerr << "[ERROR] Player missing CTransform, cannot spawn bullet.\n";
        return nullptr;
    }
    auto& pTrans = player->get<CTransform>();

    // FIXED: Get the facing direction DIRECTLY from the transform where it's maintained
    float facingDir = pTrans.facingDirection;
    
    // Log the facing direction to debug
    std::cout << "[DEBUG] Player facing direction: " << facingDir << "\n";

    // 3) Calculate bullet spawn position & velocity
    float offsetX = facingDir * PLAYER_BULLET_OFFSET_X; // Simplified offset calculation
    float offsetY = PLAYER_BULLET_OFFSET_Y;
    Vec2<float> bulletPos = pTrans.pos + Vec2<float>(offsetX, offsetY);

    // Generate a random angle within a specified range
    // For example, ±15 degrees (±0.26 radians) from the original direction
    float randomAngleRange = 0.12f; // approx 15 degrees in radians
    float randomAngle = ((static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 2.0f - 1.0f) * randomAngleRange;
    
    // FIXED: Simplified angle calculation to avoid potential errors
    //float baseAngle = (facingDir < 0) ? M_PI : 0.0f; // Base angle depending on facing direction
    //float finalAngle = baseAngle + randomAngle;
    
    // FIXED: Add a sanity check to ensure valid facing direction
    if (facingDir == 0.0f) {
        std::cout << "[WARNING] Zero facing direction detected, defaulting to right.\n";
        facingDir = 1.0f; // Default to facing right if somehow facingDir is 0
    }
    
    // Calculate velocity - simpler approach
    Vec2<float> bulletVelocity;
    if (facingDir > 0) {
        // Facing right
        bulletVelocity.x = PLAYER_BULLET_SPEED * std::cos(randomAngle);
        bulletVelocity.y = PLAYER_BULLET_SPEED * std::sin(randomAngle);
    } else {
        // Facing left
        bulletVelocity.x = -PLAYER_BULLET_SPEED * std::cos(randomAngle);
        bulletVelocity.y = PLAYER_BULLET_SPEED * std::sin(randomAngle);
    }

    // 4) Add components to the bullet
    bullet->add<CTransform>(bulletPos, bulletVelocity);
    bullet->add<CLifeSpan>(PLAYER_BULLET_DURATION);

    // (Optional) If you want to store which player fired the bullet, add a CState or similar
    bullet->add<CState>("playerBullet"); 

    // 5) Choose an animation name (e.g., "FuturePlayerBullet")
    std::string animationName = "FuturePurpleBullet";

    // 6) Load & attach bullet animation
    if (m_game.assets().hasAnimation(animationName)) {
        auto& bulletAnim = m_game.assets().getAnimation(animationName);
        bullet->add<CAnimation>(bulletAnim, false);

        // Use the animation size for bounding box
        sf::Vector2i animSize = bulletAnim.getSize();
        float w = static_cast<float>(animSize.x);
        float h = static_cast<float>(animSize.y);
        bullet->add<CBoundingBox>(Vec2<float>(w, h), Vec2<float>(w * 0.5f, h * 0.5f));
    } else {
        std::cerr << "[ERROR] Missing animation for " << animationName << "!\n";
    }

    std::cout << "[DEBUG] Spawned player bullet at (" 
              << bulletPos.x << ", " << bulletPos.y << ") with velocity (" 
              << bulletVelocity.x << ", " << bulletVelocity.y << ")\n";
    return bullet;
}

std::shared_ptr<Entity> Spawner::spawnEnemyBullet(std::shared_ptr<Entity> enemy) {
    auto bullet = m_entityManager.addEntity("enemyBullet");

    // Copy relevant data from the enemy
    if (enemy->has<CEnemyAI>()) {
        bullet->add<CEnemyAI>(enemy->get<CEnemyAI>()); 
    }

    // Basic positioning & velocity
    auto& enemyAI = enemy->get<CEnemyAI>();
    auto& eTrans  = enemy->get<CTransform>();
    float dir     = enemyAI.facingDirection;

    // Bullet velocity goes left or right depending on dir
    Vec2<float> bulletVelocity(dir * ENEMY_BULLET_SPEED, 0.0f);

    // Offsets so the bullet spawns near the enemy
    float offsetX = (dir < 0) ? -ENEMY_BULLET_OFFSET_X : ENEMY_BULLET_OFFSET_X;
    float offsetY = ENEMY_BULLET_OFFSET_Y;
    Vec2<float> bulletPos = eTrans.pos + Vec2<float>(offsetX, offsetY);

    bullet->add<CTransform>(bulletPos, bulletVelocity);
    bullet->add<CLifeSpan>(ENEMY_BULLET_DURATION);
    bullet->add<CState>(std::to_string(enemy->id()));

    std::cout << "[DEBUG] Spawned enemy bullet at (" << bulletPos.x << ", " << bulletPos.y << ")\n";

    // Pick bullet animation based on enemy type
    std::string animationName;
    switch (enemyAI.enemyType) {
        case EnemyType::Emperor: animationName = "FutureRedBullet"; break; // Add this line
        case EnemyType::Elite:   animationName = "FutureBlackBullet"; break;
        case EnemyType::Strong:  animationName = "FutureRedBullet";  break;
        case EnemyType::Fast:    animationName = "FutureBlueBullet";  break;
        case EnemyType::Normal:  animationName = "FutureGoldBullet"; break;
        case EnemyType::Super:   animationName = "FutureSilverBullet"; break;  
        default:
            std::cerr << "[WARNING] Unhandled EnemyType in Spawner! Defaulting to FutureRedBullet.\n";
            animationName = "FutureRedBullet"; // Default case for safety
            break;
    }
    // Load bullet animation
    if (m_game.assets().hasAnimation(animationName)) {
        auto& bulletAnim = m_game.assets().getAnimation(animationName);
        bullet->add<CAnimation>(bulletAnim, false);

        // Use animation size for bounding box
        sf::Vector2i animSize = bulletAnim.getSize();
        float w = static_cast<float>(animSize.x);
        float h = static_cast<float>(animSize.y);
        bullet->add<CBoundingBox>(Vec2<float>(w, h), Vec2<float>(w * 0.5f, h * 0.5f));
    } else {
        std::cerr << "[ERROR] Missing " << animationName << " animation!\n";
    }

    return bullet;
}


// Spawn della spada del nemico
std::shared_ptr<Entity> Spawner::spawnEnemySword(std::shared_ptr<Entity> enemy) {
    auto sword = m_entityManager.addEntity("enemySword");
    auto& enemyAI = enemy->get<CEnemyAI>();
    auto& eTrans = enemy->get<CTransform>();

    float dir = enemyAI.facingDirection;
    float offsetX = (dir < 0) ? -ENEMY_SWORD_OFFSET_X : ENEMY_SWORD_OFFSET_X;
    float offsetY = ENEMY_SWORD_OFFSET_Y;

    
    Vec2<float> swordPos = eTrans.pos + Vec2<float>(offsetX, offsetY);
    sword->add<CTransform>(swordPos);
    sword->add<CLifeSpan>(ENEMY_SWORD_DURATION);
    sword->add<CState>(std::to_string(enemy->id()));

    std::cout << "[DEBUG] Spawned enemy sword at (" << swordPos.x << ", " << swordPos.y << ")\n";

    // Determine sword animation based on world and enemy type
    std::string animationName;

    animationName = (enemyAI.enemyType == EnemyType::Super) ? "SuperSword" : "EnemySword";

    std::cout << "[DEBUG] Using " << animationName << " animation for enemy sword.\n";

    if (m_game.assets().hasAnimation(animationName)) {
        auto& swordAnim = m_game.assets().getAnimation(animationName);
        sword->add<CAnimation>(swordAnim, false);
        sf::Vector2i animSize = swordAnim.getSize();
        float w = static_cast<float>(animSize.x);
        float h = static_cast<float>(animSize.y);
        Vec2<float> boxSize(w, h);
        Vec2<float> halfSize(w * 0.5f, h * 0.5f);
        sword->add<CBoundingBox>(boxSize, halfSize);

        if (dir < 0)
            flipSpriteLeft(sword->get<CAnimation>().animation.getMutableSprite());
        else
            flipSpriteRight(sword->get<CAnimation>().animation.getMutableSprite());
    } else {
        std::cerr << "[ERROR] Missing " << animationName << " animation!\n";
    }
    
    // Copy AI data to sword
    if (enemy->has<CEnemyAI>()) {
        sword->add<CEnemyAI>(enemy->get<CEnemyAI>());
    }

    return sword;
}

// Esempio: spada con offset Y casuale (singola spada)
std::shared_ptr<Entity> Spawner::spawnEmperorSwordOffset(std::shared_ptr<Entity> enemy) {
    auto sword = m_entityManager.addEntity("EmperorSword");

    auto& eTrans = enemy->get<CTransform>();
    auto& eAI    = enemy->get<CEnemyAI>();

    float dir = eAI.facingDirection;
    
    // Offset X fixed (left or right of enemy)
    float offsetX = (dir < 0) ? -EMPEROR_SWORD_OFFSET_X : EMPEROR_SWORD_OFFSET_X;

    // Random Y offset between 0 and 80
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> distY(0.f, 80.f);
    float offsetY = distY(gen);

    Vec2<float> swordPos = eTrans.pos + Vec2<float>(offsetX, offsetY);
    sword->add<CTransform>(swordPos);
    sword->add<CLifeSpan>(ENEMY_SWORD_DURATION);
    
    // Store the creator ID and sword type in the state component
    sword->add<CState>(std::to_string(enemy->id()));  // Not setting "radial" flag means it's a regular sword

    // Load animation
    if (m_game.assets().hasAnimation("EmperorSword")) {
        auto& swordAnim = m_game.assets().getAnimation("EmperorSword");
        sword->add<CAnimation>(swordAnim, false);
        sf::Vector2i animSize = swordAnim.getSize();
        Vec2<float> boxSize(animSize.x, animSize.y);
        Vec2<float> halfSize(boxSize.x * 0.5f, boxSize.y * 0.5f);
        sword->add<CBoundingBox>(boxSize, halfSize);
        if (dir < 0) flipSpriteLeft(sword->get<CAnimation>().animation.getMutableSprite());
        else         flipSpriteRight(sword->get<CAnimation>().animation.getMutableSprite());
    } else {
        std::cerr << "[ERROR] Missing enemy sword animation!\n";
    }

    // Copy AI/damage data from enemy
    if (enemy->has<CEnemyAI>()) {
        sword->add<CEnemyAI>(enemy->get<CEnemyAI>());
    }

    std::cout << "[DEBUG] Spawned Emperor sword with random Y offset at (" 
              << swordPos.x << ", " << swordPos.y << ")\n";

    return sword;
}

void Spawner::spawnEmperorSwordsRadial(std::shared_ptr<Entity> enemy, int swordCount, float radius, float swordSpeed) {
    auto& eTrans = enemy->get<CTransform>();

    float centerX = eTrans.pos.x;
    float centerY = eTrans.pos.y;

    // Generate a random angle offset for this burst (between 0 and 60 degrees)
    float randomAngleOffset = (std::rand() % 60); 

    for (int i = 0; i < swordCount; i++) {
        // Apply random offset to the base angle calculation
        float angleDeg = (360.f / swordCount) * i + randomAngleOffset;
        float angleRad = angleDeg * 3.1415926535f / 180.f;

        // Calculate radial position
        float offsetX = std::cos(angleRad) * radius;
        float offsetY = std::sin(angleRad) * radius;
        Vec2<float> spawnPos(centerX + offsetX, centerY + offsetY);

        // Create EmperorSword entity but mark it as radial
        auto sword = m_entityManager.addEntity("EmperorSword");
        sword->add<CTransform>(spawnPos);
        sword->add<CLifeSpan>(EMPEROR_ROTATING_SWORD_DURATION);
        
        // Store the creator ID and the "radial" flag in the state component
        CState stateComponent(std::to_string(enemy->id()));
        stateComponent.state = "radial";  // This flag identifies it as a radial sword
        sword->add<CState>(stateComponent);

        // Load animation
        if (m_game.assets().hasAnimation("EmperorSword")) {
            auto& swordAnim = m_game.assets().getAnimation("EmperorSword");
            sword->add<CAnimation>(swordAnim, false);

            sf::Vector2i animSize = swordAnim.getSize();
            Vec2<float> boxSize(animSize.x, animSize.y);
            Vec2<float> halfSize(boxSize.x * 0.5f, boxSize.y * 0.5f);
            sword->add<CBoundingBox>(boxSize, halfSize);

            // Rotate sprite to point outward
            auto& sprite = sword->get<CAnimation>().animation.getMutableSprite();
            sprite.setRotation(angleDeg);
        } else {
            std::cerr << "[ERROR] Missing EmperorSword animation!\n";
        }

        // Copy AI/Damage properties from enemy
        if (enemy->has<CEnemyAI>()) {
            sword->add<CEnemyAI>(enemy->get<CEnemyAI>());
        }

        // Assign velocity to CTransform
        float vx = std::cos(angleRad) * swordSpeed;
        float vy = std::sin(angleRad) * swordSpeed;
        sword->get<CTransform>().velocity = Vec2<float>(vx, vy);

        // std::cout << "[DEBUG] Spawned Emperor radial sword " << i 
        //           << " angle=" << angleDeg 
        //           << " deg (random offset=" << randomAngleOffset << ")"
        //           << " pos(" << spawnPos.x << "," << spawnPos.y 
        //           << ") velocity(" << vx << "," << vy << ")\n";
    }
}

void Spawner::spawnEnemyGrave(const Vec2<float>& position, bool isEmperor) {
    auto grave = m_entityManager.addEntity("enemyGrave");

    // Offset to spawn at the top of the enemy
    float spawnHeightOffset = 96;
    Vec2<float> spawnPos(position.x, position.y - spawnHeightOffset);

    grave->add<CTransform>(spawnPos, Vec2<float>(0.f, 0.f)); // Start with zero velocity
    grave->add<CGravity>(1000.f); // Apply gravity

    std::string graveAnimName = m_game.worldType + (isEmperor ? "EmperorGrave" : "EnemyGrave");
    std::cout << graveAnimName << std::endl;

    if (m_game.assets().hasAnimation(graveAnimName)) {
        auto& graveAnim = m_game.assets().getAnimation(graveAnimName);
        grave->add<CAnimation>(graveAnim, true); // Loop the animation
        Vec2<float> size(static_cast<float>(graveAnim.getSize().x), static_cast<float>(graveAnim.getSize().y));
        grave->add<CBoundingBox>(size, size * 0.5f);
    } else {
        std::cerr << "[ERROR] Missing grave animation: " << graveAnimName << "!\n";
    }

    std::cout << "[DEBUG] Spawned " << (isEmperor ? "Emperor" : "Enemy") 
              << " grave at (" << spawnPos.x << ", " << spawnPos.y << "), affected by gravity.\n";
}

void Spawner::spawnEmperorSwordArmorRadial(std::shared_ptr<Entity> enemy, int swordCount, float radius, float swordSpeed, float initialStopTime, float stopTimeIncrement)
{
    auto& eTrans = enemy->get<CTransform>();
    float centerX = eTrans.pos.x;
    float centerY = eTrans.pos.y;

    // Generate random angle offset for visual variety
    float randomAngleOffset = (std::rand() % 60);

    for (int i = 0; i < swordCount; i++) {
        float angleDeg = (360.f / swordCount) * i + randomAngleOffset;
        float angleRad = angleDeg * 3.1415926535f / 180.f;

        float offsetX = std::cos(angleRad) * radius;
        float offsetY = std::sin(angleRad) * radius;
        Vec2<float> spawnPos(centerX + offsetX, centerY + offsetY);

        auto sword = m_entityManager.addEntity("EmperorSwordArmor");
        sword->add<CTransform>(spawnPos);
        sword->add<CLifeSpan>(EMPEROR_ROTATING_SWORD_DURATION);

        CState stateComponent(std::to_string(enemy->id()));
        stateComponent.state = "radialArmor";
        sword->add<CState>(stateComponent);

        // Animation setup
        if (m_game.assets().hasAnimation("EmperorSword")) {
            auto& swordAnim = m_game.assets().getAnimation("EmperorSword");
            sword->add<CAnimation>(swordAnim, false);

            sf::Vector2i animSize = swordAnim.getSize();
            Vec2<float> boxSize(animSize.x, animSize.y);
            Vec2<float> halfSize(boxSize.x * 0.5f, boxSize.y * 0.5f);
            sword->add<CBoundingBox>(boxSize, halfSize);

            // Set rotation pointing outward
            auto& sprite = sword->get<CAnimation>().animation.getMutableSprite();
            sprite.setRotation(angleDeg);
        } else {
            std::cerr << "[ERROR] Missing EmperorSword animation!\n";
        }

        // Copy Enemy AI properties
        if (enemy->has<CEnemyAI>()) {
            sword->add<CEnemyAI>(enemy->get<CEnemyAI>());
        }

        // Assign velocity
        float vx = std::cos(angleRad) * swordSpeed;
        float vy = std::sin(angleRad) * swordSpeed;
        sword->get<CTransform>().velocity = Vec2<float>(vx, vy);

        // Gradual stop logic (incremental timing for fan-out effect)
        sword->add<CStopAfterTime>(initialStopTime + stopTimeIncrement);

        // std::cout << "[DEBUG] Spawned Armor sword angle=" << angleDeg 
        //           << " stopTimer=" << (initialStopTime + stopTimeIncrement)
        //           << " velocity=(" << vx << ", " << vy << ")\n";
    }
}
// Spawn degli item
std::shared_ptr<Entity> Spawner::spawnItem(const Vec2<float>& position, const std::string& tileType) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::string itemName;
    Vec2<float> spawnPos = position;

    if (tileType == m_game.worldType + "Box1" || tileType == m_game.worldType + "Box2" ) {  
        // 5 possibili risultati: 20% ciascuno
        //  1) CoinBronze
        //  2) CoinSilver
        //  3) GrapeSmall
        //  4) ChickenSmall
        //  5) Nessun item
        std::uniform_int_distribution<int> dist(0, 99);
        int roll = dist(gen);

        if (roll < 15) {
            itemName = m_game.worldType + "CoinBronze";
        } else if (roll < 30) {
            itemName = m_game.worldType + "CoinSilver";
        } else if (roll < 60) {
            itemName = m_game.worldType + "GrapeSmall";
        } else if (roll < 90) {
            itemName = m_game.worldType + "ChickenSmall";
        } else {
            std::cout << "[DEBUG] Box is empty, no item spawned.\n";
            return nullptr; 
        }
    } 
    else if (tileType == m_game.worldType + "TreasureHit") {
        // 4 possibili risultati: 25% ciascuno
        //  1) CoinGold
        //  2) GrapeBig
        //  3) ChickenBig
        //  4) Nessun item
        std::cout << "Hit treasure box\n";
        std::uniform_int_distribution<int> dist(0, 99);
        int roll = dist(gen);

        if (roll < 20) {
            itemName = m_game.worldType + "CoinGold";
        } else if (roll < 60) {
            itemName = m_game.worldType + "GrapeBig";
        } else if (roll < 95) {
            itemName = m_game.worldType + "ChickenBig";
        } else {
            std::cout << "[DEBUG] Treasure is empty, no item spawned.\n";
            return nullptr; 
        }
        // Se esce un item, lo spawniamo un po' più in alto
        spawnPos.y -= TREASURE_BOX_TILE_SIZE;
    } 
    else {
        std::cerr << "[ERROR] Unknown tile type in spawnItem: " << tileType << std::endl;
        return nullptr;
    }

    // Se itemName è impostato, creiamo l'entità
    auto item = m_entityManager.addEntity("collectable");
    item->add<CTransform>(spawnPos);

    // Carichiamo l'animazione associata all'item
    if (m_game.assets().hasAnimation(itemName)) {
        auto& anim = m_game.assets().getAnimation(itemName);
        item->add<CAnimation>(anim, true);
    } else {
        std::cerr << "[ERROR] Missing animation for item: " << itemName << std::endl;
        item->destroy();
        return nullptr;
    }

    // Creiamo bounding box basandoci sulla dimensione dell'animazione
    auto& anim = item->get<CAnimation>().animation;
    Vec2<float> animSize(static_cast<float>(anim.getSize().x), static_cast<float>(anim.getSize().y));
    Vec2<float> bboxSize = animSize * COLLECTABLE_SCALE_FACTOR;
    Vec2<float> bboxOffset = bboxSize * 0.5f;
    item->add<CBoundingBox>(bboxSize, bboxOffset);

    // Aggiungiamo un CState con il nome dell'item (opzionale)
    item->add<CState>(itemName);

    std::cout << "[DEBUG] Spawned " << itemName << " from " << tileType
              << " at (" << spawnPos.x << ", " << spawnPos.y << ")" << std::endl;
    return item;
}

// Aggiornamento dei frammenti
void Spawner::updateFragments(float deltaTime) {
    for (auto& fragment : m_entityManager.getEntities("fragment")) {
        auto& transform = fragment->get<CTransform>();
        auto& anim      = fragment->get<CAnimation>();
        auto& lifespan  = fragment->get<CLifeSpan>();

        transform.pos += transform.velocity * deltaTime;

        sf::Color color = anim.animation.getMutableSprite().getColor();
        float alpha = (lifespan.remainingTime / lifespan.totalTime) * ALPHA_MAX;
        color.a = static_cast<int>(std::max(0.0f, alpha));
        anim.animation.getMutableSprite().setColor(color);

        lifespan.remainingTime -= deltaTime;
        if (lifespan.remainingTime <= 0.0f) {
            fragment->destroy();
        }
    }
}

std::shared_ptr<Entity> Spawner::spawnBlackHoleAfterTileDestruction(const Vec2<float>& position)
{
    // 2) Spawn the permanent black-hole tile
    //
    auto blackHoleTile = m_entityManager.addEntity("tile");
    blackHoleTile->add<CTransform>(position);

    // Load black hole tile animation
    if (m_game.assets().hasAnimation("AlienBlackHoleAttack")) {
        auto& tileAnim = m_game.assets().getAnimation("AlienBlackHoleAttack");
        blackHoleTile->add<CAnimation>(tileAnim, true); // loop
        sf::Vector2i animSize = tileAnim.getSize();
        Vec2<float> boxSize(animSize.x, animSize.y);
        Vec2<float> boxOffset(boxSize.x * 0.5f, boxSize.y * 0.5f);
        blackHoleTile->add<CBoundingBox>(boxSize, boxOffset);
    } else {
        std::cerr << "[ERROR] Missing AlienBlackHoleRedSmall animation!\n";
    }

    std::cout << "[DEBUG] Spawned BlackHoleVanish + lethal BlackHoleTilc at (" 
              << position.x << ", " << position.y << ")\n";

    return blackHoleTile;
}

void Spawner::updateGraves(float deltaTime) {
    for (auto& grave : m_entityManager.getEntities("enemyGrave")) {
        auto& transform = grave->get<CTransform>();
        auto& velocity  = transform.velocity;
        float gravity   = grave->has<CGravity>() ? grave->get<CGravity>().gravity : 1000.f;

        // Apply gravity
        velocity.y += gravity * deltaTime;
        velocity.y = std::min(velocity.y, 800.f); // Clamp to avoid high speed

        // Check collision with ground
        bool onGround = false;
        for (auto& tile : m_entityManager.getEntities("tile")) {
            if (!tile->has<CTransform>() || !tile->has<CBoundingBox>()) continue;

            auto& tileTrans = tile->get<CTransform>();
            auto& tileBB    = tile->get<CBoundingBox>();
            sf::FloatRect tileRect = tileBB.getRect(tileTrans.pos);
            sf::FloatRect graveRect = grave->get<CBoundingBox>().getRect(transform.pos + velocity * deltaTime);

            if (tileRect.intersects(graveRect)) {
                onGround = true;
                transform.velocity.y = 0.f;
                break;
            }
        }

        if (!onGround) {
            transform.pos += velocity * deltaTime;
        } else {
            velocity.y = 0.f;
        }
    }
}

// Creazione dei frammenti del blocco
void Spawner::createBlockFragments(const Vec2<float>& position, const std::string & blockType) {
    std::vector<Vec2<float>> directions = {
        {-1, -1}, {0, -1}, {1, -1},
        {-1,  0}, {1,  0},
        {-1,  1}, {0,  1}, {1,  1}
    };

    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<int> angleDist(FRAGMENT_ANGLE_MIN, FRAGMENT_ANGLE_MAX);
    std::uniform_int_distribution<int> rotationSpeedDist(FRAGMENT_ROTATION_SPEED_MIN, FRAGMENT_ROTATION_SPEED_MAX);

    for (auto dir : directions) {
        auto fragment = m_entityManager.addEntity("fragment");
        fragment->add<CTransform>(position, Vec2<float>(dir.x * FRAGMENT_SPREAD_SPEED, dir.y * FRAGMENT_SPREAD_SPEED));

        if (m_game.assets().hasAnimation(blockType)) {
            const Animation& anim = m_game.assets().getAnimation(blockType);
            fragment->add<CAnimation>(anim, false);
            sf::Sprite& sprite = fragment->get<CAnimation>().animation.getMutableSprite();
            sf::Vector2i textureSize = anim.getSize();
            float scaleX = FRAGMENT_SIZE / static_cast<float>(textureSize.x);
            float scaleY = FRAGMENT_SIZE / static_cast<float>(textureSize.y);
            sprite.setScale(scaleX, scaleY);
        } else {
            std::cerr << "[ERROR] Missing animation for fragments: " << blockType << "\n";
        }

        fragment->add<CRotation>(angleDist(gen), rotationSpeedDist(gen));
        fragment->add<CLifeSpan>(FRAGMENT_DURATION);
    }
}

// Add this to your Spawner.cpp file

// Spawns bullets in a radial pattern around the Emperor
void Spawner::spawnEmperorBulletsRadial(std::shared_ptr<Entity> enemy, int bulletCount, 
                                     float radius, float bulletSpeed, const std::string& bulletType) {
    auto& eTrans = enemy->get<CTransform>();

    float centerX = eTrans.pos.x;
    float centerY = eTrans.pos.y;

    // Generate a random angle offset for this burst (between 0 and 60 degrees)
    float randomAngleOffset = (std::rand() % 60); 

    for (int i = 0; i < bulletCount; i++) {
        // Apply random offset to the base angle calculation
        float angleDeg = (360.f / bulletCount) * i + randomAngleOffset;
        float angleRad = angleDeg * 3.1415926535f / 180.f;

        // Calculate radial position
        float offsetX = std::cos(angleRad) * radius;
        float offsetY = std::sin(angleRad) * radius;
        Vec2<float> spawnPos(centerX + offsetX, centerY + offsetY);

        // Create enemyBullet entity
        auto bullet = m_entityManager.addEntity("enemyBullet");
        bullet->add<CTransform>(spawnPos);
        bullet->add<CLifeSpan>(3.0f); // Bullet lifespan
        
        // Store the creator ID in the state component
        CState stateComponent(std::to_string(enemy->id()));
        bullet->add<CState>(stateComponent);

        // Determine which bullet animation to use based on provided type or phase
        std::string animName;
        
        if (bulletType == "Random") {
            // Random bullets for final phase or mixed attacks
            int randType = std::rand() % 4; // 0-3: Normal, Fast, Strong, Elite
            switch (randType) {
                case 0: animName = "FutureGoldBullet"; break;  // Normal
                case 1: animName = "FutureBlueBullet"; break;  // Fast
                case 2: animName = "FutureRedBullet"; break;   // Strong
                case 3: animName = "FutureBlackBullet"; break; // Elite
                default: animName = "FutureGoldBullet"; break;
            }
        } else if (bulletType == "Normal") {
            animName = "FutureGoldBullet";  // Gold bullets
        } else if (bulletType == "Fast") {
            animName = "FutureBlueBullet";  // Blue bullets
        } else if (bulletType == "Strong") {
            animName = "FutureRedBullet";   // Red bullets
        } else if (bulletType == "Elite") {
            animName = "FutureBlackBullet"; // Black bullets
        } else if (bulletType == "Emperor") {
            animName = "FuturePurpleBullet";   // Default Emperor bullet (red)
        } else {
            // Default to Emperor bullet type (red) if no valid type specified
            animName = "FuturePurpleBullet";
        }
        
        // Load animation
        if (m_game.assets().hasAnimation(animName)) {
            auto& bulletAnim = m_game.assets().getAnimation(animName);
            bullet->add<CAnimation>(bulletAnim, true);

            sf::Vector2i animSize = bulletAnim.getSize();
            Vec2<float> boxSize(animSize.x, animSize.y);
            Vec2<float> halfSize(boxSize.x * 0.5f, boxSize.y * 0.5f);
            bullet->add<CBoundingBox>(boxSize, halfSize);

            // Rotate sprite to point outward
            auto& sprite = bullet->get<CAnimation>().animation.getMutableSprite();
            sprite.setRotation(angleDeg);
        } else {
            std::cerr << "[ERROR] Missing " << animName << " animation!\n";
        }

        // Assign velocity to CTransform
        float vx = std::cos(angleRad) * bulletSpeed;
        float vy = std::sin(angleRad) * bulletSpeed;
        bullet->get<CTransform>().velocity = Vec2<float>(vx, vy);
    }
}
// Spawns black holes in multiple directions that destroy tiles on impact
void Spawner::spawnEmperorBlackHoles(std::shared_ptr<Entity> enemy, int blackHoleCount, 
                                     float radius, float blackHoleSpeed) {
    auto& eTrans = enemy->get<CTransform>();

    float centerX = eTrans.pos.x;
    float centerY = eTrans.pos.y;

    // Generate a random angle offset for this burst (between 0 and 36 degrees)
    float randomAngleOffset = (std::rand() % 36); 

    for (int i = 0; i < blackHoleCount; i++) {
        // Apply random offset to the base angle calculation
        float angleDeg = (360.f / blackHoleCount) * i + randomAngleOffset;
        float angleRad = angleDeg * 3.1415926535f / 180.f;

        // Calculate radial position
        float offsetX = std::cos(angleRad) * radius;
        float offsetY = std::sin(angleRad) * radius;
        Vec2<float> spawnPos(centerX + offsetX, centerY + offsetY);

        // Create blackHole entity
        auto blackHole = m_entityManager.addEntity("emperorBlackHole");
        blackHole->add<CTransform>(spawnPos);
        blackHole->add<CLifeSpan>(5.0f); // Black hole lifespan - longer than bullets
        
        // Store the creator ID in the state component
        blackHole->add<CState>(std::to_string(enemy->id()));

        // Set animation for black hole
        std::string animName = "AlienBlackHoleAttack";
        
        // Load animation
        if (m_game.assets().hasAnimation(animName)) {
            auto& blackHoleAnim = m_game.assets().getAnimation(animName);
            blackHole->add<CAnimation>(blackHoleAnim, true);

            sf::Vector2i animSize = blackHoleAnim.getSize();
            Vec2<float> boxSize(animSize.x, animSize.y);
            Vec2<float> halfSize(boxSize.x * 0.5f, boxSize.y * 0.5f);
            blackHole->add<CBoundingBox>(boxSize, halfSize);
        } else {
            std::cerr << "[ERROR] Missing " << animName << " animation!\n";
        }

        // Assign velocity to CTransform
        float vx = std::cos(angleRad) * blackHoleSpeed;
        float vy = std::sin(angleRad) * blackHoleSpeed;
        blackHole->get<CTransform>().velocity = Vec2<float>(vx, vy);

        std::cout << "[DEBUG] Spawned Emperor black hole " << i 
                  << " angle=" << angleDeg 
                  << " deg (random offset=" << randomAngleOffset << ")"
                  << " pos(" << spawnPos.x << "," << spawnPos.y 
                  << ") velocity(" << vx << "," << vy << ")\n";
    }
}

// Overload without bulletType for backward compatibility
void Spawner::spawnEmperorBulletsRadial(std::shared_ptr<Entity> enemy, int bulletCount, 
                                      float radius, float bulletSpeed) {
    // Default to random bullets if no type specified
    spawnEmperorBulletsRadial(enemy, bulletCount, radius, bulletSpeed, "Random");
}
