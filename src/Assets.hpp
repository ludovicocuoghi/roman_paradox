#pragma once

#include <unordered_map>
#include <string>
#include <SFML/Graphics.hpp>
#include <sstream> 
#include "Animation.hpp"

class Assets {
private:
    std::unordered_map<std::string, sf::Texture> m_textureMap;
    std::unordered_map<std::string, Animation> m_animationMap;
    std::unordered_map<std::string, sf::Font> m_fontMap;

    sf::Texture m_defaultTexture;
    Animation m_defaultAnimation;
    sf::Font m_defaultFont;

public:
    Assets();

    void addTexture(const std::string& name, const std::string& path);
    void addFont(const std::string& name, const std::string& path);
    void addAnimation(const std::string& name, const std::string& textureName, int frameWidth, int frameHeight, int frameCount, int speed);
    bool hasFont(const std::string& name) const;

    bool hasAnimation(const std::string& name) const;

    const sf::Texture& getTexture(const std::string& name) const;
    const Animation& getAnimation(const std::string& name) const;
    const sf::Font& getFont(const std::string& name) const;

    void loadFromFile(const std::string& filePath);

    const Animation& getDefaultAnimation() const; 
};
