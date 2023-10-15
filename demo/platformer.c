#include "body.h"
#include "color.h"
#include "forces.h"
#include "list.h"
#include "polygon.h"
#include "scene.h"
#include "sdl_wrapper.h"
#include "state.h"
#include "vector.h"
#include "platform.h"
#include <assert.h>
#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <emscripten.h>

// Constants for screen width and height
const vector_t WINDOW_PLATFORMER = (vector_t){.x = 1000, .y = 500};
const vector_t CENTER_PLATFORMER = (vector_t){.x = 500, .y = 250};

// Platform Parameters
const double PLATFORM_MASS = 10;
const vector_t PLATFORM_SIZE = {100, 20};
const rgb_color_t PLATFORM_COLOR = {1, 0, 0};

const double TWO_PI_PLATFORMER = 2.0 * M_PI;
const double WIND_CHANCE = 0.1;

// Testing Parameters
const vector_t TEST_VELOCITY = {0, -30};
const rgb_color_t BODY_COLOR_PLATFORMER = {0, 0, 0};

// Player Parameters
const vector_t PLAYER_ACCELERATION = {0, -3};
const vector_t GRAVITY_VECTOR = {0, -500};
const size_t PLAYER_LIVES = 3;
const rgb_color_t ZERO_SCORE_COLOR = {0, 0, 0}; //black
const rgb_color_t MID_SCORE_COLOR = {0.0, 0.0, 1.0}; //dark blue
const rgb_color_t TOP_SCORE_COLOR = {0.627, 0.125, 0.941}; //purple
const size_t JUMP_TIME = 5;
const double GAMMA = 50;
const vector_t WIND_VECTOR = {500, -500};
const int WIND_MODULUS = 5; //This is modulused with the current time to
//see if the wind system is activated, higher number = less frequent

// Powerup Parameters
const double POWERUP_RADIUS = 15;
const size_t POWERUP_GRADIENT_SIZE = 100;
const double POWERUP_MASS = .0001;
const rgb_color_t POWERUP_COLOR = {0, 1, 0};

// SDL variables
SDL_Renderer *platformer_renderer = NULL;
SDL_Rect round_location = {10, 10, 100, 100};
SDL_Rect gameover_location = {150, 100, 700, 700};
SDL_Rect wind_location = {10, 50, 100, 150};

double time_step = 0;
const double TIME_DELAY = 2;

// Structure for players
typedef struct player_struct{
    body_t *player;
    size_t score;
    size_t lives;
    bool is_alive;
    bool is_winner;
    bool on_jump;
    time_t start_of_jump;
    bool has_powerup;
    bool first_jump;
} player_struct_t;

// Structure for game state
typedef struct platformer_state {
    scene_t *scene;
    body_t *player;
    player_struct_t *player_1;
    player_struct_t *player_2;
    size_t round_number;
    bool wind; //bool for if wind is active
    bool game_over;

    //pointer to textures
    SDL_Texture *round1_texture; 
    SDL_Texture *round2_texture;
    SDL_Texture *round3_texture;
    SDL_Texture *gameover_texture;
    SDL_Texture *wind_texture;
} platformer_state_t;

// Structure for game states
typedef struct state {
    scene_t *scene;
    state_t *tanks_state;
    platformer_state_t *platformer_state;
    bool switch_game;
    size_t curr_game;
} state_t;

body_t *draw_platform(SDL_Renderer *platformer_renderer, scene_t *scene, vector_t size,
 rgb_color_t color, double x, double y) {
    list_t *shape = platform_generate_rectangle(size.x, size.y, x, y, TWO_PI_PLATFORMER);
    body_t *platform = platform_init(shape, PLATFORM_MASS, color, NULL, NULL);
    scene_add_body(scene, platform);
    platform_draw(platformer_renderer, platform);

    return platform;
}

body_t *draw_player(SDL_Renderer *platformer_renderer, scene_t *scene, vector_t size, rgb_color_t color) {
    list_t *shape = platform_generate_rectangle(size.x, size.y, WINDOW_PLATFORMER.x / 3, 300, TWO_PI_PLATFORMER);
    body_t *player = body_init(shape, PLATFORM_MASS, color);
    body_set_velocity(player, VEC_ZERO);
    body_set_acceleration(player, PLAYER_ACCELERATION);
    scene_add_body(scene, player);
    platform_draw(platformer_renderer, player);

    return player;
}

void draw_falling_rectangles(SDL_Renderer *platformer_renderer, scene_t *scene, body_t *player) {
    double random_x = (PLATFORM_SIZE.x / 2) + ((rand() % (int)(WINDOW_PLATFORMER.x - PLATFORM_SIZE.x)));
    body_t *platform = draw_platform(platformer_renderer, scene, PLATFORM_SIZE, PLATFORM_COLOR, random_x, WINDOW_PLATFORMER.y - PLATFORM_SIZE.y);

    body_set_velocity(platform, TEST_VELOCITY);
    create_platform_for_body(scene, platform, player);
}

body_t *draw_powerup(scene_t *scene, double center_x, double center_y) {
    double curr_angle = 0;
    double vert_angle = TWO_PI_PLATFORMER / POWERUP_GRADIENT_SIZE;
    double x;
    double y;
    list_t *vertices = list_init(POWERUP_GRADIENT_SIZE, free);
    assert(vertices != NULL);
    for (size_t i = 0; i < POWERUP_GRADIENT_SIZE; i++) {
        x = cos(curr_angle) * POWERUP_RADIUS + center_x;
        y = sin(curr_angle) * POWERUP_RADIUS + center_y;

        vector_t *vec_ptr = malloc(sizeof(vector_t));
        vec_ptr->x = x;
        vec_ptr->y = y;
        list_add(vertices, vec_ptr);
        curr_angle += vert_angle;
    }

    char* powerup_label = malloc(sizeof(char) * 5);
    strcpy(powerup_label, "pow");
    body_t *powerup = 
        body_init_with_info(vertices, POWERUP_MASS, POWERUP_COLOR, powerup_label, free);
    scene_add_body(scene, powerup);
    return powerup;
}


void create_double_jump(scene_t *scene, double center_x, double center_y, body_t *player) {
    body_t *powerup = draw_powerup(scene, center_x, center_y);
    create_double_jump_collision(scene, player, powerup);
}

void wind(body_t *player) {
    double num = rand() / ((double) RAND_MAX);

    if (num < WIND_CHANCE) {
        body_set_velocity(player, (vector_t){0, 0});
    }
}

void on_key_platformer(char key, key_event_type_t type, double held_time, state_t *overall_state) {
    platformer_state_t *state = overall_state->platformer_state;
    scene_t *scene = state->scene;
  if (type == KEY_PRESSED) {
    switch (key) {
        case LEFT_ARROW:
            body_set_velocity(state->player_1->player, (vector_t){-60, 0});
            break;
        case SDLK_a:
            body_set_velocity(state->player_2->player, (vector_t){-60, 0});
            break;
        case RIGHT_ARROW:
            body_set_velocity(state->player_1->player, (vector_t){60, 0});
            break;
        case SDLK_d:
            body_set_velocity(state->player_2->player, (vector_t){60, 0});
            break;
        case DOWN_ARROW:
            body_set_velocity(state->player_1->player, (vector_t){0, -60});
            break;
        case SDLK_s:
            body_set_velocity(state->player_2->player, (vector_t){0, -60});
            break;
        case UP_ARROW:
            body_set_velocity(state->player_1->player, (vector_t){0, 60});
            break;
        case SDLK_w:
            body_set_velocity(state->player_2->player, (vector_t){0, 60});
            break;
    }
  }
}

platformer_state_t *platformer_init() {
    vector_t min = VEC_ZERO;
    vector_t max = WINDOW_PLATFORMER;

    SDL_Init(SDL_INIT_AUDIO);

    platformer_renderer = sdl_return_renderer();

    platformer_state_t *state = malloc(sizeof(platformer_state_t));
    assert(state != NULL);

    scene_t *scene = scene_init();
    state -> round_number = 0;
    state -> wind = false;

    body_t *player = draw_player(platformer_renderer, scene, PLATFORM_SIZE, BODY_COLOR_PLATFORMER);
    player_struct_t *player_1_struct = malloc(sizeof(player_struct_t));
    player_1_struct->player = player;
    player_1_struct->lives = PLAYER_LIVES;
    player_1_struct->score = 0;
    player_1_struct->is_alive = true;
    state->player_1 = player_1_struct;

    body_t *player2 = draw_player(platformer_renderer, scene, PLATFORM_SIZE, BODY_COLOR_PLATFORMER);
    player_struct_t *player_2_struct = malloc(sizeof(player_struct_t));
    player_2_struct->player = player2;
    player_2_struct->lives = PLAYER_LIVES;
    player_2_struct->score = 0;
    player_2_struct->is_alive = true;
    state->player_2 = player_2_struct;

    
    create_double_jump(scene, 150, 150, player);
    state->scene = scene;
    state->player = player;
    state->game_over = false;

    //load textures
    state->round1_texture = IMG_LoadTexture(platformer_renderer, 
    "assets/round1.png");
    state->round2_texture = IMG_LoadTexture(platformer_renderer, 
    "assets/round2.png");
    state->round3_texture = IMG_LoadTexture(platformer_renderer, 
    "assets/round3.png");
    state->gameover_texture = IMG_LoadTexture(platformer_renderer, 
    "assets/gameover.png");
    state->wind_texture = IMG_LoadTexture(platformer_renderer, 
    "assets/wind.png");

    sdl_on_key(on_key_platformer);
    return state;
}

void redraw_player(SDL_Renderer *platformer_renderer, scene_t *scene, vector_t size, player_struct_t *player_struct) {
    rgb_color_t color = ZERO_SCORE_COLOR;
    if(player_struct->score == 1){
        color = MID_SCORE_COLOR;
    }
    if(player_struct->score == 2){
        color = TOP_SCORE_COLOR;
    }
    list_t *shape = platform_generate_rectangle(size.x, size.y, WINDOW_PLATFORMER.x / 3, 300, TWO_PI_PLATFORMER);
    body_t *player = body_init(shape, PLATFORM_MASS, color);
    body_set_velocity(player, VEC_ZERO);
    body_set_acceleration(player, PLAYER_ACCELERATION);
    scene_add_body(scene, player);
    platform_draw(platformer_renderer, player);

    player_struct->player = player;
}

platformer_state_t *emscripten_new_game_init(platformer_state_t *state, player_struct_t *player_1, player_struct_t *player_2){
    scene_t *scene = state->scene;
    sdl_clear();

    platformer_renderer = sdl_return_renderer();

    redraw_player(platformer_renderer, scene, PLATFORM_SIZE, player_1);
    redraw_player(platformer_renderer, scene, PLATFORM_SIZE, player_2);

    create_double_jump(scene, 150, 150, state->player_1->player);
    state->player = state->player_1->player;

    return state;

}

//this is a seprate function to manage the player updates
//just so there is less repeated code
void update_player(platformer_state_t *state, size_t player_num){
    player_struct_t *player = NULL;
    player_struct_t *other = NULL;
    scene_t *scene = state->scene;
    if(player_num == 1){
        player = state->player_1;
        other = state->player_2;
    }
    else{
        player = state->player_2;
        other = state->player_1;
    }

    body_t *body = player->player;
    body_t *oBody = other->player;
    if(get_window_position(body_get_centroid(body), CENTER_PLATFORMER).y >=
          WINDOW_PLATFORMER.y){
        other->score = other->score + 1;
        body_remove(body);
        body_remove(oBody);

        size_t body_count = scene_bodies(scene);
        for (size_t i = 0; i < body_count; i++) {
            body_t *body = scene_get_body(scene, i);
            list_t *shape = body_get_shape(body);
            char* info = (char*)body_get_info(body);
            if(info && !strcmp(info, "pow")){
               scene_remove_body(scene, i);
               body_count --;
               i --;
            }
        }
        state -> round_number += 1;
        emscripten_new_game_init(state, state->player_1, state->player_2);
    }

    vector_t vel = body_get_velocity(body);
    body_add_force(body, GRAVITY_VECTOR);

    time_t raw_time;
    time(&raw_time);
    if((int)raw_time % WIND_MODULUS == 0){
        body_add_force(body, WIND_VECTOR);
        state -> wind = true;
    } else{
        state -> wind = false;
    }


}

void platformer_end(platformer_state_t *state) {
    state->game_over = true;
    SDL_RenderClear(platformer_renderer);
    SDL_DestroyTexture(state->round1_texture);
    SDL_DestroyTexture(state->round2_texture);
    SDL_DestroyTexture(state->round3_texture);
    SDL_DestroyTexture(state->gameover_texture);
    SDL_DestroyTexture(state->wind_texture);

    return;
}

void platformer_main(platformer_state_t *state, double game_dt) {
    scene_t *scene = state->scene;
    double dt = game_dt;
    time_step += dt;

    SDL_SetRenderDrawColor(platformer_renderer, 173, 216, 230, 255);
    //set backround color
    SDL_RenderClear(platformer_renderer);
    SDL_Log("immage %p\n", state->round1_texture);
    SDL_Log("location x: %d y: %d h: %d w: %d\n", round_location.x, round_location.y, round_location.h, round_location.w );
    SDL_SetRenderDrawColor(platformer_renderer, 173, 0, 0, 0);
   
   //Draw round number / gameover
   size_t round = state -> round_number;
   if(round < 1){
        SDL_RenderCopy(platformer_renderer, state->round1_texture, 
        NULL, &round_location);
   } else if(round == 1){
        SDL_RenderCopy(platformer_renderer, 
        state->round2_texture, NULL, &round_location);
   } else if(round == 2){
        SDL_RenderCopy(platformer_renderer, 
        state->round3_texture, NULL, &round_location);
   } else{
        SDL_RenderCopy(platformer_renderer, 
        state->gameover_texture, NULL, &gameover_location);
        platformer_end(state);
   }

   //Draw wind
   if(state -> wind){
        SDL_RenderCopy(platformer_renderer, 
        state->wind_texture, NULL, &wind_location);
   }
   
    sdl_render_scene(scene);
    
    scene_tick(scene, dt);
    sdl_show();

    //remove everything taged for removal
    size_t body_count = scene_bodies(scene);
    for (size_t i = 0; i < body_count; i++) {
        body_t *body = scene_get_body(scene, i);
        list_t *shape = body_get_shape(body);
        if(body_is_removed(body)){
            scene_remove_body(scene, i);
            body_count --;
            i --;
        }
    }
    
    //check player behavior 
    update_player(state, 1);
    update_player(state, 2);

    // Generate new platforms
    if (time_step > TIME_DELAY) {
        draw_falling_rectangles(platformer_renderer, scene, state->player);
        time_step = 0;
        wind(state->player);
    }

    //remove old platforms
    for(size_t i = 1; i < scene_bodies(scene); i++){
        body_t *body = scene_get_body(scene, i);
        if(get_window_position(body_get_centroid(body), CENTER_PLATFORMER).x > 20000 ||
        get_window_position(body_get_centroid(body), CENTER_PLATFORMER).x < -20000){
            scene_remove_body(scene, i);
        }
    }
}

void platformer_free(platformer_state_t *state) {
    scene_free(state->scene);
    free(state->player_1->player);
    free(state->player_1);
    free(state->player_2->player);
    free(state->player_2);
    free(state);
}
