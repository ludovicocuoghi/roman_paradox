#include "Scene.h"

Scene::Scene(GameEngine& game) : m_game(game) {}

void Scene::registerAction(int key, const std::string& actionName) {
    m_actionMap[key] = actionName;
}

// Register default actions available in all scenes:
// - ESC: Quit the game
void Scene::registerCommonActions() {
    registerAction(sf::Keyboard::Escape, "QUIT");
}

void Scene::togglePause() {
    m_paused = !m_paused;
}
