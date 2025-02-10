#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include "EntityManager.hpp"
#include "GameEngine.h"
#include <SFML/Graphics.hpp>

// Includi il file che definisce EnemyType (es. Components.hpp)
#include "Components.hpp"

// Rimuovi la definizione duplicata di EnemyType qui!

class LoadLevel {
public:
    // --- Costanti di configurazione ---
    static const int GRID_SIZE = 96;
    static constexpr float GRAVITY_VAL = 1000.f;
    static constexpr float PLAYER_BB_SIZE = 80.f;

    // Costruttore: riceve un riferimento al motore di gioco.
    LoadLevel(GameEngine& game);

    // Carica il livello dal file specificato e aggiorna l'EntityManager.
    void load(const std::string& levelPath, EntityManager& entityManager);

private:
    GameEngine& m_game;
};
