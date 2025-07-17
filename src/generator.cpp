/**
 * Dungeon Pathfinder - Maze Generation
 * 
 * This file implements recursive backtracking maze generation.
 * The algorithm creates a "perfect maze" - exactly one path between any two points.
 */

#include "generator.h"
#include <random>
#include <algorithm>
#include <iostream>

using namespace std;

// Directions for maze carving: North, East, South, West (2-step moves)
const int CARVE_DIRECTIONS[4][2] = {{-2, 0}, {0, 2}, {2, 0}, {0, -2}};

// Random number generator (better than rand())
static mt19937 rng(random_device{}());

/**
 * Helper function: Check if a coordinate is within bounds
 */
bool isInBounds(int row, int col, int rows, int cols) {
    return row >= 0 && row < rows && col >= 0 && col < cols;
}

/**
 * Helper function: Check if a cell has been visited (carved)
 * In our representation, carved cells contain ' ' (space)
 */
bool isCarved(const vector<string>& maze, int row, int col) {
    return maze[row][col] == ' ';
}

/**
 * Helper function: Carve a passage between two cells
 * This removes the wall between start and end positions
 */
void carvePassage(vector<string>& maze, int startRow, int startCol, int endRow, int endCol) {
    // Carve the destination cell
    maze[endRow][endCol] = ' ';
    
    // Carve the wall between start and end (the cell in the middle)
    int wallRow = (startRow + endRow) / 2;
    int wallCol = (startCol + endCol) / 2;
    maze[wallRow][wallCol] = ' ';
}

/**
 * Helper function: Get all valid unvisited neighbors
 * Returns list of cells that can be carved to (2 steps away and not yet carved)
 */
vector<pair<int, int>> getUnvisitedNeighbors(const vector<string>& maze, int row, int col) {
    vector<pair<int, int>> neighbors;
    int rows = maze.size();
    int cols = maze[0].size();
    
    // Check all 4 directions (2 steps away)
    for (int i = 0; i < 4; i++) {
        int newRow = row + CARVE_DIRECTIONS[i][0];
        int newCol = col + CARVE_DIRECTIONS[i][1];
        
        if (isInBounds(newRow, newCol, rows, cols) && !isCarved(maze, newRow, newCol)) {
            neighbors.push_back({newRow, newCol});
        }
    }
    
    return neighbors;
}

/**
 * Helper function: Shuffle a vector randomly
 */
template<typename T>
void shuffleVector(vector<T>& vec) {
    shuffle(vec.begin(), vec.end(), rng);
}

void recursiveBacktrack(vector<string>& maze, int row, int col) {
    maze[row][col] = ' ';
    auto neighbors = getUnvisitedNeighbors(maze, row, col);
    shuffleVector(neighbors);
    for (auto& nb : neighbors) {
        int nRow = nb.first;
        int nCol = nb.second;
        if (!isCarved(maze, nRow, nCol)) {
            carvePassage(maze, row, col, nRow, nCol);
            recursiveBacktrack(maze, nRow, nCol);
        }
    }
}

/**
 * Helper function: Add random rooms to the maze
 * Punches holes in walls to create larger open areas
 */
void addRandomRooms(vector<string>& maze, int roomRate) {
    int rows = maze.size();
    int cols = maze[0].size();
    
    // Calculate how many rooms to add based on roomRate percentage
    int totalWalls = (rows * cols) / 4;  // Rough estimate of wall cells
    int roomsToAdd = (totalWalls * roomRate) / 100;
    
    for (int i = 0; i < roomsToAdd; i++) {
        int row = 2 + (rng() % (rows - 4));  // Avoid borders
        int col = 2 + (rng() % (cols - 4));
        
        // Only carve if it's currently a wall
        if (maze[row][col] == '#') {
            maze[row][col] = ' ';
        }
    }
}

/**
 * Helper function: Place start and exit positions
 * Finds two open cells that are far apart and places S and E
 */
void placeStartAndExit(vector<string>& maze) {
    vector<pair<int, int>> openCells;
    int rows = maze.size();
    int cols = maze[0].size();
    
    // Find all open cells
    for (int r = 1; r < rows - 1; r++) {
        for (int c = 1; c < cols - 1; c++) {
            if (maze[r][c] == ' ') {
                openCells.push_back({r, c});
            }
        }
    }
    
    if (openCells.size() < 2) {
        cout << "Warning: Not enough open cells for start/exit placement!" << endl;
        return;
    }
    
    // Place start at first open cell
    maze[openCells[0].first][openCells[0].second] = 'S';
    
    // Place exit at last open cell (likely far from start)
    maze[openCells.back().first][openCells.back().second] = 'E';
}

std::vector<std::string> generateDungeon(int rows, int cols, int roomRate) {
    // Ensure odd dimensions for proper maze structure
    if (rows % 2 == 0) rows++;
    if (cols % 2 == 0) cols++;
    
    // Minimum size check
    if (rows < 5 || cols < 5) {
        rows = max(rows, 5);
        cols = max(cols, 5);
    }
    
    // Initialize maze with all walls
    vector<string> maze(rows, string(cols, '#'));
    
  recursiveBacktrack(maze, 1, 1);
    
    // Add random rooms (provided)
    addRandomRooms(maze, roomRate);
    
    // Place start and exit (provided)
    placeStartAndExit(maze);
    
    return maze;
    
    // DEBUGGING TIPS:
    // 1. Test with small sizes first (7x7 or 9x9)
    // 2. Print the maze to see if passages look right
    // 3. Check that most cells are reachable
    // 4. Verify start 'S' and exit 'E' are placed properly
}
