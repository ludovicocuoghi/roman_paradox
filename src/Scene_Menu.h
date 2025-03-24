#pragma once

#include "Scene.h"
#include <vector>
#include <string>

class Scene_Menu : public Scene {
public:
    // Make MenuState public so it can be accessed from other classes
    enum class MenuState {
        LANGUAGE,
        MAIN,
        LEVEL_SELECT
    };

    Scene_Menu(GameEngine& game);

    void update(float deltaTime) override;
    void sRender() override;
    void sDoAction(const Action& action) override;
    
    // Public method to set menu state directly
    void setMenuState(MenuState state) { m_state = state; }

private:
    void loadLevelOptions();
    void renderLanguageMenu(sf::Text& text, float startY, float spacing);
    void renderMainMenu(sf::Text& text, float startY, float spacing);
    void renderLevelSelectMenu(sf::Text& text, float startY, float spacing);
    void updateMenuTranslations();
    
    void handleUpAction();
    void handleDownAction();
    void handleSelectAction();
    void handleBackAction();

    MenuState m_state;
    
    std::vector<std::string> m_languageOptions;
    std::vector<std::string> m_mainMenuOptions;
    std::vector<std::string> m_levelOptions;
    
    size_t m_selectedLanguageIndex;
    size_t m_selectedMainMenuIndex;
    size_t m_selectedLevelIndex;
    
    std::string m_language;
    std::vector<std::string> m_displayMenuOptions;  // For display only
    std::map<std::string, std::string> m_menuOptionMap;  // Maps Japanese display options to English code options

};
