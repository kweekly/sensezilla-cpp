#pragma once
#include "SDL.h"
#include "PF_IPS.h"

#include <thread>
#include <mutex>


class Visualization {

public:
	Visualization(PF_IPS * prog);
	~Visualization();

	bool start();

private:
	thread vis_thread;
	void _vis_thread_fn();
	void _handle_events();
	void _render();
	bool running;
	
	PF_IPS * prog;

	SDL_Surface *message;
	SDL_Surface *background;
	SDL_Surface *screen;

	double minx,maxx,miny,maxy;
	double scale_factor;
	int width;
	int height;
};