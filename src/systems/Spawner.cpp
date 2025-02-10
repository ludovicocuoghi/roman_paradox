#include "Spawner.h"
#include "Entity.hpp"
#include "Components.hpp"
#include "Animation.hpp"
#include "Vec2.hpp"
#include <iostream>
#include <random>
#include "SpriteUtils.h"  // Per flipSpriteLeft / flipSpriteRight

Spawner::Spawner(GameEngine& game, EntityManager& entityManager)
    : m_game(game), m_entityManager(entityManager)
{
}

std::shared_ptr<Entity> Spawner::spawnSword(std::shared_ptr<Entity> player) {
    auto sword = m_entityManager.addEntity("sword");
    auto& pTrans = player->get<CTransform>();
    sword->add<CTransform>(pTrans.pos, Vec2<float>(0.f, 0.f));
    // Imposta una durata breve per la spada (es. 0.2 secondi)
    sword->add<CLifeSpan>(0.2f);

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

std::shared_ptr<Entity> Spawner::spawnEnemySword(std::shared_ptr<Entity> enemy) {
    auto sword = m_entityManager.addEntity("enemySword");
    auto& enemyAI = enemy->get<CEnemyAI>();
    auto& eTrans = enemy->get<CTransform>();

    float dir = enemyAI.facingDirection;
    float offsetX = (dir < 0) ? -50.f : 50.f;
    float offsetY = 10.f;
    Vec2<float> swordPos = eTrans.pos + Vec2<float>(offsetX, offsetY);
    sword->add<CTransform>(swordPos);
    sword->add<CLifeSpan>(0.3f);
    // Memorizza l'ID del nemico nello stato per eventuale logica successiva
    sword->add<CState>(std::to_string(enemy->id()));

    if (m_game.assets().hasAnimation("Sword")) {
        auto& swordAnim = m_game.assets().getAnimation("Sword");
        sword->add<CAnimation>(swordAnim, true);
        sf::Vector2i animSize = swordAnim.getSize();
        float w = static_cast<float>(animSize.x);
        float h = static_cast<float>(animSize.y);
        Vec2<float> boxSize(w, h);
        Vec2<float> halfSize(w * 0.5f, h * 0.5f);
        sword->add<CBoundingBox>(boxSize, halfSize);
        // Flip dello sprite in base alla direzione del nemico
        if (dir < 0)
            flipSpriteLeft(sword->get<CAnimation>().animation.getMutableSprite());
        else
            flipSpriteRight(sword->get<CAnimation>().animation.getMutableSprite());
    } else {
        std::cerr << "[ERROR] Missing Sword animation for enemy sword!\n";
    }
    return sword;
}

std::shared_ptr<Entity> Spawner::spawnItem(const Vec2<float>& position, const std::string& tileType) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::string itemName;
    Vec2<float> spawnPos = position;

    if (tileType == "Box1" || tileType == "Box2") {
        std::uniform_int_distribution<> dist(0, 9);
        int roll = dist(gen);
        if (roll < 5)
            itemName = "CoinBronze";
        else if (roll < 9)
            itemName = "GrapeSmall";
        else
            itemName = "CoinSilver";
    } else if (tileType == "TreasureBoxAnim") {
        std::uniform_int_distribution<> dist(0, 1);
        itemName = (dist(gen) == 0) ? "CoinGold" : "GrapeBig";
        float tileSize = 96.0f;
        spawnPos.y -= tileSize;
    } else {
        std::cerr << "[ERROR] Unknown tile type in spawnItem: " << tileType << std::endl;
        return nullptr;
    }

    auto item = m_entityManager.addEntity("collectable");
    item->add<CTransform>(spawnPos);
    if (m_game.assets().hasAnimation(itemName)) {
        auto& anim = m_game.assets().getAnimation(itemName);
        item->add<CAnimation>(anim, true);  // Animazione in loop
    } else {
        std::cerr << "[ERROR] Missing animation for item: " << itemName << std::endl;
        item->destroy();
        return nullptr;
    }
    auto& anim = item->get<CAnimation>().animation;
    Vec2<float> animSize(static_cast<float>(anim.getSize().x), static_cast<float>(anim.getSize().y));
    float scaleFactor = 0.3f;
    Vec2<float> bboxSize = animSize * scaleFactor;
    Vec2<float> bboxOffset = bboxSize * 0.5f;
    item->add<CBoundingBox>(bboxSize, bboxOffset);
    item->add<CState>(itemName);

    std::cout << "[DEBUG] Spawned " << itemName << " from " << tileType
              << " at (" << spawnPos.x << ", " << spawnPos.y << ")" << std::endl;
    return item;
}
