#include <pthread.h>
#include <iostream>
#include <SFML/Graphics.hpp>
#include <vector>
#include <bits/stdc++.h>


class Helicopter {
public:
    float speed;

    Helicopter(float startX, float startY, float movementSpeed)
    : position(startX, startY)
    , speed(movementSpeed)
    , capacity(1)
    , occupation(0)
    {}

    // Get soldier on the helicopter. Return true when a soldier is picked up
    bool boardSoldier() {
        if (occupation < capacity) {
            ++occupation;
            return true;
        }
        return false;
    }

    // Drop soldier from the helicopter. Return true when a soldier is dropped
    bool dropSoldier() {
        if (occupation > 0) {
            --occupation;
            return true;
        }
        return false;
    }

    // Setters
    void setPosition(float x, float y) { position.x = x; position.y = y; }
    // Getters
    const sf::Vector2f& getPosition() const { return position; }
    int getOccupation() const { return occupation; }
    int getCapacity() const { return capacity; }

private:
    sf::Vector2f position;
    int occupation;
    int capacity;
};
