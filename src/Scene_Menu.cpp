#include "Scene_Menu.h"
#include "GameEngine.h"
#include "Scene_Play.h"
#include "Scene_LevelEditor.h"

#include <filesystem>
#include <iostream>
#include <algorithm>

Scene_Menu::Scene_Menu(GameEngine& game)
    : Scene(game)
{
    loadLevelOptions();  // ✅ Ensures Level Editor & Quit are added only once

    sf::View defaultView = m_game.window().getDefaultView();
    defaultView.setSize(static_cast<float>(m_game.window().getSize().x), 
                        static_cast<float>(m_game.window().getSize().y));
    m_game.window().setView(defaultView);
    sf::Mouse::setPosition(sf::Vector2i(0, 0), m_game.window());

    registerAction(sf::Keyboard::W, "UP");
    registerAction(sf::Keyboard::S, "DOWN");
    registerAction(sf::Keyboard::D, "SELECT");
    registerAction(sf::Keyboard::Escape, "BACK");
}

void Scene_Menu::loadLevelOptions() {
    const std::string levelPath = "./bin/levels/";
    m_menuStrings.clear();  // ✅ Prevent duplicate entries

    for (const auto& entry : std::filesystem::directory_iterator(levelPath)) {
        if (entry.path().extension() == ".txt") {
            m_menuStrings.push_back(entry.path().stem().string());
        }
    }

    std::sort(m_menuStrings.begin(), m_menuStrings.end());

    // ✅ Prevent duplicate entries
    if (std::find(m_menuStrings.begin(), m_menuStrings.end(), "Level Editor") == m_menuStrings.end()) {
        m_menuStrings.push_back("Level Editor");
    }

    if (std::find(m_menuStrings.begin(), m_menuStrings.end(), "Quit") == m_menuStrings.end()) {
        m_menuStrings.push_back("Quit");
    }
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

    // Render menu options
    for (size_t i = 0; i < m_menuStrings.size(); ++i) {
        text.setString(m_menuStrings[i]);
        text.setPosition(m_game.window().getSize().x / 2 - text.getLocalBounds().width / 2, startY + i * spacing);

        text.setFillColor((i == m_selectedMenuIndex) ? sf::Color::Blue : sf::Color::White);
        m_game.window().draw(text);
    }

    // Render control info
    text.setCharacterSize(20);
    text.setFillColor(sf::Color::Black);
    text.setString("UP: W   DOWN: S   SELECT: D");
    text.setPosition(20.f, m_game.window().getSize().y - 40.f);
    m_game.window().draw(text);

    m_game.window().display();
}

void Scene_Menu::update(float deltaTime) {
    (void)deltaTime;
}

void Scene_Menu::sDoAction(const Action& action) {
    if (action.type() == "START") {
        if (action.name() == "UP") {
            m_selectedMenuIndex = (m_selectedMenuIndex > 0) ? m_selectedMenuIndex - 1 : m_menuStrings.size() - 1;
        } else if (action.name() == "DOWN") {
            m_selectedMenuIndex = (m_selectedMenuIndex + 1) % m_menuStrings.size();
        } else if (action.name() == "SELECT") {
            std::string selectedOption = m_menuStrings[m_selectedMenuIndex];

            if (selectedOption == "Quit") {
                m_game.window().close();
            } else if (selectedOption == "Level Editor") {
                std::cout << "[DEBUG] Opening Level Editor...\n";
                m_game.changeScene("EDITOR", std::make_shared<Scene_LevelEditor>(m_game));
            } else {
                std::string selectedLevel = "./bin/levels/" + selectedOption + ".txt";
                m_game.loadLevel(selectedLevel);
            }
        } else if (action.name() == "BACK") {
            m_game.window().close();
        }
    }
}
