# ♠️ GuanDan    
👉 [中文版 README](./README.md)

##### This project was completed by Si-xiyu as a C++ training assignment for the Class of 2024 at South China University of Technology (SCUT). It is for academic exchange only and should not be used for academic fraud or commercial purposes.
Welcome to apply to South China University of Technology (SCUT)! OwO

#### 🃏 This is likely the most feature-complete C++ implementation of the GuanDan card game you'll find on GitHub. If you find it helpful, please leave a star ⭐!

# Features
This project implements the following features:

**Task One**

1.  Simple graphical interface and rule introduction.
2.  Random card dealing, determination of the lead player for the round, and the first player to play.
3.  Correctly validates card combinations (hands) and checks if plays are according to the rules.
4.  Basic AI opponent (simple logic: plays the smallest possible valid hand if available).

**Task Two**

1.  **Card Counter**: Displays the remaining count of each card rank and a history of cards played by each player.
2.  **Score Multiplier**: The score for the round is doubled for each Bomb or Straight Flush played.
3.  **Game Ranking**: Correctly ranks all four players at the end of a round.

**Task Three**

1.  **Settings & Persistence**: Adjustable and savable parameters like turn timer, sound volume, and mute toggle.
2.  **Smarter UI/UX**: Provides hints for valid plays that can beat the previous hand and indicates when a player has no valid plays ("Pass").
3.  **Enhanced UI/UX**: More polished graphical interface and sound effects.

# Game Rules

1.  **Singles**: The rank of cards is: Red Joker, Black Joker, Level Card, A, K, ..., 2.
2.  **Pairs**: The ranking is the same as for singles. The highest pair is a pair of Red Jokers.
3.  **Trios**: Can be played as a stand-alone trio or as a trio with a pair (like a Full House). It cannot be played with a single kicker card.
4.  **Bombs**: Four or more cards of the same rank. A bomb with more cards beats a bomb with fewer cards.
5.  **Plates (Steel Plates)**: Two consecutive trios (e.g., 555666). Unlike in Dou Dizhu, '2' can be part of a Plate (e.g., AAA222 and 222333). The highest Plate is KKKAAA.
6.  **Straights**: **Five** consecutive single cards. Unlike in Dou Dizhu, '2' can be part of a Straight (e.g., A2345 and 23456). The highest Straight is 10-J-Q-K-A. A straight with all cards of the same suit is a **Straight Flush**. A Straight Flush beats a 5-card Bomb but is beaten by a 6-card Bomb.
7.  **Consecutive Pairs**: **Three or more** consecutive pairs. Unlike in Dou Dizhu, '2' can be part of Consecutive Pairs (e.g., AA2233 and 223344). The highest Consecutive Pairs are QQKKAA.
8.  **Wild Card Rule**: The **Level Card of the Hearts suit** acts as a Wild Card. It can substitute for any card except the Jokers.
9.  **Leveling Up Rule**: If a team finishes 1st and 2nd, they level up by 3. If they finish 1st and 3rd, they level up by 2. If they finish 1st and 4th, they level up by 1. The last player to finish gives their highest card as a "tribute" to the first-place player. The first-place player then returns a card ranked 10 or lower. No tribute is required if the losing player holds both Jokers or has no cards higher than an Ace.

### Tribute and Playing Order

These rules apply from the second round onwards, based on the rankings of the previous round.

1.  **Single Loser Scenario**: If the last-place player's team did not secure both last-place spots, the last-place player must pay tribute (their highest card) to the first-place player.

2.  **Double Loser Scenario**: If the last two players (3rd and 4th place) are on the same team (a "double-down"), both must pay tribute (their highest cards) to the two players on the winning team, respectively.

3.  **Tribute Exemption (Anti-Tribute)**: A player can refuse to pay tribute if they hold both Red Jokers. In a "double-down" scenario, the tribute is canceled if each of the losing players has one Red Joker, or if one of them has both.

### Tribute Card Selection

-   The tribute must be the player's highest-ranking single card, excluding the Heart Level Card (Wild Card).
-   If the player has no Jokers, they pay their highest-ranking single card.

### Returning Tribute Rule

-   The player receiving the tribute must return a card from their hand to the tribute payer.
-   If returning to a teammate, the card must be ranked 10 or lower.
-   If returning to an opponent, any card can be returned.

### Playing Order

-   **Single Loser Scenario**: The tribute payer plays first.
-   **Double Loser Scenario**: The player who paid the higher-ranking tribute card plays first. If the tribute cards are of the same rank, the turn order proceeds clockwise.
-   **Tribute Exemption**: The first-place player from the previous round plays first.

# Software Architecture
This project uses the **MVC (Model-View-Controller)** architecture, with the `GD_Controller` class managing the main game flow.
**Model** includes data classes like `Card`, `Team`, and `Player`, as well as utility classes like `CardCombo`.
**View** includes classes like `CardWidget`, `PlayerAreaWidget`, `LeftWidget`, and `GuanDan`, which are responsible for the UI logic.

### Main Flowcharts
![游戏核心状态机图](./游戏核心状态机图.png)

![玩家出牌交互时序图](./玩家出牌交互时序图.png)

### Detailed Class Descriptions

#### I. Model - Game Data & Core Rules

This is the backend of the game, defining all objects and rules.

-   `Card.h/.cpp`:
    -   **Purpose**: Defines the most basic unit of the game—a single **card**.
    -   **Core Logic**: Contains the card's point (`m_point`) and suit (`m_suit`). The key method is `getComparisonValue()`, which calculates a card's actual rank based on the current "Level Card" of the game. `isWildCard()` determines if a card is a Wild Card (Heart Level Card).

-   `CardDeck.h/.cpp`:
    -   **Purpose**: Represents one or more decks of cards.
    -   **Core Logic**: `initializeDecks()` creates two full decks of cards (108 total). `shuffle()` randomizes the deck to ensure fair dealing.

-   `Player.h/.cpp`:
    -   **Purpose**: Defines a generic **player** object.
    -   **Core Logic**: Stores player information (name, ID, team) and manages their hand (`m_handCards`). It defines a crucial virtual function, `autoPlay()`, which provides a base for polymorphic behavior between human and AI players.

-   `NPCPlayer.h/.cpp`:
    -   **Purpose**: Inherits from `Player` and implements the **AI Player**.
    -   **Core Logic**: `getBestPlay()` is the heart of its AI. It uses a series of `find...` helper functions to identify all possible plays and then selects the best one based on a predefined strategy (e.g., play small cards first, conserve bombs).

-   `Team.h/.cpp`:
    -   **Purpose**: Defines the concept of a **team**.
    -   **Core Logic**: Groups two players into a team and stores shared state, such as the current level rank (`m_currentLevelRank`) and total score (`m_score`).

-   `CardCombo.h/.cpp`:
    -   **Purpose**: A critical **static utility class** that acts as one of the game's "rules engines."
    -   **Core Logic**: `evaluateConcreteCombo()` determines the type of a given hand (single, pair, straight, etc.) that does not contain wild cards. Its most complex function, `getAllPossibleValidPlays()`, uses recursion (`findCombinationsWithWildsRecursive`) to explore all valid combinations when a player's selection includes Wild Cards.

-   `LevelStatus.h/.cpp`:
    -   **Purpose**: Manages the game's **leveling system and win/loss progression**.
    -   **Core Logic**: Tracks and updates the level card for both teams. The `updateLevelsAfterRound()` method determines which team levels up and by how much, based on the round's outcome. It also handles special rules and determines if the entire game is over.

#### II. Controller - The Brain of the Game

-   `GD_Controller.h/.cpp`:
    -   **Purpose**: The project's **main controller**, orchestrating all game logic.
    -   **Core Logic**:
        1.  **State Machine**: Uses a `GamePhase` enum and the `enterState()` method to manage different game stages (Dealing, Tribute, Playing, Round End), ensuring a robust game flow.
        2.  **Event Handling**: Receives player actions from the View via slot functions like `onPlayerPlay` and `onPlayerPass`.
        3.  **Logic Dispatch**: Calls Model classes (e.g., `Player`, `CardCombo`) to handle business logic, such as validating plays, removing cards from a player's hand, and updating the table.
        4.  **View Updates**: Emits signals (e.g., `sigUpdatePlayerHand`, `sigUpdateTableCards`) to notify the View to refresh after the model's state changes.
        5.  **Flow Control**: Manages player turn rotation (`advanceToNextPlayer`), timers, and the complex tribute/return-tribute process.

#### III. View - The "Face" of the Game

This part handles all user-facing UI elements and interactions.

-   `GuanDan.h/.cpp`:
    -   **Purpose**: The **main window (QMainWindow)** that contains all other UI components.
    -   **Core Logic**: Initializes the overall UI layout, creates the `GD_Controller` and all player area widgets (`PlayerAreaWidget`), and connects controller signals to the appropriate UI update slots via `setupConnections()`.

-   `PlayerAreaWidget.h/.cpp`:
    -   **Purpose**: A **composite widget** representing a complete player area.
    -   **Core Logic**: Contains a `PlayerWidget` (for hand and info) and a `ShowCardWidget` (for played cards). It arranges these sub-widgets based on the player's position (top, bottom, left, right).

-   `PlayerWidget.h/.cpp`:
    -   **Purpose**: Displays a **player's hand and information**, one of the most complex View components.
    -   **Core Logic**: Dynamically and aesthetically renders all of a player's hand cards (`CardWidget`), handling stacking, fanning, and rotation. It also processes user interactions like clicking and selecting cards, emitting signals in response.

-   `CardWidget.h/.cpp`:
    -   **Purpose**: The view for a **single playing card**.
    -   **Core Logic**: Loads and displays the appropriate card face or back image based on a `Card` object's data. It handles its own visual effects like selection and hover highlights and responds to click events.

-   `ShowCardWidget.h/.cpp`:
    -   **Purpose**: Displays the **cards just played** in each player's area.
    -   **Core Logic**: Receives a `CardCombo::ComboInfo` object and displays the cards within it clearly.

-   `LeftWidget.h/.cpp`:
    -   **Purpose**: The **information panel on the left side** of the main window.
    -   **Core Logic**: A composite widget that integrates a `CardCounterWidget`, `LevelIndicatorWidget`, scoreboard, ranking list, and current turn info, providing a centralized display of all important global game state.

-   `CardCounterWidget.h/.cpp`:
    -   **Purpose**: The implementation of the **Card Counter**.
    -   **Core Logic**: Displays the remaining quantity of each card rank (Jokers, A, K...2) in a list format.

-   `LevelIndicatorWidget.h/.cpp`:
    -   **Purpose**: A compact **Level Card display**.
    -   **Core Logic**: Visually indicates the current level card for both teams using two card images.

-   **Various Dialogs (`...Dialog.h/.cpp`)**:
    -   `WildCardDialog`: Appears when a play with Wild Cards has multiple interpretations, allowing the player to **choose the intended hand type**.
    -   `TributeDialog`: Appears during the tribute/return phase, allowing the player to **select a card to give**.
    -   `SettingsDialog`: The **settings window**, allowing the player to adjust volume, turn timers, etc.
    -   `RulesDialog`: A **rules window** that explains the game to the player.

#### IV. Utility/Helper Classes

These classes provide standalone functionality used by other parts of the project.

-   `SoundManager.h/.cpp`:
    -   **Purpose**: A singleton **sound manager**.
    -   **Core Logic**: Manages the playback of background music (BGM) and various sound effects (playing cards, clicks). Provides global volume control.

-   `SettingsManager.h/.cpp`:
    -   **Purpose**: A static **settings manager utility**.
    -   **Core Logic**: Saves and loads game settings (like volume) to and from a local configuration file (`.ini`), enabling settings persistence.

# Development Environment
This project is built with **Qt 5.12.5** and the **MSVC** compiler, developed using **Visual Studio 2022** with the Qt VS Tools extension.

# Highlights
-   **Heuristic-based AI** for card playing logic, which avoids the high memory consumption of exhaustive search algorithms.
-   Clean **MVC architecture** ensures low coupling between modules, making it easy to extend and maintain.

# Known Issues
-   There are some logic issues in the view for displaying hand cards.
-   The `NPCPlayer` class does not correctly incorporate the current level card when evaluating sequential hands (Straights, Plates, Consecutive Pairs). This affects both the hint system and the AI's playing ability.
-   The layout of the `LeftWidget` (the left-side info panel) could be improved. Logic for multiplayer (`HMPlayer`) is not fully implemented.
-   A database interface has not been integrated into the main game flow (e.g., for user profiles or game history).
