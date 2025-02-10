#pragma once

#include "Vec2.hpp"
#include <string>
#include <SFML/Graphics.hpp>
#include "Animation.hpp"

// Base Component
class Component {
public:
    virtual ~Component() = default;
};

// Transform Component
struct CTransform {
    Vec2<float> pos;
    Vec2<float> velocity;
    Vec2<float> scale;
    float rotation;

    CTransform(const Vec2<float>& p = {0, 0}, const Vec2<float>& v = {0, 0}, 
               const Vec2<float>& s = {1, 1}, float r = 0.0f)
        : pos(p), velocity(v), scale(s), rotation(r) {}
};

class CHealth : public Component {
public:
    int currentHealth;          // Salute attuale
    int maxHealth;              // Salute massima
    float invulnerabilityTimer; // Timer di invulnerabilità in secondi

    // Costruttore di default (necessario per il tuple)
    CHealth()
        : currentHealth(0), maxHealth(0), invulnerabilityTimer(0.f) {}

    // Costruttore per impostare salute corrente e massima uguali
    CHealth(int health)
        : currentHealth(health), maxHealth(health), invulnerabilityTimer(0.f) {}

    // Costruttore per impostare salute corrente e massima in modo diverso
    CHealth(int current, int max)
        : currentHealth(current), maxHealth(max), invulnerabilityTimer(0.f) {}

    // Applica danno se l'entity non è in invulnerabilità
    void takeDamage(int damage) {
        if (invulnerabilityTimer <= 0.f) {
            currentHealth -= damage;
            if (currentHealth < 0)
                currentHealth = 0;
        }
    }

    // (Facoltativo) Funzione per guarire l'entity
    void heal(int amount) {
        currentHealth += amount;
        if (currentHealth > maxHealth)
            currentHealth = maxHealth;
    }

    // Aggiorna il timer di invulnerabilità (da chiamare ogni frame)
    void update(float deltaTime) {
        if (invulnerabilityTimer > 0.f) {
            invulnerabilityTimer -= deltaTime;
            if (invulnerabilityTimer < 0.f)
                invulnerabilityTimer = 0.f;
        }
    }

    // Restituisce true se l'entity è ancora viva (currentHealth > 0)
    bool isAlive() const {
        return currentHealth > 0;
    }
};

// Lifespan Component
class CLifeSpan : public Component {
public:
    float remainingTime;
    float totalTime;

    CLifeSpan() : remainingTime(0.f), totalTime(0.f) {}
    CLifeSpan(float time)
        : remainingTime(time), totalTime(time) {}
};

// Input Component
class CInput : public Component {
public:
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
    bool shoot = false;
};

// Bounding Box Component
class CBoundingBox : public Component {
public:
    Vec2<float> size;
    Vec2<float> halfSize;

    // ✅ Fix: Ensure this constructor accepts both size and offset
    CBoundingBox(const Vec2<float>& s = {0, 0}, const Vec2<float>& hs = {0, 0})
        : size(s), halfSize(hs) {}

    sf::FloatRect getRect(const Vec2<float>& pos) const {
        return sf::FloatRect(pos.x - halfSize.x, pos.y - halfSize.y, size.x, size.y);
    }
};

// Animation Component
class CAnimation : public Component {
public:
    Animation animation;
    bool repeat = false;

    CAnimation() = default;
    CAnimation(const Animation& anim, bool rep)
        : animation(anim), repeat(rep) {}

    void update(float deltaTime) {
        animation.update(deltaTime);
    }
};

// Gravity Component
class CGravity : public Component {
public:
    float gravity;

    explicit CGravity(float g = 800.0f) : gravity(g) {}
};

class CState : public Component {
public:
    std::string state = "idle";
    bool isInvincible = false;
    float invincibilityTimer = 0.0f;

    // Campi per il salto e il knockback
    bool isJumping = false;
    float jumpTime = 0.0f;
    float knockbackTime = 0.0f;
    bool onGround = false;

    // Campi per l'attacco
    float attackTime = 0.0f;       // Durata dell'attacco (per l'animazione)
    float attackCooldown = 0.0f;   // Tempo di attesa prima che un nuovo attacco sia possibile

    CState(const std::string& s = "idle", bool inv = false, float timer = 0.0f,
           bool jumping = false, float jTime = 0.0f, float attTime = 0.0f, float attCooldown = 0.0f)
        : state(s),
          isInvincible(inv),
          invincibilityTimer(timer),
          isJumping(jumping),
          jumpTime(jTime),
          knockbackTime(0.0f),
          onGround(false),
          attackTime(attTime),
          attackCooldown(attCooldown)
    {}

    void update(float deltaTime) {
        if (isInvincible && invincibilityTimer > 0.f) {
            invincibilityTimer -= deltaTime;
            if (invincibilityTimer <= 0.f)
                isInvincible = false;
        }
        if (attackCooldown > 0.f) {
            attackCooldown -= deltaTime;
            if (attackCooldown < 0.f)
                attackCooldown = 0.f;
        }
        if (knockbackTime > 0.f) {
            knockbackTime -= deltaTime;
            if (knockbackTime < 0.f)
                knockbackTime = 0.f;
        }
        // Se necessario, aggiornare anche jumpTime qui
    }
};

class CShape : public Component {
public:
    int sides = 0;
    float radius = 0.0f;
    sf::Color fillColor = sf::Color::White;
    sf::Color outlineColor = sf::Color::Transparent;
    float outlineThickness = 0.0f;

    CShape() = default;
    CShape(int s, float r, const sf::Color& fill, const sf::Color& outline, float thickness)
        : sides(s), radius(r), fillColor(fill), outlineColor(outline), outlineThickness(thickness) {}
};

class CRotation : public Component {
public:
    float angle = 0.0f;
    float speed = 0.0f;  // Rotation speed

    CRotation() = default;
    CRotation(float a, float s) : angle(a), speed(s) {}
};

// New Enums for Enemy AI
enum class EnemyType {
    Fast,
    Normal,
    Strong,
    Elite
};

enum class EnemyState {
    Patrol,
    Chase,
    Attack,
    Returning
};

class CEnemyAI : public Component {
public:
    EnemyType enemyType;
    EnemyState enemyState;
    float detectionRadius;
    float attackRadius;
    float speedMultiplier;
    int damage;
    std::vector<Vec2<float>> patrolPoints;
    int currentPatrolIndex;
    float facingDirection; // already used for sprite flipping
    bool swordSpawned;     // NEW: to avoid spamming swords

    CEnemyAI() = default;  // Fix: missing semicolon

    // Fix: New constructor to accept EnemyType
    CEnemyAI(EnemyType type)
        : enemyType(type),
          enemyState(EnemyState::Patrol),
          detectionRadius(200.f),
          attackRadius(100.f),
          speedMultiplier(1.0f),
          damage(10),
          currentPatrolIndex(0),
          facingDirection(1.f),
          swordSpawned(false)
    {}
};
