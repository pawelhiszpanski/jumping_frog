# Terminal Frogger: Console Survival Game (C / PDCurses)

A text-based arcade game built in C using the PDCurses library. This project recreates the classic Frogger experience directly in the terminal, featuring dynamic difficulty progression, time-based events, and custom file-based configuration.

## Tech Stack
* **Language:** C
* **Graphics Library:** PDCurses (Terminal UI)
* **Core Libraries:** `<time.h>`, `<stdlib.h>`, `<string.h>`

## Key Engineering Features
* **Custom Configuration Engine:** Loads game parameters (character shapes, entity counts, screen width) at runtime from a `frogger.txt` file using string parsing (`strncmp`, `fgets`).
* **Dynamic Memory Management:** Utilizes dynamic memory allocation (`malloc`/`free`) to generate variable amounts of cars and obstacles based on the parsed configuration.
* **Time-Based Event System:** Uses `clock()` to implement a non-blocking game loop, managing delta-time for smooth input handling (100ms move delays) and delayed entity spawning.
* **Advanced Collision & Trajectory Detection:** Implements both exact-coordinate matching (for static obstacles) and continuous trajectory analysis for moving vehicles to ensure accurate hit detection.

## Gameplay Mechanics
* **Progressive Difficulty (3 Levels):**
  * **Level 1:** Introduces "friendly" cars. Colliding with them triggers a quick-time event allowing a special 3-tile forward jump (pressing 'j').
  * **Level 2:** Friendly cars spawn rate is reduced. Static obstacles are procedurally generated across safe zones.
  * **Level 3:** Hardcore mode. All vehicles are hostile and deadly.
* **Anti-Camper Mechanic (The Stork):** On Level 3, remaining idle for more than 10 seconds spawns a Stork entity. It utilizes a basic pathfinding algorithm to actively track and hunt the player.
* **Dynamic Scoring:** Calculates the final score by taking base level points and subtracting penalties based on total elapsed time and the number of jumps taken.

## Requirements & Setup
1. A C compiler (GCC, MSVC, etc.).
2. The **PDCurses** library installed and linked to your project.
3. A `frogger.txt` configuration file placed in the same directory as the executable.
4. Run the application in a terminal that supports standard curses color rendering.
