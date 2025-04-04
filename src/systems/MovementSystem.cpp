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
    float halfTrapZoneWidth  = 250.f;
    float halfTrapZoneHeight = 250.f;

    // Calculate how far 'targetPos' is from the camera center
    float dx = targetPos.x - cameraCenter.x;
    float dy = targetPos.y - cameraCenter.y;

    sf::Vector2f desiredCenter = cameraCenter;

    // Horizontal trap zone check
    if (dx < -halfTrapZoneWidth) {
        desiredCenter.x = targetPos.x + halfTrapZoneWidth;
    } else if (dx > halfTrapZoneWidth) {
        desiredCenter.x = targetPos.x - halfTrapZoneWidth;
    }

    // Vertical trap zone check
    if (dy < -halfTrapZoneHeight) {
        desiredCenter.y = targetPos.y + halfTrapZoneHeight;
    } else if (dy > halfTrapZoneHeight) {
        desiredCenter.y = targetPos.y - halfTrapZoneHeight;
    }

    // Set the camera center
    m_cameraView.setCenter(desiredCenter);

    // Check for Emperor boss with low health to trigger zoom out
    float targetZoomStrength = ZOOM_STRENGTH; // Default zoom level

    // Find Emperor boss and check health
    for (auto& enemy : m_entityManager.getEntities("enemy")) {
        if (enemy->has<CEnemyAI>() && enemy->has<CHealth>()) {
            auto& enemyAI = enemy->get<CEnemyAI>();
            auto& health = enemy->get<CHealth>();
            
            // Only apply special zoom for Future Emperor
            if (enemyAI.enemyType == EnemyType::Emperor && m_game.worldType == "Future") {
                float healthPercentage = static_cast<float>(health.currentHealth) / static_cast<float>(health.maxHealth);
                
                // Check if the Emperor is in the final teleported position (Phase 4)
                if (enemy->has<CTransform>()) {
                    auto& transform = enemy->get<CTransform>();
                    
                    // If Emperor is at the final position (around x=3777)
                    if (std::abs(transform.pos.x - 3777) < 50) {
                        // Set zoom for final battle phase after teleport
                        targetZoomStrength = 1.5f;
                        break;
                    }
                }
                
                // Otherwise use health-based zoom levels
                if (healthPercentage < 0.3f) {
                    // Zoom out more for final attack phase
                    targetZoomStrength = 3.0f;
                } else if (healthPercentage <= 0.5f) {
                    // Medium zoom out for phase 2
                    targetZoomStrength = 1.5f;
                }
                
                break; // No need to check other enemies once we found the Emperor
            }
        }
    }

    // Apply zoom if it changed
    if (m_currentZoom != targetZoomStrength) {
        float zoomFactor = targetZoomStrength / m_currentZoom;
        m_cameraView.zoom(zoomFactor);
        m_currentZoom = targetZoomStrength;

        // Debug prints:
        // std::cout << "Current zoom factor: " << m_currentZoom << "\n";
        // std::cout << "Zooming view by: " << zoomFactor << "\n";
        
        //auto size = m_cameraView.getSize();
        // std::cout << "View size after zoom: " << size.x << " x " << size.y << "\n";
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

        // Handle horizontal movement based on state
        if (state.state == "defense") {
            // If player is on ground, cancel horizontal movement
            if (state.onGround)
                transform.velocity.x = 0.f;
            // Update position with full velocity
            transform.pos += transform.velocity * deltaTime;
        }
        else {
            // Update animation state based on current velocity
            if (transform.velocity.x < 0) {
                m_lastDirection = -1.f;
                if (state.state != "air" && state.state != "attack") {
                    state.state = "run";
                }
            }
            else if (transform.velocity.x > 0) {
                m_lastDirection = 1.f;
                if (state.state != "air" && state.state != "attack") {
                    state.state = "run";
                }
            }
            
            // Update position
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
    
        // Only swords with active stop timers (timer > 0) should count down
        if (eSword->has<CStopAfterTime>()) {
            auto& stopTimer = eSword->get<CStopAfterTime>();
    
            if (stopTimer.timer > 0.f) {
                stopTimer.timer -= deltaTime;
                if (stopTimer.timer <= 0.f) {
                    swTrans.velocity = Vec2(0.f, 0.f);
                    stopTimer.timer = 0.f; // Mark as "stopped"
                }
            }
        }
    
        swTrans.pos += swTrans.velocity * deltaTime;

    }

    for (auto& blackHole : m_entityManager.getEntities("emperorBlackHole")) {
        if (!blackHole->has<CTransform>()) continue;
        auto& swTrans = blackHole->get<CTransform>();
    
        // Only swords with active stop timers (timer > 0) should count down
        if (blackHole->has<CStopAfterTime>()) {
            auto& stopTimer = blackHole->get<CStopAfterTime>();
    
            if (stopTimer.timer > 0.f) {
                stopTimer.timer -= deltaTime;
                if (stopTimer.timer <= 0.f) {
                    swTrans.velocity = Vec2(0.f, 0.f);
                    stopTimer.timer = 0.f; // Mark as "stopped"
                }
            }
        }
    
        swTrans.pos += swTrans.velocity * deltaTime;

    }
    
    auto armorSwords = m_entityManager.getEntities("EmperorSwordArmor");
    for (auto& sword : armorSwords) {
        auto& trans = sword->get<CTransform>();

        if (sword->has<CStopAfterTime>()) {
            auto& stopTimer = sword->get<CStopAfterTime>();
            stopTimer.timer -= deltaTime;

            if (stopTimer.timer <= 0.f) {
                trans.velocity = Vec2<float>(0.f, 0.f);
            }
        }
        trans.pos += trans.velocity * deltaTime;
    }

    for (auto& bullet : m_entityManager.getEntities("enemyBullet")) {
        if (!bullet->has<CTransform>()) continue;
        auto& trans = bullet->get<CTransform>();
        trans.pos += trans.velocity * deltaTime;
    }

    for (auto& bullet : m_entityManager.getEntities("playerBullet")) {
        if (!bullet->has<CTransform>()) continue;
        auto& trans = bullet->get<CTransform>();
        trans.pos += trans.velocity * deltaTime;
    }

    m_game.window().setView(m_cameraView);
}
