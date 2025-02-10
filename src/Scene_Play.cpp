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

// Define internal constants for clarity and easy adjustments
namespace {
    constexpr float MAX_FALL_SPEED          = 1000.f;
    constexpr float FRAGMENT_SIZE           = 16.f;
}

Scene_Play::Scene_Play(GameEngine& game, const std::string& levelPath)
    : Scene(game),  // Inizializza la base class Scene (che non ha costruttore di default)
    m_levelPath(levelPath),                                   // (1)
    m_entityManager(),                                        // (2)
    m_lastDirection(1.f),                                     // (3)
    m_animationSystem(game, m_entityManager, m_lastDirection),  // (4)
    m_playRenderer(game, m_entityManager, m_backgroundSprite, m_backgroundTexture, m_cameraView), // (5)
    m_backgroundTexture(),                                    // (6)
    m_backgroundSprite(),                                     // (7)
    m_game(game),                                             // (8)
    m_gameOver(false),                                        // (9)
    m_levelLoader(game),                                      // (10)
    m_showBoundingBoxes(false),                               // (11)
    m_showGrid(false),                                        // (12)
    m_backgroundPath(""),                                     // (13) – oppure un valore di default
    m_timeofday(""),                                          // (14) – oppure "Day" se preferisci
    m_cameraView(game.window().getDefaultView()),             // (15)
    m_score(0),
    m_movementSystem(game, m_entityManager, m_cameraView, m_lastDirection),
    m_enemyAISystem(game, m_entityManager),
    m_spawner(game, m_entityManager),
    m_swordCollisionSystem(game, m_entityManager)
{
    // Aggiorna la vista con quella predefinita della finestra
    m_cameraView = m_game.window().getDefaultView();

    // Seleziona uno sfondo casuale
    selectRandomBackground();

    // Carica la texture dello sfondo
    if (!m_backgroundTexture.loadFromFile(m_backgroundPath)) {
        std::cerr << "Error: Could not load background image: " << m_backgroundPath << std::endl;
    }
    std::cout << "[DEBUG] Loaded background image: " << m_backgroundPath << std::endl;

    // Abilita il tiling dello sfondo
    m_backgroundTexture.setRepeated(true);

    // Imposta la texture sullo sprite dello sfondo
    m_backgroundSprite.setTexture(m_backgroundTexture);

    // Centra la camera
    sf::Vector2u windowSize = m_game.window().getSize();
    m_cameraView.setCenter(sf::Vector2f(windowSize.x / 2, windowSize.y / 2));
    m_cameraView.zoom(1.5f);
    m_game.window().setView(m_cameraView);

    init();
}
void Scene_Play::selectRandomBackground() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<int> dist(0, 2);

    int selection = dist(gen);

    // Randomly select one of the three backgrounds
    switch (selection) {
        case 0:
            m_backgroundPath = "src/images/day1.png";
            m_timeofday = "Day";
            break;
        case 1:
            m_backgroundPath = "src/images/night1.png";
            m_timeofday = "Night";
            break;
        case 2:
            m_backgroundPath = "src/images/sunset1.png";
            m_timeofday = "Sunset";
            break;
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
        sMovement(deltaTime);
        sEnemyAI(deltaTime);
        sCollision();
        sAnimation(deltaTime);
        updateFragments(deltaTime);

        // Aggiorna i timer per CHealth e CState
        for (auto& entity : m_entityManager.getEntities()) {
            if (entity->has<CHealth>()) {
                entity->get<CHealth>().update(deltaTime);
            }
            if (entity->has<CState>()) {
                entity->get<CState>().update(deltaTime);
            }
            // Aggiorna CLifeSpan solo per entità effimere (sword, enemySword, fragment)
            std::string tag = entity->tag();
            if ((tag == "sword" || tag == "enemySword" || tag == "fragment") && entity->has<CLifeSpan>()) {
                auto& lifespan = entity->get<CLifeSpan>();
                lifespan.remainingTime -= deltaTime;
                if (lifespan.remainingTime <= 0.f) {
                    entity->destroy();
                }
            }
        }

        // Aggiorna le collisioni delle spade
        m_swordCollisionSystem.updateSwordCollisions(deltaTime);

        // Verifica la morte degli enemy
        lifeCheckEnemyDeath();

        // Controlla se il giocatore è morto o fuori limite
        lifeCheckPlayerDeath();
    } else {
        m_game.window().setView(m_game.window().getDefaultView());
        m_game.changeScene("GAMEOVER", std::make_shared<Scene_GameOver>(m_game));
    }
    sRender();
}
//
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
// Fragment Update (for brick break effect)
//
void Scene_Play::updateFragments(float deltaTime)
{
    for (auto& fragment : m_entityManager.getEntities("fragment"))
    {
        auto& transform = fragment->get<CTransform>();
        auto& anim      = fragment->get<CAnimation>(); // Brick texture
        auto& lifespan  = fragment->get<CLifeSpan>();

        // Move fragment
        transform.pos += transform.velocity * deltaTime;

        // Fade out by reducing alpha
        sf::Color color = anim.animation.getMutableSprite().getColor();
        float alpha = (lifespan.remainingTime / lifespan.totalTime) * 255.0f;
        color.a = static_cast<int>(std::max(0.0f, alpha));
        anim.animation.getMutableSprite().setColor(color);

        // Destroy when fully faded
        lifespan.remainingTime -= deltaTime;
        if (lifespan.remainingTime <= 0.0f) {
            fragment->destroy();
        }
    }
}

//
// Create Brick Fragments (on brick break)
//
// Nuova funzione per creare le particelle in base al tipo di blocco (Brick o Box)
void Scene_Play::createBlockFragments(Vec2<float> position, const std::string & blockType)
{
    const float spreadSpeed = 400.f;

    std::vector<Vec2<float>> directions = {
        {-1, -1}, {0, -1}, {1, -1},  // Up-Left, Up, Up-Right
        {-1,  0}, {1,  0},            // Left, Right
        {-1,  1}, {0,  1}, {1,  1}     // Down-Left, Down, Down-Right
    };

    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<int> angleDist(0, 359);
    std::uniform_int_distribution<int> rotationSpeedDist(0, 199);

    for (auto dir : directions)
    {
        auto fragment = m_entityManager.addEntity("fragment");

        // Imposta la posizione e la velocità
        fragment->add<CTransform>(position, Vec2<float>(dir.x * spreadSpeed, dir.y * spreadSpeed));

        // Usa l'animazione corretta in base al tipo di blocco (Brick o Box)
        if (m_game.assets().hasAnimation(blockType)) {
            const Animation& anim = m_game.assets().getAnimation(blockType);
            fragment->add<CAnimation>(anim, false);

            // Scala la particella
            sf::Sprite& sprite = fragment->get<CAnimation>().animation.getMutableSprite();
            sf::Vector2i textureSize = anim.getSize();
            float scaleX = FRAGMENT_SIZE / static_cast<float>(textureSize.x);
            float scaleY = FRAGMENT_SIZE / static_cast<float>(textureSize.y);
            sprite.setScale(scaleX, scaleY);
        }
        else {
            std::cerr << "[ERROR] Missing animation for fragments: " << blockType << "\n";
        }

        // Applica una rotazione casuale
        fragment->add<CRotation>(angleDist(gen), rotationSpeedDist(gen));

        // Imposta una breve durata (0.6 secondi per il fade-out)
        fragment->add<CLifeSpan>(0.6f);
    }
}

//
// Collision Handling
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
            // Player must be on ground AND in idle/run/attack if you want jump from attack stance
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
