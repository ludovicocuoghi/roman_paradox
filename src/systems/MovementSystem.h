#pragma once

#include <SFML/Graphics.hpp>
#include "EntityManager.hpp"
#include "GameEngine.h"
#include "Components.hpp"
#include "Vec2.hpp"

// Il MovementSystem si occupa di aggiornare il movimento del giocatore e della telecamera.
class MovementSystem {
public:
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

    static constexpr float xSpeed = 350.f;
    // Parametri di salto e caduta:
    // jumpBoostAcceleration: Impulso verticale extra applicato quando il tasto di salto viene tenuto.
    //   - Aumentandolo, il personaggio riceverà un impulso più forte in salita (quindi salterà più rapidamente).
    //   - Diminuendolo, l'impulso in salita sarà più debole e il salto risulterà meno "snappy".
    static constexpr float jumpBoostAcceleration = 4500.f;   

    // maxJumpHoldTime: Tempo massimo (in secondi) durante il quale l'impulso extra viene applicato mentre il tasto di salto è premuto.
    //   - Valori più alti permettono di accumulare più impulso, portando a un salto più alto.
    //   - Valori più bassi riducono il tempo in cui il boost è attivo, limitando l'altezza del salto.
    static constexpr float maxJumpHoldTime = 0.09f;           

    // MaxUpwardVelocity: Velocità massima in salita (valore negativo, poiché l'asse Y aumenta verso il basso).
    //   - Un valore più negativo permette una salita più veloce e, in teoria, un salto più alto (fino al limite imposto da maxJumpHoldTime).
    //   - Un valore meno negativo limiterà la velocità in salita, rendendo il salto meno rapido.
    static constexpr float MaxUpwardVelocity = -1900.f;       

    // GravityMultiplier: Moltiplicatore della gravità applicato quando il personaggio non sta più saltando e sta cadendo.
    //   - Aumentandolo, la caduta diventa più rapida, rendendo il salto più "snappy" e dinamico.
    //   - Diminuendolo, la caduta sarà più lenta e "fluttuante".
    static constexpr float GravityMultiplier = 1.5f;          

    // MAX_FALL_SPEED: Velocità massima di caduta.
    //   - Valori maggiori permettono una caduta più veloce, mentre valori inferiori limitano la velocità durante la discesa.
    static constexpr float MAX_FALL_SPEED = 3000.f;           

    // CAMERA_Y_OFFSET: Offset verticale della camera rispetto al giocatore.
    //   - Valori più negativi spostano la camera più in alto rispetto al giocatore.
    static constexpr float CAMERA_Y_OFFSET = -80.f;

    // CAMERA_SMOOTHING_FACTOR: Fattore di smoothing per il movimento della camera.
    //   - Valori vicini a 1 fanno seguire la camera il giocatore in modo più stretto (meno smoothing).
    //   - Valori più bassi rendono il movimento della camera più morbido e dilazionato.
    static constexpr float CAMERA_SMOOTHING_FACTOR = 0.1f;
};
