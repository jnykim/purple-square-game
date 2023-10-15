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

typedef struct player_struct player_struct_t;

typedef struct platformer_state platformer_state_t;

platformer_state_t *platformer_init();

void platformer_main(platformer_state_t *state, double game_dt);

void platformer_end(platformer_state_t *state);
