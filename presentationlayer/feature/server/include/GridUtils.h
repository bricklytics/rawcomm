//
// Created by julio-martins on 5/19/25.
//

#ifndef GRIDUTILS_H
#define GRIDUTILS_H

#include <ncurses.h>

#include "../../../../flowcontrollayer/feature/base/include/PacketType.h"

constexpr int GRID_SIZE = 8;
constexpr int LOG_SIZE = 10;

enum class Direction {
    UP,
    DOWN,
    LEFT,
    RIGHT
};

class GridUtils {
public:
    static int toInt(const uint8_t cmd) {
        int key = 0;
        switch (cmd) {
            case Direction::UP: key = KEY_UP; break;
            case Direction::DOWN: key = KEY_DOWN; break;
            case Direction::LEFT: key = KEY_LEFT; break;
            case Direction::RIGHT: key = KEY_RIGHT; break;
            default: key = 0; break;
        }
        return key;
    }
};

class Position {
public:
    int x, y;

    bool operator<(const Position &other) const {
        return (y * GRID_SIZE + x) < (other.y * GRID_SIZE + other.x);
    }

    bool operator==(const Position &other) const {
        return x == other.x && y == other.y;
    }
};

#endif //GRIDUTILS_H
