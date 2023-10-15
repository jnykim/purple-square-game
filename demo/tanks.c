#include "tanks.h"
#include "body.h"
#include "color.h"
#include "forces.h"
#include "list.h"
#include "polygon.h"
#include "scene.h"
#include "sdl_wrapper.h"
#include "state.h"
#include "vector.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>

// Constants for screen width and height
const vector_t WINDOW_TANKS = (vector_t){.x = 1000, .y = 500};
const vector_t CENTER_TANKS = (vector_t){.x = 500, .y = 250};
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

// Physics constants
#define G 6.67e-11
#define M 6e24
#define g_const 9.8
#define R (sqrt(G * M / g_const))
#define INIT_VEL 70
const double ELASTICITY = 1;

// Sound
Mix_Chunk *bullet_shot_wav = NULL;
Mix_Chunk *player_won_wav = NULL;
#define BULLET_SHOT_WAV_PATH "assets/shoot.wav"
#define PLAYER_WON_WAV_PATH "assets/player_won.wav"

// Game constants
const rgb_color_t BODY_COLOR = (rgb_color_t){.r = 0, .g = 0, .b = 0};

// Landscape constants
const size_t LANDSCAPE_Y = -300;
const size_t GROUND_BORDER = 130;

// Tank constants
const size_t PLAYER_1 = 1;
const size_t PLAYER_2 = 2;
const double TANK_ANGLE_CHANGE = M_PI / 6.0;
const size_t TANK_DX = 2;
const size_t TANK_IMG_WIDTH = 200;

// Bullet constants
const double TIME_TO_FIRE_BULLET = 5.0;
const size_t BULLET_IMG_WIDTH = 20;
const double BULLET_MASS = 2.0;
const size_t BULLET_DMG = 20;

// Crater constants
const size_t CRATER_IMG_WIDTH = 100;

// Boulder constants
const size_t BOULDER_IMG_WIDTH = 60;
const size_t BOULDER_WINDOW_HEIGHT = 320;
const size_t BOULDER_IMG_OFFSET = 150;

// Health constants
const size_t HEALTH_MAX = 80;


SDL_Renderer *tanks_renderer = NULL;
SDL_Rect texr1;
SDL_Rect texr2;

double time_since_last_bullet_1 = TIME_TO_FIRE_BULLET;
double time_since_last_bullet_2 = TIME_TO_FIRE_BULLET;
double time_since_game_over = 0.0;

// Structure for player character
typedef struct player {
    int shape;              // shape identifier for the player character
    SDL_Texture *image;     // SDL_Texture pointer for the tank image
    SDL_Rect location;
    size_t health;
    body_t *body;
    bool left; // True if player is currently facing left direction, False if not
    float angle;
    float img_angle;
    bool last_move;
} player_t;

// Structure for bullet
typedef struct bullet {
    int shape;
    size_t player_num;
    SDL_Texture *image;
    SDL_Rect location;
    body_t *body;
} bullet_t;

// Structure for crater
typedef struct crater {
    int shape;
    SDL_Texture *image;
    SDL_Rect location;
} crater_t;

// Structure for boulder
typedef struct boulder {
    SDL_Texture *image;
    SDL_Rect location;
} boulder_t;

// Structure for landscape
typedef struct landscape {
    SDL_Texture *image;
    SDL_Rect location;
} landscape_t;

// Structure for game state
typedef struct tanks_state {
    scene_t *scene;
    player_t player1;    // Player 1 character
    player_t player2;    // Player 2 character
    landscape_t landscape;
    list_t *bullets;
    list_t *crater_list;
    list_t *boulder_list;
    double camera_x;
    bool game_over;
    size_t counter;
    size_t winner;
} tanks_state_t;

// Structure for camera
typedef struct {
    int x;
    int y;
    int width;
    int height;
} Camera;

// Structure for game states
typedef struct state {
    scene_t *scene;
    state_t *game_state;
    bool switch_game;
    size_t curr_game;
} state_t;

list_t *make_rect(SDL_Rect rect) {
    list_t *vertices = list_init(4, free);
    assert(vertices != NULL);
    vector_t *v1 = malloc(sizeof(vector_t));
    vector_t *v2 = malloc(sizeof(vector_t));
    vector_t *v3 = malloc(sizeof(vector_t));
    vector_t *v4 = malloc(sizeof(vector_t));

    v1->x = rect.x;
    v1->y = rect.y;

    v2->x = rect.x + rect.w;
    v2->y = rect.y;
    
    v3->x = rect.x + rect.w;
    v3->y = rect.y - rect.h;

    v4->x = rect.x;
    v4->y = rect.y - rect.h;

    list_add(vertices, v1);
    list_add(vertices, v2);
    list_add(vertices, v3);
    list_add(vertices, v4);

    return vertices;
}

void shoot_bullet(tanks_state_t *state, size_t player_num, scene_t *scene) {
    // Make new bullet
    bullet_t *new_bullet = malloc(sizeof(bullet_t));
    new_bullet->image = IMG_LoadTexture(tanks_renderer, "assets/bullet.png");

    // Variables for width and height of image
    size_t w = 0;
    size_t h = 0;

    // Initialize bullets location
    if (player_num == 1) {
        SDL_QueryTexture(new_bullet->image, NULL, NULL, &w, &h);
        if (state->player1.left) {
            new_bullet->location.x = state->player1.location.x; new_bullet->location.y = state->player1.location.y; new_bullet->location.w = BULLET_IMG_WIDTH; new_bullet->location.h = h * BULLET_IMG_WIDTH / w; 
        } else {
            new_bullet->location.x = state->player1.location.x + state->player1.location.w; new_bullet->location.y = state->player1.location.y; new_bullet->location.w = BULLET_IMG_WIDTH; new_bullet->location.h = h * BULLET_IMG_WIDTH / w; 
        }
    } else {
        SDL_QueryTexture(new_bullet->image, NULL, NULL, &w, &h);
        if (state->player2.left) {
            new_bullet->location.x = state->player2.location.x; new_bullet->location.y = state->player2.location.y; new_bullet->location.w = BULLET_IMG_WIDTH; new_bullet->location.h = h * BULLET_IMG_WIDTH / w; 
        } else {
            new_bullet->location.x = state->player2.location.x + state->player2.location.w; new_bullet->location.y = state->player2.location.y; new_bullet->location.w = BULLET_IMG_WIDTH; new_bullet->location.h = h * BULLET_IMG_WIDTH / w; 
        }
    }

    // Initialize bullet's properties
    new_bullet->shape = 2;
    new_bullet->player_num = player_num;
    new_bullet->body = body_init(make_rect(new_bullet->location), BULLET_MASS, BODY_COLOR);
    scene_add_body(scene, new_bullet->body);

    // Add collision handler
    if (new_bullet->player_num == PLAYER_1) {
        create_destructive_physics_collision(scene, ELASTICITY, state->player2.body, new_bullet->body);
    } else {
        create_destructive_physics_collision(scene, ELASTICITY, state->player1.body, new_bullet->body);
    }

    // Add gravity force creator
    create_newtonian_gravity(scene, G, scene_get_body(scene, 0), new_bullet->body);

    // Set initial velocity
    vector_t init_velocity = {.x = -1.0 * INIT_VEL, .y = 0.0};
    if ((player_num == PLAYER_1 && !state->player1.left) || (player_num == PLAYER_2 && !state->player2.left)) {
        init_velocity.x = INIT_VEL;
    }
    if (player_num == PLAYER_1) {
        init_velocity.x = init_velocity.x * cos(state->player1.angle);
        init_velocity.y = init_velocity.y * sin(state->player1.angle);
    } else {
        init_velocity.x = init_velocity.x * cos(state->player2.angle);
        init_velocity.y = init_velocity.y * sin(state->player2.angle);
    }
    body_set_velocity(new_bullet->body, init_velocity);

    list_add(state->bullets, new_bullet);

    Mix_PlayChannel(-1, bullet_shot_wav, 0);
}

void make_crater(tanks_state_t *state, size_t location_x, size_t location_y) {
    // Make new crater
    crater_t *new_crater = malloc(sizeof(crater_t));
    new_crater->image = IMG_LoadTexture(tanks_renderer, "assets/crater.png");

    // Variables for width and height of image
    size_t w = 0;
    size_t h = 0;
    SDL_QueryTexture(new_crater->image, NULL, NULL, &w, &h);

    new_crater->location.x = location_x; new_crater->location.y = location_y; new_crater->location.w = CRATER_IMG_WIDTH; new_crater->location.h = h * CRATER_IMG_WIDTH / w; 

    list_add(state->crater_list, new_crater);
}

void make_boulder(tanks_state_t *state, size_t location_x, size_t location_y) {
    // Make new boulder
    boulder_t *new_boulder = malloc(sizeof(boulder_t));
    new_boulder->image = IMG_LoadTexture(tanks_renderer, "assets/boulder.png");

    // Variables for width and height of image
    size_t w = 0;
    size_t h = 0;
    SDL_QueryTexture(new_boulder->image, NULL, NULL, &w, &h);

    new_boulder->location.x = location_x; new_boulder->location.y = location_y; new_boulder->location.w = BOULDER_IMG_WIDTH; new_boulder->location.h = h * BOULDER_IMG_WIDTH / w; 

    list_add(state->boulder_list, new_boulder);
}

void tanks_end(tanks_state_t *state) {
    Mix_PlayChannel(-1, player_won_wav, 0);
    state->game_over = true;
    SDL_RenderClear(tanks_renderer);

     // Free tank images
    SDL_DestroyTexture(state->player1.image);

    while ( Mix_PlayingMusic() ) ;
	Mix_FreeChunk(bullet_shot_wav);
    Mix_FreeChunk(player_won_wav);

    return;
}

char *curr_tank_image(size_t health, float angle, bool left, size_t player_num) {
    char image_url[] = "assets/tanks/";

    // Add health
    if (health == HEALTH_MAX) {
        strcat(image_url, "high");
    } else if (health >= HEALTH_MAX - BULLET_DMG) {
        strcat(image_url, "mid2");
    } else if (health >= HEALTH_MAX - 2 * BULLET_DMG) {
        strcat(image_url, "mid1");
    } else if (health >= HEALTH_MAX - 3 * BULLET_DMG) {
        strcat(image_url, "low");
    } else {
        strcat(image_url, "death");
    }

    if (health > 0) {
        strcat(image_url, "Tank");

        // Add angle
        if (angle >= 2 * TANK_ANGLE_CHANGE) {
            strcat(image_url, "High");
        } else if (angle >= TANK_ANGLE_CHANGE) {
            strcat(image_url, "Mid");
        } else {
            strcat(image_url, "Low");
        }

        // Add direction
        if (left) {
            strcat(image_url, "Left");
        } else {
            strcat(image_url, "Right");
        }
    }

    // Add player number
    if (player_num == PLAYER_1) {
        strcat(image_url, "1.png");
    } else {
        strcat(image_url, "2.png");
    }

    char *image_url_return = malloc(sizeof(char) * strlen(image_url));
    image_url_return = image_url;

    return image_url_return;
}

void update_camera(tanks_state_t *state) {
    // Adjust player locations
    SDL_Rect adjusted_player1_location = state->player1.location;
    adjusted_player1_location.x -= state->camera_x;

    SDL_Rect adjusted_player2_location = state->player2.location;
    adjusted_player2_location.x -= state->camera_x;

    state->player1.location = adjusted_player1_location;  
    state->player2.location = adjusted_player2_location; 

    body_set_centroid(state->player1.body, (vector_t){.x = body_get_centroid(state->player1.body).x - state->camera_x, .y = body_get_centroid(state->player1.body).y});
    body_set_centroid(state->player2.body, (vector_t){.x = body_get_centroid(state->player2.body).x - state->camera_x, .y = body_get_centroid(state->player2.body).y});

    // Adjust bullet locations
    for (size_t i = 0; i < list_size(state->bullets); i++) {
        bullet_t *curr_bullet = list_get(state->bullets, i);
        curr_bullet->location.x -= state->camera_x;
        body_set_centroid(curr_bullet->body, (vector_t){.x = body_get_centroid(curr_bullet->body).x - state->camera_x, .y = body_get_centroid(curr_bullet->body).y});
    }

    // Adjust crater locations
    for (size_t i = 0; i < list_size(state->crater_list); i++) {
        crater_t *curr_crater = list_get(state->crater_list, i);
        curr_crater->location.x -= state->camera_x;
    }

    // Adjust boulder locations
    for (size_t i = 0; i < list_size(state->boulder_list); i++) {
        boulder_t *curr_boulder = list_get(state->boulder_list, i);
        curr_boulder->location.x -= state->camera_x;
    }
}

void on_key(char key, key_event_type_t type, double held_time, state_t *overall_state) {
  tanks_state_t *state = overall_state->game_state;
  scene_t *scene = state->scene;

  if (type == KEY_PRESSED) {
    switch (key) {
        case SDLK_a:
            state->player1.location.x = state->player1.location.x - TANK_DX;
            body_set_centroid(state->player1.body, (vector_t){.x = body_get_centroid(state->player1.body).x - TANK_DX, .y = body_get_centroid(state->player1.body).y});
            state->player1.left = true;
            state->player1.last_move = true;
            break;
        case SDLK_d:
            state->player1.location.x = state->player1.location.x + TANK_DX;
            body_set_centroid(state->player1.body, (vector_t){.x = body_get_centroid(state->player1.body).x + TANK_DX, .y = body_get_centroid(state->player1.body).y});
            state->player1.left = false;
            state->player1.last_move = true;
            break;
        case SDLK_s:
            if (time_since_last_bullet_1 >= TIME_TO_FIRE_BULLET) {
                shoot_bullet(state, PLAYER_1, scene);
                time_since_last_bullet_1 = 0.0;
            }
            break;
        case SDLK_w:
            if (state->player1.angle >= 2 * TANK_ANGLE_CHANGE) {
                state->player1.angle = 0.0;
            } else {
                state->player1.angle = state->player1.angle + TANK_ANGLE_CHANGE;
            }
            break;
        case LEFT_ARROW:
            state->player2.location.x = state->player2.location.x - TANK_DX;
            body_set_centroid(state->player2.body, (vector_t){.x = body_get_centroid(state->player2.body).x - TANK_DX, .y = body_get_centroid(state->player2.body).y});
            state->player2.left = true;
            state->player2.last_move = true;
            break;
        case RIGHT_ARROW:
            state->player2.location.x = state->player2.location.x + TANK_DX;
            body_set_centroid(state->player2.body, (vector_t){.x = body_get_centroid(state->player2.body).x + TANK_DX, .y = body_get_centroid(state->player2.body).y});
            state->player2.left = false;
            state->player2.last_move = true;
            break;
        case DOWN_ARROW:
            if (time_since_last_bullet_2 >= TIME_TO_FIRE_BULLET) {
                shoot_bullet(state, PLAYER_2, scene);
                time_since_last_bullet_2 = 0.0;
            }
            break;
        case UP_ARROW:
            if (state->player2.angle >= 2 * TANK_ANGLE_CHANGE) {
                state->player2.angle = 0.0;
            } else {
                state->player2.angle = state->player2.angle + TANK_ANGLE_CHANGE;
            }
            break;
        case SDLK_g:
            state->camera_x = state->player1.location.x - WINDOW_TANKS.x / 2;
            update_camera(state);
            break;
        case SDLK_h:
            state->camera_x = state->player2.location.x - WINDOW_TANKS.x / 2;
            update_camera(state);
            break;
        case SDLK_y:
            state->camera_x = state->player1.location.x - (WINDOW_TANKS.x / 2 - (abs(state->player1.location.x - state->player2.location.x)) / 2 - TANK_IMG_WIDTH / 2);
            update_camera(state);
            break;
        case SDLK_t:
            state->game_over = true;
            break;
    }
  }
}

void on_key_blank(char key, key_event_type_t type, double held_time, state_t *overall_state) {
}


// Initialize the game state
tanks_state_t *tanks_init() {
    vector_t min = VEC_ZERO;
    vector_t max = WINDOW_TANKS;
    SDL_Init(SDL_INIT_AUDIO);
    IMG_Init(IMG_INIT_PNG);
    Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 4096);

    tanks_state_t *state = malloc(sizeof(tanks_state_t));
    scene_t *scene = scene_init();

    tanks_renderer = sdl_return_renderer();

    // Load sound effects
    bullet_shot_wav = Mix_LoadWAV(BULLET_SHOT_WAV_PATH);
    player_won_wav = Mix_LoadWAV(PLAYER_WON_WAV_PATH);

    // Create gravity body and add to scene
    body_t *gravity_body = body_init(make_rect((SDL_Rect){.x = 0, .y = -R, .w = WINDOW_TANKS.x, .h = 1}), M, BODY_COLOR);
    scene_add_body(scene, gravity_body);

    // Initialize state's list of bullets
    state->bullets = list_init(10, free);

    // Initialize state's list of craters
    state->crater_list = list_init(10, free);

    // Initialize state's list of bullets
    state->boulder_list = list_init(10, free);

    // Variables for width and height of image
    size_t w = 0;
    size_t h = 0;

    // Initialize the landscape properties
    state->landscape.image = IMG_LoadTexture(tanks_renderer, "assets/ground.png");
    SDL_QueryTexture(state->landscape.image, NULL, NULL, &w, &h);
    state->landscape.location.x = 0; state->landscape.location.y = LANDSCAPE_Y; state->landscape.location.w = w * WINDOW_TANKS.x / h; state->landscape.location.h = WINDOW_TANKS.x; 

    // Load initial tank images for player 1 and player 2
    state->player1.image = IMG_LoadTexture(tanks_renderer, "assets/tanks/highTankLowRight1.png");
    state->player2.image = IMG_LoadTexture(tanks_renderer, "assets/tanks/highTankLowLeft2.png");

    // Initialize player 1's location
    SDL_QueryTexture(state->player1.image, NULL, NULL, &w, &h);
    state->player1.location.x = 0; state->player1.location.y = CENTER_TANKS.y; state->player1.location.w = TANK_IMG_WIDTH; state->player1.location.h = h * TANK_IMG_WIDTH / w; 

    // Initialize player 2's location
    SDL_QueryTexture(state->player2.image, NULL, NULL, &w, &h);
    state->player2.location.x = WINDOW_TANKS.x - 200; state->player2.location.y = CENTER_TANKS.y; state->player2.location.w = TANK_IMG_WIDTH; state->player2.location.h = h * TANK_IMG_WIDTH / w; 

    // Initialize player characters' properties
    state->player1.shape = PLAYER_1;               // Set shape identifier for player 1
    state->player2.shape = PLAYER_2;               // Set shape identifier for player 2
    state->player1.left = false;
    state->player2.left = true;
    state->player1.angle = 0.0;
    state->player2.angle = 0.0;
    state->player1.health = HEALTH_MAX;
    state->player2.health = HEALTH_MAX;
    state->player1.img_angle = 0.0;
    state->player2.img_angle = 0.0;
    state->player1.last_move = false;
    state->player2.last_move = false;
    state->game_over = false;
    state->counter = 0;
    state->winner = 0;

    // Initialize player characters' bodies
    state->player1.body = body_init(make_rect(state->player1.location), INFINITY, BODY_COLOR);
    state->player2.body = body_init(make_rect(state->player2.location), INFINITY, BODY_COLOR);
    scene_add_body(scene, state->player1.body);
    scene_add_body(scene, state->player2.body);

    // Initialize boulders
    for (int i = -1800; i <= 2200; i += 500) {
        make_boulder(state, i, BOULDER_WINDOW_HEIGHT);
    }

    state->scene = scene; 
    sdl_on_key(on_key);
    return state;
}

// Main game loop
void tanks_main(tanks_state_t *state, double game_dt) {
    // Draw game objects on the screen
    scene_t *scene = state->scene;
    double dt = game_dt;
    time_since_last_bullet_1 = time_since_last_bullet_1 + dt;
    time_since_last_bullet_2 = time_since_last_bullet_2 + dt;
    sdl_render_scene(scene);

    // Draw background
    SDL_SetRenderDrawColor(tanks_renderer, 173, 216, 230, 255);

    // Clear the screen
	SDL_RenderClear(tanks_renderer);

    // Render landscape
    SDL_RenderCopy(tanks_renderer, state->landscape.image, NULL, &state->landscape.location);

    scene_tick(scene, dt);

    // Loop through each bullet and render it
    for (size_t i = 0; i < list_size(state->bullets); i++) {
        if (i >= list_size(state->bullets)) {
            continue;
        }
        bullet_t *curr_bullet = list_get(state->bullets, i);

        // Check if bullet has been removed
        if (body_is_removed(curr_bullet->body)) {
            // Take off health
            if (curr_bullet->player_num == 1) {
                state->player2.health = state->player2.health - BULLET_DMG;
            } else {
                state->player1.health = state->player1.health - BULLET_DMG;
            }
            list_remove(state->bullets, i);
            i--;
        } else if (body_get_centroid(curr_bullet->body).y < GROUND_BORDER) {
            // Add crater
            make_crater(state, curr_bullet->location.x, curr_bullet->location.y);

            // Destroy bullet if below window
            scene_remove_body(scene, i + 3);
            list_remove(state->bullets, i);
        } else {
            // Update bullet's image position
            curr_bullet->location.x = body_get_centroid(curr_bullet->body).x;
            curr_bullet->location.y = WINDOW_TANKS.y - 1.0 * body_get_centroid(curr_bullet->body).y;
            SDL_RenderCopy(tanks_renderer, curr_bullet->image, NULL, &curr_bullet->location);
        }
    }

    // Loop through each crater and render it
    for (size_t i = 0; i < list_size(state->crater_list); i++) {
        crater_t *curr_crater = list_get(state->crater_list, i);
        SDL_RenderCopy(tanks_renderer, curr_crater->image, NULL, &curr_crater->location);
    }
    
    bool player_1_over_boulder = false;
    bool player_2_over_boulder = false;
    // Loop through each boulder and render it
    for (size_t i = 0; i < list_size(state->boulder_list); i++) {
        boulder_t *curr_boulder = list_get(state->boulder_list, i);
        SDL_RenderCopy(tanks_renderer, curr_boulder->image, NULL, &curr_boulder->location);

        // Check if players are going over boulder
        if (state->player1.location.x >= curr_boulder->location.x - BOULDER_IMG_OFFSET && state->player1.location.x <= curr_boulder->location.x - BOULDER_IMG_OFFSET + BOULDER_IMG_WIDTH / 2) {
            state->player1.img_angle = 345 + 0.5 * (curr_boulder->location.x - BOULDER_IMG_OFFSET - state->player1.location.x);
            if (state->player1.last_move) {
                if (state->player1.left) {
                    state->player1.location.y += 2;
                    body_set_centroid(state->player1.body, (vector_t){.x = body_get_centroid(state->player1.body).x, .y = body_get_centroid(state->player1.body).y - 2});
                } else {
                    state->player1.location.y -= 2;
                    body_set_centroid(state->player1.body, (vector_t){.x = body_get_centroid(state->player1.body).x, .y = body_get_centroid(state->player1.body).y + 2});
                }
                state->player1.last_move = false;
            }
            player_1_over_boulder = true;
        } else if (state->player1.location.x >= curr_boulder->location.x - BOULDER_IMG_OFFSET + BOULDER_IMG_WIDTH / 2 + TANK_IMG_WIDTH / 2 && state->player1.location.x <= curr_boulder->location.x - BOULDER_IMG_OFFSET + BOULDER_IMG_WIDTH + TANK_IMG_WIDTH / 2) {
            state->player1.img_angle = 15 + 0.5 * (state->player1.location.x - (curr_boulder->location.x - BOULDER_IMG_OFFSET + BOULDER_IMG_WIDTH / 2 + TANK_IMG_WIDTH / 2));
            if (state->player1.last_move) {
                if (state->player1.left) {
                    state->player1.location.y -= 2;
                    body_set_centroid(state->player1.body, (vector_t){.x = body_get_centroid(state->player1.body).x, .y = body_get_centroid(state->player1.body).y + 2});
                } else {
                    state->player1.location.y += 2;
                    body_set_centroid(state->player1.body, (vector_t){.x = body_get_centroid(state->player1.body).x, .y = body_get_centroid(state->player1.body).y - 2});
                }
                state->player1.last_move = false;
            }
            player_1_over_boulder = true;
        } else if (!player_1_over_boulder) {
            state->player1.img_angle = 0;
        }

        if (state->player2.location.x >= curr_boulder->location.x - BOULDER_IMG_OFFSET && state->player2.location.x <= curr_boulder->location.x - BOULDER_IMG_OFFSET + BOULDER_IMG_WIDTH / 2) {
            state->player2.img_angle = 345 + 0.5 * (curr_boulder->location.x - BOULDER_IMG_OFFSET - state->player2.location.x);
            if (state->player2.last_move) {
                if (state->player2.left) {
                    state->player2.location.y += 2;
                    body_set_centroid(state->player2.body, (vector_t){.x = body_get_centroid(state->player2.body).x, .y = body_get_centroid(state->player2.body).y - 2});
                } else {
                    state->player2.location.y -= 2;
                    body_set_centroid(state->player2.body, (vector_t){.x = body_get_centroid(state->player2.body).x, .y = body_get_centroid(state->player2.body).y + 2});
                }
                state->player2.last_move = false;
            }
            player_2_over_boulder = true;
        } else if (state->player2.location.x >= curr_boulder->location.x - BOULDER_IMG_OFFSET + BOULDER_IMG_WIDTH / 2 + TANK_IMG_WIDTH / 2 && state->player2.location.x <= curr_boulder->location.x - BOULDER_IMG_OFFSET + BOULDER_IMG_WIDTH + TANK_IMG_WIDTH / 2) {
            state->player2.img_angle = 15 + 0.5 * (state->player2.location.x - (curr_boulder->location.x - BOULDER_IMG_OFFSET + BOULDER_IMG_WIDTH / 2 + TANK_IMG_WIDTH / 2));
            if (state->player2.last_move) {
                if (state->player2.left) {
                    state->player2.location.y -= 2;
                    body_set_centroid(state->player2.body, (vector_t){.x = body_get_centroid(state->player2.body).x, .y = body_get_centroid(state->player2.body).y + 2});
                } else {
                    state->player2.location.y += 2;
                    body_set_centroid(state->player2.body, (vector_t){.x = body_get_centroid(state->player2.body).x, .y = body_get_centroid(state->player2.body).y - 2});
                }
                state->player2.last_move = false;
            }
            player_2_over_boulder = true;
        } else if (!player_2_over_boulder) {
            state->player2.img_angle = 0;
        }
    }

    // Update player images
    state->player1.image = IMG_LoadTexture(tanks_renderer, curr_tank_image(state->player1.health, state->player1.angle, state->player1.left, 1));
    state->player2.image = IMG_LoadTexture(tanks_renderer, curr_tank_image(state->player2.health, state->player2.angle, state->player2.left, 2));

    // Copy the texture to the rendering context
	SDL_RenderCopyEx(tanks_renderer, state->player1.image, NULL, &state->player1.location, state->player1.img_angle, NULL, SDL_FLIP_NONE);
    SDL_RenderCopyEx(tanks_renderer, state->player2.image, NULL, &state->player2.location, state->player2.img_angle, NULL, SDL_FLIP_NONE);

    sdl_show();
    state->counter = state->counter + 1;

    // Check if game is over
    if (state->player1.health <= 0) {
        state->winner = 2;
        sdl_on_key(on_key_blank);
        if (time_since_game_over < 3.0) {
            time_since_game_over += dt;
        } else {
            tanks_end(state);
        }
    }

    if (state->player2.health <= 0) {
        state->winner = 1;
        sdl_on_key(on_key_blank);
        if (time_since_game_over < 3.0) {
            time_since_game_over += dt;
        } else {
            tanks_end(state);
        }
    }
}

// Free memory and resources
void tanks_free(tanks_state_t *state) {
    // Free tank images
    SDL_DestroyTexture(state->player1.image);
    SDL_DestroyTexture(state->player2.image);

    while ( Mix_PlayingMusic() ) ;
	Mix_FreeChunk(bullet_shot_wav);
    Mix_FreeChunk(player_won_wav);
	
	// Quit SDL_Mixer
	Mix_CloseAudio();
}
