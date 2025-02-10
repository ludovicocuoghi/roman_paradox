#include "Scene.h"

Scene::Scene(GameEngine& game) : m_game(game) {}

void Scene::registerAction(int key, const std::string& actionName) {
    m_actionMap[key] = actionName;
}

void Scene::registerCommonActions() {
    registerAction(sf::Keyboard::Escape, "QUIT");
    registerAction(sf::Keyboard::P, "PAUSE");
}

void Scene::togglePause() {
    m_paused = !m_paused;
}
