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
    // Optionally, you can set m_currentZoom to something else here
    // m_currentZoom = 1.5f;
}

void MovementSystem::updateCamera()
{
    auto players = m_entityManager.getEntities("player");
    if (players.empty()) {
        return;
    }
    auto& transform = players[0]->get<CTransform>();

    // Offset for the camera
    sf::Vector2f offset(200.f, -100.f);

    // 'targetPos' is player pos + offset
    sf::Vector2f targetPos = transform.pos + offset;

    // Current camera center
    sf::Vector2f cameraCenter = m_cameraView.getCenter();

    // Trap zone dimensions
    float halfTrapZoneWidth  = 150.f;
    float halfTrapZoneHeight = 200.f;

    // Calculate how far 'targetPos' is from the camera center
    float dx = targetPos.x - cameraCenter.x;
    float dy = targetPos.y - cameraCenter.y;

    sf::Vector2f desiredCenter = cameraCenter;

    // Horizontal trap zone check
    if (dx < -halfTrapZoneWidth) {
        desiredCenter.x = targetPos.x + halfTrapZoneWidth;
    } 
    else if (dx > halfTrapZoneWidth) {
        desiredCenter.x = targetPos.x - halfTrapZoneWidth;
    }

    // Vertical trap zone check
    if (dy < -halfTrapZoneHeight) {
        desiredCenter.y = targetPos.y + halfTrapZoneHeight;
    } 
    else if (dy > halfTrapZoneHeight) {
        desiredCenter.y = targetPos.y - halfTrapZoneHeight;
    }

    // Set the camera center
    m_cameraView.setCenter(desiredCenter);
    // Example: One-time zoom to 1.1f
    if (m_currentZoom != 1.f) {
        float zoomFactor = 1.f / m_currentZoom;
        m_cameraView.zoom(zoomFactor);
        m_currentZoom = 1.f;

        // Debug prints:
        std::cout << "Current zoom factor: " << m_currentZoom << "\n";
        std::cout << "Zooming view by: " << zoomFactor << "\n";
        
        // Also helpful to see the final View size:
        auto size = m_cameraView.getSize();
        std::cout << "View size after zoom: " << size.x << " x " << size.y << "\n";
    }

    // Apply the final camera
    m_game.window().setView(m_cameraView);
}

void MovementSystem::update(float deltaTime)
{
    // -- Player movement, etc. --
    for (auto& entity : m_entityManager.getEntities("player")) {
        auto& transform = entity->get<CTransform>();
        auto& state     = entity->get<CState>();
        auto& canim     = entity->get<CAnimation>();

        // Gravity, jump logic, etc.
        float baseGravity = entity->get<CGravity>().gravity;
        transform.velocity.y += baseGravity * deltaTime;
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
        transform.velocity.y = std::min(transform.velocity.y, MAX_FALL_SPEED);

        if (state.state == "knockback") {
            state.knockbackTimer -= deltaTime;
            if (state.knockbackTimer > 0.f) {
                transform.pos += transform.velocity * deltaTime;
            } else {
                state.state = state.onGround ? "idle" : "air";
                transform.velocity.x = 0.f;
            }
        } else {
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

    // Update camera
    updateCamera();

    // Sword follows player
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
    auto enemySwords = m_entityManager.getEntities("EmperorSword");
    for (auto& eSword : enemySwords) {
        if (!eSword->has<CTransform>()) continue;
        auto& swTrans = eSword->get<CTransform>();

        // Se la spada ha velocity, la aggiorniamo
        swTrans.pos += swTrans.velocity * deltaTime;

        // Se vuoi gestire un flip dello sprite in base alla velocity:
        if (eSword->has<CAnimation>()) {
            auto& swordAnim = eSword->get<CAnimation>();
            if (swTrans.velocity.x < 0.f) {
                flipSpriteLeft(swordAnim.animation.getMutableSprite());
            } else {
                flipSpriteRight(swordAnim.animation.getMutableSprite());
            }
        }
    }

    m_game.window().setView(m_cameraView);
}
