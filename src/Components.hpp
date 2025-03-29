#pragma once

#include "Vec2.hpp"
#include <string>
#include <SFML/Graphics.hpp>
#include "Animation.hpp"
#include <vector>

// Base Component
class Component {
public:
    virtual ~Component() = default;
};

// Transform Component
class CTransform : public Component {
    public:
        Vec2<float> pos;
        Vec2<float> velocity;
        Vec2<float> scale;
        float rotation;
        float facingDirection = -1.f; // or -1.f
    
        CTransform(const Vec2<float>& p = {0, 0},
                   const Vec2<float>& v = {0, 0},
                   const Vec2<float>& s = {1, 1},
                   float r = 0.0f)
            : pos(p), velocity(v), scale(s), rotation(r) {}
    
        // Rotate velocity vector by a given angle in degrees
        void rotate(float angleDegrees) {
            float radians = angleDegrees * (3.14 / 180.f); // Convert degrees to radians
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

    // Returns collision rectangle at given position
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
    
        float defenseTimer      = 0.f;  // Timer for defense
        float shieldStamina;
        float maxshieldStamina;
    
        // Sword and bullet parameters
        int   maxConsecutiveSwordAttacks; // Maximum consecutive sword attacks
        float bulletDamage;               // Bullet damage
        int   bulletBurstCount;           // Number of bullets in a burst
        int   superBulletCount;           // Number of super bullets in a burst
        float superBulletDamage;          // Super bullet damage
    
        //====================================================
        //  Sword Cooldown (Separate from bulletCooldown)
        //====================================================
        float swordCooldown    = 0.f;  // Current sword cooldown
        float swordCooldownMax = 0.3f; // Default value (e.g., 0.3 seconds)
    
        //====================================================
        //  Fields for BURST + SUPER MOVE
        //====================================================
        bool  inBurst         = false; // True if player is in burst mode
        float burstTimer      = 0.f;   // Timer for burst duration
        float burstDuration   = 5.f;   // Maximum burst duration
        float burstInterval   = 0.3f;  // Interval between bullets in burst
        float burstFireTimer  = 0.f;   // Timer since last bullet in burst
        int   bulletsShot     = 0;     // Count of bullets shot in burst
    
        float superBulletTimer   = 0.f;   // Timer for super move
        float superBulletCooldown= 6.f;   // Total cooldown for super move
        bool  superMoveReady     = false; // True when superBulletTimer >= superBulletCooldown
    
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
            // Optionally set any default for swordCooldownMax, bulletCooldownMax, etc.
            // e.g., swordCooldownMax = 0.3f; bulletCooldownMax = 0.5f;
        }
    
        //====================================================
        //  Update Function
        //====================================================
        void update(float deltaTime) 
        {
            //===============================
            // Timers
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
            // Sword Cooldown
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
            //  SUPER MOVE Timer 
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
    
        // Default constructor required by tuple
        CUniqueID() = default;
    
        // Constructor that initializes with a string
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
    Super2,
    Emperor,
    Citizen
};

enum class EnemyBehavior {
    FollowOne,  
    FollowTwo,  
    FollowThree,
    FollowFour,
    Flee
};

enum class EnemyState {
    Idle,
    Follow,     // The enemy is following the player
    Attack,     // The enemy is attacking
    Knockback,  
    FinalAttack,
    BlockedByTile,
    Defeated,
    Flee
};

enum class BossPhase {
    Phase1,
    Phase2,
    Phase3,
    Phase4
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
        // 1) Basic properties and behavior
        // ------------------------------------------------------
        EnemyType     enemyType;      // Type of enemy (Normal, Elite, etc.)
        EnemyBehavior enemyBehavior;  // Behavior (FollowOne, FollowTwo, etc.)
        EnemyState    enemyState;     // Current state (Idle, Attack, etc.)
    
        // ------------------------------------------------------
        // 2) Movement and combat parameters
        // ------------------------------------------------------
        float speedMultiplier;        // Movement speed multiplier
        int   damage;                 // Damage inflicted by the enemy
    
        // ------------------------------------------------------
        // 3) Vision and attack parameters
        // ------------------------------------------------------
        float lineOfSightRange;       // Enemy's line of sight range
        float attackRadius;           // Distance within which it attacks
    
        // ------------------------------------------------------
        // 4) Jump and movement handling
        // ------------------------------------------------------
        float facingDirection;        // 1.f or -1.f for direction
        float jumpCooldown;
        bool  isJumping;
        float jumpTimer;
        float blockedHorizontallyTime; // Time blocked horizontally
    
        // ------------------------------------------------------
        // 5) Standard attack handling
        // ------------------------------------------------------
        bool  swordSpawned;
        float attackCooldown;
        float attackTimer;
    
        // ------------------------------------------------------
        // 6) Knockback handling
        // ------------------------------------------------------
        float knockbackTimer;
    
        // ------------------------------------------------------
        // 7) Player recognition
        // ------------------------------------------------------
        float recognitionTimer;       // Recognition timer
        float maxRecognitionTime;     // Maximum follow time after losing sight of player
        bool  inRecognitionArea;      // If player is in recognition area
        Vec2<float> lastSeenPlayerPos;// Last seen player position
    
        // ------------------------------------------------------
        // 8) Radial and shooting attack timers
        // ------------------------------------------------------
        float radialAttackTimer;      // Timer for radial attack
        float protectionPulseTimer = 0.f;
        bool protectionSwordsActive = false;
        float horizontalShootTimer;   // Timer for horizontal attack
        bool  TileInFront;            // If there's a tile in front (for jump/block logic)
        float shootTimer;             // Timer for normal shooting intervals
    
        // ------------------------------------------------------
        // 9) Parameters for Emperor Attack Scaling
        // ------------------------------------------------------
        float radialAttackMultiplier; // Multiplier for number of swords in radial attack
        float radialAttackCooldown;   // Cooldown between radial attacks
        float radialAttackTimerSuper; // Timer for super radial attack
        float radialAttackDamage; // Timer for super radial attack
        float finalBurstTimer;        // Timer for final burst phase
        float defeatTimer;            // Timer for defeat
        int   burstCount;             // Number of bursts in final phase
        bool  burstCooldownActive;    // If burst is in cooldown
        float burstCooldownTimer;     // Timer for burst cooldown
        bool  isInBurstMode;          // If enemy is in final burst phase

    
        // ------------------------------------------------------
        // 10) Parameters for "Bullet Burst" and "Super Move"
        // ------------------------------------------------------
        bool  inBurst;          // True if shooting a burst of bullets
        int   bulletsShot;      // Number of bullets shot in this burst
        float burstInterval;    // Delay between bullets in a burst
        float burstTimer;       // Timer for spacing bullets in a burst
        int   bulletCount;      // Total number of bullets per burst
    
        bool  superMoveReady;   // If ready to perform a super move
        float superMoveTimer;   // Accumulated timer for super move
        float superMoveCooldown;// Cooldown time between super moves
    
        // ------------------------------------------------------
        // 11) Shooting distance and cooldown
        // ------------------------------------------------------
        float minShootDistance; // Minimum distance to shoot
        float maxShootDistance; // Maximum distance to shoot
        float shootCooldown;    // Cooldown between shots
    
        // ------------------------------------------------------
        // 12) Forced cooldown after X consecutive attacks
        // ------------------------------------------------------
        int   consecutiveAttacks;       // Number of consecutive attacks
        int   maxAttacksBeforeCooldown; // Number of attacks before cooldown
        float forcedCooldownDuration;   // Cooldown duration (in seconds)
        float forcedCooldownTimer;      // Timer for cooldown
        bool  isInForcedCooldown;       // If enemy is in cooldown
    
        // ------------------------------------------------------
        // 13) Additional parameters for collision and state management
        // ------------------------------------------------------
        bool tileDetected;
        bool isAttackingTile;
        bool canChangeState;
        float stateChangeCooldown;
        float phaseTimer;
        float blackHoleTimer;
        float blackHoleCooldown;
    
        // ------------------------------------------------------
        // Constructor
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
                facingDirection(-1.f),
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
                defeatTimer(0.f),
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
                minShootDistance(200.f),
                maxShootDistance(900.f),
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
                phaseTimer(0.0f),
                blackHoleTimer(0.0f),
                blackHoleCooldown(0.0f)
        {
        }
    };

class CPlayerEquipment : public Component {
    public:
        bool hasFutureArmor = false;
    };
