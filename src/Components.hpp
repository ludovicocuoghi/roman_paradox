#pragma once

#include "Vec2.hpp"
#include <string>
#include <SFML/Graphics.hpp>
#include "Animation.hpp"
#include <vector>

// Base Component.
class Component {
public:
    virtual ~Component() = default;
};

// Transform Component.
struct CTransform {
    Vec2<float> pos;
    Vec2<float> velocity;
    Vec2<float> scale;
    float rotation;

    CTransform(const Vec2<float>& p = {0, 0},
               const Vec2<float>& v = {0, 0},
               const Vec2<float>& s = {1, 1},
               float r = 0.0f)
        : pos(p), velocity(v), scale(s), rotation(r) {}
};

class CHealth : public Component {
public:
    int currentHealth;
    int maxHealth;
    float invulnerabilityTimer;

    CHealth() : currentHealth(0), maxHealth(0), invulnerabilityTimer(0.f) {}
    CHealth(int health) : currentHealth(health), maxHealth(health), invulnerabilityTimer(0.f) {}
    CHealth(int current, int max) : currentHealth(current), maxHealth(max), invulnerabilityTimer(0.f) {}

    void takeDamage(int damage) {
        if (invulnerabilityTimer <= 0.f) {
            currentHealth -= damage;
            if (currentHealth < 0) {
                currentHealth = 0;
            }
        }
    }

    void heal(int amount) {
        currentHealth += amount;
        if (currentHealth > maxHealth) {
            currentHealth = maxHealth;
        }
    }

    void update(float deltaTime) {
        if (invulnerabilityTimer > 0.f) {
            invulnerabilityTimer -= deltaTime;
            if (invulnerabilityTimer < 0.f) {
                invulnerabilityTimer = 0.f;
            }
        }
    }

    bool isAlive() const {
        return currentHealth > 0;
    }
};

class CLifeSpan : public Component {
public:
    float remainingTime;
    float totalTime;

    CLifeSpan() : remainingTime(0.f), totalTime(0.f) {}
    CLifeSpan(float time) : remainingTime(time), totalTime(time) {}
};

class CInput : public Component {
public:
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
    bool shoot = false;
};

class CBoundingBox : public Component {
public:
    Vec2<float> size;
    Vec2<float> halfSize;

    CBoundingBox(const Vec2<float>& s = {0, 0}, const Vec2<float>& hs = {0, 0})
        : size(s), halfSize(hs) {}

    sf::FloatRect getRect(const Vec2<float>& pos) const {
        return sf::FloatRect(pos.x - halfSize.x, pos.y - halfSize.y, size.x, size.y);
    }
};

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

class CGravity : public Component {
public:
    float gravity;
    explicit CGravity(float g = 800.f) : gravity(g) {}
};

class CState : public Component {
public:
    std::string state;  // "idle", "run", "attack" (only for players)
    bool isInvincible;
    float invincibilityTimer;
    bool isJumping;
    float jumpTime;
    float knockbackTimer;
    bool onGround;
    float attackTime;
    float attackCooldown;

    CState(const std::string& s = "idle")
        : state(s),
          isInvincible(false),
          invincibilityTimer(0.f),
          isJumping(false),
          jumpTime(0.f),
          knockbackTimer(0.f),
          onGround(false),
          attackTime(0.f),
          attackCooldown(0.f)
    {}

    void update(float deltaTime) {
        if (isInvincible && invincibilityTimer > 0.f) {
            invincibilityTimer -= deltaTime;
            if (invincibilityTimer <= 0.f) {
                isInvincible = false;
            }
        }
        if (attackCooldown > 0.f) {
            attackCooldown -= deltaTime;
            if (attackCooldown < 0.f) {
                attackCooldown = 0.f;
            }
        }
        if (knockbackTimer > 0.f) {
            knockbackTimer -= deltaTime;
            if (knockbackTimer < 0.f) {
                knockbackTimer = 0.f;
            }
        }
    }
};

class CShape : public Component {
public:
    int sides = 0;
    float radius = 0.f;
    sf::Color fillColor = sf::Color::White;
    sf::Color outlineColor = sf::Color::Transparent;
    float outlineThickness = 0.f;

    CShape() = default;
    CShape(int s, float r, const sf::Color& fill, const sf::Color& outline, float thickness)
        : sides(s), radius(r), fillColor(fill), outlineColor(outline), outlineThickness(thickness) {}
};

class CRotation : public Component {
public:
    float angle = 0.f;
    float speed = 0.f;

    CRotation() = default;
    CRotation(float a, float s) : angle(a), speed(s) {}
};

enum class EnemyType {
    Fast,
    Normal,
    Strong,
    Elite
};

enum class EnemyBehavior {
    FollowOne,  // Only follows when player is in line-of-sight
    FollowTwo   // Keeps following once the player is spotted
};

enum class EnemyState {
    Idle,
    Recognition,
    Patrol,
    Follow,     // The enemy is following the player
    Attack,     // The enemy is attacking
    Knockback   // The enemy is being pushed back
};

// Reordered fields: declare 'patrolPoints' before 'enemyState' 
// so that 'enemyState' can safely check patrolPoints in its own initialization.
class CEnemyAI : public Component {
public:
    // Basic properties
    EnemyType enemyType;
    EnemyBehavior enemyBehavior;

    // Patrol Behavior
    std::vector<Vec2<float>> patrolPoints;
    EnemyState enemyState;  // Now declared after 'patrolPoints'

    // Movement & Combat Stats
    float speedMultiplier;
    int damage;

    // Vision & Attack Parameters
    float lineOfSightRange;
    float attackRadius;

    int currentPatrolIndex;
    float patrolWaitTime;

    // Jumping & Movement
    float facingDirection;
    float jumpCooldown;

    // Attack Handling
    bool swordSpawned;
    float attackCooldown;
    float attackTimer;

    // Knockback Handling
    float knockbackTimer;

    // Recognition Handling
    float recognitionTimer;
    float maxRecognitionTime;
    bool inRecognitionArea;
    Vec2<float> lastSeenPlayerPos;

    // Constructor
    CEnemyAI(EnemyType type = EnemyType::Normal, EnemyBehavior behavior = EnemyBehavior::FollowOne)
        : enemyType(type),
          enemyBehavior(behavior),
          patrolPoints(),
          enemyState(patrolPoints.empty() ? EnemyState::Idle : EnemyState::Patrol),
          speedMultiplier(1.0f),
          damage(1),
          lineOfSightRange(200.f),
          attackRadius(100.f),
          currentPatrolIndex(0),
          patrolWaitTime(0.f),
          facingDirection(1.f),
          jumpCooldown(0.5f),
          swordSpawned(false),
          attackCooldown(0.f),
          attackTimer(0.f),
          knockbackTimer(0.f),
          recognitionTimer(0.f),
          maxRecognitionTime(5.0f),
          inRecognitionArea(false),
          lastSeenPlayerPos(Vec2<float>(0.f, 0.f))
    {}
};
