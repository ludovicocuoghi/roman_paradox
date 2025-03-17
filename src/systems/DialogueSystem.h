#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <map>
#include "EntityManager.hpp"
#include "GameEngine.h"

// Updated DialogueMessage struct with additional customization parameters
struct DialogueMessage {
    std::string speaker;
    std::string message;
    std::string portraitPath;
    bool portraitOnLeft;
    sf::Color speakerColor;
    sf::Color messageColor;
    sf::Vector2f dialogueBoxPosition;
    // New customization parameters
    float boxWidth = 650.f;         // Default width
    float boxHeight = 150.f;        // Default height
    int messageFontSize = 20;       // Default message font size (only for message text)
    bool useTypewriterEffect = true;  // Enable/disable typewriter effect
};

class DialogueSystem {
public:
    DialogueSystem(GameEngine& game, EntityManager& entityManager);

    void addDialogueTrigger(float xPosition, const std::vector<DialogueMessage>& dialogue);
    void checkTriggers();
    void update(float deltaTime);
    void advanceDialogue();
    void handleAttackAction();

    bool isDialogueActive() const;
    const DialogueMessage* getCurrentMessage() const;
    const std::string& getDisplayedText() const;
    bool isTyping() const;
    bool isWaitingAfterCompletion() const;
    float getCompletionTimer() const;
    const sf::Texture& getPortraitTexture() const;
    void triggerDialogueByID(const std::string& dialogueID);
    void addNamedDialogue(const std::string& dialogueID, const std::vector<DialogueMessage>& dialogue);

    // Public rendering components
    sf::RectangleShape dialogueBox;
    sf::Text continueText;
    sf::Text speakerText;
    sf::Text messageText;
    sf::Sprite portraitSprite;
    sf::Font dialogueFont;

private:
    GameEngine& m_game;
    EntityManager& m_entityManager;

    std::map<float, std::vector<DialogueMessage>> m_dialogueTriggers;
    std::vector<float> m_triggeredPositions;

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

    void startNewMessage(const DialogueMessage& message);
    void updateTypewriterEffect(float deltaTime);
    std::map<std::string, std::vector<DialogueMessage>> m_namedDialogues;
};
