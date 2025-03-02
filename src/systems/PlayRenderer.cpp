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
    
        // Calcola il rapporto di salute
        float currentHealth = eHealth.currentHealth;
        float maxHealth     = eHealth.maxHealth;
        float healthRatio   = (maxHealth > 0.f) ? (currentHealth / maxHealth) : 0.f;
        if (healthRatio < 0.f) healthRatio = 0.f;
        if (healthRatio > 1.f) healthRatio = 1.f;
    
        // Dimensioni e offset della barra
        float barWidth  = 40.f;  // Larghezza della barra
        float barHeight = 5.f;   // Altezza della barra
        float offsetY   = 60.f;  // Distanza sopra la testa del nemico
    
        // Posizione di base per la barra (centrata orizzontalmente sopra il nemico)
        float barX = eTrans.pos.x - (barWidth / 2.f);
        float barY = eTrans.pos.y - offsetY;
    
        // Sfondo grigio
        sf::RectangleShape healthBg(sf::Vector2f(barWidth, barHeight));
        healthBg.setFillColor(sf::Color(50, 50, 50));
        healthBg.setPosition(barX, barY);
    
        // Barra di colore (verde, giallo, rosso... se vuoi differenziarla)
        // Qui usiamo solo verde come esempio
        sf::RectangleShape healthRect(sf::Vector2f(barWidth * healthRatio, barHeight));
        healthRect.setFillColor(sf::Color::Green);
        healthRect.setPosition(barX, barY);
    
        // Disegno delle due parti
        m_game.window().draw(healthBg);
        m_game.window().draw(healthRect);

        if (enemy->has<CAnimation>()) {
            auto& animation = enemy->get<CAnimation>();
            sf::Sprite sprite = animation.animation.getSprite();
            sprite.setPosition(eTrans.pos.x, eTrans.pos.y);
            sprite.setOrigin(animation.animation.getSize().x / 2.f,
                             animation.animation.getSize().y / 2.f);
            m_game.window().draw(sprite);
        }
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
