
# Shape Wars 2D Game

**Shape Wars 2D** is a fast-paced action game where the player controls a randomly generated shape and fights against a variety of enemy shapes. The goal is to survive as long as possible, earn points, and achieve the highest score. The game utilizes **SFML** for graphics and input handling.


**Video Demo** [Watch the video demo](https://www.youtube.com/watch?v=sr0TglMLr-w)

## Features

- **Random Shape Generation**: The player’s shape is randomly generated, and enemy shapes appear at random locations.
- **Shooting Mechanism**: Players can fire bullets to destroy enemies, with a cooldown mechanic for regular bullets and a powerful supermove that fires bullets in all directions.
- **Survival Gameplay**: The player earns points by surviving and defeating enemies. The longer the player survives, the higher the score.
- **Enemy AI**: Enemies spawn in waves, move towards the player, and rotate while trying to collide with the player.
- **HUD Display**: Real-time information like score, lives remaining, and supermove availability is displayed.
- **Supermove**: A special ability that clears enemies in all directions, available after a cooldown period.

## Technologies Used

- **SFML**: Used for rendering graphics, handling window events, and managing input.
- **C++**: The core programming language for game logic and mechanics.
  
## Gameplay

- **Player Movement**: The player can move the shape using arrow keys and shoot with spacebar.
- **Enemies**: Different enemy shapes spawn randomly. The player must avoid collision while trying to destroy as many enemies as possible.
- **Lives**: The player starts with 3 lives. If a collision occurs with an enemy, the player loses a life.
- **Supermove**: Activated after a cooldown, this powerful attack allows the player to shoot bullets in all directions to clear nearby enemies.

## Installation

### Prerequisites

- **SFML**: Ensure that SFML is installed and properly linked to your project.
  
You can install SFML on macOS via Homebrew:

```bash
brew install sfml
```

### Clone the Repository

```bash
git clone https://github.com/ludovicocuoghi/CPP_2D_Shooting_Shapes.git
cd CPP_2D_Shooting_Shapes
```

### Build the Project

1. Open the terminal in the project folder.
2. Use the following command to compile and run the project:

```bash
make
./Shape_Wars_2d_project
```

## Gameplay Mechanics

- **Movement**: Use the arrow keys to move the player’s shape.
- **Shoot**: Press the spacebar to shoot bullets.
- **Supermove**: Press the supermove key (if available) to shoot in all directions.
- **Score**: The score increases as the player destroys enemies. The best score for each shape is tracked.

## Controls

- **Arrow keys**: Move the player’s shape.
- **Spacebar**: Fire a bullet.
- **Supermove (if available)**: Fires bullets in all directions.

## Game States

- **Playing**: The game is ongoing, and the player can control the character and shoot enemies.
- **Game Over**: Triggered when the player loses all lives. The game displays the final score and the best score for the player's shape.

## Contributions

Feel free to fork the repository and contribute! For contributions, open an issue or submit a pull request with your improvements.
