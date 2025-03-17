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
#include "systems/Spawner.h"

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
      m_enemyAISystem(m_entityManager, m_spawner, m_game)
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
    
    // Mapping of levels to backgrounds
    if (levelName == "alien_rome_level_1.txt") {
        m_backgroundPath = "src/images/Background/alien_rome/alien_rome_phase_1.png";
        m_timeofday = "ALIEN EMPIRE";
    } else if (levelName == "alien_rome_level_2.txt") {
        m_backgroundPath = "src/images/Background/alien_rome/alien_rome_phase_2.png";
        m_timeofday = "ALIEN EMPIRE";
    } else if (levelName == "ancient_rome_level_1_day.txt") {
        m_backgroundPath = "src/images/Background/ancient_rome/ancient_rome_level_1_day.png";
        m_timeofday = "ANCIENT ROME (DAY)";
    } else if (levelName == "ancient_rome_level_2_sunset.txt") {
        m_backgroundPath = "src/images/Background/ancient_rome/ancient_rome_level_2_sunset.png";
        m_timeofday = "ANCIENT ROME (SUNSET)";
    } else if (levelName == "ancient_rome_level_3_night.txt") {
        m_backgroundPath = "src/images/Background/ancient_rome/ancient_rome_level_3_night.png";
        m_timeofday = "ANCIENT ROME (NIGHT)";
    } else if (levelName == "ancient_rome_level_4_emperor_room.txt") {
        m_backgroundPath = "src/images/Background/ancient_rome/ancient_rome_level_4_emperor_room.png";
        m_timeofday = "EMPEROR ROOM";
    } else if (levelName == "ancient_rome_level_5_day_v2.txt") {
        m_backgroundPath = "src/images/Background/ancient_rome/ancient_rome_level_1_day.png";
        m_timeofday = "ANCIENT ROME (DAY 2)";
    } else if (levelName == "future_rome_level_1.txt") {
        m_backgroundPath = "src/images/Background/future_rome/morning_future_3.png";
        m_timeofday = "FUTURE ROME (DAY)";
    } else if (levelName == "future_rome_level_2.txt") {
        m_backgroundPath = "src/images/Background/future_rome/future_sunset3.png";
        m_timeofday = "FUTURE ROME (SUNSET)";
    } else if (levelName == "future_rome_level_3.txt") {
        m_backgroundPath = "src/images/Background/future_rome/future_rome_night.png";
        m_timeofday = "FUTURE ROME (NIGHT)";
    } else if (levelName == "future_rome_level_emperor_room.txt") {
        m_backgroundPath = "src/images/Background/future_rome/emperor_room.png";
        m_timeofday = "FUTURE EMPEROR ROOM";
    }  
    else {
        // Default background if level is not mapped
        m_backgroundPath = "src/images/Background/default.png";
        m_timeofday = "Unknown";
    }
}

void Scene_Play::init()
{
    std::cout << "[DEBUG] Scene_Play::init() - Start\n";

    registerCommonActions();
    registerAction(sf::Keyboard::T, "TOGGLE_TEXTURE");
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

    // ✅ Ensure entity manager is clean before loading
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
    
    // Add dialogue triggers based on the current level
    std::string levelName = extractLevelName(m_levelPath);
    
    // Setup dialogue triggers based on level
    if (levelName == "alien_rome_level_1.txt") {
        std::vector<DialogueMessage> Dialogue1 = {
            {
                "????",                               // speaker
                "I FINALLY FOUND YOU!!",            // message
                "bin/images/Portraits/future_fast.png",        // portraitPath
                false,                                            // portraitOnLeft
                sf::Color::Cyan,
                sf::Color::Red,
                sf::Vector2f(800.f, 600.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                40,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            },
            {
                "????",                               // speaker
                "YOU DAMNED ALIEN...",            // message
                "bin/images/Portraits/future_fast.png",        // portraitPath
                false,                                            // portraitOnLeft
                sf::Color::Cyan,
                sf::Color::Red,
                sf::Vector2f(800.f, 600.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                40,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            },
            {
                "????",                               // speaker
                "YOU WILL PAY FOR THIS!!!",            // message
                "bin/images/Portraits/future_fast.png",        // portraitPath
                false,                                            // portraitOnLeft
                sf::Color::Cyan,
                sf::Color::Red,
                sf::Vector2f(800.f, 600.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                35,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            },
            {
                "Alien Centurion",                               // speaker
                "Who are you ??!",            // message
                "bin/images/Portraits/alien_ancient.png",        // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(50.f, 600.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                30,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            },
        };
        m_dialogueSystem->addDialogueTrigger(1300, Dialogue1);
        std::vector<DialogueMessage> Dialogue2 = {
            {
                "Alien Centurion",                               // speaker
                "Have I met him before?",            // message
                "bin/images/Portraits/alien_ancient.png",        // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(50.f, 600.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                30,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            }
        };
        m_dialogueSystem->addDialogueTrigger(1700, Dialogue2);
        std::vector<DialogueMessage> introDialogue = {
            {
                "Alien Centurion",                               // speaker
                "HELP!!!",                                       // message
                "bin/images/Portraits/alien_ancient_right.png",  // portraitPath
                false,                                           // portraitOnLeft
                sf::Color::Yellow,                               // speakerColor
                sf::Color::Red,                                  // messageColor
                sf::Vector2f(800.f, 500.f),                      // dialogueBoxPosition
                650.f,                                           // boxWidth - compact for short message
                150.f,                                           // boxHeight
                40,                                              // messageFontSize - larger for emphasis
                true                                            // useTypewriterEffect - immediate display
            },
            {
                "Alien Centurion",
                "WE ARE BEING INVADED!!",
                "bin/images/Portraits/alien_ancient_right.png",
                false,
                sf::Color::Yellow,
                sf::Color::Red,
                sf::Vector2f(800.f, 500.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                40,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            }
        };
        m_dialogueSystem->addDialogueTrigger(2000, introDialogue);
        std::vector<DialogueMessage> Dialogue3 = {
            {
                "Alien Centurion",                               // speaker
                "Who are these black warriors???",            // message
                "bin/images/Portraits/alien_ancient_left.png",        // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(50.f, 500.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                30,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            },
            {
                "Alien Centurion",                               // speaker
                "Why are they invading us??",            // message
                "bin/images/Portraits/alien_ancient_left.png",        // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(50.f, 500.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                30,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            }
        };
        m_dialogueSystem->addDialogueTrigger(4500, Dialogue3);
        std::vector<DialogueMessage> Dialogue4 = {
            {
                "Alien Centurion",                               // speaker
                "They can destroy whatever they hit...",            // message
                "bin/images/Portraits/alien_ancient_left.png",        // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(150.f, 500.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                25,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            },
            {
                "Alien Centurion",                               // speaker
                "I'd better not get close to them...?!",            // message
                "bin/images/Portraits/alien_ancient_left.png",        // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(150.f, 500.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                26,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            }
        };
        m_dialogueSystem->addDialogueTrigger(6500, Dialogue4);
        std::vector<DialogueMessage> Dialogue5 = {
            {
                "Alien Centurion",                               // speaker
                "AAAHH!!",            // message
                "bin/images/Portraits/alien_ancient_right.png",        // portraitPath
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
                "Alien Centurion",                               // speaker
                "HELP!!!",            // message
                "bin/images/Portraits/alien_ancient_right.png",        // portraitPath
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
    }
    else if (levelName ==  "alien_rome_level_2.txt") {
        std::vector<DialogueMessage> Dialogue0 = {
            {
                "?????",                               // speaker
                "The Invaders must..",            // message
                "bin/images/Portraits/alien_super2.png",        // portraitPath
                false,                                            // portraitOnLeft
                sf::Color::Red,
                sf::Color::White,
                sf::Vector2f(600.f, 300.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                35,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            },
            {
                "?????",                               // speaker
                "PERISH!!!!!",            // message
                "bin/images/Portraits/alien_super2.png",        // portraitPath
                false,                                            // portraitOnLeft
                sf::Color::Red,
                sf::Color::White,
                sf::Vector2f(600.f, 300.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                40,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            },
            {
                "Alien Centurion",                               // speaker
                "Invaders??",            // message
                "bin/images/Portraits/alien_ancient.png",        // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(50.f, 300.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                40,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            }
            ,
            {
                "Alien Centurion",                               // speaker
                "What is he talking about??",            // message
                "bin/images/Portraits/alien_ancient.png",        // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(50.f, 300.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                35,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            }
        };
        m_dialogueSystem->addDialogueTrigger(1450, Dialogue0);
        std::vector<DialogueMessage> Dialogue1 = {
            {
                "Alien Centurion",                               // speaker
                "Those weapons look like black holes...",            // message
                "bin/images/Portraits/alien_ancient.png",        // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(50.f, 300.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                26,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            },
            {
                "Alien Centurion",                               // speaker
                "could they be coming from the...",            // message
                "bin/images/Portraits/alien_ancient.png",        // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(50.f, 300.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                26,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            },
            {
                "Alien Centurion",                               // speaker
                "...Future?!",            // message
                "bin/images/Portraits/alien_ancient.png",        // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(50.f, 300.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                40,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            }
        };
        m_dialogueSystem->addDialogueTrigger(2200, Dialogue1);
        std::vector<DialogueMessage> Dialogue2 = {
            {
                "Alien Centurion",                               // speaker
                "I saw them coming out of that black hole!!!",            // message
                "bin/images/Portraits/alien_ancient_right.png",        // portraitPath
                false,                                            // portraitOnLeft
                sf::Color::Yellow,
                sf::Color::White,
                sf::Vector2f(750.f, 400.f),
                750.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                26,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            },
            {
                "Alien Centurion",                               // speaker
                "Maybe its the only way to escape!!",            // message
                "bin/images/Portraits/alien_ancient_right.png",        // portraitPath
                false,                                            // portraitOnLeft
                sf::Color::Yellow,
                sf::Color::White,
                sf::Vector2f(750.f, 400.f),
                750.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                26,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            },
            {
                "Alien Centurion",                               // speaker
                "Should I jump there?!?",            // message
                "bin/images/Portraits/alien_ancient.png",        // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(0.f, 400.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                26,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            },
        };
        m_dialogueSystem->addDialogueTrigger(11300, Dialogue2);
        std::vector<DialogueMessage> Dialogue3 = {
            {
                "????",                               // speaker
                "ATTACK HIM BEFORE HE ESCAPE!!!",            // message
                "bin/images/Portraits/alien_super2.png",        // portraitPath
                false,                                            // portraitOnLeft
                sf::Color::Red,
                sf::Color::White,
                sf::Vector2f(0.f, 300.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                26,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            },
            {
                "Alien Centurion",                               // speaker
                "I dont any other choice then!!!",            // message
                "bin/images/Portraits/alien_ancient.png",        // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(200.f, 500.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                26,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            },
            {
                "Alien Centurion",                               // speaker
                "I MUST JUMP IN THE BLACK HOLE!!!!",            // message
                "bin/images/Portraits/alien_ancient.png",        // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(200.f, 500.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                26,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            }
        };
        m_dialogueSystem->addDialogueTrigger(13500, Dialogue3);
    }
    else if (levelName ==  "ancient_rome_level_1_day.txt") {
        std::vector<DialogueMessage> Dialogue0 = {
            {
                "Alien Centurion",                               // speaker
                "Where am I???",            // message
                "bin/images/Portraits/alien_ancient.png",        // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(0.f, 300.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                35,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            }
        };
        m_dialogueSystem->addDialogueTrigger(850, Dialogue0);
        std::vector<DialogueMessage> Dialogue1 = {
            {
                "Alien Centurion",                               // speaker
                "I heard that black hole make you move through dimensions..",            // message
                "bin/images/Portraits/alien_ancient.png",        // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(0.f, 400.f),
                950.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                26,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            },
            {
                "Alien Centurion",                               // speaker
                "Could it be possible that I moved to the black warrios..",            // message
                "bin/images/Portraits/alien_ancient.png",        // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(0.f, 400.f),
                900.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                26,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            },
            {
                "Alien Centurion",                               // speaker
                "PAST????",            // message
                "bin/images/Portraits/alien_ancient.png",        // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(50.f, 400.f),
                900.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                50,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            },
        };
        m_dialogueSystem->addDialogueTrigger(1000, Dialogue1);
        std::vector<DialogueMessage> Dialogue2 = {
            {
                "Alien Centurion",                               // speaker
                "Oh, there are golden stairs up there..",            // message
                "bin/images/Portraits/alien_ancient.png",        // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(50.f, 400.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                26,                                              // messageFontSize - still emphasized
               
            },
            {
                "Alien Centurion",                               // speaker
                "They seem to lead to the black hole..",            // message
                "bin/images/Portraits/alien_ancient.png",        // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(50.f, 400.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                26,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            },
            {
                "Alien Centurion",                               // speaker
                "But how can I get there??",            // message
                "bin/images/Portraits/alien_ancient.png",        // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(50.f, 400.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                26,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            },
            {
                "Alien Centurion",                               // speaker
                "I guess I should continue exploring..",            // message
                "bin/images/Portraits/alien_ancient.png",        // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(50.f, 400.f),
                800.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                26,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            },
            {
                "Alien Centurion",                               // speaker
                "..and find a way to go back to the present",            // message
                "bin/images/Portraits/alien_ancient.png",        // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(50.f, 400.f),
                800.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                26,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            },
        };
        m_dialogueSystem->addDialogueTrigger(1800, Dialogue2);
        std::vector<DialogueMessage> Dialogue4 = {
            {
                "Ancient Centurion",                               // speaker
                "WHO ARE YOU??",            // message
                "bin/images/Portraits/ancient_normal.png",        // portraitPath
                false,                                            // portraitOnLeft
                sf::Color::Yellow,
                sf::Color::White,
                sf::Vector2f(850.f, 200.f),
                600.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                40,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            },
            {
                "Ancient Centurion",                               // speaker
                "GET OUT OF HERE!!!",            // message
                "bin/images/Portraits/ancient_normal.png",        // portraitPath
                false,                                            // portraitOnLeft
                sf::Color::Yellow,
                sf::Color::White,
                sf::Vector2f(850.f, 200.f),
                600.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                40,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            },
        };
        m_dialogueSystem->addDialogueTrigger(2400, Dialogue4);
        std::vector<DialogueMessage> Dialogue5 = {
            {
                "Ancient Centurion",                               // speaker
                "WE ARE BEING INVADED!",            // message
                "bin/images/Portraits/ancient_normal.png",        // portraitPath
                false,                                            // portraitOnLeft
                sf::Color::Yellow,
                sf::Color::White,
                sf::Vector2f(850.f, 200.f),
                600.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                40,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            },
            {
                "Ancient Centurion",                               // speaker
                "SEND THE REINFORCEMENTS!!!",            // message
                "bin/images/Portraits/ancient_normal.png",        // portraitPath
                false,                                            // portraitOnLeft
                sf::Color::Yellow,
                sf::Color::White,
                sf::Vector2f(850.f, 200.f),
                800.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                40,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            },
            {
                "Alien Centurion",                               // speaker
                "Oh no.. is this for REAL?!",            // message
                "bin/images/Portraits/alien_ancient.png",        // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(50.f, 200.f),
                600.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                26,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            },
        };
        m_dialogueSystem->addDialogueTrigger(3000, Dialogue5);
    }
    else if (levelName ==  "ancient_rome_level_3_night.txt") {
        std::vector<DialogueMessage> Dialogue0 = {
            {
                "Ancient Centurion (Elite)",                               // speaker
                "Are you the invader?",            // message
                "bin/images/Portraits/ancient_elite.png",        // portraitPath
                false,                                            // portraitOnLeft
                sf::Color::Red,
                sf::Color::White,
                sf::Vector2f(800.f, 500.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                35,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            },
            {
                "Ancient Centurion (Elite)",                                // speaker
                "How dare you coming to this land?",            // message
                "bin/images/Portraits/ancient_elite.png",        // portraitPath
                false,                                            // portraitOnLeft
                sf::Color::Red,
                sf::Color::White,
                sf::Vector2f(800.f, 500.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                30,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            },
            {
                "Ancient Centurion (Elite)",                              // speaker
                "We will protect the emperor..",            // message
                "bin/images/Portraits/ancient_elite.png",        // portraitPath
                false,                                            // portraitOnLeft
                sf::Color::Red,
                sf::Color::White,
                sf::Vector2f(800.f, 500.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                30,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            }
            ,
            {
                "Ancient Centurion (Elite)",                               // speaker
                "AT ALL COSTS!!",            // message
                "bin/images/Portraits/ancient_elite.png",        // portraitPath
                false,                                            // portraitOnLeft
                sf::Color::Red,
                sf::Color::Red,
                sf::Vector2f(800.f, 500.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                50,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            }
        };
        m_dialogueSystem->addDialogueTrigger(300, Dialogue0);
        std::vector<DialogueMessage> Dialogue1 = {
            {
                "Ancient Centurion (Elite)",                               // speaker
                "STOP THE INVADER!!",            // message
                "bin/images/Portraits/ancient_elite.png",        // portraitPath
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
                "Alien Centurion",                               // speaker
                "Does this golden portal lead to the emperor?!",            // message
                "bin/images/Portraits/alien_ancient.png",        // portraitPath
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
                "Alien Centurion",                               // speaker
                "There's only way to find it out!!",            // message
                "bin/images/Portraits/alien_ancient.png",        // portraitPath
                true,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(750.f, 400.f),
                750.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                26,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            }

        };
        m_dialogueSystem->addDialogueTrigger(11200, Dialogue2);
    }
    else if (levelName ==  "ancient_rome_level_4_emperor_room.txt") {
        std::vector<DialogueMessage> Dialogue0 = {
            {
                "Ancient Emperor",                               // speaker
                "...And so you eventually got here..invader...",            // message
                "bin/images/Portraits/ancient_emperor.png",        // portraitPath
                false,                                            // portraitOnLeft
                sf::Color::Yellow,
                sf::Color::White,
                sf::Vector2f(800.f, 500.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                20,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            },
            {
                "Ancient Emperor",                             // speaker
                "I am the Emperor of this kingdom..",            // message
                "bin/images/Portraits/ancient_emperor.png",        // portraitPath
                false,                                            // portraitOnLeft
                sf::Color::Yellow,
                sf::Color::White,
                sf::Vector2f(800.f, 500.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                20,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            },
            {
                "Ancient Emperor",                             // speaker
                "how dare you coming here...",            // message
                "bin/images/Portraits/ancient_emperor.png",        // portraitPath
                false,                                            // portraitOnLeft
                sf::Color::Yellow,
                sf::Color::White,
                sf::Vector2f(800.f, 500.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                20,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            }
            ,
            {
                "Alien Centurion",                               // speaker
                "I came from the future...!!",            // message
                "bin/images/Portraits/alien_ancient.png",       // portraitPath
                false,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(0.f, 500.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                20,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            }
            ,
            {
                "Alien Centurion",                              // speaker
                "Your future kingdom invaded my planet",            // message
                "bin/images/Portraits/alien_ancient.png",       // portraitPath
                false,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(0.f, 500.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                20,                                             // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            }
            ,
            {
                "Alien Centurion",                              // speaker
                "And I came here trough a black hole while escaping..",            // message
                "bin/images/Portraits/alien_ancient.png",       // portraitPath
                false,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(0.f, 500.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                20,                                             // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            }
            ,
            {
                "Ancient Emperor",                             // speaker
                "What are you even talking about?!?",            // message
                "bin/images/Portraits/ancient_emperor.png",        // portraitPath
                false,                                            // portraitOnLeft
                sf::Color::Yellow,
                sf::Color::White,
                sf::Vector2f(800.f, 500.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                20,                                             // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            },
            {
                "Ancient Emperor",                             // speaker
                "Black holes?! How I could believe you..",            // message
                "bin/images/Portraits/ancient_emperor.png",        // portraitPath
                false,                                            // portraitOnLeft
                sf::Color::Yellow,
                sf::Color::White,
                sf::Vector2f(800.f, 500.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                20,                                             // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            }
            ,
            {
                "Ancient Emperor",                               // speaker
                "YOU WILL PERISH!!!",            // message
                "bin/images/Portraits/ancient_emperor.png",        // portraitPath
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
        m_dialogueSystem->addDialogueTrigger(300, Dialogue0);
        std::vector<DialogueMessage> Dialogue1 = {
            {
                "Alien Centurion",                               // speaker
                "It seems like there is a way to escape back there..!!",            // message
               "bin/images/Portraits/alien_ancient.png",       // portraitPath
                false,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(0.f, 500.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                20,                                             // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            },
            {
                "Alien Centurion",                               // speaker
                "but it seems blocked...",            // message
                "bin/images/Portraits/alien_ancient.png",       // portraitPath
                false,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(0.f, 500.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                20,                                             // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            },
            {
                "Alien Centurion",                             // speaker
                "As expected the only way to go back is to..",            // message
                "bin/images/Portraits/alien_ancient.png",        // portraitPath
                false,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(0.f, 500.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                20,                                           // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            }
            ,
            {
                "Alien Centurion",                               // speaker
                "defeat him!!!",            // message
                "bin/images/Portraits/alien_ancient.png",       // portraitPath
                false,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(0.f, 500.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                20,                                             // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            }
            ,
        };
        m_dialogueSystem->addDialogueTrigger(1000, Dialogue1);
        std::vector<DialogueMessage> LowHPAncientEmperorDialogue = {
            {
                "EMPEROR",                               // speaker
                "I have no chance but to show you my true..",                           // message
                "bin/images/Portraits/ancient_emperor.png", // portraitPath
                false,                                   // portraitOnLeft
                sf::Color::Cyan,                         // speakerColor
                sf::Color::White,                          // messageColor
                sf::Vector2f(800.f, 550.f),              // dialogueBoxPosition
                650.f,                                   // boxWidth
                150.f,                                   // boxHeight
                25,                                      // messageFontSize
                true                                     // useTypewriterEffect
            },
            {
                "EMPEROR",                               // speaker
                "POWER!",                           // message
                "bin/images/Portraits/ancient_emperor.png", // portraitPath
                false,                                   // portraitOnLeft
                sf::Color::Cyan,                         // speakerColor
                sf::Color::Red,                          // messageColor
                sf::Vector2f(800.f, 550.f),              // dialogueBoxPosition
                650.f,                                   // boxWidth
                150.f,                                   // boxHeight
                50,                                      // messageFontSize
                true                                     // useTypewriterEffect
            }
        };
        // Add the named dialogues to our system
        m_dialogueSystem->addNamedDialogue("emperor_lowHP", LowHPAncientEmperorDialogue);
        std::vector<DialogueMessage> phasedefeatedAncientDialogue = {
            {
                "Ancient Emperor",                              // speaker
                "You.. damned... invader...",            // message
                "bin/images/Portraits/ancient_emperor_defeated.png",       // portraitPath
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
                "Alien Centurion",                             // speaker
                "I cannot get close to him....",            // message
                "bin/images/Portraits/alien_ancient.png",        // portraitPath
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
                "Alien Centurion",                             // speaker
                "but he seems out of energy.....",            // message
                "bin/images/Portraits/alien_ancient.png",        // portraitPath
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
                "Alien Centurion",                             // speaker
                "This is the right time to escape!!",            // message
                "bin/images/Portraits/alien_ancient.png",        // portraitPath
                false,                                            // portraitOnLeft
                sf::Color::Magenta,
                sf::Color::White,
                sf::Vector2f(600.f, 400.f),
                650.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                30,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            },

        };
        m_dialogueSystem->addNamedDialogue("emperor_ancient_defeated", phasedefeatedAncientDialogue);
    }
    else if (levelName == "future_rome_level_emperor_room.txt") {
        std::vector<DialogueMessage> bossDialogue = {
            {
                "EMPEROR",
                "... And so you are back...",
                "bin/images/Portraits/future_emperor.png",
                false,                                    // portraitOnLeft (right side)
                sf::Color::Cyan,                           // speakerColor
                sf::Color::White,                         // messageColor
                sf::Vector2f(800.f, 550.f),                // dialogueBoxPosition
                500.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                30,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            },
            {
                "EMPEROR",
                "I remember you... ",
                "bin/images/Portraits/future_emperor.png",
                false,
                sf::Color::Cyan,
                sf::Color::White,
                sf::Vector2f(800.f, 550.f),
                500.f,                                           // boxWidth - medium for medium message
                150.f,                                           // boxHeight
                30,                                              // messageFontSize - still emphasized
                true                                             // useTypewriterEffect
            }
        };
        m_dialogueSystem->addDialogueTrigger(500, bossDialogue);
        std::vector<DialogueMessage> bossDialogue2 = {
            {
                "EMPEROR",
                "Thousands of years ago you came here.. ",
                "bin/images/Portraits/future_emperor.png",
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
                "EMPEROR",
                ".. and I barely survived your wrath.",
                "bin/images/Portraits/future_emperor.png",
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
                "EMPEROR",
                "But this time..",
                "bin/images/Portraits/future_emperor.png",
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
                "EMPEROR",
                "I will make you pay for it!!!",
                "bin/images/Portraits/future_emperor.png",
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
                "EMPEROR",                               // speaker
                "You are stronger than I expected...",    // message
                "bin/images/Portraits/future_emperor_2.png", // portraitPath (adjust path as needed)
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
                "EMPEROR",
                "But this is just the beginning!!",
                "bin/images/Portraits/future_emperor_2.png",
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
                "EMPEROR",                               // speaker
                "HOW IS THIS POSSIBLE??",                           // message
                "bin/images/Portraits/future_emperor_3.png", // portraitPath
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
                "EMPEROR",
                "IT'S NOT OVER YET!!!",
                "bin/images/Portraits/future_emperor_3.png",
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
                "EMPEROR",                               // speaker
                "IMP055IBLE!!!",                           // message
                "bin/images/Portraits/future_emperor_3.png", // portraitPath
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
                "EMPEROR",
                "TIME T0 SH0W Y0U MY...",
                "bin/images/Portraits/future_emperor_3.png",
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
                "EMPEROR",
                "TRUE POWER!",
                "bin/images/Portraits/future_emperor_3.png",
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
                "EMPEROR",                               // speaker
                "...",                           // message
                "bin/images/Portraits/future_emperor_3.png", // portraitPath
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
    
    std::cout << "[DEBUG] Dialogue system initialized for level: " << levelName << std::endl;
}
// Action Processing (Input Handling)                        

void Scene_Play::sDoAction(const Action& action)
{
    // Check if dialogue is active - if so, only process ATTACK to advance dialogue
    if (m_dialogueSystem && m_dialogueSystem->isDialogueActive()) {
        if (action.name() == "ATTACK" && action.type() == "START") {
            // Direct call to advance dialogue
            m_dialogueSystem->handleAttackAction();
        }
        // Block all other actions during dialogue
        return;
    }

    // Normal gameplay actions - only process if dialogue is not active
    auto playerEntities = m_entityManager.getEntities("player");
    if (playerEntities.empty()) return;

    auto player = playerEntities[0];
    auto& PTrans   = player->get<CTransform>();
    auto& vel      = PTrans.velocity;  
    auto& state    = player->get<CState>();

    bool inDefense = (state.state == "defense");
    bool hasFutureArmor = player->has<CPlayerEquipment>() && 
                          player->get<CPlayerEquipment>().hasFutureArmor;

    static bool isMovingLeft  = false;
    static bool isMovingRight = false;
    

    if (action.type() == "START")
    {
        if (!inDefense)
        {
            if (action.name() == "MOVE_LEFT") {
                isMovingLeft    = true;
                isMovingRight   = false;
                PTrans.facingDirection = -1.f;
                vel.x = -xSpeed;
                if (state.state != "air") {
                    state.state = "run";
                }
            }
            else if (action.name() == "MOVE_RIGHT") {
                isMovingRight   = true;
                isMovingLeft    = false;
                PTrans.facingDirection = 1.f;
                vel.x = xSpeed;
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
                isMovingLeft = false;
                if (!isMovingRight) {
                    vel.x = 0.f;
                    if (state.state != "air") {
                        state.state = "idle";
                    }
                }
            }
            else if (action.name() == "MOVE_RIGHT") {
                isMovingRight = false;
                if (!isMovingLeft) {
                    vel.x = 0.f;
                    if (state.state != "air") {
                        state.state = "idle";
                    }
                }
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
                state.state = "idle";
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
                break; // esci dopo averla trovata
            }
        }
    }
}

// Add this function to your Scene_Play class declaration in Scene_Play.h
void handleEmperorDeath(std::shared_ptr<Entity> emperor);

// Add this implementation to Scene_Play.cpp
void Scene_Play::handleEmperorDeath(std::shared_ptr<Entity> emperor) {
    // Check if the emperor entity exists and has the required components
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
            
            // If we're in level 4, remove the pipe
            std::string levelName = extractLevelName(m_levelPath);
            if (levelName.find("emperor_room") != std::string::npos) {
                // This is specific to emperor room levels
                std::cout << "[DEBUG] Emperor defeated in throne room, removing exit pipe...\n";
                
                // Check world type to find the appropriate pipe ID
                if (m_game.worldType == "Future") {
                    removeTileByID("PipeTall_152"); // Future emperor room pipe
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
            

            // ✅ Spawn the correct grave type
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
