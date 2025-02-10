#include "MovementSystem.h"
#include <algorithm>
#include <cmath>
#include <SFML/Window/Keyboard.hpp>
#include "SpriteUtils.h"  // Per flipSpriteLeft / flipSpriteRight

MovementSystem::MovementSystem(GameEngine& game,
                               EntityManager& entityManager,
                               sf::View& cameraView,
                               float& lastDirection)
    : m_game(game),
      m_entityManager(entityManager),
      m_cameraView(cameraView),
      m_lastDirection(lastDirection)
{
}

void MovementSystem::update(float deltaTime) {
    // --- MOVIMENTO DEL GIOCATORE ---
    for (auto& entity : m_entityManager.getEntities("player")) {
        auto& transform = entity->get<CTransform>();
        auto& state     = entity->get<CState>();
        auto& canim     = entity->get<CAnimation>();

        // Applica la gravità
        transform.velocity.y += entity->get<CGravity>().gravity * deltaTime;
        transform.velocity.y = std::clamp(transform.velocity.y, -MAX_FALL_SPEED, MAX_FALL_SPEED);

        // Gestione dello stato "knockback"
        if (state.state == "knockback") {
            state.knockbackTimer -= deltaTime;
            if (state.knockbackTimer > 0.f) {
                transform.pos += transform.velocity * deltaTime;
            } else {
                state.state = state.onGround ? "idle" : "air";
                transform.velocity.x = 0.f;
            }
        } else {
            // Movimento normale
            transform.velocity.x = 0.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
                transform.velocity.x = -xSpeed;
                m_lastDirection = -1.f;
            } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
                transform.velocity.x = xSpeed;
                m_lastDirection = 1.f;
            }

            // Logica di salto
            if (state.isJumping && state.jumpTime < MaxJumpDuration) {
                transform.velocity.y -= JumpAcceleration * deltaTime;
                state.jumpTime += deltaTime;
                if (transform.velocity.y < MaxUpwardVelocity) {
                    transform.velocity.y = MaxUpwardVelocity;
                }
            }

            // Applica il movimento
            transform.pos += transform.velocity * deltaTime;
        }

        // Flip dello sprite del giocatore
        if (m_lastDirection < 0)
            flipSpriteLeft(canim.animation.getMutableSprite());
        else
            flipSpriteRight(canim.animation.getMutableSprite());
    }

    // --- MOVIMENTO DELLA CAMERA ---
    for (auto& entity : m_entityManager.getEntities("player")) {
        auto& transform = entity->get<CTransform>();

        float screenWidth  = m_game.window().getSize().x;
        float screenHeight = m_game.window().getSize().y;
        float thresholdX   = screenWidth * 0.25f;
        float thresholdY   = screenHeight * 0.15f;

        sf::Vector2f cameraCenter = m_cameraView.getCenter();

        if (transform.pos.x < cameraCenter.x - thresholdX)
            cameraCenter.x = transform.pos.x + thresholdX;
        else if (transform.pos.x > cameraCenter.x + thresholdX)
            cameraCenter.x = transform.pos.x - thresholdX;

        if (transform.pos.y < cameraCenter.y - thresholdY)
            cameraCenter.y = transform.pos.y + thresholdY;
        else if (transform.pos.y > cameraCenter.y + thresholdY)
            cameraCenter.y = transform.pos.y - thresholdY;

        cameraCenter.y += CAMERA_Y_OFFSET;

        static float smoothCameraX = cameraCenter.x;
        static float smoothCameraY = cameraCenter.y;
        smoothCameraX = smoothCameraX * (1 - CAMERA_SMOOTHING_FACTOR) + cameraCenter.x * CAMERA_SMOOTHING_FACTOR;
        smoothCameraY = smoothCameraY * (1 - CAMERA_SMOOTHING_FACTOR) + cameraCenter.y * CAMERA_SMOOTHING_FACTOR;
        m_cameraView.setCenter(std::round(smoothCameraX), std::round(smoothCameraY));
    }

    // --- LOGICA DI SEGUIMENTO DELLA SPADA ---
    auto players = m_entityManager.getEntities("player");
    if (!players.empty()) {
        auto& player = players[0];
        auto& pTrans = player->get<CTransform>();

        float dir = m_lastDirection;
        float offsetX = (dir < 0) ? -80.f : 30.f;
        float offsetY = 10.f;

        for (auto& sword : m_entityManager.getEntities("sword")) {
            if (!sword->has<CTransform>()) continue;
            auto& swTrans = sword->get<CTransform>();

            swTrans.pos.x = pTrans.pos.x + offsetX;
            swTrans.pos.y = pTrans.pos.y + offsetY;

            if (sword->has<CAnimation>()) {
                auto& swordAnim = sword->get<CAnimation>();
                if (dir < 0)
                    flipSpriteLeft(swordAnim.animation.getMutableSprite());
                else
                    flipSpriteRight(swordAnim.animation.getMutableSprite());
            }
        }
    }

    // Aggiorna la vista della finestra
    m_game.window().setView(m_cameraView);
}
