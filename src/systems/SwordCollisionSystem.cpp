#include "SwordCollisionSystem.h"
#include "Components.hpp"
#include "Physics.hpp"
#include "SpriteUtils.h"  // Per flipSpriteLeft / flipSpriteRight
#include <iostream>
#include <SFML/Graphics.hpp>

SwordCollisionSystem::SwordCollisionSystem(EntityManager& entityManager)
    : m_entityManager(entityManager) {}

void SwordCollisionSystem::updateSwordCollisions() {
    // Per ogni spada del giocatore, controlla le collisioni con i nemici.
    for (auto& sword : m_entityManager.getEntities("sword")) {
        if (!sword->has<CTransform>() || !sword->has<CBoundingBox>())
            continue;

        auto& swTrans = sword->get<CTransform>();
        auto& swBB = sword->get<CBoundingBox>();
        sf::FloatRect swordRect = swBB.getRect(swTrans.pos);

        // Controlla la collisione con ogni nemico.
        for (auto& enemy : m_entityManager.getEntities("enemy")) {
            if (!enemy->has<CTransform>() || !enemy->has<CBoundingBox>() || !enemy->has<CState>())
                continue;

            auto& enemyTrans = enemy->get<CTransform>();
            auto& enemyBB = enemy->get<CBoundingBox>();
            auto& enemyState = enemy->get<CState>();
            sf::FloatRect enemyRect = enemyBB.getRect(enemyTrans.pos);

            if (!swordRect.intersects(enemyRect))
                continue;

            // Se il nemico è già in stato knockback o in invulnerabilità, ignora il colpo.
            if (enemyState.state == "knockback")
                continue;
            if (enemy->has<CHealth>()) {
                auto& health = enemy->get<CHealth>();
                if (health.invulnerabilityTimer > 0.f)
                    continue;
            }

            // Calcola la direzione dell'attacco: se la spada è a sinistra dell'enemy, l'attacco è verso destra, e viceversa.
            float attackDirection = (swTrans.pos.x < enemyTrans.pos.x) ? 1.f : -1.f;
            Vec2<float> hitDirection = { attackDirection, -0.5f };

            // Applica il knockback tramite il sistema fisico.
            Physics::Forces::ApplyKnockback(enemy, hitDirection, 1.0f);
            enemyState.state = "knockback";  // Imposta lo stato knockback

            // Se il nemico ha il componente salute, applica il danno e imposta il timer di invulnerabilità.
            if (enemy->has<CHealth>()) {
                auto& health = enemy->get<CHealth>();
                health.takeDamage(1);
                health.invulnerabilityTimer = 0.5f;  // 0.5 secondi di invulnerabilità
                std::cout << "[DEBUG] Enemy hit by sword! Health: " << health.currentHealth << "\n";
            }
        }
    }
}
