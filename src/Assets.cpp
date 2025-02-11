#include "Assets.hpp"
#include <iostream>
#include <fstream>

// Constructor: Initialize default assets
Assets::Assets() {
    // Create a 1x1 white texture
    m_defaultTexture.create(1, 1);
    sf::Uint8 whitePixel[4] = {255, 255, 255, 255};  // RGBA (White)
    m_defaultTexture.update(whitePixel, 1, 1, 0, 0);

    // Default Animation
    m_defaultAnimation = Animation(m_defaultTexture, 1, 1, 1, 0);

    // Default Font (Avoid crash if fonts are missing)
    if (!m_defaultFont.loadFromFile("bin/fonts/default.ttf")) {
        std::cerr << "[ERROR] Default font missing! Some text may not display properly.\n";
    }
}

// getDefaultAnimation() is const
const Animation& Assets::getDefaultAnimation() const {
    return m_defaultAnimation;
}

// Load and store textures
void Assets::addTexture(const std::string& name, const std::string& path) {
    sf::Texture texture;
    if (!texture.loadFromFile("bin/images/" + path)) {
        std::cerr << "[Warning] Failed to load texture: bin/images/" << path << ". Using default.\n";
        m_textureMap[name] = m_defaultTexture;
        return;
    }
    m_textureMap[name] = std::move(texture);
}

// Load and store fonts
void Assets::addFont(const std::string& name, const std::string& path) {
    sf::Font font;
    std::string fullPath = "bin/" + path;

    if (!font.loadFromFile(fullPath)) {
        std::cerr << "[ERROR] Failed to load font: " << fullPath << ". Using default.\n";
        m_fontMap[name] = m_defaultFont;
        return;
    }

    m_fontMap[name] = std::move(font);
}

/**
 * addAnimation:
 *    @param name        = string key for this animation
 *    @param textureName = key of the already-loaded texture
 *    @param frameWidth  = width of each frame in px
 *    @param frameHeight = height of each frame in px
 *    @param frameCount  = total number of frames (across one row)
 *    @param fps         = frames per second (NOT ms/frame)
 */
void Assets::addAnimation(const std::string& name, 
                          const std::string& textureName,
                          int frameWidth, 
                          int frameHeight, 
                          int frameCount, 
                          int fps)
{
    auto it = m_textureMap.find(textureName);
    if (it == m_textureMap.end()) {
        std::cerr << "[Warning] Texture " << textureName 
                  << " not loaded for animation " << name 
                  << ". Using default animation.\n";
        m_animationMap[name] = m_defaultAnimation;
        return;
    }

    // Pass 'fps' for speed, 'true' for repeating, and 'name' to label the animation
    m_animationMap[name] = Animation(it->second, 
                                     frameWidth, 
                                     frameHeight, 
                                     frameCount, 
                                     fps,       // frames/second
                                     true,      // repeating
                                     name);     // sets m_name to "PlayerRun", etc.
}

// Retrieve assets safely
const sf::Texture& Assets::getTexture(const std::string& name) const {
    auto it = m_textureMap.find(name);
    if (it == m_textureMap.end()) {
        std::cerr << "[ERROR] Texture '" << name << "' not found! Using default.\n";
        return m_defaultTexture;
    }
    return it->second;
}
bool Assets::hasFont(const std::string& name) const {
    return m_fontMap.find(name) != m_fontMap.end();
}

const Animation& Assets::getAnimation(const std::string& name) const {
    auto it = m_animationMap.find(name);
    if (it == m_animationMap.end()) {
        std::cerr << "[ERROR] Animation '" << name << "' not found! Using default.\n";
        return getDefaultAnimation();
    }
    return it->second;
}

const sf::Font& Assets::getFont(const std::string& name) const {
    auto it = m_fontMap.find(name);
    if (it == m_fontMap.end()) {
        std::cerr << "[ERROR] Font '" << name << "' not found! Using default.\n";
        return m_defaultFont;
    }
    return it->second;
}

// Check if an animation exists
bool Assets::hasAnimation(const std::string& name) const {
    return m_animationMap.find(name) != m_animationMap.end();
}

// Load assets from a file
void Assets::loadFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "[ERROR] Failed to open assets file: " << filePath << std::endl;
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream stream(line);
        std::string type, name, textureName;
        stream >> type >> name >> textureName;

        if (type == "Texture") {
            addTexture(name, textureName);
            std::cout << "[DEBUG] Loaded Texture: " << name 
                      << " -> " << textureName << std::endl;
        } 
        else if (type == "Font") {
            addFont(name, textureName);
            std::cout << "[DEBUG] Loaded Font: " << name 
                      << " -> " << textureName << std::endl;
        } 
        else if (type == "Animation") {
            int frameWidth, frameHeight, frameCount, fps;
            stream >> frameWidth >> frameHeight >> frameCount >> fps;

            if (m_textureMap.find(textureName) == m_textureMap.end()) {
                std::cerr << "[WARNING] Missing texture: " 
                          << textureName << " for animation: " 
                          << name << std::endl;
                continue;
            }

            addAnimation(name, textureName, frameWidth, frameHeight, frameCount, fps);

            std::cout << "[DEBUG] Loaded Animation: " << name 
                      << " from " << textureName << " [" 
                      << frameWidth << "x" << frameHeight << ", " 
                      << frameCount << " frames, FPS: " << fps << "]" 
                      << std::endl;
        }
    }

    std::cout << "[DEBUG] Asset Loading Completed. Textures: " 
              << m_textureMap.size() << " | Animations: " 
              << m_animationMap.size() << " | Fonts: " 
              << m_fontMap.size() << std::endl;
}
