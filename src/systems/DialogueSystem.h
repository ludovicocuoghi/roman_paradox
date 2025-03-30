#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <map>
#include <string>
#include "EntityManager.hpp"
#include "GameEngine.h"

// Structure for dialogue messages
struct DialogueMessage {
    std::string speaker;
    std::string message;
    std::string portraitPath;
    bool portraitOnLeft = true;
    sf::Color speakerColor = sf::Color::White;
    sf::Color messageColor = sf::Color::White;
    sf::Vector2f dialogueBoxPosition = {100.f, 400.f};
    float boxWidth = 600.f;
    float boxHeight = 150.f;
    int messageFontSize = 18;
    bool useTypewriterEffect = true;
};

class DialogueSystem {
private:
    GameEngine& m_game;
    EntityManager& m_entityManager;
    std::map<float, std::vector<DialogueMessage>> m_dialogueTriggers;
    std::vector<float> m_triggeredPositions;
    std::map<std::string, std::vector<DialogueMessage>> m_namedDialogues;
    std::vector<DialogueMessage> m_currentDialogue;
    size_t m_currentMessageIndex;
    sf::Texture m_portraitTexture;
    sf::Color m_speakerColor;
    sf::Color m_messageColor;
    bool m_portraitOnLeft;
    sf::Vector2f m_dialogueBoxPosition;
    std::string m_displayedText;
    float m_textSpeed;
    float m_textTimer;
    bool m_isTyping;
    bool m_dialogueActive;
    float m_completionTimer;
    bool m_waitingAfterCompletion;
    
    // Fonts for language support
    sf::Font dialogueFont;      // Default font
    sf::Font japaneseFontJpn;   // Japanese font
    std::string m_language;     // Current language
    
public:
    // UI elements - keeping these public to maintain compatibility with PlayRenderer
    sf::RectangleShape dialogueBox;
    sf::Text continueText;
    sf::Text speakerText;
    sf::Text messageText;
    sf::Sprite portraitSprite;
    
    DialogueSystem(GameEngine& game, EntityManager& entityManager);
    
    // Add language support method
    void setLanguage(const std::string& language);
    void loadFonts();
    
    void addDialogueTrigger(float xPosition, const std::vector<DialogueMessage>& dialogue);
    void checkTriggers();
    void update(float deltaTime);
    void updateTypewriterEffect(float deltaTime);
    void handleAttackAction();
    void triggerDialogueByID(const std::string& dialogueID);
    void addNamedDialogue(const std::string& dialogueID, const std::vector<DialogueMessage>& dialogue);
    void startNewMessage(const DialogueMessage& message);
    void advanceDialogue();
    
    // Getters for rendering
    bool isDialogueActive() const;
    bool isTyping() const;
    bool isWaitingAfterCompletion() const;
    float getCompletionTimer() const;
    const std::string& getDisplayedText() const;
    const DialogueMessage* getCurrentMessage() const;
    const sf::Texture& getPortraitTexture() const;
};
