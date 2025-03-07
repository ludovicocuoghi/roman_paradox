#include "PlayRenderer.h"
#include "Animation.hpp"
#include <iostream>
#include <cmath>
#include "SpriteUtils.h"

PlayRenderer::PlayRenderer(GameEngine& game,
                           EntityManager& entityManager,
                           sf::Sprite& backgroundSprite,
                           sf::Texture& backgroundTexture,
                           sf::View& cameraView,
                           int& score)
    : m_game(game),
      m_entityManager(entityManager),
      m_backgroundSprite(backgroundSprite),
      m_backgroundTexture(backgroundTexture),
      m_cameraView(cameraView),
      m_showGrid(false),
      m_showBoundingBoxes(false),
      m_score(score),         // assign reference
      m_timeofday("Day")
{
}

void PlayRenderer::setShowGrid(bool show) {
    m_showGrid = show;
}

void PlayRenderer::setShowBoundingBoxes(bool show) {
    m_showBoundingBoxes = show;
}

void PlayRenderer::setScore(int score) {
    m_score = score;
}

void PlayRenderer::setTimeOfDay(const std::string& tod) {
    m_timeofday = tod;
}

void PlayRenderer::render() {
    m_game.window().clear();

    // Draw background (scaled to window)
    sf::Vector2u windowSize = m_game.window().getSize();
    sf::Vector2u textureSize = m_backgroundTexture.getSize();
    float scaleX = static_cast<float>(windowSize.x) / static_cast<float>(textureSize.x);
    float scaleY = static_cast<float>(windowSize.y) / static_cast<float>(textureSize.y);
    m_backgroundSprite.setScale(scaleX, scaleY);

    sf::View defaultView = m_game.window().getDefaultView();
    m_game.window().setView(defaultView);
    m_game.window().draw(m_backgroundSprite);

    m_game.window().setView(m_cameraView);
    sf::RectangleShape debugBox;

    if (m_showGrid) {
        drawGrid();
    }

    // Render decorations
    for (auto& dec : m_entityManager.getEntities("decoration")) {
        auto& transform = dec->get<CTransform>();
        if (dec->has<CAnimation>()) {
            auto& anim = dec->get<CAnimation>();
            sf::Sprite sprite = anim.animation.getSprite();
            sprite.setPosition(transform.pos.x, transform.pos.y);
            sprite.setOrigin(anim.animation.getSize().x / 2.f,
                             anim.animation.getSize().y / 2.f);
            m_game.window().draw(sprite);
        }
    }

    // Render tiles
    for (auto& tile : m_entityManager.getEntities("tile")) {
        auto& transform = tile->get<CTransform>();
        if (tile->has<CAnimation>()) {
            auto& animation = tile->get<CAnimation>();
            sf::Sprite sprite = animation.animation.getSprite();
            sprite.setPosition(transform.pos.x, transform.pos.y);
            sprite.setOrigin(animation.animation.getSize().x / 2.f,
                             animation.animation.getSize().y / 2.f);
            m_game.window().draw(sprite);
        }
        if (m_showBoundingBoxes && tile->has<CBoundingBox>()) {
            auto& bbox = tile->get<CBoundingBox>();
            debugBox.setSize(sf::Vector2f(bbox.size.x, bbox.size.y));
            debugBox.setOrigin(bbox.halfSize.x, bbox.halfSize.y);
            debugBox.setPosition(transform.pos.x, transform.pos.y);
            debugBox.setFillColor(sf::Color::Transparent);
            debugBox.setOutlineColor(sf::Color::Red);
            debugBox.setOutlineThickness(2.f);
            m_game.window().draw(debugBox);
        }
    }

    // Render fragments
    for (auto& fragment : m_entityManager.getEntities("fragment")) {
        auto& transform = fragment->get<CTransform>();
        if (fragment->has<CAnimation>()) {
            auto& anim = fragment->get<CAnimation>();
            sf::Sprite sprite = anim.animation.getSprite();
            sprite.setPosition(transform.pos.x, transform.pos.y);
            sprite.setOrigin(anim.animation.getSize().x / 2.f,
                             anim.animation.getSize().y / 2.f);
            sprite.setScale(0.5f, 0.5f);
            m_game.window().draw(sprite);
        }
    }

    // Render items (collectables)
    for (auto& item : m_entityManager.getEntities()) {
        if (item->tag() != "collectable") continue;
        auto& transform = item->get<CTransform>();
        if (item->has<CAnimation>()) {
            auto& anim = item->get<CAnimation>();
            sf::Sprite sprite = anim.animation.getSprite();
            sprite.setPosition(transform.pos.x, transform.pos.y);
            sprite.setOrigin(anim.animation.getSize().x / 2.f,
                             anim.animation.getSize().y / 2.f);
            m_game.window().draw(sprite);
        }
        if (m_showBoundingBoxes && item->has<CBoundingBox>()) {
            auto& bbox = item->get<CBoundingBox>();
            debugBox.setSize(sf::Vector2f(bbox.size.x, bbox.size.y));
            debugBox.setOrigin(bbox.halfSize.x, bbox.halfSize.y);
            debugBox.setPosition(transform.pos.x, transform.pos.y);
            debugBox.setFillColor(sf::Color::Transparent);
            debugBox.setOutlineColor(sf::Color::Red);
            debugBox.setOutlineThickness(2.f);
            m_game.window().draw(debugBox);
        }
    }

    // Render player
    for (auto& player : m_entityManager.getEntities("player")) {
        auto& transform = player->get<CTransform>();
        if (player->has<CAnimation>()) {
            auto& animation = player->get<CAnimation>();
            sf::Sprite sprite = animation.animation.getSprite();
            sprite.setPosition(transform.pos.x, transform.pos.y);
            sprite.setOrigin(animation.animation.getSize().x / 2.f,
                             animation.animation.getSize().y / 2.f);
            m_game.window().draw(sprite);
        }
        if (m_showBoundingBoxes && player->has<CBoundingBox>()) {
            auto& bbox = player->get<CBoundingBox>();
            debugBox.setSize(sf::Vector2f(bbox.size.x, bbox.size.y));
            debugBox.setOrigin(bbox.halfSize.x, bbox.halfSize.y);
            debugBox.setPosition(transform.pos.x, transform.pos.y);
            debugBox.setOutlineColor(sf::Color::Green);
            debugBox.setOutlineThickness(2.f);
            m_game.window().draw(debugBox);
        }
    }
// Render enemies
for (auto& enemy : m_entityManager.getEntities("enemy")) {
    if (!enemy->has<CTransform>() || !enemy->has<CHealth>()) continue;

    auto& eTrans  = enemy->get<CTransform>();
    auto& eHealth = enemy->get<CHealth>();

    // Check if this is an Emperor
    bool isEmperor = false;
    if (enemy->has<CEnemyAI>()) {
        auto& enemyAI = enemy->get<CEnemyAI>();
        isEmperor = enemyAI.enemyType == EnemyType::Emperor;
    }

    if (isEmperor) {
        // Emperor health bar settings
        float barWidth = 120.f;
        float barHeight = 10.f;
        float offsetY = 140.f;
        float spacing = 15.f; // Space between bars
        
        // Calculate health ratio
        float currentHealth = eHealth.currentHealth;
        float maxHealth = eHealth.maxHealth;
        float healthRatio = (maxHealth > 0.f) ? (currentHealth / maxHealth) : 0.f;
        healthRatio = std::max(0.f, std::min(1.f, healthRatio));
        
        // Define phase thresholds based on the attack logic
        const float phase1Threshold = 0.7f;  // > 70% HP (Radial Attack)
        const float phase2Threshold = 0.5f;  // 70-50% HP (Enhanced Radial Attack)
        const float phase3Threshold = 0.1f;  // 50-10% HP (Final Burst Phase)
        // Below 10% would be Final Attack phase (not shown in the code snippet)
        
        // Bar colors for different phases
        sf::Color barColors[3] = {
            sf::Color(220, 50, 50),    // Bottom bar - red (Final Burst Phase)
            sf::Color(220, 220, 50),   // Middle bar - yellow (Enhanced Radial)
            sf::Color(50, 220, 50)     // Top bar - green (Normal Radial)
        };
        
        // Base position for the bars
        float baseX = eTrans.pos.x - (barWidth / 2.f);
        float baseY = eTrans.pos.y - offsetY;
        
        // Draw all three phase bars
        for (int i = 0; i < 3; i++) {
            // Calculate position for this bar (stacked from top to bottom)
            float barX = baseX;
            float barY = baseY + (2-i) * (barHeight + spacing);
            
            // Background
            sf::RectangleShape healthBg(sf::Vector2f(barWidth, barHeight));
            healthBg.setFillColor(sf::Color(50, 50, 50));
            healthBg.setPosition(barX, barY);
            m_game.window().draw(healthBg);
            
            // Calculate fill amount for each bar based on current health
            float fillRatio = 0.0f;
            
            if (i == 2) { // Top bar (Phase 1: 100-70%)
                if (healthRatio > phase1Threshold) {
                    fillRatio = (healthRatio - phase1Threshold) / (1.0f - phase1Threshold);
                }
            }
            else if (i == 1) { // Middle bar (Phase 2: 70-50%)
                if (healthRatio > phase2Threshold && healthRatio <= phase1Threshold) {
                    fillRatio = (healthRatio - phase2Threshold) / (phase1Threshold - phase2Threshold);
                }
                else if (healthRatio > phase1Threshold) {
                    fillRatio = 1.0f;
                }
            }
            else if (i == 0) { // Bottom bar (Phase 3: 50-10%)
                if (healthRatio > phase3Threshold && healthRatio <= phase2Threshold) {
                    fillRatio = (healthRatio - phase3Threshold) / (phase2Threshold - phase3Threshold);
                }
                else if (healthRatio > phase2Threshold) {
                    fillRatio = 1.0f;
                }
            }
            
            // Draw filled portion of bar
            sf::RectangleShape healthRect(sf::Vector2f(barWidth * fillRatio, barHeight));
            healthRect.setFillColor(barColors[i]);
            healthRect.setPosition(barX, barY);
            m_game.window().draw(healthRect);
            
            // Border
            sf::RectangleShape borderRect(sf::Vector2f(barWidth, barHeight));
            borderRect.setFillColor(sf::Color::Transparent);
            borderRect.setOutlineColor(sf::Color::White);
            borderRect.setOutlineThickness(1.f);
            borderRect.setPosition(barX, barY);
            m_game.window().draw(borderRect);
            
            // Draw division markers at 10% intervals
            int divisions = (i == 0) ? 4 : (i == 1) ? 2 : 3; // Number of divisions based on phase width
            for (int j = 1; j < divisions; j++) {
                float markerX = barX + (barWidth * j / divisions);
                
                sf::Vertex line[] = {
                    sf::Vertex(sf::Vector2f(markerX, barY), sf::Color::White),
                    sf::Vertex(sf::Vector2f(markerX, barY + barHeight), sf::Color::White)
                };
                
                m_game.window().draw(line, 2, sf::Lines);
            }
            
        }
        
        // Add special indicator for "Final Attack" phase (below 10%)
        if (healthRatio <= phase3Threshold) {
            sf::RectangleShape finalPhaseBar(sf::Vector2f(barWidth, barHeight));
            finalPhaseBar.setFillColor(sf::Color(150, 0, 0));
            finalPhaseBar.setPosition(baseX, baseY + 3 * (barHeight + spacing));
            m_game.window().draw(finalPhaseBar);
            
            // Add "FINAL ATTACK" text
            if (m_game.assets().hasFont("Menu")) {
                sf::Text finalText;
                finalText.setFont(m_game.assets().getFont("Menu"));
                finalText.setString("FINAL ATTACK (<10%)");
                finalText.setCharacterSize(12);
                finalText.setFillColor(sf::Color::Red);
                
                // Position label to the left of the bar
                sf::FloatRect textBounds = finalText.getLocalBounds();
                finalText.setPosition(
                    baseX - textBounds.width - 5.f,
                    baseY + 3 * (barHeight + spacing) + (barHeight / 2.f) - (textBounds.height / 2.f)
                );
                
                m_game.window().draw(finalText);
            }
        }
        
        // Add overall health percentage text
        if (m_game.assets().hasFont("Menu")) {
            sf::Text healthText;
            healthText.setFont(m_game.assets().getFont("Menu"));
            healthText.setString(std::to_string(static_cast<int>(healthRatio * 100)) + "%");
            healthText.setCharacterSize(14);
            healthText.setFillColor(sf::Color::White);
            
            // Center the text above all bars
            sf::FloatRect textBounds = healthText.getLocalBounds();
            healthText.setPosition(
                baseX + (barWidth / 2.f) - (textBounds.width / 2.f),
                baseY - 25.f
            );
            
            m_game.window().draw(healthText);
        }
    } else {
        // Regular enemy health bar code (unchanged)
        float barWidth = 40.f;
        float barHeight = 5.f;
        float offsetY = 60.f;
        
        // Calculate health ratio
        float currentHealth = eHealth.currentHealth;
        float maxHealth = eHealth.maxHealth;
        float healthRatio = (maxHealth > 0.f) ? (currentHealth / maxHealth) : 0.f;
        healthRatio = std::max(0.f, std::min(1.f, healthRatio));
        
        // Base position for the bar
        float barX = eTrans.pos.x - (barWidth / 2.f);
        float barY = eTrans.pos.y - offsetY;
        
        // Background
        sf::RectangleShape healthBg(sf::Vector2f(barWidth, barHeight));
        healthBg.setFillColor(sf::Color(50, 50, 50));
        healthBg.setPosition(barX, barY);
        
        // Health bar color based on health percentage
        sf::Color healthColor;
        if (healthRatio > 0.7f) {
            healthColor = sf::Color::Green;
        } else if (healthRatio > 0.3f) {
            healthColor = sf::Color::Yellow;
        } else {
            healthColor = sf::Color::Red;
        }
        
        // Health bar
        sf::RectangleShape healthRect(sf::Vector2f(barWidth * healthRatio, barHeight));
        healthRect.setFillColor(healthColor);
        healthRect.setPosition(barX, barY);
        
        // Draw background and health bar
        m_game.window().draw(healthBg);
        m_game.window().draw(healthRect);
    }

        // Render enemy sprite
        // Render enemy sprite
        if (enemy->has<CAnimation>()) {
            auto& animation = enemy->get<CAnimation>();
            auto& enemyAI   = enemy->get<CEnemyAI>();
            //std::cout << "[DEBUG] Enemy " << enemy->id() 
            //<< " State=" << static_cast<int>(enemyAI.enemyState) << std::endl;

            // If Emperor is defeated, forcibly set "AncientStandEmperorDefeated" animation
            if (enemyAI.enemyType == EnemyType::Emperor &&
                enemyAI.enemyState == EnemyState::Defeated)
            {
                std::string defAnimName = "AncientStandEmperorDefeated";
                if (animation.animation.getName() != defAnimName) {
                    const Animation& defeatAnim = m_game.assets().getAnimation(defAnimName);
                    animation.animation = defeatAnim;
                    animation.animation.reset();
                }
                sf::Sprite sprite = animation.animation.getSprite();
                float scaleX = (enemyAI.facingDirection < 0.f) ? -1.f : 1.f;
                sprite.setScale(scaleX, 1.f);
                sprite.setPosition(eTrans.pos.x, eTrans.pos.y);
                sprite.setOrigin(animation.animation.getSize().x * 0.5f,
                                animation.animation.getSize().y * 0.5f);
                m_game.window().draw(sprite);
            }
            else
            {
                // Check if enemy is invulnerable
                bool shouldDraw = true;
                if (enemy->has<CHealth>()) {
                    auto& health = enemy->get<CHealth>();
                    if (health.invulnerabilityTimer) {
                        // Blink effect - show sprite every other 0.1 seconds
                        shouldDraw = static_cast<int>(health.invulnerabilityTimer * 13) % 2 == 0;
                    }
                }
                
                if (shouldDraw) {
                    // Continue with your normal sprite drawing code
                    sf::Sprite sprite = animation.animation.getSprite();
                    // Set position, scale, etc.
                    sprite.setPosition(eTrans.pos.x, eTrans.pos.y);
                    sprite.setOrigin(animation.animation.getSize().x * 0.5f,
                                    animation.animation.getSize().y * 0.5f);
                    m_game.window().draw(sprite);
                }
            }
        }
        
        // Debug bounding box
        if (m_showBoundingBoxes && enemy->has<CBoundingBox>()) {
            auto& bbox = enemy->get<CBoundingBox>();
            debugBox.setSize(sf::Vector2f(bbox.size.x, bbox.size.y));
            debugBox.setOrigin(bbox.halfSize.x, bbox.halfSize.y);
            debugBox.setPosition(eTrans.pos.x, eTrans.pos.y);
            debugBox.setOutlineColor(sf::Color::Magenta);
            debugBox.setOutlineThickness(2.f);
            m_game.window().draw(debugBox);
        }
    }
    // Render player sword
    for (auto& sword : m_entityManager.getEntities("sword")) {
        if (!sword->has<CTransform>()) continue;
        auto& swTrans = sword->get<CTransform>();
        if (sword->has<CAnimation>()) {
            auto& anim = sword->get<CAnimation>();
            sf::Sprite sprite = anim.animation.getSprite();
            sprite.setPosition(swTrans.pos.x, swTrans.pos.y);
            m_game.window().draw(sprite);
        }
        if (m_showBoundingBoxes && sword->has<CBoundingBox>()) {
            auto& bbox = sword->get<CBoundingBox>();
            debugBox.setSize(sf::Vector2f(bbox.size.x, bbox.size.y));
            debugBox.setOrigin(bbox.halfSize.x, bbox.halfSize.y);
            debugBox.setPosition(swTrans.pos.x, swTrans.pos.y);
            debugBox.setFillColor(sf::Color::Transparent);
            debugBox.setOutlineColor(sf::Color::Yellow);
            debugBox.setOutlineThickness(2.f);
            m_game.window().draw(debugBox);
        }
    }

    // Render enemy swords
    for (auto& esword : m_entityManager.getEntities("enemySword")) {
        if (!esword->has<CTransform>()) continue;
        auto& esTrans = esword->get<CTransform>();
        //std::cout << "Rendering enemy sword at " << esTrans.pos.x << "," << esTrans.pos.y << "\n";
        if (esword->has<CAnimation>()) {
            //std::cout << "Rendering enemy sword\n";
            auto& anim = esword->get<CAnimation>();
            sf::Sprite sprite = anim.animation.getSprite();
            sprite.setPosition(esTrans.pos.x, esTrans.pos.y);
            sprite.setOrigin(anim.animation.getSize().x / 2.f,
                             anim.animation.getSize().y / 2.f);
            m_game.window().draw(sprite);
        }
        if (m_showBoundingBoxes && esword->has<CBoundingBox>()) {
            //std::cout << "Rendering enemy sword bounding box\n";
            auto& bbox = esword->get<CBoundingBox>();
            debugBox.setSize(sf::Vector2f(bbox.size.x, bbox.size.y));
            float dir = (esTrans.pos.x < 0) ? -1.f : 1.f;
            debugBox.setOrigin((dir < 0) ? bbox.size.x : bbox.halfSize.x, bbox.halfSize.y);
            debugBox.setPosition(esTrans.pos.x, esTrans.pos.y);
            debugBox.setFillColor(sf::Color::Transparent);
            debugBox.setOutlineColor(sf::Color::Yellow);
            debugBox.setOutlineThickness(2.f);
            m_game.window().draw(debugBox);
        }
    }

    // Render enemy swords
    for (auto& esword : m_entityManager.getEntities("EmperorSword")) {
        if (!esword->has<CTransform>()) continue;
        auto& esTrans = esword->get<CTransform>();
        //std::cout << "Rendering enemy sword at " << esTrans.pos.x << "," << esTrans.pos.y << "\n";
        if (esword->has<CAnimation>()) {
            //std::cout << "Rendering enemy sword\n";
            auto& anim = esword->get<CAnimation>();
            sf::Sprite sprite = anim.animation.getSprite();
            sprite.setPosition(esTrans.pos.x, esTrans.pos.y);
            sprite.setOrigin(anim.animation.getSize().x / 2.f,
                                anim.animation.getSize().y / 2.f);
            m_game.window().draw(sprite);
        }
        if (m_showBoundingBoxes && esword->has<CBoundingBox>()) {
            //std::cout << "Rendering enemy sword bounding box\n";
            auto& bbox = esword->get<CBoundingBox>();
            debugBox.setSize(sf::Vector2f(bbox.size.x, bbox.size.y));
            float dir = (esTrans.pos.x < 0) ? -1.f : 1.f;
            debugBox.setOrigin((dir < 0) ? bbox.size.x : bbox.halfSize.x, bbox.halfSize.y);
            debugBox.setPosition(esTrans.pos.x, esTrans.pos.y);
            debugBox.setFillColor(sf::Color::Transparent);
            debugBox.setOutlineColor(sf::Color::Yellow);
            debugBox.setOutlineThickness(2.f);
            m_game.window().draw(debugBox);
        }
    }

    // Render enemy swords
    for (auto& esword : m_entityManager.getEntities("EmperorSwordArmor")) {
        if (!esword->has<CTransform>()) continue;
        auto& esTrans = esword->get<CTransform>();
        //std::cout << "Rendering enemy sword at " << esTrans.pos.x << "," << esTrans.pos.y << "\n";
        if (esword->has<CAnimation>()) {
            //std::cout << "Rendering enemy sword\n";
            auto& anim = esword->get<CAnimation>();
            sf::Sprite sprite = anim.animation.getSprite();
            sprite.setPosition(esTrans.pos.x, esTrans.pos.y);
            sprite.setOrigin(anim.animation.getSize().x / 2.f,
                                anim.animation.getSize().y / 2.f);
            m_game.window().draw(sprite);
        }
        if (m_showBoundingBoxes && esword->has<CBoundingBox>()) {
            //std::cout << "Rendering enemy sword bounding box\n";
            auto& bbox = esword->get<CBoundingBox>();
            debugBox.setSize(sf::Vector2f(bbox.size.x, bbox.size.y));
            float dir = (esTrans.pos.x < 0) ? -1.f : 1.f;
            debugBox.setOrigin((dir < 0) ? bbox.size.x : bbox.halfSize.x, bbox.halfSize.y);
            debugBox.setPosition(esTrans.pos.x, esTrans.pos.y);
            debugBox.setFillColor(sf::Color::Transparent);
            debugBox.setOutlineColor(sf::Color::Yellow);
            debugBox.setOutlineThickness(2.f);
            m_game.window().draw(debugBox);
        }
    }
    
    // Render enemy grave
    for (auto& egrave : m_entityManager.getEntities("enemyGrave")) {
        if (!egrave->has<CTransform>()) continue;
        auto& esTrans = egrave->get<CTransform>();
        //std::cout << "Rendering enemy sword at " << esTrans.pos.x << "," << esTrans.pos.y << "\n";
        if (egrave->has<CAnimation>()) {
            //std::cout << "Rendering enemy sword\n";
            auto& anim = egrave->get<CAnimation>();
            sf::Sprite sprite = anim.animation.getSprite();
            sprite.setPosition(esTrans.pos.x, esTrans.pos.y);
            sprite.setOrigin(anim.animation.getSize().x / 2.f,
                                anim.animation.getSize().y / 2.f);
            m_game.window().draw(sprite);
        }
        if (m_showBoundingBoxes && egrave->has<CBoundingBox>()) {
            //std::cout << "Rendering enemy sword bounding box\n";
            auto& bbox = egrave->get<CBoundingBox>();
            debugBox.setSize(sf::Vector2f(bbox.size.x, bbox.size.y));
            float dir = (esTrans.pos.x < 0) ? -1.f : 1.f;
            debugBox.setOrigin((dir < 0) ? bbox.size.x : bbox.halfSize.x, bbox.halfSize.y);
            debugBox.setPosition(esTrans.pos.x, esTrans.pos.y);
            debugBox.setFillColor(sf::Color::Transparent);
            debugBox.setOutlineColor(sf::Color::Yellow);
            debugBox.setOutlineThickness(2.f);
            m_game.window().draw(debugBox);
        }
    }

    // Render enemy bullets
    for (auto& bullet : m_entityManager.getEntities("enemyBullet")) {
        if (!bullet->has<CTransform>()) continue;
        auto& bulletTrans = bullet->get<CTransform>();

        if (bullet->has<CAnimation>()) {
            auto& anim = bullet->get<CAnimation>();
            sf::Sprite sprite = anim.animation.getSprite();
            sprite.setPosition(bulletTrans.pos.x, bulletTrans.pos.y);
            sprite.setOrigin(anim.animation.getSize().x / 2.f,
                            anim.animation.getSize().y / 2.f);
            m_game.window().draw(sprite);
        }

        if (m_showBoundingBoxes && bullet->has<CBoundingBox>()) {
            auto& bbox = bullet->get<CBoundingBox>();
            sf::RectangleShape debugBox;
            debugBox.setSize(sf::Vector2f(bbox.size.x, bbox.size.y));
            debugBox.setOrigin(bbox.halfSize.x, bbox.halfSize.y);
            debugBox.setPosition(bulletTrans.pos.x, bulletTrans.pos.y);
            debugBox.setFillColor(sf::Color::Transparent);
            debugBox.setOutlineColor(sf::Color::Cyan); // Different color for bullets
            debugBox.setOutlineThickness(2.f);
            m_game.window().draw(debugBox);
        }
    }
    for (auto& bullet : m_entityManager.getEntities("playerBullet")) {
        if (!bullet->has<CTransform>()) continue;
        auto& bulletTrans = bullet->get<CTransform>();
    
        // Draw bullet sprite if it has an animation
        if (bullet->has<CAnimation>()) {
            auto& anim = bullet->get<CAnimation>();
            sf::Sprite sprite = anim.animation.getSprite();
            sprite.setPosition(bulletTrans.pos.x, bulletTrans.pos.y);
            sprite.setOrigin(anim.animation.getSize().x * 0.5f,
                             anim.animation.getSize().y * 0.5f);
            m_game.window().draw(sprite);
        }
    
        // Draw bounding box if enabled
        if (m_showBoundingBoxes && bullet->has<CBoundingBox>()) {
            auto& bbox = bullet->get<CBoundingBox>();
            sf::RectangleShape debugBox;
            debugBox.setSize(sf::Vector2f(bbox.size.x, bbox.size.y));
            debugBox.setOrigin(bbox.halfSize.x, bbox.halfSize.y);
            debugBox.setPosition(bulletTrans.pos.x, bulletTrans.pos.y);
            debugBox.setFillColor(sf::Color::Transparent);
            debugBox.setOutlineColor(sf::Color::Green); // or any color you prefer
            debugBox.setOutlineThickness(2.f);
            m_game.window().draw(debugBox);
        }
    }
    // --- HUD: Black Bar with Score, Time-of-Day, Health, and Stamina ---
    m_game.window().setView(defaultView);
    {
        // Altezza della barra HUD
        float hudHeight = 80.f;
        sf::RectangleShape hudBar(sf::Vector2f(static_cast<float>(windowSize.x), hudHeight));
        hudBar.setFillColor(sf::Color::Black);
        hudBar.setPosition(0.f, windowSize.y - hudHeight);
        m_game.window().draw(hudBar);
    
        // Determina l’era (PRESENT, PAST, ALTERED PRESENT) dal worldType
        std::string centerEra;
        if      (m_game.worldType == "Alien")   centerEra = "PRESENT";
        else if (m_game.worldType == "Ancient") centerEra = "PAST";
        else if (m_game.worldType == "Future")  centerEra = "ALTERED PRESENT";
        else                                    centerEra = "UNKNOWN ERA";
    
        // ----- LATO SINISTRO: NOME LIVELLO + SCORE -----
        {
            // 1) Nome del livello (es. "ANCIENT ROME (NIGHT)")
            sf::Text levelText;
            levelText.setFont(m_game.assets().getFont("Menu"));
            levelText.setCharacterSize(18);
            levelText.setFillColor(sf::Color::White);
            levelText.setString(m_timeofday); 
            float leftX = 10.f;
            float topY  = windowSize.y - hudHeight + 10.f; 
            levelText.setPosition(leftX, topY);
            m_game.window().draw(levelText);
    
            // 2) Score sotto al nome del livello
            sf::Text scoreText;
            scoreText.setFont(m_game.assets().getFont("Menu"));
            scoreText.setCharacterSize(18);
            scoreText.setFillColor(sf::Color::White);
            scoreText.setString("Score: " + std::to_string(m_score));
            // Più spazio tra level name e score (35 invece di 25)
            scoreText.setPosition(leftX, topY + 35.f);
            m_game.window().draw(scoreText);
        }
    
        // ----- CENTRO: ERA (PRESENT, PAST, ALTERED PRESENT) -----
        {
            sf::Text eraText;
            eraText.setFont(m_game.assets().getFont("Menu"));
            eraText.setCharacterSize(28); // Più grande
            eraText.setFillColor(sf::Color::White);
            eraText.setString(centerEra);
    
            sf::FloatRect textRect = eraText.getLocalBounds();
            float centerX = (windowSize.x - textRect.width) * 0.5f;
            float centerY = windowSize.y - hudHeight + 25.f;
            eraText.setPosition(centerX, centerY);
            m_game.window().draw(eraText);
        }
        // ----- LATO DESTRO: BARRA HEALTH E STAMINA -----
        auto players = m_entityManager.getEntities("player");
        if (!players.empty()) {
            auto& player = players[0];
            if (player->has<CHealth>() && player->has<CState>()) {
                auto& health = player->get<CHealth>();
                auto& state  = player->get<CState>();

                // Calcolo ratio Health
                float currentHealth = health.currentHealth;
                float maxHealth     = health.maxHealth;
                float healthRatio   = (maxHealth > 0.f) ? (currentHealth / maxHealth) : 0.f;
                healthRatio         = std::clamp(healthRatio, 0.f, 1.f);

                // Calcolo ratio Stamina
                float shieldStamina = state.shieldStamina;
                float maxStamina    = state.maxshieldStamina;
                float staminaRatio  = (maxStamina > 0.f) ? (shieldStamina / maxStamina) : 0.f;
                staminaRatio        = std::clamp(staminaRatio, 0.f, 1.f);

                // Dimensioni delle barre
                float barWidth   = 120.f;
                float barHeight  = 14.f;
                float barSpacing = 12.f;  // Distanza verticale tra Health e Stamina
                float rightX     = windowSize.x - 10.f - barWidth;
                float baseY      = windowSize.y - hudHeight + 10.f;

                //---------------------------------------
                // 1) BARRA HEALTH
                //---------------------------------------
                // Posizione della barra Health
                float barX = rightX;
                float barY = baseY;

                // Sfondo (grigio)
                sf::RectangleShape healthBg(sf::Vector2f(barWidth, barHeight));
                healthBg.setFillColor(sf::Color(50, 50, 50));
                healthBg.setPosition(barX, barY);
                m_game.window().draw(healthBg);

                // Riempimento (colore in base a ratio)
                sf::RectangleShape healthRect(sf::Vector2f(barWidth * healthRatio, barHeight));
                healthRect.setFillColor(sf::Color::Green);
                healthRect.setPosition(barX, barY);
                m_game.window().draw(healthRect);

                // Etichetta "Health" a sinistra della barra
                {
                    sf::Text healthLabel;
                    healthLabel.setFont(m_game.assets().getFont("Menu"));
                    healthLabel.setCharacterSize(16);
                    healthLabel.setFillColor(sf::Color::White);
                    healthLabel.setString("Health");

                    // Calcola bounding box del testo
                    sf::FloatRect lblRect = healthLabel.getLocalBounds();

                    // Offset orizzontale a sinistra
                    float offsetX = 15.f; 
                    float offsetY = 8.f;
                    float labelX  = barX - (lblRect.width + offsetX);
                    // Centra verticalmente
                    float labelY  = barY + (barHeight - lblRect.height - offsetY) * 0.5f;

                    healthLabel.setPosition(labelX, labelY);
                    m_game.window().draw(healthLabel);
                }

                //---------------------------------------
                // 2) BARRA STAMINA (facoltativo)
                //---------------------------------------
                // Se vuoi anche la barra Stamina, calcola la Y spostata di barHeight + barSpacing
                float staminaBarX = rightX;
                float staminaBarY = baseY + barHeight + barSpacing;

                // Sfondo Stamina
                sf::RectangleShape staminaBg(sf::Vector2f(barWidth, barHeight));
                staminaBg.setFillColor(sf::Color(50, 50, 50));
                staminaBg.setPosition(staminaBarX, staminaBarY);
                m_game.window().draw(staminaBg);

                // Riempimento Stamina (blu)
                sf::RectangleShape staminaRect(sf::Vector2f(barWidth * staminaRatio, barHeight));
                staminaRect.setFillColor(sf::Color::Blue);
                staminaRect.setPosition(staminaBarX, staminaBarY);
                m_game.window().draw(staminaRect);

                // Etichetta "Stamina" a sinistra
                {
                    sf::Text staminaLabel;
                    staminaLabel.setFont(m_game.assets().getFont("Menu"));
                    staminaLabel.setCharacterSize(16);
                    staminaLabel.setFillColor(sf::Color::White);
                    staminaLabel.setString("Stamina");

                    sf::FloatRect lblRect = staminaLabel.getLocalBounds();
                    float offsetX  = 15.f; 
                    float offsetY = 8.f;
                    float labelX   = staminaBarX - (lblRect.width + offsetX);
                    float labelY   = staminaBarY + (barHeight - lblRect.height - offsetY) * 0.5f;

                    staminaLabel.setPosition(labelX, labelY);
                    m_game.window().draw(staminaLabel);
                }

                // Se stamina == 0, mostra "No stamina!"
                if (shieldStamina <= 0.f) {
                    sf::Text noStaminaText;
                    noStaminaText.setFont(m_game.assets().getFont("Menu"));
                    noStaminaText.setCharacterSize(12);
                    noStaminaText.setFillColor(sf::Color::White);
                    noStaminaText.setString("No stamina!");
                    sf::FloatRect textRect = noStaminaText.getLocalBounds();

                    float textX = staminaBarX + (barWidth - textRect.width)/2.f;
                    float textY = staminaBarY + (barHeight - textRect.height)/2.f;
                    noStaminaText.setPosition(textX, textY);
                    m_game.window().draw(noStaminaText);
                }
            }
            if (player->has<CPlayerEquipment>() && 
            player->get<CPlayerEquipment>().hasFutureArmor &&
            player->has<CAmmo>()) {
            
            auto& ammo = player->get<CAmmo>();
            
            // Calcolo ratio Ammo
            float currentAmmo = static_cast<float>(ammo.currentBullets);
            float maxAmmo = static_cast<float>(ammo.maxBullets);
            float ammoRatio = (maxAmmo > 0.f) ? (currentAmmo / maxAmmo) : 0.f;
            ammoRatio = std::clamp(ammoRatio, 0.f, 1.f);
            
            // Dimensioni della barra ammo
            float barWidth = 120.f;
            float barHeight = 14.f;
            float barSpacing = 12.f;
            float rightX = windowSize.x - 10.f - barWidth;
            float baseY = windowSize.y - hudHeight + 10.f;
            
            // Posizione della barra Ammo (sotto Stamina)
            float ammoBarX = rightX;
            float ammoBarY = baseY + (barHeight + barSpacing) * 2; // Sotto Health e Stamina
            
            // Sfondo Ammo
            sf::RectangleShape ammoBg(sf::Vector2f(barWidth, barHeight));
            ammoBg.setFillColor(sf::Color(50, 50, 50));
            ammoBg.setPosition(ammoBarX, ammoBarY);
            m_game.window().draw(ammoBg);
            
            // Riempimento Ammo (giallo/arancione)
            sf::RectangleShape ammoRect(sf::Vector2f(barWidth * ammoRatio, barHeight));
            ammoRect.setFillColor(sf::Color(255, 165, 0)); // Arancione
            ammoRect.setPosition(ammoBarX, ammoBarY);
            m_game.window().draw(ammoRect);
            
            // Etichetta "Ammo" a sinistra
            {
                sf::Text ammoLabel;
                ammoLabel.setFont(m_game.assets().getFont("Menu"));
                ammoLabel.setCharacterSize(16);
                ammoLabel.setFillColor(sf::Color::White);
                ammoLabel.setString("Ammo");
                
                sf::FloatRect lblRect = ammoLabel.getLocalBounds();
                float offsetX = 15.f;
                float offsetY = 8.f;
                float labelX = ammoBarX - (lblRect.width + offsetX);
                float labelY = ammoBarY + (barHeight - lblRect.height - offsetY) * 0.5f;
                
                ammoLabel.setPosition(labelX, labelY);
                m_game.window().draw(ammoLabel);
            }
            
            // Se l'ammo è esaurito, mostra "No ammo, reloading..."
            if (ammo.currentBullets <= 0) {
                sf::Text noAmmoText;
                noAmmoText.setFont(m_game.assets().getFont("Menu"));
                noAmmoText.setCharacterSize(11);
                noAmmoText.setFillColor(sf::Color::White);
                noAmmoText.setString("No ammo, reloading...");
                sf::FloatRect textRect = noAmmoText.getLocalBounds();
                
                float textX = ammoBarX + (barWidth - textRect.width)/2.f;
                float textY = ammoBarY + (barHeight - textRect.height)/2.f;
                noAmmoText.setPosition(textX, textY);
                m_game.window().draw(noAmmoText);
            }
            // ----- SUPER MOVE STATUS -----
            if (player->has<CPlayerEquipment>() && 
                player->get<CPlayerEquipment>().hasFutureArmor) {
                
                auto& state = player->get<CState>();
                
                // Use the existing variables with their correct names
                float superMoveCooldown = state.superBulletTimer;
                float maxSuperMoveCooldown = state.superBulletCooldown;
                
                // Determine SuperMove status based on its cooldown
                std::string superMoveStatus;
                sf::Color statusColor;
                
                if (superMoveCooldown <= 0.f) {
                    superMoveStatus = "SUPER MOVE: READY";
                    statusColor = sf::Color::Green;
                } else {
                    // Calculate percentage of cooldown remaining
                    float cooldownPercent = (maxSuperMoveCooldown - superMoveCooldown) / maxSuperMoveCooldown * 100.f;
                    // Make sure it's between 0-100%
                    cooldownPercent = std::clamp(cooldownPercent, 0.f, 100.f);
                    superMoveStatus = "SUPER MOVE: CHARGING " + std::to_string(static_cast<int>(cooldownPercent)) + "%";
                    statusColor = sf::Color::Yellow;
                }
                
                // Positioning to the left of Ammo bar
                sf::Text superMoveText;
                superMoveText.setFont(m_game.assets().getFont("Menu"));
                superMoveText.setCharacterSize(16);
                superMoveText.setFillColor(statusColor);
                superMoveText.setString(superMoveStatus);
                
                // Use the same X position as the "Ammo" label but with adjusted Y
                float superMoveX = ammoBarX - 300.f; // Position it to the left of the Ammo bar
                float superMoveY = ammoBarY; // Same height as Ammo bar
                
                superMoveText.setPosition(superMoveX, superMoveY);
                m_game.window().draw(superMoveText);
            }
            // Opzionale: mostra numerici (es. "3 / 6")
            {
                sf::Text ammoCountText;
                ammoCountText.setFont(m_game.assets().getFont("Menu"));
                ammoCountText.setCharacterSize(14);
                ammoCountText.setFillColor(sf::Color::White);
                ammoCountText.setString(std::to_string(ammo.currentBullets) + " / " + 
                                    std::to_string(ammo.maxBullets));
                
                sf::FloatRect countRect = ammoCountText.getLocalBounds();
                float textX = ammoBarX + barWidth + 10.f; // A destra della barra
                float textY = ammoBarY + (barHeight - countRect.height)/2.f;
                ammoCountText.setPosition(textX, textY);
                m_game.window().draw(ammoCountText);
            }
            
            // Se è in ricarica, mostra il progresso
            if (ammo.isReloading) {
                float reloadProgress = ammo.currentReloadTime / ammo.reloadTime;
                
                // Barra di progresso ricarica (sotto la barra principale)
                float progressY = ammoBarY + barHeight + 2.f;
                sf::RectangleShape reloadBar(sf::Vector2f(barWidth * reloadProgress, 3.f));
                reloadBar.setFillColor(sf::Color::Yellow);
                reloadBar.setPosition(ammoBarX, progressY);
                m_game.window().draw(reloadBar);
            }
        }
        }
    }
    m_game.window().setView(m_cameraView);
    m_game.window().display();
}

void PlayRenderer::drawGrid() {
    int windowHeight = m_game.window().getSize().y;
    const int gridSize = 96;
    const int worldWidth = 120;
    const int worldHeight = 40;
    sf::Color gridColor(255, 255, 255, 100);

    for (int x = 0; x <= worldWidth; ++x) {
        sf::Vertex line[] = {
            sf::Vertex(sf::Vector2f(x * gridSize, windowHeight - (worldHeight * gridSize)), gridColor),
            sf::Vertex(sf::Vector2f(x * gridSize, windowHeight), gridColor)
        };
        m_game.window().draw(line, 2, sf::Lines);
    }

    for (int y = 0; y <= worldHeight; ++y) {
        sf::Vertex line[] = {
            sf::Vertex(sf::Vector2f(0, windowHeight - (y * gridSize)), gridColor),
            sf::Vertex(sf::Vector2f(worldWidth * gridSize, windowHeight - (y * gridSize)), gridColor)
        };
        m_game.window().draw(line, 2, sf::Lines);
    }

    const sf::Font& font = m_game.assets().getFont("Menu");
    for (int x = 0; x < worldWidth; ++x) {
        for (int y = 0; y < worldHeight; ++y) {
            sf::Text coordText;
            coordText.setFont(font);
            coordText.setCharacterSize(14);
            coordText.setFillColor(sf::Color::White);
            coordText.setString(std::to_string(x) + "," + std::to_string(y));
            coordText.setPosition(x * gridSize + 4, windowHeight - (y * gridSize) - gridSize + 4);
            m_game.window().draw(coordText);
        }
    }
}

void PlayRenderer::drawDebugLine(const Vec2<float>& start, const Vec2<float>& end, sf::Color color) {
    sf::Vertex line[] = {
        sf::Vertex(sf::Vector2f(start.x, start.y), color),
        sf::Vertex(sf::Vector2f(end.x, end.y), color)
    };
    m_game.window().draw(line, 2, sf::Lines);
}
