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
    void saveLevelToFile(const std::string& savePath);
    void loadLevel(const std::string& filePath);

    // Funzioni per piazzare gli entity
    void placeTile(int gridX, int gridY);
    void placeDec(int gridX, int gridY);
    void placeEnemy(int gridX, int gridY);
    void placePlayer(int gridX, int gridY);

    // Funzioni per rimuovere gli entity
    void removeTile(int gridX, int gridY);
    void removeDec(int gridX, int gridY);
    void removeEnemy(int gridX, int gridY);
    void removePlayer();

    // Disegna la griglia e il menu ImGui
    void drawGrid();
    void renderImGui();
    
    // Caricamento delle opzioni disponibili
    void loadTileOptions();
    void loadDecOptions();
    void loadEnemyOptions();
    void loadLevelFiles();
    void setCameraToBottomLeft();

    void loadBackground(const std::string& path);
    void updateBackgroundPosition();

    // Debug
    void printEntities();

    // Zoom settings
    static constexpr float MIN_ZOOM = 0.5f;
    static constexpr float MAX_ZOOM = 10.0f;

private:
    // Gestione degli entity e della camera
    EntityManager m_entityManager;
    sf::View m_cameraView;
    
    int m_mode = 0;         // 0=Tile, 1=Decoration, 2=Enemy
    float m_zoom = 1.0f;
    bool mousePressed = false;

    sf::Texture m_backgroundTexture;
    sf::Sprite m_backgroundSprite;
    sf::Vector2f m_backgroundScale;
    
    // Percorso del file del livello corrente
    std::string m_currentLevelPath;

    // Opzioni per le tile, decorazioni e nemici
    std::vector<std::string> m_tileOptions;
    std::vector<std::string> m_decOptions;
    std::vector<std::string> m_enemyOptions;

    // Asset attualmente selezionati
    std::string m_selectedTile;
    std::string m_selectedDec;
    std::string m_selectedEnemy;

    // Livello corrente e categoria del mondo
    std::string worldcategory;
    
    // Lista dei file livelli disponibili
    std::vector<std::string> levelFiles;
    std::vector<const char*> levelFileNames;
    int selectedLevelIndex = 0;

    bool usesImGui() const override { return true; }
};
