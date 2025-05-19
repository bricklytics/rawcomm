//
// Created by julio-martins on 5/19/25.
//

#ifndef GRIDUTILS_H
#define GRIDUTILS_H
const int GRID_SIZE = 8;
const int LOG_SIZE = 10;

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
