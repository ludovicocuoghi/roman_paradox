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

class CAmmo : public Component {
    public:
        int maxBullets = 6;            // Maximum bullets the player can have
        int currentBullets = 6;        // Current bullet count
        float reloadTime = 2.0f;       // Time to reload all bullets
        float currentReloadTime = 0.f; // Current reload timer
        bool isReloading = false;      // Whether player is currently reloading
        
        CAmmo() {}
        CAmmo(int max) : maxBullets(max), currentBullets(max) {}
        CAmmo(int max, int current) : maxBullets(max), currentBullets(current) {}
    };

class CLifeSpan : public Component {
public:
    float remainingTime;
    float totalTime;

    CLifeSpan(float duration = 1.0f) 
        : remainingTime(duration), totalTime(duration) {}
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

class CTileTouched : public Component {
    public:
        bool inFront;

        explicit CTileTouched(bool inFront_ = false) 
            : inFront(inFront_) {}
};

class CStopAfterTime : public Component {
    public:
        float timer = -1.f;  // Default is negative, meaning "inactive"
    
        CStopAfterTime() = default; 
    
        explicit CStopAfterTime(float timeToStop)
            : timer(timeToStop) {}
};

class CState : public Component {
    public:
        //====================================================
        //  Original Fields
        //====================================================
        std::string state;           // "idle", "run", "attack", "defense", etc.
        bool  isInvincible;
        float invincibilityTimer;
        bool  isJumping;
        float jumpTime;
        float knockbackTimer;
        bool  onGround;
        float attackTime;
        float attackCooldown;
        bool  blockedHorizontally;
    
        // Bullet-related cooldown (for ranged attacks)
        float bulletCooldown    = 0.f; 
        float bulletCooldownMax = 0.5f; 
    
        float defenseTimer      = 0.f;  // Timer per la difesa
        float shieldStamina;
        float maxshieldStamina;
    
        // Sword and bullet parameters
        int   maxConsecutiveSwordAttacks; // Numero massimo di attacchi consecutivi con spada
        float bulletDamage;               // Danno dei proiettili
        int   bulletBurstCount;           // Numero di proiettili sparati in un burst
        int   superBulletCount;           // Numero di super proiettili sparati in un burst
        float superBulletDamage;          // Danno dei super proiettili
    
        //====================================================
        //  NEW: Sword Cooldown (Separate from bulletCooldown)
        //====================================================
        float swordCooldown    = 0.f;  // Tempo corrente di cooldown per la spada
        float swordCooldownMax = 0.3f; // Valore di default (ad es. 0.3 secondi)
    
        //====================================================
        //  NEW Fields for BURST + SUPER MOVE
        //====================================================
        bool  inBurst         = false; // True se il player spara in burst
        float burstTimer      = 0.f;   // Timer che conta la durata del burst
        float burstDuration   = 5.f;   // Durata massima del burst
        float burstInterval   = 0.3f;  // Intervallo tra un proiettile e l'altro
        float burstFireTimer  = 0.f;   // Timer dal precedente proiettile
        int   bulletsShot     = 0;     // Contatore di quanti proiettili sparati
    
        float superBulletTimer   = 0.f;   // Timer per la super mossa
        float superBulletCooldown= 6.f;   // Cooldown totale per la super mossa
        bool  superMoveReady     = false; // Diventa true quando superBulletTimer >= superBulletCooldown
    
        //====================================================
        //  Constructor
        //====================================================
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
              maxshieldStamina(10.0f),
              maxConsecutiveSwordAttacks(3),
              bulletDamage(5.f),
              bulletBurstCount(3),
              superBulletCount(4),
              superBulletDamage(2.f)
        {
            // Optionally set any default you prefer for swordCooldownMax, bulletCooldownMax, etc.
            // e.g., swordCooldownMax = 0.3f; bulletCooldownMax = 0.5f;
        }
    
        //====================================================
        //  Update Function
        //====================================================
        void update(float deltaTime) 
        {
            //===============================
            // Original Timers
            //===============================
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
        
            //===============================
            // Ranged Bullet Cooldown
            //===============================
            if (bulletCooldown > 0.f) {
                bulletCooldown -= deltaTime;
                if (bulletCooldown < 0.f) {
                    bulletCooldown = 0.f;
                }
            }
        
            //===============================
            // Sword Cooldown (NEW)
            //===============================
            if (swordCooldown > 0.f) {
                swordCooldown -= deltaTime;
                if (swordCooldown < 0.f) {
                    swordCooldown = 0.f;
                }
            }
        
            //===============================
            // Attack Animation Timer
            //===============================
            if (attackTime > 0.f) {
                attackTime -= deltaTime;
                if (attackTime < 0.f) {
                    attackTime = 0.f;
                }
            }
        
            //===============================
            // Defense Logic
            //===============================
            if (state == "defense") {
                shieldStamina -= deltaTime;
                if (shieldStamina <= 0.f) {
                    shieldStamina = 0.f;
                    state = "idle";
                }
            }
        
            //===============================
            //  SUPER MOVE Timer (OPPOSITE APPROACH)
            //===============================
            if (superBulletTimer > 0.f) {
                superBulletTimer -= deltaTime;
                if (superBulletTimer <= 0.f) {
                    superBulletTimer = 0.f;
                    superMoveReady = true;
                }
            }
        
            //===============================
            //  BURST Logic
            //===============================
            if (inBurst) {
                burstTimer     += deltaTime;
                burstFireTimer += deltaTime;
        
                // End the burst if it exceeds burstDuration
                if (burstTimer >= burstDuration) {
                    inBurst        = false;
                    burstTimer     = 0.f;
                    burstFireTimer = 0.f;
                    bulletsShot    = 0;
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
    FollowTwo,   // Keeps following once the player is spotted
    FollowThree,  // Super always follows (ignores distance checks)
    FollowFour
};

enum class EnemyState {
    Idle,
    Follow,     // The enemy is following the player
    Attack,     // The enemy is attacking
    Knockback,   // The enemy is being pushed back,
    FinalAttack,
    BlockedByTile,
    Defeated
};

enum class BossPhase {
    Phase1,
    Phase2,
    Phase3
};

class CBossPhase : public Component {
    public:
        BossPhase phase;
        CBossPhase(BossPhase initialPhase = BossPhase::Phase1)
            : phase(initialPhase) {}
    };

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
        // 5) Gestione dell'attacco standard
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
        // 8) Timer di attacco "radiale" e "shooting"
        // ------------------------------------------------------
        float radialAttackTimer;      // Timer per l'attacco radiale
        float protectionPulseTimer = 0.f;
        bool protectionSwordsActive = false;
        float horizontalShootTimer;   // Timer per l'attacco orizzontale
        bool  TileInFront;            // Se c'è un tile di fronte (per logiche di salto/blocco)
        float shootTimer;             // Timer per gli intervalli di tiro "normale"
    
        // ------------------------------------------------------
        // 9) Parametri per Emperor Attack Scaling
        // ------------------------------------------------------
        float radialAttackMultiplier; // Moltiplicatore per il numero di spade in attacco radiale
        float radialAttackCooldown;   // Cooldown tra un attacco radiale e l'altro
        float radialAttackTimerSuper; // Timer per super attacco radiale
        float radialAttackDamage; // Timer per super attacco radiale
        float finalBurstTimer;        // Timer per la fase finale di burst
        int   burstCount;             // Quanti burst sono stati fatti nella fase finale
        bool  burstCooldownActive;    // Se il burst è in cooldown
        float burstCooldownTimer;     // Timer per il cooldown del burst
        bool  isInBurstMode;          // Se il nemico è in fase finale di burst

    
        // ------------------------------------------------------
        // 10) Parametri per il "Bullet Burst" e la "Super Move"
        // ------------------------------------------------------
        bool  inBurst;          // True se sta sparando un burst di proiettili
        int   bulletsShot;      // Quanti proiettili sono stati sparati in questo burst
        float burstInterval;    // Ritardo tra i proiettili di un burst
        float burstTimer;       // Timer per lo spacing dei proiettili in un burst
        int   bulletCount;      // Numero totale di proiettili per burst
    
        bool  superMoveReady;   // Se è pronto a eseguire una super mossa
        float superMoveTimer;   // Timer accumulato per la super move
        float superMoveCooldown;// Tempo di cooldown tra le super move
    
        // ------------------------------------------------------
        // 11) Distanza di shooting e cooldown
        // ------------------------------------------------------
        float minShootDistance; // Distanza minima per sparare
        float shootCooldown;    // Cooldown tra un colpo e l'altro
    
        // ------------------------------------------------------
        // 12) Cooldown forzato dopo X attacchi consecutivi
        // ------------------------------------------------------
        int   consecutiveAttacks;       // Quanti attacchi consecutivi ha fatto il nemico
        int   maxAttacksBeforeCooldown; // Quanti attacchi prima del cooldown
        float forcedCooldownDuration;   // Durata del cooldown (in secondi)
        float forcedCooldownTimer;      // Timer per il cooldown
        bool  isInForcedCooldown;       // Se il nemico è in cooldown
    
        // ------------------------------------------------------
        // 13) Parametri aggiuntivi per collisioni e gestione stato
        // ------------------------------------------------------
        bool tileDetected;
        bool isAttackingTile;
        bool canChangeState;
        float stateChangeCooldown;
        float phaseTimer;
    
        // ------------------------------------------------------
        // Costruttore
        // ------------------------------------------------------
        CEnemyAI(EnemyType type = EnemyType::Normal, EnemyBehavior behavior = EnemyBehavior::FollowOne)
            :   enemyType(type),
                enemyBehavior(behavior),
                enemyState(EnemyState::Idle),
    
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
                protectionPulseTimer(0.f),
                protectionSwordsActive(false),
                horizontalShootTimer(0.f),
                TileInFront(false),
                shootTimer(0.f),
    
                // Emperor Attack Scaling
                radialAttackMultiplier(1.0f),
                radialAttackCooldown(5.0f),
                radialAttackTimerSuper(5.0f),
                radialAttackDamage(1.0f),
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
                isInForcedCooldown(false),
    
                // Additional parameters
                tileDetected(false),
                isAttackingTile(false),
                canChangeState(true),
                stateChangeCooldown(0.f),
                phaseTimer(0.0f)
        {
        }
    };

class CPlayerEquipment : public Component {
    public:
        bool hasFutureArmor = false;
    };
