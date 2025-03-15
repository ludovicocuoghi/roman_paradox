#pragma once

#include "Scene.h"
#include "GameEngine.h"
#include "Scene_GameOver.h"
#include "Entity.hpp"
#include "EntityManager.hpp"
#include <string>
#include <memory>
#include <SFML/Graphics.hpp>
#include "systems/LoadLevel.h"
#include "systems/PlayRenderer.h"
#include "systems/AnimationSystem.h"
#include "systems/MovementSystem.h"
#include "systems/EnemyAISystem.h"
#include "systems/Spawner.h"
#include "systems/DialogueSystem.h"


class Scene_Play : public Scene {
public:
    // Constructor: Pass a reference to the game engine and the level file path.
    Scene_Play(GameEngine& game, const std::string& levelPath);

    // Scene lifecycle and initialization functions
    void initializeCamera();
    void init();
    void loadLevel(const std::string& levelPath);
    void selectRandomTimeOfDay();
    void onEnter();
    void toggleBoundingBoxes() { m_showBoundingBoxes = !m_showBoundingBoxes; }

    // Main update and game logic functions
    void sMovement(float deltaTime);
    void sCollision();
    void sAnimation(float deltaTime);
    void sAmmoSystem(float deltaTime);
    void sDoAction(const Action& action) override;
    void update(float deltaTime) override;

    std::shared_ptr<Entity> m_activeSword = nullptr;

    std::shared_ptr<Entity> spawnSword(std::shared_ptr<Entity> player);
    std::shared_ptr<Entity> spawnEnemySword(std::shared_ptr<Entity> enemy);
    std::shared_ptr<Entity> spawnItem(Vec2<float> position, const std::string& tileType);

    // Add these function declarations to your class
    void initializeDialogues();


    void applyKnockback(std::shared_ptr<Entity> entity, Vec2<float> hitDirection, float duration);

    // Utility functions for game logic
    bool isPathClear(Vec2<float> start, Vec2<float> end);
    bool isObstacleInFront(Vec2<float> enemyPos, float direction);
    void UpdateFragments(float deltaTime);
    void lifeCheckPlayerDeath();
    void lifeCheckEnemyDeath();
    void showGameOverScreen();
    void restartLevel();
    void goToMenu();
    void sUpdateSword();
    void sLifespan(float deltaTime);
    void sEnemyAI(float deltaTime);
    void selectBackgroundFromLevel(const std::string& levelPath);

    // sRender() now delegates all drawing to PlayRenderer
    void sRender() override;

    std::string extractLevelName(const std::string& path);
    void removeTileByID(const std::string& tileID);
    void createTile(const std::string& tileType, int gridX, int gridY);
    void updateBurstFire(float deltaTime);
    void handleEmperorDeath(std::shared_ptr<Entity> emperor);

    // --- Configuration Constants
    const float gravityVal = 1000.f;
    const float xSpeed     = 350.f;
    const float ySpeed     = 350.f;
    const float playerBBsize = 80.f;
    const int gridSize     = 96;
    const float ATTACK_DURATION = 0.1f;
    const float CAMERA_Y_OFFSET = 1300;
    const float MAX_DEFENSE_TIME = 2.0f;
    const float CAMERA_ZOOM = 1.3f;

    bool m_firstCameraUpdate = true;
    void selectRandomBackground();

private:
    std::string m_levelPath;              // (1)
    EntityManager m_entityManager;        // (2)
    float m_lastDirection = 1.f;          // (3) 
    AnimationSystem m_animationSystem;    // (4)
    PlayRenderer m_playRenderer;          // (5)
    sf::Texture m_backgroundTexture;      // (6)
    sf::Sprite m_backgroundSprite;        // (7)
    GameEngine& m_game;                   // (8)
    bool m_gameOver = false;              // (9)
    LoadLevel m_levelLoader;              // (10)
    bool m_showBoundingBoxes = false;     // (11)
    bool m_showGrid = false;              // (12)
    std::string m_backgroundPath;         // (13)
    std::string m_timeofday;              // (14)
    sf::View m_cameraView;                // (15)
    int m_score = 0;                      // (16)
    MovementSystem m_movementSystem;
    Spawner m_spawner;
    EnemyAISystem m_enemyAISystem;
    
    // Dialogue system (new)
    std::shared_ptr<DialogueSystem> m_dialogueSystem;
};
