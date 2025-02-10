#include "CollisionSystem.h"
#include "Physics.hpp"
#include <iostream>
#include <algorithm>
#include <limits>
#include <SFML/Graphics.hpp>

CollisionSystem::CollisionSystem(EntityManager& entityManager, GameEngine& game, Spawner* spawner)
    : m_entityManager(entityManager), m_game(game), m_spawner(spawner) {}

void CollisionSystem::updateCollisions() {
    handlePlayerTileCollisions();
    handleEnemyTileCollisions();
    handlePlayerEnemyCollisions();
    handleSwordCollisions();
    handlePlayerCollectibleCollisions(); // Nuova funzione per i collectible
}

// ----------------------------------
// 🏃 PLAYER - TILE COLLISIONS
// ----------------------------------
void CollisionSystem::handlePlayerTileCollisions() {
    for (auto& player : m_entityManager.getEntities("player")) {
        auto& transform = player->get<CTransform>();
        auto& velocity  = transform.velocity;
        auto& playerBB  = player->get<CBoundingBox>();
        auto& state     = player->get<CState>();

        sf::FloatRect pRect = playerBB.getRect(transform.pos);
        bool wasOnGround = state.onGround;
        state.onGround = false; // Reset

        for (auto& tile : m_entityManager.getEntities("tile")) {
            if (!tile->has<CTransform>() || !tile->has<CBoundingBox>() || !tile->has<CAnimation>())
                continue;

            auto& tileTransform = tile->get<CTransform>();
            auto& tileBB        = tile->get<CBoundingBox>();
            auto& tileAnim      = tile->get<CAnimation>().animation;
            sf::FloatRect tRect = tileBB.getRect(tileTransform.pos);

            if (!pRect.intersects(tRect))
                continue;

            float overlapX = std::min(pRect.left + pRect.width, tRect.left + tRect.width) -
                             std::max(pRect.left, tRect.left);
            float overlapY = std::min(pRect.top + pRect.height, tRect.top + tRect.height) -
                             std::max(pRect.top, tRect.top);

            if (overlapX < overlapY) {
                // Collisione orizzontale
                if (transform.pos.x < tileTransform.pos.x)
                    transform.pos.x -= overlapX;
                else
                    transform.pos.x += overlapX;
                velocity.x = 0.f;
            } else {
                // Collisione verticale
                if (transform.pos.y < tileTransform.pos.y) {
                    // Il giocatore è sopra il tile: atterraggio
                    transform.pos.y -= overlapY;
                    velocity.y = 0.f;
                    state.onGround = true;
                } else {
                    // Il giocatore colpisce il tile dal basso (stiamo salendo)
                    if (velocity.y < 0) {
                        transform.pos.y += overlapY;
                        velocity.y = 0.f;
                        std::string animName = tileAnim.getName();

                        // Solo Box1 e Box2 vengono rotti dal salto dal basso.
                        if (animName == "Box1" || animName == "Box2") {
                            m_spawner->createBlockFragments(tileTransform.pos, animName);
                            m_spawner->spawnItem(tileTransform.pos, animName);
                            tile->destroy();
                            std::cout << "[DEBUG] " << animName << " broken from below!\n";
                        }
                        // Treasure/Question box: attivazione solo dal salto (non dalla spada)
                        else if (animName == "TreasureBoxAnim" || animName == "QuestionAnim") {
                            auto& tileState = tile->get<CState>();
                            if (tileState.state == "inactive") {
                                tileState.state = "activated";
                                if (m_game.assets().hasAnimation("TreasureBoxHit")) {
                                    tile->get<CAnimation>().animation =
                                        m_game.assets().getAnimation("TreasureBoxHit");
                                    tile->get<CAnimation>().repeat = false;
                                    std::cout << "[DEBUG] Treasure/Question box hit from below.\n";
                                }
                                m_spawner->spawnItem(tileTransform.pos, "TreasureBoxAnim");
                            }
                        }
                        // Se si tratta di un Brick, non deve essere rotto
                    }
                }
            }

            pRect = playerBB.getRect(transform.pos);
        }

        if (state.state != "attack") {
            if (state.onGround)
                state.state = (std::abs(velocity.x) > 1.f) ? "run" : "idle";
            else
                state.state = "air";
        }
        //if (state.onGround && !wasOnGround)
        //    std::cout << "[DEBUG] Player landed! Jump reset.\n";
    }
}

// ----------------------------------
// 👹 ENEMY - TILE COLLISIONS (invariata)
void CollisionSystem::handleEnemyTileCollisions() {
    for (auto& enemy : m_entityManager.getEntities("enemy")) {
        auto& transform = enemy->get<CTransform>();
        auto& enemyBB   = enemy->get<CBoundingBox>();
        sf::FloatRect eRect = enemyBB.getRect(transform.pos);

        bool onGround = false;
        float minOverlapY = std::numeric_limits<float>::max();

        for (auto& tile : m_entityManager.getEntities("tile")) {
            auto& tileTransform = tile->get<CTransform>();
            auto& tileBB        = tile->get<CBoundingBox>();
            sf::FloatRect tRect = tileBB.getRect(tileTransform.pos);

            if (!eRect.intersects(tRect))
                continue;

            float overlapX = std::min(eRect.left + eRect.width, tRect.left + tRect.width) -
                             std::max(eRect.left, tRect.left);
            float overlapY = std::min(eRect.top + eRect.height, tRect.top + tRect.height) -
                             std::max(eRect.top, tRect.top);

            if (overlapX < overlapY) {
                transform.pos.x += (transform.pos.x < tileTransform.pos.x) ? -overlapX : overlapX;
                transform.velocity.x = 0.f;
            } else {
                if (transform.velocity.y > 0) {
                    if ((eRect.top + eRect.height) > tRect.top && overlapY < minOverlapY) {
                        minOverlapY = overlapY;
                        transform.pos.y -= overlapY;
                        transform.velocity.y = 0.f;
                        onGround = true;
                    }
                } else if (transform.velocity.y < 0) {
                    if (eRect.top < (tRect.top + tRect.height) && overlapY < minOverlapY) {
                        minOverlapY = overlapY;
                        transform.pos.y += overlapY;
                        transform.velocity.y = 0.f;
                    }
                }
            }
        }

        //if (onGround)
        //   std::cout << "[DEBUG] Enemy landed at: (" << transform.pos.x << ", " << transform.pos.y << ")\n";
    }
}

// ----------------------------------
// ⚔️ PLAYER - ENEMY COLLISIONS (invariata)
void CollisionSystem::handlePlayerEnemyCollisions() {
    for (auto& enemy : m_entityManager.getEntities("enemy")) {
        auto& enemyTrans = enemy->get<CTransform>();
        auto& enemyBB    = enemy->get<CBoundingBox>();
        sf::FloatRect enemyRect = enemyBB.getRect(enemyTrans.pos);

        for (auto& player : m_entityManager.getEntities("player")) {
            auto& pTrans = player->get<CTransform>();
            auto& pBB    = player->get<CBoundingBox>();
            sf::FloatRect playerRect = pBB.getRect(pTrans.pos);

            if (enemyRect.intersects(playerRect)) {
                auto& playerState = player->get<CState>();

                // Se il giocatore è già in knockback o il timer è attivo, salta l'applicazione del danno
                if (playerState.state == "knockback" || playerState.knockbackTimer > 0.f)
                    continue;

                player->get<CHealth>().takeDamage(1);
                Vec2<float> hitDirection = { (enemyTrans.pos.x < pTrans.pos.x) ? 1.f : -1.f, 0.f };
                Physics::Forces::ApplyKnockback(player, hitDirection, 1400.0f);
                playerState.state = "knockback";
                std::cout << "[DEBUG] Player hit by enemy! Knocked back hard.\n";
            }
        }
    }
}

// ----------------------------------
// 🗡️ SWORD - ENEMY & TILE COLLISIONS
// ----------------------------------
void CollisionSystem::handleSwordCollisions() {
    for (auto& sword : m_entityManager.getEntities("sword")) {
        if (!sword->has<CTransform>() || !sword->has<CBoundingBox>())
            continue;

        auto& swTrans = sword->get<CTransform>();
        auto& swBB    = sword->get<CBoundingBox>();
        sf::FloatRect swordRect = swBB.getRect(swTrans.pos);

        // 1) SWORD <-> TILE: Qui solo Box1 e Box2 devono essere rotti dalla spada.
        for (auto& tile : m_entityManager.getEntities("tile")) {
            if (!tile->has<CTransform>() || !tile->has<CBoundingBox>() || !tile->has<CAnimation>())
                continue;

            auto& tileTransform = tile->get<CTransform>();
            auto& tileBB        = tile->get<CBoundingBox>();
            auto& tileAnim      = tile->get<CAnimation>().animation;
            sf::FloatRect tileRect = tileBB.getRect(tileTransform.pos);

            if (swordRect.intersects(tileRect)) {
                std::string animName = tileAnim.getName();
                if (animName == "Box1" || animName == "Box2") {
                    m_spawner->createBlockFragments(tileTransform.pos, animName);
                    m_spawner->spawnItem(tileTransform.pos, animName);
                    tile->destroy();
                    std::cout << "[DEBUG] " << animName << " broken by sword!\n";
                }
                // Non attivare i Treasure/Question box dalla spada.
            }
        }

        // 2) SWORD <-> ENEMY
        for (auto& enemy : m_entityManager.getEntities("enemy")) {
            if (!enemy->has<CTransform>() || !enemy->has<CBoundingBox>() || !enemy->has<CHealth>())
                continue;

            auto& enemyTrans = enemy->get<CTransform>();
            auto& enemyBB    = enemy->get<CBoundingBox>();
            sf::FloatRect enemyRect = enemyBB.getRect(enemyTrans.pos);

            if (swordRect.intersects(enemyRect)) {
                auto& health = enemy->get<CHealth>();
                if (health.invulnerabilityTimer <= 0.f) {
                    std::cout << "[DEBUG] Enemy took a hit from the sword!\n";
                    Vec2<float> hitDirection = { (swTrans.pos.x < enemyTrans.pos.x) ? 1.f : -1.f, 0.f };
                    // Applica knockback e riduci la salute
                    Physics::Forces::ApplyKnockback(enemy, hitDirection, 300.0f);
                    enemy->get<CHealth>().takeDamage(3);
                    health.invulnerabilityTimer = 0.5f;
                    // Se l'enemy è morto dopo il danno, distruggilo
                    if (!health.isAlive()) {
                        std::cout << "[DEBUG] Enemy destroyed: ID = " << enemy->id() << "\n";
                        enemy->destroy();
                    }
                }
            }
        }
    }
}

// ----------------------------------
// PLAYER - COLLECTIBLE COLLISIONS
// ----------------------------------
void CollisionSystem::handlePlayerCollectibleCollisions() {
    // Per ogni giocatore, controlla le collisioni con i collectible
    for (auto& player : m_entityManager.getEntities("player")) {
        if (!player->has<CTransform>() || !player->has<CBoundingBox>())
            continue;
        auto& pTrans = player->get<CTransform>();
        auto& pBB    = player->get<CBoundingBox>();
        sf::FloatRect pRect = pBB.getRect(pTrans.pos);

        // Cicla su tutti i collectible
        for (auto& item : m_entityManager.getEntities("collectable")) {
            if (!item->has<CTransform>() || !item->has<CBoundingBox>() || !item->has<CState>())
                continue;
            auto& iTrans = item->get<CTransform>();
            auto& iBB    = item->get<CBoundingBox>();
            sf::FloatRect iRect = iBB.getRect(iTrans.pos);

            if (pRect.intersects(iRect)) {
                // Il tipo di oggetto è memorizzato nel CState (come nel metodo spawnItem)
                std::string itemType = item->get<CState>().state;
                if (itemType == "GrapeSmall") {
                    // Aggiungi 5 punti (oppure incrementa il punteggio, come preferisci)
                    std::cout << "[DEBUG] Collected small grape (+5 points)!\n";
                    // TODO: Incrementa il punteggio del giocatore
                } else if (itemType == "GrapeBig") {
                    // Aumenta la salute del giocatore di 10
                    if (player->has<CHealth>()) {
                        player->get<CHealth>().heal(10);
                        std::cout << "[DEBUG] Collected big grape (+10 health)!\n";
                    }
                }
                // Distruggi il collectible dopo la raccolta.
                item->destroy();
            }
        }
    }
}
