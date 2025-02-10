#pragma once

#include <tuple>
#include <string>
#include <utility>
#include <SFML/Graphics.hpp>
#include "Components.hpp"

// Componenti inclusi nel tuple, ora con CEnemyAI aggiornato
using ComponentTuple = std::tuple<
    CTransform,
    CState,
    CBoundingBox,
    CLifeSpan,
    CInput,
    CAnimation,
    CGravity,
    CRotation,
    CHealth,
    CShape,
    CEnemyAI
>;

class Entity {
private:
    ComponentTuple m_components;
    bool m_alive = true;
    std::string m_tag;
    size_t m_id;

public:
    // Costruttore
    Entity(const std::string& tag, size_t id)
        : m_components(), m_tag(tag), m_id(id) {}

    // Stato dell'entità
    bool isAlive() const { return m_alive; }
    void destroy() { m_alive = false; }

    // Metadati
    const std::string& tag() const { return m_tag; }
    size_t id() const { return m_id; }

    // Gestione dei Componenti
    template <typename T, typename... Args>
    void add(Args&&... args) {
        static_assert(tuple_has_type<T, ComponentTuple>::value, "Component not found in tuple!");
        std::get<T>(m_components) = T(std::forward<Args>(args)...);
    }

    template <typename T>
    T& get() {
        static_assert(tuple_has_type<T, ComponentTuple>::value, "Component not found in tuple!");
        return std::get<T>(m_components);
    }

    template <typename T>
    const T& get() const {
        static_assert(tuple_has_type<T, ComponentTuple>::value, "Component not found in tuple!");
        return std::get<T>(m_components);
    }

    // Verifica se un componente è presente (a compile-time, restituisce true per i tipi del tuple)
    template <typename T>
    bool has() const {
        return tuple_has_type<T, ComponentTuple>::value;
    }

    // Bounding Box
    sf::FloatRect getBounds() const {
        if constexpr (tuple_has_type<CBoundingBox, ComponentTuple>::value) {
            const auto& transform = get<CTransform>();
            const auto& bbox = get<CBoundingBox>();
            return bbox.getRect(transform.pos);
        }
        return sf::FloatRect();
    }

private:
    // Utilità per verificare se un tipo esiste nel tuple
    template <typename T, typename Tuple>
    struct tuple_has_type;

    template <typename T, typename... Ts>
    struct tuple_has_type<T, std::tuple<Ts...>> {
        static constexpr bool value = (std::is_same_v<T, Ts> || ...);
    };
};
