#include "Scene_Menu.h"
#include "GameEngine.h"
#include "Scene_StoryText.h"
#include "Scene_Play.h"
#include "Scene_LevelEditor.h"
#include "ResourcePath.h"

#include <filesystem>
#include <iostream>
#include <algorithm>

// Modify your Scene_Menu constructor as follows:
Scene_Menu::Scene_Menu(GameEngine& game)
    : Scene(game)
    , m_state(MenuState::LANGUAGE)
    , m_selectedLanguageIndex(0)
    , m_selectedMainMenuIndex(0)
    , m_selectedLevelIndex(0)
{
    loadLevelOptions();

    sf::View defaultView = m_game.window().getDefaultView();
    defaultView.setSize(static_cast<float>(m_game.window().getSize().x),
                        static_cast<float>(m_game.window().getSize().y));
    m_game.window().setView(defaultView);
    sf::Mouse::setPosition(sf::Vector2i(0, 0), m_game.window());

    // Grab whatever language the GameEngine already has
    std::string currentLang = m_game.getLanguage();

    // If it’s empty, this must be the very start — default to English
    if (currentLang.empty()) {
        currentLang = "English";
        m_game.setLanguage("English");
    }

    // Store in Menu’s own field
    m_language = currentLang;

    // Initialize language options
    m_languageOptions = {"English", "日本語"};

    // Initialize main menu options
    m_mainMenuOptions = {"PLAY MAIN STORY", "MANUAL LEVEL SELECTION", "LEVEL EDITOR", "QUIT"};

    registerAction(sf::Keyboard::W, "UP");
    registerAction(sf::Keyboard::S, "DOWN");
    registerAction(sf::Keyboard::D, "SELECT");
    registerAction(sf::Keyboard::Escape, "BACK");

    // Update menu translations according to the *current* language
    updateMenuTranslations();
    
    //std::cout << "[DEBUG] Scene_Menu created; language = " << m_language << std::endl;
}

// Modify updateMenuTranslations to populate both arrays
void Scene_Menu::updateMenuTranslations() {
    // Always keep original English options for logic
    m_mainMenuOptions = {"PLAY MAIN STORY", "MANUAL LEVEL SELECTION", "LEVEL EDITOR", "QUIT"};
    
    if (m_language == "Japanese") {
        // Japanese display options
        m_displayMenuOptions = {"メインストーリーをプレイ", "レベル選択", "レベルエディタ", "終了"};
    } else {
        // For English, display and logic options are the same
        m_displayMenuOptions = m_mainMenuOptions;
    }
}

void Scene_Menu::loadLevelOptions() {
    // Use getResourcePath instead of hardcoded path
    const std::string levelPath = getResourcePath("levels");
    m_levelOptions.clear();

    try {
        if (std::filesystem::exists(levelPath)) {
            for (const auto& entry : std::filesystem::directory_iterator(levelPath)) {
                if (entry.path().extension() == ".txt") {
                    m_levelOptions.push_back(entry.path().stem().string());
                }
            }
        } else {
            std::cerr << "[ERROR] Levels directory not found at: " << levelPath << std::endl;
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "[ERROR] Failed to access levels directory: " << e.what() << std::endl;
    }

    std::sort(m_levelOptions.begin(), m_levelOptions.end());
}

void Scene_Menu::sRender() {
    m_game.window().clear(sf::Color(0, 0, 0));

    sf::Text text;
    text.setFont(m_game.assets().getFont("Japanese"));
    text.setCharacterSize(40);
    text.setStyle(sf::Text::Bold);
    text.setFillColor(sf::Color::White);

    float startY = 150.f;
    float spacing = 40.f;

    // Render title
    text.setString("ROME INTERSTELLAR PARADOX");
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
    
    text.setCharacterSize(20);
    text.setFillColor(sf::Color::White);
    
    std::string controlInstructions;
    
    if (m_state == MenuState::LANGUAGE) {
        controlInstructions = "DOWN: S  SELECT: D / 下: S   決定: D";
    } else if (m_game.getLanguage() == "Japanese"){
        // Japanese
        if (m_state == MenuState::MAIN) {
            controlInstructions = "下: S   決定: D   戻る: ESC";
        } 
        else { // LEVEL_SELECT
            controlInstructions = "下: S   決定: D   戻る: ESC";
        }
    } else {
        // English
        if (m_state == MenuState::MAIN) {
            controlInstructions = "DOWN: S  SELECT: D  BACK: ESC";
        } 
        else { // LEVEL_SELECT
            controlInstructions = "DOWN: S  SELECT: D  BACK: ESC";
        }
    }
    
    text.setString(sf::String::fromUtf8(
        controlInstructions.begin(),
        controlInstructions.end()
    ));
    
    text.setPosition(20.f, m_game.window().getSize().y - 40.f);
    m_game.window().draw(text);

    m_game.window().display();
}

void Scene_Menu::renderLanguageMenu(sf::Text& text, float startY, float spacing) {
    // Increase vertical spacing for language menu
    startY = 150.f;  // Position the "SELECT LANGUAGE" text higher
    
    std::string langSelectText = "SELECT LANGUAGE / 言語を選択してください";
    text.setString(sf::String::fromUtf8(
        langSelectText.begin(),
        langSelectText.end()
    ));
    
    text.setPosition(m_game.window().getSize().x / 2 - text.getLocalBounds().width / 2, startY);
    m_game.window().draw(text);

    // Add more space between title and options
    startY += spacing * 3;  // Increased from spacing * 2

    // Increase spacing between language options
    float languageSpacing = spacing * 2;  // Double the spacing between language options
    
    for (size_t i = 0; i < m_languageOptions.size(); ++i) {
        text.setString(sf::String::fromUtf8(
            m_languageOptions[i].begin(),
            m_languageOptions[i].end()
        ));
        text.setPosition(m_game.window().getSize().x / 2 - text.getLocalBounds().width / 2, startY + i * languageSpacing);
        text.setFillColor((i == m_selectedLanguageIndex) ? sf::Color::Blue : sf::Color::White);
        m_game.window().draw(text);
    }
}

void Scene_Menu::renderMainMenu(sf::Text& text, float startY, float spacing) {
    // Position for the centered first option
    startY = 250.f;
    
    // Draw first option much larger and centered
    text.setString(sf::String::fromUtf8(
        m_displayMenuOptions[0].begin(),
        m_displayMenuOptions[0].end()
    ));
    text.setCharacterSize(70); // Larger font size for first option
    text.setPosition(m_game.window().getSize().x / 2 - text.getLocalBounds().width / 2, startY);
    text.setFillColor((0 == m_selectedMainMenuIndex) ? sf::Color::Blue : sf::Color::White);
    m_game.window().draw(text);
    
    // Reset character size for other options
    text.setCharacterSize(40);
    
    // More space after first option before showing other options
    startY += spacing * 4.5f;
    
    // Draw the rest of the menu options below with standard spacing
    float mainMenuSpacing = spacing * 1.5f;
    for (size_t i = 1; i < m_displayMenuOptions.size(); ++i) {
        text.setString(sf::String::fromUtf8(
            m_displayMenuOptions[i].begin(),
            m_displayMenuOptions[i].end()
        ));
        text.setPosition(m_game.window().getSize().x / 2 - text.getLocalBounds().width / 2, startY + (i-1) * mainMenuSpacing);
        text.setFillColor((i == m_selectedMainMenuIndex) ? sf::Color::Blue : sf::Color::White);
        m_game.window().draw(text);
    }
}

void Scene_Menu::renderLevelSelectMenu(sf::Text& text, float startY, float spacing) {
    // Start higher on the screen
    startY = 120.f;
    
    // Use the string directly for UTF-8 conversion
    std::string levelSelectText = (m_language == "Japanese") ? "レベルを選択:" : "SELECT LEVEL:";
    text.setString(sf::String::fromUtf8(
        levelSelectText.begin(),
        levelSelectText.end()
    ));
    
    text.setPosition(m_game.window().getSize().x / 2 - text.getLocalBounds().width / 2, startY);
    m_game.window().draw(text);

    // More space after the title
    startY += spacing * 2;
    
    // Don't use scrolling - show all levels with appropriate spacing
    float levelSpacing = 35.f;  // Fixed spacing value to ensure all levels fit
    
    // Based on the number of levels, we might adjust the spacing
    if (m_levelOptions.size() > 12) {
        levelSpacing = 30.f;  // Smaller spacing for many levels
    }
    
    for (size_t i = 0; i < m_levelOptions.size(); ++i) {
        text.setString(sf::String::fromUtf8(
            m_levelOptions[i].begin(),
            m_levelOptions[i].end()
        ));
        text.setPosition(m_game.window().getSize().x / 2 - text.getLocalBounds().width / 2, startY + i * levelSpacing);
        text.setFillColor((i == m_selectedLevelIndex) ? sf::Color::Blue : sf::Color::White);
        m_game.window().draw(text);
    }
}

void Scene_Menu::update(float deltaTime) {
    (void)deltaTime;
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
            {
                // Get the selected language from options
                std::string selectedLanguage = m_languageOptions[m_selectedLanguageIndex];
                
                // Convert 日本語 to Japanese for internal use
                if (selectedLanguage == "日本語") {
                    m_language = "Japanese";
                } else {
                    m_language = selectedLanguage;
                }
                
                // Store language in GameEngine
                m_game.setLanguage(m_language);
                
                // Update menu translations based on selected language
                updateMenuTranslations();
                m_state = MenuState::MAIN;
            }
            break;
            
        case MenuState::MAIN:
            {
                // Get the index of the selected option, which is the same in both arrays
                int selectedIndex = m_selectedMainMenuIndex;
                
                // Use the English option for logic checks
                std::string selectedOption = m_mainMenuOptions[selectedIndex];
                
                if (selectedOption == "PLAY MAIN STORY") {
                    // Start with the intro story screen
                    m_game.changeScene("INTRO", std::make_shared<Scene_StoryText>(m_game, StoryType::INTRO));
                } 
                else if (selectedOption == "MANUAL LEVEL SELECTION") {
                    m_state = MenuState::LEVEL_SELECT;
                } 
                else if (selectedOption == "LEVEL EDITOR") {
                    m_game.changeScene("EDITOR", std::make_shared<Scene_LevelEditor>(m_game));
                } 
                else if (selectedOption == "QUIT") {
                    m_game.window().close();
                }
            }
            break;
            
        case MenuState::LEVEL_SELECT:
            {
                std::string selectedLevel = getResourcePath("levels") + "/" + m_levelOptions[m_selectedLevelIndex] + ".txt";
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
