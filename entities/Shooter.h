#include <SFML/Graphics.hpp>
#include <bits/stdc++.h>


const int RELOAD_POSITION_X = 365;


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
    int speed = 5;
	bool done = false;

	Shooter(int ammo_capacity_, int fire_cooldown_, int reload_time, int pos_y_, int pos_x_, int speed_ = 5)
        :ammo_capacity(ammo_capacity_),
        ammo(ammo_capacity_),
        fire_cooldown(fire_cooldown_),
        position(pos_x_, pos_y_),
        initial_pos_x(pos_x_),
        speed(speed_)
    {}

    // Shooter methods
    Bullet shoot();
	void passBridge(bool left_direction);
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

void Shooter::reload() {
    usleep(16000);
    ammo = ammo_capacity;
}
