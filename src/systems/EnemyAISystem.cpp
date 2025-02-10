#include "EnemyAISystem.h"
#include "SpriteUtils.h" // Per flipSpriteLeft e flipSpriteRight
#include <cmath>
#include <algorithm>

EnemyAISystem::EnemyAISystem(GameEngine& game, EntityManager& entityManager)
    : m_game(game), m_entityManager(entityManager)
{
}

void EnemyAISystem::update(float deltaTime) {
    // Ottieni tutti i nemici
    auto enemies = m_entityManager.getEntities("enemy");
    for (auto& enemy : enemies) {
        // Assicurati che l'enemy abbia i componenti necessari per il movimento e il knockback
        if (!enemy->has<CTransform>() || !enemy->has<CState>())
            continue;

        auto& enemyTrans = enemy->get<CTransform>();
        auto& state = enemy->get<CState>();
        auto& enemyAI = enemy->get<CEnemyAI>();

        // Se l'enemy è in knockback, applica un decadimento alla velocità e aggiorna la posizione
        if (state.state == "knockback" && state.knockbackTimer > 0.f) {
            enemyTrans.velocity.x *= 0.95f; // Decadimento graduale del knockback
            enemyTrans.pos.x += enemyTrans.velocity.x * deltaTime;
            enemyTrans.pos.y += enemyTrans.velocity.y * deltaTime; // Se vuoi mantenere eventuali influenze verticali (opzionale)
            continue; // Salta la logica di pattugliamento
        }

        // Gestione della gravità per l'enemy
        bool isOnGround = false;
        sf::FloatRect enemyRect = enemy->get<CBoundingBox>().getRect(enemyTrans.pos);
        for (auto& tile : m_entityManager.getEntities("tile")) {
            if (!tile->has<CTransform>() || !tile->has<CBoundingBox>())
                continue;
            auto& tileTrans = tile->get<CTransform>();
            auto& tileBB = tile->get<CBoundingBox>();
            sf::FloatRect tileRect = tileBB.getRect(tileTrans.pos);
            if (enemyRect.intersects(tileRect)) {
                isOnGround = true;
                break;
            }
        }
        if (!isOnGround) {
            enemyTrans.velocity.y += enemy->get<CGravity>().gravity * deltaTime;
            enemyTrans.velocity.y = std::clamp(enemyTrans.velocity.y, -MAX_FALL_SPEED, MAX_FALL_SPEED);
        } else {
            enemyTrans.velocity.y = 0.f;
        }

        // Se lo stato è Chase o Attack, ignora il movimento di pattugliamento
        if (enemyAI.enemyState == EnemyState::Chase || enemyAI.enemyState == EnemyState::Attack) {
            // (Eventuali logiche di inseguimento o attacco sono gestite altrove)
            continue;
        }

        // Movimento di pattuglia (se non in knockback)
        if (!enemyAI.patrolPoints.empty()) {
            Vec2<float> patrolTarget = enemyAI.patrolPoints[enemyAI.currentPatrolIndex];
            float dx = patrolTarget.x - enemyTrans.pos.x;
            float dy = patrolTarget.y - enemyTrans.pos.y;
            float distance = std::sqrt(dx * dx + dy * dy);
            if (distance < 10.f) {
                enemyAI.currentPatrolIndex = (enemyAI.currentPatrolIndex + 1) % enemyAI.patrolPoints.size();
            }
            float patrolSpeed = 100.f * enemyAI.speedMultiplier;
            enemyTrans.velocity.x = (dx > 0) ? patrolSpeed : -patrolSpeed;
            enemyAI.facingDirection = (dx > 0) ? 1.f : -1.f;
        }

        // Aggiorna la posizione del nemico (movimento orizzontale e verticale)
        enemyTrans.pos.x += enemyTrans.velocity.x * deltaTime;
        enemyTrans.pos.y += enemyTrans.velocity.y * deltaTime;

        // Gestione del flip della sprite in base alla direzione di movimento
        if (enemy->has<CAnimation>()) {
            auto& anim = enemy->get<CAnimation>();
            if (enemyTrans.velocity.x < 0)
                flipSpriteLeft(anim.animation.getMutableSprite());
            else if (enemyTrans.velocity.x > 0)
                flipSpriteRight(anim.animation.getMutableSprite());
        }
    }
}
