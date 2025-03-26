#pragma once
#include "GameEngine.h"
#include "EntityManager.hpp"
#include "Components.hpp"
#include <SFML/Graphics.hpp>

class MovementSystem {
public:
    // Costanti per il movimento del giocatore
    static constexpr float MAX_JUMP_HOLD_TIME = 0.11f;        // Durata massima per mantenere il salto
    static constexpr float JUMP_BOOST_ACCELERATION = 4500.f;       // Accelerazione aggiuntiva durante il salto
    static constexpr float MAX_UPWARD_VELOCITY = -1900.f;          // Velocità massima verso l'alto (valore negativo)
    static constexpr float GRAVITY_MULTIPLIER = 1.5f;             // Moltiplicatore della gravità durante la caduta
    static constexpr float MAX_FALL_SPEED = 3000.f;                // Velocità massima di caduta
    static constexpr float X_SPEED = 350.f;                       // Velocità orizzontale del giocatore

    // Costanti per il movimento della telecamera
    void updateCamera();
    // Costanti per il posizionamento della spada (che segue il giocatore)
    static constexpr float SWORD_OFFSET_X_LEFT = -80.f;
    static constexpr float SWORD_OFFSET_X_RIGHT = 30.f;
    static constexpr float SWORD_OFFSET_Y = 10.f;
    static constexpr float ZOOM_STRENGTH = 1.0f;

    MovementSystem(GameEngine& game,
                   EntityManager& entityManager,
                   sf::View& cameraView,
                   float& lastDirection);

    void update(float deltaTime);

private:
    GameEngine& m_game;
    EntityManager& m_entityManager;
    sf::View& m_cameraView;
    float& m_lastDirection;
    float m_currentZoom = 1.0f;
};
