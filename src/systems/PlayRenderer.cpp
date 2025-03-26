#include "PlayRenderer.h"
#include "Animation.hpp"
#include "DialogueSystem.h"
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
      m_score(score),
      m_timeofday("day"),
      m_dialogueSystem(nullptr)
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

void PlayRenderer::renderDialogue(DialogueSystem* dialogueSystem)
{
    if (!dialogueSystem || !dialogueSystem->isDialogueActive()) {
        return;
    }

    sf::View currentView = m_game.window().getView();
    m_game.window().setView(m_game.window().getDefaultView());

    const DialogueMessage* message = dialogueSystem->getCurrentMessage();
    if (!message) {
        m_game.window().setView(currentView);
        return;
    }

    // Use custom dimensions from the message
    float boxWidth = message->boxWidth;
    float boxHeight = message->boxHeight;
    float boxX = message->dialogueBoxPosition.x;
    float boxY = message->dialogueBoxPosition.y;

    dialogueSystem->dialogueBox.setSize({boxWidth, boxHeight});
    dialogueSystem->dialogueBox.setPosition(boxX, boxY);

    float portraitSize = 120.f;
    float portraitX = message->portraitOnLeft ? boxX + 15.f : boxX + boxWidth - portraitSize - 15.f;

    dialogueSystem->portraitSprite.setTexture(dialogueSystem->getPortraitTexture(), true);
    float scaleX = portraitSize / dialogueSystem->getPortraitTexture().getSize().x;
    float scaleY = portraitSize / dialogueSystem->getPortraitTexture().getSize().y;
    dialogueSystem->portraitSprite.setScale(scaleX, scaleY);
    dialogueSystem->portraitSprite.setPosition(portraitX, boxY + 15.f);

    float textX = message->portraitOnLeft ? portraitX + portraitSize + 15.f : boxX + 15.f;
    // Calculate max text width to avoid overlapping with portrait
    float maxTextWidth = message->portraitOnLeft ? 
                         boxWidth - (portraitSize + 30.f) : 
                         boxWidth - portraitSize - 30.f;

    // Choose the right font based on language
    const sf::Font& fontToUse = (m_game.getLanguage() == "Japanese") ? 
                                m_game.assets().getFont("Japanese") : 
                                m_game.assets().getFont("Menu");
    
    // Set the speaker text font
    dialogueSystem->speakerText.setFont(fontToUse);
    dialogueSystem->speakerText.setCharacterSize(30);
    dialogueSystem->speakerText.setFillColor(message->speakerColor);
    
    std::string speakerWithUniverse;
    if (m_game.getLanguage() == "English") {
        if (message->speaker.find("Alien Legionary") != std::string::npos) {
            speakerWithUniverse = message->speaker + " [Universe #" + std::to_string(m_game.universeNumber) + "]";
        } else if (message->speaker == "????") {
            speakerWithUniverse = message->speaker + " [Universe #" + std::to_string(m_game.alternateUniverseNumber) + "]";
        } else if (message->speaker.find("GUIDE") != std::string::npos) {
            speakerWithUniverse = message->speaker;
        } else {
            speakerWithUniverse = message->speaker + " [Universe #" + std::to_string(m_game.alternateUniverseNumber2) + "]";
        }
    } else {
        if (message->speaker.find("エイリアン兵士") != std::string::npos) {
            speakerWithUniverse = message->speaker + " [宇宙 #" + std::to_string(m_game.universeNumber) + "]";
        } else if (message->speaker == "????") {
            speakerWithUniverse = message->speaker + " [宇宙 #" + std::to_string(m_game.alternateUniverseNumber) + "]";
        } else if (message->speaker.find("ガイド") != std::string::npos) {
            speakerWithUniverse = message->speaker;
        } else {
            speakerWithUniverse = message->speaker + " [宇宙 #" + std::to_string(m_game.alternateUniverseNumber2) + "]";
        }
    }
    dialogueSystem->speakerText.setString(
        sf::String::fromUtf8(
            speakerWithUniverse.begin(), 
            speakerWithUniverse.end()
        )
    );
    dialogueSystem->speakerText.setPosition(textX, boxY + 15.f);

    // Set the message text font
    dialogueSystem->messageText.setFont(fontToUse);
    dialogueSystem->messageText.setCharacterSize(message->messageFontSize); 
    dialogueSystem->messageText.setFillColor(message->messageColor);
    auto displayed = dialogueSystem->getDisplayedText();
    dialogueSystem->messageText.setString(
        sf::String::fromUtf8(
            displayed.begin(), 
            displayed.end()
        )
    );
    dialogueSystem->messageText.setPosition(textX, boxY + 50.f);

    m_game.window().draw(dialogueSystem->dialogueBox);
    m_game.window().draw(dialogueSystem->portraitSprite);
    m_game.window().draw(dialogueSystem->speakerText);
    m_game.window().draw(dialogueSystem->messageText);

    if (!dialogueSystem->isTyping()) 
    {
        // Set up font once
        dialogueSystem->continueText.setFont(fontToUse);
        dialogueSystem->continueText.setCharacterSize(16);
        dialogueSystem->continueText.setFillColor(sf::Color::White);
    
        // Calculate remaining time if we are waiting
        int remainingTime = static_cast<int>(5.0f - dialogueSystem->getCompletionTimer());
    
        // Japanese texts
        std::string japWaiting = std::to_string(remainingTime) + "秒後に続行します...";
        std::string japPrompt  = "スペースキーで続行...";
    
        // English texts
        std::string engWaiting = "Continuing in " + std::to_string(remainingTime) + " seconds...";
        std::string engPrompt  = "Press SPACE to continue...";
    
        // Pick waiting or prompt version
        std::string chosenText = dialogueSystem->isWaitingAfterCompletion()
            ? (m_game.getLanguage() == "Japanese" ? japWaiting : engWaiting)
            : (m_game.getLanguage() == "Japanese" ? japPrompt  : engPrompt);
    
        // Convert from UTF-8 so SFML renders correctly
        dialogueSystem->continueText.setString(
            sf::String::fromUtf8(chosenText.begin(), chosenText.end())
        );
    
        float continueTextHeight = dialogueSystem->continueText.getLocalBounds().height;
        float continueTextOffsetX = (message->speaker == "エイリアン兵士" || message->speaker == "***ガイド***" || message->speaker == "Alien Legionary" || message->speaker == "***GUIDE***") ? 150.f : 10.f;

        dialogueSystem->continueText.setPosition(
            boxX + continueTextOffsetX,
            boxY + boxHeight - continueTextHeight - 10.f
        );
        m_game.window().draw(dialogueSystem->continueText);
    }

    m_game.window().setView(currentView);
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
        if (!enemy->has<CTransform>() || !enemy->has<CHealth>())
            continue;

        auto& eTrans  = enemy->get<CTransform>();
        auto& eHealth = enemy->get<CHealth>();

        // Determine if this enemy is an Emperor
        bool isEmperor = false;
        if (enemy->has<CEnemyAI>()) {
            auto& enemyAI = enemy->get<CEnemyAI>();
            isEmperor = (enemyAI.enemyType == EnemyType::Emperor);
        }

        // ----- Draw Health Bars -----
        if (isEmperor) {
            // Emperor health bar settings
            float barWidth = 120.f;
            float barHeight = 10.f;
            float offsetY = 140.f;
            float spacing = 15.f;

            float currentHealth = eHealth.currentHealth;
            float maxHealth = eHealth.maxHealth;
            float healthRatio = (maxHealth > 0.f) ? (currentHealth / maxHealth) : 0.f;
            healthRatio = std::clamp(healthRatio, 0.f, 1.f);

            // Define phase thresholds
            const float phase1Threshold = 0.7f; // > 70% HP (Phase 1)
            const float phase2Threshold = 0.5f; // 70%-50% HP (Phase 2)
            const float phase3Threshold = 0.1f; // 50%-10% HP (Phase 3)

            // Bar colors for each phase (top: normal, middle: enhanced, bottom: final burst)
            sf::Color barColors[3] = {
                sf::Color(220, 50, 50),    // Bottom bar (Phase 3)
                sf::Color(220, 220, 50),   // Middle bar (Phase 2)
                sf::Color(50, 220, 50)     // Top bar (Phase 1)
            };

            float baseX = eTrans.pos.x - (barWidth / 2.f);
            float baseY = eTrans.pos.y - offsetY;

            // Draw three stacked bars (top to bottom)
            for (int i = 0; i < 3; i++) {
                float barX = baseX;
                float barY = baseY + (2 - i) * (barHeight + spacing);

                // Background
                sf::RectangleShape healthBg(sf::Vector2f(barWidth, barHeight));
                healthBg.setFillColor(sf::Color(50, 50, 50));
                healthBg.setPosition(barX, barY);
                m_game.window().draw(healthBg);

                // Calculate fill ratio per bar
                float fillRatio = 0.f;
                if (i == 2) { // Top bar (Phase 1: 100%-70%)
                    if (healthRatio > phase1Threshold)
                        fillRatio = (healthRatio - phase1Threshold) / (1.f - phase1Threshold);
                } else if (i == 1) { // Middle bar (Phase 2: 70%-50%)
                    if (healthRatio > phase2Threshold && healthRatio <= phase1Threshold)
                        fillRatio = (healthRatio - phase2Threshold) / (phase1Threshold - phase2Threshold);
                    else if (healthRatio > phase1Threshold)
                        fillRatio = 1.f;
                } else if (i == 0) { // Bottom bar (Phase 3: 50%-10%)
                    if (healthRatio > phase3Threshold && healthRatio <= phase2Threshold)
                        fillRatio = (healthRatio - phase3Threshold) / (phase2Threshold - phase3Threshold);
                    else if (healthRatio > phase2Threshold)
                        fillRatio = 1.f;
                }

                // Filled portion
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

                // Division markers
                int divisions = (i == 0) ? 4 : (i == 1) ? 2 : 3;
                for (int j = 1; j < divisions; j++) {
                    float markerX = barX + (barWidth * j / divisions);
                    sf::Vertex line[] = {
                        sf::Vertex(sf::Vector2f(markerX, barY), sf::Color::White),
                        sf::Vertex(sf::Vector2f(markerX, barY + barHeight), sf::Color::White)
                    };
                    m_game.window().draw(line, 2, sf::Lines);
                }
            }

            // Final Attack indicator if health is below phase3 threshold
            if (healthRatio <= phase3Threshold) {
                sf::RectangleShape finalPhaseBar(sf::Vector2f(barWidth, barHeight));
                finalPhaseBar.setFillColor(sf::Color(150, 0, 0));
                finalPhaseBar.setPosition(baseX, baseY + 3 * (barHeight + spacing));
                m_game.window().draw(finalPhaseBar);

                if (m_game.assets().hasFont("Menu")) {
                    sf::Text finalText;
                    finalText.setFont(m_game.assets().getFont("Menu"));
                    finalText.setString("FINAL ATTACK (<10%)");
                    finalText.setCharacterSize(12);
                    finalText.setFillColor(sf::Color::Red);
                    sf::FloatRect textBounds = finalText.getLocalBounds();
                    finalText.setPosition(baseX - textBounds.width - 5.f,
                        baseY + 3 * (barHeight + spacing) + (barHeight / 2.f) - (textBounds.height / 2.f));
                    m_game.window().draw(finalText);
                }
            }

            // Draw overall health percentage text above the bars
            if (m_game.assets().hasFont("Menu")) {
                sf::Text healthText;
                healthText.setFont(m_game.assets().getFont("Menu"));
                healthText.setString(std::to_string(static_cast<int>(healthRatio * 100)) + "%");
                healthText.setCharacterSize(14);
                healthText.setFillColor(sf::Color::White);
                sf::FloatRect textBounds = healthText.getLocalBounds();
                healthText.setPosition(baseX + (barWidth / 2.f) - (textBounds.width / 2.f), baseY - 25.f);
                m_game.window().draw(healthText);
            }
        } else if (enemy->get<CEnemyAI>().enemyType == EnemyType::Super || enemy->get<CEnemyAI>().enemyType == EnemyType::Super2) {
            // Super enemy "mystery" health bar
            float barWidth = 60.f;  // Slightly wider than regular enemies
            float barHeight = 7.f;  // Slightly taller than regular enemies
            float offsetY = 65.f;
            
            float barX = eTrans.pos.x - (barWidth / 2.f);
            float barY = eTrans.pos.y - offsetY;
            
            // Black background
            sf::RectangleShape healthBg(sf::Vector2f(barWidth, barHeight));
            healthBg.setFillColor(sf::Color(0, 0, 0));
            healthBg.setPosition(barX, barY);
            m_game.window().draw(healthBg);
            
            // Red border
            sf::RectangleShape borderRect(sf::Vector2f(barWidth, barHeight));
            borderRect.setFillColor(sf::Color::Transparent);
            borderRect.setOutlineColor(sf::Color(200, 0, 0));
            borderRect.setOutlineThickness(1.5f);
            borderRect.setPosition(barX, barY);
            m_game.window().draw(borderRect);
            
            // "???" text in the middle
            if (m_game.assets().hasFont("Menu")) {
                sf::Text mysteryText;
                mysteryText.setFont(m_game.assets().getFont("Menu"));
                mysteryText.setString("???");
                mysteryText.setCharacterSize(12);
                mysteryText.setFillColor(sf::Color(200, 0, 0));  // Red text
                
                sf::FloatRect textBounds = mysteryText.getLocalBounds();
                mysteryText.setPosition(
                    barX + (barWidth / 2.f) - (textBounds.width / 2.f),
                    barY + (barHeight / 2.f) - (textBounds.height / 2.f) - 2.f  // Small offset for better centering
                );
                
                m_game.window().draw(mysteryText);
            }
        } else {
            // Regular enemy health bar
            float barWidth = 40.f;
            float barHeight = 5.f;
            float offsetY = 60.f;
            float currentHealth = eHealth.currentHealth;
            float maxHealth = eHealth.maxHealth;
            float healthRatio = (maxHealth > 0.f) ? (currentHealth / maxHealth) : 0.f;
            healthRatio = std::clamp(healthRatio, 0.f, 1.f);
            float barX = eTrans.pos.x - (barWidth / 2.f);
            float barY = eTrans.pos.y - offsetY;
            sf::RectangleShape healthBg(sf::Vector2f(barWidth, barHeight));
            healthBg.setFillColor(sf::Color(50, 50, 50));
            healthBg.setPosition(barX, barY);
            m_game.window().draw(healthBg);
            sf::Color healthColor = (healthRatio > 0.7f) ? sf::Color::Green :
                                    (healthRatio > 0.3f) ? sf::Color::Yellow :
                                    sf::Color::Red;
            sf::RectangleShape healthRect(sf::Vector2f(barWidth * healthRatio, barHeight));
            healthRect.setFillColor(healthColor);
            healthRect.setPosition(barX, barY);
            m_game.window().draw(healthRect);
        }

        if (enemy->has<CAnimation>()) {
            auto& animation = enemy->get<CAnimation>();
            if (isEmperor) {
                auto& enemyAI = enemy->get<CEnemyAI>();
                BossPhase bossPhase = BossPhase::Phase1;  // Default initialization
        
                if (enemy->has<CBossPhase>()) {
                    bossPhase = enemy->get<CBossPhase>().phase;
                }
        
                // If defeated, force defeat animation (Ancient Emperor only)
                if (enemyAI.enemyState == EnemyState::Defeated) {
                    if (m_game.worldType == "Ancient") {
                        std::string defeatAnimName = "AncientStandEmperorDefeated";
                        if (animation.animation.getName() != defeatAnimName) {
                            animation.animation = m_game.assets().getAnimation(defeatAnimName);
                            animation.animation.reset();
                        }
                    }
                }
                // Future Emperor uses CBossPhase instead of EnemyState
                else if (m_game.worldType == "Future" && enemy->has<CBossPhase>()) {
                    std::string phaseNumber = "";
                    std::string actionPrefix = "";
                    
                    // Determine phase number suffix based on boss phase
                    switch (bossPhase) {
                        case BossPhase::Phase1: phaseNumber = ""; break;    // No suffix for Phase 1
                        case BossPhase::Phase2: phaseNumber = "2"; break;   // "2" suffix for Phase 2
                        case BossPhase::Phase3: phaseNumber = "3"; break;   // "3" suffix for Phase 3
                        default: phaseNumber = ""; break;
                    }
                    
                    // Determine action prefix based on current animation or enemy state
                    std::string currentAnimName = animation.animation.getName();
                    
                    // Check what kind of animation is currently playing
                    if (currentAnimName.find("Run") != std::string::npos) {
                        actionPrefix = "FutureRun";
                    } else if (currentAnimName.find("Hit") != std::string::npos) {
                        actionPrefix = "FutureHit";
                    } else if (currentAnimName.find("Attack") != std::string::npos) {
                        actionPrefix = "FutureAttack";
                    } else {
                        actionPrefix = "FutureStand";  // Default to Stand
                    }
                    
                    // Construct the full animation name: [ActionPrefix]Emperor[PhaseNumber]
                    std::string desiredAnimName = actionPrefix + "Emperor" + phaseNumber;
                    
                    // Update animation if needed and it exists
                    if (desiredAnimName != currentAnimName) {
                        if (m_game.assets().hasAnimation(desiredAnimName)) {
                            animation.animation = m_game.assets().getAnimation(desiredAnimName);
                            animation.animation.reset();
                            //std::cout << "[DEBUG] Emperor animation updated to: " << desiredAnimName << std::endl;
                        } else {
                            // std::cerr << "[ERROR] Missing animation: " << desiredAnimName << " for Emperor enemy!" << std::endl;
                            
                            // Fallback to the stand animation for this phase if available
                            std::string fallbackAnim = "FutureStandEmperor" + phaseNumber;
                            if (m_game.assets().hasAnimation(fallbackAnim)) {
                                animation.animation = m_game.assets().getAnimation(fallbackAnim);
                                animation.animation.reset();
                                //std::cout << "[DEBUG] Using fallback animation: " << fallbackAnim << std::endl;
                            }
                        }
                    }
                }
        
                // Apply blink effect if invulnerable
                bool shouldDraw = true;
                if (enemy->has<CHealth>()) {
                    auto& health = enemy->get<CHealth>();
                    if (health.invulnerabilityTimer > 0) {
                        shouldDraw = static_cast<int>(health.invulnerabilityTimer * 10) % 2 == 0;
                    }
                }
        
                if (shouldDraw) {
                    float scaleX = (enemyAI.facingDirection < 0.f) ? -1.f : 1.f;
                    sf::Sprite sprite = animation.animation.getSprite();
                    sprite.setScale(scaleX, 1.f);
                    sprite.setPosition(eTrans.pos.x, eTrans.pos.y);
                    sprite.setOrigin(animation.animation.getSize().x * 0.5f, animation.animation.getSize().y * 0.5f);
                    m_game.window().draw(sprite);
                }
        
                // std::cout << "[DEBUG] Emperor state: " << static_cast<int>(bossPhase)
                //           << " | Current Animation: " << animation.animation.getName() << std::endl;
            } else {
                // Non-Emperor enemies remain unchanged
                bool shouldDraw = true;
                if (enemy->has<CHealth>()) {
                    auto& health = enemy->get<CHealth>();
                    if (health.invulnerabilityTimer > 0) {
                        shouldDraw = static_cast<int>(health.invulnerabilityTimer * 10) % 2 == 0;
                    }
                }
        
                if (shouldDraw) {
                    sf::Sprite sprite = animation.animation.getSprite();
                    sprite.setPosition(eTrans.pos);
                    sprite.setOrigin(animation.animation.getSize().x * 0.5f, animation.animation.getSize().y * 0.5f);
                    m_game.window().draw(sprite);
                }
            }
        }

        // Draw enemy bounding box if enabled
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
    
    // Render emperor black holes
    for (auto& blackHole : m_entityManager.getEntities("emperorBlackHole")) {
        if (!blackHole->has<CTransform>()) continue;
        
        auto& blackHoleTrans = blackHole->get<CTransform>();
        
        if (blackHole->has<CAnimation>()) {
            auto& anim = blackHole->get<CAnimation>();
            sf::Sprite sprite = anim.animation.getSprite();
            sprite.setPosition(blackHoleTrans.pos.x, blackHoleTrans.pos.y);
            sprite.setOrigin(anim.animation.getSize().x / 2.f,
                            anim.animation.getSize().y / 2.f);
            m_game.window().draw(sprite);
        }
        
        if (m_showBoundingBoxes && blackHole->has<CBoundingBox>()) {
            auto& bbox = blackHole->get<CBoundingBox>();
            debugBox.setSize(sf::Vector2f(bbox.size.x, bbox.size.y));
            float dir = (blackHoleTrans.pos.x < 0) ? -1.f : 1.f;
            debugBox.setOrigin((dir < 0) ? bbox.size.x : bbox.halfSize.x, bbox.halfSize.y);
            debugBox.setPosition(blackHoleTrans.pos.x, blackHoleTrans.pos.y);
            debugBox.setFillColor(sf::Color::Transparent);
            debugBox.setOutlineColor(sf::Color::Magenta); // Different color to distinguish black holes
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
        if (m_game.getLanguage() == "English") {
            // Original English era text
            if      (m_game.worldType == "Alien")   centerEra = "PRESENT (YEAR: 2135)";
            else if (m_game.worldType == "Ancient") centerEra = "PAST (YEAR: 225)";
            else if (m_game.worldType == "Future")  centerEra = "ALTERED PRESENT (YEAR: 2135)";
            
            // Add Universe number in English
            m_game.worldType == "Alien" ? centerEra += " | Universe #" + std::to_string(m_game.universeNumber) 
                                        : centerEra += " | Universe #" + std::to_string(m_game.alternateUniverseNumber2);
        } else {
            // Japanese era text
            if      (m_game.worldType == "Alien")   centerEra = "現在 (西暦: 2135年)";
            else if (m_game.worldType == "Ancient") centerEra = "過去 (西暦: 225年)";
            else if (m_game.worldType == "Future")  centerEra = "変化した現在 (西暦: 2135年)";
            
            // Add Universe number in Japanese
            m_game.worldType == "Alien" ? centerEra += " | 宇宙 #" + std::to_string(m_game.universeNumber) 
                                        : centerEra += " | 宇宙 #" + std::to_string(m_game.alternateUniverseNumber2);
        }
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
            eraText.setFont(m_game.assets().getFont("Japanese"));
            eraText.setCharacterSize(28); // Più grande
            eraText.setFillColor(sf::Color::White);
            
            // Set text with UTF-8 conversion for proper Japanese support
            eraText.setString(sf::String::fromUtf8(
                centerEra.begin(),
                centerEra.end()
            ));
    
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

                // Determine color based on health percentage
                sf::Color healthColor;
                if (healthRatio > 0.6f) {
                    healthColor = sf::Color::Green;
                } else if (healthRatio > 0.3f) {
                    healthColor = sf::Color(255, 165, 0); // Orange
                } else {
                    healthColor = sf::Color::Red;
                }

                // Riempimento (colore in base a ratio)
                sf::RectangleShape healthRect(sf::Vector2f(barWidth * healthRatio, barHeight));
                healthRect.setFillColor(healthColor);
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
            ammoRect.setFillColor(sf::Color(142, 68, 173)); 
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

    if (m_dialogueSystem) {
        renderDialogue(m_dialogueSystem);
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
