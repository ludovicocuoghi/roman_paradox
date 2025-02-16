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
    else
        return EnemyType::Normal;
}

// Dimensione della cella/griglia
constexpr int tileSize = 96;
constexpr int worldWidth = 200;
constexpr int worldHeight = 40;
constexpr float CAMERA_SPEED = 800.f; // Adjust camera speed as needed

Scene_LevelEditor::Scene_LevelEditor(GameEngine& game)
    : Scene(game), m_mode(0), m_zoom(1.0f)
{
    // Inizializza l'EntityManager per questa scena (solo una volta)
    m_entityManager = EntityManager();

    // Inizializza ImGui per questa scena (una sola volta)
    bool imguiInitOK = ImGui::SFML::Init(m_game.window());
    if (!imguiInitOK) {
        std::cerr << "[ERROR] ImGui::SFML::Init() failed!\n";
    }

    // Imposta la vista della camera con il default
    m_cameraView = m_game.window().getDefaultView();
    m_game.window().setView(m_cameraView);

    // Registra azioni (ad es. Escape per tornare)
    registerAction(sf::Keyboard::Escape, "BACK");

    loadTileOptions();
    loadEnemyOptions(); // Nuova funzione per caricare le opzioni degli enemy
}

Scene_LevelEditor::~Scene_LevelEditor() {
    ImGui::SFML::Shutdown();
}

void Scene_LevelEditor::update(float deltaTime) {
    ImGui::SFML::Update(m_game.window(), sf::seconds(deltaTime));
    
    // --- CAMERA MOVEMENT VIA ARROW KEYS ---
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

    // Gestione click sinistro: in base alla modalità (0=Tile, 1=Decorazione, 2=Enemy)
    if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
        if (!mousePressed) {
            sf::Vector2i mousePosScreen = sf::Mouse::getPosition(m_game.window());
            sf::Vector2f mousePosWorld = m_game.window().mapPixelToCoords(mousePosScreen, m_cameraView);

            bool overImGui = ImGui::IsAnyItemHovered() || ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);
            if (!overImGui) {
                int cellX = static_cast<int>(mousePosWorld.x / tileSize);
                int cellY = static_cast<int>(mousePosWorld.y / tileSize);
                if (m_mode == 0)
                    placeTile(cellX, cellY);
                else if (m_mode == 1)
                    placeDec(cellX, cellY);
                else if (m_mode == 2)
                    placeEnemy(cellX, cellY);
            }
            mousePressed = true;
        }
    } else if (sf::Mouse::isButtonPressed(sf::Mouse::Right)) {
        if (!mousePressed) {
            sf::Vector2i mousePosScreen = sf::Mouse::getPosition(m_game.window());
            sf::Vector2f mousePosWorld = m_game.window().mapPixelToCoords(mousePosScreen, m_cameraView);
            int cellX = static_cast<int>(mousePosWorld.x / tileSize);
            int cellY = static_cast<int>(mousePosWorld.y / tileSize);
            if (m_mode == 0)
                removeTile(cellX, cellY);
            else if (m_mode == 1)
                removeDec(cellX, cellY);
            // Per gli enemy, potresti voler rimuovere l'enemy in quella cella
            else if (m_mode == 2)
                removeEnemy(cellX, cellY);
            mousePressed = true;
        }
    } else {
        mousePressed = false;
    }

    printEntities();
}

void Scene_LevelEditor::sRender() {
    m_game.window().clear(sf::Color(100, 100, 255));
    m_game.window().setView(m_cameraView);
    drawGrid();

    // Disegna tile, decorazioni e, eventualmente, enemy (puoi gestirlo separatamente se hai un sistema di rendering per gli enemy)
    for (auto& entity : m_entityManager.getEntities()) {
        if (!entity->has<CTransform>())
            continue;
        auto& transform = entity->get<CTransform>();
        sf::Sprite sprite;
        if (entity->has<CAnimation>()) {
            sprite = entity->get<CAnimation>().animation.getSprite();
        } else {
            sprite = sf::Sprite();
        }
        sprite.setOrigin(0.f, 0.f);
        sprite.setScale(1.f, 1.f);
        sprite.setPosition(transform.pos.x, transform.pos.y);
        m_game.window().draw(sprite);
    }

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
    
    // Controlla se esiste già una tile in quella cella
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
    
    // Controlla se esiste già una decorazione in quella cella
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
    
    // Controlla se esiste già un enemy in quella cella (opzionale)
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
    // Imposta l'AI dell'enemy in base al tipo selezionato.
    // Il tipo selezionato è memorizzato in m_selectedEnemy (ad es. "EnemyFast", "EnemyStrong", "EnemyElite", "EnemyNormal")
    if (m_game.assets().hasAnimation(m_selectedEnemy + "_Stand")) {
        enemy->add<CAnimation>(m_game.assets().getAnimation(m_selectedEnemy + "_Stand"), true);
    } else {
        std::cerr << "[ERROR] Animazione mancante per: " << m_selectedEnemy << "\n";
    }
    // Aggiungi il componente AI, impostando il tipo corrispondente
    // Nota: la conversione da stringa a EnemyType va implementata; qui ipotizziamo una funzione helper getEnemyType(m_selectedEnemy)
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
    
    // Disegna linee della griglia
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

    // Modalità: 0 = Tile, 1 = Decoration, 2 = Enemy
    ImGui::Text("Modalità:");
    ImGui::RadioButton("Tile", &m_mode, 0);
    ImGui::SameLine();
    ImGui::RadioButton("Decoration", &m_mode, 1);
    ImGui::SameLine();
    ImGui::RadioButton("Enemy", &m_mode, 2);

    if (m_mode == 0) {
        ImGui::Text("Seleziona una tile:");
        static int selectedTileIndex = 0;
        std::vector<const char*> tileNames;
        tileNames.reserve(m_tileOptions.size());
        for (auto& t : m_tileOptions)
            tileNames.push_back(t.c_str());
        if (ImGui::Combo("##tileSelect", &selectedTileIndex, tileNames.data(), static_cast<int>(tileNames.size()))) {
            m_selectedTile = m_tileOptions[selectedTileIndex];
            std::cout << "[DEBUG] Tile selezionata: " << m_selectedTile << "\n";
        }
    } else if (m_mode == 1) {
        ImGui::Text("Seleziona una decorazione:");
        static int selectedDecIndex = 0;
        std::vector<const char*> decNames;
        decNames.reserve(m_decOptions.size());
        for (auto& d : m_decOptions)
            decNames.push_back(d.c_str());
        if (ImGui::Combo("##decSelect", &selectedDecIndex, decNames.data(), static_cast<int>(decNames.size()))) {
            m_selectedDec = m_decOptions[selectedDecIndex];
            std::cout << "[DEBUG] Decoration selezionata: " << m_selectedDec << "\n";
        }
    } else if (m_mode == 2) {
        ImGui::Text("Seleziona il tipo di enemy:");
        static int selectedEnemyIndex = 0;
        std::vector<const char*> enemyNames;
        enemyNames.reserve(m_enemyOptions.size());
        for (auto& e : m_enemyOptions)
            enemyNames.push_back(e.c_str());
        if (ImGui::Combo("##enemySelect", &selectedEnemyIndex, enemyNames.data(), static_cast<int>(enemyNames.size()))) {
            m_selectedEnemy = m_enemyOptions[selectedEnemyIndex];
            std::cout << "[DEBUG] Enemy selezionato: " << m_selectedEnemy << "\n";
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
    
    // Save tiles
    for (auto& tile : m_entityManager.getEntities("tile")) {
        auto& transform = tile->get<CTransform>();
        int gridX = static_cast<int>(transform.pos.x / tileSize);
        int gridY = static_cast<int>(transform.pos.y / tileSize);
        int savedGridY = worldHeight - 1 - gridY;
        out << "Tile " << tile->get<CAnimation>().animation.getName() << " " << gridX << " " << savedGridY << "\n";
    }
    
    // Save decorations
    for (auto& dec : m_entityManager.getEntities("decoration")) {
        auto& transform = dec->get<CTransform>();
        int gridX = static_cast<int>(transform.pos.x / tileSize);
        int gridY = static_cast<int>(transform.pos.y / tileSize);
        int savedGridY = worldHeight - 1 - gridY;
        out << "Dec " << dec->get<CAnimation>().animation.getName() << " " << gridX << " " << savedGridY << "\n";
    }
    
    //Save Player
    out << "Player 2 2\n";

    // Save enemies with patrol points (adding +2 to X positions)
    for (auto& enemy : m_entityManager.getEntities("enemy")) {
        auto& transform = enemy->get<CTransform>();
        int gridX = static_cast<int>(transform.pos.x / tileSize);
        int gridY = static_cast<int>(transform.pos.y / tileSize);
        int savedGridY = worldHeight - 1 - gridY;
        
        // Determine enemy type
        std::string enemyType = enemy->get<CEnemyAI>().enemyType == EnemyType::Fast ? "EnemyFast" :
                                enemy->get<CEnemyAI>().enemyType == EnemyType::Strong ? "EnemyStrong" :
                                enemy->get<CEnemyAI>().enemyType == EnemyType::Elite ? "EnemyElite" :
                                "EnemyNormal";
        
        // Add patrol points (original X +2)
        out << "Enemy " << enemyType << " " << gridX << " " << savedGridY <<  " " << gridX << " " << savedGridY 
            << " " << (gridX + 2) << " " << savedGridY << "\n";
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
            // Converti da coordinate in basso a sinistra (file) a quelle del gioco
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
            std::cout << "[DEBUG] Loaded Decoration: " << assetType << " at (" << gridX << ", " << gridY << ")\n";
        }
        else if (token == "Enemy") {
            std::string enemyType;
            int fileGridX, fileGridY;
            file >> enemyType >> fileGridX >> fileGridY;
            int gridX = fileGridX;
            int gridY = worldHeight - 1 - fileGridY;
            auto entity = m_entityManager.addEntity("enemy");
            entity->add<CTransform>(Vec2<float>(gridX * tileSize, gridY * tileSize));
            // Usa la funzione helper per convertire la stringa in EnemyType
            entity->add<CEnemyAI>(getEnemyType(enemyType));
            // Aggiungi l'animazione di stand corrispondente
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
    m_tileOptions = { "Ground", "Brick", "Box1", "Box2", "PipeTall", "Pipe", "PipeBroken", "TreasureBoxAnim", "GoldGround" };
    if (!m_tileOptions.empty()) {
        m_selectedTile = m_tileOptions[0];
    }
    m_decOptions = { "BushSmall", "BushTall", "CloudSmall", "CloudBig","GoldPipeTall","GoldPipeTall" };
    if (!m_decOptions.empty()) {
        m_selectedDec = m_decOptions[0];
    }
    // Carica anche le opzioni degli enemy
    loadEnemyOptions();
}

void Scene_LevelEditor::loadEnemyOptions() {
    m_enemyOptions = { "EnemyFast", "EnemyStrong", "EnemyElite", "EnemyNormal" };
    if (!m_enemyOptions.empty()) {
        m_selectedEnemy = m_enemyOptions[0];
    }
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
