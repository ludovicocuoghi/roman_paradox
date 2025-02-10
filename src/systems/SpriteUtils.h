#pragma once
#include <SFML/Graphics.hpp>

// Flips a sprite horizontally so that its bottom center remains fixed.
inline void flipSpriteLeft(sf::Sprite& sprite) {
    sf::FloatRect bounds = sprite.getLocalBounds();
    sprite.setScale(-1.f, 1.f);
    sprite.setOrigin(bounds.width, bounds.height * 0.5f);
}

// Resets the sprite to its original orientation.
inline void flipSpriteRight(sf::Sprite& sprite) {
    sprite.setScale(1.f, 1.f);
    sprite.setOrigin(0.f, sprite.getLocalBounds().height * 0.5f);
}
