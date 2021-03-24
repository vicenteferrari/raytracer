//
// Created by vfs on 3/21/2021.
//

#include <cstdio>
#include <cmath>
#include <SDL2/SDL.h>
#include <glm/glm.hpp>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

struct Color {
	u8 r;
	u8 g;
	u8 b;
	u8 a;

	Color operator *(float k) const {
		float new_r = ((float) r / 255.0f) * k;
		if (new_r > 1.0f) new_r = 1.0f;

		float new_g = ((float) g / 255.0f) * k;
		if (new_g > 1.0f) new_g = 1.0f;

		float new_b = ((float) b / 255.0f) * k;
		if (new_b > 1.0f) new_b = 1.0f;

		float new_a = ((float) a / 255.0f) * k;
		if (new_a > 1.0f) new_a = 1.0f;

		return {(u8) (new_r * 255), (u8) (new_g * 255), (u8) (new_b * 255), (u8) (new_a * 255)};
	}

	Color operator +(Color c) {
		s32 new_r = r + c.r;
		if (new_r > 255) new_r = 255;

		s32 new_g = g + c.g;
		if (new_g > 255) new_g = 255;

		s32 new_b = b + c.b;
		if (new_b > 255) new_b = 255;

		s32 new_a = a + c.a;
		if (new_a > 255) new_a = 255;

		return {(u8) new_r, (u8) new_g, (u8) new_b, (u8) new_a};
	}
};

struct Sphere {
	glm::vec3 center;
	float radius;
	Color color;
	float shininess;
	float reflectiveness;
};

struct AmbientLight {
	float intensity;
};

struct DirectionalLight {
	glm::vec3 direction;
	float intensity;
};

struct PointLight {
	glm::vec3 pos;
	float intensity;
};

const Color red = {255, 0, 0, 0};
const Color green = {0, 255, 0, 0};
const Color blue = {0, 0, 255, 0};
const Color white = {255, 255, 255, 0};
const Color black = {0, 0, 0, 0};
const Color yellow = {255, 255, 0, 0};
const Color purple = {255, 0, 255, 0};
const Color cyan = {0, 255, 255, 0};
const Color grey = {85, 85, 85, 0};

const Color background_color = grey;

SDL_Window *window;
SDL_Surface *backbuffer;
SDL_Surface *triplebuffer;

bool running = true;

float t = 0.0f;
float dt = 1.0f / 240.0f;
float accumulator = 0.0f;
u64 frame = 0;

s32 window_width = 1280;
s32 window_height = 720;

s32 canvas_width = 500;
s32 canvas_height = 500;

const float viewport_width = 1.0f;
const float viewport_height = 1.0f;
const float viewport_distance = 1.0f;

const u32 sphere_count = 3;
Sphere spheres[sphere_count];

AmbientLight ambient = {0.2f};
//DirectionalLight light = {{0.0f, -1.0f, 0.0f}, 0.8f};
PointLight pointlight;

void update_backbuffer() {

	float triplebuffer_ar = (float) triplebuffer->w / (float) triplebuffer->h;
	float window_ar = (float) backbuffer->w / (float) backbuffer->h;

	SDL_FillRect(backbuffer, NULL, 0x0);

	if (triplebuffer_ar > window_ar) {
		s32 window_height_without_bars = (s32) ((float) backbuffer->w / (float) triplebuffer_ar);
		s32 bar_height = ((s32) window_height - window_height_without_bars) / 2;

		SDL_FillRect(backbuffer, NULL, 0x0);

		SDL_Rect destination = {0, bar_height, backbuffer->w, window_height_without_bars};
		SDL_BlitScaled(triplebuffer, NULL, backbuffer, &destination);
	} else if (triplebuffer_ar < window_ar) {
		s32 window_width_without_bars = (s32) (window_height * triplebuffer_ar);
		s32 bar_width = (s32) ((window_width - window_width_without_bars) / 2);

		SDL_FillRect(backbuffer, NULL, 0x0);

		SDL_Rect destination = {bar_width, 0, window_width_without_bars, backbuffer->h};
		SDL_BlitScaled(triplebuffer, NULL, backbuffer, &destination);
	} else {
		SDL_BlitScaled(triplebuffer, NULL, backbuffer, NULL);
	}
}

void set_pixel(int x, int y, Color color) {
	assert(x >= -triplebuffer->w / 2);
	assert(x < triplebuffer->w / 2);
	assert(y >= -triplebuffer->h / 2);
	assert(y < triplebuffer->h / 2);

	x += triplebuffer->w / 2;
	y += triplebuffer->h / 2;

	y = (canvas_height - 1) - y;

	u32 color_as_u32 = (color.r << 16) | (color.g << 8) | color.b;

	u32 *memory = (u32 *) triplebuffer->pixels;
	u32 *pixel = memory + (y * triplebuffer->w + x);

	*pixel = color_as_u32;
}

void clear() {
	for (s32 x = -canvas_width / 2; x < canvas_width / 2; ++x) {
		for (s32 y = -canvas_height / 2; y < canvas_height / 2; ++y) {
			set_pixel(x, y, grey);
		}
	}
}

glm::vec3 canvas_to_viewport(s32 x, s32 y) {
	return {x * viewport_width / (float) canvas_width, y * viewport_height / (float) canvas_height, viewport_distance};
}

void intersect_ray_sphere(float *t1, float *t2, glm::vec3 O, glm::vec3 ray, Sphere sphere) {
	float r = sphere.radius;
	glm::vec3 CO = O - sphere.center;

	float a = glm::dot(ray, ray);
	float b = 2 * glm::dot(CO, ray);
	float c = glm::dot(CO, CO) - r*r;

	float discriminant = b*b - 4 * a * c;
	if (discriminant < 0) {
		*t1 = INFINITY;
		*t2 = INFINITY;
	}

	*t1 = (-b + glm::sqrt(discriminant)) / (2 * a);
	*t2 = (-b - glm::sqrt(discriminant)) / (2 * a);
}

void closest_intersection(float *closest_t, Sphere **closest_sphere, glm::vec3 O, glm::vec3 ray, float t_min, float t_max) {
	float t1, t2;

	for (u32 i = 0; i < sphere_count; ++i) {
		intersect_ray_sphere(&t1, &t2, O, ray, spheres[i]);

		if (t1 > t_min && t1 < t_max && t1 < *closest_t) {
			*closest_t = t1;
			*closest_sphere = &spheres[i];
		}
		if (t2 > t_min && t2 < t_max && t2 < *closest_t) {
			*closest_t = t2;
			*closest_sphere = &spheres[i];
		}
	}
}

float lighting(glm::vec3 point, glm::vec3 norm, glm::vec3 dir_to_camera, float shininess) {
	norm = glm::normalize(norm);
	dir_to_camera = glm::normalize(dir_to_camera);
	float intensity = 0.0f;

	intensity += ambient.intensity;

//	if (glm::dot(light.direction, norm) > 0) {
//		intensity += light.intensity * glm::dot(light.direction, norm);
//	}

	float shadow_t = INFINITY;
	Sphere *shadow_sphere = NULL;
	closest_intersection(&shadow_t, &shadow_sphere, point, pointlight.pos - point, 0.01f, 1.0f);

	if (shadow_sphere != NULL) {
		return intensity;
	}

	glm::vec3 pos_to_pointlight = glm::normalize(pointlight.pos - point);
	if (glm::dot(pos_to_pointlight, norm) > 0) {
		intensity += pointlight.intensity * glm::dot(pos_to_pointlight, norm);
	}

	// negate pos_to_pointlight because glm expects the incident ray to actually go towards the surface, unlike our mental model
	glm::vec3 reflected = glm::normalize(glm::reflect(-pos_to_pointlight, norm));

	float dot = glm::dot(dir_to_camera, reflected);
	if (dot > 0.0f) {
		float power = powf(dot, shininess);
		intensity += pointlight.intensity * power;
	}

	return intensity;
}

Color trace_ray(glm::vec3 O, glm::vec3 D, float t_min, float t_max, u32 depth) {
	float closest_t = INFINITY;
	Sphere *closest_sphere = NULL;

	closest_intersection(&closest_t, &closest_sphere, O, D, t_min, t_max);

	if (closest_sphere == NULL) return background_color;

	glm::vec3 pos = O + closest_t * D;
	glm::vec3 norm = glm::normalize(pos - closest_sphere->center);
	glm::vec3 dir_to_camera = glm::normalize(-D);

	Color local_color = closest_sphere->color * lighting(pos, norm, dir_to_camera, closest_sphere->shininess);

	float r = closest_sphere->reflectiveness;
	if (depth <= 0 || r <= 0.0f) {
		return local_color;
	}

	Color reflected_color = trace_ray(pos, glm::reflect(D, norm), 0.01f, INFINITY, depth - 1);

	return local_color * (1.0f - r) + reflected_color * (r);
}

void init() {
	printf("hello sailor\n");

	SDL_Init(SDL_INIT_VIDEO);

	window = SDL_CreateWindow("Raytracer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_width, window_height, SDL_WINDOW_RESIZABLE);
	backbuffer = SDL_GetWindowSurface(window);
	triplebuffer = SDL_CreateRGBSurfaceWithFormat(0, canvas_width, canvas_height, backbuffer->format->BitsPerPixel, backbuffer->format->format);
}

void update(float dt) {
	spheres[0].center.z += 0.5f * dt;
}

void render() {
	for (s32 x = -canvas_width / 2; x < canvas_width / 2; ++x) {
		for (s32 y = -canvas_height / 2; y < canvas_height / 2; ++y) {
			glm::vec3 D = canvas_to_viewport(x, y);

			Color color = trace_ray({0.0f, 0.0f, 0.0f}, D, 1.0f, INFINITY, 3);

			set_pixel(x, y, color);
		}
	}
}

int main(int argc, char *argv[]) {
	init();

	spheres[0] = {(glm::vec3) {0.0f, -1.0f, 3.0f}, 1.0f, red, 32.0f, 0.2f};
	spheres[1] = {(glm::vec3) {-2.0f, 1.0f, 4.0f}, 1.0f, green, 32.0f, 0.5f};
	spheres[2] = {(glm::vec3) {0.0f, -5001.0f, 0.0f}, 5000.0f, yellow, 32.0f, 0.5f};

	pointlight = {(glm::vec3) {4.0f, 2.0f, 0.0f}, 0.8f};

	float old_time = (float) SDL_GetPerformanceCounter() / (float) SDL_GetPerformanceFrequency();

	while (running) {
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) {
				running = false;
			} else if (e.type == SDL_WINDOWEVENT) {
				switch (e.window.event) {
					case SDL_WINDOWEVENT_RESIZED: {
						window_width = e.window.data1;
						window_height = e.window.data2;

						SDL_Surface *new_backbuffer = SDL_GetWindowSurface(window);

						if (new_backbuffer != backbuffer) {
							backbuffer = new_backbuffer;
						}

						break;
					}
				}
			} else if (e.type == SDL_KEYDOWN) {
				switch (e.key.keysym.sym) {
					case SDLK_ESCAPE: {
						running = false;
						break;
					}
				}
			} else if (e.type == SDL_MOUSEWHEEL) {
				if (e.wheel.y > 0) {
					canvas_width += 2 * e.wheel.y;
					canvas_height += 2 * e.wheel.y;

					SDL_FreeSurface(triplebuffer);
					triplebuffer = SDL_CreateRGBSurfaceWithFormat(0, canvas_width, canvas_height, backbuffer->format->BitsPerPixel, backbuffer->format->format);
				} else if (e.wheel.y < 0) {
					if (canvas_width > 0 && canvas_height > 0) {
						canvas_width += 2 * e.wheel.y;
						canvas_height += 2 * e.wheel.y;

						SDL_FreeSurface(triplebuffer);
						triplebuffer = SDL_CreateRGBSurfaceWithFormat(0, canvas_width, canvas_height, backbuffer->format->BitsPerPixel, backbuffer->format->format);
					}
				}
			}
		}

		float new_time = (float) SDL_GetPerformanceCounter() / (float) SDL_GetPerformanceFrequency();
		float frame_time = new_time - old_time;
//		if (frame_time > 0.25f) frame_time = 0.25f;

		old_time = new_time;
		accumulator += frame_time;

		while (accumulator > dt) {
			update(dt);
			t += dt;
			accumulator -= dt;
		}

		render();

		update_backbuffer();
		SDL_UpdateWindowSurface(window);

//		printf("frame took %f s - %f fps\n", frame_time, 1.0f / frame_time);
		++frame;
	}

	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
