#pragma once
#include "GameEngine.h"
#include "EntityManager.hpp"
#include "Components.hpp"
#include <SFML/Graphics.hpp>

class MovementSystem {
public:
    // Costanti per il movimento del giocatore
    static constexpr float MAX_JUMP_HOLD_TIME = 0.09f;        // Durata massima per mantenere il salto
    static constexpr float JUMP_BOOST_ACCELERATION = 4500.f;       // Accelerazione aggiuntiva durante il salto
    static constexpr float MAX_UPWARD_VELOCITY = -1900.f;          // Velocità massima verso l'alto (valore negativo)
    static constexpr float GRAVITY_MULTIPLIER = 1.5f;             // Moltiplicatore della gravità durante la caduta
    static constexpr float MAX_FALL_SPEED = 3000.f;                // Velocità massima di caduta
    static constexpr float X_SPEED = 350.f;                       // Velocità orizzontale del giocatore

    // Costanti per il movimento della telecamera
    static constexpr float CAMERA_THRESHOLD_X_FACTOR = 0.25f;     // Fattore per la soglia orizzontale
    static constexpr float CAMERA_THRESHOLD_Y_FACTOR = 0.15f;     // Fattore per la soglia verticale
    static constexpr float CAMERA_Y_OFFSET = -80.f;                // Offset verticale della telecamera
    static constexpr float CAMERA_SMOOTHING_FACTOR = 0.1f;        // Fattore di smoothing per la telecamera

    // Costanti per il posizionamento della spada (che segue il giocatore)
    static constexpr float SWORD_OFFSET_X_LEFT = -80.f;
    static constexpr float SWORD_OFFSET_X_RIGHT = 30.f;
    static constexpr float SWORD_OFFSET_Y = 10.f;

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
};
