#include "body.h"
#include "color.h"
#include "forces.h"
#include "list.h"
#include "polygon.h"
#include "scene.h"
#include "sdl_wrapper.h"
#include "state.h"
#include "vector.h"
#include "tanks.h"
#include "platformer.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// Constants for screen width and height
const vector_t WINDOW = (vector_t){.x = 1000, .y = 500};
const vector_t CENTER = (vector_t){.x = 500, .y = 250};
const SDL_Rect popup_location = {0, 0, 1000, 500};
const SDL_Rect mainplayer1_location = {150, 80, 200, 80};
const SDL_Rect mainplayer2_location = {650, 80, 200, 80};

// Body constants
const double TWO_PI = 2.0 * M_PI;
const size_t RADIUS = 20;
const rgb_color_t BLACK_COLOR = (rgb_color_t){.r = 0, .g = 0, .b = 0};
const rgb_color_t GRAY_COLOR = (rgb_color_t){.r = 0.3059, .g = 0.3176, .b = 0.3412};
const rgb_color_t PURPLE_COLOR = (rgb_color_t){.r = 150.0 / 255.0 , .g = 101.0 / 255.0, .b = 247.0 / 255.0};

SDL_Renderer *square_renderer = NULL;

// Structure for game states
typedef struct main_player {
    size_t score;
    body_t *body;
} main_player_t;

typedef struct state {
    scene_t *scene;
    tanks_state_t *tanks_state;
    platformer_state_t *platformer_state;
    bool switch_game;
    size_t curr_game;
    main_player_t player1;
    main_player_t player2;
    size_t curr_popup;
} state_t;

// Structures for tanks state
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

typedef struct bullet {
    int shape;
    size_t player_num;
    SDL_Texture *image;
    SDL_Rect location;
    body_t *body;
} bullet_t;

typedef struct crater {
    int shape;
    SDL_Texture *image;
    SDL_Rect location;
} crater_t;

typedef struct boulder {
    SDL_Texture *image;
    SDL_Rect location;
} boulder_t;

typedef struct landscape {
    SDL_Texture *image;
    SDL_Rect location;
} landscape_t;

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

// Structures for platformer state
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

list_t *make_rect_square(SDL_Rect rect) {
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

body_t *make_player_body(size_t stage, size_t center_x, size_t center_y) {
    list_t *vertices;
    rgb_color_t color;

    switch (stage) {
        case 0: { // Gray circle
            double curr_angle = 0;
            double vert_angle = TWO_PI / 20;
            double x;
            double y;
            vertices = list_init(20, free);
            assert(vertices != NULL);
            for (size_t i = 0; i < 20; i++) {
                x = cos(curr_angle) * RADIUS + center_x;
                y = sin(curr_angle) * RADIUS + center_y;
                vector_t *vec_ptr = malloc(sizeof(vector_t));
                vec_ptr->x = x;
                vec_ptr->y = y;
                list_add(vertices, vec_ptr);
                curr_angle += vert_angle;
            }

            color = GRAY_COLOR;
            break;
        }
        case 1: { // Gray line
            vertices = list_init(4, free);
            vector_t *v1 = malloc(sizeof(vector_t));
            v1->x = center_x - 50 / sqrt(3) * 4; v1->y = center_y + 1.25;
            vector_t *v2 = malloc(sizeof(vector_t));
            v2->x = center_x + 50 / sqrt(3) * 4; v2->y = center_y + 1.25;
            vector_t *v3 = malloc(sizeof(vector_t));
            v3->x = center_x + 50 / sqrt(3) * 4; v3->y = center_y - 1.25;
            vector_t *v4 = malloc(sizeof(vector_t));
            v4->x = center_x - 50 / sqrt(3) * 4; v4->y = center_y - 1.25;

            list_add(vertices, v1);
            list_add(vertices, v2);
            list_add(vertices, v3);
            list_add(vertices, v4);
            
            color = GRAY_COLOR;
            break;
        }
        case 2: { // Gray top angle
            vertices = list_init(6, free);
            vector_t *v1 = malloc(sizeof(vector_t));
            v1->x = center_x - 50 / sqrt(3) * 2 - 3; v1->y = center_y - 50;
            vector_t *v2 = malloc(sizeof(vector_t));
            v2->x = center_x - 50 / sqrt(3) * 2 + 1; v2->y = center_y - 50;
            vector_t *v3 = malloc(sizeof(vector_t));
            v3->x = center_x, v3->y = center_y + 50 - 3;
            vector_t *v4 = malloc(sizeof(vector_t));
            v4->x = center_x + 50 / sqrt(3) * 2 - 1; v4->y = center_y - 50;
            vector_t *v5 = malloc(sizeof(vector_t));
            v5->x = center_x + 50 / sqrt(3) * 2 + 3; v5->y = center_y - 50;
            vector_t *v6 = malloc(sizeof(vector_t));
            v6->x = center_x, v6->y = center_y + 50 + 3;

            list_add(vertices, v1);
            list_add(vertices, v2);
            list_add(vertices, v3);
            list_add(vertices, v4);
            list_add(vertices, v5);
            list_add(vertices, v6);

            color = GRAY_COLOR;
            break;
        }
        case 3: { // Gray triangle
            vertices = list_init(3, free);

            vector_t *v1 = malloc(sizeof(vector_t));
            v1->x = center_x, v1->y = center_y + 50;
            vector_t *v2 = malloc(sizeof(vector_t));
            v2->x = center_x - 50 / sqrt(3) * 2; v2->y = center_y - 50;
            vector_t *v3 = malloc(sizeof(vector_t));
            v3->x = center_x + 50 / sqrt(3) * 2; v3->y = center_y - 50;

            list_add(vertices, v1);
            list_add(vertices, v2);
            list_add(vertices, v3);

            color = GRAY_COLOR;
            break;
        }
        case 4: { // Purple triangle
            vertices = list_init(3, free);

            vector_t *v1 = malloc(sizeof(vector_t));
            v1->x = center_x, v1->y = center_y + 50;
            vector_t *v2 = malloc(sizeof(vector_t));
            v2->x = center_x - 50 / sqrt(3) * 2; v2->y = center_y - 50;
            vector_t *v3 = malloc(sizeof(vector_t));
            v3->x = center_x + 50 / sqrt(3) * 2; v3->y = center_y - 50;

            list_add(vertices, v1);
            list_add(vertices, v2);
            list_add(vertices, v3);

            color = PURPLE_COLOR;
            break;
        }
        case 5: { // Purple diamond
            vertices = list_init(4, free);
            
            vector_t *v1 = malloc(sizeof(vector_t));
            v1->x = center_x; v1->y = center_y + 50 * sqrt(2);
            vector_t *v2 = malloc(sizeof(vector_t));
            v2->x = center_x + 50 * sqrt(2); v2->y = center_y;
            vector_t *v3 = malloc(sizeof(vector_t));
            v3->x = center_x; v3->y = center_y - 50 * sqrt(2);
            vector_t *v4 = malloc(sizeof(vector_t));
            v4->x = center_x - 50 * sqrt(2); v4->y = center_y;

            list_add(vertices, v1);
            list_add(vertices, v2);
            list_add(vertices, v3);
            list_add(vertices, v4);

            color = PURPLE_COLOR;
            break;
        }
        case 6: { // Purple square
            vertices = list_init(4, free);
            
            vector_t *v1 = malloc(sizeof(vector_t));
            v1->x = center_x - 50; v1->y = center_y + 50;
            vector_t *v2 = malloc(sizeof(vector_t));
            v2->x = center_x + 50; v2->y = center_y + 50;
            vector_t *v3 = malloc(sizeof(vector_t));
            v3->x = center_x + 50; v3->y = center_y - 50;
            vector_t *v4 = malloc(sizeof(vector_t));
            v4->x = center_x - 50; v4->y = center_y - 50;

            list_add(vertices, v1);
            list_add(vertices, v2);
            list_add(vertices, v3);
            list_add(vertices, v4);

            color = PURPLE_COLOR;
            break;
        }
    }

    return body_init(vertices, 1.0, color);
}

void on_key_square(char key, key_event_type_t type, double held_time, state_t *state) {
    scene_t *scene = state->scene;

    if (type == KEY_PRESSED) {
        switch (key) {
            case SDLK_SPACE:
                switch (state->curr_popup) {
                    case 1:
                        state->curr_popup = 2;
                        break;
                    case 2:
                        state->curr_popup = 3;
                        state->switch_game = true;
                        state->curr_game = 1;
                        break;
                    case 3:
                        if (state->player1.score + state->player2.score < 30) {
                            state->switch_game = true;
                            state->curr_game = 1;
                        } else {
                            state->curr_popup = 4;
                        }
                        break;
                    case 4:
                        state->curr_popup = 5;
                        state->switch_game = true;
                        state->curr_game = 2;
                        break;
                }
        }
    }
}

state_t *emscripten_init() {
    vector_t min = VEC_ZERO;
    vector_t max = WINDOW;
    sdl_init(min, max);
    IMG_Init(IMG_INIT_PNG);
    square_renderer = sdl_return_renderer();

    state_t *state = malloc(sizeof(state_t));
    scene_t *scene = scene_init();

    state->scene = scene;
    state->switch_game = false;
    state->curr_game = 0;
    state->curr_popup = 1;

    // Initialize main players
    state->player1.score = 0;
    state->player2.score = 0;

    state->player1.body = make_player_body(1, CENTER.x - 200, CENTER.y);
    state->player2.body = make_player_body(2, CENTER.x + 200, CENTER.y);

    scene_add_body(scene, state->player1.body);
    scene_add_body(scene, state->player2.body);

    sdl_on_key(on_key_square);

    return state;
}

void emscripten_main(state_t *state) {
    scene_t *scene = state->scene;
    double dt = time_since_last_tick();

    scene_tick(scene, dt);

    // Initialize mini game
    if (state->switch_game) {
        switch (state->curr_game) {
            case 1:
                state->tanks_state = tanks_init();
                break;
            case 2:
                state->platformer_state = platformer_init();
                break;
        }
        state->switch_game = false;
    }

    // Run main loop of mini game
    switch (state->curr_game) {
        case 0:
            // Draw background
            SDL_SetRenderDrawColor(square_renderer, 185, 237, 232, 255);

            // Clear the screen
            SDL_RenderClear(square_renderer);

            // Show curr player bodies
            scene_remove_body(scene, 0);
            scene_remove_body(scene, 1);

            state->player1.body = make_player_body(state->player1.score / 10, CENTER.x - 200, CENTER.y);
            state->player2.body = make_player_body(state->player2.score / 10, CENTER.x + 200, CENTER.y);

            scene_add_body(scene, state->player1.body);
            scene_add_body(scene, state->player2.body);

            sdl_render_scene(scene);

            // Display popup if needed
            switch (state->curr_popup) {
                case 1:
                    SDL_RenderCopy(square_renderer, IMG_LoadTexture(square_renderer, "assets/story_blurb.png"), NULL, &popup_location);
                    break;
                case 2:
                    SDL_RenderCopy(square_renderer, IMG_LoadTexture(square_renderer, "assets/tanks_instructions.png"), NULL, &popup_location);
                    break;
                case 3:
                    SDL_RenderCopy(square_renderer, IMG_LoadTexture(square_renderer, "assets/press_space.png"), NULL, &popup_location);
                    SDL_RenderCopy(square_renderer, IMG_LoadTexture(square_renderer, "assets/mainplayer1.png"), NULL, &mainplayer1_location);
                    SDL_RenderCopy(square_renderer, IMG_LoadTexture(square_renderer, "assets/mainplayer2.png"), NULL, &mainplayer2_location);
                    break;
                case 4:
                    SDL_RenderCopy(square_renderer, IMG_LoadTexture(square_renderer, "assets/platformer_instructions.png"), NULL, &popup_location);
                    break;
                case 6:
                    SDL_RenderCopy(square_renderer, IMG_LoadTexture(square_renderer, "assets/winner_screen.png"), NULL, &popup_location);
                    SDL_RenderCopy(square_renderer, IMG_LoadTexture(square_renderer, "assets/mainplayer1.png"), NULL, &mainplayer1_location);
                    SDL_RenderCopy(square_renderer, IMG_LoadTexture(square_renderer, "assets/mainplayer2.png"), NULL, &mainplayer2_location);
                    break;

            }

            sdl_show();
            sdl_on_key(on_key_square);
            break;
        case 1:
            tanks_main(state->tanks_state, dt);
            // Check if mini game is over
            if (state->tanks_state->game_over) {
                state->curr_game = 0;
                if (state->tanks_state->winner == 1) {
                    state->player1.score += 10;
                } else {
                    state->player2.score += 10;
                }

                if (state->player1.score + state->player2.score >= 30) {
                    state->curr_popup = 4;
                }
            }
            break;
        case 2:
            platformer_main(state->platformer_state, dt);
            // Check if platformer game is over
            if (state->platformer_state->game_over) {
                state->curr_game = 0;
                state->player1.score += state->platformer_state->player_2->score * 10;
                state->player2.score += state->platformer_state->player_1->score * 10;

                // Make player w/ highest score be the purple square
                if (state->player1.score > state->player2.score) {
                    state->player1.score = 60;
                } else if (state->player1.score == state->player2.score) {
                    state->player1.score = 60;
                    state->player2.score = 60;
                } else {
                    state->player2.score = 60;
                }

                state->curr_popup = 6;
            }
            break;
    }
}

void emscripten_free(state_t *state) {

}
