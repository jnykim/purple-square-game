#include "platform.h"
#include "body.h"
#include "list.h"
#include "color.h"
#include "forces.h"
#include "collision.h"
#include <SDL2/SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "assert.h"

body_t *platform_init(list_t *shape, double mass, rgb_color_t color,
                            void *info, free_func_t info_freer) {
    body_t *platform = body_init_with_info(shape, mass, color, info, info_freer);
    return platform;
}

vector_t *platform_make_rectangle_helper(double length, double angle, double center_x,
                                double center_y) {
  vector_t *vec_ptr = malloc(sizeof(vector_t));
  vec_ptr->x = length * cos(angle) + center_x;
  vec_ptr->y = length * sin(angle) + center_y;
  return vec_ptr;
}

list_t *platform_generate_rectangle(double length, double height, double center_x, double center_y, double TWO_PI) {
    list_t *vertices = list_init(4, free);
    assert(vertices != NULL);
    double half_diagonal_length = 0.5 * sqrt(pow(length, 2) + pow(height, 2));

    double corner1_angle = atan(height / length);
    vector_t *vertex1 = platform_make_rectangle_helper(half_diagonal_length, corner1_angle,
                                                center_x, center_y);
    list_add(vertices, vertex1);

    double corner2_angle = TWO_PI / 2 - corner1_angle;
    vector_t *vertext2 = platform_make_rectangle_helper(half_diagonal_length,
                                                corner2_angle, center_x, center_y);
    list_add(vertices, vertext2);

    double corner3_angle = TWO_PI / 2 + corner1_angle;
    vector_t *vertex3 = platform_make_rectangle_helper(half_diagonal_length, corner3_angle,
                                                center_x, center_y);
    list_add(vertices, vertex3);

    double corner4_angle = TWO_PI - corner1_angle;
    vector_t *vertex4 = platform_make_rectangle_helper(half_diagonal_length, corner4_angle,
                                                center_x, center_y);
    list_add(vertices, vertex4);

    return vertices;
}

void platform_draw(SDL_Renderer *renderer, body_t *platform) {  
    // Assuming the body shape is a polygon
    list_t *shape = body_get_shape(platform);
    rgb_color_t color = body_get_color(platform);

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);

    for (size_t i = 0; i < list_size(shape); i++) {
        vector_t *vertex1 = list_get(shape, i);
        vector_t *vertex2 = list_get(shape, (i + 1) % list_size(shape));
        SDL_RenderDrawLine(renderer, vertex1->x, vertex1->y, vertex2->x, vertex2->y);
    }
}

void platform_free(body_t *platform) {
    body_free(platform);
}

void create_platform_for_body(scene_t *scene, body_t *platform, body_t *body) {
    create_platform_collision(scene, platform, body);
}
