#include "Scene_GameOver.h"
#include "Scene_Menu.h"
#include "Scene_Play.h"
#include "GameEngine.h"

Scene_GameOver::Scene_GameOver(GameEngine& game, const std::string& levelPath)
    : Scene(game), m_levelPath(levelPath), m_selectedOption(0)
{
    registerAction(sf::Keyboard::Enter, "SELECT");
    registerAction(sf::Keyboard::Up, "UP");
    registerAction(sf::Keyboard::Down, "DOWN");
    registerAction(sf::Keyboard::Escape, "QUIT");
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

    // Option 1: Restart Level
    sf::Text restartText;
    restartText.setFont(m_game.assets().getFont("Menu"));
    restartText.setString("Restart Level");
    restartText.setCharacterSize(30);
    restartText.setFillColor(m_selectedOption == 0 ? sf::Color::Yellow : sf::Color::White);

    sf::FloatRect restartBounds = restartText.getLocalBounds();
    restartText.setOrigin(restartBounds.width / 2, restartBounds.height / 2);
    restartText.setPosition(m_game.window().getSize().x / 2, m_game.window().getSize().y / 2);

    m_game.window().draw(restartText);

    // Option 2: Return to Menu
    sf::Text menuText;
    menuText.setFont(m_game.assets().getFont("Menu"));
    menuText.setString("Return to Menu");
    menuText.setCharacterSize(30);
    menuText.setFillColor(m_selectedOption == 1 ? sf::Color::Yellow : sf::Color::White);

    sf::FloatRect menuBounds = menuText.getLocalBounds();
    menuText.setOrigin(menuBounds.width / 2, menuBounds.height / 2);
    menuText.setPosition(m_game.window().getSize().x / 2, m_game.window().getSize().y / 2 + 50);

    m_game.window().draw(menuText);

    // Instructions
    sf::Text instructionText;
    instructionText.setFont(m_game.assets().getFont("Menu"));
    instructionText.setString("Use UP/DOWN arrows to select and ENTER to confirm");
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
            m_selectedOption = 0; // Select Restart Level
        } 
        else if (action.name() == "DOWN") {
            m_selectedOption = 1; // Select Return to Menu
        }
        else if (action.name() == "SELECT") {
            if (m_selectedOption == 0) {
                // Restart the current level
                if (!m_levelPath.empty()) {
                    m_game.changeScene("PLAY", std::make_shared<Scene_Play>(m_game, m_levelPath));
                } else {
                    // Fallback if level path is not available
                    m_game.changeScene("MENU", std::make_shared<Scene_Menu>(m_game));
                }
            } 
            else if (m_selectedOption == 1) {
                // Return to menu
                m_game.changeScene("MENU", std::make_shared<Scene_Menu>(m_game));
            }
        } 
        else if (action.name() == "QUIT") {
            m_game.window().close(); // Close the game
        }
    }
}
