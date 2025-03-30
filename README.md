# Time-Traveling Alien Legionary Game

## Overview

This game follows the journey of an Alien Legionary who travels through time, experiencing different eras of Rome while fighting enemies and unraveling a mysterious plot connecting past, present, and future.

## Game Structure

### Scene Management

The game uses a scene-based architecture where different scenes (Menu, Play, GameOver, StoryText) handle specific game states. The key scene is `Scene_Play` which manages gameplay, entities, collisions, and dialogue.

### Entity-Component System

- **Entities**: Base game objects (players, enemies, obstacles, etc.)
- **Components**: Building blocks that define entity behavior (CTransform, CAnimation, CState, etc.)
- **Systems**: Process entities with specific components (AnimationSystem, CollisionSystem, etc.)

## Key Features

### Level Structure

The game progresses through multiple eras:
- **Alien Rome**: The starting point, under invasion by mysterious dark warriors
- **Ancient Rome**: Where the player first time-travels to after entering a black hole
- **Future Rome**: A paradoxical timeline created by the player's actions in the past

### Player Mechanics

- Basic movement (left/right/jump) with momentum-based physics
- Melee combat with sword attacks
- Defense mechanism using stamina
- Special abilities (in Future Rome):
  - Burst fire shooting
  - Super move

### Enemy AI System

The `EnemyAISystem` handles different enemy behaviors:

1. **Basic Enemies**:
   - Patrol within boundaries
   - Chase player when in detection range
   - Attack when in striking distance

2. **Elite Enemies**:
   - More aggressive chasing
   - Special attacks with longer range
   - Higher health and damage

3. **Boss Enemies**:
   - Phase-based combat
   - Special attack patterns
   - Dialogue triggers at specific health thresholds

### Dialogue System

An interactive dialogue system presents the story through character conversations:
- Supports multiple languages (English/Japanese)
- Dialogue triggers based on position and events
- Character portraits with customizable text formatting
- Special dialogue for boss encounters and key story moments

### Camera System

- Smooth following camera that tracks the player
- Zoom and offset settings for optimal viewing
- Camera boundaries to prevent viewing outside level bounds

### Resource Management

- Health system with visual feedback
- Stamina for defensive actions
- Score system with rewards at threshold values
- Items and power-ups found in containers

### Visual Effects

- Animation system for smooth character movements
- Particle effects for impacts and special moves
- Environmental visuals unique to each time period

## Controls

- **A/D**: Move left/right
- **W**: Jump
- **Space**: Attack (also used for burst fire in Future Rome)
- **M**: Defend (uses stamina)
- **Enter**: Super move (Future Rome only)
- **G**: Toggle grid display (debug)
- **B**: Toggle bounding boxes (debug)

## Level Design

Each level features:
- Unique backgrounds representing different time periods
- Platforms and obstacles requiring precise jumps
- Enemy placements for strategic combat
- Items and power-ups in chests and containers
- Story progression through dialogue and boss encounters

## Technical Implementation

### Physics System

- Gravity-based movement
- Platform collision detection
- Momentum and velocity calculations

### Collision System

- Bounding box collision detection
- Different collision responses based on entity types
- Damage calculation based on attack types

### Spawning System

- Dynamic enemy spawning at designated points
- Item generation from destroyed containers
- Visual effects for enemy deaths and item appearance

### Game State Management

- Scene transitions between different game states
- Save/load functionality for level progress
- Score tracking across levels

## Development Guidelines

- Use the entity-component system for new game elements
- Follow the established dialogue format for story additions
- Maintain consistent physics parameters for predictable gameplay
- Test thoroughly across all game levels and scenes

## Future Enhancements

- Additional time periods and alternate timelines
- More enemy types with unique behaviors
- Extended dialogue and story branches
