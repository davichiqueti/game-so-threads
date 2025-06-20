#include <SFML/Graphics.hpp>
#include <bits/stdc++.h>


const int RELOAD_POSITION_X = 375;


struct Bullet {
    bool active = true;
    int speed = 5;
    sf::Vector2f position;
};

class Shooter {
public:
    int ammo_capacity;
	int ammo;
    int fire_cooldown;
    int reload_time;
    sf::Vector2f position;
    int initial_pos_x;
    float speed;
	bool done = false;

	Shooter(int ammo_capacity_, int fire_cooldown_, int reload_time, int speed_, int pos_y_, int pos_x_)
        :ammo_capacity(ammo_capacity_),
        ammo(ammo_capacity_),
        fire_cooldown(fire_cooldown_),
        speed(speed_),
        position(pos_x_, pos_y_),
        initial_pos_x(pos_x_)
    {}

    // Shooter methods
    Bullet shoot();
	void passBridge(bool left_direction);
    void move(int target_position_x);
    void reload();

};

Bullet Shooter::shoot() {
    ammo--;
    Bullet bullet;
    bullet.position = this->position;
    bullet.position.x += 38;
    // Pushing new bullet to the bullets array
    return bullet;
};

void Shooter::passBridge(bool left_direction) {
    // Locking bridge to other shooter dont pass
    if (left_direction) {
        while (position.x > RELOAD_POSITION_X) {
            this->position.x -= speed;
            usleep(16000);
        }
        this->position.x = RELOAD_POSITION_X;
    } else {
        while (this->position.x < initial_pos_x) {
            this->position.x += speed;
            usleep(16000);
        }
        position.x = initial_pos_x;
    }
}

void Shooter::move(int target_position_x) {
    // Locking bridge to other shooter dont pass
    if (target_position_x < this->position.x) {
        while (position.x > target_position_x) {
            this->position.x -= speed;
            usleep(16000);
        }
        this->position.x = target_position_x;
    } else {
        while (target_position_x > this->position.x) {
            this->position.x += speed;
            usleep(16000);
        }
        this->position.x = initial_pos_x;
    }
}

void Shooter::reload() {
    usleep(fire_cooldown);
    ammo = ammo_capacity;
}
