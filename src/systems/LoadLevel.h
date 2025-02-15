#pragma once
#include "GameEngine.h"
#include "EntityManager.hpp"
#include <string>
#include <unordered_map>

// Se non sono già definiti altrove, qui definiamo alcune costanti usate per il posizionamento degli oggetti.
class LoadLevel {
public:
    // Dimensione della griglia (modifica questo valore in base alle esigenze)
    static constexpr float GRID_SIZE = 96;
    static constexpr float HALF_GRID = GRID_SIZE * 0.5f;

    // Offset per i pipe (calcolati come moltiplicatori della dimensione della griglia)
    static constexpr float PIPETALL_REALY_OFFSET_MULTIPLIER = 1.5f;   // GRID_SIZE/2 * 3  = GRID_SIZE * 1.5
    static constexpr float PIPESHORT_REALY_OFFSET_MULTIPLIER = 0.5f;   // GRID_SIZE/2
    static constexpr float PIPE_REALY_OFFSET_MULTIPLIER = 1.0f;        // GRID_SIZE/2 * 2  = GRID_SIZE
    static constexpr float PIPE_REALX_OFFSET_MULTIPLIER = 0.5f;        // GRID_SIZE/2

    // Se non sono già definiti nel progetto, qui definiamo i valori per il bounding box del player e la gravità
    static constexpr float PLAYER_BB_SIZE = 80.f;
    static constexpr float GRAVITY_VAL = 1000.f;

    LoadLevel(GameEngine& game);

    void load(const std::string& levelPath, EntityManager& entityManager);

private:
    GameEngine& m_game;
};
