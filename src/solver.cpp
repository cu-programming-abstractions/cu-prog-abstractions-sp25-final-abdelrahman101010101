/**
 * Dungeon Pathfinder - BFS Solver
 * 
 * This file implements BFS pathfinding algorithms for dungeon navigation.
 */

#include "solver.h"
#include "cell.h"
#include <vector>
#include <algorithm>
#include <string>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <iostream>

using namespace std;

/**
 * Helper function: Find a specific character in the dungeon
 * Returns Cell(-1, -1) if not found
 */
Cell findPosition(const vector<string>& dungeon, char target) {
    for (int row = 0; row < dungeon.size(); row++) {
        for (int col = 0; col < dungeon[row].size(); col++) {
            if (dungeon[row][col] == target) {
                return Cell(row, col);
            }
        }
    }
    return Cell(-1, -1);  // Not found
}

/**
 * Helper function: Check if a position is passable for basic BFS
 * (not a wall or door, and within bounds)
 */
bool isPassable(const vector<string>& dungeon, int row, int col) {
    if (row < 0 || row >= dungeon.size() || col < 0 || col >= dungeon[0].size()) {
        return false;  // Out of bounds
    }
    
    char cell = dungeon[row][col];
    
    // Walls are impassable
    if (cell == '#') return false;
    
    // Doors are impassable for basic BFS (key-door BFS handles doors separately)
    // Note: 'E' (exit) is not a door, so exclude it from door check
    if (cell >= 'A' && cell <= 'F' && cell != 'E') return false;
    
    // Everything else (spaces, S, E, keys a-f) is passable
    return true;
}

/**
 * Helper function: Check if we can pass through a door
 * Door 'A' requires key 'a', door 'B' requires key 'b', etc.
 */
bool canPassDoor(char door, int keyMask) {
    if (door < 'A' || door > 'F') return true;  // Not a door
    
    char requiredKey = door - 'A' + 'a';  // Convert 'A' to 'a', 'B' to 'b', etc.
    int keyBit = requiredKey - 'a';       // Convert to bit position (0-5)
    
    return (keyMask >> keyBit) & 1;       // Check if that bit is set
}

/**
 * Helper function: Collect a key by setting the appropriate bit
 */
int collectKey(char key, int keyMask) {
    if (key < 'a' || key > 'f') return keyMask;  // Not a key
    
    int keyBit = key - 'a';              // Convert to bit position (0-5)
    return keyMask | (1 << keyBit);      // Set that bit
}

/**
 * Helper function: Reconstruct path from parent pointers
 */
vector<Cell> reconstructPath(const unordered_map<Cell, Cell, CellHash>& parents, 
                            const Cell& start, const Cell& goal) {
    vector<Cell> path;
    Cell current = goal;
    
    // Follow parent pointers back to start
    while (!(current.r == start.r && current.c == start.c)) {
        path.push_back(current);
        auto it = parents.find(current);
        if (it == parents.end()) {
            return vector<Cell>(); // Path reconstruction failed
        }
        current = it->second;
    }
    path.push_back(start);
    
    reverse(path.begin(), path.end());
    return path;
}

/**
 * Helper function: Check if a position is within bounds and not a wall
 * (used by key-door BFS which handles doors separately)
 */
bool isValidPosition(const vector<string>& dungeon, int row, int col) {
    if (row < 0 || row >= dungeon.size() || col < 0 || col >= dungeon[0].size()) {
        return false;  // Out of bounds
    }
    
    // Only walls are impassable for key-door BFS (doors handled separately)
    return dungeon[row][col] != '#';
}

/**
 * Helper function: Get all valid neighboring cells for basic BFS
 */
vector<Cell> getNeighbors(const vector<string>& dungeon, const Cell& current) {
    vector<Cell> neighbors;
    
    for (int i = 0; i < NUM_DIRECTIONS; i++) {
        int newRow = current.r + DIRECTIONS[i][0];
        int newCol = current.c + DIRECTIONS[i][1];
        
        if (isPassable(dungeon, newRow, newCol)) {
            neighbors.push_back(Cell(newRow, newCol));
        }
    }
    
    return neighbors;
}

std::vector<Cell> bfsPath(const std::vector<std::string>& dungeon) {
    Cell start = findPosition(dungeon, 'S');
    Cell exit = findPosition(dungeon, 'E');

    if (start.r == -1 || exit.r == -1) {
        return vector<Cell>();
    }

    queue<Cell> bfsQueue;
    unordered_set<Cell, CellHash> visited;
    unordered_map<Cell, Cell, CellHash> parents;

    bfsQueue.push(start);
    visited.insert(start);

    while (!bfsQueue.empty()) {
        Cell current = bfsQueue.front();
        bfsQueue.pop();

        if (current == exit) {
            return reconstructPath(parents, start, exit);
        }

       vector<Cell> neighbors = getNeighbors(dungeon, current);
for (const Cell& neighbor : neighbors) {
    if (visited.find(neighbor) == visited.end()) {
        bfsQueue.push(neighbor);
        visited.insert(neighbor);
        parents[neighbor] = current;
    }
}

    return vector<Cell>();
}

/**
 * State structure for key-door BFS that includes position and collected keys.
 * 
 * CONCEPT: In basic BFS, state = (row, col). In key-door BFS, state = (row, col, keys).
 * The same position with different keys represents different states with different possibilities.
 * 
 * See BITMASK_BFS_GUIDE.md for detailed explanation of state augmentation concepts.
 */
struct KeyState {
    int row, col;    // Position (same as basic BFS)
    int keyMask;     // Bitmask representing which keys we have
    
    // Default constructor (needed for unordered_map)
    KeyState() : row(0), col(0), keyMask(0) {}
    
    // Parameterized constructor
    KeyState(int r, int c, int keys) : row(r), col(c), keyMask(keys) {}
    
    bool operator==(const KeyState& other) const {
        return row == other.row && col == other.col && keyMask == other.keyMask;
    }
};

/**
 * Hash function for KeyState to use in unordered containers.
 */
struct KeyStateHash {
    size_t operator()(const KeyState& state) const {
        // Combine position and key mask into a single hash
        return (static_cast<size_t>(state.keyMask) << 16) | 
               (static_cast<size_t>(state.row) << 8) | 
               static_cast<size_t>(state.col);
    }
};

/**
 * BITMASK OVERVIEW:
 * A bitmask efficiently stores which keys we have using a single integer.
 * Each bit represents one key: bit 0 = key 'a', bit 1 = key 'b', etc.
 * 
 * Examples: keyMask=0 (no keys), keyMask=1 (key 'a'), keyMask=3 (keys 'a' and 'b')
 * 
 * For detailed explanation and practice exercises, see BITMASK_BFS_GUIDE.md
 */

/**
 * Helper function to convert a keyMask to a readable string for debugging.
 */
string keyMaskToString(int keyMask) {
    string result = "Keys: ";
    bool hasAny = false;
    for (int i = 0; i < 6; i++) {  // Check keys 'a' through 'f'
        if ((keyMask >> i) & 1) {  // If bit i is set
            result += char('a' + i);
            result += " ";
            hasAny = true;
        }
    }
    if (!hasAny) result += "none";
    return result;
}

/**
 * Helper function to reconstruct path from KeyState parents.
 */
vector<Cell> reconstructKeyPath(const unordered_map<KeyState, KeyState, KeyStateHash>& parents,
                               const KeyState& start, const KeyState& goal) {
    vector<Cell> path;
    KeyState current = goal;
    
    // Follow parent pointers back to start
    while (!(current.row == start.row && current.col == start.col && current.keyMask == start.keyMask)) {
        path.push_back(Cell(current.row, current.col));
        auto it = parents.find(current);
        if (it == parents.end()) {
            return vector<Cell>(); // Path reconstruction failed
        }
        current = it->second;
    }
    path.push_back(Cell(start.row, start.col));
    
    reverse(path.begin(), path.end());
    return path;
}

std::vector<Cell> bfsPathKeys(const std::vector<std::string>& dungeon) {
    // This function demonstrates Bitmask BFS - an advanced algorithm that extends basic BFS
    // to handle complex state spaces where "how you got somewhere" affects "where you can go next".
    // 
    // For comprehensive explanation of concepts and algorithms, see: BITMASK_BFS_GUIDE.md
    
    // ==================== LEARNING SECTION (PROVIDED) ====================
    
    Cell start = findPosition(dungeon, 'S');
    Cell exit = findPosition(dungeon, 'E');
    
    if (start.r == -1 || exit.r == -1) {
        return vector<Cell>();
    }
    
    // Initialize BFS data structures with KeyState instead of simple Cell
    queue<KeyState> bfsQueue;
    unordered_set<KeyState, KeyStateHash> visited;
    unordered_map<KeyState, KeyState, KeyStateHash> parents;
    
    // Start with position + no keys (keyMask = 0)
    KeyState startState(start.r, start.c, 0);
    bfsQueue.push(startState);
    visited.insert(startState);
    
    cout << "Starting key-door BFS from (" << start.r << "," << start.c << ")" << endl;
    
    // Main BFS loop with key-door logic
    while (!bfsQueue.empty()) {
        KeyState current = bfsQueue.front();
        bfsQueue.pop();
        
        if (current.row == exit.r && current.col == exit.c) {
            cout << "Exit found!" << endl;
            return reconstructKeyPath(parents, startState, current);
        }
        
        // Explore all 4 neighboring directions
        for (int i = 0; i < NUM_DIRECTIONS; i++) {
            int newRow = current.row + DIRECTIONS[i][0];
            int newCol = current.col + DIRECTIONS[i][1];
            
            if (!isValidPosition(dungeon, newRow, newCol)) {
                continue;
            }
            
     char cellChar = dungeon[newRow][newCol];


if (cellChar >= 'A' && cellChar <= 'F') {  
    if (!canPassDoor(cellChar, current.keyMask)) {
        continue;
    }
}
            
            // TODO #2: Implement key collection logic  
            // HINT: Use the collectKey() helper function
            // Start with current keyMask, then update it if we step on a key ('a' to 'f')
            
            // YOUR CODE HERE: Key collection logic
       int newKeyMask = current.keyMask;
if (cellChar >= 'a' && cellChar <= 'f') {
    newKeyMask = collectKey(cellChar, current.keyMask);
}
            
            
            // TODO #3: Create new state with updated key information
            // HINT: Use KeyState constructor with (newRow, newCol, newKeyMask)
            
            // YOUR CODE HERE: State creation
            KeyState newState(newRow, newCol, newKeyMask);
            
            if (visited.find(newState) == visited.end()) {
                bfsQueue.push(newState);
                visited.insert(newState);
                parents[newState] = current;
            }
        }
    }
    
    return vector<Cell>();
    
    // ==================== END LEARNING SECTION ====================
    
    // KEY INSIGHTS:
    // 1. State = (position, keys) instead of just position
    // 2. Same position with different keys = different states
    // 3. Memory usage: O(rows × cols × 2^numKeys) vs basic BFS O(rows × cols)
    // 
    // For implementation tips, exercises, and real-world applications:
    // See BITMASK_BFS_GUIDE.md
}

#ifdef IMPLEMENT_OPTIONAL_FUNCTIONS
/**
 * OPTIONAL INTERMEDIATE CHALLENGE:
 * Practice bitmask operations by counting reachable keys (ignoring doors).
 * This is simpler than full key-door BFS and good preparation for bitmask concepts.
 * 
 * For bitmask tutorials and practice exercises, see BITMASK_BFS_GUIDE.md
 */
int countReachableKeys(const std::vector<std::string>& dungeon) {
    // OPTIONAL CHALLENGE: Practice bitmask operations in a simpler context
    // This function uses basic BFS but practices key collection with bitmasks
    // Good preparation for understanding the full key-door BFS complexity
    
    Cell start = findPosition(dungeon, 'S');
    if (start.r == -1) return 0;
    
    
    queue<Cell> bfsQueue;
    unordered_set<Cell, CellHash> visited;
    int keyMask = 0;

    bfsQueue.push(start);
    visited.insert(start);

    
    while (!bfsQueue.empty()) {
        Cell current = bfsQueue.front();
        bfsQueue.pop();

        
        char cellChar = dungeon[current.r][current.c];
        if (cellChar >= 'a' && cellChar <= 'f') {
            keyMask = collectKey(cellChar, keyMask);
        }

        
        for (int i = 0; i < NUM_DIRECTIONS; i++) {
            int newRow = current.r + DIRECTIONS[i][0];
            int newCol = current.c + DIRECTIONS[i][1];

            if (isValidPosition(dungeon, newRow, newCol) && visited.find(Cell(newRow, newCol)) == visited.end()) {
                visited.insert(Cell(newRow, newCol));
                bfsQueue.push(Cell(newRow, newCol));
            }
        }
    }

    // Count the number of keys collected by checking the bits in keyMask
    int count = 0;
    for (int i = 0; i < 6; i++) {
        if ((keyMask >> i) & 1) {
            count++;  // Increment if the i-th key is collected
        }
    }

    return count;  // Return the number of collected keys
}
#else
/**
 * Stub implementation when optional functions are disabled.
 * This prevents compilation errors when the function is not implemented.
 */
int countReachableKeys(const std::vector<std::string>& dungeon) {
    // Optional function not implemented - returning 0
    return 0;
}
#endif
