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

    bool imguiInitOK = ImGui::SFML::Init(m_game.window());
    if (!imguiInitOK) {
        std::cerr << "[ERROR] ImGui::SFML::Init() failed!\n";
    }

    m_cameraView = m_game.window().getDefaultView();
    m_game.window().setView(m_cameraView);

    registerAction(sf::Keyboard::Escape, "BACK");

    // Carica le opzioni per tile, decorazioni ed enemy
    loadTileOptions();
    loadDecOptions();
    loadEnemyOptions();
}

Scene_LevelEditor::~Scene_LevelEditor() {
    ImGui::SFML::Shutdown();
}

void Scene_LevelEditor::update(float deltaTime) {
    ImGui::SFML::Update(m_game.window(), sf::seconds(deltaTime));

    ImGuiIO& io = ImGui::GetIO();
    bool imguiHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);
    if (io.WantCaptureMouse || imguiHovered) {
        std::cout << "[DEBUG] Il mouse è sopra ImGui." << std::endl;
        // Non processare input di gioco quando il mouse è sopra ImGui
    } else {
        //std::cout << "[DEBUG] ImGui NON sta catturando il mouse." << std::endl;
        // Processa input di gioco

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
            m_cameraView.move(-CAMERA_SPEED * deltaTime, 0.f);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
            m_cameraView.move(CAMERA_SPEED * deltaTime, 0.f);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
            m_cameraView.move(0.f, -CAMERA_SPEED * deltaTime);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
            m_cameraView.move(0.f, CAMERA_SPEED * deltaTime);

        sf::Vector2f newSize = m_game.window().getDefaultView().getSize() * m_zoom;
        m_cameraView.setSize(newSize);
        m_game.window().setView(m_cameraView);

        m_entityManager.update();

        // Gestione click sinistro
        if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
            if (!mousePressed) {
                sf::Vector2i mousePosScreen = sf::Mouse::getPosition(m_game.window());
                sf::Vector2f mousePosWorld = m_game.window().mapPixelToCoords(mousePosScreen, m_cameraView);
                int cellX = static_cast<int>(mousePosWorld.x / tileSize);
                int cellY = static_cast<int>(mousePosWorld.y / tileSize);
                std::cout << "[DEBUG] Click sinistro in (" << cellX << ", " << cellY << ")" << std::endl;
                if (m_mode == 0)
                    placeTile(cellX, cellY);
                else if (m_mode == 1)
                    placeDec(cellX, cellY);
                else if (m_mode == 2)
                    placeEnemy(cellX, cellY);
                mousePressed = true;
            }
        } else if (sf::Mouse::isButtonPressed(sf::Mouse::Right)) {
            if (!mousePressed) {
                sf::Vector2i mousePosScreen = sf::Mouse::getPosition(m_game.window());
                sf::Vector2f mousePosWorld = m_game.window().mapPixelToCoords(mousePosScreen, m_cameraView);
                int cellX = static_cast<int>(mousePosWorld.x / tileSize);
                int cellY = static_cast<int>(mousePosWorld.y / tileSize);
                std::cout << "[DEBUG] Click destro in (" << cellX << ", " << cellY << ")" << std::endl;
                if (m_mode == 0)
                    removeTile(cellX, cellY);
                else if (m_mode == 1)
                    removeDec(cellX, cellY);
                else if (m_mode == 2)
                    removeEnemy(cellX, cellY);
                mousePressed = true;
            }
        } else {
            mousePressed = false;
        }
    }
}

void Scene_LevelEditor::sRender() {
    m_game.window().clear(sf::Color(100, 100, 255));
    m_game.window().setView(m_cameraView);
    drawGrid();

    // Disegna le entità di gioco
    for (auto& entity : m_entityManager.getEntities()) {
        if (!entity->has<CTransform>())
            continue;
        auto& transform = entity->get<CTransform>();
        sf::Sprite sprite;
        if (entity->has<CAnimation>())
            sprite = entity->get<CAnimation>().animation.getSprite();
        else
            sprite = sf::Sprite();
        sprite.setOrigin(0.f, 0.f);
        sprite.setScale(1.f, 1.f);
        sprite.setPosition(transform.pos.x, transform.pos.y);
        m_game.window().draw(sprite);
    }

    // Ripristina la vista predefinita prima di renderizzare ImGui
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
    
    // Verifica se esiste già una tile in quella cella
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
    if (m_game.assets().hasAnimation(m_selectedTile)) {
        tile->add<CAnimation>(m_game.assets().getAnimation(m_selectedTile), true);
        std::cout << "[DEBUG] Tile posizionata: " << m_selectedTile << " in (" << gridX << ", " << gridY << ")\n";
    } else {
        std::cerr << "[ERROR] Animazione mancante per: " << m_selectedTile << "\n";
    }
}

void Scene_LevelEditor::placeDec(int gridX, int gridY) {
    float realX = gridX * tileSize;
    float realY = gridY * tileSize;
    
    // Verifica se esiste già una decorazione in quella cella
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
    if (m_game.assets().hasAnimation(m_selectedDec)) {
        dec->add<CAnimation>(m_game.assets().getAnimation(m_selectedDec), true);
        std::cout << "[DEBUG] Decorazione posizionata: " << m_selectedDec << " in (" << gridX << ", " << gridY << ")\n";
    } else {
        std::cerr << "[ERROR] Animazione mancante per: " << m_selectedDec << "\n";
    }
}

void Scene_LevelEditor::placeEnemy(int gridX, int gridY) {
    float realX = gridX * tileSize;
    float realY = gridY * tileSize;
    
    // Verifica se esiste già un enemy in quella cella (opzionale)
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
    if (m_game.assets().hasAnimation(m_selectedEnemy + "_Stand")) {
        enemy->add<CAnimation>(m_game.assets().getAnimation(m_selectedEnemy + "_Stand"), true);
    } else {
        std::cerr << "[ERROR] Animazione mancante per: " << m_selectedEnemy << "\n";
    }
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
    ImGui::SameLine();
    ImGui::RadioButton("Player", &m_mode, 3);

    if (m_mode == 0) {
        ImGui::Text("Seleziona una tile:");
        if (ImGui::TreeNode("Ancient Rome Tiles")) {
            for (const auto &tile : m_ancientRomanTileOptions) {
                bool isSelected = (m_selectedTile == tile);
                if (ImGui::Selectable(tile.c_str(), isSelected)) {
                    m_selectedTile = tile;
                    std::cout << "[DEBUG] Tile selezionata: " << m_selectedTile << "\n";
                }
            }
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Alien Rome Tiles")) {
            for (const auto &tile : m_alienTileOptions) {
                bool isSelected = (m_selectedTile == tile);
                if (ImGui::Selectable(tile.c_str(), isSelected)) {
                    m_selectedTile = tile;
                    std::cout << "[DEBUG] Tile selezionata: " << m_selectedTile << "\n";
                }
            }
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Quantum Rome Tiles")) {
            for (const auto &tile : m_quantumTileOptions) {
                bool isSelected = (m_selectedTile == tile);
                if (ImGui::Selectable(tile.c_str(), isSelected)) {
                    m_selectedTile = tile;
                    std::cout << "[DEBUG] Tile selezionata: " << m_selectedTile << "\n";
                }
            }
            ImGui::TreePop();
        }
    }
    else if (m_mode == 1) {
        ImGui::Text("Seleziona una decorazione:");
        if (ImGui::TreeNode("Ancient Rome Decs")) {
            for (const auto &dec : m_ancientRomanDecOptions) {
                bool isSelected = (m_selectedDec == dec);
                if (ImGui::Selectable(dec.c_str(), isSelected)) {
                    m_selectedDec = dec;
                    std::cout << "[DEBUG] Decorazione selezionata: " << m_selectedDec << "\n";
                }
            }
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Alien Rome Decs")) {
            for (const auto &dec : m_alienDecOptions) {
                bool isSelected = (m_selectedDec == dec);
                if (ImGui::Selectable(dec.c_str(), isSelected)) {
                    m_selectedDec = dec;
                    std::cout << "[DEBUG] Decorazione selezionata: " << m_selectedDec << "\n";
                }
            }
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Quantum Rome Decs")) {
            for (const auto &dec : m_quantumDecOptions) {
                bool isSelected = (m_selectedDec == dec);
                if (ImGui::Selectable(dec.c_str(), isSelected)) {
                    m_selectedDec = dec;
                    std::cout << "[DEBUG] Decorazione selezionata: " << m_selectedDec << "\n";
                }
            }
            ImGui::TreePop();
        }
    }
    else if (m_mode == 2) {
        ImGui::Text("Seleziona il tipo di enemy:");
        if (ImGui::TreeNode("Ancient Rome Enemies")) {
            for (const auto &enemy : m_ancientRomanEnemyOptions) {
                bool isSelected = (m_selectedEnemy == enemy);
                if (ImGui::Selectable(enemy.c_str(), isSelected)) {
                    m_selectedEnemy = enemy;
                    std::cout << "[DEBUG] Enemy selezionato: " << m_selectedEnemy << "\n";
                }
            }
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Alien Rome Enemies")) {
            for (const auto &enemy : m_alienEnemyOptions) {
                bool isSelected = (m_selectedEnemy == enemy);
                if (ImGui::Selectable(enemy.c_str(), isSelected)) {
                    m_selectedEnemy = enemy;
                    std::cout << "[DEBUG] Enemy selezionato: " << m_selectedEnemy << "\n";
                }
            }
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Quantum Rome Enemies")) {
            for (const auto &enemy : m_quantumEnemyOptions) {
                bool isSelected = (m_selectedEnemy == enemy);
                if (ImGui::Selectable(enemy.c_str(), isSelected)) {
                    m_selectedEnemy = enemy;
                    std::cout << "[DEBUG] Enemy selezionato: " << m_selectedEnemy << "\n";
                }
            }
            ImGui::TreePop();
        }
    }
    
    // Dropdown per la selezione del file livello
    static std::vector<std::string> levelFiles;
    static std::vector<const char*> levelFileNames;
    static int selectedLevelIndex = 0;
    if (levelFiles.empty()) {
        for (const auto& entry : fs::directory_iterator("src/levels/")) {
            if (entry.is_regular_file() && entry.path().extension() == ".txt") {
                levelFiles.push_back(entry.path().filename().string());
            }
        }
        for (auto& file : levelFiles) {
            levelFileNames.push_back(file.c_str());
        }
    }
    ImGui::Text("Seleziona un file livello:");
    if (ImGui::Combo("##levelSelect", &selectedLevelIndex, levelFileNames.data(), static_cast<int>(levelFileNames.size()))) {
        std::cout << "[DEBUG] File livello selezionato: " << levelFiles[selectedLevelIndex] << "\n";
    }
    
    ImGui::SliderFloat("Zoom", &m_zoom, MIN_ZOOM, MAX_ZOOM);
    
    if (ImGui::Button("Carica")) {
        std::string filePath = "src/levels/" + levelFiles[selectedLevelIndex];
        loadLevel(filePath);
    }
    ImGui::SameLine();
    if (ImGui::Button("Salva")) {
        std::string filePath = m_currentLevelPath.empty() ? "src/levels/level_save.txt" : m_currentLevelPath;
        saveLevel(filePath);
    }
    
    ImGui::End();
}

void Scene_LevelEditor::saveLevel(const std::string& filePath) {
    m_entityManager.update();
    std::ofstream out(filePath);
    if (!out.is_open()) {
        std::cerr << "[ERROR] Cannot open file for saving: " << filePath << "\n";
        return;
    }
    
    // Salva le tile
    for (auto& tile : m_entityManager.getEntities("tile")) {
        auto& transform = tile->get<CTransform>();
        int gridX = static_cast<int>(transform.pos.x / tileSize);
        int gridY = static_cast<int>(transform.pos.y / tileSize);
        int savedGridY = worldHeight - 1 - gridY;
        out << "Tile " << tile->get<CAnimation>().animation.getName() << " " << gridX << " " << savedGridY << "\n";
    }
    
    // Salva le decorazioni
    for (auto& dec : m_entityManager.getEntities("decoration")) {
        auto& transform = dec->get<CTransform>();
        int gridX = static_cast<int>(transform.pos.x / tileSize);
        int gridY = static_cast<int>(transform.pos.y / tileSize);
        int savedGridY = worldHeight - 1 - gridY;
        out << "Dec " << dec->get<CAnimation>().animation.getName() << " " << gridX << " " << savedGridY << "\n";
    }

    // Salva gli enemy (con patrol points: X originale e X+2)
    for (auto& enemy : m_entityManager.getEntities("enemy")) {
        auto& transform = enemy->get<CTransform>();
        int gridX = static_cast<int>(transform.pos.x / tileSize);
        int gridY = static_cast<int>(transform.pos.y / tileSize);
        int savedGridY = worldHeight - 1 - gridY;
        
        std::string enemyType = enemy->get<CEnemyAI>().enemyType == EnemyType::Fast ? "EnemyFast" :
                                enemy->get<CEnemyAI>().enemyType == EnemyType::Strong ? "EnemyStrong" :
                                enemy->get<CEnemyAI>().enemyType == EnemyType::Elite ? "EnemyElite" :
                                enemy->get<CEnemyAI>().enemyType == EnemyType::Emperor ? "Emperor" :
                                "EnemyNormal";
        
        out << "Enemy " << enemyType << " " << gridX << " " << savedGridY << " " 
            << gridX << " " << savedGridY << " " << (gridX + 2) << " " << savedGridY << "\n";
    }
    out.close();
    std::cout << "[DEBUG] Level saved to " << filePath << "\n";
}

void Scene_LevelEditor::loadLevel(const std::string& filePath) {
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
            if (m_game.assets().hasAnimation(assetType)) {
                entity->add<CAnimation>(m_game.assets().getAnimation(assetType), true);
            } else {
                std::cerr << "[ERROR] Missing animation for " << assetType << "\n";
            }
            std::cout << "[DEBUG] Loaded Tile: " << assetType << " at (" << gridX << ", " << gridY << ")\n";
        }
        else if (token == "Dec") {
            std::string assetType;
            int fileGridX, fileGridY;
            file >> assetType >> fileGridX >> fileGridY;
            int gridX = fileGridX;
            int gridY = worldHeight - 1 - fileGridY;
            auto entity = m_entityManager.addEntity("decoration");
            entity->add<CTransform>(Vec2<float>(gridX * tileSize, gridY * tileSize));
            if (m_game.assets().hasAnimation(assetType)) {
                entity->add<CAnimation>(m_game.assets().getAnimation(assetType), true);
            } else {
                std::cerr << "[ERROR] Missing animation for " << assetType << "\n";
            }
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
            std::string standAnim = enemyType + "_Stand";
            if (m_game.assets().hasAnimation(standAnim)) {
                entity->add<CAnimation>(m_game.assets().getAnimation(standAnim), true);
            } else {
                std::cerr << "[ERROR] Missing animation for enemy type: " << enemyType << "\n";
            }
            std::cout << "[DEBUG] Loaded Enemy: " << enemyType << " at (" << gridX << ", " << gridY << ")\n";
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

void Scene_LevelEditor::loadTileOptions() {
    // Definizione delle tile per le tre categorie
    m_ancientRomanTileOptions = { "Ground", "Brick", "Box1", "Box2", "PipeTall", "Pipe", "PipeBroken", "TreasureBoxAnim", "GoldGround", "LevelDoor", "BlackHoleRed", "BlackHoleBlue", "BlackHoleRedBig", "BlackHoleBlueBig" };
    m_alienTileOptions       = { "AlienTile1", "AlienTile2", "AlienTile3" };
    m_quantumTileOptions     = { "QuantumTile1", "QuantumTile2", "QuantumTile3" };

    if (!m_ancientRomanTileOptions.empty())
        m_selectedTile = m_ancientRomanTileOptions[0];
}

void Scene_LevelEditor::loadDecOptions() {
    // Definizione delle decorazioni per le tre categorie
    m_ancientRomanDecOptions = { "BushSmall", "BushTall", "CloudSmall", "CloudBig", "GoldPipeTall" };
    m_alienDecOptions        = { "AlienDec1", "AlienDec2" };
    m_quantumDecOptions      = { "QuantumDec1", "QuantumDec2" };

    if (!m_ancientRomanDecOptions.empty())
        m_selectedDec = m_ancientRomanDecOptions[0];
}

void Scene_LevelEditor::loadEnemyOptions() {
    // Definizione degli enemy per le tre categorie
    m_ancientRomanEnemyOptions = { "EnemyFast", "EnemyStrong", "EnemyElite", "EnemyNormal", "Emperor" };
    m_alienEnemyOptions        = { "AlienEnemy1", "AlienEnemy2" };
    m_quantumEnemyOptions      = { "QuantumEnemy1", "QuantumEnemy2" };

    if (!m_ancientRomanEnemyOptions.empty())
        m_selectedEnemy = m_ancientRomanEnemyOptions[0];
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
