#include "Scene_StoryText.h"
#include "GameEngine.h"
#include "Scene_Menu.h"
#include "Scene_Play.h"
#include "ResourcePath.h"
#include <iostream>

Scene_StoryText::Scene_StoryText(GameEngine& game, StoryType type)
    : Scene(game)
    , m_type(type)
    , m_textFadeTime(2.0f)
    , m_textFadeTimer(0.0f)
    , m_fadingIn(true)
{
    registerAction(sf::Keyboard::Space, "CONTINUE");
    registerAction(sf::Keyboard::Return, "CONTINUE");
    registerAction(sf::Keyboard::Escape, "BACK");
    
    std::cout << "[DEBUG] Story text scene created, type: " << (type == StoryType::INTRO ? "INTRO" : "ENDING") << std::endl;
    loadStoryText(type);
    
    // Set proper view
    m_game.window().setView(m_game.window().getDefaultView());
}

void Scene_StoryText::loadStoryText(StoryType type) {
    m_storyLines.clear();
    std::cout << "[DEBUG] Loading story text for " << (type == StoryType::INTRO ? "INTRO" : "ENDING") << std::endl;
    
    // Get the language from the game engine
    std::string language = m_game.getLanguage();
    
    if (type == StoryType::INTRO) {
        if (language == "Japanese") {
            // Improved Japanese intro text
            m_storyLines.push_back("ローマ銀河パラドックス");
            m_storyLines.push_back(" ");
            m_storyLines.push_back("無限の宇宙と並行して存在する数多の現実があると言われています。");
            m_storyLines.push_back(" ");
            m_storyLines.push_back("理論物理学によれば、ブラックホールは異なる次元間の");
            m_storyLines.push_back("移動を可能にし、様々なマルチバースを繋げる鍵となり得ます。");
            m_storyLines.push_back(" ");
            m_storyLines.push_back("このゲームでは、地球とは異なる発展を遂げた");
            m_storyLines.push_back("ローマ帝国の宇宙に存在するエイリアンとして冒険します。");
            m_storyLines.push_back(" ");
            m_storyLines.push_back("無数の宇宙には無限のローマ帝国が存在し、");
            m_storyLines.push_back("それぞれが独自の歴史と発展の道を歩んでいます。");
            m_storyLines.push_back(" ");
            m_storyLines.push_back("平穏な日常を過ごしていたある日、突如として");
            m_storyLines.push_back("未来の軍団兵による侵略に見舞われることになります。");
            m_storyLines.push_back(" ");
            m_storyLines.push_back("あなたは脱出し、この謎めいた侵略の真相を解き明かさなければなりません...");
            
            m_continueText = "スペースキーを押して冒険を始める";
        } else {
            // Default English intro text
            m_storyLines.push_back("ROME INTERGALACTIC PARADOX");
            m_storyLines.push_back(" ");
            m_storyLines.push_back("It is said that there are infinite universes");
            m_storyLines.push_back("and multiple realities existing in parallel.");
            m_storyLines.push_back(" ");
            m_storyLines.push_back("Theoretical physics suggests that black holes");
            m_storyLines.push_back("could allow travel between different dimensions,");
            m_storyLines.push_back("connecting various multiverses together.");
            m_storyLines.push_back(" ");
            m_storyLines.push_back("In this game, you play as an alien in a universe");
            m_storyLines.push_back("where the Roman Empire developed differently than on Earth.");
            m_storyLines.push_back(" ");
            m_storyLines.push_back("There are infinite Roman Empires across multiple universes,");
            m_storyLines.push_back("each with its own history and development.");
            m_storyLines.push_back(" ");
            m_storyLines.push_back("During what seemed like a normal day,");
            m_storyLines.push_back("you suddenly find yourself invaded by futuristic Legionaries.");
            m_storyLines.push_back(" ");
            m_storyLines.push_back("You must escape and discover the truth behind this invasion...");
            
            m_continueText = "Press SPACE to begin your journey";
        }
    }
    else if (type == StoryType::ENDING) {
        if (language == "Japanese") {
            // Improved Japanese ending text
            m_storyLines.push_back("ローマ銀河パラドックス");
            m_storyLines.push_back(" ");
            m_storyLines.push_back("そして歴史は繰り返される...");
            m_storyLines.push_back(" ");
            m_storyLines.push_back("復讐を胸に秘めた未来の軍団兵は、");
            m_storyLines.push_back("ブラックホールを通じて別の次元へと旅立ちます。");
            m_storyLines.push_back(" ");
            m_storyLines.push_back("その先で彼は別のエイリアンと遭遇するでしょう。");
            m_storyLines.push_back("何も知らないままのローマのエイリアンと。");
            m_storyLines.push_back(" ");
            m_storyLines.push_back("こうして循環は再び始まります...");
            m_storyLines.push_back("歴史は永遠に繰り返され、");
            m_storyLines.push_back("次元と時間を超えて続いていくのです。");
            m_storyLines.push_back(" ");
            m_storyLines.push_back("プレイしていただき、ありがとうございました！");
            
            m_continueText = "スペースキーを押してメインメニューに戻る";
        } else {
            // Default English ending text
            m_storyLines.push_back("ROME INTERGALACTIC PARADOX");
            m_storyLines.push_back(" ");
            m_storyLines.push_back("And so the history repeats itself...");
            m_storyLines.push_back(" ");
            m_storyLines.push_back("The future Legionary, determined for revenge,");
            m_storyLines.push_back("travels through a black hole to another dimension.");
            m_storyLines.push_back(" ");
            m_storyLines.push_back("There, he will find a different alien,");
            m_storyLines.push_back("a Roman alien who knows nothing of what happened.");
            m_storyLines.push_back(" ");
            m_storyLines.push_back("And so the cycle begins again...");
            m_storyLines.push_back("The history will repeat infinitely,");
            m_storyLines.push_back("across dimensions and time itself.");
            m_storyLines.push_back(" ");
            m_storyLines.push_back("Thank you for playing!");
            
            m_continueText = "Press SPACE to return to the main menu";
        }
    }
    
    std::cout << "[DEBUG] Loaded " << m_storyLines.size() << " lines of text" << std::endl;
}

void Scene_StoryText::update(float deltaTime) {
    
    // Handle text fade effect
    if (m_fadingIn) {
        m_textFadeTimer += deltaTime;
        if (m_textFadeTimer >= m_textFadeTime) {
            m_textFadeTimer = m_textFadeTime;
            m_fadingIn = false;
        }
    }
}

void Scene_StoryText::renderStoryText() {
    // Skip the fade effect for now to ensure text is visible
    float alpha = 255.0f;
    
    sf::Text text;
    
    // Use Japanese font if language is set to Japanese
    if (m_game.getLanguage() == "Japanese") {
        // std::cout << "[DEBUG] Using Japanese font for story text" << std::endl;
        text.setFont(m_game.assets().getFont("Japanese"));
    } else {
        // std::cout << "[DEBUG] Using default font for story text" << std::endl;
        text.setFont(m_game.assets().getFont("Menu"));
    }

    
    
    text.setCharacterSize(28);
    text.setStyle(sf::Text::Regular);
    
    float startY = 80.0f;  // Start higher
    float lineSpacing = 35.0f;  // Slightly reduced spacing
    float maxY = m_game.window().getSize().y - 150.0f; // Leave plenty of room at bottom
    
    // Calculate total content height
    float totalContentHeight = m_storyLines.size() * lineSpacing;
    
    // Adjust starting position if content would overflow
    if (startY + totalContentHeight > maxY) {
        startY = (m_game.window().getSize().y - totalContentHeight) / 2.0f - 50.0f;
    }
    
    // Render story text
    for (size_t i = 0; i < m_storyLines.size(); ++i) {
        text.setString(sf::String::fromUtf8(
            m_storyLines[i].begin(),
            m_storyLines[i].end()
        ));
        text.setFillColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(alpha)));
        
        // Center align text
        sf::FloatRect textBounds = text.getLocalBounds();
        text.setPosition(
            m_game.window().getSize().x / 2.0f - textBounds.width / 2.0f,
            startY + i * lineSpacing
        );
        
        m_game.window().draw(text);
    }
    
    // Draw a separator line
    sf::RectangleShape separator(sf::Vector2f(m_game.window().getSize().x * 0.6f, 2.0f));
    separator.setFillColor(sf::Color(150, 150, 150, 100));
    separator.setPosition(
        m_game.window().getSize().x * 0.2f,
        m_game.window().getSize().y - 130.0f
    );
    m_game.window().draw(separator);
    
    // Render continue prompt at the bottom with bright color and more space
    text.setString(sf::String::fromUtf8(
        m_continueText.begin(),
        m_continueText.end()
    ));
    text.setCharacterSize(32); // Larger size
    text.setFillColor(sf::Color::Yellow); // Bright color
    
    sf::FloatRect promptBounds = text.getLocalBounds();
    text.setPosition(
        m_game.window().getSize().x / 2.0f - promptBounds.width / 2.0f,
        m_game.window().getSize().y - 100.0f
    );
    
    m_game.window().draw(text);
}

void Scene_StoryText::sRender() {
    // Set black background
    m_game.window().clear(sf::Color(0, 0, 0));
    
    // Reset the view to default to ensure proper text positioning
    m_game.window().setView(m_game.window().getDefaultView());
    
    // Render story text
    renderStoryText();
    
    m_game.window().display();
}

void Scene_StoryText::sDoAction(const Action& action) {
    if (action.type() == "START") {
        if (action.name() == "CONTINUE") {
            if (m_type == StoryType::INTRO) {
                // Start the first level after intro
                std::string firstLevel = getResourcePath("levels") + "/alien_rome_level_1.txt";
                m_game.loadLevel(firstLevel);
            }
            else if (m_type == StoryType::ENDING) {
                // Return to main menu after ending
                m_game.changeScene("MENU", std::make_shared<Scene_Menu>(m_game));
            }
        }
        else if (action.name() == "BACK") {
            // Return to main menu
            m_game.changeScene("MENU", std::make_shared<Scene_Menu>(m_game));
        }
    }
}
