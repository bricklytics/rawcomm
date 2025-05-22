//
// Created by julio-martins on 5/19/25.
//

#ifndef GRIDUTILS_H
#define GRIDUTILS_H

#include <ncurses.h>

constexpr int GRID_SIZE = 8;
constexpr int LOG_SIZE = 10;

enum class Direction{
    UNKNOWN,
    UP,
    DOWN,
    LEFT,
    RIGHT,
    QUIT,
};

class GridUtils {
public:
    static Direction uInt8ToDirection(const uint8_t cmd) {
        switch (cmd) {
            case 1: return Direction::UP;
            case 2: return Direction::DOWN;
            case 3: return Direction::LEFT;
            case 4: return Direction::RIGHT;
            case 5: return Direction::QUIT;
            default: return Direction::UNKNOWN;
        }
    }

    static int toInt(const uint8_t cmd) {
        int key = 0;
        switch (uInt8ToDirection(cmd)) {
            case Direction::UP: key = KEY_UP; break;
            case Direction::DOWN: key = KEY_DOWN; break;
            case Direction::LEFT: key = KEY_LEFT; break;
            case Direction::RIGHT: key = KEY_RIGHT; break;
            case Direction::QUIT: key = 'q'; break;
            default: key = 0; break;
        }
        return key;
    }

    static uint8_t toUint8(const int cmd) {
        uint8_t key = 0;
        switch (cmd) {
            case 'i':
            case KEY_UP: key = 1; break;
            case 'k':
            case KEY_DOWN: key = 2; break;
            case 'j':
            case KEY_LEFT: key = 3; break;
            case 'l':
            case KEY_RIGHT: key = 4; break;
            case 'q': key = 5; break;
            default: key = 0; break;
        }
        return key;
    }
};

struct Position {
    int x, y;

    bool operator==(const Position& other) const {
        return x == other.x && y == other.y;
    }
};

namespace std {
    template <>
    struct hash<Position> {
        size_t operator()(const Position& p) const noexcept {
            return hash<int>()(p.x) ^ (hash<int>()(p.y) << 1);
        }
    };
}

#endif //GRIDUTILS_H
