#include "MovementSystem.h"
#include <SFML/Window/Keyboard.hpp>
#include "SpriteUtils.h"

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

// Add a member variable in your MovementSystem, for example:
float m_currentZoom = 1.0f; // starts unzoomed

void MovementSystem::updateCamera()
{
    auto players = m_entityManager.getEntities("player");
    if (players.empty()) {
        return;
    }
    auto& transform = players[0]->get<CTransform>();

    // 1) Define an offset (e.g., +200 to the right, -100 up).
    sf::Vector2f offset(200.f, -100.f);

    // 2) 'targetPos' is where we want the camera to focus,
    //    i.e., the player plus our offset.
    sf::Vector2f targetPos = transform.pos + offset;

    // 3) Current camera center
    sf::Vector2f cameraCenter = m_cameraView.getCenter();

    // 4) Trap zone dimensions
    float halfTrapZoneWidth  = 150.f;  // 300px total
    float halfTrapZoneHeight = 200.f;  // 400px total

    // 5) Check how far the offsetted position is from camera center
    float dx = targetPos.x - cameraCenter.x;
    float dy = targetPos.y - cameraCenter.y;

    sf::Vector2f desiredCenter = cameraCenter;

    // 6) Horizontal trap zone check
    if (dx < -halfTrapZoneWidth) {
        desiredCenter.x = targetPos.x + halfTrapZoneWidth;
    } 
    else if (dx > halfTrapZoneWidth) {
        desiredCenter.x = targetPos.x - halfTrapZoneWidth;
    }

    // 7) Vertical trap zone check
    if (dy < -halfTrapZoneHeight) {
        desiredCenter.y = targetPos.y + halfTrapZoneHeight;
    } 
    else if (dy > halfTrapZoneHeight) {
        desiredCenter.y = targetPos.y - halfTrapZoneHeight;
    }

    // 8) Now we set the camera center without adding offset again,
    //    because we've already accounted for it in 'targetPos'
    m_cameraView.setCenter(desiredCenter);
    m_game.window().setView(m_cameraView);
}
void MovementSystem::update(float deltaTime)
{
    // Player movement
    for (auto& entity : m_entityManager.getEntities("player")) {
        auto& transform = entity->get<CTransform>();
        auto& state     = entity->get<CState>();
        auto& canim     = entity->get<CAnimation>();

        // Apply gravity
        float baseGravity = entity->get<CGravity>().gravity;
        transform.velocity.y += baseGravity * deltaTime;

        // Jump logic
        if (state.isJumping && state.jumpTime < MAX_JUMP_HOLD_TIME) {
            transform.velocity.y -= JUMP_BOOST_ACCELERATION * deltaTime;
            state.jumpTime += deltaTime;
            if (transform.velocity.y < MAX_UPWARD_VELOCITY) {
                transform.velocity.y = MAX_UPWARD_VELOCITY;
            }
        } else {
            if (transform.velocity.y > 0) {
                transform.velocity.y += baseGravity * (GRAVITY_MULTIPLIER - 1.0f) * deltaTime;
            }
        }

        // Cap fall speed
        transform.velocity.y = std::min(transform.velocity.y, MAX_FALL_SPEED);

        // Knockback or normal movement
        if (state.state == "knockback") {
            state.knockbackTimer -= deltaTime;
            if (state.knockbackTimer > 0.f) {
                transform.pos += transform.velocity * deltaTime;
            } else {
                state.state = state.onGround ? "idle" : "air";
                transform.velocity.x = 0.f;
            }
        } else {
            // Left/right movement
            transform.velocity.x = 0.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
                transform.velocity.x = -X_SPEED;
                m_lastDirection = -1.f;
            } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
                transform.velocity.x = X_SPEED;
                m_lastDirection = 1.f;
            }
            transform.pos += transform.velocity * deltaTime;
        }

        // Flip sprite based on direction
        if (m_lastDirection < 0) {
            flipSpriteLeft(canim.animation.getMutableSprite());
        } else {
            flipSpriteRight(canim.animation.getMutableSprite());
        }
    }

    // Update camera with the trap zone logic
    updateCamera();

    // Sword follows the player
    auto players = m_entityManager.getEntities("player");
    if (!players.empty()) {
        auto& player = players[0];
        auto& pTrans = player->get<CTransform>();

        float dir     = m_lastDirection;
        float offsetX = (dir < 0) ? SWORD_OFFSET_X_LEFT : SWORD_OFFSET_X_RIGHT;
        float offsetY = SWORD_OFFSET_Y;

        for (auto& sword : m_entityManager.getEntities("sword")) {
            if (!sword->has<CTransform>()) continue;
            auto& swTrans = sword->get<CTransform>();
            swTrans.pos.x = pTrans.pos.x + offsetX;
            swTrans.pos.y = pTrans.pos.y + offsetY;

            if (sword->has<CAnimation>()) {
                auto& swordAnim = sword->get<CAnimation>();
                if (dir < 0) {
                    flipSpriteLeft(swordAnim.animation.getMutableSprite());
                } else {
                    flipSpriteRight(swordAnim.animation.getMutableSprite());
                }
            }
        }
    }

    // Finalize the camera on the window
    m_game.window().setView(m_cameraView);
}
