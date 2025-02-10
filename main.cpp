#include <SFML/Graphics.hpp>
#include <iostream>
#include "GameEngine.h"

int main() {
    // Initialize the GameEngine with the assets configuration file
    GameEngine g("bin/assets/assets.txt");

    // Start the game loop
    g.run();
}
