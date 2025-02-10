#include "Scene_GameOver.h"
#include "Scene_Menu.h"
#include "GameEngine.h"

Scene_GameOver::Scene_GameOver(GameEngine& game)
    : Scene(game)
{
    registerAction(sf::Keyboard::Enter, "RESTART");
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
    gameOverText.setPosition(m_game.window().getSize().x / 2, m_game.window().getSize().y / 2 - 50);

    m_game.window().draw(gameOverText);

    // Instructions
    sf::Text instructionText;
    instructionText.setFont(m_game.assets().getFont("Menu"));
    instructionText.setString("Press ENTER to Restart or ESC to Quit");
    instructionText.setCharacterSize(24);
    instructionText.setFillColor(sf::Color::White);

    sf::FloatRect instructionBounds = instructionText.getLocalBounds();
    instructionText.setOrigin(instructionBounds.width / 2, instructionBounds.height / 2);
    instructionText.setPosition(m_game.window().getSize().x / 2, m_game.window().getSize().y / 2 + 50);

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
        if (action.name() == "RESTART") {
            // Restart the game by going back to the menu or reloading the current level
            m_game.changeScene("MENU", std::make_shared<Scene_Menu>(m_game));
        } else if (action.name() == "QUIT") {
            m_game.window().close(); // Close the game
        }
    }
}
