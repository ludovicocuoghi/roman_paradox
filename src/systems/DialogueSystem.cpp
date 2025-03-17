#include "DialogueSystem.h"
#include "Components.hpp"
#include <iostream>

DialogueSystem::DialogueSystem(GameEngine& game, EntityManager& entityManager)
    : m_game(game),
      m_entityManager(entityManager),
      m_dialogueTriggers(),
      m_triggeredPositions(),
      m_currentDialogue(),
      m_currentMessageIndex(0),
      m_portraitTexture(),
      m_speakerColor(sf::Color::White),
      m_messageColor(sf::Color::White),
      m_portraitOnLeft(true),
      m_dialogueBoxPosition(0.f, 0.f),
      m_displayedText(""),
      m_textSpeed(30.0f),
      m_textTimer(0.0f),
      m_isTyping(false),
      m_dialogueActive(false),
      m_completionTimer(0.0f),
      m_waitingAfterCompletion(false),
      dialogueBox(),
      continueText(),
      speakerText(),
      messageText(),
      portraitSprite(),
      dialogueFont()
{
    if (!dialogueFont.loadFromFile("bin/font/font.ttf")) {
        std::cerr << "[ERROR] Could not load font!\n";
    }

    // Initialize with default values, actual dimensions will be set in renderDialogue
    dialogueBox.setSize(sf::Vector2f(600.f, 150.f));
    dialogueBox.setFillColor(sf::Color(0, 0, 0, 200));

    continueText.setFont(dialogueFont);
    continueText.setCharacterSize(14);
    continueText.setString("Press ATTACK to continue...");

    speakerText.setFont(dialogueFont);
    speakerText.setCharacterSize(20);
    speakerText.setFillColor(m_speakerColor);

    messageText.setFont(dialogueFont);
    messageText.setCharacterSize(18);
    messageText.setFillColor(m_messageColor);

    portraitSprite.setTexture(m_portraitTexture);

    (void)m_game; // Silence unused warning temporarily if m_game is not yet used

    std::cout << "[DEBUG] DialogueSystem initialized\n";
}

void DialogueSystem::addDialogueTrigger(float xPosition, const std::vector<DialogueMessage>& dialogue)
{
    m_dialogueTriggers[xPosition] = dialogue;
    std::cout << "[DEBUG] Added dialogue trigger at x=" << xPosition << " with " 
              << dialogue.size() << " messages\n";
}

void DialogueSystem::checkTriggers()
{
    if (m_dialogueActive) return;

    auto playerEntities = m_entityManager.getEntities("player");
    if (playerEntities.empty()) return;

    auto& playerTransform = playerEntities[0]->get<CTransform>();
    float playerX = playerTransform.pos.x;

    for (const auto& trigger : m_dialogueTriggers) {
        float triggerPosition = std::abs(trigger.first); // Get absolute position
        bool greaterThanOrEqual = (trigger.first >= 0);  // If position is negative, we check for player <= position
        
        bool shouldTrigger = greaterThanOrEqual ? 
                             (playerX >= triggerPosition) : 
                             (playerX <= triggerPosition);
                             
        if (shouldTrigger && std::find(m_triggeredPositions.begin(), m_triggeredPositions.end(), trigger.first) == m_triggeredPositions.end()) {
            m_currentDialogue = trigger.second;
            m_currentMessageIndex = 0;
            m_dialogueActive = true;
            m_isTyping = false;
            m_waitingAfterCompletion = false;
            m_triggeredPositions.push_back(trigger.first);

            if (!m_currentDialogue.empty()) {
                startNewMessage(m_currentDialogue[0]);
            }

            std::cout << "[DEBUG] Triggered dialogue at x=" << trigger.first 
                      << " (type: " << (greaterThanOrEqual ? "player >= position" : "player <= position") << ")\n";
            break;
        }
    }
}

void DialogueSystem::startNewMessage(const DialogueMessage& message)
{
    m_displayedText = "";
    m_textTimer = 0.0f;
    m_isTyping = message.useTypewriterEffect; // Only start typing effect if enabled for this message

    // If typewriter effect is disabled, immediately display the full message
    if (!message.useTypewriterEffect) {
        m_displayedText = message.message;
    }

    dialogueBox.setPosition(message.dialogueBoxPosition);
    dialogueBox.setSize(sf::Vector2f(message.boxWidth, message.boxHeight));

    continueText.setPosition(dialogueBox.getPosition().x + 10.f, dialogueBox.getPosition().y - 20.f);

    if (!message.portraitPath.empty() && !m_portraitTexture.loadFromFile(message.portraitPath)) {
        std::cerr << "[WARNING] Could not load portrait: " << message.portraitPath << "\n";
    }
    portraitSprite.setTexture(m_portraitTexture);

    m_speakerColor = message.speakerColor;
    m_messageColor = message.messageColor;
    m_portraitOnLeft = message.portraitOnLeft;

    messageText.setCharacterSize(message.messageFontSize);

    speakerText.setFillColor(m_speakerColor);
    messageText.setFillColor(m_messageColor);

    speakerText.setString(message.speaker);
    messageText.setString(m_displayedText);

    std::cout << "[DEBUG] Started new message from " << message.speaker << "\n";
}

void DialogueSystem::updateTypewriterEffect(float deltaTime)
{
    if (!m_isTyping) return;

    const auto& currentMessage = m_currentDialogue[m_currentMessageIndex];

    // Skip typewriter effect if disabled for this message
    if (!currentMessage.useTypewriterEffect) {
        m_displayedText = currentMessage.message;
        m_isTyping = false;
        std::cout << "[DEBUG] Typewriter effect skipped for message\n";
        return;
    }

    m_textTimer += deltaTime;
    int charCount = static_cast<int>(m_textTimer * m_textSpeed);

    if (static_cast<size_t>(charCount) >= currentMessage.message.length()) {
        m_displayedText = currentMessage.message;
        m_isTyping = false;
        std::cout << "[DEBUG] Finished typing message\n";
    } else {
        m_displayedText = currentMessage.message.substr(0, charCount);
    }

    speakerText.setString(currentMessage.speaker);
    messageText.setString(m_displayedText);
}

void DialogueSystem::advanceDialogue()
{
    if (m_isTyping) {
        m_displayedText = m_currentDialogue[m_currentMessageIndex].message;
        m_isTyping = false;
        return;
    }

    if (m_waitingAfterCompletion) return;

    m_currentMessageIndex++;

    if (m_currentMessageIndex >= m_currentDialogue.size()) {
        m_dialogueActive = false;
        m_waitingAfterCompletion = false;
        std::cout << "[DEBUG] Dialogue ended, game resumed\n";
    } else {
        startNewMessage(m_currentDialogue[m_currentMessageIndex]);
    }
}

void DialogueSystem::handleAttackAction()
{
    advanceDialogue();
}

bool DialogueSystem::isDialogueActive() const { return m_dialogueActive; }

const DialogueMessage* DialogueSystem::getCurrentMessage() const
{
    return (m_dialogueActive && m_currentMessageIndex < m_currentDialogue.size())
           ? &m_currentDialogue[m_currentMessageIndex] : nullptr;
}

const std::string& DialogueSystem::getDisplayedText() const { return m_displayedText; }
bool DialogueSystem::isTyping() const { return m_isTyping; }
bool DialogueSystem::isWaitingAfterCompletion() const { return m_waitingAfterCompletion; }
float DialogueSystem::getCompletionTimer() const { return m_completionTimer; }
const sf::Texture& DialogueSystem::getPortraitTexture() const { return m_portraitTexture; }

void DialogueSystem::update(float deltaTime)
{
    if (!m_dialogueActive) {
        checkTriggers();
    }

    if (m_dialogueActive) {
        if (m_isTyping) {
            updateTypewriterEffect(deltaTime);
        }

        if (m_waitingAfterCompletion) {
            m_completionTimer += deltaTime;

            if (m_completionTimer >= 5.0f) {
                m_dialogueActive = false;
                m_waitingAfterCompletion = false;

                auto playerEntities = m_entityManager.getEntities("player");
                if (!playerEntities.empty()) {
                    auto player = playerEntities[0];
                    if (player->has<CState>() && player->has<CTransform>()) {
                        player->get<CState>().state = "idle";
                        player->get<CTransform>().velocity = {0.f, 0.f};
                    }
                }
            }
        }

        // Get current message explicitly and update rendering positions/colors each frame
        const DialogueMessage* currentMessage = getCurrentMessage();
        if (currentMessage) {
            dialogueBox.setPosition(currentMessage->dialogueBoxPosition);
            dialogueBox.setSize(sf::Vector2f(currentMessage->boxWidth, currentMessage->boxHeight));

            continueText.setPosition(dialogueBox.getPosition().x + 10.f, dialogueBox.getPosition().y - 20.f);

            // Update colors and font sizes explicitly per frame
            speakerText.setFillColor(currentMessage->speakerColor);
            messageText.setFillColor(currentMessage->messageColor);
            speakerText.setCharacterSize(24); // Fixed size for speaker
            messageText.setCharacterSize(currentMessage->messageFontSize);

            // Update portrait position explicitly
            if (currentMessage->portraitOnLeft) {
                portraitSprite.setPosition(dialogueBox.getPosition().x + 10.f, dialogueBox.getPosition().y + 10.f);
                speakerText.setPosition(dialogueBox.getPosition().x + 150.f, dialogueBox.getPosition().y + 10.f);
                messageText.setPosition(dialogueBox.getPosition().x + 150.f, dialogueBox.getPosition().y + 40.f);
            } else {
                portraitSprite.setPosition(
                    dialogueBox.getPosition().x + dialogueBox.getSize().x - portraitSprite.getGlobalBounds().width - 10.f,
                    dialogueBox.getPosition().y + 10.f
                );
                speakerText.setPosition(dialogueBox.getPosition().x + 10.f, dialogueBox.getPosition().y + 10.f);
                messageText.setPosition(dialogueBox.getPosition().x + 10.f, dialogueBox.getPosition().y + 40.f);
            }

            // Set continue text explicitly above box
            continueText.setPosition(dialogueBox.getPosition().x + 10.f, dialogueBox.getPosition().y - 25.f);
        }
    }
}

void DialogueSystem::triggerDialogueByID(const std::string& dialogueID) {
    // Look up the dialogue in a map of named dialogues
    if (m_namedDialogues.find(dialogueID) != m_namedDialogues.end()) {
        m_currentDialogue = m_namedDialogues[dialogueID];
        m_currentMessageIndex = 0;
        m_dialogueActive = true;
        m_isTyping = false;
        m_waitingAfterCompletion = false;
        
        if (!m_currentDialogue.empty()) {
            startNewMessage(m_currentDialogue[0]);
        }
        
        std::cout << "[DEBUG] Triggered dialogue by ID: " << dialogueID << "\n";
    } else {
        std::cout << "[WARNING] Could not find dialogue with ID: " << dialogueID << "\n";
    }
}


void DialogueSystem::addNamedDialogue(const std::string& dialogueID, const std::vector<DialogueMessage>& dialogue) {
    m_namedDialogues[dialogueID] = dialogue;
    std::cout << "[DEBUG] Added named dialogue: " << dialogueID << " with " 
              << dialogue.size() << " messages\n";
}
