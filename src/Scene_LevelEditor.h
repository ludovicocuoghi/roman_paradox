#pragma once

#include "Scene.h"
#include "EntityManager.hpp"
#include "Assets.hpp"
#include "imgui.h"
#include "imgui-SFML.h"

class Scene_LevelEditor : public Scene {
private:
    sf::View m_cameraView;
    std::vector<std::string> m_tileOptions;
    std::string m_selectedTile;
    std::vector<std::string> m_decOptions; // Opzioni per decorazioni
    std::string m_selectedDec;           // Decorazione selezionata
    std::string m_currentLevelPath;        // Percorso del livello caricato
    bool mousePressed = false;
    int m_mode = 0;
    float m_zoom = 1.0f;                // Zoom factor (1.0 = default)
    sf::Vector2f m_baseViewSize;        // Salviamo la dimensione base del view
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
    void placeDec(int gridX, int gridY);
    void saveLevel(const std::string& filePath);
    void loadLevel(const std::string& filePath);
    void loadAssets();
    void printEntities();
    // In Scene_LevelEditor.h
    virtual bool usesImGui() const override { return true; };
    void removeTile(int gridX, int gridY);
    void removeDec(int gridX, int gridY);

    void update(float deltaTime) override;
    void sRender() override;
    void sDoAction(const Action& action) override;

    void loadTileOptions();  // ✅ Missing function declared here
    void drawGrid();
    void renderImGui();
    void placeTile(int gridX, int gridY);
};
