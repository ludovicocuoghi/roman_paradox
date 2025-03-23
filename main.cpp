#include <SFML/Graphics.hpp>
#include <iostream>
#include "GameEngine.h"
#include "ResourcePath.h"

int main() {
    // Initialize the GameEngine with the assets configuration file
    GameEngine g(getResourcePath("assets/assets.txt"));

    // Start the game loop
    g.run();
}
