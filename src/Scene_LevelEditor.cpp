#include "Scene_LevelEditor.h"
#include "GameEngine.h"
#include <iostream>
#include <fstream>
#include <cmath>      
#include <filesystem> 
#include "ResourcePath.h" 

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
    else if (typeStr == "EnemySuper2")
        return EnemyType::Super2;
    else if (typeStr == "Emperor")
        return EnemyType::Emperor;
    else if (typeStr == "EnemyCitizen")
        return EnemyType::Citizen;
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

    loadBackground("images/Background/ancient_rome/ancient_rome_level_1_day.png");
    
    // Position camera at bottom left after loading background
    setCameraToBottomLeft();

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

    std::string levelPath = getResourcePath("levels/");
    
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

        updateBackgroundPosition();

        m_entityManager.update();

        // Left mouse click handling
        if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
            if (!mousePressed) {
                sf::Vector2i mousePosScreen = sf::Mouse::getPosition(m_game.window());
                sf::Vector2f mousePosWorld = m_game.window().mapPixelToCoords(mousePosScreen, m_cameraView);
                int cellX = static_cast<int>(mousePosWorld.x / tileSize);
                int cellY = static_cast<int>(mousePosWorld.y / tileSize);
                std::cout << "[DEBUG] Left click on (" << cellX << ", " << cellY << ")\n";
                if (m_mode == 0)
                    placeTile(cellX, cellY);
                else if (m_mode == 1)
                    placeDec(cellX, cellY);
                else if (m_mode == 2)
                    placeEnemy(cellX, cellY);
                else if (m_mode == 3)
                    placePlayer(cellX, cellY);
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
                std::cout << "[DEBUG] Right click on (" << cellX << ", " << cellY << ")\n";
                if (m_mode == 0)
                    removeTile(cellX, cellY);
                else if (m_mode == 1)
                    removeDec(cellX, cellY);
                else if (m_mode == 2)
                    removeEnemy(cellX, cellY);
                else if (m_mode== 3)
                    removePlayer();
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

    // Draw background if it exists
    if (m_backgroundTexture.getSize().x > 0) {
        m_game.window().draw(m_backgroundSprite);
    }
    
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

void Scene_LevelEditor::loadBackground(const std::string& path) {
    std::string fullPath = getResourcePath(path);
    if (!m_backgroundTexture.loadFromFile(fullPath)) {
        std::cerr << "[ERROR] Could not load background image: " << fullPath << std::endl;
        return;
    }
    
    m_backgroundTexture.setRepeated(true);
    m_backgroundSprite.setTexture(m_backgroundTexture);
    
    // Set initial background scale and position
    updateBackgroundPosition();
    
    std::cout << "[DEBUG] Background loaded: " << path << std::endl;
}

void Scene_LevelEditor::setCameraToBottomLeft() {
    // Calculate the position for the bottom left corner of the world
    float bottomLeftX = (tileSize * 2); // Small offset to show a bit of space
    float bottomLeftY = (worldHeight * tileSize) - (m_cameraView.getSize().y / 2);
    
    // Set the camera position
    m_cameraView.setCenter(bottomLeftX, bottomLeftY);
    m_game.window().setView(m_cameraView);
    
    // Update the background to match the new camera position
    updateBackgroundPosition();
    
    std::cout << "[DEBUG] Camera positioned at bottom left\n";
}

void Scene_LevelEditor::updateBackgroundPosition() {
    if (m_backgroundTexture.getSize().x == 0 || m_backgroundTexture.getSize().y == 0) {
        return; // No valid texture loaded
    }
    
    // Get the current view bounds
    sf::Vector2f viewCenter = m_cameraView.getCenter();
    sf::Vector2f viewSize = m_cameraView.getSize();
    
    // Position the background so it's centered on the view
    float bgX = viewCenter.x - (viewSize.x / 2);
    float bgY = viewCenter.y - (viewSize.y / 2);
    
    // Set the position and scale of the background sprite
    m_backgroundSprite.setPosition(bgX, bgY);
    
    // Calculate scale to cover the view area
    float scaleX = viewSize.x / static_cast<float>(m_backgroundTexture.getSize().x);
    float scaleY = viewSize.y / static_cast<float>(m_backgroundTexture.getSize().y);
    
    // Use the larger scale to ensure the background covers the entire view
    float scale = std::max(scaleX, scaleY);
    m_backgroundSprite.setScale(scale, scale);
}


void Scene_LevelEditor::placePlayer(int gridX, int gridY) {
    float realX = gridX * tileSize;
    float realY = gridY * tileSize;
    
    // First, check if a player already exists and remove it if it does
    auto& players = m_entityManager.getEntities("player");
    for (auto& player : players) {
        player->destroy();
        std::cout << "[DEBUG] Existing player removed\n";
    }
    
    // Create new player at the specified position
    auto player = m_entityManager.addEntity("player");
    player->add<CTransform>(Vec2<float>(realX, realY));
    
// Add the player animation
    std::string fullName = "PlayerStand";
    if (m_game.assets().hasAnimation(fullName)) {
        player->add<CAnimation>(m_game.assets().getAnimation(fullName), true);
        std::cout << "[DEBUG] Player positioned at (" << gridX << ", " << gridY << ")\n";
    }
    else {
        std::cerr << "[ERROR] Missing animation for: " << fullName << "\n";
        // Fallback to a default player animation if the world-specific one doesn't exist
        if (m_game.assets().hasAnimation("PlayerStand")) {
            player->add<CAnimation>(m_game.assets().getAnimation("PlayerStand"), true);
            std::cout << "[DEBUG] Player positioned with fallback animation at (" << gridX << ", " << gridY << ")\n";
        }
    }
}

void Scene_LevelEditor::removePlayer() {
    auto& players = m_entityManager.getEntities("player");
    if (!players.empty()) {
        for (auto& player : players) {
            player->destroy();
        }
        std::cout << "[DEBUG] Player removed\n";
    } else {
        std::cout << "[DEBUG] No player found to remove.\n";
    }
}

void Scene_LevelEditor::placeTile(int gridX, int gridY) {
    float realX = gridX * tileSize;
    float realY = gridY * tileSize;
    for (auto& tile : m_entityManager.getEntities("tile")) {
        auto& transform = tile->get<CTransform>();
        if (std::abs(transform.pos.x - realX) < 0.1f &&
            std::abs(transform.pos.y - realY) < 0.1f) {
            std::cout << "[DEBUG] Tile already present at (" << gridX << ", " << gridY << "), skipping.\n";
            return;
        }
    }
    auto tile = m_entityManager.addEntity("tile");
    tile->add<CTransform>(Vec2<float>(realX, realY));
    std::string fullName = worldcategory + m_selectedTile;
    if (m_game.assets().hasAnimation(fullName)) {
        tile->add<CAnimation>(m_game.assets().getAnimation(fullName), true);
        std::cout << "[DEBUG] Tile placed: " << fullName << " at (" << gridX << ", " << gridY << ")\n";
    }
    else
        std::cerr << "[ERROR] Missing animation for: " << fullName << "\n";
}

void Scene_LevelEditor::placeDec(int gridX, int gridY) {
    float realX = gridX * tileSize;
    float realY = gridY * tileSize;
    for (auto& dec : m_entityManager.getEntities("decoration")) {
        auto& transform = dec->get<CTransform>();
        if (std::abs(transform.pos.x - realX) < 0.1f &&
            std::abs(transform.pos.y - realY) < 0.1f) {
            std::cout << "[DEBUG] Decoration already present at (" << gridX << ", " << gridY << "), skipping.\n";
            return;
        }
    }
    auto dec = m_entityManager.addEntity("decoration");
    dec->add<CTransform>(Vec2<float>(realX, realY));
    std::string fullName = worldcategory + m_selectedDec;
    if (m_game.assets().hasAnimation(fullName)) {
        dec->add<CAnimation>(m_game.assets().getAnimation(fullName), true);
        std::cout << "[DEBUG] Decoration placed: " << fullName << " at (" << gridX << ", " << gridY << ")\n";
    }
    else
        std::cerr << "[ERROR] Missing animation for: " << fullName << "\n";
}

void Scene_LevelEditor::placeEnemy(int gridX, int gridY) {
    float realX = gridX * tileSize;
    float realY = gridY * tileSize;
    for (auto& enemy : m_entityManager.getEntities("enemy")) {
        auto& transform = enemy->get<CTransform>();
        if (std::abs(transform.pos.x - realX) < 0.1f &&
            std::abs(transform.pos.y - realY) < 0.1f) {
            std::cout << "[DEBUG] Enemy already present at (" << gridX << ", " << gridY << "), skipping.\n";
            return;
        }
    }
    auto enemy = m_entityManager.addEntity("enemy");
    enemy->add<CTransform>(Vec2<float>(realX, realY));
    std::string fullName = worldcategory + "Stand" + m_selectedEnemy;
    if (m_game.assets().hasAnimation(fullName))
        enemy->add<CAnimation>(m_game.assets().getAnimation(fullName), true);
    else
        std::cerr << "[ERROR] Missing animation for: " << fullName << "\n";
    enemy->add<CEnemyAI>(getEnemyType(m_selectedEnemy));
    std::cout << "[DEBUG] Enemy placed: " << m_selectedEnemy << " at (" << gridX << ", " << gridY << ")\n";
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
            std::cout << "[DEBUG] Tile removed at (" << gridX << ", " << gridY << ")\n";
            return;
        }
    }
    std::cout << "[DEBUG] No tile found at (" << gridX << ", " << gridY << ") to be removed.\n";
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

    ImGui::Text("Mode:");
    ImGui::RadioButton("Tile", &m_mode, 0);
    ImGui::SameLine();
    ImGui::RadioButton("Decoration", &m_mode, 1);
    ImGui::SameLine();
    ImGui::RadioButton("Enemy", &m_mode, 2);
    ImGui::SameLine();
    ImGui::RadioButton("Player", &m_mode, 3);

    // Zoom slider
    ImGui::Text("Zoom:");
    ImGui::SliderFloat("##zoom", &m_zoom, MIN_ZOOM, MAX_ZOOM);

    // Display available assets based on mode
    if (m_mode == 0) {
        ImGui::Text("Select a Tile:");
        for (const auto& tile : m_tileOptions) {
            if (ImGui::Selectable(tile.c_str(), m_selectedTile == tile)) {
                m_selectedTile = tile;
                std::cout << "[DEBUG] Selected Tile: " << m_selectedTile << "\n";
            }
        }
    }
    else if (m_mode == 1) {
        ImGui::Text("Select a Decoration:");
        for (const auto& dec : m_decOptions) {
            if (ImGui::Selectable(dec.c_str(), m_selectedDec == dec)) {
                m_selectedDec = dec;
                std::cout << "[DEBUG] Selected Decoration: " << m_selectedDec << "\n";
            }
        }
    }
    else if (m_mode == 2) {
        ImGui::Text("Select an Enemy:");
        for (const auto& enemy : m_enemyOptions) {
            if (ImGui::Selectable(enemy.c_str(), m_selectedEnemy == enemy)) {
                m_selectedEnemy = enemy;
                std::cout << "[DEBUG] Selected Enemy: " << m_selectedEnemy << "\n";
            }
        }
    }
    else if (m_mode == 3) {
        // Player mode has no selection, just instruction
        ImGui::Text("Click to place player (only one player allowed)");
    }

    // Level file selection dropdown and load/save buttons
    ImGui::Text("Select a level file:");
    if (!levelFiles.empty()) {
        if (ImGui::Combo("##levelSelect", &selectedLevelIndex, levelFileNames.data(), static_cast<int>(levelFileNames.size()))) {
            std::cout << "[DEBUG] level file selected: " << levelFiles[selectedLevelIndex] << "\n";
        }
    } else {
        ImGui::Text("[ERROR] No level files found.");
    }

    if (ImGui::Button("Load")) {
        if (!levelFiles.empty()) {
            std::string filePath = getResourcePath("levels/" + levelFiles[selectedLevelIndex]);
            loadLevel(filePath);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Save")) {
        std::string filePath = m_currentLevelPath.empty() ? getResourcePath("levels/level_save.txt") : m_currentLevelPath;
        saveLevel(filePath);
    }

    ImGui::End();
}

void Scene_LevelEditor::loadTileOptions() {
    m_tileOptions = { "Ground", "Brick", "Box1", "Box2", "PipeTall", "Pipe", "PipeBroken", "Treasure",  "LevelDoor", "BlackHoleRedBig","GoldGround",  "Armor" };
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
    m_enemyOptions = { "EnemyFast", "EnemyStrong", "EnemyElite", "EnemyNormal", "EnemySuper", "EnemySuper2", "Emperor", "EnemyCitizen" };
    if (!m_enemyOptions.empty()) {
        m_selectedEnemy = m_enemyOptions[0];
    }
}

void Scene_LevelEditor::saveLevel(const std::string& filePath) {
    m_entityManager.update();
    
    // Extract just the filename from the path
    std::string filename = fs::path(filePath).filename().string();
    
    // Define bin level path (where the game will look for levels at runtime)
    std::string binLevelPath = getResourcePath("levels/") + filename;
    
    // Based on the screenshot, construct the source path
    std::string srcLevelPath = "";
    
    // Current bin path is something like:
    // /Users/ludovicocuoghi/Documents/cpp_course/comp4300_course/final_project/bin/levels/...
    
    // We need to replace "bin/levels" with "src/levels"
    std::string binPath = getResourcePath("levels/");
    std::cout << "[DEBUG] Bin levels path: " << binPath << "\n";
    
    // Try to find "bin" in the path and replace it with "src"
    size_t binPos = binPath.find("/bin/");
    if (binPos != std::string::npos) {
        srcLevelPath = binPath.substr(0, binPos) + "/src/levels/" + filename;
        std::cout << "[DEBUG] Determined src path: " << srcLevelPath << "\n";
    } else {
        // Alternative: Parse the full project path from the current binPath
        std::string projectPattern = "Documents/cpp_course/comp4300_course/final_project/";
        size_t projPos = binPath.find(projectPattern);
        if (projPos != std::string::npos) {
            std::string basePath = binPath.substr(0, projPos + projectPattern.length());
            srcLevelPath = basePath + "src/levels/" + filename;
            std::cout << "[DEBUG] Using project pattern to determine src path: " << srcLevelPath << "\n";
        }
    }
    
    // Fallback: Try a hardcoded path based on the screenshot if all else fails
    if (srcLevelPath.empty()) {
        // Determine home directory
        const char* homeDir = getenv("HOME");
        if (homeDir) {
            srcLevelPath = std::string(homeDir) + 
                           "/Documents/cpp_course/comp4300_course/final_project/src/levels/" + 
                           filename;
            std::cout << "[DEBUG] Using home directory fallback for src path: " << srcLevelPath << "\n";
        } else {
            // Last resort hardcoded path
            srcLevelPath = "/Users/ludovicocuoghi/Documents/cpp_course/comp4300_course/final_project/src/levels/" + filename;
            std::cout << "[DEBUG] Using hardcoded fallback path: " << srcLevelPath << "\n";
        }
    }
    
    // Ensure both directories exist
    try {
        std::string binLevelsDir = fs::path(binLevelPath).parent_path().string();
        if (!fs::exists(binLevelsDir)) {
            std::cout << "[DEBUG] Creating bin levels directory: " << binLevelsDir << "\n";
            fs::create_directories(binLevelsDir);
        }
        
        std::string srcLevelsDir = fs::path(srcLevelPath).parent_path().string();
        if (!fs::exists(srcLevelsDir)) {
            std::cout << "[DEBUG] Creating src levels directory: " << srcLevelsDir << "\n";
            fs::create_directories(srcLevelsDir);
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "[ERROR] Failed to create directories: " << e.what() << "\n";
    }
    
    // Save to both paths
    std::cout << "[DEBUG] Saving level to bin path: " << binLevelPath << "\n";
    saveLevelToFile(binLevelPath);
    
    if (!srcLevelPath.empty()) {
        std::cout << "[DEBUG] Also saving level to src path: " << srcLevelPath << "\n";
        saveLevelToFile(srcLevelPath);
    } else {
        std::cerr << "[ERROR] Could not determine source directory path!\n";
    }
    
    // Store the bin path as current path for future saves
    m_currentLevelPath = binLevelPath;
    
    // Refresh level files list to include the newly saved level
    loadLevelFiles();
}

// Add this function to your Scene_LevelEditor.cpp file
void Scene_LevelEditor::saveLevelToFile(const std::string& savePath) {
    std::ofstream out(savePath);
    if (!out.is_open()) {
        std::cerr << "[ERROR] Cannot open file for saving: " << savePath << "\n";
        return;
    }
    
    auto stripWorldCategory = [this](const std::string& fullName) -> std::string {
        if (!worldcategory.empty() && fullName.find(worldcategory) == 0)
            return fullName.substr(worldcategory.size());
        return fullName;
    };

    // ========================
    // Save Tiles
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

        // Check if animName is empty
        if (animName.empty()) {
            std::cerr << "[ERROR] Empty tile name at (" << gridX << ", " << savedGridY << ")! Skipping...\n";
            continue;
        }

        out << "Tile " << animName << " " << gridX << " " << savedGridY << "\n";
    }

    // Save Player
    auto& players = m_entityManager.getEntities("player");
    if (!players.empty()) {
        auto& player = players.front();
        auto& transform = player->get<CTransform>();
        int gridX = static_cast<int>(transform.pos.x / tileSize);
        int gridY = static_cast<int>(transform.pos.y / tileSize);
        int savedGridY = worldHeight - 1 - gridY;
        
        out << "Player " << gridX << " " << savedGridY << "\n";
    }
    
    // ========================
    // Save Decorations
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

        // Check if animName is empty
        if (animName.empty()) {
            std::cerr << "[ERROR] Empty decoration name at (" << gridX << ", " << savedGridY << ")! Skipping...\n";
            continue;
        }

        out << "Dec " << animName << " " << gridX << " " << savedGridY << "\n";
    }
    
    // ========================
    // Save Enemies
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
            case EnemyType::Normal:  enemyType = "EnemyNormal"; break;
            case EnemyType::Super:   enemyType = "EnemySuper"; break;
            case EnemyType::Super2:  enemyType = "EnemySuper2"; break;
            case EnemyType::Citizen: enemyType = "EnemyCitizen"; break;
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

        out << "Enemy " << enemyType << " " << gridX << " " << savedGridY << "\n";
    }

    out.close();
    
    // Verify the file was written successfully
    if (!out) {
        std::cerr << "[ERROR] Failed to write level file: " << savePath << "\n";
    } else {
        std::cout << "[DEBUG] Level successfully saved to " << savePath << "\n";
    }
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
                // std::cerr << "[ERROR] Missing animation for " << fullAsset << "\n";
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
            if (m_game.assets().hasAnimation(fullAsset)) {
                entity->add<CAnimation>(m_game.assets().getAnimation(fullAsset), true);
            } else {
            //     std::cerr << "[ERROR] Missing animation for " << fullAsset << "\n";
            }
            // std::cout << "[DEBUG] Loaded Dec: " << fullAsset << " at (" << gridX << ", " << gridY << ")\n";
        }
        else if (token == "Player") {
            int fileGridX, fileGridY;
            file >> fileGridX >> fileGridY;
            int gridX = fileGridX;
            int gridY = worldHeight - 1 - fileGridY;
            auto entity = m_entityManager.addEntity("player");
            entity->add<CTransform>(Vec2<float>(gridX * tileSize, gridY * tileSize));
            
            std::string standAnim = "PlayerStand";
            if (m_game.assets().hasAnimation(standAnim)) {
                entity->add<CAnimation>(m_game.assets().getAnimation(standAnim), true);
            } else if (m_game.assets().hasAnimation("PlayerStand")) {
                entity->add<CAnimation>(m_game.assets().getAnimation("PlayerStand"), true);
            } else {
                // std::cerr << "[ERROR] Missing animation for player: " << standAnim << "\n";
            }
            // std::cout << "[DEBUG] Loaded Player at (" << gridX << ", " << gridY << ")\n";
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
                // std::cerr << "[ERROR] Missing animation for enemy type: " << standAnim << "\n";
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

    setCameraToBottomLeft();
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
