#pragma once

#include "Scene.h"
#include "EntityManager.hpp"
#include "Assets.hpp"
#include "imgui.h"
#include "imgui-SFML.h"
#include <vector>
#include <string>

class Scene_LevelEditor : public Scene {
private:
    sf::View m_cameraView;
    std::vector<std::string> m_tileOptions;
    std::string m_selectedTile;
    std::vector<std::string> m_decOptions; // Opzioni per decorazioni
    std::string m_selectedDec;           // Decorazione selezionata
    std::vector<std::string> m_enemyOptions; // Nuove opzioni per enemy
    std::string m_selectedEnemy;           // Tipo di enemy selezionato
    std::string m_currentLevelPath;        // Percorso del livello caricato
    bool mousePressed = false;
    int m_mode = 0;       // 0 = Tile, 1 = Decoration, 2 = Enemy
    float m_zoom = 1.0f;  // Zoom factor (1.0 = default)
    sf::Vector2f m_baseViewSize; // Salviamo la dimensione base del view
    float m_panSpeed = 800.0f; 
    bool m_isDragging = false;
    sf::Vector2i m_lastMousePos;
    float MIN_ZOOM = 0.5f;
    float MAX_ZOOM = 2.5f;

public:
    Scene_LevelEditor(GameEngine& game);
    ~Scene_LevelEditor();

    EntityManager m_entityManager;

    void handleEvents();
    void placeTile(int gridX, int gridY);
    void placeDec(int gridX, int gridY);
    void placeEnemy(int gridX, int gridY); // Nuova funzione per posizionare enemy
    void removeTile(int gridX, int gridY);
    void removeDec(int gridX, int gridY);
    void removeEnemy(int gridX, int gridY); // Nuova funzione per rimuovere enemy

    void saveLevel(const std::string& filePath);
    void loadLevel(const std::string& filePath);
    void loadAssets();
    void printEntities();
    virtual bool usesImGui() const override { return true; };

    void loadTileOptions();  // Funzione per caricare le opzioni delle tile
    void loadEnemyOptions(); // Nuova funzione per caricare le opzioni degli enemy
    void drawGrid();
    void renderImGui();

    void update(float deltaTime) override;
    void sRender() override;
    void sDoAction(const Action& action) override;
};
