#include "Spawner.h"
#include "SpriteUtils.h"
#include <iostream>
#include <random>

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

    if (m_game.assets().hasAnimation("EnemySword")) {
        std::cout << "[DEBUG] Using EnemySword animation for enemy sword.\n";
        auto& swordAnim = m_game.assets().getAnimation("EnemySword");
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
    } else if (m_game.assets().hasAnimation("Sword")) {
        auto& swordAnim = m_game.assets().getAnimation("Sword");
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
        std::cerr << "[ERROR] Missing enemy sword animation!\n";
    }
    
    // Attach a CEnemyAI component to the sword and copy the damage from the enemy.
    if (enemy->has<CEnemyAI>()) {
        sword->add<CEnemyAI>(enemy->get<CEnemyAI>());
    }

    return sword;
}

// Spawn degli item
std::shared_ptr<Entity> Spawner::spawnItem(const Vec2<float>& position, const std::string& tileType) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::string itemName;
    Vec2<float> spawnPos = position;

    if (tileType == "Box1" || tileType == "Box2") {
        std::uniform_int_distribution<> dist(BOX_ITEM_DIST_MIN, BOX_ITEM_DIST_MAX);
        int roll = dist(gen);
        if (roll < BOX_ITEM_THRESHOLD_COINBRONZE)
            itemName = "CoinBronze";
        else if (roll < BOX_ITEM_THRESHOLD_GRAPESMALL)
            itemName = "GrapeSmall";
        else
            itemName = "CoinSilver";
    } else if (tileType == "TreasureBoxAnim") {
        std::uniform_int_distribution<> dist(TREASURE_BOX_DIST_MIN, TREASURE_BOX_DIST_MAX);
        itemName = (dist(gen) == 0) ? "CoinGold" : "GrapeBig";
        spawnPos.y -= TREASURE_BOX_TILE_SIZE;
    } else {
        std::cerr << "[ERROR] Unknown tile type in spawnItem: " << tileType << std::endl;
        return nullptr;
    }

    auto item = m_entityManager.addEntity("collectable");
    item->add<CTransform>(spawnPos);
    if (m_game.assets().hasAnimation(itemName)) {
        auto& anim = m_game.assets().getAnimation(itemName);
        item->add<CAnimation>(anim, true);
    } else {
        std::cerr << "[ERROR] Missing animation for item: " << itemName << std::endl;
        item->destroy();
        return nullptr;
    }
    auto& anim = item->get<CAnimation>().animation;
    Vec2<float> animSize(static_cast<float>(anim.getSize().x), static_cast<float>(anim.getSize().y));
    Vec2<float> bboxSize = animSize * COLLECTABLE_SCALE_FACTOR;
    Vec2<float> bboxOffset = bboxSize * 0.5f;
    item->add<CBoundingBox>(bboxSize, bboxOffset);
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
