#include "Scene_Play.h"
#include "Scene_Menu.h"
#include "Scene_GameOver.h"
#include "Physics.hpp"
#include "Entity.hpp"
#include "Components.hpp"
#include "Animation.hpp"
#include <iostream>
#include <fstream>
#include <random>
#include <algorithm>
#include "systems/PlayRenderer.h"
#include "systems/AnimationSystem.h"
#include <cmath>
#include "systems/CollisionSystem.h"
#include "systems/SpriteUtils.h"

void Scene_Play::initializeCamera()
{
    // 1) Start with the window's default view
    m_cameraView = m_game.window().getDefaultView();

    // 2) Get window size
    sf::Vector2u windowSize = m_game.window().getSize();

    // 3) Get the background texture size (assuming m_backgroundTexture is loaded)
    sf::Vector2u bgSize = m_backgroundTexture.getSize();
    if (bgSize.x == 0 || bgSize.y == 0) {
        // Safety check: if the texture didn't load properly, just return
        return;
    }

    // 4) Center the camera on the middle of the background
    float centerX = static_cast<float>(bgSize.x) / 2.f;
    float centerY = static_cast<float>(bgSize.y) / 2.f;
    m_cameraView.setCenter(centerX, centerY);

    m_cameraView.zoom(1.5f);

    // 6) Apply the view to the window
    m_game.window().setView(m_cameraView);
}
// Then call it from the constructor:
Scene_Play::Scene_Play(GameEngine& game, const std::string& levelPath)
    : Scene(game),
      m_levelPath(levelPath),
      m_entityManager(),
      m_lastDirection(1.f),
      m_animationSystem(game, m_entityManager, m_lastDirection),
      m_playRenderer(game, m_entityManager, m_backgroundSprite, m_backgroundTexture, m_cameraView),
      m_backgroundTexture(),
      m_backgroundSprite(),
      m_game(game),
      m_gameOver(false),
      m_levelLoader(game),
      m_showBoundingBoxes(false),
      m_showGrid(false),
      m_backgroundPath(""),
      m_timeofday(""),
      m_cameraView(game.window().getDefaultView()),
      m_score(0),
      m_movementSystem(game, m_entityManager, m_cameraView, m_lastDirection),
      m_spawner(game, m_entityManager),
      m_enemyAISystem(m_entityManager, m_spawner, m_game)
{
    selectBackgroundFromLevel(m_levelPath);

    if (!m_backgroundTexture.loadFromFile(m_backgroundPath)) {
        std::cerr << "Error: Could not load background image: " << m_backgroundPath << std::endl;
    } else {
        m_backgroundTexture.setRepeated(true);
        m_backgroundSprite.setTexture(m_backgroundTexture);
    }

    // Initialize camera after loading background
    initializeCamera();

    init();
}

void Scene_Play::selectBackgroundFromLevel(const std::string& levelPath) {
    // Extract level name from path
    std::string levelName = levelPath.substr(levelPath.find_last_of("/\\") + 1);
    
    // Mapping of levels to backgrounds
    if (levelName == "ancient_rome_level_1_day.txt") {
        m_backgroundPath = "src/images/Background/ancient_rome_level_1_day.png";
        m_timeofday = "ANCIENT ROME (DAY)";
    } else if (levelName == "ancient_rome_level_2_sunset.txt") {
        m_backgroundPath = "src/images/Background/ancient_rome_level_2_sunset.png";
        m_timeofday = "ANCIENT ROME (NIGHT)";
    } else if (levelName == "ancient_rome_level_3_night.txt") {
        m_backgroundPath = "src/images/Background/ancient_rome_level_3_night.png";
        m_timeofday = "ANCIENT ROME (SUNSET)";
    } else if (levelName == "ancient_rome_level_4_emperor_room.txt") {
        m_backgroundPath = "src/images/Background/ancient_rome_level_4_emperor_room.png";
        m_timeofday = "EMPEROR ROOM";
    }  
    else {
        // Default background if level is not mapped
        m_backgroundPath = "src/images/Background/default.png";
        m_timeofday = "Unknown";
    }
}

void Scene_Play::init()
{
    registerCommonActions();
    registerAction(sf::Keyboard::T, "TOGGLE_TEXTURE");
    registerAction(sf::Keyboard::A, "MOVE_LEFT");
    registerAction(sf::Keyboard::D, "MOVE_RIGHT");
    registerAction(sf::Keyboard::W, "JUMP");
    registerAction(sf::Keyboard::Escape, "QUIT");
    registerAction(sf::Keyboard::Space, "ATTACK");
    registerAction(sf::Keyboard::G, "TOGGLE_GRID");
    registerAction(sf::Keyboard::B, "TOGGLE_BB");

    // Utilizza il membro m_levelLoader già inizializzato:
    m_levelLoader.load(m_levelPath, m_entityManager);
}

//
// Main Update Function
//
void Scene_Play::update(float deltaTime) {
    if (!m_gameOver) {
        m_entityManager.update();
        
        // Update immediate timers (invulnerability, knockback, etc.) before collisions.
        for (auto& entity : m_entityManager.getEntities()) {
            if (entity->has<CHealth>())
                entity->get<CHealth>().update(deltaTime);
            if (entity->has<CState>())
                entity->get<CState>().update(deltaTime);
        }
        
        sMovement(deltaTime);
        sEnemyAI(deltaTime);
        sCollision();  // Calls updateCollisions() which now includes sword collisions.
        sAnimation(deltaTime);
        UpdateFragments(deltaTime);
        
        // Update CLifeSpan for ephemeral entities AFTER collision processing.
        for (auto& entity : m_entityManager.getEntities()) {
            std::string tag = entity->tag();
            if ((tag == "sword" || tag == "enemySword" || tag == "fragment") && entity->has<CLifeSpan>()) {
                auto& lifespan = entity->get<CLifeSpan>();
                lifespan.remainingTime -= deltaTime;
                if (lifespan.remainingTime <= 0.f)
                    entity->destroy();
            }
        }
        
        lifeCheckEnemyDeath();
        lifeCheckPlayerDeath();
    } else {
        m_game.window().setView(m_game.window().getDefaultView());
        m_game.changeScene("GAMEOVER", std::make_shared<Scene_GameOver>(m_game));
    }
    sRender();
}
// Rendering
//

void Scene_Play::sRender() {
    // Aggiorna eventuali impostazioni di rendering se necessario:
    m_playRenderer.setShowGrid(m_showGrid);
    m_playRenderer.setShowBoundingBoxes(m_showBoundingBoxes);
    m_playRenderer.setScore(m_score);
    m_playRenderer.setTimeOfDay(m_timeofday);

    // Esegui il rendering tramite PlayRenderer
    m_playRenderer.render();
}

//
// Movement and Camera Update
//
void Scene_Play::sMovement(float deltaTime) {
    m_movementSystem.update(deltaTime);
}
//
// Animation Updates
//
void Scene_Play::sAnimation(float deltaTime)
{
    m_animationSystem.update(deltaTime);
}

//
//Collision Handliong
//
void Scene_Play::sCollision() {
    CollisionSystem collisionSystem(m_entityManager, m_game, &m_spawner);
    collisionSystem.updateCollisions();
}

// Action Processing (Input Handling)
//
void Scene_Play::sDoAction(const Action& action)
{
    auto playerEntities = m_entityManager.getEntities("player");
    if (playerEntities.empty()) return;

    auto player = playerEntities[0];
    auto& vel   = player->get<CTransform>().velocity;
    auto& state = player->get<CState>();

    static bool isMovingLeft  = false;
    static bool isMovingRight = false;

    if (action.type() == "START") {
        if (action.name() == "MOVE_LEFT") {
            isMovingLeft = true;
            isMovingRight = false;
            vel.x = -xSpeed;
            if (state.state != "air")
                state.state = "run";
        }
        else if (action.name() == "MOVE_RIGHT") {
            isMovingRight = true;
            isMovingLeft = false;
            vel.x = xSpeed;
            if (state.state != "air")
                state.state = "run";
        }
        else if (action.name() == "TOGGLE_GRID")
        {
            m_showGrid = !m_showGrid;
        }
        else if (action.name() == "TOGGLE_BB")
        {
            m_showBoundingBoxes = !m_showBoundingBoxes;
        }
        else if (action.name() == "ATTACK") {
            // Attacca solo se il cooldown è terminato
            if (state.attackCooldown <= 0.f) {
                state.state = "attack";
                state.attackTime = 0.2f;      // Durata dell'attacco (animazione)
                state.attackCooldown = 0.5f;  // Imposta il cooldown di attacco (0.5 secondi)
                spawnSword(player);           // Spawna la spada
            }
        }
        else if (action.name() == "JUMP") {
            // Player must abe on ground AND in idle/run/attack if you want jump from attack stance
            if (state.onGround && (state.state == "idle" || state.state == "run" || state.state == "attack")) {
                state.isJumping = true;
                state.jumpTime  = 0.0f;
                vel.y           = -ySpeed;
                state.state     = "air";
            }
        }
    }
    else if (action.type() == "END") {
        if (action.name() == "MOVE_LEFT") {
            isMovingLeft = false;
            if (!isMovingRight) {
                vel.x = 0.f;
                if (state.state != "air")
                    state.state = "idle";
            }
        }
        else if (action.name() == "MOVE_RIGHT") {
            isMovingRight = false;
            if (!isMovingLeft) {
                vel.x = 0.f;
                if (state.state != "air")
                    state.state = "idle";
            }
        }
        else if (action.name() == "JUMP") {
            state.isJumping = false;
        }
    }
}

// Wrapper: chiama il metodo updateFragments del Spawner
void Scene_Play::UpdateFragments(float deltaTime) {
    m_spawner.updateFragments(deltaTime);
}

//
// Check for Player Death
//
void Scene_Play::lifeCheckPlayerDeath() {
    auto players = m_entityManager.getEntities("player");
    if (players.empty()) {
        return;
    }

    auto& player = players.front();
    const auto& transform = player->get<CTransform>();
    const auto& health = player->get<CHealth>();

    bool isOutOfBounds = transform.pos.y > 1000;
    bool isDead = (health.currentHealth <= 0);

    if (isOutOfBounds || isDead) {
        m_gameOver = true;
        m_game.changeScene("GAMEOVER", std::make_shared<Scene_GameOver>(m_game));
    }
}
void Scene_Play::lifeCheckEnemyDeath() {
    auto enemies = m_entityManager.getEntities("enemy");
    for (auto& enemy : enemies) {
        // Se l'enemy non ha il componente CHealth, salta (oppure gestisci in altro modo)
        if (!enemy->has<CHealth>()) continue;

        const auto& transform = enemy->get<CTransform>();
        const auto& health = enemy->get<CHealth>();

        // Imposta una soglia: ad esempio, se l'enemy cade oltre y > 1000 o sale troppo in alto (y < -100)
        bool isOutOfBounds = (transform.pos.y > 1000) || (transform.pos.y < -100);
        bool isDead = (health.currentHealth <= 0);

        if (isOutOfBounds || isDead) {
            enemy->destroy();
            std::cout << "[DEBUG] Enemy destroyed: ID = " << enemy->id() << "\n";
        }
    }
}
std::shared_ptr<Entity> Scene_Play::spawnSword(std::shared_ptr<Entity> player) {
    return m_spawner.spawnSword(player);
}

std::shared_ptr<Entity> Scene_Play::spawnItem(Vec2<float> position, const std::string& tileType) {
    return m_spawner.spawnItem(position, tileType);
}

std::shared_ptr<Entity> Scene_Play::spawnEnemySword(std::shared_ptr<Entity> enemy) {
    return m_spawner.spawnEnemySword(enemy);
}

void Scene_Play::sEnemyAI(float deltaTime) {
    m_enemyAISystem.update(deltaTime);
}

bool Scene_Play::isObstacleInFront(Vec2<float> enemyPos, float direction)
{
    Vec2<float> checkPos = enemyPos + Vec2<float>(direction * 50.f, 0.f);
    for (auto& tile : m_entityManager.getEntities("tile")) {
        auto& tileTrans = tile->get<CTransform>();
        sf::FloatRect tileBB = tile->get<CBoundingBox>().getRect(tileTrans.pos);

        if (tileBB.contains(checkPos.x, checkPos.y)) {
            return true; // Obstacle detected
        }
    }
    return false;
}
