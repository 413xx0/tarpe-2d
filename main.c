#include <errno.h>
#include <libtarpe2d.h>
#include <libtarpe2d_draw.h>


#define WIDTH 1280
#define HEIGHT 720
/*
#define SHAPES_DIST 20
#define _SIDE_SHAPES_COUNT(SIDE_LEN) \
	((SIDE_LEN + 1) / SHAPES_DIST + ((SIDE_LEN + 1) % SHAPES_DIST == 0 ? 0 : 1))
#define SHAPES_ROW_COUNT (_SIDE_SHAPES_COUNT(WIDTH))
#define SHAPES_COL_COUNT (_SIDE_SHAPES_COUNT(HEIGHT))
#define SHAPES_COUNT (SHAPES_ROW_COUNT * SHAPES_COL_COUNT)
*/
#define SHAPES_COUNT 5
#define NO_VSYNC

#ifdef NO_VSYNC
#	define MAX_FPS 60
#	define MIN_DT (1. / MAX_FPS)
#endif


#define shape_check_null(shape_ptr, ret, cleanup_label)                    \
	do {                                                               \
		if (shape_ptr == NULL)                                     \
		{                                                          \
			printf("Couldn't allocate memory for a shape!\n"); \
			ret = ENOMEM;                                      \
			goto cleanup_label;                                \
		}                                                          \
	} while (0);


int main(void)
{
	int ret = 0;

	struct shape * shapes = malloc(sizeof(struct shape) * SHAPES_COUNT);
	if (shapes == NULL)
	{
		printf("Couldn't allocate memory for shapes array!\n");
		return ENOMEM;
	}

	struct vec2 * trajs = malloc(sizeof(struct vec2) * SHAPES_COUNT * 900 * MAX_FPS);

	// clang-format off
	// for (size_t y = 0; y < SHAPES_COL_COUNT; ++y)
	// {
	// 	for (size_t x = 0; x < SHAPES_ROW_COUNT; ++x)
	// 	{
	// 		shapes[SHAPES_ROW_COUNT * y + x].color = (SDL_Color){0xff, 0, 0, 0xff};
	// 		shapes[SHAPES_ROW_COUNT * y + x].rb_shape = (struct rb_shape_base *)rb_circle_new(
	// 			SHAPES_DIST / 2., 50,
	// 			&(struct vec2){x * SHAPES_DIST, y * SHAPES_DIST}, &(struct vec2){0, 0},
	// 			0, 0
	// 		);
	// 	}
	// }

	shapes[0].color = (SDL_Color){0xff, 0, 0, 0xff};
	shapes[0].rb_shape = (struct rb_shape_base *)rb_circle_new(
		50, 100,
		&(struct vec2){WIDTH / 2., HEIGHT / 2.}, &(struct vec2){0, 0},
		0, 0
	);

	shapes[1].color = (SDL_Color){0x3f, 0x9f, 0x5f, 0xff};
	shapes[1].rb_shape = (struct rb_shape_base *)rb_circle_new(
		10, 0.2,
		&(struct vec2){WIDTH / 1.5, HEIGHT / 2.}, &(struct vec2){0, 300},
		0, 0
	);

	shapes[2].color = (SDL_Color){0x3f, 0x9f, 0x5f, 0xff};
	shapes[2].rb_shape = (struct rb_shape_base *)rb_circle_new(
		10, 0.2,
		&(struct vec2){WIDTH / 1.2, HEIGHT / 2.}, &(struct vec2){0, 100},
		0, 0
	);

	shapes[3].color = (SDL_Color){0x3f, 0x9f, 0x5f, 0xff};
	shapes[3].rb_shape = (struct rb_shape_base *)rb_circle_new(
		20, 0.5,
		&(struct vec2){WIDTH, HEIGHT / 2.}, &(struct vec2){0, 140},
		0, 0
	);

	shapes[4].color = (SDL_Color){0x7f, 0xff, 0, 0xff};
	shapes[4].rb_shape = (struct rb_shape_base *)rb_circle_new(
		15, 0.5,
		&(struct vec2){WIDTH + 50, HEIGHT / 2.}, &(struct vec2){0, 200},
		0, 0
	);

	struct rb_ptr_array_iter iter = (struct rb_ptr_array_iter){
		.start = (struct rb_shape_base **)((int8_t *)shapes + offsetof(struct shape, rb_shape)),
		.step = sizeof(struct shape),
		.end = (struct rb_shape_base **)(shapes + SHAPES_COUNT),
		.rb_count = SHAPES_COUNT
	};

	// clang-format on

	GPU_Target * screen = tarpe2d_draw_init_flag(WIDTH,
						     HEIGHT,
						     true,
#ifdef NO_VSYNC
						     GPU_INIT_DISABLE_VSYNC
#else
						     GPU_DEFAULT_INIT_FLAGS
#endif
	);
	tarpe2d_draw_zoom_camera(screen, 0.4, 0.4);
	if (screen == NULL) goto cleanup_shapes;

	struct tarpe_config tarpe_cfg;
	tarpe_config_set_default(&tarpe_cfg);
	tarpe_cfg.grav_const = 1.67E+5;
	tarpe_init(&tarpe_cfg);

	int is_done = 0, frame = 0;
	uint64_t start_time;
	double_t dt;
	char fps_title[8] = {0};
	start_time = SDL_GetPerformanceCounter();
	tarpe2d_draw(screen, shapes, SHAPES_COUNT, true);
	while (!is_done)
	{
#ifdef NO_VSYNC
		tarpe2d_draw_wait_until_min_dt(MIN_DT, start_time);
#endif
		dt = tarpe2d_draw_get_dt(&start_time);
		snprintf(fps_title, 7, "%6.2f", 1. / dt);
		SDL_SetWindowTitle(SDL_GetWindowFromID(screen->context->windowID), fps_title);

		is_done = tarpe2d_draw_poll_events();

		for (int i = 0; i < SHAPES_COUNT; ++i)
		{
			vec2_copy(&trajs[frame * SHAPES_COUNT + i], &shapes[i].rb_shape->rb.pos);
		}
		
		if (tarpe_tick_ptr_arr_iter(&iter, dt))
		{
			printf("Couldn't allocate memory to tick rigidbodies!\n");
			ret = ENOMEM;
			goto cleanup_shapes;
		}

		tarpe2d_draw(screen, shapes, SHAPES_COUNT, false);

		for (struct vec2 * v = trajs; v < trajs + SHAPES_COUNT * 900 * MAX_FPS && v != NULL; ++v)
		{
			GPU_Pixel(screen, v->x, HEIGHT - v->y, (SDL_Color){0x7f, 0x7f, 0, 0xff});
		}

		GPU_Flip(screen);

		++frame;
		if (frame >= (SHAPES_COUNT * 900)) frame = 0;
	}

	GPU_Quit();

cleanup_shapes:
	if (shapes != NULL)
	{
		for (size_t i = 0; i < SHAPES_COUNT && shapes[i].rb_shape != NULL; ++i)
			rb_shape_delete(shapes[i].rb_shape);
	}
	free(shapes);

	return ret;
}
