#pragma once

#include "Entity.hpp"
#include <vector>
#include <map>
#include <memory>
#include <string>
#include <algorithm>

using EntityVec = std::vector<std::shared_ptr<Entity>>;
using EntityMap = std::map<std::string, EntityVec>;

class EntityManager {
    EntityVec m_entities;    // Stores all entities
    EntityVec m_toAdd;       // Temporary storage for entities to be added
    EntityMap m_entityMap;   // Maps tags to groups of entities
    size_t m_totalEntities = 0; // Counter for unique entity IDs

public:
    // Add a new entity with a given tag
    std::shared_ptr<Entity> addEntity(const std::string& tag) {
        auto entity = std::shared_ptr<Entity>(new Entity(tag, m_totalEntities++));
        m_toAdd.push_back(entity);
        return entity;
    }
    // Update the EntityManager
    void update() {
        // Move new entities from m_toAdd to main storage
        for (auto& entity : m_toAdd) {
            m_entities.push_back(entity);
            m_entityMap[entity->tag()].push_back(entity);
        }
        m_toAdd.clear();

        // Remove dead entities from m_entities
        m_entities.erase(
            std::remove_if(m_entities.begin(), m_entities.end(),
                           [](const auto& e) { return !e->isAlive(); }),
            m_entities.end()
        );

        // Remove dead entities from m_entityMap
        for (auto& [tag, vec] : m_entityMap) {
            vec.erase(
                std::remove_if(vec.begin(), vec.end(),
                               [](const auto& e) { return !e->isAlive(); }),
                vec.end()
            );
        }
    }

    // Retrieve all entities
    EntityVec& getEntities() { return m_entities; }

    // Retrieve entities by tag
    EntityVec& getEntities(const std::string& tag) { return m_entityMap[tag]; }

    size_t countEntities(const std::string& tag) const {
        auto it = m_entityMap.find(tag);
        if (it != m_entityMap.end()) {
            return it->second.size();
        }
        return 0;
    }
        // **New Method: Clear All Entities**
    void clear() {
        m_entities.clear();
        m_toAdd.clear();
        m_entityMap.clear();
        m_totalEntities = 0;
    }
};
