#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#include "body.h"
#include "list.h"
#include "color.h"
#include "scene.h"
#include <SDL2/SDL.h>

body_t *platform_init(list_t *shape, double mass, rgb_color_t color,
                            void *info, free_func_t info_freer);

list_t *platform_generate_rectangle(double length, double height, double center_x, 
                           double center_y, double TWO_PI);

void platform_draw(SDL_Renderer *SDL_Renderer, body_t *platform);

void platform_free(body_t *platform);

void create_platform_for_body(scene_t *scene, body_t *platform, body_t *body);

#endif
