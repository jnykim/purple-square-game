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

typedef struct player player_t;

typedef struct bullet bullet_t;

typedef struct crater crater_t;

typedef struct boulder boulder_t;

typedef struct landscape landscape_t;

typedef struct tanks_state tanks_state_t;

//oid run_tanks();

tanks_state_t *tanks_init();

void tanks_main(tanks_state_t *state, double game_dt);

void tanks_free(tanks_state_t *state);

void tanks_end(tanks_state_t *state);


