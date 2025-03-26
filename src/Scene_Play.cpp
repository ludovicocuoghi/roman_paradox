#include "Scene_Play.h"
#include "Scene_Menu.h"
#include "Scene_GameOver.h"
#include "Scene_StoryText.h"
#include "Physics.hpp"
#include "Entity.hpp"
#include "Components.hpp"
#include "Animation.hpp"
#include <iostream>
#include <fstream>
#include <random>
#include <filesystem>
#include <algorithm>
#include "systems/PlayRenderer.h"
#include "systems/AnimationSystem.h"
#include <cmath>
#include "systems/CollisionSystem.h"
#include "systems/SpriteUtils.h"
#include "systems/Spawner.h"
#include "ResourcePath.h"

//Booleans for keypresses, used to make smooth transactions between dialogue and gameplay
bool leftKeyPressed = false;
bool rightKeyPressed = false;
bool wasInDialogue = false;


void Scene_Play::initializeCamera()
{
    // 1) Start with the window's default view
    m_cameraView = m_game.window().getDefaultView();
    sf::Vector2u bgSize = m_backgroundTexture.getSize();
    if (bgSize.x == 0 || bgSize.y == 0) {
        // Safety check: if the texture didn't load properly, just return
        return;
    }

    // 4) Center the camera on the middle of the background
    float centerX = static_cast<float>(bgSize.x) / 2.f;
    float centerY = static_cast<float>(bgSize.y) / 2.f - CAMERA_Y_OFFSET;
    m_cameraView.setCenter(centerX, centerY);

    m_cameraView.zoom(Scene_Play::CAMERA_ZOOM);

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
      m_playRenderer(game, m_entityManager, m_backgroundSprite, m_backgroundTexture, m_cameraView, m_score),
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
      m_enemyAISystem(m_entityManager, m_spawner, m_game),
      m_language(game.getLanguage())
{
    std::cout << "[DEBUG] Scene_Play constructor: levelPath = " << levelPath << std::endl;

    if (m_levelPath.empty()) {
        std::cerr << "[ERROR] Scene_Play received an empty level path!" << std::endl;
        return;
    }

    selectBackgroundFromLevel(m_levelPath);
    std::cout << "[DEBUG] Selected background: " << m_backgroundPath << std::endl;

    if (!m_backgroundTexture.loadFromFile(m_backgroundPath)) {
        std::cerr << "[ERROR] Could not load background image: " << m_backgroundPath << std::endl;
    } else {
        m_backgroundTexture.setRepeated(true);
        m_backgroundSprite.setTexture(m_backgroundTexture);
    }

    std::cout << "[DEBUG] Initializing Camera...\n";
    initializeCamera();

    // Add dialogue system initialization here, right after camera initialization
    std::cout << "[DEBUG] Initializing Dialogue System...\n";
    initializeDialogues();

    std::cout << "[DEBUG] Calling init()...\n";
    init();

    std::cout << "[DEBUG] Scene_Play initialized successfully!\n";
}

void Scene_Play::selectBackgroundFromLevel(const std::string& levelPath) {
    // Extract level name from path
    std::string levelName = levelPath.substr(levelPath.find_last_of("/\\") + 1);
    
    // Base path using getResourcePath
    std::string basePath = getResourcePath("images/Background");
    
    // Mapping of levels to backgrounds
    if (levelName == "alien_rome_level_1.txt") {
        m_backgroundPath = basePath + "/alien_rome/alien_rome_phase_1.png";
        m_timeofday = "ALIEN EMPIRE";
    } else if (levelName == "alien_rome_level_2.txt") {
        m_backgroundPath = basePath + "/alien_rome/alien_rome_phase_2.png";
        m_timeofday = "ALIEN EMPIRE";
    } else if (levelName == "ancient_rome_level_1_day.txt") {
        m_backgroundPath = basePath + "/ancient_rome/ancient_rome_level_1_day.png";
        m_timeofday = "ANCIENT ROME (DAY)";
    } else if (levelName == "ancient_rome_level_2_sunset.txt") {
        m_backgroundPath = basePath + "/ancient_rome/ancient_rome_level_2_sunset.png";
        m_timeofday = "ANCIENT ROME (SUNSET)";
    } else if (levelName == "ancient_rome_level_3_night.txt") {
        m_backgroundPath = basePath + "/ancient_rome/ancient_rome_level_3_night.png";
        m_timeofday = "ANCIENT ROME (NIGHT)";
    } else if (levelName == "ancient_rome_level_4_emperor_room.txt") {
        m_backgroundPath = basePath + "/ancient_rome/ancient_rome_level_4_emperor_room.png";
        m_timeofday = "EMPEROR ROOM";
    } else if (levelName == "ancient_rome_level_5_day_v2.txt") {
        m_backgroundPath = basePath + "/ancient_rome/ancient_rome_level_1_day.png";
        m_timeofday = "ANCIENT ROME (DAY 2)";
    } else if (levelName == "future_rome_level_1.txt") {
        m_backgroundPath = basePath + "/future_rome/morning_future_3.png";
        m_timeofday = "FUTURE ROME (DAY)";
    } else if (levelName == "future_rome_level_2.txt") {
        m_backgroundPath = basePath + "/future_rome/future_sunset3.png";
        m_timeofday = "FUTURE ROME (SUNSET)";
    } else if (levelName == "future_rome_level_3.txt") {
        m_backgroundPath = basePath + "/future_rome/future_rome_night.png";
        m_timeofday = "FUTURE ROME (NIGHT)";
    } else if (levelName == "future_rome_level_4_emperor_room.txt") {
        m_backgroundPath = basePath + "/future_rome/emperor_room.png";
        m_timeofday = "FUTURE EMPEROR ROOM";
    } else if (levelName == "future_rome_level_5_day_v2.txt") {
        m_backgroundPath = basePath + "/future_rome/morning_future_3.png";
        m_timeofday = "FUTURE ROME (DAY)";
    }  
    else {
        // Default background if level is not mapped
        m_backgroundPath = getResourcePath("images") + "/Background/default.png";
        m_timeofday = "Unknown";
    }
    
    // Add debug output to verify the path
    std::cout << "[DEBUG] Background path set to: " << m_backgroundPath << std::endl;
    
    // Check if the file exists
    std::ifstream file(m_backgroundPath);
    if (!file.good()) {
        std::cerr << "[ERROR] Background image does not exist: " << m_backgroundPath << std::endl;
    } else {
        std::cout << "[DEBUG] Background image file exists." << std::endl;
    }
}

void Scene_Play::init()
{
    std::cout << "[DEBUG] Scene_Play::init() - Start\n";

    //Reset movement keys
    leftKeyPressed = false;
    rightKeyPressed = false;
    wasInDialogue = false;

    registerCommonActions();
    registerAction(sf::Keyboard::A, "MOVE_LEFT");
    registerAction(sf::Keyboard::D, "MOVE_RIGHT");
    registerAction(sf::Keyboard::W, "JUMP");
    registerAction(sf::Keyboard::Escape, "QUIT");
    registerAction(sf::Keyboard::Space, "ATTACK");
    registerAction(sf::Keyboard::M, "DEFENSE");
    registerAction(sf::Keyboard::G, "TOGGLE_GRID");
    registerAction(sf::Keyboard::B, "TOGGLE_BB");
    registerAction(sf::Keyboard::Enter, "SUPERMOVE");

    std::cout << "[DEBUG] Scene_Play::init() - Loading level: " << m_levelPath << std::endl;

    if (m_levelPath.empty()) {
        std::cerr << "[ERROR] Cannot load level: Level path is empty!\n";
        return;
    }

    if (!std::filesystem::exists(m_levelPath)) {
        std::cerr << "[ERROR] Level file does not exist: " << m_levelPath << std::endl;
        return;
    }

    //Ensure entity manager is clean before loading
    m_entityManager = EntityManager();

    m_game.setCurrentLevel(m_levelPath);

    std::cout << "[DEBUG] Scene_Play::init() - Calling m_levelLoader.load()\n";
    m_levelLoader.load(m_levelPath, m_entityManager);
    std::cout << "[DEBUG] Scene_Play::init() - Level loaded successfully!\n";
}//
// Main Update Function
//
void Scene_Play::update(float deltaTime)
{
    if (!m_gameOver)
    {
        // Update entity manager
        m_entityManager.update();

        // Update states
        for (auto& entity : m_entityManager.getEntities()) {
            if (entity->has<CHealth>())
                entity->get<CHealth>().update(deltaTime);
            if (entity->has<CState>())
                entity->get<CState>().update(deltaTime);
        }
        
        // Update dialogue system first
        if (m_dialogueSystem) {
            m_dialogueSystem->update(deltaTime);
        }
        auto playerEntities = m_entityManager.getEntities("player");
        if (!playerEntities.empty()) {
            auto player = playerEntities[0];
            if (player->has<CTransform>()) {
                auto& transform = player->get<CTransform>();

                // Debug: Print player's current Y position
                std::cout << "[DEBUG] Player Y position: " << transform.pos.y << " Player X position: " << transform.pos.x << std::endl;


                static float lastVelX = 0.0f;
                float currentVelX = player->get<CTransform>().velocity.x;
                
                // If velocity changed significantly without action
                if (std::abs(lastVelX) > 0.1f && std::abs(currentVelX) < 0.1f) {
                    std::cout << "[DEBUG] Velocity reset unexpectedly from " << lastVelX 
                              << " to " << currentVelX << std::endl;
                }
                
                lastVelX = currentVelX;
            }
        }
        
        // Only process game mechanics if dialogue is not active
        if (!m_dialogueSystem || !m_dialogueSystem->isDialogueActive()) {
            sMovement(deltaTime);
            sEnemyAI(deltaTime);
            sCollision();
            sAnimation(deltaTime);
            UpdateFragments(deltaTime);
            m_spawner.updateGraves(deltaTime);
            sUpdateSword();
            sLifespan(deltaTime);
            sAmmoSystem(deltaTime);
            updateBurstFire(deltaTime);
            
            // Life checks
            lifeCheckEnemyDeath();
            lifeCheckPlayerDeath();
        }
    }
    else
    {
        m_game.window().setView(m_game.window().getDefaultView());
        std::cout << "[DEBUG] Transitioning to GameOver scene with level path: " << m_levelPath << std::endl;
        m_game.changeScene("GAMEOVER", std::make_shared<Scene_GameOver>(m_game, m_levelPath));
    }

    // 3) Finally, render
    sRender();
}
// Rendering
//

void Scene_Play::sRender() {
    // Update rendering settings
    m_playRenderer.setShowGrid(m_showGrid);
    m_playRenderer.setShowBoundingBoxes(m_showBoundingBoxes);
    m_playRenderer.setScore(m_score);
    m_playRenderer.setTimeOfDay(m_timeofday);
    
    // Set dialogue system for rendering
    if (m_dialogueSystem) {
        m_playRenderer.setDialogueSystem(m_dialogueSystem.get());
    }
    
    // Render
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
    CollisionSystem collisionSystem(m_entityManager, m_game, &m_spawner, m_score, m_levelPath);
    collisionSystem.updateCollisions();
}
void Scene_Play::initializeDialogues()
{
    // Create the dialogue system
    m_dialogueSystem = std::make_shared<DialogueSystem>(m_game, m_entityManager);

    m_enemyAISystem.setDialogueSystem(m_dialogueSystem);

    m_dialogueSystem->setLanguage(m_game.getLanguage());
    
    // Add dialogue triggers based on the current level
    std::string levelName = extractLevelName(m_levelPath);
    
    // Base path using getResourcePath
    std::string basePath = getResourcePath("images/Portraits/");

    
    // Setup dialogue triggers based on level
    if (levelName == "alien_rome_level_1.txt") {
        if (m_language == "English") {
            std::vector<DialogueMessage> Dialogue0 = {
                {
                    "Alien Legionary (Comrade)",                               // speaker
                    "HELP!!!",                                       // message
                    basePath + "alien_ancient_right.png",  // portraitPath
                    false,                                           // portraitOnLeft
                    sf::Color::Yellow,                               // speakerColor
                    sf::Color::Red,                                  // messageColor
                    sf::Vector2f(800.f, 450.f),                      // dialogueBoxPosition
                    650.f,                                           // boxWidth - compact for short message
                    150.f,                                           // boxHeight
                    50,                                              // messageFontSize - larger for emphasis
                    true                                            // useTypewriterEffect - immediate display
                },
                {
                    "Alien Legionary (Comrade)",
                    "WE ARE BEING INVADED!!",
                    basePath + "alien_ancient_right.png",
                    false,
                    sf::Color::Yellow,
                    sf::Color::Red,
                    sf::Vector2f(800.f, 450.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    40,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "***GUIDE**",                           // speaker
                    "Press A or D to move LEFT or RIGHT",            // message
                    basePath + "ancient_alien_right.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 200.f),
                    1100.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    40,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "***GUIDE***",                           // speaker
                    "Press W to JUMP",            // message
                    basePath + "ancient_alien_right.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 200.f),
                    1100.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    40,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(100, Dialogue0);
            std::vector<DialogueMessage> Dialogue1 = {
                {
                    "????",                          // speaker
                    "I HAVE FINALLY FOUND YOU!!",            // message
                    basePath + "future_fast.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Cyan,
                    sf::Color::Red,
                    sf::Vector2f(750.f, 450.f),
                    750.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    40,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "????",                               // speaker
                    "YOU DAMNED ALIEN...",            // message
                    basePath + "future_fast.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Cyan,
                    sf::Color::Red,
                    sf::Vector2f(750.f, 450.f),
                    750.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    40,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "????",                               // speaker
                    "I WILL MAKE YOU PAY FOR WHAT YOU DID!!!",            // message
                    basePath + "future_fast.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Cyan,
                    sf::Color::Red,
                    sf::Vector2f(500.f, 450.f),
                    1000.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    40,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "Alien Legionary",                               // speaker
                    "Who are you ?!",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(250.f, 450.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    40,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(1300, Dialogue1);
            std::vector<DialogueMessage> Dialogue2 = {
                {
                    "Alien Legionary",                               // speaker
                    "Have I met him before?",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(50.f, 450.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    35,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(1700, Dialogue2);
            std::vector<DialogueMessage> introDialogue = {
                {
                    "Alien Legionary (Comrade)",                               // speaker
                    "RUN!!!",                                       // message
                    basePath + "alien_ancient_right.png",  // portraitPath
                    false,                                           // portraitOnLeft
                    sf::Color::Yellow,                               // speakerColor
                    sf::Color::Red,                                  // messageColor
                    sf::Vector2f(800.f, 450.f),                      // dialogueBoxPosition
                    650.f,                                           // boxWidth - compact for short message
                    150.f,                                           // boxHeight
                    50,                                              // messageFontSize - larger for emphasis
                    true                                            // useTypewriterEffect - immediate display
                },
                {
                    "Alien Legionary (Comrade)",
                    "THEY ARE INVINCIBLE!!",
                    basePath + "alien_ancient_right.png",
                    false,
                    sf::Color::Yellow,
                    sf::Color::Red,
                    sf::Vector2f(800.f, 450.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    40,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(2000, introDialogue);
            std::vector<DialogueMessage> Dialogue3 = {
                {
                    "Alien Legionary",                               // speaker
                    "Who are these shadow legionnaires???",            // message
                    basePath + "alien_ancient_left.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(150.f, 450.f),
                    850.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "Alien Legionary",                               // speaker
                    "And why are they invading us??",            // message
                    basePath + "alien_ancient_left.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(150.f, 450.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(4300, Dialogue3);
            std::vector<DialogueMessage> Dialogue4 = {
                {
                    "Alien Legionary",                               // speaker
                    "They can destroy everything they hit...",            // message
                    basePath + "alien_ancient_left.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(50.f, 450.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    25,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "Alien Legionary",                               // speaker
                    "I must avoid them at all costs!",            // message
                    basePath + "alien_ancient_left.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(50.f, 450.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    26,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(6500, Dialogue4);
            std::vector<DialogueMessage> Dialogue5 = {
                {
                    "Alien Legionary (Comrade)",                               // speaker
                    "AAAHH!!",            // message
                    basePath + "alien_ancient_right.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Yellow,
                    sf::Color::Red,
                    sf::Vector2f(800.f, 500.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    40,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "Alien Legionary (Comrade)",                               // speaker
                    "HELP!!!",            // message
                    basePath + "alien_ancient_right.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Yellow,
                    sf::Color::Red,
                    sf::Vector2f(800.f, 500.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    40,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(10500, Dialogue5);
        } else {
                std::vector<DialogueMessage> Dialogue0 = {
                    {
                        "エイリアン兵士（仲間）",                               // speaker
                        "助けて！！！",                                       // message
                        basePath + "alien_ancient_right.png",  // portraitPath
                        false,                                           // portraitOnLeft
                        sf::Color::Yellow,                               // speakerColor
                        sf::Color::Red,                                  // messageColor
                        sf::Vector2f(800.f, 450.f),                      // dialogueBoxPosition
                        700.f,                                           // boxWidth - compact for short message
                        150.f,                                           // boxHeight
                        50,                                              // messageFontSize - larger for emphasis
                        true                                            // useTypewriterEffect - immediate display
                    },
                    {
                        "エイリアン兵士（仲間）",
                        "侵略されている！！",
                        basePath + "alien_ancient_right.png",
                        false,
                        sf::Color::Yellow,
                        sf::Color::Red,
                        sf::Vector2f(800.f, 450.f),
                        700.f,                                           // boxWidth - medium for medium message
                        150.f,                                           // boxHeight
                        50,                                              // messageFontSize - still emphasized
                        true                                             // useTypewriterEffect
                    },
                    {
                        "***ガイド***",                              // speaker (could also leave it as GUIDE)
                        "AキーまたはDキーで左右に移動",               // message
                        basePath + "ancient_alien_right.png",        // portraitPath
                        true,                                       // portraitOnLeft
                        sf::Color::Magenta,
                        sf::Color::White,
                        sf::Vector2f(0.f, 200.f),
                        900.f,                                      // boxWidth
                        150.f,                                       // boxHeight
                        50,                                          // messageFontSize
                        true                                         // useTypewriterEffect
                    },
                    {
                        "***ガイド***",
                        "Wキーでジャンプ",
                        basePath + "ancient_alien_right.png",
                        true,
                        sf::Color::Magenta,
                        sf::Color::White,
                        sf::Vector2f(0.f, 200.f),
                        750.f,
                        150.f,
                        50,
                        true
                    }
                };
                m_dialogueSystem->addDialogueTrigger(100, Dialogue0);
                std::vector<DialogueMessage> Dialogue1 = {
                    {
                        "????",                          // speaker
                        "ついに見つけたぞ！！",            // message
                        basePath + "future_fast.png",        // portraitPath
                        false,                                            // portraitOnLeft
                        sf::Color::Cyan,
                        sf::Color::Red,
                        sf::Vector2f(750.f, 450.f),
                        750.f,                                           // boxWidth - medium for medium message
                        150.f,                                           // boxHeight
                        40,                                              // messageFontSize - still emphasized
                        true                                             // useTypewriterEffect
                    },
                    {
                        "????",                               // speaker
                        "貴様、呪われた異星人め...",            // message
                        basePath + "future_fast.png",        // portraitPath
                        false,                                            // portraitOnLeft
                        sf::Color::Cyan,
                        sf::Color::Red,
                        sf::Vector2f(750.f, 450.f),
                        750.f,                                           // boxWidth - medium for medium message
                        150.f,                                           // boxHeight
                        40,                                              // messageFontSize - still emphasized
                        true                                             // useTypewriterEffect
                    },
                    {
                        "????",                               // speaker
                        "お前の所業の報いを受けさせてやる！！！",            // message
                        basePath + "future_fast.png",        // portraitPath
                        false,                                            // portraitOnLeft
                        sf::Color::Cyan,
                        sf::Color::Red,
                        sf::Vector2f(500.f, 450.f),
                        1000.f,                                           // boxWidth - medium for medium message
                        150.f,                                           // boxHeight
                        40,                                              // messageFontSize - still emphasized
                        true                                             // useTypewriterEffect
                    },
                    {
                        "エイリアン兵士",                               // speaker
                        "お前は何者だ？！",            // message
                        basePath + "alien_ancient.png",        // portraitPath
                        true,                                            // portraitOnLeft
                        sf::Color::Magenta,
                        sf::Color::White,
                        sf::Vector2f(250.f, 450.f),
                        650.f,                                           // boxWidth - medium for medium message
                        150.f,                                           // boxHeight
                        40,                                              // messageFontSize - still emphasized
                        true                                             // useTypewriterEffect
                    }
                };
                m_dialogueSystem->addDialogueTrigger(1300, Dialogue1);
                std::vector<DialogueMessage> Dialogue2 = {
                    {
                        "エイリアン兵士",                               // speaker
                        "彼に前に会ったことがあるのか？",            // message
                        basePath + "alien_ancient.png",        // portraitPath
                        true,                                            // portraitOnLeft
                        sf::Color::Magenta,
                        sf::Color::White,
                        sf::Vector2f(50.f, 450.f),
                        650.f,                                           // boxWidth - medium for medium message
                        150.f,                                           // boxHeight
                        35,                                              // messageFontSize - still emphasized
                        true                                             // useTypewriterEffect
                    }
                };
                m_dialogueSystem->addDialogueTrigger(1700, Dialogue2);
                std::vector<DialogueMessage> introDialogue = {
                    {
                        "エイリアン兵士（仲間）",                               // speaker
                        "逃げろ！！！",                                       // message
                        basePath + "alien_ancient_right.png",  // portraitPath
                        false,                                           // portraitOnLeft
                        sf::Color::Yellow,                               // speakerColor
                        sf::Color::Red,                                  // messageColor
                        sf::Vector2f(800.f, 450.f),                      // dialogueBoxPosition
                        650.f,                                           // boxWidth - compact for short message
                        150.f,                                           // boxHeight
                        50,                                              // messageFontSize - larger for emphasis
                        true                                            // useTypewriterEffect - immediate display
                    },
                    {
                        "エイリアン兵士（仲間）",
                        "奴らは無敵だ！！",
                        basePath + "alien_ancient_right.png",
                        false,
                        sf::Color::Yellow,
                        sf::Color::Red,
                        sf::Vector2f(800.f, 450.f),
                        650.f,                                           // boxWidth - medium for medium message
                        150.f,                                           // boxHeight
                        40,                                              // messageFontSize - still emphasized
                        true                                             // useTypewriterEffect
                    }
                };
                m_dialogueSystem->addDialogueTrigger(2000, introDialogue);
            std::vector<DialogueMessage> Dialogue3 = {
                {
                    "エイリアン兵士",
                    "あの黒い兵士たちは何者だ…？",
                    basePath + "alien_ancient_left.png",
                    true,
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(150.f, 450.f),
                    850.f,
                    150.f,
                    30,
                    true
                },
                {
                    "エイリアン兵士",
                    "なぜ我々を襲うんだ…？",
                    basePath + "alien_ancient_left.png",
                    true,
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(150.f, 450.f),
                    700.f,
                    150.f,
                    30,
                    true
                }
            };
            m_dialogueSystem->addDialogueTrigger(4300, Dialogue3);
        
            std::vector<DialogueMessage> Dialogue4 = {
                {
                    "エイリアン兵士",
                    "奴らの攻撃はすべてを破壊する…",
                    basePath + "alien_ancient_left.png",
                    true,
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(50.f, 450.f),
                    700.f,
                    150.f,
                    25,
                    true
                },
                {
                    "エイリアン兵士",
                    "絶対に近づけちゃダメだ！",
                    basePath + "alien_ancient_left.png",
                    true,
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(50.f, 450.f),
                    700.f,
                    150.f,
                    26,
                    true
                }
            };
            m_dialogueSystem->addDialogueTrigger(6500, Dialogue4);
        
            std::vector<DialogueMessage> Dialogue5 = {
                {
                    "エイリアン兵士（仲間）",
                    "うわぁぁぁ！！",
                    basePath + "alien_ancient_right.png",
                    false,
                    sf::Color::Yellow,
                    sf::Color::Red,
                    sf::Vector2f(800.f, 500.f),
                    650.f,
                    150.f,
                    40,
                    true
                },
                {
                    "エイリアン兵士（仲間）",
                    "助けてくれ！！",
                    basePath + "alien_ancient_right.png",
                    false,
                    sf::Color::Yellow,
                    sf::Color::Red,
                    sf::Vector2f(800.f, 500.f),
                    650.f,
                    150.f,
                    40,
                    true
                }
            };
            m_dialogueSystem->addDialogueTrigger(10500, Dialogue5);
        }
    }
    else if (levelName ==  "alien_rome_level_2.txt") {
        if (m_language == "English") {
            std::vector<DialogueMessage> Dialogue0 = {
                {
                    "????????",                               // speaker
                    "The Invaders must..",            // message
                    basePath + "alien_super2.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Red,
                    sf::Color::White,
                    sf::Vector2f(800.f, 300.f),
                    600.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    40,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "????????",                               // speaker
                    "PERISH!!!!!",            // message
                    basePath + "alien_super2.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Red,
                    sf::Color::White,
                    sf::Vector2f(800.f, 300.f),
                    600.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    50,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "Alien Legionary",                               // speaker
                    "Invaders??",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(100.f, 300.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    40,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "Alien Legionary",                               // speaker
                    "What is he talking about??",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(100.f, 300.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    35,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(1450, Dialogue0);
            std::vector<DialogueMessage> Dialogue1 = {
                {
                    "Alien Legionary",                               // speaker
                    "Those things look like black holes...",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(100.f, 300.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    26,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "Alien Legionary",                               // speaker
                    "Could those warriors be coming from the...",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(100.f, 300.f),
                    750.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    26,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "Alien Legionary",                               // speaker
                    "...Future?!",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(100.f, 300.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    60,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(2200, Dialogue1);
            std::vector<DialogueMessage> Dialogue2 = {
                {
                    "Alien Legionary (Comrade)",                               // speaker
                    "I SAW THEM COMING OUT OF THAT BLACK HOLE!!!",            // message
                    basePath + "alien_ancient_right.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Yellow,
                    sf::Color::Red,
                    sf::Vector2f(500.f, 400.f),
                    950.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "Alien Legionary",                               // speaker
                    "I heard black holes allow travel through space-time...",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Yellow,
                    sf::Color::White,
                    sf::Vector2f(0.f, 400.f),
                    1000.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "Alien Legionary",                               // speaker
                    "Could it be the only way to escape??",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 400.f),
                    750.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
            };
            m_dialogueSystem->addDialogueTrigger(11300, Dialogue2);
            std::vector<DialogueMessage> Dialogue3 = {
                {
                    "????????",                               // speaker
                    "DESTROY HIM BEFORE HE ESCAPES!!!",            // message
                    basePath + "alien_super2.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Red,
                    sf::Color::White,
                    sf::Vector2f(0.f, 300.f),
                    850.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    40,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "Alien Legionary",                               // speaker
                    "I have no other choice then!!!",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(150.f, 500.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    35,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "Alien Legionary",                               // speaker
                    "I MUST JUMP IN THE BLACK HOLE!!!!",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(150.f, 500.f),
                    850.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    40,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(13500, Dialogue3);
        } else {
            // Japanese translation
            std::vector<DialogueMessage> Dialogue0 = {
                {
                    "????????",                               // speaker
                    "侵入者どもは...",            // message
                    basePath + "alien_super2.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Red,
                    sf::Color::White,
                    sf::Vector2f(800.f, 300.f),
                    600.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    40,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "????????",                               // speaker
                    "死滅せよ！！！！！",            // message
                    basePath + "alien_super2.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Red,
                    sf::Color::White,
                    sf::Vector2f(800.f, 300.f),
                    600.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    50,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "エイリアン兵士",                               // speaker
                    "侵入者？？",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(100.f, 300.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    40,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "エイリアン兵士",                               // speaker
                    "彼は何を言っているんだ？？",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(100.f, 300.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    35,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(1450, Dialogue0);
            std::vector<DialogueMessage> Dialogue1 = {
                {
                    "エイリアン兵士",                               // speaker
                    "あれはブラックホールのように見えるが...",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(100.f, 300.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    26,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "エイリアン兵士",                               // speaker
                    "あの戦士たちは...",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(100.f, 300.f),
                    750.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    26,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "エイリアン兵士",                               // speaker
                    "...未来から来たのか！？",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(100.f, 300.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    60,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(2200, Dialogue1);
            std::vector<DialogueMessage> Dialogue2 = {
                {
                    "エイリアン兵士（仲間）",                               // speaker
                    "奴らがブラックホールから出てくるのを見たぞ！！！",            // message
                    basePath + "alien_ancient_right.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Yellow,
                    sf::Color::Red,
                    sf::Vector2f(500.f, 400.f),
                    950.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "エイリアン兵士",                               // speaker
                    "ブラックホールは時空を超える通路だと聞いたことがある...",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Yellow,
                    sf::Color::White,
                    sf::Vector2f(0.f, 400.f),
                    1000.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "エイリアン兵士",                               // speaker
                    "逃げる唯一の方法かもしれない！？",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 400.f),
                    750.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
            };
            m_dialogueSystem->addDialogueTrigger(11300, Dialogue2);
            std::vector<DialogueMessage> Dialogue3 = {
                {
                    "????????",                               // speaker
                    "奴が逃げる前に破壊しろ！！！",            // message
                    basePath + "alien_super2.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Red,
                    sf::Color::White,
                    sf::Vector2f(0.f, 300.f),
                    850.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    40,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "エイリアン兵士",                               // speaker
                    "他に選択肢はなさそうだ！！！",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(150.f, 500.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    35,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "エイリアン兵士",                               // speaker
                    "ブラックホールに飛び込むしかない！！！！",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(150.f, 500.f),
                    850.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    40,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(13500, Dialogue3);
        }
    }
    else if (levelName ==  "ancient_rome_level_1_day.txt") {
        if (m_language == "English") {
            std::vector<DialogueMessage> Dialogue0 = {
                {
                    "Alien Legionary",                               // speaker
                    "Where am I???",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 400.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    35,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(850, Dialogue0);
            std::vector<DialogueMessage> Dialogue1 = {
                {
                    "Alien Legionary",                               // speaker
                    "After jumping into that black hole..",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 400.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "Alien Legionary",                               // speaker
                    "could it be possible that I moved to the dark warrios..",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 400.f),
                    1000.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "Alien Legionary",                               // speaker
                    "PAST????",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::Red,
                    sf::Vector2f(0.f, 400.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    60,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
            };
            m_dialogueSystem->addDialogueTrigger(1000, Dialogue1);
            std::vector<DialogueMessage> Dialogue2 = {
                {
                    "Alien Legionary",                               // speaker
                    "Oh, there are golden stairs up there..",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(50.f, 400.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    26,                                              // messageFontSize - still emphasized
                
                },
                {
                    "Alien Legionary",                               // speaker
                    "They seem to lead to the black hole..",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(50.f, 400.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    26,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "Alien Legionary",                               // speaker
                    "But how can I get there??",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(50.f, 400.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    26,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
            };
            m_dialogueSystem->addDialogueTrigger(1800, Dialogue2);
            std::vector<DialogueMessage> Dialogue4 = {
                {
                    "Legionary",                               // speaker
                    "WHO ARE YOU??",            // message
                    basePath + "ancient_normal.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Yellow,
                    sf::Color::White,
                    sf::Vector2f(850.f, 450.f),
                    600.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    40,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "Legionary",                               // speaker
                    "GET OUT OF HERE!!!",            // message
                    basePath + "ancient_normal.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Yellow,
                    sf::Color::White,
                    sf::Vector2f(850.f, 450.f),
                    600.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    40,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
            };
            m_dialogueSystem->addDialogueTrigger(2400, Dialogue4);
            std::vector<DialogueMessage> Dialogue5 = {
                {
                    "Legionary",                               // speaker
                    "WE ARE BEING INVADED!",            // message
                    basePath + "ancient_normal.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Yellow,
                    sf::Color::White,
                    sf::Vector2f(850.f, 450.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    40,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "Legionary",                               // speaker
                    "SEND THE REINFORCEMENTS!!!",            // message
                    basePath + "ancient_normal.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Yellow,
                    sf::Color::White,
                    sf::Vector2f(600.f, 450.f),
                    800.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    40,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "Alien Legionary",                               // speaker
                    "Oh no.. is this happening for REAL?!",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(50.f, 450.f),
                    750.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
            };
            m_dialogueSystem->addDialogueTrigger(3000, Dialogue5);
        } else {
            // Japanese translation
            std::vector<DialogueMessage> Dialogue0 = {
                {
                    "エイリアン兵士",                               // speaker
                    "私はどこにいる？？？",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 400.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    35,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(850, Dialogue0);
            std::vector<DialogueMessage> Dialogue1 = {
                {
                    "エイリアン兵士",                               // speaker
                    "ブラックホールに飛び込んだ後...",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 400.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "エイリアン兵士",                               // speaker
                    "もしかして暗黒の戦士たちの...",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 400.f),
                    1000.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "エイリアン兵士",                               // speaker
                    "過去！？",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::Red,
                    sf::Vector2f(0.f, 400.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    60,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
            };
            m_dialogueSystem->addDialogueTrigger(1000, Dialogue1);
            std::vector<DialogueMessage> Dialogue2 = {
                {
                    "エイリアン兵士",                               // speaker
                    "おっ、あそこに金の階段がある...",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(50.f, 400.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    26,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "エイリアン兵士",                               // speaker
                    "ブラックホールへと続いているようだ...",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(50.f, 400.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    26,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "エイリアン兵士",                               // speaker
                    "どうやって登れるんだ？？",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(50.f, 400.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    26,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
            };
            m_dialogueSystem->addDialogueTrigger(1800, Dialogue2);
            std::vector<DialogueMessage> Dialogue4 = {
                {
                    "兵士",                               // speaker
                    "貴様は何者だ！？",            // message
                    basePath + "ancient_normal.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Yellow,
                    sf::Color::White,
                    sf::Vector2f(850.f, 450.f),
                    600.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    40,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "兵士",                               // speaker
                    "立ち去れ！！！",            // message
                    basePath + "ancient_normal.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Yellow,
                    sf::Color::White,
                    sf::Vector2f(850.f, 450.f),
                    600.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    40,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
            };
            m_dialogueSystem->addDialogueTrigger(2400, Dialogue4);
            std::vector<DialogueMessage> Dialogue5 = {
                {
                    "兵士",                               // speaker
                    "敵襲だ！",            // message
                    basePath + "ancient_normal.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Yellow,
                    sf::Color::White,
                    sf::Vector2f(850.f, 450.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    40,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "兵士",                               // speaker
                    "増援を呼べ！！！",            // message
                    basePath + "ancient_normal.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Yellow,
                    sf::Color::White,
                    sf::Vector2f(600.f, 450.f),
                    800.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    40,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "エイリアン兵士",                               // speaker
                    "まさか...これが現実なのか！？",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(50.f, 450.f),
                    750.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
            };
            m_dialogueSystem->addDialogueTrigger(3000, Dialogue5);
        }
    }
    else if (levelName ==  "ancient_rome_level_2_sunset.txt") {
        if (m_language == "English") {
            std::vector<DialogueMessage> Dialogue1 = {
                {
                    "Alien Legionary",                               // speaker
                    "I think I really am in the past..",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 200.f),
                    800.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    26,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "Alien Legionary",                               // speaker
                    "I guess I should continue exploring..",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 200.f),
                    800.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    26,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "Alien Legionary",                               // speaker
                    "..and find a way to go back to the present",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 200.f),
                    800.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    26,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
            };
            m_dialogueSystem->addDialogueTrigger(150, Dialogue1);
            std::vector<DialogueMessage> Dialogue2 = {
                {
                    "Legionary",                                // speaker
                    "AAAAAH!",            // message
                    basePath + "ancient_normal.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Yellow,
                    sf::Color::White,
                    sf::Vector2f(500.f, 650.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    40,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
            };
            m_dialogueSystem->addDialogueTrigger(850, Dialogue2);
            std::vector<DialogueMessage> Dialogue3 = {
                {
                    "Alien Legionary",                               // speaker
                    "hahaha..",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(100.f, 300.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    35,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "Alien Legionary",                               // speaker
                    "that was funny",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(100.f, 300.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    35,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
            };
            m_dialogueSystem->addDialogueTrigger(1000, Dialogue3);
        }else {
            std::vector<DialogueMessage> Dialogue1 = {
                {
                    "エイリアン兵士",                               // speaker
                    "本当に過去にいるようだ...",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 200.f),
                    800.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    26,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "エイリアン兵士",                               // speaker
                    "探索を続けるべきだな...",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 200.f),
                    800.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    26,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "エイリアン兵士",                               // speaker
                    "..そして現代に戻る方法を見つけないと",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 200.f),
                    800.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    26,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
            };
            m_dialogueSystem->addDialogueTrigger(150, Dialogue1);
            std::vector<DialogueMessage> Dialogue2 = {
                {
                    "兵士",                                // speaker
                    "ぎゃあああぁぁ！",            // message
                    basePath + "ancient_normal.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Yellow,
                    sf::Color::White,
                    sf::Vector2f(500.f, 650.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    40,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
            };
            m_dialogueSystem->addDialogueTrigger(850, Dialogue2);
            std::vector<DialogueMessage> Dialogue3 = {
                {
                    "エイリアン兵士",                               // speaker
                    "wwwww",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(100.f, 300.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    35,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "エイリアン兵士",                               // speaker
                    "今のは笑えるわ",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(100.f, 300.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    35,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
            };
            m_dialogueSystem->addDialogueTrigger(1000, Dialogue3);
        }
    }
    else if (levelName ==  "ancient_rome_level_3_night.txt") {
        if (m_language == "English") {
            std::vector<DialogueMessage> Dialogue0 = {
                {
                    "Legionary (Elite)",                               // speaker
                    "Are you the invader?",            // message
                    basePath + "ancient_elite.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Red,
                    sf::Color::White,
                    sf::Vector2f(700.f, 450.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    35,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "Legionary (Elite)",                                // speaker
                    "How dare you trespass on imperial lands?",            // message
                    basePath + "ancient_elite.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Red,
                    sf::Color::White,
                    sf::Vector2f(700.f, 450.f),
                    800.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "Legionary (Elite)",                              // speaker
                    "We will protect the Emperor..",            // message
                    basePath + "ancient_elite.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Red,
                    sf::Color::White,
                    sf::Vector2f(700.f, 450.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "Legionary (Elite)",                               // speaker
                    "AT ALL COSTS!!",            // message
                    basePath + "ancient_elite.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Red,
                    sf::Color::Red,
                    sf::Vector2f(700.f, 450.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    50,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(200, Dialogue0);
            std::vector<DialogueMessage> Dialogue1 = {
                {
                    "Legionary (Elite)",                               // speaker
                    "STOP THE INVADER!!",            // message
                    basePath + "ancient_elite.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Red,
                    sf::Color::Red,
                    sf::Vector2f(0.f, 400.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    50,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(11000, Dialogue1);
            std::vector<DialogueMessage> Dialogue2 = {
                {
                    "Alien Legionary",                               // speaker
                    "Does this golden portal lead to the Emperor?!",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(750.f, 400.f),
                    750.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    26,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "Alien Legionary",                               // speaker
                    "There's only one way to find out!!",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(750.f, 400.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    26,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }

            };
            m_dialogueSystem->addDialogueTrigger(11200, Dialogue2);
        } else {
            // Japanese translation
            std::vector<DialogueMessage> Dialogue0 = {
                {
                    "兵士（精鋭）",                               // speaker
                    "貴様が侵入者か？",            // message
                    basePath + "ancient_elite.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Red,
                    sf::Color::White,
                    sf::Vector2f(700.f, 450.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    35,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "兵士（精鋭）",                                // speaker
                    "よくも帝国の地を踏む勇気があったな？",            // message
                    basePath + "ancient_elite.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Red,
                    sf::Color::White,
                    sf::Vector2f(700.f, 450.f),
                    800.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "兵士（精鋭）",                              // speaker
                    "我々は皇帝を守る..",            // message
                    basePath + "ancient_elite.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Red,
                    sf::Color::White,
                    sf::Vector2f(700.f, 450.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "兵士（精鋭）",                               // speaker
                    "どんな犠牲を払っても！！",            // message
                    basePath + "ancient_elite.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Red,
                    sf::Color::Red,
                    sf::Vector2f(700.f, 450.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    50,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(200, Dialogue0);
            std::vector<DialogueMessage> Dialogue1 = {
                {
                    "兵士（精鋭）",                               // speaker
                    "侵入者を止めろ！！",            // message
                    basePath + "ancient_elite.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Red,
                    sf::Color::Red,
                    sf::Vector2f(0.f, 400.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    50,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(11000, Dialogue1);
            std::vector<DialogueMessage> Dialogue2 = {
                {
                    "エイリアン兵士",                               // speaker
                    "この黄金の門は皇帝のもとへ続いているのか？！",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(750.f, 400.f),
                    750.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    26,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "エイリアン兵士",                               // speaker
                    "確かめる方法はひとつだ！！",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(750.f, 400.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    26,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
        
            };
            m_dialogueSystem->addDialogueTrigger(11200, Dialogue2);
        }
    }
    else if (levelName ==  "ancient_rome_level_4_emperor_room.txt") {
        if (m_language == "English") {
            std::vector<DialogueMessage> Dialogue0 = {
                {
                    "Emperor",                               // speaker
                    "...And so you eventually got here..",            // message
                    basePath + "ancient_emperor.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Yellow,
                    sf::Color::White,
                    sf::Vector2f(800.f, 500.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "Emperor",                               // speaker
                    "..Alien Invader...",            // message
                    basePath + "ancient_emperor.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Yellow,
                    sf::Color::White,
                    sf::Vector2f(800.f, 500.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    40,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "Emperor",                             // speaker
                    "I am the Emperor of this kingdom...",            // message
                    basePath + "ancient_emperor.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Yellow,
                    sf::Color::White,
                    sf::Vector2f(800.f, 500.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "Emperor",                             // speaker
                    "How dare you come here!?",            // message
                    basePath + "ancient_emperor.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Yellow,
                    sf::Color::White,
                    sf::Vector2f(800.f, 500.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    35,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "Alien Legionary",                               // speaker
                    "I came from the future...!!",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "Alien Legionary",                              // speaker
                    "Your future kingdom invaded my planet",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    800.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "Alien Legionary",                              // speaker
                    "And I came here through a black hole while escaping...",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    1000.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "Emperor",                             // speaker
                    "What are you even talking about?!?",            // message
                    basePath + "ancient_emperor.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Yellow,
                    sf::Color::White,
                    sf::Vector2f(750.f, 500.f),
                    750.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "Emperor",                             // speaker
                    "Black holes?! What nonsense.",            // message
                    basePath + "ancient_emperor.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Yellow,
                    sf::Color::White,
                    sf::Vector2f(800.f, 500.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "Emperor",                               // speaker
                    "YOU WILL PERISH!!!",            // message
                    basePath + "ancient_emperor.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Yellow,
                    sf::Color::Red,
                    sf::Vector2f(800.f, 500.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    50,                                               // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(400, Dialogue0);
            std::vector<DialogueMessage> LowHPAncientEmperorDialogue = {
                {
                    "Emperor",                               // speaker
                    "I have no choice but to show you my true...",                           // message
                    basePath + "ancient_emperor.png", // portraitPath
                    false,                                   // portraitOnLeft
                    sf::Color::Cyan,                         // speakerColor
                    sf::Color::White,                          // messageColor
                    sf::Vector2f(600.f, 550.f),              // dialogueBoxPosition
                    910.f,                                   // boxWidth
                    150.f,                                   // boxHeight
                    30,                                      // messageFontSize
                    true                                     // useTypewriterEffect
                },
                {
                    "Emperor",                               // speaker
                    "POWER!",                           // message
                    basePath + "ancient_emperor.png", // portraitPath
                    false,                                   // portraitOnLeft
                    sf::Color::Cyan,                         // speakerColor
                    sf::Color::Red,                          // messageColor
                    sf::Vector2f(800.f, 550.f),              // dialogueBoxPosition
                    600.f,                                   // boxWidth
                    150.f,                                   // boxHeight
                    60,                                      // messageFontSize
                    true                                     // useTypewriterEffect
                }
            };
            // Add the named dialogues to our system
            m_dialogueSystem->addNamedDialogue("emperor_lowHP", LowHPAncientEmperorDialogue);
            std::vector<DialogueMessage> phasedefeatedAncientDialogue = {
                {
                    "Emperor",                              // speaker
                    "YOU.. DAMNED... ALIEN...",            // message
                    basePath + "ancient_emperor_defeated.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Yellow,
                    sf::Color::White,
                    sf::Vector2f(100.f, 400.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "Alien Legionary",                             // speaker
                    "I can't get close to him...",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(600.f, 400.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "Alien Legionary",                             // speaker
                    "but he seems exhausted...",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(600.f, 400.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "Alien Legionary",                             // speaker
                    "Now's my chance to escape!",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(600.f, 400.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },

            };
            m_dialogueSystem->addNamedDialogue("emperor_ancient_defeated", phasedefeatedAncientDialogue);
            std::vector<DialogueMessage> DialogueFinalEmperorRoom0 = {
                {
                    "Alien Legionary",                               // speaker
                    "Ah! The barrier blocking the way is gone...",            // message
                basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    900.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(4400, DialogueFinalEmperorRoom0);
            std::vector<DialogueMessage> DialogueFinalEmperorRoom1 = {
                {
                    "Alien Legionary",                               // speaker
                    "Lets go back home...",            // message
                basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(6000, DialogueFinalEmperorRoom1);
        } else {
            // Japanese translation
            std::vector<DialogueMessage> Dialogue0 = {
                {
                    "皇帝",                               // speaker
                    "...ついにここまで来たか..",            // message
                    basePath + "ancient_emperor.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Yellow,
                    sf::Color::White,
                    sf::Vector2f(800.f, 500.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "皇帝",                               // speaker
                    "..異星の侵入者よ...",            // message
                    basePath + "ancient_emperor.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Yellow,
                    sf::Color::White,
                    sf::Vector2f(800.f, 500.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    40,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "皇帝",                             // speaker
                    "私はこの王国の皇帝だ...",            // message
                    basePath + "ancient_emperor.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Yellow,
                    sf::Color::White,
                    sf::Vector2f(800.f, 500.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "皇帝",                             // speaker
                    "よくもここへ来たな！？",            // message
                    basePath + "ancient_emperor.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Yellow,
                    sf::Color::White,
                    sf::Vector2f(800.f, 500.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    35,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "エイリアン兵士",                               // speaker
                    "私は未来から来た...！！",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "エイリアン兵士",                              // speaker
                    "未来のあなたの王国が私の惑星を侵略した",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    800.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "エイリアン兵士",                              // speaker
                    "そして逃げている途中でブラックホールを通ってここに...",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    1000.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "皇帝",                             // speaker
                    "何を言っているのだ？！",            // message
                    basePath + "ancient_emperor.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Yellow,
                    sf::Color::White,
                    sf::Vector2f(750.f, 500.f),
                    750.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "皇帝",                             // speaker
                    "ブラックホール？！馬鹿げた話だ。",            // message
                    basePath + "ancient_emperor.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Yellow,
                    sf::Color::White,
                    sf::Vector2f(800.f, 500.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "皇帝",                               // speaker
                    "死せよ！！！",            // message
                    basePath + "ancient_emperor.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Yellow,
                    sf::Color::Red,
                    sf::Vector2f(800.f, 500.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    50,                                               // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(400, Dialogue0);
            std::vector<DialogueMessage> LowHPAncientEmperorDialogue = {
                {
                    "皇帝",                               // speaker
                    "我が真の力を見せるしかないようだな...",                           // message
                    basePath + "ancient_emperor.png", // portraitPath
                    false,                                   // portraitOnLeft
                    sf::Color::Cyan,                         // speakerColor
                    sf::Color::White,                          // messageColor
                    sf::Vector2f(600.f, 550.f),              // dialogueBoxPosition
                    910.f,                                   // boxWidth
                    150.f,                                   // boxHeight
                    30,                                      // messageFontSize
                    true                                     // useTypewriterEffect
                },
                {
                    "皇帝",                               // speaker
                    "力よ！",                           // message
                    basePath + "ancient_emperor.png", // portraitPath
                    false,                                   // portraitOnLeft
                    sf::Color::Cyan,                         // speakerColor
                    sf::Color::Red,                          // messageColor
                    sf::Vector2f(800.f, 550.f),              // dialogueBoxPosition
                    600.f,                                   // boxWidth
                    150.f,                                   // boxHeight
                    60,                                      // messageFontSize
                    true                                     // useTypewriterEffect
                }
            };
            // Add the named dialogues to our system
            m_dialogueSystem->addNamedDialogue("emperor_lowHP", LowHPAncientEmperorDialogue);
            std::vector<DialogueMessage> phasedefeatedAncientDialogue = {
                {
                    "皇帝",                              // speaker
                    "貴様...この...異星人め...",            // message
                    basePath + "ancient_emperor_defeated.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Yellow,
                    sf::Color::White,
                    sf::Vector2f(100.f, 400.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "エイリアン兵士",                             // speaker
                    "近づけないな...",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(600.f, 400.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "エイリアン兵士",                             // speaker
                    "だが疲れ切っているようだ...",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(600.f, 400.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "エイリアン兵士",                             // speaker
                    "今こそ逃げるチャンスだ！",            // message
                    basePath + "alien_ancient.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(600.f, 400.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
        
            };
            m_dialogueSystem->addNamedDialogue("emperor_ancient_defeated", phasedefeatedAncientDialogue);
            std::vector<DialogueMessage> DialogueFinalEmperorRoom0 = {
                {
                    "エイリアン兵士",                               // speaker
                    "あっ！道を塞いでいたバリアが消えた...",            // message
                basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    900.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(4400, DialogueFinalEmperorRoom0);
            std::vector<DialogueMessage> DialogueFinalEmperorRoom1 = {
                {
                    "エイリアン兵士",                               // speaker
                    "家に帰ろう...",            // message
                basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(6000, DialogueFinalEmperorRoom1);
        }
    }
    else if (levelName ==  "ancient_rome_level_5_day_v2.txt") {
        if (m_language == "English") {
            std::vector<DialogueMessage> Dialogue0 = {
                {
                    "Alien Legionary",                               // speaker
                    "Oh, I've returned here...",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "Alien Legionary",                              // speaker
                    "Let's see if those golden stairs are accessible now... ",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    970.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(-10800, Dialogue0);
            std::vector<DialogueMessage> Dialogue1 = {
                {
                    "Alien Legionary",                               // speaker
                    "Oh, the stairs are finally accessible!",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "Alien Legionary",                              // speaker
                    "Lets see if I can go get back to the black hole..",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    900.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(-2750, Dialogue1);
            std::vector<DialogueMessage> Dialogue2 = {
                {
                    "Alien Legionary",                               // speaker
                    "Great! I can actually return now!",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "Alien Legionary",                              // speaker
                    "With the Emperor fallen, I wonder if...",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    720.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "Alien Legionary",                              // speaker
                    "..those dark warriors have been erased from time?",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    960.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "Alien Legionary",                              // speaker
                    "Only one way to find out!",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(-950, Dialogue2);
        } else {
            // Japanese translation
            std::vector<DialogueMessage> Dialogue0 = {
                {
                    "エイリアン兵士",                               // speaker
                    "ここに戻ってきたか...",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "エイリアン兵士",                              // speaker
                    "あの黄金の階段に今は登れるかな...",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    970.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(-10800, Dialogue0);
            std::vector<DialogueMessage> Dialogue1 = {
                {
                    "エイリアン兵士",                               // speaker
                    "おお、ついに階段に登れるようになった！",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "エイリアン兵士",                              // speaker
                    "ブラックホールに戻れるかどうか見てみよう..",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    900.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(-2750, Dialogue1);
            std::vector<DialogueMessage> Dialogue2 = {
                {
                    "エイリアン兵士",                               // speaker
                    "よし！これで帰れるぞ！",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "エイリアン兵士",                              // speaker
                    "皇帝が倒れたことで...",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    720.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "エイリアン兵士",                              // speaker
                    "..あの暗黒の戦士たちは時間軸から消えたのかな？",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    960.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "エイリアン兵士",                              // speaker
                    "確かめる方法はひとつだけだ！",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(-950, Dialogue2);
        }
    }
    else if (levelName ==  "future_rome_level_1.txt") {
        if (m_language == "English") {
            std::vector<DialogueMessage> Dialogue0 = {
                {
                    "Alien Legionary",                               // speaker
                    "Where am I???",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    40,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "Alien Legionary",                              // speaker
                    "What happened???",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    40,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "Alien Legionary",                               // speaker
                    "I entered the same black hole but...",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    800.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "Alien Legionary",                               // speaker
                    "...this present seem different",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    800.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                };
                m_dialogueSystem->addDialogueTrigger(600, Dialogue0);
                std::vector<DialogueMessage> Dialogue02 = {
                {
                    "Alien Legionary",
                    "Let me think:",
                    basePath + "alien_ancient.png",
                    true,
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    750.f,
                    150.f,
                    30,
                    true
                },
                {
                    "Alien Legionary",
                    "I went through the black hole...",
                    basePath + "alien_ancient.png",
                    true,
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    750.f,
                    150.f,
                    30,
                    true
                },
                {
                    "Alien Legionary",
                    "And this was supposed to be their present...",
                    basePath + "alien_ancient.png",
                    true,
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    850.f,
                    150.f,
                    30,
                    true
                },
                {
                    "Alien Legionary",
                    "But I killed their Emperor in the past...",
                    basePath + "alien_ancient.png",
                    true,
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    850.f,
                    150.f,
                    30,
                    true
                },
                {
                    "Alien Legionary",
                    "And so they shouldn't exist anymore...",
                    basePath + "alien_ancient.png",
                    true,
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    850.f,
                    150.f,
                    30,
                    true
                },
                {
                    "Alien Legionary",
                    "Did I somehow change history...",
                    basePath + "alien_ancient.png",
                    true,
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    750.f,
                    150.f,
                    30,
                    true
                },
                {
                    "Alien Legionary",
                    "AND CREATED A NEW PRESENT?!",
                    basePath + "alien_ancient.png",
                    true,
                    sf::Color::Magenta,
                    sf::Color::Red,
                    sf::Vector2f(0.f, 500.f),
                    800.f,
                    150.f,
                    40,
                    true
                }
            };
            m_dialogueSystem->addDialogueTrigger(900, Dialogue02);
            std::vector<DialogueMessage> Dialogue1 = {
                {
                    "Legionary",                               // speaker
                    "Who are you??",            // message
                    basePath + "future_normal.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Cyan,
                    sf::Color::White,
                    sf::Vector2f(700.f, 500.f),
                    600.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "Alien Legionary",                              // speaker
                    "Oh no..don't tell me that..",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "Alien Legionary",                              // speaker
                    "they are trying kill me??",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "Alien Legionary",                              // speaker
                    "AGAIN?!",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    50,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(1500, Dialogue1);
            std::vector<DialogueMessage> Dialogue21 = {
                {
                    "Legionary",                              // speaker
                    "WHO ARE YOU???",            // message
                    basePath + "future_fast.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Cyan,
                    sf::Color::White,
                    sf::Vector2f(800.f, 300.f),
                    600.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    50,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(3000, Dialogue21);
            std::vector<DialogueMessage> Dialogue2 = {
                {
                    "Alien Legionary",                              // speaker
                    "Thinking about it...",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "Alien Legionary",                              // speaker
                    "these legionaries look familiar...",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "Alien Legionary",                              // speaker
                    "but how is that possible??",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(3900, Dialogue2);
            std::vector<DialogueMessage> Dialogue3 = {
            {
                "Alien Legionary",                              // speaker
                "They are too strong...",            // message
                basePath + "alien_ancient.png",       // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(0.f, 500.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                30,                                             // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            }
            ,
            {
                "Alien Legionary",                              // speaker
                "With my sword there is not much I can do...",            // message
                basePath + "alien_ancient.png",       // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(0.f, 500.f),
                950.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                30,                                             // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            }
            ,
            {
                "Alien Legionary",                              // speaker
                "What should I do??",            // message
                basePath + "alien_ancient.png",       // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(0.f, 500.f),
                700.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                30,                                             // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            }
            ,
            {
                "Alien Legionary",                              // speaker
                "I am in big trouble now...",            // message
                basePath + "alien_ancient.png",       // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(0.f, 500.f),
                700.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                30,                                             // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            }
            };
            m_dialogueSystem->addDialogueTrigger(4800, Dialogue3);
        std::vector<DialogueMessage> Dialogue4 = {
            {
                "Alien Legionary",                              // speaker
                "Oh... what is that futuristic looking armor??",            // message
                basePath + "alien_ancient.png",       // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(0.f, 500.f),
                950.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                30,                                             // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            }
            ,
            {
                "Alien Legionary",                              // speaker
                "Maybe If I wear it I can shoot bullets like they do!",            // message
                basePath + "alien_ancient.png",       // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(0.f, 500.f),
                950.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                30,                                             // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            }
        };
        m_dialogueSystem->addDialogueTrigger(5700, Dialogue4);
        std::vector<DialogueMessage> Dialogue5 = {
            {
                "Alien Legionary",                              // speaker
                "IT WORKED!!!",            // message
                basePath + "alien_future.png",       // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(0.f, 500.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                40,                                             // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            }
            ,
            {
                "Alien Legionary",                           // speaker
                "NOW I CAN SHOOT BULLETS!!",            // message
                basePath + "alien_future.png",       // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(0.f, 500.f),
                700.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                40,                                             // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            }
            ,
            {
                "Alien Legionary",                           // speaker
                "TAKE THESE YOU DAMN PARADOX PEOPLE!!!",            // message
                basePath + "alien_future.png",       // portraitPath
                false,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(0.f, 500.f),
                1100.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                40,                                             // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            }
            ,
            {
                "***GUIDE***",                           // speaker
                "(Hold SPACE to shoot bursts of bullets!)",            // message
                basePath + "alien_future.png",       // portraitPath
                false,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(0.f, 200.f),
                1100.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                40,                                             // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            }
            ,
            {
                "***GUIDE***",                           // speaker
                "(Press ENTER to activate the supermove!!)",            // message
                basePath + "alien_future.png",       // portraitPath
                false,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(0.f, 200.f),
                1100.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                40,                                             // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            }
        };
        m_dialogueSystem->addDialogueTrigger(6400, Dialogue5);
        } else {
            // Japanese translation
            std::vector<DialogueMessage> Dialogue0 = {
                {
                    "エイリアン兵士",                               // speaker
                    "ここはどこだ？？？",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    40,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "エイリアン兵士",                              // speaker
                    "何が起きた？？？",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    40,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "エイリアン兵士",                               // speaker
                    "同じブラックホールに入ったはずなのに...",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    800.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "エイリアン兵士",                               // speaker
                    "...この現在は違う場所のようだ",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    800.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                };
                m_dialogueSystem->addDialogueTrigger(600, Dialogue0);
                std::vector<DialogueMessage> Dialogue02 = {
                {
                    "エイリアン兵士",
                    "考えてみよう：",
                    basePath + "alien_ancient.png",
                    true,
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    750.f,
                    150.f,
                    30,
                    true
                },
                {
                    "エイリアン兵士",
                    "ブラックホールを通り抜けて...",
                    basePath + "alien_ancient.png",
                    true,
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    750.f,
                    150.f,
                    30,
                    true
                },
                {
                    "エイリアン兵士",
                    "ここは彼らの現在のはずだったが...",
                    basePath + "alien_ancient.png",
                    true,
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    850.f,
                    150.f,
                    30,
                    true
                },
                {
                    "エイリアン兵士",
                    "過去で皇帝を倒したから...",
                    basePath + "alien_ancient.png",
                    true,
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    850.f,
                    150.f,
                    30,
                    true
                },
                {
                    "エイリアン兵士",
                    "彼らはもう存在しないはずなのに...",
                    basePath + "alien_ancient.png",
                    true,
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    850.f,
                    150.f,
                    30,
                    true
                },
                {
                    "エイリアン兵士",
                    "まさか歴史を変えてしまったのか...",
                    basePath + "alien_ancient.png",
                    true,
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    750.f,
                    150.f,
                    30,
                    true
                },
                {
                    "エイリアン兵士",
                    "新たな現在を作り出してしまった！？",
                    basePath + "alien_ancient.png",
                    true,
                    sf::Color::Magenta,
                    sf::Color::Red,
                    sf::Vector2f(0.f, 500.f),
                    800.f,
                    150.f,
                    40,
                    true
                }
            };
            m_dialogueSystem->addDialogueTrigger(900, Dialogue02);
            std::vector<DialogueMessage> Dialogue1 = {
                {
                    "兵士",                               // speaker
                    "お前は誰だ？？",            // message
                    basePath + "future_normal.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Cyan,
                    sf::Color::White,
                    sf::Vector2f(700.f, 500.f),
                    600.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "エイリアン兵士",                              // speaker
                    "まさか...そんな...",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "エイリアン兵士",                              // speaker
                    "また殺そうとしてるのか？",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "エイリアン兵士",                              // speaker
                    "また！？",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    50,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(1500, Dialogue1);
            std::vector<DialogueMessage> Dialogue21 = {
                {
                    "兵士",                              // speaker
                    "お前は何者だ！？",            // message
                    basePath + "future_fast.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Cyan,
                    sf::Color::White,
                    sf::Vector2f(800.f, 300.f),
                    600.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    50,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(3000, Dialogue21);
            std::vector<DialogueMessage> Dialogue2 = {
                {
                    "エイリアン兵士",                              // speaker
                    "そういえば...",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "エイリアン兵士",                              // speaker
                    "この兵士たち、見覚えがある...",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "エイリアン兵士",                              // speaker
                    "どうしてそんなことが？？",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(3900, Dialogue2);
            std::vector<DialogueMessage> Dialogue3 = {
            {
                "エイリアン兵士",                              // speaker
                "あいつら強すぎる...",            // message
                basePath + "alien_ancient.png",       // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(0.f, 500.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                30,                                             // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            }
            ,
            {
                "エイリアン兵士",                              // speaker
                "この剣だけでは太刀打ちできない...",            // message
                basePath + "alien_ancient.png",       // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(0.f, 500.f),
                950.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                30,                                             // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            }
            ,
            {
                "エイリアン兵士",                              // speaker
                "どうすればいい？？",            // message
                basePath + "alien_ancient.png",       // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(0.f, 500.f),
                700.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                30,                                             // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            }
            ,
            {
                "エイリアン兵士",                              // speaker
                "大変なことになった...",            // message
                basePath + "alien_ancient.png",       // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(0.f, 500.f),
                700.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                30,                                             // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            }
            };
            m_dialogueSystem->addDialogueTrigger(4800, Dialogue3);
        std::vector<DialogueMessage> Dialogue4 = {
            {
                "エイリアン兵士",                              // speaker
                "あれは...未来的な装甲？？",            // message
                basePath + "alien_ancient.png",       // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(0.f, 500.f),
                950.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                30,                                             // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            }
            ,
            {
                "エイリアン兵士",                              // speaker
                "着てみたら彼らのように弾を撃てるかも！",            // message
                basePath + "alien_ancient.png",       // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(0.f, 500.f),
                950.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                30,                                             // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            }
        };
        m_dialogueSystem->addDialogueTrigger(5700, Dialogue4);
        std::vector<DialogueMessage> Dialogue5 = {
            {
                "エイリアン兵士",                              // speaker
                "できた！！！",            // message
                basePath + "alien_future.png",       // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(0.f, 500.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                40,                                             // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            }
            ,
            {
                "エイリアン兵士",                           // speaker
                "弾を撃てるぞ！！",            // message
                basePath + "alien_future.png",       // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(0.f, 500.f),
                700.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                40,                                             // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            }
            ,
            {
                "エイリアン兵士",                           // speaker
                "食らえ、この時空の矛盾め！！！",            // message
                basePath + "alien_future.png",       // portraitPath
                false,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(0.f, 500.f),
                1100.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                40,                                             // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            }
            ,
            {
                "***ガイド***",                           // speaker
                "(スペースキーを長押しで連射！)",            // message
                basePath + "alien_future.png",       // portraitPath
                false,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(0.f, 200.f),
                1100.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                40,                                             // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            }
            ,
            {
                "***ガイド***",                           // speaker
                "(エンターキーで必殺技！！)",            // message
                basePath + "alien_future.png",       // portraitPath
                false,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(0.f, 200.f),
                1100.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                40,                                             // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            }
        };
        m_dialogueSystem->addDialogueTrigger(6400, Dialogue5);
        }
    }
    else if (levelName ==  "future_rome_level_3.txt") {
        if (m_language == "English") {
            std::vector<DialogueMessage> Dialogue0 = {
                {
                    "Legionary (Elite)",                               // speaker
                    "You.. invader...",            // message
                    basePath + "future_elite.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Red,
                    sf::Color::White,
                    sf::Vector2f(700.f, 450.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "Legionary (Elite)",                             // speaker
                    "How dare you coming to this place??",            // message
                    basePath + "future_elite.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Red,
                    sf::Color::White,
                    sf::Vector2f(700.f, 450.f),
                    750.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                };
                m_dialogueSystem->addDialogueTrigger(600, Dialogue0);
                std::vector<DialogueMessage> Dialogue1 = {
                {
                    "Alien Legionary",                               // speaker
                    "These legionnaries look danegrous..",            // message
                    basePath + "alien_future.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(50.f, 450.f),
                    800.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "Alien Legionary",                             // speaker
                    "I can barely see them...",            // message
                    basePath + "alien_future.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(50.f, 450.f),
                    750.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "Alien Legionary",                             // speaker
                    "Like fighting shadows... or ghosts...",            // message
                    basePath + "alien_future.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(50.f, 450.f),
                    750.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "Alien Legionary",                             // speaker
                    "Gotta be very careful",            // message
                    basePath + "alien_future.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(50.f, 450.f),
                    750.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                };
                m_dialogueSystem->addDialogueTrigger(2600, Dialogue1);
                std::vector<DialogueMessage> Dialogue02 = {
                {
                    "Legionary (Elite)",                           // speaker
                    "Who is this strange legionary?",            // message
                    basePath + "future_elite.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Red,
                    sf::Color::White,
                    sf::Vector2f(500.f, 400.f),
                    750.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                               // useTypewriterEffect
                }
                ,
                {
                    "Legionary (Elite)",                           // speaker
                    "Wait... those violet and black colors...",            // message
                    basePath + "future_elite.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Red,
                    sf::Color::White,
                    sf::Vector2f(600.f, 400.f),
                    780.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                                 // messageFontSize - still emphasized
                    true                                               // useTypewriterEffect
                }
                ,
                {
                    "Legionary (Elite)",                           // speaker
                    "IMPOSSIBLE",            // message
                    basePath + "future_elite.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Red,
                    sf::Color::Red,
                    sf::Vector2f(500.f, 400.f),
                    780.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    45,                                                  // messageFontSize - still emphasized
                    true                                               // useTypewriterEffect
                }
                ,
                {
                    "Legionary (Elite)",                           // speaker
                    "You match the ancient records...",            // message
                    basePath + "future_elite.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Red,
                    sf::Color::White,
                    sf::Vector2f(600.f, 400.f),
                    780.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                                  // messageFontSize - still emphasized
                    true                                               // useTypewriterEffect
                }
                ,
                {
                    "Legionary (Elite)",                             // speaker
                    "A warrior who wounded our emperor and then..",            // message
                    basePath + "future_elite.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Red,
                    sf::Color::White,
                    sf::Vector2f(500.f, 400.f),
                    900.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                                // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "Legionary (Elite)",                             // speaker
                    "VANISHED...",            // message
                    basePath + "future_elite.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Red,
                    sf::Color::White,
                    sf::Vector2f(600.f, 400.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    50,                                                // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "Legionary (Elite)",                             // speaker
                    "could it be really you?!",            // message
                    basePath + "future_elite.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Red,
                    sf::Color::White,
                    sf::Vector2f(500.f, 400.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                                // messageFontSize - still emphasized
                    true                                              // useTypewriterEffect
                }
                ,
                {
                    "Alien Legionary",                              // speaker
                    "Oh.. I think I really DID travel through time...",// message
                    basePath + "alien_future.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 400.f),
                    900.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(5700, Dialogue02);
            std::vector<DialogueMessage> Dialoguelast = {
            {
                "Legionary (Elite)",                                // speaker
                "STOP THE INVADER!!!",// message
                basePath + "future_elite.png",       // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Red,
                sf::Color::Red,
                sf::Vector2f(0.f, 400.f),
                800.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                40,                                             // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            },
            {
                "Legionary (Elite)",                                // speaker
                "STOP HIM NOW! HISTORY MUST NOT REPEAT ITSELF!",// message
                basePath + "future_elite.png",       // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Red,
                sf::Color::Red,
                sf::Vector2f(0.f, 400.f),
                1100.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                35,                                             // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            }
            };
            m_dialogueSystem->addDialogueTrigger(12500, Dialoguelast);
        } else {
            // Japanese translation
            std::vector<DialogueMessage> Dialogue0 = {
                {
                    "兵士（精鋭）",                               // speaker
                    "貴様...侵入者...",            // message
                    basePath + "future_elite.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Red,
                    sf::Color::White,
                    sf::Vector2f(700.f, 450.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "兵士（精鋭）",                             // speaker
                    "よくもここへ来たな？？",            // message
                    basePath + "future_elite.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Red,
                    sf::Color::White,
                    sf::Vector2f(700.f, 450.f),
                    750.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                };
                m_dialogueSystem->addDialogueTrigger(600, Dialogue0);
                std::vector<DialogueMessage> Dialogue1 = {
                {
                    "エイリアン兵士",                               // speaker
                    "この兵士たちは危険そうだ...",            // message
                    basePath + "alien_future.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(50.f, 450.f),
                    800.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "エイリアン兵士",                             // speaker
                    "ほとんど見えないほどだ...",            // message
                    basePath + "alien_future.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(50.f, 450.f),
                    750.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "エイリアン兵士",                             // speaker
                    "影か幽霊と戦っているようだ...",            // message
                    basePath + "alien_future.png",        // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(50.f, 450.f),
                    750.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "エイリアン兵士",                             // speaker
                    "かなり注意しないと",            // message
                    basePath + "alien_future.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(50.f, 450.f),
                    750.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                };
                m_dialogueSystem->addDialogueTrigger(2600, Dialogue1);
                std::vector<DialogueMessage> Dialogue02 = {
                {
                    "兵士（精鋭）",                           // speaker
                    "この奇妙な兵士は誰だ？",            // message
                    basePath + "future_elite.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Red,
                    sf::Color::White,
                    sf::Vector2f(500.f, 400.f),
                    750.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                               // useTypewriterEffect
                }
                ,
                {
                    "兵士（精鋭）",                           // speaker
                    "待て...あの紫と黒の色...",            // message
                    basePath + "future_elite.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Red,
                    sf::Color::White,
                    sf::Vector2f(600.f, 400.f),
                    780.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                                 // messageFontSize - still emphasized
                    true                                               // useTypewriterEffect
                }
                ,
                {
                    "兵士（精鋭）",                           // speaker
                    "ありえない",            // message
                    basePath + "future_elite.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Red,
                    sf::Color::Red,
                    sf::Vector2f(500.f, 400.f),
                    780.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    45,                                                  // messageFontSize - still emphasized
                    true                                               // useTypewriterEffect
                }
                ,
                {
                    "兵士（精鋭）",                           // speaker
                    "古代の記録に一致している...",            // message
                    basePath + "future_elite.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Red,
                    sf::Color::White,
                    sf::Vector2f(600.f, 400.f),
                    780.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                                  // messageFontSize - still emphasized
                    true                                               // useTypewriterEffect
                }
                ,
                {
                    "兵士（精鋭）",                             // speaker
                    "我らが皇帝に傷を負わせ、そして...",            // message
                    basePath + "future_elite.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Red,
                    sf::Color::White,
                    sf::Vector2f(500.f, 400.f),
                    900.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                                // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "兵士（精鋭）",                             // speaker
                    "消え去った戦士...",            // message
                    basePath + "future_elite.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Red,
                    sf::Color::White,
                    sf::Vector2f(600.f, 400.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    50,                                                // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "兵士（精鋭）",                             // speaker
                    "本当にお前なのか！？",            // message
                    basePath + "future_elite.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Red,
                    sf::Color::White,
                    sf::Vector2f(500.f, 400.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                                // messageFontSize - still emphasized
                    true                                              // useTypewriterEffect
                }
                ,
                {
                    "エイリアン兵士",                              // speaker
                    "なるほど...本当に時間を超えたんだな...",// message
                    basePath + "alien_future.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 400.f),
                    900.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(5700, Dialogue02);
            std::vector<DialogueMessage> Dialoguelast = {
            {
                "兵士（精鋭）",                                // speaker
                "侵入者を止めろ！！！",// message
                basePath + "future_elite.png",       // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Red,
                sf::Color::Red,
                sf::Vector2f(0.f, 400.f),
                800.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                40,                                             // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            },
            {
                "兵士（精鋭）",                                // speaker
                "今すぐ止めろ！歴史は繰り返してはならない！",// message
                basePath + "future_elite.png",       // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Red,
                sf::Color::Red,
                sf::Vector2f(0.f, 400.f),
                1100.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                35,                                             // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            }
            };
            m_dialogueSystem->addDialogueTrigger(12500, Dialoguelast);
        }
    }
    else if (levelName == "future_rome_level_4_emperor_room.txt") {
        if (m_language == "English") {
            std::vector<DialogueMessage> bossDialogue = {
                {
                    "Emperor",
                    "... And so you are back...",
                    basePath + "future_emperor.png",
                    false,                                    // portraitOnLeft (right side)
                    sf::Color::Cyan,                           // speakerColor
                    sf::Color::White,                         // messageColor
                    sf::Vector2f(800.f, 550.f),                // dialogueBoxPosition
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "Emperor",
                    "I remember you... ",
                    basePath + "future_emperor.png",
                    false,
                    sf::Color::Cyan,
                    sf::Color::White,
                    sf::Vector2f(800.f, 550.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(500, bossDialogue);
            std::vector<DialogueMessage> bossDialogue2 = {
                {
                    "Emperor",
                    "Thousands of years ago you came here.. ",
                    basePath + "future_emperor.png",
                    false,
                    sf::Color::Cyan,
                    sf::Color::White,
                    sf::Vector2f(800.f, 550.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    26,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "Emperor",
                    ".. and I barely survived your wrath.",
                    basePath + "future_emperor.png",
                    false,
                    sf::Color::Cyan,
                    sf::Color::White,
                    sf::Vector2f(800.f, 550.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    26,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "Emperor",
                    "But this time..",
                    basePath + "future_emperor.png",
                    false,
                    sf::Color::Cyan,
                    sf::Color::White,
                    sf::Vector2f(800.f, 550.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "Emperor",
                    "YOU WILL NOT SURVIVE!!!",
                    basePath + "future_emperor.png",
                    false,
                    sf::Color::Cyan,
                    sf::Color::Red,
                    sf::Vector2f(800.f, 550.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    40,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(700, bossDialogue2);
            std::vector<DialogueMessage> phase2Dialogue = {
                {
                    "Emperor",                               // speaker
                    "You are stronger than I expected...",    // message
                    basePath + "future_emperor_2.png", // portraitPath (adjust path as needed)
                    false,                                   // portraitOnLeft
                    sf::Color::Cyan,                         // speakerColor
                    sf::Color::White,                        // messageColor
                    sf::Vector2f(800.f, 550.f),              // dialogueBoxPosition
                    650.f,                                   // boxWidth
                    150.f,                                   // boxHeight
                    30,                                      // messageFontSize
                    true                                     // useTypewriterEffect
                },
                {
                    "Emperor",
                    "But this is just the beginning!!",
                    basePath + "future_emperor_2.png",
                    false,
                    sf::Color::Cyan,
                    sf::Color::Red,
                    sf::Vector2f(800.f, 550.f),
                    650.f,
                    150.f,
                    35,
                    true
                }
            };
            
            // Phase 3 dialogue (30% health)
            std::vector<DialogueMessage> phase3Dialogue = {
                {
                    "Emperor",                               // speaker
                    "HOW IS THIS POSSIBLE??",                           // message
                    basePath + "future_emperor_3.png", // portraitPath
                    false,                                   // portraitOnLeft
                    sf::Color::Cyan,                         // speakerColor
                    sf::Color::Red,                          // messageColor
                    sf::Vector2f(800.f, 550.f),              // dialogueBoxPosition
                    650.f,                                   // boxWidth
                    150.f,                                   // boxHeight
                    30,                                      // messageFontSize
                    true                                     // useTypewriterEffect
                },
                {
                    "Emperor",
                    "IT'S NOT OVER YET!!!",
                    basePath + "future_emperor_3.png",
                    false,
                    sf::Color::Cyan,
                    sf::Color::Red,
                    sf::Vector2f(800.f, 550.f),
                    650.f,
                    150.f,
                    40,
                    true
                }
            };
            // Phase 3 dialogue (30% health)
            std::vector<DialogueMessage> phasefinalDialogue = {
                {
                    "Emperor",                               // speaker
                    "IMP055IBLE!!!",                           // message
                    basePath + "future_emperor_3.png", // portraitPath
                    false,                                   // portraitOnLeft
                    sf::Color::Cyan,                         // speakerColor
                    sf::Color::Red,                          // messageColor
                    sf::Vector2f(800.f, 550.f),              // dialogueBoxPosition
                    650.f,                                   // boxWidth
                    150.f,                                   // boxHeight
                    30,                                      // messageFontSize
                    true                                     // useTypewriterEffect
                },
                {
                    "Emperor",
                    "TIME T0 SH0W Y0U MY...",
                    basePath + "future_emperor_3.png",
                    false,
                    sf::Color::Cyan,
                    sf::Color::Red,
                    sf::Vector2f(800.f, 550.f),
                    650.f,
                    150.f,
                    40,
                    true
                },
                {
                    "Emperor",
                    "TRUE POWER!",
                    basePath + "future_emperor_3.png",
                    false,
                    sf::Color::Cyan,
                    sf::Color::Red,
                    sf::Vector2f(800.f, 550.f),
                    650.f,
                    150.f,
                    50,
                    true
                }
            };
            std::vector<DialogueMessage> phasedefeatedFutureDialogue = {
                {
                    "Emperor",                               // speaker
                    "...",                           // message
                    basePath + "future_emperor_3.png", // portraitPath
                    false,                                   // portraitOnLeft
                    sf::Color::Cyan,                         // speakerColor
                    sf::Color::White,                          // messageColor
                    sf::Vector2f(800.f, 550.f),              // dialogueBoxPosition
                    650.f,                                   // boxWidth
                    150.f,                                   // boxHeight
                    30,                                      // messageFontSize
                    true                                     // useTypewriterEffect
                }
            };
            // Add the named dialogues to our system
            m_dialogueSystem->addNamedDialogue("emperor_phase2", phase2Dialogue);
            m_dialogueSystem->addNamedDialogue("emperor_phase3", phase3Dialogue);
            m_dialogueSystem->addNamedDialogue("emperor_future_final", phasefinalDialogue);
            m_dialogueSystem->addNamedDialogue("emperor_future_defeated", phasedefeatedFutureDialogue);
        } if (m_language == "Japanese") {
            std::vector<DialogueMessage> bossDialogue = {
                {
                    "皇帝",
                    "...そして戻ってきたか...",
                    basePath + "future_emperor.png",
                    false,                                    // portraitOnLeft (right side)
                    sf::Color::Cyan,                           // speakerColor
                    sf::Color::White,                         // messageColor
                    sf::Vector2f(800.f, 550.f),                // dialogueBoxPosition
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "皇帝",
                    "お前のことは覚えている...",
                    basePath + "future_emperor.png",
                    false,
                    sf::Color::Cyan,
                    sf::Color::White,
                    sf::Vector2f(800.f, 550.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(500, bossDialogue);
            std::vector<DialogueMessage> bossDialogue2 = {
                {
                    "皇帝",
                    "数千年前、お前はここに来た...",
                    basePath + "future_emperor.png",
                    false,
                    sf::Color::Cyan,
                    sf::Color::White,
                    sf::Vector2f(800.f, 550.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    26,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "皇帝",
                    "...私はかろうじてお前の怒りから生き延びた",
                    basePath + "future_emperor.png",
                    false,
                    sf::Color::Cyan,
                    sf::Color::White,
                    sf::Vector2f(800.f, 550.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    26,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "皇帝",
                    "だが今回は...",
                    basePath + "future_emperor.png",
                    false,
                    sf::Color::Cyan,
                    sf::Color::White,
                    sf::Vector2f(800.f, 550.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "皇帝",
                    "お前は生き残れん！！！",
                    basePath + "future_emperor.png",
                    false,
                    sf::Color::Cyan,
                    sf::Color::Red,
                    sf::Vector2f(800.f, 550.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    40,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(700, bossDialogue2);
            std::vector<DialogueMessage> phase2Dialogue = {
                {
                    "皇帝",                               // speaker
                    "予想以上に強いようだな...",    // message
                    basePath + "future_emperor_2.png", // portraitPath (adjust path as needed)
                    false,                                   // portraitOnLeft
                    sf::Color::Cyan,                         // speakerColor
                    sf::Color::White,                        // messageColor
                    sf::Vector2f(800.f, 550.f),              // dialogueBoxPosition
                    650.f,                                   // boxWidth
                    150.f,                                   // boxHeight
                    30,                                      // messageFontSize
                    true                                     // useTypewriterEffect
                },
                {
                    "皇帝",
                    "だがこれはほんの始まりだ！！",
                    basePath + "future_emperor_2.png",
                    false,
                    sf::Color::Cyan,
                    sf::Color::Red,
                    sf::Vector2f(800.f, 550.f),
                    650.f,
                    150.f,
                    35,
                    true
                }
            };
            
            // Phase 3 dialogue (30% health)
            std::vector<DialogueMessage> phase3Dialogue = {
                {
                    "皇帝",                               // speaker
                    "どうしてこれが可能だ！？",                           // message
                    basePath + "future_emperor_3.png", // portraitPath
                    false,                                   // portraitOnLeft
                    sf::Color::Cyan,                         // speakerColor
                    sf::Color::Red,                          // messageColor
                    sf::Vector2f(800.f, 550.f),              // dialogueBoxPosition
                    650.f,                                   // boxWidth
                    150.f,                                   // boxHeight
                    30,                                      // messageFontSize
                    true                                     // useTypewriterEffect
                },
                {
                    "皇帝",
                    "まだ終わっていない！！！",
                    basePath + "future_emperor_3.png",
                    false,
                    sf::Color::Cyan,
                    sf::Color::Red,
                    sf::Vector2f(800.f, 550.f),
                    650.f,
                    150.f,
                    40,
                    true
                }
            };
            // Phase 3 dialogue (30% health)
            std::vector<DialogueMessage> phasefinalDialogue = {
                {
                    "皇帝",                               // speaker
                    "あり得ない！！！",                           // message
                    basePath + "future_emperor_3.png", // portraitPath
                    false,                                   // portraitOnLeft
                    sf::Color::Cyan,                         // speakerColor
                    sf::Color::Red,                          // messageColor
                    sf::Vector2f(800.f, 550.f),              // dialogueBoxPosition
                    650.f,                                   // boxWidth
                    150.f,                                   // boxHeight
                    30,                                      // messageFontSize
                    true                                     // useTypewriterEffect
                },
                {
                    "皇帝",
                    "今こそ見せてやろう...",
                    basePath + "future_emperor_3.png",
                    false,
                    sf::Color::Cyan,
                    sf::Color::Red,
                    sf::Vector2f(800.f, 550.f),
                    650.f,
                    150.f,
                    40,
                    true
                },
                {
                    "皇帝",
                    "真の力を！",
                    basePath + "future_emperor_3.png",
                    false,
                    sf::Color::Cyan,
                    sf::Color::Red,
                    sf::Vector2f(800.f, 550.f),
                    650.f,
                    150.f,
                    50,
                    true
                }
            };
            std::vector<DialogueMessage> phasedefeatedFutureDialogue = {
                {
                    "皇帝",                               // speaker
                    "...",                           // message
                    basePath + "future_emperor_3.png", // portraitPath
                    false,                                   // portraitOnLeft
                    sf::Color::Cyan,                         // speakerColor
                    sf::Color::White,                          // messageColor
                    sf::Vector2f(800.f, 550.f),              // dialogueBoxPosition
                    650.f,                                   // boxWidth
                    150.f,                                   // boxHeight
                    30,                                      // messageFontSize
                    true                                     // useTypewriterEffect
                }
            };
            // Add the named dialogues to our system
            m_dialogueSystem->addNamedDialogue("emperor_phase2", phase2Dialogue);
            m_dialogueSystem->addNamedDialogue("emperor_phase3", phase3Dialogue);
            m_dialogueSystem->addNamedDialogue("emperor_future_final", phasefinalDialogue);
            m_dialogueSystem->addNamedDialogue("emperor_future_defeated", phasedefeatedFutureDialogue);
        }
    }
    else if (levelName ==  "future_rome_level_5_day_v2.txt") {
        if (m_language == "English") {
            std::vector<DialogueMessage> Dialogue0 = {
                {
                    "Alien Legionary",                               // speaker
                    "Oh, I am back here...",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "Alien Legionary",                              // speaker
                    "..I am so tired.. ",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "Alien Legionary",                              // speaker
                    "I must go back to the black hole and hope i can get home..",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    1050.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(-13900, Dialogue0);
            std::vector<DialogueMessage> Dialogue1 = {
                {
                    "Alien Legionary",                               // speaker
                    "Thinking about it..",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "Alien Legionary",                              // speaker
                    "I killed so many people...",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "Alien Legionary",                               // speaker
                    "but I had no other choices..",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "Alien Legionary",                              // speaker
                    "But.. did I really do the right thing?...",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    700.f,                                           // boxWidth - medium for medium messageN
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(-11000, Dialogue1);
            std::vector<DialogueMessage> Dialogue2 = {
                {
                    "Alien Legionary",                               // speaker
                    "Here are the stairs..",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(-3800, Dialogue2);
            std::vector<DialogueMessage> Dialogue3 = {
                {
                    "Alien Legionary",                               // speaker
                    "I am so afraid...",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "Alien Legionary",                              // speaker
                    "Will I ever see my home again?",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "Alien Legionary",                               // speaker
                    "Am I even real anymore?",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "Alien Legionary",                              // speaker
                    "I must keep fighting... it's my only hope of finding a way back...",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    1100.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(-1100, Dialogue3);
            std::vector<DialogueMessage> Dialogue4 = {
                {
                    "Alien Legionary",                               // speaker
                    "Here goes nothing!!",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    40,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "Legionary",                                // speaker
                    "HEY !!! STOP!!!",            // message
                    basePath + "future_fast.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Cyan,
                    sf::Color::White,
                    sf::Vector2f(600.f, 400.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    35,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "Legionary",                              // speaker
                    "YOU DAMNED PARADOX ALIEN..",            // message
                    basePath + "future_fast.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Cyan,
                    sf::Color::White,
                    sf::Vector2f(600.f, 400.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    35,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "Legionary",                              // speaker
                    "NOW I GET IT...",            // message
                    basePath + "future_fast.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Cyan,
                    sf::Color::White,
                    sf::Vector2f(600.f, 400.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    35,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "Legionary",                              // speaker
                    "YOU CAME HERE THROUGH THAT BLACK HOLE...",            // message
                    basePath + "future_fast.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Cyan,
                    sf::Color::White,
                    sf::Vector2f(500.f, 400.f),
                    950.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    35,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "Legionary",                              // speaker
                    "AND TRAVELED THROUGH DIMENSIONS..",            // message
                    basePath + "future_fast.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Cyan,
                    sf::Color::White,
                    sf::Vector2f(600.f, 400.f),
                    850.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    35,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "Legionary",                              // speaker
                    "I'LL HUNT YOU ACROSS EVERY UNIVERSE!!",            // message
                    basePath + "future_fast.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Cyan,
                    sf::Color::White,
                    sf::Vector2f(600.f, 400.f),
                    880.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    35,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "Legionary",                              // speaker
                    "AND I WILL MAKE YOU PAY FOR IT...",            // message
                    basePath + "future_fast.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Cyan,
                    sf::Color::White,
                    sf::Vector2f(600.f, 400.f),
                    750.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    35,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "Legionary",                              // speaker
                    "AT ALL COSTS!!!",            // message
                    basePath + "future_fast.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Cyan,
                    sf::Color::Red,
                    sf::Vector2f(600.f, 400.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    50,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(-400, Dialogue4);
        } else {
            std::vector<DialogueMessage> Dialogue0 = {
                {
                    "エイリアン兵士",                               // speaker
                    "ああ、またここに戻ってきたのか...",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "エイリアン兵士",                              // speaker
                    "..とても疲れた..",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "エイリアン兵士",                              // speaker
                    "ブラックホールに戻って、家に帰れることを祈るしかない..",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    1050.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(-13900, Dialogue0);
            std::vector<DialogueMessage> Dialogue1 = {
                {
                    "エイリアン兵士",                               // speaker
                    "考えてみれば..",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "エイリアン兵士",                              // speaker
                    "私はあんなにも多くの人を殺してしまった...",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "エイリアン兵士",                               // speaker
                    "でも他に選択肢はなかった..",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "エイリアン兵士",                              // speaker
                    "だが...本当に正しいことをしたのだろうか？...",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    700.f,                                           // boxWidth - medium for medium messageN
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(-11000, Dialogue1);
            std::vector<DialogueMessage> Dialogue2 = {
                {
                    "エイリアン兵士",                               // speaker
                    "階段はここか..",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(-3800, Dialogue2);
            std::vector<DialogueMessage> Dialogue3 = {
                {
                    "エイリアン兵士",                               // speaker
                    "怖くてたまらない...",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "エイリアン兵士",                              // speaker
                    "もう二度と故郷を見ることはできないのか？",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "エイリアン兵士",                               // speaker
                    "私はもう本当の自分なのだろうか？",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "エイリアン兵士",                              // speaker
                    "戦い続けるしかない...帰る道を見つける唯一の希望なのだから...",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    1100.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    30,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(-1100, Dialogue3);
            std::vector<DialogueMessage> Dialogue4 = {
                {
                    "エイリアン兵士",                               // speaker
                    "やるしかない！！",            // message
                    basePath + "alien_ancient.png",       // portraitPath
                    true,                                            // portraitOnLeft
                    sf::Color::Magenta,
                    sf::Color::White,
                    sf::Vector2f(0.f, 500.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    40,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "兵士",                                // speaker
                    "おい！！！止まれ！！！",            // message
                    basePath + "future_fast.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Cyan,
                    sf::Color::White,
                    sf::Vector2f(600.f, 400.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    35,                                              // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "兵士",                              // speaker
                    "貴様、パラドックスの異星人め..",            // message
                    basePath + "future_fast.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Cyan,
                    sf::Color::White,
                    sf::Vector2f(600.f, 400.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    35,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "兵士",                              // speaker
                    "今わかったぞ...",            // message
                    basePath + "future_fast.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Cyan,
                    sf::Color::White,
                    sf::Vector2f(600.f, 400.f),
                    700.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    35,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "兵士",                              // speaker
                    "お前はそのブラックホールを通ってここに来たんだな...",            // message
                    basePath + "future_fast.png",       // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Cyan,
                    sf::Color::White,
                    sf::Vector2f(500.f, 400.f),
                    950.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    35,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                },
                {
                    "兵士",                              // speaker
                    "そして次元を超えて旅をしていたのか..",            // message
                    basePath + "future_fast.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Cyan,
                    sf::Color::White,
                    sf::Vector2f(600.f, 400.f),
                    850.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    35,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "兵士",                              // speaker
                    "あらゆる宇宙でお前を追い詰めてやる！！",            // message
                    basePath + "future_fast.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Cyan,
                    sf::Color::White,
                    sf::Vector2f(600.f, 400.f),
                    880.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    35,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "兵士",                              // speaker
                    "そしてその報いを受けさせる...",            // message
                    basePath + "future_fast.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Cyan,
                    sf::Color::White,
                    sf::Vector2f(600.f, 400.f),
                    750.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    35,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
                ,
                {
                    "兵士",                              // speaker
                    "どんな犠牲を払ってでも！！！",            // message
                    basePath + "future_fast.png",        // portraitPath
                    false,                                            // portraitOnLeft
                    sf::Color::Cyan,
                    sf::Color::Red,
                    sf::Vector2f(600.f, 400.f),
                    650.f,                                           // boxWidth - medium for medium message
                    150.f,                                           // boxHeight
                    50,                                             // messageFontSize - still emphasized
                    true                                             // useTypewriterEffect
                }
            };
            m_dialogueSystem->addDialogueTrigger(-400, Dialogue4);
        }
    }
    std::cout << "[DEBUG] Dialogue system initialized for level: " << levelName << std::endl;
}
// Action Processing (Input Handling)                        
void Scene_Play::sDoAction(const Action& action)
{
    // First, track dialogue state changes
    bool isDialogueActive = (m_dialogueSystem && m_dialogueSystem->isDialogueActive());
    
    // When dialogue starts, pause but remember key states
    if (isDialogueActive && !wasInDialogue) {
        std::cout << "[DEBUG] Dialogue started - pausing movement" << std::endl;
        
        // Pause velocity - key states remain tracked
        auto playerEntities = m_entityManager.getEntities("player");
        if (!playerEntities.empty()) {
            auto player = playerEntities[0];
            player->get<CTransform>().velocity.x = 0.f;
        }
    }
    
    // Track key state changes regardless of dialogue
    if (action.name() == "MOVE_LEFT") {
        if (action.type() == "START") {
            leftKeyPressed = true;
            std::cout << "[DEBUG] leftKeyPressed set to TRUE" << std::endl;
        } else if (action.type() == "END") {
            std::cout << "[DEBUG] leftKeyPressed set to FALSE" << std::endl;
            leftKeyPressed = false;
        }
    }
    else if (action.name() == "MOVE_RIGHT") {
        if (action.type() == "START") {
            rightKeyPressed = true;
        } else if (action.type() == "END") {
            rightKeyPressed = false;
        }
    }
    
    // Block gameplay effects during dialogue but keep tracking key states
    if (isDialogueActive) {
        if (action.name() == "ATTACK" && action.type() == "START") {
            m_dialogueSystem->handleAttackAction();
        }
        
        wasInDialogue = true;
        return;
    }
    
    // When dialogue ends, apply movement based on CURRENT key states
    if (!isDialogueActive && wasInDialogue) {
        std::cout << "[DEBUG] Dialogue ended - applying current key states" << std::endl;
        
        auto playerEntities = m_entityManager.getEntities("player");
        if (!playerEntities.empty()) {
            auto player = playerEntities[0];
            auto& PTrans = player->get<CTransform>();
            auto& vel = PTrans.velocity;
            auto& state = player->get<CState>();
            
            // Apply movement based on currently pressed keys
            if (leftKeyPressed && !rightKeyPressed) {
                PTrans.facingDirection = -1.f;
                vel.x = -xSpeed;
                if (state.state != "air") {
                    state.state = "run";
                }
                std::cout << "[MOVEMENT] Continuing left movement after dialogue" << std::endl;
            } 
            else if (rightKeyPressed && !leftKeyPressed) {
                PTrans.facingDirection = 1.f;
                vel.x = xSpeed;
                if (state.state != "air") {
                    state.state = "run";
                }
                std::cout << "[MOVEMENT] Continuing right movement after dialogue" << std::endl;
            }
            else {
                vel.x = 0.f;
                if (state.state != "air") {
                    state.state = "idle";
                }
            }
        }
    }
    
    wasInDialogue = isDialogueActive;
    
    // Normal gameplay actions
    auto playerEntities = m_entityManager.getEntities("player");
    if (playerEntities.empty()) return;

    auto player = playerEntities[0];
    auto& PTrans = player->get<CTransform>();
    auto& vel = PTrans.velocity;  
    auto& state = player->get<CState>();

    bool inDefense = (state.state == "defense");
    bool hasFutureArmor = player->has<CPlayerEquipment>() && 
                          player->get<CPlayerEquipment>().hasFutureArmor;

    // Static flags for original system compatibility
    static bool isMovingLeft = false;
    static bool isMovingRight = false;

    // Keep these flags in sync with key state for compatibility
    isMovingLeft = leftKeyPressed;
    isMovingRight = rightKeyPressed;

    if (action.name() == "WINDOW_LOST_FOCUS") {
        // When app loses focus, reset all movement flags
        leftKeyPressed = false;
        rightKeyPressed = false;
        isMovingLeft = false;
        isMovingRight = false;
        vel.x = 0.f;
        if (state.state != "air") {
            state.state = "idle";
        }
        return;
    }
    
    if (action.type() == "START")
    {
        if (!inDefense)
        {
            if (action.name() == "MOVE_LEFT") {
                PTrans.facingDirection = -1.f;
                vel.x = -xSpeed;
                std::cout << "[MOVEMENT] Starting left movement, vel.x = " << vel.x << std::endl;
                if (state.state != "air") {
                    state.state = "run";
                }
            }
            else if (action.name() == "MOVE_RIGHT") {
                PTrans.facingDirection = 1.f;
                vel.x = xSpeed;
                std::cout << "[MOVEMENT] Starting right movement, vel.x = " << vel.x << std::endl;
                if (state.state != "air") {
                    state.state = "run";
                }
            }
            else if (action.name() == "JUMP") {
                if (state.onGround && 
                   (state.state == "idle" || 
                    state.state == "run" || 
                    state.state == "attack"))
                {
                    state.isJumping = true;
                    state.jumpTime  = 0.0f;
                    vel.y           = -ySpeed;
                    state.state     = "air";
                }
            }
        }

        if (action.name() == "TOGGLE_GRID") {
            m_showGrid = !m_showGrid;
        }
        else if (action.name() == "TOGGLE_BB") {
            m_showBoundingBoxes = !m_showBoundingBoxes;
        }
        else if (action.name() == "ATTACK") {
            // Check cooldowns first
            if (state.bulletCooldown > 0.f) {
                std::cout << "[DEBUG] Attack on cooldown! " << state.bulletCooldown << "s left.\n";
                return;
            }
            
            // For gun attacks, we don't use the attack cooldown anymore (as requested)
            if ((hasFutureArmor || state.attackCooldown <= 0.f)) {
                state.state = "attack";
                state.attackTime = 0.5f;
                
                // Only set attack cooldown for melee weapons
                if (!hasFutureArmor) {
                    state.attackCooldown = 0.5f;
                }
                
                if (hasFutureArmor) {
                    // Check for ammo if the ammo system is enabled
                    if (player->has<CAmmo>()) {
                        auto& ammo = player->get<CAmmo>();
                        if (ammo.currentBullets <= 0) {
                            std::cout << "[DEBUG] Out of ammo! Cannot attack.\n";
                            return;
                        }
                        // Consume a bullet
                        ammo.currentBullets--;
                        std::cout << "[DEBUG] Bullet fired. Ammo left: " << ammo.currentBullets << "\n";
                    }
                    
                    // Fire a bullet immediately for feedback
                    m_spawner.spawnPlayerBullet(player);
                    //state.bulletCooldown = 0.2f;
                    
                    // Enable burst mode so we can keep firing
                    state.inBurst        = true;
                    state.burstTimer     = 0.f;
                    state.burstFireTimer = 0.f;
                    state.bulletsShot    = 1;
                    std::cout << "[DEBUG] Burst started.\n";
                }
                else {
                    // First, destroy any existing sword
                    if (m_activeSword) {
                        m_activeSword->destroy();
                        m_activeSword = nullptr;
                    }
                    
                    // Then spawn a new sword and store the reference
                    m_activeSword = m_spawner.spawnSword(player);
                    state.bulletCooldown = 0.5f;
                    std::cout << "[DEBUG] Sword attack. Cooldown: 0.5s\n";
                }
            }
        }
        else if (action.name() == "SUPERMOVE") {
            // Super move logic
            if (state.bulletCooldown > 0.f) {
                std::cout << "[DEBUG] Super Move on cooldown! " << state.bulletCooldown << "s left.\n";
                return;
            }
            
            // Add check for super move readiness
            if (state.superBulletTimer <= 0.f && state.attackCooldown <= 0.f) {
                if (hasFutureArmor) {
                    state.state = "attack";
                    state.attackTime = 0.5f;
                    state.attackCooldown = 0.5f;
        
                    // Fire multiple bullets in a spread pattern
                    int bulletCount = state.superBulletCount;
                    float angleRange = 40.0f;
                    for (int i = 0; i < bulletCount; i++) {
                        auto bullet = m_spawner.spawnPlayerBullet(player);
                        if (bullet && bullet->has<CTransform>()) {
                            auto& bulletTrans = bullet->get<CTransform>();
                            float step = angleRange / (bulletCount - 1);
                            float angle = -angleRange * 0.5f + step * i;
                            bulletTrans.rotate(angle);
                        }
                    }
                    
                    // Set both cooldowns
                    state.bulletCooldown = 1.0f; // Normal cooldown for basic shots
                    
                    // Reset the super move timer
                    state.superBulletTimer = state.superBulletCooldown; // Reset super move cooldown
                    state.superMoveReady = false; // No longer ready
                    
                    std::cout << "[DEBUG] Super Move! Fired " << bulletCount << " bullets. Super cooldown reset to " 
                              << state.superBulletCooldown << "s\n";
                } 
                else {
                    std::cout << "[DEBUG] No future armor, can't perform Super Move.\n";
                }
            } else if (state.superBulletTimer > 0.f) {
                // Provide feedback that super move isn't ready yet
                std::cout << "[DEBUG] Super Move not ready yet. Cooldown remaining: " 
                          << state.superBulletTimer << "s\n";
            }
        }
        else if (action.name() == "DEFENSE") {
            // Cancel burst mode if active
            if (state.inBurst) {
                state.inBurst = false;
                state.burstTimer = 0.f;
                state.burstFireTimer = 0.f;
                state.bulletsShot = 0;
                std::cout << "[DEBUG] Burst canceled by DEFENSE.\n";
            }
            
            // Activate defense only if there's stamina left
            if (state.shieldStamina > 0.f && state.state != "defense") {
                std::cout << "[DEBUG] Defense activated.\n";
                state.state = "defense";
            }
        }
    }
    else if (action.type() == "END")
    {
        if (!inDefense) {
            if (action.name() == "MOVE_LEFT") {
                if (!rightKeyPressed) {
                    vel.x = 0.f;
                    if (state.state != "air") {
                        state.state = "idle";
                    }
                } else {
                    // Right is still pressed, so move right
                    PTrans.facingDirection = 1.f;
                    vel.x = xSpeed;
                    if (state.state != "air") {
                        state.state = "run";
                    }
                }
                std::cout << "[MOVEMENT] Stopping left movement, rightKeyPressed = " << rightKeyPressed << std::endl;
            }
            else if (action.name() == "MOVE_RIGHT") {
                if (!leftKeyPressed) {
                    vel.x = 0.f;
                    if (state.state != "air") {
                        state.state = "idle";
                    }
                } else {
                    // Left is still pressed, so move left
                    PTrans.facingDirection = -1.f;
                    vel.x = -xSpeed;
                    if (state.state != "air") {
                        state.state = "run";
                    }
                }
                std::cout << "[MOVEMENT] Stopping right movement, leftKeyPressed = " << leftKeyPressed << std::endl;
            }
            else if (action.name() == "JUMP") {
                state.isJumping = false;
            }
            else if (action.name() == "ATTACK") 
            {
                // If user releases ATTACK during a burst, cancel it
                if (state.inBurst) {
                    state.inBurst        = false;
                    state.burstTimer     = 0.f;
                    state.burstFireTimer = 0.f;
                    state.bulletsShot    = 0;
                    std::cout << "[DEBUG] Burst ended by releasing ATTACK.\n";
                }
            }
        }
        else if (action.name() == "DEFENSE") {
            // End defense when defense key is released
            if (state.state == "defense") {
                // Apply movement based on currently pressed keys when exiting defense
                if (leftKeyPressed && !rightKeyPressed) {
                    PTrans.facingDirection = -1.f;
                    vel.x = -xSpeed;
                    state.state = "run";
                    std::cout << "[MOVEMENT] Continuing left movement after defense" << std::endl;
                } 
                else if (rightKeyPressed && !leftKeyPressed) {
                    PTrans.facingDirection = 1.f;
                    vel.x = xSpeed;
                    state.state = "run";
                    std::cout << "[MOVEMENT] Continuing right movement after defense" << std::endl;
                }
                else {
                    vel.x = 0.f;
                    state.state = "idle";
                    std::cout << "[MOVEMENT] Returning to idle after defense" << std::endl;
                }
            }
        }
    }
}

void Scene_Play::sLifespan(float deltaTime)
{
    for (auto e : m_entityManager.getEntities())
    {
        if (e->has<CLifeSpan>())
        {
            // Only process lifespans for specific entity types
            if (e->tag() == "enemySword" || e->tag() == "EmperorSword" || 
                e->tag() == "playerBullet" || e->tag() == "enemyBullet" || 
                e->tag() == "fragment" || e->tag() == "effect")
            {
                auto& lifespan = e->get<CLifeSpan>();
                lifespan.remainingTime -= deltaTime;
                
                if (lifespan.remainingTime <= 0)
                {
                    //std::cout << "[DEBUG] Entity '" << e->tag() << "' #" << e->id() 
                    //          << " destroyed by lifespan system\n";
                    e->destroy();
                }
            }
            // For all other entities, we ignore their lifespan
        }
    }
}

void Scene_Play::sUpdateSword()
{
    auto playerEntities = m_entityManager.getEntities("player");
    if (playerEntities.empty() || !m_activeSword) return;

    auto player = playerEntities[0];
    auto& state = player->get<CState>();
    
    // If the attack animation is done or player state changed, destroy the sword
    if (m_activeSword && (state.attackTime <= 0.f || state.state != "attack")) {
        m_activeSword->destroy();
        m_activeSword = nullptr;
    }
}

void Scene_Play::sAmmoSystem(float dt)
{
    auto playerEntities = m_entityManager.getEntities("player");
    if (playerEntities.empty()) return;
    
    auto player = playerEntities[0];
    if (!player->has<CAmmo>()) return;
    
    // Only process if player has future armor
    if (!player->has<CPlayerEquipment>() || !player->get<CPlayerEquipment>().hasFutureArmor) {
        return;
    }
    
    auto& ammo = player->get<CAmmo>();
    
    // Check if we need to start reloading
    if (ammo.currentBullets <= 0 && !ammo.isReloading) {
        ammo.isReloading = true;
        ammo.currentReloadTime = 0.f;
        ammo.reloadTime = 3.0f; // 5 seconds for reload
        std::cout << "[DEBUG] Auto-reload started.\n";
    }
    
    // Handle reloading process
    if (ammo.isReloading) {
        ammo.currentReloadTime += dt;
        
        // Reload complete
        if (ammo.currentReloadTime >= ammo.reloadTime) {
            ammo.currentBullets = ammo.maxBullets;
            ammo.isReloading = false;
            std::cout << "[DEBUG] Reload complete. Bullets: " << ammo.currentBullets << "\n";
        }
    }
}

// Update the updateBurstFire function in Scene_Play.cpp

void Scene_Play::updateBurstFire(float deltaTime)
{
    // Get the player
    auto playerEntities = m_entityManager.getEntities("player");
    if (playerEntities.empty()) return;
    auto player = playerEntities[0];

    auto& state = player->get<CState>();

    // If not in a burst, do nothing
    if (!state.inBurst) return;

    // IMPORTANT FIX: Make sure player stays in attack state during burst
    if (state.state != "attack") {
        state.state = "attack";
        state.attackTime = 0.1f; // Small value to keep animation running
        std::cout << "[DEBUG] Restored attack state during burst fire" << std::endl;
    }

    // 1) Update timers
    state.burstTimer     += deltaTime;
    state.burstFireTimer += deltaTime;

    // 2) If the total burst time exceeded
    if (state.burstTimer >= state.burstDuration) {
        // End the burst
        state.inBurst        = false;
        state.burstTimer     = 0.f;
        state.burstFireTimer = 0.f;
        state.bulletsShot    = 0;
        // Let the animation system handle transitioning back to idle/run
        std::cout << "[DEBUG] Burst ended (time limit reached).\n";
        return;
    }

    // 3) Check if enough time has passed to fire again
    if (state.burstFireTimer >= state.burstInterval) 
    {
        // Check if we have ammo, if ammo system is enabled
        if (player->has<CAmmo>()) {
            auto& ammo = player->get<CAmmo>();
            if (ammo.currentBullets <= 0) {
                // End the burst if out of ammo
                state.inBurst = false;
                std::cout << "[DEBUG] Burst ended - out of ammo.\n";
                return;
            }
            
            // Consume a bullet
            ammo.currentBullets--;
        }
        
        // Reset the interval timer
        state.burstFireTimer = 0.f;
        state.bulletsShot++;

        // Keep the attack animation active
        state.attackTime = 0.1f;

        // Spawn a bullet
        m_spawner.spawnPlayerBullet(player);
        
        if (player->has<CAmmo>()) {
            auto& ammo = player->get<CAmmo>();
            std::cout << "[DEBUG] Burst bullet #" << state.bulletsShot << ", Ammo left: " << ammo.currentBullets << "\n";
        } else {
            std::cout << "[DEBUG] Burst bullet #" << state.bulletsShot << "\n";
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

    bool isOutOfBounds = transform.pos.y > 1800;
    bool isDead = (health.currentHealth <= 0);

    if (isOutOfBounds || isDead) {
        m_gameOver = true;
        m_game.changeScene("GAMEOVER", std::make_shared<Scene_GameOver>(m_game));
    }
}

std::string Scene_Play::extractLevelName(const std::string& path) {
    auto pos = path.find_last_of("/\\");
    if (pos == std::string::npos) return path; // nessuna slash
    return path.substr(pos + 1);
}

void Scene_Play::removeTileByID(const std::string& tileID) {
    auto tiles = m_entityManager.getEntities("tile");
    for (auto& t : tiles) {
        if (t->has<CUniqueID>()) {
            auto& uid = t->get<CUniqueID>();
            if (uid.id == tileID) {
                t->destroy();
                std::cout << "[DEBUG] Tile " << tileID << " distrutta!\n";
                break;
            }
        }
    }
}

void handleEmperorDeath(std::shared_ptr<Entity> emperor);

void Scene_Play::handleEmperorDeath(std::shared_ptr<Entity> emperor) {
    if (!emperor || !emperor->has<CEnemyAI>() || !emperor->has<CTransform>() || !emperor->has<CHealth>()) {
        return;
    }
    
    auto& enemyAI = emperor->get<CEnemyAI>();
    auto& enemyTrans = emperor->get<CTransform>();
    auto& health = emperor->get<CHealth>();
    
    // Only proceed if this is actually the Emperor and is dead
    if (health.currentHealth <= 0 && enemyAI.enemyType == EnemyType::Emperor) {
        // Set state to defeated if not already
        if (enemyAI.enemyState != EnemyState::Defeated) {
            enemyAI.enemyState = EnemyState::Defeated;
            
            // Get the position where the emperor died
            Vec2<float> deathPosition = enemyTrans.pos;
            
            // Spawn the emperor's grave at the same position
            m_spawner.spawnEnemyGrave(deathPosition, true); // true = isEmperor
            
            std::cout << "[DEBUG] Emperor defeated. Grave spawned at position (" 
                      << deathPosition.x << "," << deathPosition.y << ")\n";
            
            std::string levelName = extractLevelName(m_levelPath);
            if (levelName.find("emperor_room") != std::string::npos) {
                // Check world type to find the appropriate pipe ID
                if (m_game.worldType == "Future") {
                    removeTileByID("PipeTall_151"); // Future emperor room pipe
                } else {
                    removeTileByID("PipeTall_150"); // Ancient emperor room pipe
                }
            }
            
            // Destroy the emperor entity
            emperor->destroy();
        }
    }
}


void Scene_Play::lifeCheckEnemyDeath() {
    auto enemies = m_entityManager.getEntities("enemy");
    for (auto& enemy : enemies) {
        if (!enemy->has<CHealth>()) continue;

        auto& transform = enemy->get<CTransform>();
        const auto& health    = enemy->get<CHealth>();
        const auto& enemyAI = enemy->get<CEnemyAI>();

        bool isOutOfBounds = (transform.pos.y > 1800);
        bool isDead        = (health.currentHealth <= 0);

        // Skip if enemy is in defeated state
        if (enemyAI.enemyState == EnemyState::Defeated) {
            // Keep the bounding box, keep it on screen
            transform.velocity.x = 0.f;
            transform.velocity.y = 0.f;
            std::cout << "[DEBUG] (Level4) Emperor health critical, removing pipe...\n";
            removeTileByID("PipeTall_152");
            continue;
        }

        if (isOutOfBounds || isDead) {
            bool isEmperor = (enemy->has<CEnemyAI>() && enemy->get<CEnemyAI>().enemyType == EnemyType::Emperor);

            std::cout << "[DEBUG] Enemy ID " << enemy->id() << " is dead! Spawning " 
                      << (isEmperor ? "Emperor" : "normal") << " grave...\n";
            

            //Spawn the correct grave type
            m_spawner.spawnEnemyGrave(transform.pos, isEmperor);

            if (isEmperor) {
                // Use the specialized emperor death handler
                handleEmperorDeath(enemy);
            } else {
                // Handle unique enemy ID logic (e.g., tile removal)
                if (enemy->has<CUniqueID>()) {
                    auto& uniqueID = enemy->get<CUniqueID>();
                    std::string levelName = extractLevelName(m_levelPath);

                    std::cout << "[DEBUG] Checking tile removal for enemy ID: " << uniqueID.id << "\n";
                    
                    if (levelName == "ancient_rome_level_1_day.txt") {
                        if (uniqueID.id == "EnemyFast_4") {
                            std::cout << "[DEBUG] Removing tile PipeTall_275\n";
                            removeTileByID("PipeTall_275");
                        }
                    }
                    else if (levelName == "ancient_rome_level_2_sunset.txt") {
                        if (uniqueID.id == "EnemyStrong_12") {
                            std::cout << "[DEBUG] Removing tile PipeTall_900\n";
                            removeTileByID("PipeTall_900");
                        }
                    }
                }

                // Destroy the enemy
                enemy->destroy();
                std::cout << "[DEBUG] Enemy destroyed: ID = " << enemy->id() << "\n";
            }
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
