#include "Scene_LevelEditor.h"
#include "GameEngine.h"
#include <iostream>
#include <fstream>
#include <cmath> // per std::floor
#include <filesystem> // per std::filesystem (C++17)

namespace fs = std::filesystem;

// Dimensione della cella/griglia
constexpr int tileSize = 96;
constexpr int worldWidth = 200;
constexpr int worldHeight = 40;
constexpr float CAMERA_SPEED = 800.f; // Adjust camera speed as needed

Scene_LevelEditor::Scene_LevelEditor(GameEngine& game)
    : Scene(game), m_mode(0) // Inizialmente in modalità Tile (0)
{
    // Inizializza l'EntityManager per questa scena (solo una volta)
    m_entityManager = EntityManager();

    // Inizializza ImGui per questa scena (una sola volta)
    bool imguiInitOK = ImGui::SFML::Init(m_game.window());
    if (!imguiInitOK) {
        std::cerr << "[ERROR] ImGui::SFML::Init() failed!\n";
    }

    // Do not flip the view—keep the default coordinate system.
    m_cameraView.setSize(m_game.window().getDefaultView().getSize());
    m_game.window().setView(m_cameraView);

    // Registra azioni (ad es. Escape per tornare)
    registerAction(sf::Keyboard::Escape, "BACK");

    loadTileOptions();
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

    // Gestione del click sinistro: in base alla modalità, posiziona tile o decorazione
    if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
        if (!mousePressed) {
            std::cout << "[DEBUG] Mouse premuto (sinistro)\n";
            sf::Vector2i mousePosScreen = sf::Mouse::getPosition(m_game.window());
            sf::Vector2f mousePosWorld = m_game.window().mapPixelToCoords(mousePosScreen, m_cameraView);
            std::cout << "[DEBUG] Mouse (schermo): (" << mousePosScreen.x << ", " << mousePosScreen.y << ")\n";
            std::cout << "[DEBUG] Mouse (mondo): (" << mousePosWorld.x << ", " << mousePosWorld.y << ")\n";

            bool overImGui = ImGui::IsAnyItemHovered() || ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);
            std::cout << "[DEBUG] Mouse " << (overImGui ? "sopra ImGui" : "fuori da ImGui") << "\n";
            if (!overImGui) {
                std::cout << "[DEBUG] Click sinistro elaborato.\n";
                int cellX = static_cast<int>(mousePosWorld.x / tileSize);
                int cellY = static_cast<int>(mousePosWorld.y / tileSize);
                std::cout << "[DEBUG] Coordinate griglia: (" << cellX << ", " << cellY << ")\n";
                if (m_mode == 0)
                    placeTile(cellX, cellY);
                else
                    placeDec(cellX, cellY);
            } else {
                std::cout << "[DEBUG] Click sinistro ignorato perché sopra ImGui.\n";
            }
            mousePressed = true;
        }
    }
    // Gestione del click destro per rimuovere tile (o decorazioni)
    else if (sf::Mouse::isButtonPressed(sf::Mouse::Right)) {
        if (!mousePressed) {
            std::cout << "[DEBUG] Mouse premuto (destro)\n";
            sf::Vector2i mousePosScreen = sf::Mouse::getPosition(m_game.window());
            sf::Vector2f mousePosWorld = m_game.window().mapPixelToCoords(mousePosScreen, m_cameraView);
            int cellX = static_cast<int>(mousePosWorld.x / tileSize);
            int cellY = static_cast<int>(mousePosWorld.y / tileSize);
            std::cout << "[DEBUG] Rimozione elemento in cella: (" << cellX << ", " << cellY << ")\n";
            if (m_mode == 0)
                removeTile(cellX, cellY);
            else
                removeDec(cellX, cellY);
            mousePressed = true;
        }
    } else {
        mousePressed = false;
    }

    printEntities();
}

void Scene_LevelEditor::sRender() {
    static float elapsedTime = 0.0f;
    elapsedTime += m_game.getDeltaTime();
    if (elapsedTime >= 5.0f) {
        elapsedTime = 0.0f;
    }

    m_game.window().clear(sf::Color(100, 100, 255));
    m_game.window().setView(m_cameraView);
    drawGrid();

    // Disegna le tile
    for (auto& tile : m_entityManager.getEntities("tile")) {
        auto& transform = tile->get<CTransform>();
        if (tile->has<CAnimation>()) {
            auto& animation = tile->get<CAnimation>();
            sf::Sprite sprite = animation.animation.getSprite();
            // Keep the original orientation by not flipping the sprite.
            sprite.setOrigin(0.f, 0.f);
            sprite.setScale(1.f, 1.f);
            sprite.setPosition(transform.pos.x, transform.pos.y);
            m_game.window().draw(sprite);
        } else {
            sf::CircleShape debugShape(10.f);
            debugShape.setFillColor(sf::Color::Red);
            debugShape.setPosition(transform.pos.x, transform.pos.y);
            m_game.window().draw(debugShape);
        }
    }
    
    // Disegna le decorazioni
    for (auto& dec : m_entityManager.getEntities("decoration")) {
        auto& transform = dec->get<CTransform>();
        if (dec->has<CAnimation>()) {
            auto& animation = dec->get<CAnimation>();
            sf::Sprite sprite = animation.animation.getSprite();
            sprite.setOrigin(0.f, 0.f);
            sprite.setScale(1.f, 1.f);
            sprite.setPosition(transform.pos.x, transform.pos.y);
            m_game.window().draw(sprite);
        } else {
            sf::CircleShape debugShape(10.f);
            debugShape.setFillColor(sf::Color::Green);
            debugShape.setPosition(transform.pos.x, transform.pos.y);
            m_game.window().draw(debugShape);
        }
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

void Scene_LevelEditor::drawGrid() {
    sf::Vector2u windowSize = m_game.window().getSize();
    sf::Color gridColor(255, 255, 255, 100);
    
    // Draw grid lines
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
    
    // Comment out grid labels
    /*
    sf::View hudView(sf::FloatRect(0, 0, windowSize.x, windowSize.y));
    m_game.window().setView(hudView);
    const sf::Font& font = m_game.assets().getFont("Menu");
    const float offset = 4.f;
    for (int x = 0; x < worldWidth; ++x) {
        for (int y = 0; y < worldHeight; ++y) {
            sf::Text coordText;
            coordText.setFont(font);
            coordText.setCharacterSize(14);
            coordText.setFillColor(sf::Color::White);
            coordText.setString(std::to_string(x) + "," + std::to_string(y));
            coordText.setPosition(x * tileSize + offset, windowSize.y - ((y + 1) * tileSize) + offset);
            m_game.window().draw(coordText);
        }
    }
    m_game.window().setView(m_cameraView);
    */
}

void Scene_LevelEditor::renderImGui() {
    ImGui::Begin("Tile Selector");
    
    // Modalità: 0 = Tile, 1 = Decoration
    ImGui::Text("Modalità:");
    ImGui::RadioButton("Tile", &m_mode, 0);
    ImGui::SameLine();
    ImGui::RadioButton("Decoration", &m_mode, 1);
    
    if (m_mode == 0) {
        ImGui::Text("Seleziona una tile:");
        static int selectedTileIndex = 0;
        std::vector<const char*> tileNames;
        tileNames.reserve(m_tileOptions.size());
        for (auto& t : m_tileOptions) {
            tileNames.push_back(t.c_str());
        }
        if (ImGui::Combo("##tileSelect", &selectedTileIndex, tileNames.data(), static_cast<int>(tileNames.size()))) {
            m_selectedTile = m_tileOptions[selectedTileIndex];
            std::cout << "[DEBUG] Tile selezionata: " << m_selectedTile << "\n";
        }
    } else {
        ImGui::Text("Seleziona una decorazione:");
        static int selectedDecIndex = 0;
        std::vector<const char*> decNames;
        decNames.reserve(m_decOptions.size());
        for (auto& d : m_decOptions) {
            decNames.push_back(d.c_str());
        }
        if (ImGui::Combo("##decSelect", &selectedDecIndex, decNames.data(), static_cast<int>(decNames.size()))) {
            m_selectedDec = m_decOptions[selectedDecIndex];
            std::cout << "[DEBUG] Decoration selezionata: " << m_selectedDec << "\n";
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
    
    // Save tiles: convert the Y coordinate (from top-left to bottom-left) then adjust for pipe types
    for (auto& tile : m_entityManager.getEntities("tile")) {
        auto& transform = tile->get<CTransform>();
        int gridX = static_cast<int>(transform.pos.x / tileSize);
        int gridY = static_cast<int>(transform.pos.y / tileSize);
        int savedGridY = worldHeight - 1 - gridY; // default conversion from top-left to bottom-left
        
        std::string assetType = "unknown";
        if (tile->has<CAnimation>()) {
            assetType = tile->get<CAnimation>().animation.getName();
        }
        // If the tile is a pipe type, subtract 3 tiles from Y
        if (assetType == "PipeTall" || assetType == "PipeShort") {
            //savedGridY = std::max(0, savedGridY - 3);
        }
        
        out << "Tile " << assetType << " " << gridX << " " << savedGridY << "\n";
    }
    
    // Save decorations: convert the Y coordinate then adjust for BushTall
    for (auto& dec : m_entityManager.getEntities("decoration")) {
        auto& transform = dec->get<CTransform>();
        int gridX = static_cast<int>(transform.pos.x / tileSize);
        int gridY = static_cast<int>(transform.pos.y / tileSize);
        int savedGridY = worldHeight - 1 - gridY; // default conversion
        
        std::string assetType = "unknown";
        if (dec->has<CAnimation>()) {
            assetType = dec->get<CAnimation>().animation.getName();
        }
        // If the decoration is a BushTall, subtract 2 tiles from Y
        if (assetType == "BushTall") {
            float savedGridY = static_cast<float>(worldHeight - 1 - gridY);
            savedGridY = std::max(0.f, savedGridY - 1.5f);
            savedGridY = static_cast<int>(std::round(savedGridY));
        }
        
        out << "Dec " << assetType << " " << gridX << " " << savedGridY << "\n";
    }
    
    out << "Player 5 5 48 48 5 -20 20 0.75 Buster\n";
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
        if (token == "Tile" || token == "Dec") {
            std::string assetType;
            int fileGridX, fileGridY;
            file >> assetType >> fileGridX >> fileGridY;
            // Convert file Y (bottom-left origin) to rendered Y (top-left origin)
            int gridY = worldHeight - 1 - fileGridY;
            int gridX = fileGridX;
            std::string tag = (token == "Tile") ? "tile" : "decoration";
            auto entity = m_entityManager.addEntity(tag);
            entity->add<CTransform>(Vec2<float>(gridX * tileSize, gridY * tileSize));
            if (m_game.assets().hasAnimation(assetType)) {
                entity->add<CAnimation>(m_game.assets().getAnimation(assetType), true);
            } else {
                std::cerr << "[ERROR] Missing animation for " << assetType << "\n";
            }
            std::cout << "[DEBUG] Loaded " << token << ": " << assetType << " at (" << gridX << ", " << gridY << ")\n";
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
    m_tileOptions = { "Ground", "Brick", "Box1", "Box2","PipeTall", "PipeShort", "TreasureBoxAnim" };
    if (!m_tileOptions.empty()) {
        m_selectedTile = m_tileOptions[0];
    }
    m_decOptions = { "BushSmall", "BushTall", "CloudSmall", "CloudBig" };
    if (!m_decOptions.empty()) {
        m_selectedDec = m_decOptions[0];
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
}

