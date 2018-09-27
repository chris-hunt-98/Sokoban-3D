#ifndef COMMON_H
#define COMMON_H

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wswitch-default"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#pragma GCC diagnostic pop

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include <deque>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <iostream>

class GameObject;
class Block;

struct Point {
    int x;
    int y;
};

bool operator==(const Point& a, const Point& b);

std::ostream& operator<<(std::ostream& os, const Point& p);

struct PointHash {
    std::size_t operator()(const Point& p) const;
};

typedef std::unordered_set<Point, PointHash> PointSet;
typedef std::vector<Point> PointVec;
typedef std::vector<GameObject*> ObjVec;
typedef std::unordered_set<GameObject*> ObjSet;
typedef std::unordered_set<Block*> BlockSet;
typedef std::unordered_map<Point, GameObject*, PointHash> PosIdMap;
//typedef std::pair<Point, GameObject*> PosId;
//typedef std::vector<PosId> PosIdVec;

//NOTE: all enums here should be explicitly numbered, because they are largely
// used in serializing maps.  Maps should never go "out of date", unless a
// feature absolutely has to be removed from the system (even then, the
// numbering of the items around it should stay the same).

enum class Layer {
    Floor = 1,
    Player = 2,
    Solid = 3,
};

enum class RidingState {
    Free,
    Bound,
    Riding,
};

enum class ObjCode {
    NONE = 0,
    Wall = 1,
    PushBlock = 2,
    SnakeBlock = 3,
    Door = 4,
    Player = 5,
    PlayerWall = 6,
};

enum class CameraCode {
    NONE = 0,
    Free = 1,
    Fixed = 2,
    Clamped = 3,
    Null = 4,
};

struct ObjCodeHash {
    std::size_t operator()(const ObjCode& c) const;
};

struct CameraCodeHash {
    std::size_t operator()(const CameraCode& c) const;
};

const glm::vec4 GREEN = glm::vec4(0.6f, 0.9f, 0.7f, 1.0f);
const glm::vec4 PINK = glm::vec4(0.9f, 0.6f, 0.7f, 1.0f);
const glm::vec4 PURPLE = glm::vec4(0.7f, 0.5f, 0.9f, 1.0f);
const glm::vec4 DARK_PURPLE = glm::vec4(0.3f, 0.2f, 0.6f, 1.0f);
const glm::vec4 BLUE = glm::vec4(0.0f, 0.3f, 0.8f, 1.0f);
const glm::vec4 RED = glm::vec4(1.0f, 0.5f, 0.5f, 1.0f);
const glm::vec4 DARK_RED = glm::vec4(0.6f, 0.0f, 0.1f, 1.0f);
const glm::vec4 BLACK = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
const glm::vec4 LIGHT_GREY = glm::vec4(0.7f, 0.7f, 0.7f, 1.0f);
const glm::vec4 ORANGE = glm::vec4(1.0f, 0.7f, 0.3f, 1.0f);
const glm::vec4 YELLOW = glm::vec4(0.7f, 0.7f, 0.3f, 1.0f);

// NOTE: the order matters here, for serialization reasons!
const Point DIRECTIONS[4] = {Point{-1,0}, Point{0,-1}, Point{1,0}, Point{0,1}};

#define SOKOBAN_LARGE_WINDOW
#ifdef SOKOBAN_LARGE_WINDOW
const int SCREEN_WIDTH = 1200;
const int SCREEN_HEIGHT = 900;
#else
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
#endif
const int MESH_SIZE = 50;

const float ORTHO_WIDTH = (float)SCREEN_WIDTH/(float)MESH_SIZE;
const float ORTHO_HEIGHT = (float)SCREEN_HEIGHT/(float)MESH_SIZE;

const int DEFAULT_BOARD_WIDTH = 17;
const int DEFAULT_BOARD_HEIGHT = 13;

const int MAX_COOLDOWN = 5;

const int MAX_UNDO_DEPTH = 1000;

const float DEFAULT_CAM_RADIUS = 16.0;

const int FAST_MAP_MOVE = 10;

enum State {
    SmallDims = 1, // Gets width and height as 1 byte integers
    PlayerDefault = 2, // Mark the position to start the player at when loading from this map (only useful for testing, or a select few rooms)
    Objects = 3, // Read in all map objects
    CameraRect = 4, // Get a camera context rectangle
    SnakeLink = 5, // Link two snakes (1 = Right, 2 = Down)
    DoorDest = 6, // Give a door a destination Map + Pos
    BlockedDoor = 7, // Coords of blocked doors in map; the player can't come here
    End = 255,
};

const std::unordered_map<int, Point> MOVEMENT_KEYS {
    {GLFW_KEY_RIGHT, Point {1, 0}},
    {GLFW_KEY_LEFT,  Point {-1,0}},
    {GLFW_KEY_DOWN,  Point {0, 1}},
    {GLFW_KEY_UP,    Point {0,-1}},
};

#endif // COMMON_H
