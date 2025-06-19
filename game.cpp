#include <pthread.h>
#include <iostream>
#include <SFML/Graphics.hpp>
#include <vector>
#include <bits/stdc++.h>
#include "entities/Helicopter.h"
#include "entities/Shooter.h"


// Window Config
const int WINDOW_W = 1200;
const int WINDOW_H = 600;
// Game general config
const int MS_MICROSECONDS_MULTIPLIER = 1000;
const int SHOOTER_MOVEMENT_SLEEP = 20 * MS_MICROSECONDS_MULTIPLIER;
const int SOLDIERS_NUM = 10;
const int LOWER_LIMIT = WINDOW_H - 60;
const int FRAME_DELAY_US = 16000;    // ~60 FPS
// Fixed positions
sf::Vector2f rescue_point = {100.f, WINDOW_H - 70.f};
// Mutexes used to handle critical sessions
pthread_mutex_t bridge_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t reloader_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t shooters_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t bullets_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t helicopter_mutex = PTHREAD_MUTEX_INITIALIZER;


std::vector<Bullet> bullets;

enum GameState {
    GAME_RUNNING,
    GAME_WIN,
    GAME_OVER_HELICOPTER_SHOOTED,
    GAME_OVER_HELICOPTER_COLLIDED,
    GAME_OVER_HELICOPTER_TOO_HIGH,
    GAME_ABORTED,
};
GameState game_state;

struct HelicopterControl {
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
};

HelicopterControl helicopter_control;

void *helicopter_func(void *arg) {
    Helicopter *helicopter = (Helicopter *)arg;
    while (game_state == GAME_RUNNING) {
        pthread_mutex_lock(&helicopter_mutex);
        sf::Vector2f pos = helicopter->getPosition();
        if (helicopter_control.up) {
            pos.y -= helicopter->speed;
        }
        if (helicopter_control.down && pos.y < LOWER_LIMIT - helicopter->speed) {
            pos.y += helicopter->speed;
        }
        if (helicopter_control.left && pos.x > helicopter->speed) {
            pos.x -= helicopter->speed;
        }
        if (helicopter_control.right && pos.x < WINDOW_W - helicopter->speed) {
            pos.x += helicopter->speed;
        }
        helicopter->setPosition(pos.x, pos.y);
        pthread_mutex_unlock(&helicopter_mutex);
        if (pos.y <= 0) {
            game_state = GAME_OVER_HELICOPTER_TOO_HIGH;
            break;
        }
        usleep(FRAME_DELAY_US); // ~60 FPS
    }
    return nullptr;
}

void *shooter_func(void *arg) {
    Shooter *shooter = (Shooter *)arg;
    while (!shooter->done) {
        if (shooter->ammo > 0) {
            usleep(shooter->fire_cooldown);
            Bullet bullet = shooter->shoot();
            // Pushing new bullet to the bullets vector
            pthread_mutex_lock(&bullets_mutex);
            bullets.push_back(bullet);
            pthread_mutex_unlock(&bullets_mutex);
        }
        else {
            // Moving the shooter to the ammo deposit blocking the bridge
            pthread_mutex_lock(&bridge_mutex);
            shooter->passBridge(true);
            pthread_mutex_unlock(&bridge_mutex);
            // Locking reloader to other shooter dont pass
            pthread_mutex_lock(&reloader_mutex);
            shooter->reload();
            pthread_mutex_unlock(&reloader_mutex);
            // Moving the shooter back to initial position blocking the bridge
            pthread_mutex_lock(&bridge_mutex);
            shooter->passBridge(false);
            pthread_mutex_unlock(&bridge_mutex);
        }
    }
    return nullptr;
}


void *bullets_manager_func(void *) {
    while (game_state == GAME_RUNNING) {
        // Moving all active bullets
        pthread_mutex_lock(&bullets_mutex);
        for (int i = bullets.size() - 1; i >= 0; --i) {
            bullets[i].position.y -= bullets[i].speed;
            if (bullets[i].position.y <= 0) {
                bullets.erase(bullets.begin() + i);
            }
        }
        pthread_mutex_unlock(&bullets_mutex);
        usleep(FRAME_DELAY_US);
    }
    return nullptr;
}

template<typename DrawableA, typename DrawableB>
bool collide(const DrawableA&  obj1, const DrawableB& obj2) {
    return obj1.getGlobalBounds().intersects(obj2.getGlobalBounds());
}

int game(int shooter_ammo_capacity, int shooter_fire_cooldown, int shooter_reload_time) {
    // Graficos
    sf::Font font;
    font.loadFromFile("assets/fonts/OpenSans-Regular.ttf");
    sf::Text statsText;
    statsText.setPosition(20, 20);
    statsText.setFont(font);
    statsText.setCharacterSize(16);
    statsText.setFillColor(sf::Color::Black);

    sf::Texture backgroundTexture;
    backgroundTexture.loadFromFile("assets/images/background.png");
    sf::Sprite backgroundSprite;
    backgroundSprite.setTexture(backgroundTexture);
    float scaleX = static_cast<float>(WINDOW_W) / backgroundTexture.getSize().x;
    float scaleY = static_cast<float>(WINDOW_H) / backgroundTexture.getSize().y;
    backgroundSprite.setScale(scaleX, scaleY);

    // Creating game entities
    std::vector<Shooter*> shooters;
    shooters.push_back(
        new Shooter(
            shooter_ammo_capacity, 
            shooter_fire_cooldown,
            shooter_reload_time,
            (WINDOW_H - 115),
            (WINDOW_W - 325)
        )
    );
    shooters.push_back(
        new Shooter(
            shooter_ammo_capacity,
            shooter_fire_cooldown,
            shooter_reload_time,
            (WINDOW_H - 115),
            (WINDOW_W - (325 * 2))
        )
    );
    Helicopter *helicopter = new Helicopter(400.f, 300.f, 20.f);

    // Creating threads
    pthread_t bullets_manager_thread;
    pthread_create(&bullets_manager_thread, nullptr, bullets_manager_func, &bullets);

    pthread_t helicopter_thread;
    pthread_create(&helicopter_thread, nullptr, helicopter_func, helicopter);

    std::vector<pthread_t> threads(shooters.size());
    for (size_t i = 0; i < shooters.size(); ++i) {
        pthread_create(&threads[i], nullptr, shooter_func, shooters[i]);
    }

    int soldiers_waiting = SOLDIERS_NUM;
    int rescued_soldiers = 0;
    sf::Texture soldier_texture;
    soldier_texture.loadFromFile("assets/images/soldier.png");

    sf::RenderWindow window(sf::VideoMode(WINDOW_W, WINDOW_H), "Shooter Simulation");
    window.draw(backgroundSprite);
    window.setFramerateLimit(60);

    sf::Texture shooterTexture;
    if (!shooterTexture.loadFromFile("assets/images/shooter.png")) {
        std::cerr << "Erro ao carregar shooter.png\n";
    }
    std::vector<sf::Sprite> shooters_sprites;
    shooters_sprites.reserve(shooters.size());
    for (size_t i = 0; i < shooters.size(); ++i) {
        shooters_sprites.emplace_back(shooterTexture);
    }

    // Creating helicopter and a sprite to It
    sf::Texture helicopter_texture;
    helicopter_texture.loadFromFile("assets/images/helicopter.png");
    sf::Sprite helicopter_sprite(helicopter_texture);
    helicopter_sprite.setOrigin(helicopter_texture.getSize().x/2.f, helicopter_texture.getSize().y/2.f);

    // Flag
    sf::Texture flag_texture;
    flag_texture.loadFromFile("assets/images/flag.png");
    sf::Sprite flag_sprite(flag_texture);
    flag_sprite.setPosition(WINDOW_W - 120, LOWER_LIMIT - 80);

    // 4) Loop de renderização
    game_state = GAME_RUNNING;
    while (window.isOpen() && game_state == GAME_RUNNING) {
        window.clear();
        window.draw(backgroundSprite);
        window.draw(flag_sprite);
        sf::Event window_event;
        while (window.pollEvent(window_event)) {
            if (window_event.type == sf::Event::Closed) {
                window.close();
                game_state = GAME_ABORTED;
                break;
            }
        }
        pthread_mutex_lock(&helicopter_mutex);
        helicopter_control.up = sf::Keyboard::isKeyPressed(sf::Keyboard::Up);
        helicopter_control.down = sf::Keyboard::isKeyPressed(sf::Keyboard::Down);
        helicopter_control.left = sf::Keyboard::isKeyPressed(sf::Keyboard::Left);
        helicopter_control.right = sf::Keyboard::isKeyPressed(sf::Keyboard::Right);
        sf::Vector2f heli_pos = helicopter->getPosition();
        pthread_mutex_unlock(&helicopter_mutex);
        helicopter_sprite.setPosition(heli_pos);
        window.draw(helicopter_sprite);

        // Drawn soldiers
        for (int i = 0; i < soldiers_waiting; ++i) {
            sf::Sprite sprite;
            sprite.setTexture(soldier_texture);
            float dx = SOLDIERS_NUM - i * 7;
            float dy = (i % 2 == 0) ? 5 : 0;
            sprite.setPosition(rescue_point.x + dx - 15, rescue_point.y - 10 + dy);
            window.draw(sprite);
            if (collide(sprite, helicopter_sprite)) {
                bool soldier_boarded = helicopter->boardSoldier();
                if (soldier_boarded) {
                    soldiers_waiting--;
                }
            }
        }

        // Desembarque automático
        if (collide(helicopter_sprite, flag_sprite)) {
            bool soldier_dropped = helicopter->dropSoldier();
            if (soldier_dropped) {
                rescued_soldiers++;
                if (rescued_soldiers == SOLDIERS_NUM) {
                    game_state = GAME_WIN;
                    break;
                }
            }
        }

        window.draw(helicopter_sprite);

        pthread_mutex_lock(&shooters_mutex);
        for (size_t i = 0; i < shooters.size(); ++i) {
            Shooter* shooter = shooters[i];
            // Converte pos_x, pos_y para pixels
            shooters_sprites[i].setPosition(shooter->position);
            window.draw(shooters_sprites[i]);
            if (collide(shooters_sprites[i], helicopter_sprite)) {
                game_state = GAME_OVER_HELICOPTER_COLLIDED;
                break;
            }
        }
        pthread_mutex_unlock(&shooters_mutex);

        // Drawing bullets
        pthread_mutex_lock(&bullets_mutex);
        for (auto& bullet: bullets) {
            sf::RectangleShape bullet_shape(sf::Vector2f(4.f, 20.f));
            bullet_shape.setPosition(bullet.position);
            bullet_shape.setFillColor(sf::Color::Red);
            window.draw(bullet_shape);
            if (collide(bullet_shape, helicopter_sprite)) {
                game_state = GAME_OVER_HELICOPTER_SHOOTED;
                break;
            }
        }
        pthread_mutex_unlock(&bullets_mutex);

        // Stats display
        std::ostringstream stats;
        stats << "Soldiers Rescued (" << rescued_soldiers << "/" << SOLDIERS_NUM<< ")\n";
        stats << "Helicopter Occupation (" << helicopter->getOccupation() << "/" << helicopter->getCapacity() << ")";
        statsText.setString(stats.str());
        window.draw(statsText);
        // Displaying all elements on window
        window.display();
    }

    window.close();
    for (auto& shooter: shooters) {
        shooter->done = true;
    }

    // Waiting each thread to die
    pthread_join(helicopter_thread, nullptr);
    for (auto& t: threads) pthread_join(t, nullptr);
    pthread_join(bullets_manager_thread, nullptr);

    switch (game_state) {
        case GAME_WIN:
            std::cout << "\n\n" << "GAME WIN" << "\n\n" << "All soldiers were rescued!" << "\n\n";
            break;
        case GAME_OVER_HELICOPTER_COLLIDED:
            std::cout << "\n\n" << "GAME OVER" << "\n" << "The Helicoper collided with a object." << "\n\n";
            break;
        case GAME_OVER_HELICOPTER_SHOOTED:
            std::cout << "\n\n" << "GAME OVER" << "\n\n" << "The Helicoper was shooted." << "\n\n";
            break;
        case GAME_OVER_HELICOPTER_TOO_HIGH:
            std::cout << "\n\n" << "GAME OVER" << "\n\n" << "The Helicoper went too high." << "\n\n";
        default:
            std::cout << "\n\n" << "Game aborted" << "\n\n";
            break;
    }
    return 0;
}

int main() {
    std::cout << "Select Game Difficulty:\n";
    std::cout << "1 - Easy\n";
    std::cout << "2 - Regular\n";
    std::cout << "3 - Hard\n";
    std::cout << "\n" << "Difficulty: ";
    int choice = 0;
    std::cin >> choice;
    int shooter_max_ammo, shooter_fire_cooldown, shooter_reload_time;
    switch (choice) {
        case 1:
            shooter_max_ammo = 5;
            shooter_fire_cooldown = 1200 * MS_MICROSECONDS_MULTIPLIER;
            shooter_reload_time = 500 * MS_MICROSECONDS_MULTIPLIER;
            std::cout << "Selected Difficulty: Easy\n";
            break;
        case 2:
            shooter_max_ammo = 10;
            shooter_fire_cooldown = 900 * MS_MICROSECONDS_MULTIPLIER;
            shooter_reload_time = 300 * MS_MICROSECONDS_MULTIPLIER;
            std::cout << "Selected Difficulty: Regular\n";
            break;
        case 3:
            shooter_max_ammo = 20;
            shooter_fire_cooldown = 700 * MS_MICROSECONDS_MULTIPLIER;
            shooter_reload_time = 100 * MS_MICROSECONDS_MULTIPLIER;
            std::cout << "Selected Difficulty: Hard\n";
            break;
        default:
            std::cerr << "Invalid Option. Quitting game" << "\n\n";
            exit(1);
            break;
    }
    game(shooter_max_ammo, shooter_fire_cooldown, shooter_reload_time);
}
