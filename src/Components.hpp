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
class CTransform : public Component {
    public:
        Vec2<float> pos;
        Vec2<float> velocity;
        Vec2<float> scale;
        float rotation;
        float facingDirection = 1.f; // or -1.f
    
        CTransform(const Vec2<float>& p = {0, 0},
                   const Vec2<float>& v = {0, 0},
                   const Vec2<float>& s = {1, 1},
                   float r = 0.0f)
            : pos(p), velocity(v), scale(s), rotation(r) {}
    
        // Rotate velocity vector by a given angle in degrees
        void rotate(float angleDegrees) {
            float radians = angleDegrees * (M_PI / 180.f); // Convert degrees to radians
            float cosA = cos(radians);
            float sinA = sin(radians);
    
            float newX = velocity.x * cosA - velocity.y * sinA;
            float newY = velocity.x * sinA + velocity.y * cosA;
    
            velocity.x = newX;
            velocity.y = newY;
        }
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
        std::string state;  // "idle", "run", "attack", "defense", ecc.
        bool isInvincible;
        float invincibilityTimer;
        bool isJumping;
        float jumpTime;
        float knockbackTimer;
        bool onGround;
        float attackTime;
        float attackCooldown;
        bool blockedHorizontally;
        float bulletCooldown   = 0.f;  // Tempo corrente di cooldown del proiettile
        float bulletCooldownMax= 0.5f;   // Cooldown di default se non hai armatura
        float defenseTimer     = 0.f;   // Timer per la difesa
        float shieldStamina;
        float maxshieldStamina;
    
        CState(const std::string& s = "idle")
            : state(s),
              isInvincible(false),
              invincibilityTimer(0.f),
              isJumping(false),
              jumpTime(0.f),
              knockbackTimer(0.f),
              onGround(false),
              attackTime(0.f),
              attackCooldown(0.f),
              blockedHorizontally(false),
              shieldStamina(10.0f),
              maxshieldStamina(10.0f)
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
            if (bulletCooldown > 0.f) {
                bulletCooldown -= deltaTime;
                if (bulletCooldown < 0.f) bulletCooldown = 0.f;
            }
            // Aggiornamento del timer per l'attacco
            if (attackTime > 0.f) {
                attackTime -= deltaTime;
                if (attackTime < 0.f) attackTime = 0.f;
            }
            // Se siamo in stato defense, accumula il tempo d'uso e verifica se si è esaurito il tempo massimo (2 sec)
            if (state == "defense") {
                shieldStamina -= deltaTime;
                if (shieldStamina <= 0.f) {
                    shieldStamina = 0.f;
                    state = "idle";
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

class CUniqueID : public Component {
    public:
        std::string id;
    
        // Costruttore di default richiesto dal tuple
        CUniqueID() = default;
    
        // Costruttore che inizializza con una stringa
        CUniqueID(const std::string& idString)
            : id(idString)
        {}
    };

enum class EnemyType {
    Fast,
    Normal,
    Strong,
    Elite,
    Super,
    Emperor
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
    Knockback,   // The enemy is being pushed back,
    FinalAttack,
    BlockedByTile
};

// Reordered fields: declare 'patrolPoints' before 'enemyState' 
// so that 'enemyState' can safely check patrolPoints in its own initialization.
class CEnemyAI : public Component {
    public:
        // ------------------------------------------------------
        // 1) Proprietà di base e comportamento
        // ------------------------------------------------------
        EnemyType     enemyType;      // Tipo di nemico (Normal, Elite, ecc.)
        EnemyBehavior enemyBehavior;  // Comportamento (FollowOne, FollowTwo, ecc.)
        EnemyState    enemyState;     // Stato corrente (Idle, Attack, ecc.)
    
        // ------------------------------------------------------
        // 2) Parametri di movimento e combattimento
        // ------------------------------------------------------
        float speedMultiplier;        // Moltiplicatore di velocità di movimento
        int   damage;                 // Danno inflitto dal nemico
    
        // ------------------------------------------------------
        // 3) Parametri di visione e attacco
        // ------------------------------------------------------
        float lineOfSightRange;       // Raggio di vista del nemico
        float attackRadius;           // Distanza entro la quale attacca
    
        // ------------------------------------------------------
        // 4) Gestione del salto e del movimento
        // ------------------------------------------------------
        float facingDirection;        // 1.f o -1.f per direzione
        float jumpCooldown;
        bool  isJumping;
        float jumpTimer;
        float blockedHorizontallyTime; // Tempo bloccato orizzontalmente
    
        // ------------------------------------------------------
        // 5) Gestione dell’attacco standard
        // ------------------------------------------------------
        bool  swordSpawned;
        float attackCooldown;
        float attackTimer;
    
        // ------------------------------------------------------
        // 6) Gestione del knockback
        // ------------------------------------------------------
        float knockbackTimer;
    
        // ------------------------------------------------------
        // 7) Riconoscimento del giocatore
        // ------------------------------------------------------
        float recognitionTimer;       // Timer di riconoscimento
        float maxRecognitionTime;     // Tempo massimo di inseguimento dopo aver perso di vista il player
        bool  inRecognitionArea;      // Se il player è nell'area di riconoscimento
        Vec2<float> lastSeenPlayerPos;// Ultima posizione vista del giocatore
    
        // ------------------------------------------------------
        // 8) Timer di attacco “radiale” e “shooting”
        // ------------------------------------------------------
        float radialAttackTimer;      // Timer per l'attacco radiale
        float horizontalShootTimer;   // Timer per l'attacco orizzontale
        bool  TileInFront;            // Se c’è un tile di fronte (per logiche di salto/blocco)
        float shootTimer;             // Timer per gli intervalli di tiro "normale"
    
        // ------------------------------------------------------
        // 9) Parametri per Emperor Attack Scaling
        // ------------------------------------------------------
        float radialAttackMultiplier; // Moltiplicatore per numero di spade in attacco radiale
        float radialAttackCooldown;   // Cooldown tra un attacco radiale e l'altro
        float radialAttackTimerSuper; // Timer per super attacco radiale
        float finalBurstTimer;        // Timer per la fase finale di burst
        int   burstCount;             // Quanti burst sono stati fatti nella fase finale
        bool  burstCooldownActive;    // Se il burst è in cooldown
        float burstCooldownTimer;     // Timer per il cooldown del burst
        bool  isInBurstMode;          // Se il nemico è in fase finale di burst
    
        // ------------------------------------------------------
        // 10) Parametri per il “Bullet Burst” e la “Super Move”
        // ------------------------------------------------------
        bool  inBurst;          // True se sta sparando un burst di proiettili
        int   bulletsShot;      // Quanti proiettili sparati in questo burst
        float burstInterval;    // Ritardo tra i proiettili di un burst
        float burstTimer;       // Timer per lo spacing dei proiettili in un burst
        int   bulletCount;      // Numero totale di proiettili per burst
    
        bool  superMoveReady;   // Se è pronto a eseguire una super mossa
        float superMoveTimer;   // Timer accumulato per la super move
        float superMoveCooldown;// Tempo di cooldown tra super move
    
        // ------------------------------------------------------
        // 11) Distanza di shooting e cooldown
        // ------------------------------------------------------
        float minShootDistance; // Distanza minima per sparare
        float shootCooldown;    // Cooldown tra un colpo e l’altro
    
        // ------------------------------------------------------
        // 12) Cooldown forzato dopo X attacchi consecutivi
        // ------------------------------------------------------
        int   consecutiveAttacks;       // Quanti attacchi consecutivi ha fatto
        int   maxAttacksBeforeCooldown; // Quanti attacchi prima del cooldown
        float forcedCooldownDuration;   // Durata del cooldown (in secondi)
        float forcedCooldownTimer;      // Timer per il cooldown
        bool  isInForcedCooldown;       // Se il nemico è in cooldown
    
        // ------------------------------------------------------
        // Costruttore
        // ------------------------------------------------------
        CEnemyAI(EnemyType type = EnemyType::Normal, EnemyBehavior behavior = EnemyBehavior::FollowOne)
            : enemyType(type),
              enemyBehavior(behavior),
              enemyState(EnemyState::Idle), // Default: Idle
    
              // Movement & Combat
              speedMultiplier(1.0f),
              damage(1),
    
              // Vision & Attack
              lineOfSightRange(200.f),
              attackRadius(100.f),
    
              // Jump & Movement
              facingDirection(1.f),
              jumpCooldown(0.5f),
              isJumping(false),
              jumpTimer(0.f),
              blockedHorizontallyTime(0.f),
    
              // Attack Handling
              swordSpawned(false),
              attackCooldown(0.f),
              attackTimer(0.f),
    
              // Knockback
              knockbackTimer(0.f),
    
              // Recognition
              recognitionTimer(0.f),
              maxRecognitionTime(5.0f),
              inRecognitionArea(false),
              lastSeenPlayerPos(Vec2<float>(0.f, 0.f)),
    
              // Attack Timers
              radialAttackTimer(0.f),
              horizontalShootTimer(0.f),
              TileInFront(false),
              shootTimer(0.f),            // Inizializzato a 0
    
              // Emperor Attack Scaling
              radialAttackMultiplier(1.0f),
              radialAttackCooldown(5.0f),
              radialAttackTimerSuper(5.0f),
              finalBurstTimer(0.f),
              burstCount(0),
              burstCooldownActive(false),
              burstCooldownTimer(0.f),
              isInBurstMode(false),
    
              // Bullet Burst & Super Move
              inBurst(false),
              bulletsShot(0),
              burstInterval(0.1f),
              burstTimer(0.f),
              bulletCount(8),
              superMoveReady(false),
              superMoveTimer(0.f),
              superMoveCooldown(5.f),
    
              // Shooting distance & cooldown
              minShootDistance(100.f),
              shootCooldown(1.f),
    
              // Forced cooldown
              consecutiveAttacks(0),
              maxAttacksBeforeCooldown(5),
              forcedCooldownDuration(5.f),
              forcedCooldownTimer(0.f),
              isInForcedCooldown(false)
        {
            // Parametri speciali per Elite
            if (type == EnemyType::Elite) {
                maxAttacksBeforeCooldown = 5;
                forcedCooldownDuration   = 2.f;  // Esempio: Elite ha cooldown più breve
            }
        }
    };

class CPlayerEquipment : public Component {
    public:
        bool hasFutureArmor = false;
    };
