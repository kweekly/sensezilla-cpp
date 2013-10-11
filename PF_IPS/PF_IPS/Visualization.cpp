#include <all.h>
#include "SDL.h"
#include "SDL_draw\SDL_draw.h"
#include "Visualization.h"


/*
#define COLOR_WHITE			0xff444444
#define COLOR_OBSTACLE		0xff000000
#define COLOR_PARTICLE		0xff00ff00
#define COLOR_GROUNDTRUTH   0x000000ff
#define COLOR_ESTIMATE		0x00ff0000
#define COLOR_RSSISENSORS	0x00ffff00
#define COLOR_GRIDLINES		0xff666666
#define COLOR_AXIS			0xffaa6666
*/


#define COLOR_WHITE			0xffffffff
#define COLOR_OBSTACLE		0xffaaaaaa
#define COLOR_PARTICLE		0xff000000
#define COLOR_GROUNDTRUTH   0xff222222
#define COLOR_ESTIMATE		0x00000000
#define COLOR_RSSISENSORS	0x00333333
#define COLOR_GRIDLINES		0xff666666
#define COLOR_AXIS			0xffaa6666


#define RADIUS_GROUNDTRUTH  10
#define RADIUS_ESTIMATE		10
#define RADIUS_RSSISENSORS	8
#define PARTICLE_X_LEN	2

Visualization::Visualization(PF_IPS * p) {
	prog = p;
	message = background = screen = nullptr;
	minx = p->minx;
	maxx = p->maxx;
	miny = p->miny;
	maxy = p->maxy;
	/*
	minx = 0;
	maxx = 20;
	miny = -15;
	maxy = 0;
	*/
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
		

		width = 800;
		height = (int)(width * (maxy-miny)/(maxx-minx));

		screen = SDL_SetVideoMode( width, height , 32, SDL_SWSURFACE );
		if ( screen == nullptr ) {
			return;
		}
		
		background = SDL_CreateRGBSurface( SDL_SWSURFACE, width, height, 32, 0xff0000, 0x00ff00, 0x0000ff00, 0xff000000);

		SDL_WM_SetCaption("Particle Filter", NULL);

		double ratx = (maxx-minx) / width;
		double raty = (maxy-miny) / height;
		double pngratx = (double)data.width / (prog->maxx - prog->minx);
		double pngraty = (double)data.height / (prog->maxy - prog->miny);
		// construct the background! PNGData -> background
		for ( int x = 0; x < width; x++ ) {
			for ( int y = 0; y < height; y++ ) {
				double cx = x * ratx + minx;
				double cy = y * raty + miny;
				int col = (int)((cx - prog->minx) * pngratx);
				int row = (int)((cy - prog->miny) * pngraty);
				bool obs = false;
				for ( int s = 0; s < 1; s++ ) {
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
		SDL_Delay(1);
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

#define PX(x) ( (int)(0.5+(x - minx) * width / (maxx - minx)))
#define PY(y) ( (int)(0.5+(y - miny) * height / (maxy - miny)))

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
		IFINB2(px-PARTICLE_X_LEN,py-PARTICLE_X_LEN) IFINB2(px+PARTICLE_X_LEN,py+PARTICLE_X_LEN) {
			//Draw_Pixel(screen, px,py, COLOR_PARTICLE);
			Draw_Line(screen,px-PARTICLE_X_LEN,py-PARTICLE_X_LEN,px+PARTICLE_X_LEN,py+PARTICLE_X_LEN, COLOR_PARTICLE);
			Draw_Line(screen,px+PARTICLE_X_LEN,py-PARTICLE_X_LEN,px-PARTICLE_X_LEN,py+PARTICLE_X_LEN, COLOR_PARTICLE);
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
		//Draw_FillCircle(screen, px, py, RADIUS_ESTIMATE, COLOR_ESTIMATE);
		Draw_FillRect(screen,px-RADIUS_ESTIMATE,py-RADIUS_ESTIMATE,2*RADIUS_ESTIMATE,2*RADIUS_ESTIMATE,COLOR_ESTIMATE);
	}

	int last_px = px;
	int last_py = py;

	// draw max estimate trajectory
	/*
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
	*/

	// draw sensors
	if ( prog->rssiparam_fname.size() > 0 ) {
		for ( size_t s = 0; s < prog->sensors.size(); s++ ){
			xycoords pos = prog->sensors[s].pos;
			int px = PX(pos.x);
			int py = PY(pos.y);
			int cx = (int)(RADIUS_RSSISENSORS * cos(M_PI / 6)+0.5);
			int cy = (int)(RADIUS_RSSISENSORS * sin(M_PI / 6)+0.5);
			IFINB {
				//Draw_FillCircle(screen, px,py, RADIUS_RSSISENSORS, COLOR_RSSISENSORS);
				Draw_Line(screen,px,py-RADIUS_RSSISENSORS,px+cx,py+cy,COLOR_RSSISENSORS);
				Draw_Line(screen,px,py-RADIUS_RSSISENSORS,px-cx,py+cy,COLOR_RSSISENSORS);
				Draw_Line(screen,px-cx,py+cy,px+cx,py+cy,COLOR_RSSISENSORS);
			}
		}
	}

	if ( !(prog->viz_frames_dir.empty()) ) {
		char buf[32];
		sprintf(buf,"%05d.bmp",(int)(prog->TIME - prog->T0));
		std::string cstr = (prog->viz_frames_dir + std::string(buf)).c_str() ;
		int i = SDL_SaveBMP( screen,cstr.c_str() );
		log_i("Saving to %s : %d",cstr.c_str(),i);
	}

	SDL_Flip( screen );
	

	exit:
	prog->data_locked_mutex.unlock(); // allow access to the data again
	prog->data_locked_ack_mutex.lock(); // reset the acknowlege lock
}