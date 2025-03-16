#pragma once

#include <string>
#include "Vec2.hpp"

struct Tile {
    std::string category;  // Category of the tile (e.g., "terrain", "item")
    std::string type;      // Specific type within category (e.g., "grass", "water")
    Vec2<int> position;    // Tile position in grid coordinates

    Tile() = default;
    Tile(const std::string& cat, const std::string& t, int x, int y)
        : category(cat), type(t), position(x, y) {}
};
