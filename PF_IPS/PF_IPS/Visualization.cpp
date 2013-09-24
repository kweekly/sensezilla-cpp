#include <all.h>
#include "SDL.h"
#include "SDL_draw\SDL_draw.h"
#include "Visualization.h"



#define COLOR_WHITE			0xff444444
#define COLOR_OBSTACLE		0xff000000
#define COLOR_PARTICLE		0xff00ff00
#define COLOR_GROUNDTRUTH   0x000000ff
#define COLOR_ESTIMATE		0x00ff0000
#define COLOR_RSSISENSORS	0x00ffff00
#define COLOR_GRIDLINES		0xff666666
#define COLOR_AXIS			0xffaa6666

#define RADIUS_GROUNDTRUTH  5
#define RADIUS_ESTIMATE		5
#define RADIUS_RSSISENSORS	3

Visualization::Visualization(PF_IPS * p) {
	prog = p;
	message = background = screen = nullptr;
}

Visualization::~Visualization() {
	running = false;
	prog->data_locked_mutex.unlock();
	vis_thread.join();

	if ( message != nullptr) SDL_FreeSurface(message);
	if ( background != nullptr ) SDL_FreeSurface(background);
	SDL_Quit();
}

bool Visualization::start() {
	running = true;
	prog->data_locked_mutex.lock();
	vis_thread = thread(&Visualization::_vis_thread_fn,this);

	return true;
}

void Visualization::_vis_thread_fn() {
	log_i("Visualizer Thread Started");
	prog->data_locked_ack_mutex.lock(); // used later

	if ( prog->use_mappng ){
		PNGData data = PNGLoader::loadFromPNG(prog->mappng_fname);
		if ( SDL_Init( SDL_INIT_EVERYTHING ) == -1 ) {
			return;
		}
		
		scale_factor = 1.0;
		while ( data.width * scale_factor > 1000 ) 
			scale_factor /= 2.0;
		
		height = (int)(scale_factor * data.height);
		width = (int)(scale_factor * data.width);

		screen = SDL_SetVideoMode( width, height , 32, SDL_SWSURFACE );
		if ( screen == nullptr ) {
			return;
		}
		
		background = SDL_CreateRGBSurface( SDL_SWSURFACE, width, height, 32, 0xff0000, 0x00ff00, 0x0000ff00, 0xff000000);

		SDL_WM_SetCaption("Particle Filter", NULL);

		// construct the background! PNGData -> background
		for ( int x = 0; x < width; x++ ) {
			for ( int y = 0; y < height; y++ ) {
				int row = y / scale_factor;
				int col = x / scale_factor;
				bool obs = false;
				for ( int s = 0; s < 1/scale_factor; s++ ) {
					if (data.B[row+s][col+s] == data.R[row+s][col+s] && data.R[row+s][col+s] == data.G[row+s][col+s] && (data.R[row+s][col+s] < 0.95)) {
						obs = true;
						break;
					}
				}
				if (obs) {
					Draw_Pixel(background,x,y,COLOR_OBSTACLE);
				} else {
					Draw_Pixel(background,x,y,COLOR_WHITE);
				}
			}
		}



		PNGLoader::freePNGData(&data);

		SDL_BlitSurface( background, NULL, screen, NULL );

		if ( SDL_Flip(screen) == -1 )
		{
			return;
		}

	} else {
		log_e("No mapPNG specified... cannot do visualization");
		return;
	}

	while(running) {
		_handle_events();
		_render();
		SDL_Delay(100);
	}
	prog->data_locked_ack_mutex.unlock();
}

void Visualization::_handle_events() {
	SDL_Event sdlevt;
	
	while ( SDL_PollEvent(&sdlevt) ) {
		if ( sdlevt.type == SDL_QUIT ) {
			running = false;
			exit(0);
		}
	}
}

#define PX(x) ( (int)(0.5+(x - prog->minx) * width / (prog->maxx - prog->minx)))
#define PY(y) ( (int)(0.5+(y - prog->miny) * height / (prog->maxy - prog->miny)))

#define IFINB if ( px >= 0 && py >= 0 && px < width-1 && py < height-1)
#define IFINB2(px,py) if ( px >= 0 && py >= 0 && px < width-1 && py < height-1)

void Visualization::_render() {
	if ( screen == nullptr || screen == NULL ) return;

	prog->req_data_lock = true; // request access to data
	prog->data_locked_mutex.lock(); // wait for program to make it accessable
	prog->data_locked_ack_mutex.unlock(); // acknowlege that we got the mutex
	prog->req_data_lock = false; // no longer requested

	if ( !running ) {
		goto exit;
	}

	// do stuff
	SDL_BlitSurface( background, NULL, screen, NULL );
	//Draw_FillRect(screen,0,0,width,height,COLOR_OBSTACLE);
	
	// draw grid lines
	const double GRID_INTERVAL = 5.0;
	for ( double x = floor(prog->grid.bounds[XMIN]/GRID_INTERVAL)*GRID_INTERVAL; x <= prog->grid.bounds[XMAX]; x += GRID_INTERVAL ) {
		int px = PX(x);
		if ( px >= 0 && px < width-1) {
			if ( x == 0 ) 
				Draw_VLine(screen, px, 0, height-1, COLOR_AXIS);
			else
				Draw_VLine(screen, px, 0, height-1, COLOR_GRIDLINES);
		}
	}

	for ( double y = floor(prog->grid.bounds[YMIN]/GRID_INTERVAL)*GRID_INTERVAL; y <= prog->grid.bounds[YMAX]; y += GRID_INTERVAL ) {
		int py = PY(y);
		if ( py >= 0 && py < height-1) {
			if ( y == 0 ) 
				Draw_HLine(screen, 0, py, width-1, COLOR_AXIS);
			else
				Draw_HLine(screen, 0, py, width-1, COLOR_GRIDLINES);
		}
	}

	// draw particles
	SIRFilter<State,Observation,Params> * filter = prog->filter;
	size_t s1 = (filter->step_no+1) % filter->pcache_time;
	for (size_t p = 0; p < filter->nParticles; p++ ) {
		int px = PX(filter->pcache[s1][p].state.pos.x);
		int py = PY(filter->pcache[s1][p].state.pos.y);
		IFINB {
			Draw_Pixel(screen, px,py, COLOR_PARTICLE);
		}
	}

	// draw ground truth blob
	if ( prog->gtdata.size() > 0 ) {
		int px = PX(prog->gtdata[0]->getPoint(prog->TIME));
		int py = PY(prog->gtdata[1]->getPoint(prog->TIME)); 
		//log_i("%.2f %.2f %.2f %d %d",prog->TIME,x,y,px,py);
		IFINB {
			Draw_FillCircle(screen, px,py, RADIUS_GROUNDTRUTH, COLOR_GROUNDTRUTH);
		}
	}

	// draw max estimate blob
	int px = PX(prog->current_best_state.pos.x);
	int py = PY(prog->current_best_state.pos.y);
	IFINB {
		Draw_FillCircle(screen, px, py, RADIUS_ESTIMATE, COLOR_ESTIMATE);
	}

	int last_px = px;
	int last_py = py;

	// draw max estimate trajectory
	size_t parent = filter->pcache[s1][prog->current_best_state_index].parent;
	
	
	for ( size_t c = 1; c < filter->pcache_time; c++ ) {
		if ( parent >= filter->nParticles ) { break; }
		size_t idx = ( prog->current_best_state_index - c + filter->pcache_time ) % filter->pcache_time;

		//printf("%d : parent=%d\n",idx,parent);

		int px = PX(filter->pcache[idx][parent].state.pos.x);
		int py = PY(filter->pcache[idx][parent].state.pos.y);
		IFINB {
			Draw_FillCircle(screen, px, py, RADIUS_ESTIMATE/2, COLOR_ESTIMATE);
			IFINB2(last_px,last_py) {
				Draw_Line(screen,px,py,last_px,last_py,COLOR_ESTIMATE);
			}
		}
		parent = filter->pcache[idx][parent].parent;
		last_px = px;
		last_py = py;
	}
	

	// draw sensors
	if ( prog->rssiparam_fname.size() > 0 ) {
		for ( size_t s = 0; s < prog->sensors.size(); s++ ){
			xycoords pos = prog->sensors[s].pos;
			int px = PX(pos.x);
			int py = PY(pos.y);

			IFINB {
				Draw_FillCircle(screen, px,py, RADIUS_RSSISENSORS, COLOR_RSSISENSORS);
			}
		}
	}

	SDL_Flip( screen );
	

	exit:
	prog->data_locked_mutex.unlock(); // allow access to the data again
	prog->data_locked_ack_mutex.lock(); // reset the acknowlege lock
}