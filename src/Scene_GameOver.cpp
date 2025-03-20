#include "Scene_GameOver.h"
#include "Scene_Menu.h"
#include "Scene_Play.h"
#include "GameEngine.h"
#include <iostream>

Scene_GameOver::Scene_GameOver(GameEngine& game, const std::string& levelPath)
    : Scene(game), m_levelPath(levelPath), m_selectedOption(0)
{
    registerAction(sf::Keyboard::D, "SELECT");
    registerAction(sf::Keyboard::W, "UP");
    registerAction(sf::Keyboard::S, "DOWN");
    registerAction(sf::Keyboard::Q, "QUIT");
    
    // If the provided level path is empty, use the one stored in GameEngine
    if (m_levelPath.empty()) {
        m_levelPath = game.getCurrentLevel();
        std::cout << "[DEBUG] Updated level path from GameEngine: " << m_levelPath << std::endl;
    }
    
    std::cout << "[DEBUG] Game Over scene created with level path: " << m_levelPath << std::endl;
}

Scene_GameOver::~Scene_GameOver() {}

void Scene_GameOver::renderGameOverText() {
    sf::Text gameOverText;
    gameOverText.setFont(m_game.assets().getFont("Menu"));
    gameOverText.setString("GAME OVER");
    gameOverText.setCharacterSize(60);
    gameOverText.setFillColor(sf::Color::Red);
    gameOverText.setStyle(sf::Text::Bold);

    // Center-align the text
    sf::FloatRect textBounds = gameOverText.getLocalBounds();
    gameOverText.setOrigin(textBounds.width / 2, textBounds.height / 2);
    gameOverText.setPosition(m_game.window().getSize().x / 2, m_game.window().getSize().y / 2 - 100);

    m_game.window().draw(gameOverText);

    // Option 1: Restart Current Level
    sf::Text restartText;
    restartText.setFont(m_game.assets().getFont("Menu"));
    restartText.setString("Restart Current Level");
    restartText.setCharacterSize(30);
    restartText.setFillColor(m_selectedOption == 0 ? sf::Color::Yellow : sf::Color::White);

    sf::FloatRect restartBounds = restartText.getLocalBounds();
    restartText.setOrigin(restartBounds.width / 2, restartBounds.height / 2);
    restartText.setPosition(m_game.window().getSize().x / 2, m_game.window().getSize().y / 2);

    m_game.window().draw(restartText);

    // Option 2: Level Selection
    sf::Text menuText;
    menuText.setFont(m_game.assets().getFont("Menu"));
    menuText.setString("Manual Level Selection");
    menuText.setCharacterSize(30);
    menuText.setFillColor(m_selectedOption == 1 ? sf::Color::Yellow : sf::Color::White);

    sf::FloatRect menuBounds = menuText.getLocalBounds();
    menuText.setOrigin(menuBounds.width / 2, menuBounds.height / 2);
    menuText.setPosition(m_game.window().getSize().x / 2, m_game.window().getSize().y / 2 + 50);

    m_game.window().draw(menuText);

    // Instructions
    sf::Text instructionText;
    instructionText.setFont(m_game.assets().getFont("Menu"));
    instructionText.setString("Use W/S to select and D to confirm");
    instructionText.setCharacterSize(20);
    instructionText.setFillColor(sf::Color(200, 200, 200));

    sf::FloatRect instructionBounds = instructionText.getLocalBounds();
    instructionText.setOrigin(instructionBounds.width / 2, instructionBounds.height / 2);
    instructionText.setPosition(m_game.window().getSize().x / 2, m_game.window().getSize().y / 2 + 120);

    m_game.window().draw(instructionText);
}

void Scene_GameOver::sRender() {
    // Reset view to default
    sf::View defaultView = m_game.window().getDefaultView();
    m_game.window().setView(defaultView);

    m_game.window().clear(sf::Color(30, 30, 30));
    renderGameOverText();
    m_game.window().display();
}

void Scene_GameOver::update(float deltaTime) {
    (void)deltaTime; // Unused
}

void Scene_GameOver::sDoAction(const Action& action) {
    if (action.type() == "START") {
        if (action.name() == "UP") {
            m_selectedOption = (m_selectedOption == 0) ? 1 : 0;
        } 
        else if (action.name() == "DOWN") {
            m_selectedOption = (m_selectedOption == 0) ? 1 : 0;
        }
        else if (action.name() == "SELECT") {
            if (m_selectedOption == 0) {
                // Restart the current level
                if (!m_levelPath.empty()) {
                    std::cout << "[DEBUG] Restarting level: " << m_levelPath << std::endl;
                    
                    // Use the restartLevel method
                    m_game.restartLevel(m_levelPath);
                } else {
                    // If we somehow still have an empty path, try one more time with GameEngine's current level
                    std::string currentLevel = m_game.getCurrentLevel();
                    if (!currentLevel.empty()) {
                        std::cout << "[DEBUG] Using GameEngine's current level as fallback: " << currentLevel << std::endl;
                        m_game.restartLevel(currentLevel);
                    } else {
                        std::cerr << "[ERROR] Cannot determine level path for restart. Going to menu." << std::endl;
                        goToLevelSelection();
                    }
                }
            } 
            else if (m_selectedOption == 1) {
                // Go to level selection
                std::cout << "[DEBUG] Going to level selection menu" << std::endl;
                goToLevelSelection();
            }
        } 
        else if (action.name() == "QUIT") {
            m_game.window().close(); // Close the game
        }
    }
}

void Scene_GameOver::goToLevelSelection() {
    // Create a new menu scene that directly opens to level selection
    auto menuScene = std::make_shared<Scene_Menu>(m_game);
    
    // Access the newly created menu scene to set its state to LEVEL_SELECT
    // We need to cast it to access its internals
    auto* menuPtr = menuScene.get();
    if (menuPtr) {
        // Set the menu state to LEVEL_SELECT directly
        // Note: This requires adding a friend declaration or a public method in Scene_Menu
        menuPtr->setMenuState(Scene_Menu::MenuState::LEVEL_SELECT);
    }
    
    m_game.changeScene("MENU", menuScene);
}
