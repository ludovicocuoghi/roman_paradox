#include "Scene_Menu.h"
#include "GameEngine.h"
#include "Scene_StoryText.h"
#include "Scene_Play.h"
#include "Scene_LevelEditor.h"

#include <filesystem>
#include <iostream>
#include <algorithm>

Scene_Menu::Scene_Menu(GameEngine& game)
    : Scene(game)
    , m_state(MenuState::LANGUAGE)
    , m_selectedLanguageIndex(0)
    , m_selectedMainMenuIndex(0)
    , m_selectedLevelIndex(0)
    , m_language("English")
{
    loadLevelOptions();

    sf::View defaultView = m_game.window().getDefaultView();
    defaultView.setSize(static_cast<float>(m_game.window().getSize().x), 
                        static_cast<float>(m_game.window().getSize().y));
    m_game.window().setView(defaultView);
    sf::Mouse::setPosition(sf::Vector2i(0, 0), m_game.window());

    // Initialize language options
    m_languageOptions = {"English", "Japanese"};
    
    // Initialize main menu options
    m_mainMenuOptions = {"PLAY STORY", "MANUAL LEVEL SELECTION", "LEVEL EDITOR", "QUIT"};

    registerAction(sf::Keyboard::W, "UP");
    registerAction(sf::Keyboard::S, "DOWN");
    registerAction(sf::Keyboard::D, "SELECT");
    registerAction(sf::Keyboard::Escape, "BACK");
}

void Scene_Menu::loadLevelOptions() {
    const std::string levelPath = "./bin/levels/";
    m_levelOptions.clear();

    for (const auto& entry : std::filesystem::directory_iterator(levelPath)) {
        if (entry.path().extension() == ".txt") {
            m_levelOptions.push_back(entry.path().stem().string());
        }
    }

    std::sort(m_levelOptions.begin(), m_levelOptions.end());
}

void Scene_Menu::sRender() {
    m_game.window().clear(sf::Color(0, 0, 0));

    sf::Text text;
    text.setFont(m_game.assets().getFont("Menu"));
    text.setCharacterSize(40);
    text.setStyle(sf::Text::Bold);
    text.setFillColor(sf::Color::White);

    float startY = 150.f;
    float spacing = 40.f;

    // Render title
    text.setString("ROME INTERGALACTIC PARADOX");
    text.setPosition(m_game.window().getSize().x / 2 - text.getLocalBounds().width / 2, 50.f);
    m_game.window().draw(text);

    // Render appropriate menu based on current state
    switch (m_state) {
        case MenuState::LANGUAGE:
            renderLanguageMenu(text, startY, spacing);
            break;
        case MenuState::MAIN:
            renderMainMenu(text, startY, spacing);
            break;
        case MenuState::LEVEL_SELECT:
            renderLevelSelectMenu(text, startY, spacing);
            break;
    }

    // Render control info
    text.setCharacterSize(20);
    text.setFillColor(sf::Color::White);
    
    // Customize controls display based on menu state
    if (m_state == MenuState::LANGUAGE) {
        text.setString("DOWN: S  SELECT: D");
    } else if (m_state == MenuState::MAIN) {
        text.setString("DOWN: S  SELECT: D  BACK: ESC");
    } else { // LEVEL_SELECT
        text.setString("DOWN: S  SELECT: D  BACK: ESC");
    }
    
    text.setPosition(20.f, m_game.window().getSize().y - 40.f);
    m_game.window().draw(text);

    m_game.window().display();
}

void Scene_Menu::renderLanguageMenu(sf::Text& text, float startY, float spacing) {
    // Increase vertical spacing for language menu
    startY = 150.f;  // Position the "SELECT LANGUAGE" text higher
    
    text.setString("SELECT LANGUAGE:");
    text.setPosition(m_game.window().getSize().x / 2 - text.getLocalBounds().width / 2, startY);
    m_game.window().draw(text);

    // Add more space between title and options
    startY += spacing * 3;  // Increased from spacing * 2

    // Increase spacing between language options
    float languageSpacing = spacing * 2;  // Double the spacing between language options
    
    for (size_t i = 0; i < m_languageOptions.size(); ++i) {
        text.setString(m_languageOptions[i]);
        text.setPosition(m_game.window().getSize().x / 2 - text.getLocalBounds().width / 2, startY + i * languageSpacing);
        text.setFillColor((i == m_selectedLanguageIndex) ? sf::Color::Blue : sf::Color::White);
        m_game.window().draw(text);
    }
}

void Scene_Menu::renderMainMenu(sf::Text& text, float startY, float spacing) {
    // Increase spacing for main menu items
    float mainMenuSpacing = spacing * 1.5f;  // 50% more space between items
    
    // Start higher on the screen
    startY = 150.f;
    
    for (size_t i = 0; i < m_mainMenuOptions.size(); ++i) {
        text.setString(m_mainMenuOptions[i]);
        text.setPosition(m_game.window().getSize().x / 2 - text.getLocalBounds().width / 2, startY + i * mainMenuSpacing);
        text.setFillColor((i == m_selectedMainMenuIndex) ? sf::Color::Blue : sf::Color::White);
        m_game.window().draw(text);
    }
}

void Scene_Menu::renderLevelSelectMenu(sf::Text& text, float startY, float spacing) {
    // Start higher on the screen
    startY = 120.f;
    
    text.setString("SELECT LEVEL:");
    text.setPosition(m_game.window().getSize().x / 2 - text.getLocalBounds().width / 2, startY);
    m_game.window().draw(text);

    // More space after the title
    startY += spacing * 2;
    
    // Don't use scrolling - show all levels with appropriate spacing
    // Based on the screenshot, we want to fit all levels on one screen
    float levelSpacing = 35.f;  // Fixed spacing value to ensure all levels fit
    
    // Based on the number of levels, we might adjust the spacing
    if (m_levelOptions.size() > 12) {
        levelSpacing = 30.f;  // Smaller spacing for many levels
    }
    
    for (size_t i = 0; i < m_levelOptions.size(); ++i) {
        text.setString(m_levelOptions[i]);
        text.setPosition(m_game.window().getSize().x / 2 - text.getLocalBounds().width / 2, startY + i * levelSpacing);
        text.setFillColor((i == m_selectedLevelIndex) ? sf::Color::Blue : sf::Color::White);
        m_game.window().draw(text);
    }
}

void Scene_Menu::update(float deltaTime) {
    (void)deltaTime;
    // You could add animation or other time-based updates here
}

void Scene_Menu::sDoAction(const Action& action) {
    if (action.type() == "START") {
        if (action.name() == "UP") {
            handleUpAction();
        } else if (action.name() == "DOWN") {
            handleDownAction();
        } else if (action.name() == "SELECT") {
            handleSelectAction();
        } else if (action.name() == "BACK") {
            handleBackAction();
        }
    }
}

void Scene_Menu::handleUpAction() {
    switch (m_state) {
        case MenuState::LANGUAGE:
            m_selectedLanguageIndex = (m_selectedLanguageIndex > 0) ? m_selectedLanguageIndex - 1 : m_languageOptions.size() - 1;
            break;
        case MenuState::MAIN:
            m_selectedMainMenuIndex = (m_selectedMainMenuIndex > 0) ? m_selectedMainMenuIndex - 1 : m_mainMenuOptions.size() - 1;
            break;
        case MenuState::LEVEL_SELECT:
            m_selectedLevelIndex = (m_selectedLevelIndex > 0) ? m_selectedLevelIndex - 1 : m_levelOptions.size() - 1;
            break;
    }
}

void Scene_Menu::handleDownAction() {
    switch (m_state) {
        case MenuState::LANGUAGE:
            m_selectedLanguageIndex = (m_selectedLanguageIndex + 1) % m_languageOptions.size();
            break;
        case MenuState::MAIN:
            m_selectedMainMenuIndex = (m_selectedMainMenuIndex + 1) % m_mainMenuOptions.size();
            break;
        case MenuState::LEVEL_SELECT:
            m_selectedLevelIndex = (m_selectedLevelIndex + 1) % m_levelOptions.size();
            break;
    }
}

void Scene_Menu::handleSelectAction() {
    switch (m_state) {
        case MenuState::LANGUAGE:
            m_language = m_languageOptions[m_selectedLanguageIndex];
            m_state = MenuState::MAIN;
            break;
            
        case MenuState::MAIN:
            {
                std::string selectedOption = m_mainMenuOptions[m_selectedMainMenuIndex];
                
                if (selectedOption == "PLAY STORY") {
                    // Start with the intro story screen
                    m_game.changeScene("INTRO", std::make_shared<Scene_StoryText>(m_game, StoryType::INTRO));
                } 
                else if (selectedOption == "LEVEL SELECTION") {
                    m_state = MenuState::LEVEL_SELECT;
                } 
                else if (selectedOption == "LEVEL EDITOR") {
                    std::cout << "[DEBUG] Opening Level Editor...\n";
                    m_game.changeScene("EDITOR", std::make_shared<Scene_LevelEditor>(m_game));
                } 
                else if (selectedOption == "QUIT") {
                    m_game.window().close();
                }
            }
            break;
            
        case MenuState::LEVEL_SELECT:
            {
                std::string selectedLevel = "./bin/levels/" + m_levelOptions[m_selectedLevelIndex] + ".txt";
                m_game.loadLevel(selectedLevel);
            }
            break;
    }
}

void Scene_Menu::handleBackAction() {
    switch (m_state) {
        case MenuState::LANGUAGE:
            // No back action for language menu, could close the game
            m_game.window().close();
            break;
        case MenuState::MAIN:
            // Go back to language selection
            m_state = MenuState::LANGUAGE;
            break;
        case MenuState::LEVEL_SELECT:
            // Go back to main menu
            m_state = MenuState::MAIN;
            break;
    }
}
