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
        auto& enemyTrans = enemy->get<CTransform>();
        auto& enemyAI = enemy->get<CEnemyAI>();

        // Controlla se il nemico è a terra (usando il bounding box)
        bool isOnGround = false;
        sf::FloatRect enemyRect = enemy->get<CBoundingBox>().getRect(enemyTrans.pos);
        for (auto& tile : m_entityManager.getEntities("tile")) {
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
            enemyTrans.velocity.y = 0;  // Reset della gravità se il nemico tocca il terreno
        }

        // Se lo stato è Chase o Attack, ignora il movimento di pattugliamento
        if (enemyAI.enemyState == EnemyState::Chase || enemyAI.enemyState == EnemyState::Attack) {
            continue;
        }

        // Movimento di pattuglia
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

        // Aggiorna la posizione del nemico
        enemyTrans.pos.x += enemyTrans.velocity.x * deltaTime;
        enemyTrans.pos.y += enemyTrans.velocity.y * deltaTime;

        // Gestione del flip della sprite in base alla direzione di movimento
        if (enemy->has<CAnimation>()) {
            auto& anim = enemy->get<CAnimation>();
            // Se il nemico si muove a sinistra, capovolge la sprite
            if (enemyTrans.velocity.x < 0)
                flipSpriteLeft(anim.animation.getMutableSprite());
            // Se si muove a destra, ripristina l'orientamento originale
            else if (enemyTrans.velocity.x > 0)
                flipSpriteRight(anim.animation.getMutableSprite());
        }
    }
}
