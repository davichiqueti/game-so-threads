#include <pthread.h>
#include <iostream>
#include <SFML/Graphics.hpp>
#include <vector>
#include <bits/stdc++.h>
#include "entities/Helicopter.h"


// --- Configurações ---
const int WINDOW_W = 1200;
const int WINDOW_H = 600;

const int MS_MICROSECONDS_MULTIPLIER = 1000;
const int SHOOTER_MOVEMENT_SLEEP = 20 * MS_MICROSECONDS_MULTIPLIER;
const int SOLDIERS = 2;
const int RELOAD_POSITION_X = 365;
const int LOWER_LIMIT = WINDOW_H - 60;
// Fixed positions
sf::Vector2f rescue_point = {100.f, WINDOW_H - 70.f};
// Mutexes used to handle critical sessions
pthread_mutex_t bridge_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t reloader_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t shooters_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t bullets_mutex = PTHREAD_MUTEX_INITIALIZER;

enum GameState {
    GAME_RUNNING,
    GAME_WIN,
    GAME_OVER_HELICOPTER_SHOOTED,
    GAME_OVER_HELICOPTER_COLLIDED,
    GAME_OVER_HELICOPTER_TOO_HIGH
};
GameState game_state;


struct Bullet {
    bool active = true;
    int speed;
    sf::Vector2f position;
};

std::vector<Bullet> bullets;

class Shooter {
public:
    int ammo_capacity;
	int ammo;
    int fire_cooldown;
    int reload_time;
    sf::Vector2f position;
    int initial_pos_x;
	bool done = false;

	Shooter(int ammo_capacity_, int fire_cooldown_, int reload_time, int pos_y_, int pos_x_)
        :ammo_capacity(ammo_capacity_),
        ammo(ammo_capacity_),
        fire_cooldown(fire_cooldown_),
        position(pos_x_, pos_y_),
        initial_pos_x(pos_x_) {}

	Bullet shoot() {
        ammo--;
        Bullet bullet;
        bullet.position = this->position;
        bullet.position.x += 38;
        bullet.speed = 5;
        // Pushing new bullet to the bullets array
        return bullet;
    };

	void pass_bridge(bool reload_direction) {
        // Locking bridge to other shooter dont pass
        if (reload_direction) {
            while (position.x > RELOAD_POSITION_X) {
                usleep(SHOOTER_MOVEMENT_SLEEP);
                this->position.x -= 5;
            }
            this->position.x = RELOAD_POSITION_X;
        } else {
            while (this->position.x < initial_pos_x) {
                usleep(SHOOTER_MOVEMENT_SLEEP);
                this->position.x += 5;
            }
            position.x = initial_pos_x;
        }
	}

    void reload() {
        usleep(reload_time);
        ammo = ammo_capacity;
    }
};

void *helicopter_func(void *arg) {
    Helicopter *helicopter = (Helicopter *)arg;
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
        usleep(16000);
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
			shooter->pass_bridge(true);
            pthread_mutex_unlock(&bridge_mutex);
            // Locking reloader to other shooter dont pass
            pthread_mutex_lock(&reloader_mutex);
            shooter->reload();
            pthread_mutex_unlock(&reloader_mutex);
            // Moving the shooter back to initial position blocking the bridge
            pthread_mutex_lock(&bridge_mutex);
            shooter->pass_bridge(false);
            pthread_mutex_unlock(&bridge_mutex);
		}
	}
	return nullptr;
}

void showGameMessage(GameState final_state) {
    // Configurações da janela de mensagem
    const int MSG_WINDOW_W = 400;
    const int MSG_WINDOW_H = 200;
    
    sf::RenderWindow messageWindow(
        sf::VideoMode(MSG_WINDOW_W, MSG_WINDOW_H), 
        "Game Result",
        sf::Style::Titlebar | sf::Style::Close
    );
    
    // Centralizar janela na tela
    messageWindow.setPosition(
        sf::Vector2i(
            (sf::VideoMode::getDesktopMode().width - MSG_WINDOW_W) / 2,
            (sf::VideoMode::getDesktopMode().height - MSG_WINDOW_H) / 2
        )
    );
    
    // Carregar fonte
    sf::Font font;
    if (!font.loadFromFile("assets/fonts/OpenSans-Regular.ttf")) {
        // Fallback para fonte padrão do sistema se não encontrar
        std::cerr << "Warning: Could not load font, using default\n";
    }
    
    // Configurar textos
    sf::Text titleText, messageText, instructionText;
    
    // Título
    titleText.setFont(font);
    titleText.setCharacterSize(24);
    titleText.setFillColor(sf::Color::White);
    titleText.setStyle(sf::Text::Bold);
    
    // Mensagem principal
    messageText.setFont(font);
    messageText.setCharacterSize(16);
    messageText.setFillColor(sf::Color::White);
    
    // Instrução
    instructionText.setFont(font);
    instructionText.setCharacterSize(12);
    instructionText.setFillColor(sf::Color::Yellow);
    instructionText.setString("Press SPACE to close or ESC to exit");
    
    // Definir conteúdo baseado no estado do jogo
    sf::Color backgroundColor;
    switch (final_state) {
        case GAME_WIN:
            titleText.setString("VICTORY!");
            titleText.setFillColor(sf::Color::Green);
            messageText.setString("Congratulations!\nAll soldiers were successfully rescued!");
            backgroundColor = sf::Color(0, 100, 0, 200); // Verde escuro
            break;
            
        case GAME_OVER_HELICOPTER_SHOOTED:
            titleText.setString("GAME OVER");
            titleText.setFillColor(sf::Color::Red);
            messageText.setString("Mission Failed!\nThe helicopter was shot down by enemy fire.");
            backgroundColor = sf::Color(100, 0, 0, 200); // Vermelho escuro
            break;
            
        case GAME_OVER_HELICOPTER_COLLIDED:
            titleText.setString("GAME OVER");
            titleText.setFillColor(sf::Color::Red);
            messageText.setString("Mission Failed!\nThe helicopter collided with an obstacle.");
            backgroundColor = sf::Color(100, 0, 0, 200);
            break;
            
        case GAME_OVER_HELICOPTER_TOO_HIGH:
            titleText.setString("GAME OVER");
            titleText.setFillColor(sf::Color::Red);
            messageText.setString("Mission Failed!\nThe helicopter flew too high and was lost.");
            backgroundColor = sf::Color(100, 0, 0, 200);
            break;
            
        default:
            titleText.setString("GAME ABORTED");
            titleText.setFillColor(sf::Color::Yellow);
            messageText.setString("The game was terminated unexpectedly.");
            backgroundColor = sf::Color(100, 100, 0, 200); // Amarelo escuro
            break;
    }
    
    // Centralizar textos
    sf::FloatRect titleBounds = titleText.getLocalBounds();
    titleText.setPosition(
        (MSG_WINDOW_W - titleBounds.width) / 2,
        30
    );
    
    sf::FloatRect messageBounds = messageText.getLocalBounds();
    messageText.setPosition(
        (MSG_WINDOW_W - messageBounds.width) / 2,
        80
    );
    
    sf::FloatRect instructionBounds = instructionText.getLocalBounds();
    instructionText.setPosition(
        (MSG_WINDOW_W - instructionBounds.width) / 2,
        MSG_WINDOW_H - 40
    );
    
    // Criar background colorido
    sf::RectangleShape background(sf::Vector2f(MSG_WINDOW_W, MSG_WINDOW_H));
    background.setFillColor(backgroundColor);
    
    // Loop da janela de mensagem
    while (messageWindow.isOpen()) {
        sf::Event event;
        while (messageWindow.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                messageWindow.close();
            }
            
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Space) {
                    messageWindow.close();
                }
                if (event.key.code == sf::Keyboard::Escape) {
                    messageWindow.close();
                    exit(0); // Sair completamente do programa
                }
            }
        }
        
        messageWindow.clear(sf::Color::Black);
        messageWindow.draw(background);
        messageWindow.draw(titleText);
        messageWindow.draw(messageText);
        messageWindow.draw(instructionText);
        messageWindow.display();
    }
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

    // 1) Cria shooters
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

    // Threads for shooters
    std::vector<pthread_t> threads(shooters.size());
    for (size_t i = 0; i < shooters.size(); ++i) {
        pthread_create(&threads[i], nullptr, shooter_func, shooters[i]);
    }

    pthread_t bullets_manager_thread;
    pthread_create(&bullets_manager_thread, nullptr, bullets_manager_func, &bullets);

    int soldiers_waiting = SOLDIERS;
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
	Helicopter helicopter(400.f, 300.f, 20.f);
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
        // Helicopter
        sf::Event ev;
        sf::Vector2f heli_pos = helicopter.getPosition();
        while (window.pollEvent(ev)) {
            if (ev.type == sf::Event::Closed) window.close();

            // Handling moving keys
            if (ev.key.code == sf::Keyboard::Up) {
                heli_pos.y -= helicopter.step;
            }
            if (ev.key.code == sf::Keyboard::Down && heli_pos.y < LOWER_LIMIT - helicopter.step) {
                heli_pos.y += helicopter.step;
            }
            if ((ev.key.code == sf::Keyboard::Left) && heli_pos.x > helicopter.step){
                heli_pos.x -= helicopter.step;
            }
            if ((ev.key.code == sf::Keyboard::Right) && (heli_pos.x < WINDOW_W - helicopter.step)){
                heli_pos.x += helicopter.step;
            }
            if (heli_pos.y <= 0) {
                game_state = GAME_OVER_HELICOPTER_TOO_HIGH;
                break;
            }
            helicopter.setPosition(heli_pos.x, heli_pos.y);
        }
		helicopter_sprite.setPosition(heli_pos);
        window.draw(helicopter_sprite);

        // Drawn soldiers
        for (int i = 0; i < soldiers_waiting; ++i) {
            sf::Sprite sprite;
            sprite.setTexture(soldier_texture);
            float dx = SOLDIERS - i * 7;
            float dy = (i % 2 == 0) ? 5 : 0;
            sprite.setPosition(rescue_point.x + dx - 15, rescue_point.y - 10 + dy);
            window.draw(sprite);
            if (collide(sprite, helicopter_sprite)) {
                bool soldier_boarded = helicopter.boardSoldier();
                if (soldier_boarded) {
                    soldiers_waiting--;
                }
            }
        }

        // Desembarque automático
        if (collide(helicopter_sprite, flag_sprite)) {
            bool soldier_dropped = helicopter.dropSoldier();
            if (soldier_dropped) {
                rescued_soldiers++;
                if (rescued_soldiers == SOLDIERS) {
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
        stats << "Soldiers Rescued (" << rescued_soldiers << "/" << SOLDIERS<< ")\n";
        stats << "Helicopter Occupation (" << helicopter.getOccupation() << "/" << helicopter.getCapacity() << ")";
        statsText.setString(stats.str());
        window.draw(statsText);
        // Displaying all elements on window
        window.display();        
    }

    window.close();
    for (auto& shooter: shooters) {
        shooter->done = true;
    }

    /* switch (game_state) {
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
    } */

    for (auto& t: threads)
        pthread_join(t, nullptr);
    
    pthread_join(bullets_manager_thread, nullptr);

    showGameMessage(game_state);

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
