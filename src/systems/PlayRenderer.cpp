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
        auto& transform = enemy->get<CTransform>();
        if (enemy->has<CAnimation>()) {
            auto& animation = enemy->get<CAnimation>();
            sf::Sprite sprite = animation.animation.getSprite();
            sprite.setPosition(transform.pos.x, transform.pos.y);
            sprite.setOrigin(animation.animation.getSize().x / 2.f,
                             animation.animation.getSize().y / 2.f);
            m_game.window().draw(sprite);
        }
        if (m_showBoundingBoxes && enemy->has<CBoundingBox>()) {
            auto& bbox = enemy->get<CBoundingBox>();
            debugBox.setSize(sf::Vector2f(bbox.size.x, bbox.size.y));
            debugBox.setOrigin(bbox.halfSize.x, bbox.halfSize.y);
            debugBox.setPosition(transform.pos.x, transform.pos.y);
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
        std::cout << "Rendering enemy sword at " << esTrans.pos.x << "," << esTrans.pos.y << "\n";
        if (esword->has<CAnimation>()) {
            std::cout << "Rendering enemy sword\n";
            auto& anim = esword->get<CAnimation>();
            sf::Sprite sprite = anim.animation.getSprite();
            sprite.setPosition(esTrans.pos.x, esTrans.pos.y);
            sprite.setOrigin(anim.animation.getSize().x / 2.f,
                             anim.animation.getSize().y / 2.f);
            m_game.window().draw(sprite);
        }
        if (m_showBoundingBoxes && esword->has<CBoundingBox>()) {
            std::cout << "Rendering enemy sword bounding box\n";
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

    // --- HUD: Black Bar with Score, Time-of-Day, and Health ---
    m_game.window().setView(defaultView);
    {
        // Create a black rectangle at the bottom (height = 50)
        sf::RectangleShape hudBar(sf::Vector2f(static_cast<float>(windowSize.x), 50.f));
        hudBar.setFillColor(sf::Color::Black);
        hudBar.setPosition(0, windowSize.y - 50.f);
        m_game.window().draw(hudBar);

        // Score text (left-aligned)
        sf::Text scoreText;
        scoreText.setFont(m_game.assets().getFont("Menu"));
        scoreText.setCharacterSize(20);
        scoreText.setFillColor(sf::Color::White);
        scoreText.setString("Score: " + std::to_string(m_score));
        scoreText.setPosition(10.f, windowSize.y - 40.f);

        // Time-of-day text (centered)
        sf::Text timeText;
        timeText.setFont(m_game.assets().getFont("Menu"));
        timeText.setCharacterSize(20);
        timeText.setFillColor(sf::Color::White);
        timeText.setString(m_timeofday);
        sf::FloatRect timeRect = timeText.getLocalBounds();
        timeText.setPosition(windowSize.x / 2.f - timeRect.width / 2.f, windowSize.y - 40.f);

        // Health text (right-aligned)
        int currentHealth = 100;
        int maxHealth = 100;
        auto players = m_entityManager.getEntities("player");
        if (!players.empty() && players[0]->has<CHealth>()) {
            auto& healthComp = players[0]->get<CHealth>();
            currentHealth = healthComp.currentHealth;
            maxHealth = healthComp.maxHealth;
        }
        sf::Text healthText;
        healthText.setFont(m_game.assets().getFont("Menu"));
        healthText.setCharacterSize(20);
        healthText.setFillColor(sf::Color::White);
        healthText.setString("HP: " + std::to_string(currentHealth) + " / " + std::to_string(maxHealth));
        sf::FloatRect healthRect = healthText.getLocalBounds();
        healthText.setPosition(windowSize.x - healthRect.width - 10.f, windowSize.y - 40.f);

        // Draw HUD texts
        m_game.window().draw(scoreText);
        m_game.window().draw(timeText);
        m_game.window().draw(healthText);
    }

    m_game.window().setView(m_cameraView);
    m_game.window().display();
}

void PlayRenderer::drawGrid() {
    int windowHeight = m_game.window().getSize().y;
    const int gridSize = 96;
    const int worldWidth = 50;
    const int worldHeight = 20;
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
