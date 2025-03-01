#include "Scene_LevelEditor.h"
#include "GameEngine.h"
#include <iostream>
#include <fstream>
#include <cmath>      // per std::floor
#include <filesystem> // per std::filesystem (C++17)

namespace fs = std::filesystem;

static EnemyType getEnemyType(const std::string &typeStr) {
    if (typeStr == "EnemyFast")
        return EnemyType::Fast;
    else if (typeStr == "EnemyStrong")
        return EnemyType::Strong;
    else if (typeStr == "EnemyElite")
        return EnemyType::Elite;
    else if (typeStr == "EnemySuper")
        return EnemyType::Super;
    else if (typeStr == "Emperor")
        return EnemyType::Emperor;
    else
        return EnemyType::Normal;
}

// Dimensione della cella/griglia
constexpr int tileSize = 96;
constexpr int worldWidth = 500;
constexpr int worldHeight = 60;
constexpr float CAMERA_SPEED = 1200.f; // velocità della camera

Scene_LevelEditor::Scene_LevelEditor(GameEngine& game)
    : Scene(game), m_mode(0), m_zoom(1.0f)
{
    m_entityManager = EntityManager();

    if (!ImGui::SFML::Init(m_game.window())) {
        std::cerr << "[ERROR] ImGui::SFML::Init() failed!\n";
    }

    m_cameraView = m_game.window().getDefaultView();
    m_game.window().setView(m_cameraView);

    registerAction(sf::Keyboard::Escape, "BACK");

    loadTileOptions();
    loadDecOptions();
    loadEnemyOptions();
    loadLevelFiles();
}


Scene_LevelEditor::~Scene_LevelEditor() {
    ImGui::SFML::Shutdown();
}

void Scene_LevelEditor::loadLevelFiles() {
    levelFiles.clear();
    levelFileNames.clear();
    selectedLevelIndex = 0;

    std::string levelPath = "src/levels/";
    if (!fs::exists(levelPath) || !fs::is_directory(levelPath)) {
        std::cerr << "[ERROR] Level directory not found: " << levelPath << "\n";
        return;
    }

    for (const auto& entry : fs::directory_iterator(levelPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".txt") {
            levelFiles.push_back(entry.path().filename().string());
        }
    }

    for (auto& file : levelFiles) {
        levelFileNames.push_back(file.c_str());
    }
}

void Scene_LevelEditor::update(float deltaTime) {
    ImGui::SFML::Update(m_game.window(), sf::seconds(deltaTime));

    ImGuiIO& io = ImGui::GetIO();
    bool imguiHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);
    if (io.WantCaptureMouse || imguiHovered) {
        // When ImGui is active, do not process game input.
    } else {
        // Process game input
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
            m_cameraView.move(-CAMERA_SPEED * deltaTime, 0.f);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
            m_cameraView.move(CAMERA_SPEED * deltaTime, 0.f);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
            m_cameraView.move(0.f, -CAMERA_SPEED * deltaTime);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
            m_cameraView.move(0.f, CAMERA_SPEED * deltaTime);

        // Apply zoom to camera view
        sf::Vector2f newSize = m_game.window().getDefaultView().getSize() * m_zoom;
        m_cameraView.setSize(newSize);
        m_game.window().setView(m_cameraView);

        m_entityManager.update();

        // Left mouse click handling
        if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
            if (!mousePressed) {
                sf::Vector2i mousePosScreen = sf::Mouse::getPosition(m_game.window());
                sf::Vector2f mousePosWorld = m_game.window().mapPixelToCoords(mousePosScreen, m_cameraView);
                int cellX = static_cast<int>(mousePosWorld.x / tileSize);
                int cellY = static_cast<int>(mousePosWorld.y / tileSize);
                std::cout << "[DEBUG] Click sinistro in (" << cellX << ", " << cellY << ")\n";
                if (m_mode == 0)
                    placeTile(cellX, cellY);
                else if (m_mode == 1)
                    placeDec(cellX, cellY);
                else if (m_mode == 2)
                    placeEnemy(cellX, cellY);
                mousePressed = true;
            }
        }
        // Right mouse click handling
        else if (sf::Mouse::isButtonPressed(sf::Mouse::Right)) {
            if (!mousePressed) {
                sf::Vector2i mousePosScreen = sf::Mouse::getPosition(m_game.window());
                sf::Vector2f mousePosWorld = m_game.window().mapPixelToCoords(mousePosScreen, m_cameraView);
                int cellX = static_cast<int>(mousePosWorld.x / tileSize);
                int cellY = static_cast<int>(mousePosWorld.y / tileSize);
                std::cout << "[DEBUG] Click destro in (" << cellX << ", " << cellY << ")\n";
                if (m_mode == 0)
                    removeTile(cellX, cellY);
                else if (m_mode == 1)
                    removeDec(cellX, cellY);
                else if (m_mode == 2)
                    removeEnemy(cellX, cellY);
                mousePressed = true;
            }
        }
        else {
            mousePressed = false;
        }
    }
}

void Scene_LevelEditor::sRender() {
    m_game.window().clear(sf::Color(100, 100, 255));
    m_game.window().setView(m_cameraView);
    drawGrid();

    // Draw game entities
    for (auto& entity : m_entityManager.getEntities()) {
        if (!entity->has<CTransform>())
            continue;
        auto& transform = entity->get<CTransform>();
        sf::Sprite sprite;
        if (entity->has<CAnimation>())
            sprite = entity->get<CAnimation>().animation.getSprite();
        sprite.setOrigin(0.f, 0.f);
        sprite.setScale(1.f, 1.f);
        sprite.setPosition(transform.pos.x, transform.pos.y);
        m_game.window().draw(sprite);
    }

    // Switch to default view for ImGui
    m_game.window().setView(m_game.window().getDefaultView());
    renderImGui();
    ImGui::SFML::Render(m_game.window());
    m_game.window().display();
}


void Scene_LevelEditor::sDoAction(const Action& action) {
    if (action.type() == "START" && action.name() == "BACK") {
        // Implementa il cambio scena se necessario
        return;
    }
}


void Scene_LevelEditor::placeTile(int gridX, int gridY) {
    float realX = gridX * tileSize;
    float realY = gridY * tileSize;
    for (auto& tile : m_entityManager.getEntities("tile")) {
        auto& transform = tile->get<CTransform>();
        if (std::abs(transform.pos.x - realX) < 0.1f &&
            std::abs(transform.pos.y - realY) < 0.1f) {
            std::cout << "[DEBUG] Tile già presente in (" << gridX << ", " << gridY << "), salto.\n";
            return;
        }
    }
    auto tile = m_entityManager.addEntity("tile");
    tile->add<CTransform>(Vec2<float>(realX, realY));
    std::string fullName = worldcategory + m_selectedTile;
    if (m_game.assets().hasAnimation(fullName)) {
        tile->add<CAnimation>(m_game.assets().getAnimation(fullName), true);
        std::cout << "[DEBUG] Tile posizionata: " << fullName << " in (" << gridX << ", " << gridY << ")\n";
    }
    else
        std::cerr << "[ERROR] Animazione mancante per: " << fullName << "\n";
}

void Scene_LevelEditor::placeDec(int gridX, int gridY) {
    float realX = gridX * tileSize;
    float realY = gridY * tileSize;
    for (auto& dec : m_entityManager.getEntities("decoration")) {
        auto& transform = dec->get<CTransform>();
        if (std::abs(transform.pos.x - realX) < 0.1f &&
            std::abs(transform.pos.y - realY) < 0.1f) {
            std::cout << "[DEBUG] Decorazione già presente in (" << gridX << ", " << gridY << "), salto.\n";
            return;
        }
    }
    auto dec = m_entityManager.addEntity("decoration");
    dec->add<CTransform>(Vec2<float>(realX, realY));
    std::string fullName = worldcategory + m_selectedDec;
    if (m_game.assets().hasAnimation(fullName)) {
        dec->add<CAnimation>(m_game.assets().getAnimation(fullName), true);
        std::cout << "[DEBUG] Decorazione posizionata: " << fullName << " in (" << gridX << ", " << gridY << ")\n";
    }
    else
        std::cerr << "[ERROR] Animazione mancante per: " << fullName << "\n";
}

void Scene_LevelEditor::placeEnemy(int gridX, int gridY) {
    float realX = gridX * tileSize;
    float realY = gridY * tileSize;
    for (auto& enemy : m_entityManager.getEntities("enemy")) {
        auto& transform = enemy->get<CTransform>();
        if (std::abs(transform.pos.x - realX) < 0.1f &&
            std::abs(transform.pos.y - realY) < 0.1f) {
            std::cout << "[DEBUG] Enemy già presente in (" << gridX << ", " << gridY << "), salto.\n";
            return;
        }
    }
    auto enemy = m_entityManager.addEntity("enemy");
    enemy->add<CTransform>(Vec2<float>(realX, realY));
    std::string fullName = worldcategory + "Stand" + m_selectedEnemy;
    if (m_game.assets().hasAnimation(fullName))
        enemy->add<CAnimation>(m_game.assets().getAnimation(fullName), true);
    else
        std::cerr << "[ERROR] Animazione mancante per: " << fullName << "\n";
    enemy->add<CEnemyAI>(getEnemyType(m_selectedEnemy));
    std::cout << "[DEBUG] Enemy posizionato: " << m_selectedEnemy << " in (" << gridX << ", " << gridY << ")\n";
}


void Scene_LevelEditor::removeTile(int gridX, int gridY) {
    float realX = gridX * tileSize;
    float realY = gridY * tileSize;
    
    auto& tiles = m_entityManager.getEntities("tile");
    for (auto& tile : tiles) {
        auto& transform = tile->get<CTransform>();
        if (std::abs(transform.pos.x - realX) < 0.1f &&
            std::abs(transform.pos.y - realY) < 0.1f) {
            tile->destroy();
            std::cout << "[DEBUG] Tile rimossa in (" << gridX << ", " << gridY << ")\n";
            return;
        }
    }
    std::cout << "[DEBUG] Nessuna tile trovata in (" << gridX << ", " << gridY << ") per rimuovere.\n";
}

void Scene_LevelEditor::removeDec(int gridX, int gridY) {
    float realX = gridX * tileSize;
    float realY = gridY * tileSize;
    
    auto& decs = m_entityManager.getEntities("decoration");
    for (auto& dec : decs) {
        auto& transform = dec->get<CTransform>();
        if (std::abs(transform.pos.x - realX) < 0.1f &&
            std::abs(transform.pos.y - realY) < 0.1f) {
            dec->destroy();
            std::cout << "[DEBUG] Decorazione rimossa in (" << gridX << ", " << gridY << ")\n";
            return;
        }
    }
    std::cout << "[DEBUG] Nessuna decorazione trovata in (" << gridX << ", " << gridY << ") per rimuovere.\n";
}

void Scene_LevelEditor::removeEnemy(int gridX, int gridY) {
    float realX = gridX * tileSize;
    float realY = gridY * tileSize;
    
    auto& enemies = m_entityManager.getEntities("enemy");
    for (auto& enemy : enemies) {
        auto& transform = enemy->get<CTransform>();
        if (std::abs(transform.pos.x - realX) < 0.1f &&
            std::abs(transform.pos.y - realY) < 0.1f) {
            enemy->destroy();
            std::cout << "[DEBUG] Enemy rimosso in (" << gridX << ", " << gridY << ")\n";
            return;
        }
    }
    std::cout << "[DEBUG] Nessun enemy trovato in (" << gridX << ", " << gridY << ") per rimuovere.\n";
}

void Scene_LevelEditor::drawGrid() {
    sf::Color gridColor(255, 255, 255, 100);
    
    for (int x = 0; x <= worldWidth; ++x) {
        sf::Vertex line[] = {
            sf::Vertex(sf::Vector2f(x * tileSize, 0), gridColor),
            sf::Vertex(sf::Vector2f(x * tileSize, worldHeight * tileSize), gridColor)
        };
        m_game.window().draw(line, 2, sf::Lines);
    }
    for (int y = 0; y <= worldHeight; ++y) {
        sf::Vertex line[] = {
            sf::Vertex(sf::Vector2f(0, y * tileSize), gridColor),
            sf::Vertex(sf::Vector2f(worldWidth * tileSize, y * tileSize), gridColor)
        };
        m_game.window().draw(line, 2, sf::Lines);
    }
}
void Scene_LevelEditor::renderImGui() {
    ImGui::Begin("Level Editor");

    ImGui::Text("Modalità:");
    ImGui::RadioButton("Tile", &m_mode, 0);
    ImGui::SameLine();
    ImGui::RadioButton("Decoration", &m_mode, 1);
    ImGui::SameLine();
    ImGui::RadioButton("Enemy", &m_mode, 2);

    // Zoom slider
    ImGui::Text("Zoom:");
    ImGui::SliderFloat("##zoom", &m_zoom, MIN_ZOOM, MAX_ZOOM);

    // Display available assets based on mode
    if (m_mode == 0) {
        ImGui::Text("Seleziona una Tile:");
        for (const auto& tile : m_tileOptions) {
            if (ImGui::Selectable(tile.c_str(), m_selectedTile == tile)) {
                m_selectedTile = tile;
                std::cout << "[DEBUG] Tile selezionata: " << m_selectedTile << "\n";
            }
        }
    }
    else if (m_mode == 1) {
        ImGui::Text("Seleziona una Decorazione:");
        for (const auto& dec : m_decOptions) {
            if (ImGui::Selectable(dec.c_str(), m_selectedDec == dec)) {
                m_selectedDec = dec;
                std::cout << "[DEBUG] Decoration selezionata: " << m_selectedDec << "\n";
            }
        }
    }
    else if (m_mode == 2) {
        ImGui::Text("Seleziona un Enemy:");
        for (const auto& enemy : m_enemyOptions) {
            if (ImGui::Selectable(enemy.c_str(), m_selectedEnemy == enemy)) {
                m_selectedEnemy = enemy;
                std::cout << "[DEBUG] Enemy selezionato: " << m_selectedEnemy << "\n";
            }
        }
    }

    // Level file selection dropdown and load/save buttons
    ImGui::Text("Seleziona un file livello:");
    if (!levelFiles.empty()) {
        if (ImGui::Combo("##levelSelect", &selectedLevelIndex, levelFileNames.data(), static_cast<int>(levelFileNames.size()))) {
            std::cout << "[DEBUG] File livello selezionato: " << levelFiles[selectedLevelIndex] << "\n";
        }
    } else {
        ImGui::Text("[ERROR] No level files found.");
    }

    if (ImGui::Button("Carica")) {
        if (!levelFiles.empty()) {
            std::string filePath = "src/levels/" + levelFiles[selectedLevelIndex];
            loadLevel(filePath);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Salva")) {
        std::string filePath = m_currentLevelPath.empty() ? "src/levels/level_save.txt" : m_currentLevelPath;
        saveLevel(filePath);
    }

    ImGui::End();
}

void Scene_LevelEditor::loadTileOptions() {
    m_tileOptions = { "Ground", "Brick", "Box1", "Box2", "PipeTall", "Pipe", "PipeBroken", "Treasure",  "LevelDoor", "BlackHoleRedBig", "Armor" };
    if (!m_tileOptions.empty()) {
        m_selectedTile = m_tileOptions[0];
    }
}

void Scene_LevelEditor::loadDecOptions() {
    m_decOptions = { "BushSmall", "BushTall","BushTall1", "BushTall2", "CloudSmall", "CloudBig" , "EnemyGrave"};
    if (!m_decOptions.empty()) {
        m_selectedDec = m_decOptions[0];
    }
}

void Scene_LevelEditor::loadEnemyOptions() {
    m_enemyOptions = { "EnemyFast", "EnemyStrong", "EnemyElite", "EnemyNormal", "EnemySuper", "Emperor" };
    if (!m_enemyOptions.empty()) {
        m_selectedEnemy = m_enemyOptions[0];
    }
}

void Scene_LevelEditor::saveLevel(const std::string& filePath) {
    m_entityManager.update();
    std::ofstream out(filePath);
    if (!out.is_open()) {
        std::cerr << "[ERROR] Cannot open file for saving: " << filePath << "\n";
        return;
    }

    auto stripWorldCategory = [this](const std::string& fullName) -> std::string {
        if (!worldcategory.empty() && fullName.find(worldcategory) == 0)
            return fullName.substr(worldcategory.size());
        return fullName;
    };

    // ========================
    // ✅ Save Tiles
    // ========================
    for (auto& tile : m_entityManager.getEntities("tile")) {
        auto& transform = tile->get<CTransform>();
        int gridX = static_cast<int>(transform.pos.x / tileSize);
        int gridY = static_cast<int>(transform.pos.y / tileSize);
        int savedGridY = worldHeight - 1 - gridY;

        // Ensure the tile has an animation
        if (!tile->has<CAnimation>()) {
            std::cerr << "[ERROR] Tile at (" << gridX << ", " << savedGridY << ") is missing an animation!\n";
            continue;
        }

        std::string animName = tile->get<CAnimation>().animation.getName();
        animName = stripWorldCategory(animName);

        // **Check if animName is empty**
        if (animName.empty()) {
            std::cerr << "[ERROR] Empty tile name at (" << gridX << ", " << savedGridY << ")! Skipping...\n";
            continue;
        }

        out << "Tile " << animName << " " << gridX << " " << savedGridY << "\n";
    }
    
    // ========================
    // ✅ Save Decorations
    // ========================
    for (auto& dec : m_entityManager.getEntities("decoration")) {
        auto& transform = dec->get<CTransform>();
        int gridX = static_cast<int>(transform.pos.x / tileSize);
        int gridY = static_cast<int>(transform.pos.y / tileSize);
        int savedGridY = worldHeight - 1 - gridY;

        // Ensure the decoration has an animation component
        if (!dec->has<CAnimation>()) {
            std::cerr << "[ERROR] Decoration at (" << gridX << ", " << savedGridY << ") is missing an animation!\n";
            continue;
        }

        std::string animName = dec->get<CAnimation>().animation.getName();
        animName = stripWorldCategory(animName);

        // **Check if animName is empty**
        if (animName.empty()) {
            std::cerr << "[ERROR] Empty decoration name at (" << gridX << ", " << savedGridY << ")! Skipping...\n";
            continue;
        }

        out << "Dec " << animName << " " << gridX << " " << savedGridY << "\n";
    }
    
    // ========================
    // ✅ Save Enemies
    // ========================
    for (auto& enemy : m_entityManager.getEntities("enemy")) {
        auto& transform = enemy->get<CTransform>();
        int gridX = static_cast<int>(transform.pos.x / tileSize);
        int gridY = static_cast<int>(transform.pos.y / tileSize);
        int savedGridY = worldHeight - 1 - gridY;
        
        // Ensure the enemy has an AI component
        if (!enemy->has<CEnemyAI>()) {
            std::cerr << "[ERROR] Enemy at (" << gridX << ", " << savedGridY << ") is missing AI! Skipping...\n";
            continue;
        }

        std::string enemyType;
        switch (enemy->get<CEnemyAI>().enemyType) {
            case EnemyType::Fast:    enemyType = "EnemyFast"; break;
            case EnemyType::Strong:  enemyType = "EnemyStrong"; break;
            case EnemyType::Elite:   enemyType = "EnemyElite"; break;
            case EnemyType::Normal:   enemyType = "EnemyNormal"; break;
            case EnemyType::Super:   enemyType = "EnemySuper"; break;
            case EnemyType::Emperor: enemyType = "Emperor"; break;
            default:
                std::cerr << "[ERROR] Unknown enemy type at (" << gridX << ", " << savedGridY << ")! Skipping...\n";
                continue;
        }

        // Ensure enemyType is valid
        if (enemyType.empty()) {
            std::cerr << "[ERROR] Empty enemy type at (" << gridX << ", " << savedGridY << ")! Skipping...\n";
            continue;
        }

        out << "Enemy " << enemyType << " " << gridX << " " << savedGridY << " "
            << gridX << " " << savedGridY << " " << (gridX + 2) << " " << savedGridY << "\n";
    }

    out.close();
    std::cout << "[DEBUG] Level saved to " << filePath << "\n";
}


void Scene_LevelEditor::loadLevel(const std::string& filePath) {
    // Compute world category from file path.
    if (filePath.find("ancient") != std::string::npos)
        worldcategory = "Ancient";
    else if (filePath.find("alien") != std::string::npos)
        worldcategory = "Alien";
    else if (filePath.find("future") != std::string::npos)
        worldcategory = "Future";
    else
        worldcategory = "";
    
    m_entityManager.clear();
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "[ERROR] Failed to open level file: " << filePath << "\n";
        return;
    }
    std::string token;
    while (file >> token) {
        if (token == "Tile") {
            std::string assetType;
            int fileGridX, fileGridY;
            file >> assetType >> fileGridX >> fileGridY;
            int gridX = fileGridX;
            int gridY = worldHeight - 1 - fileGridY;
            auto entity = m_entityManager.addEntity("tile");
            entity->add<CTransform>(Vec2<float>(gridX * tileSize, gridY * tileSize));

            std::string fullAsset = worldcategory + assetType;
            
            // **Ensure Treasure is properly prefixed**

            if (m_game.assets().hasAnimation(fullAsset)) {
                entity->add<CAnimation>(m_game.assets().getAnimation(fullAsset), true);
            } else {
                std::cerr << "[ERROR] Missing animation for " << fullAsset << "\n";
            }
        }
        else if (token == "Dec") {
            std::string assetType;
            int fileGridX, fileGridY;
            file >> assetType >> fileGridX >> fileGridY;
            int gridX = fileGridX;
            int gridY = worldHeight - 1 - fileGridY;
            auto entity = m_entityManager.addEntity("decoration");
            entity->add<CTransform>(Vec2<float>(gridX * tileSize, gridY * tileSize));
            std::string fullAsset = worldcategory + assetType;
            if (m_game.assets().hasAnimation(fullAsset))
                entity->add<CAnimation>(m_game.assets().getAnimation(fullAsset), true);
            else
                std::cerr << "[ERROR] Missing animation for " << fullAsset << "\n";
            std::cout << "[DEBUG] Loaded Dec: " << fullAsset << " at (" << gridX << ", " << gridY << ")\n";
        }
        else if (token == "Enemy") {
            std::string enemyType;
            int fileGridX, fileGridY;
            file >> enemyType >> fileGridX >> fileGridY;
            int gridX = fileGridX;
            int gridY = worldHeight - 1 - fileGridY;
            auto entity = m_entityManager.addEntity("enemy");
            entity->add<CTransform>(Vec2<float>(gridX * tileSize, gridY * tileSize));
            entity->add<CEnemyAI>(getEnemyType(enemyType));
            // Build the stand animation name as "WorldCategory" + "StandAnim" + enemyType.
            std::string standAnim = worldcategory + "Stand" + enemyType;
            if (m_game.assets().hasAnimation(standAnim))
                entity->add<CAnimation>(m_game.assets().getAnimation(standAnim), true);
            else
                std::cerr << "[ERROR] Missing animation for enemy type: " << standAnim << "\n";
            std::cout << "[DEBUG] Loaded Enemy: " << worldcategory + enemyType << " at (" << gridX << ", " << gridY << ")\n";
        }
        else if (token == "Player") {
            std::string rest;
            std::getline(file, rest);
            std::cout << "[DEBUG] Player line skipped.\n";
        }
    }
    file.close();
    m_entityManager.update();
    m_currentLevelPath = filePath;
    std::cout << "[DEBUG] Level loaded from " << filePath << "\n";
}

void Scene_LevelEditor::printEntities() {
    auto tiles = m_entityManager.getEntities("tile");
    std::cout << "[DEBUG] Entities (tile): " << tiles.size() << "\n";
    for (auto& tile : tiles) {
        auto& transform = tile->get<CTransform>();
        std::cout << "[DEBUG] Tile: pos (" << transform.pos.x << ", " << transform.pos.y << ")\n";
    }
    auto decs = m_entityManager.getEntities("decoration");
    std::cout << "[DEBUG] Entities (decoration): " << decs.size() << "\n";
    for (auto& dec : decs) {
        auto& transform = dec->get<CTransform>();
        std::cout << "[DEBUG] Decoration: pos (" << transform.pos.x << ", " << transform.pos.y << ")\n";
    }
    auto enemies = m_entityManager.getEntities("enemy");
    std::cout << "[DEBUG] Entities (enemy): " << enemies.size() << "\n";
    for (auto& enemy : enemies) {
        auto& transform = enemy->get<CTransform>();
        std::cout << "[DEBUG] Enemy: pos (" << transform.pos.x << ", " << transform.pos.y << ")\n";
    }
}
