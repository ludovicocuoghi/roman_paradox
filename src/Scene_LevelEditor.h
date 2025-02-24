#pragma once

#include "Scene.h"
#include "EntityManager.hpp"
#include "Assets.hpp"
#include "imgui.h"
#include "imgui-SFML.h"
#include <vector>
#include <string>

class Scene_LevelEditor : public Scene {
public:
    Scene_LevelEditor(GameEngine& game);
    ~Scene_LevelEditor();

    // Metodi principali della scena
    void update(float deltaTime) override;
    void sRender() override;
    void sDoAction(const Action& action) override;

    // Salvataggio/Caricamento livello
    void saveLevel(const std::string& filePath);
    void loadLevel(const std::string& filePath);

    // Funzioni per piazzare gli entity
    void placeTile(int gridX, int gridY);
    void placeDec(int gridX, int gridY);
    void placeEnemy(int gridX, int gridY);

    // Funzioni per rimuovere gli entity
    void removeTile(int gridX, int gridY);
    void removeDec(int gridX, int gridY);
    void removeEnemy(int gridX, int gridY);

    float MIN_ZOOM = 0.1f;
    float MAX_ZOOM = 2.0f;

    // Disegna la griglia e il menu ImGui
    void drawGrid();
    void renderImGui();

    // Stampa le entità per il debug
    void printEntities();

    // Funzioni per caricare le opzioni
    void loadTileOptions();
    void loadDecOptions();
    void loadEnemyOptions();

private:
    // Gestione degli entity e della camera
    EntityManager m_entityManager;
    sf::View m_cameraView;
    int m_mode;         // 0=Tile, 1=Decoration, 2=Enemy, 3=Player
    float m_zoom;
    bool mousePressed;
    bool usesImGui() const override { return true; }

    // Percorso del file del livello corrente
    std::string m_currentLevelPath;

    // Opzioni per le tile
    std::vector<std::string> m_ancientRomanTileOptions;
    std::vector<std::string> m_alienTileOptions;
    std::vector<std::string> m_quantumTileOptions;

    // Opzioni per le decorazioni
    std::vector<std::string> m_ancientRomanDecOptions;
    std::vector<std::string> m_alienDecOptions;
    std::vector<std::string> m_quantumDecOptions;

    // Opzioni per i nemici
    std::vector<std::string> m_ancientRomanEnemyOptions;
    std::vector<std::string> m_alienEnemyOptions;
    std::vector<std::string> m_quantumEnemyOptions;

    // Asset attualmente selezionati
    std::string m_selectedTile;
    std::string m_selectedDec;
    std::string m_selectedEnemy;
};
