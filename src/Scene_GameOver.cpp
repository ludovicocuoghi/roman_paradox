#include "Scene_GameOver.h"
#include "Scene_Menu.h"
#include "Scene_Play.h"
#include "GameEngine.h"
#include <iostream>

Scene_GameOver::Scene_GameOver(GameEngine& game, const std::string& levelPath)
    : Scene(game)
    , m_levelPath(levelPath)
    , m_selectedOption(0)
{
    registerAction(sf::Keyboard::D, "SELECT");
    registerAction(sf::Keyboard::W, "UP");
    registerAction(sf::Keyboard::S, "DOWN");
    registerAction(sf::Keyboard::Q, "QUIT");
    
    // Store the current language from GameEngine
    m_language = game.getLanguage();
    // std::cout << "[DEBUG] Game Over scene using language: " << m_language << std::endl;
    
    // If the provided level path is empty, use the one from GameEngine
    if (m_levelPath.empty()) {
        m_levelPath = game.getCurrentLevel();
        // std::cout << "[DEBUG] Updated level path from GameEngine: " << m_levelPath << std::endl;
    }
    
    // std::cout << "[DEBUG] Game Over scene created with level path: " << m_levelPath << std::endl;
}

Scene_GameOver::~Scene_GameOver() {}

void Scene_GameOver::renderGameOverText() 
{
    // 1) GAME OVER title
    sf::Text gameOverText;
    gameOverText.setFont(m_game.assets().getFont("Japanese"));
    
    std::string gameOverString = (m_language == "Japanese") ? "ゲームオーバー" : "GAME OVER";
    // Convert from UTF-8 for SFML
    gameOverText.setString(sf::String::fromUtf8(
        gameOverString.begin(),
        gameOverString.end()
    ));

    gameOverText.setCharacterSize(60);
    gameOverText.setFillColor(sf::Color::Red);
    gameOverText.setStyle(sf::Text::Bold);

    // Center the text
    sf::FloatRect textBounds = gameOverText.getLocalBounds();
    gameOverText.setOrigin(textBounds.width / 2, textBounds.height / 2);
    gameOverText.setPosition(
        m_game.window().getSize().x / 2.0f,
        m_game.window().getSize().y / 2.0f - 100.0f
    );
    m_game.window().draw(gameOverText);

    // 2) Option 1: Restart Current Level
    sf::Text restartText;
    restartText.setFont(m_game.assets().getFont("Japanese"));
    
    std::string restartString = (m_language == "Japanese") 
        ? "現在のレベルをリスタート" 
        : "Restart Current Level";
    restartText.setString(sf::String::fromUtf8(
        restartString.begin(),
        restartString.end()
    ));

    restartText.setCharacterSize(30);
    restartText.setFillColor(m_selectedOption == 0 ? sf::Color::Yellow : sf::Color::White);

    sf::FloatRect restartBounds = restartText.getLocalBounds();
    restartText.setOrigin(restartBounds.width / 2, restartBounds.height / 2);
    restartText.setPosition(
        m_game.window().getSize().x / 2.0f,
        m_game.window().getSize().y / 2.0f
    );
    m_game.window().draw(restartText);

    // 3) Option 2: Manual Level Selection
    sf::Text menuText;
    menuText.setFont(m_game.assets().getFont("Japanese"));
    
    std::string menuString = (m_language == "Japanese")
        ? "レベル選択へ戻る"
        : "Manual Level Selection";
    menuText.setString(sf::String::fromUtf8(
        menuString.begin(),
        menuString.end()
    ));

    menuText.setCharacterSize(30);
    menuText.setFillColor(m_selectedOption == 1 ? sf::Color::Yellow : sf::Color::White);

    sf::FloatRect menuBounds = menuText.getLocalBounds();
    menuText.setOrigin(menuBounds.width / 2, menuBounds.height / 2);
    menuText.setPosition(
        m_game.window().getSize().x / 2.0f,
        m_game.window().getSize().y / 2.0f + 50.0f
    );
    m_game.window().draw(menuText);

    // 4) Instructions at bottom
    sf::Text instructionText;
    instructionText.setFont(m_game.assets().getFont("Japanese"));
    std::string instructionString = (m_language == "Japanese") 
        ? "W/Sキーで選択、Dキーで決定"
        : "Use W/S to select and D to confirm";
    instructionText.setString(sf::String::fromUtf8(
        instructionString.begin(),
        instructionString.end()
    ));
    
    instructionText.setCharacterSize(20);
    instructionText.setFillColor(sf::Color(200, 200, 200));

    sf::FloatRect instructionBounds = instructionText.getLocalBounds();
    instructionText.setOrigin(instructionBounds.width / 2, instructionBounds.height / 2);
    instructionText.setPosition(
        m_game.window().getSize().x / 2.0f,
        m_game.window().getSize().y / 2.0f + 120.0f
    );
    m_game.window().draw(instructionText);
}

void Scene_GameOver::sRender() 
{
    m_game.window().setView(m_game.window().getDefaultView());
    m_game.window().clear(sf::Color(30, 30, 30));
    
    renderGameOverText();
    
    m_game.window().display();
}

void Scene_GameOver::update(float deltaTime) 
{
    (void)deltaTime; 
}

void Scene_GameOver::sDoAction(const Action& action) 
{
    if (action.type() == "START") {
        if (action.name() == "UP" || action.name() == "DOWN") {
            // Toggle the selected option 0 <-> 1
            m_selectedOption = (m_selectedOption == 0) ? 1 : 0;
        }
        else if (action.name() == "SELECT") {
            if (m_selectedOption == 0) {
                // Restart current level
                if (!m_levelPath.empty()) {
                    std::cout << "[DEBUG] Restarting level: " << m_levelPath << std::endl;
                    m_game.restartLevel(m_levelPath);
                } else {
                    // fallback
                    std::string currentLevel = m_game.getCurrentLevel();
                    if (!currentLevel.empty()) {
                        std::cout << "[DEBUG] Using fallback: " << currentLevel << std::endl;
                        m_game.restartLevel(currentLevel);
                    } else {
                        goToLevelSelection();
                    }
                }
            } 
            else if (m_selectedOption == 1) {
                // Go to level selection
                goToLevelSelection();
            }
        }
        else if (action.name() == "QUIT") {
            m_game.window().close();
        }
    }
}
void Scene_GameOver::goToLevelSelection()
{
    auto menuScene = std::make_shared<Scene_Menu>(m_game);
    // Scene_Menu constructor will do:
    //   m_language = m_game.getLanguage();
    //   updateMenuTranslations();

    // If you want to jump directly to the LEVEL_SELECT state:
    if (auto* menuPtr = menuScene.get()) {
        menuPtr->setMenuState(Scene_Menu::MenuState::LEVEL_SELECT);
    }
    m_game.changeScene("MENU", menuScene);
}
