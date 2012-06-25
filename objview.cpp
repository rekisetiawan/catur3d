/*
 * This code was created by Jeff Molofee '99 
 * (ported to Linux/SDL by Ti Leggett '01)
 *
 * Visit Jeff at http://nehe.gamedev.net/
 * 
 * or for port-specific comments, questions, bugreports etc. 
 * email to leggett@eecs.tulane.edu
 *
 * modifications by Joe Flint for use by teara
 * further mods by Joe Flint for use by tatac
 */
 
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef WIN32
#include <windows.h>
#endif
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <vector>
#include <string>
#include <math.h>

#include "SDL.h"
#include "SDL_opengl.h"
#include "objfile.h"
#include "texture.h"

using namespace std;

/* screen width, height, and bit depth */
#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 480
#define SCREEN_BPP     16

#define WINDOW_TITLE "objview $Revision: 1.1.2.3 $"

/* This is our SDL surface */
SDL_Surface *surface;


double xrot=0, yrot=0, zrot=0;
double scale = 20;
double zoom = -60;
int width=SCREEN_WIDTH, height=SCREEN_HEIGHT;

//Key array
bool keys[SDLK_LAST] = {false};
//Currently SDL only has 5 mouse constants, numbered 1 to 5
bool mouseb[6] = {false};
bool mouseout = false;
int mousex, mousey;
int mousedeltax = 0, mousedeltay = 0;

//Float to store fps count
GLfloat fps = 0;

ObjFile obj;

void Quit(int);

/* function to release/destroy our resources and restoring the old desktop */
void quit( int returnCode )
{
	/* clean up the window */
	SDL_Quit( );

	/* and exit appropriately */
	exit( returnCode );
}

int initGL( GLvoid );
// function to reset our viewport after a window resize
int resizeWindow( int width, int height )
{
	cout << "Resizing window" << endl;
	// Height / width ration
	GLfloat ratio;
 
	// Protect against a divide by zero
	if ( height == 0 )
		height = 1;

	ratio = ( GLfloat )width / ( GLfloat )height;

	// Setup our viewport.
	glViewport( 0, 0, ( GLsizei )width, ( GLsizei )height );

	// change to the projection matrix and set our viewing volume.
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity( );

	// Set our perspective
	gluPerspective( 45.0f, ratio, 20.0f, 100.0f );

	// Make sure we're chaning the model view and not the projection
	glMatrixMode( GL_MODELVIEW );

	// Reset The View
	glLoadIdentity( );

	initGL();
	return( true );
}


// general OpenGL initialization function
int initGL( GLvoid )
{
	cout << "Initializing OpenGL" << endl;
	// Enable smooth shading
	glShadeModel( GL_SMOOTH );

	// Set the background black
	glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );

	// Depth buffer setup
	glClearDepth( 1.0f );

	// Enables Depth Testing
	glEnable( GL_DEPTH_TEST );

	// The Type Of Depth Test To Do
	glDepthFunc( GL_LEQUAL );

	// Really Nice Perspective Calculations
	glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST );

	//Make sure that culling is disabled
	glDisable(GL_CULL_FACE);

	//Material settings and light positions stolen from teara
	//these will probably need to be changed at some point
	//may not even belong in this portion of the code
	GLfloat light_position[] = { 3.0, 3.0, 5.0, 0.0 };
	GLfloat light_ambient[]= { 0.1f, 0.1f, 0.1f, 1.0f };
	GLfloat light_diffuse[]= { 1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat light_specular[]= {1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat specularmat[]= {1.0f, 1.0f, 1.0f, 1.0f };
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
	glEnable( GL_LIGHTING );
	glEnable( GL_LIGHT0 );
	glEnable( GL_NORMALIZE );
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	glEnable( GL_COLOR_MATERIAL );
	glMaterialfv(GL_FRONT, GL_SPECULAR, specularmat);
	glMaterialf(GL_FRONT, GL_SHININESS, 128);

	
	return( true );
}

// Here goes our drawing code
int drawGLScene( GLvoid )
{
	// These are to calculate our fps
	static GLint T0     = 0;
	static GLint Frames = 0;
	static int startTime = SDL_GetTicks();
	static int start = SDL_GetTicks();

	// Make sure depth buffer writing before trying to clear it
	glDepthMask(GL_TRUE);
	// Clear The Screen And The Depth Buffer
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	//This resets the matrix and applies global rotation
	//again, this may belong elsewhere
	glLoadIdentity( );
	glTranslatef( 0.0f, 0.0f, zoom );
	glRotatef( xrot, 1.0f, 0.0f, 0.0f);
	glRotatef( yrot, 0.0f, 1.0f, 0.0f );
	glRotatef( zrot, 0.0f, 0.0f, 1.0f );

	static double matrix[] = { 	1, 0, 0, 0, 
								0, 1, 0, 0,
								0, 0, 1, 0,
								0, 0, 0, 1 };


	static bool leftdown = false;
	static Vector quant;
	static double quantw;
	if( mouseb[SDL_BUTTON_LEFT] ) {
		static Vector i;
		if( !leftdown ) {
			i.x = ( mousex * 1.0 / (( width - 1) * 0.5 ) ) - 1;
			i.y = 1 - ( mousey * 1.0 / (( height - 1) * 0.5 ) );
			i.z = 0;
			if( i.magnitude() > 1 )
				i.normalize();
			else
				i.z = sqrt(1 - (i.x*i.x + i.y*i.y) );
			leftdown = true;

		}
		Vector e;
		e.x = ( mousex * 1.0 / (( width - 1) * 0.5 ) ) - 1;
		e.y = 1 - ( mousey * 1.0 / (( height - 1) * 0.5 ) );
		e.z = 0;
		if( e.magnitude() > 1 )
			e.normalize();
		else
			e.z = sqrt(1 - (e.x*e.x + e.y*e.y));

		Vector perp;
		perp = i.cross( e );
		if( perp.magnitude() > 1e-5 ) {
			quant = perp;
			quantw = i.dot( e );
		} else {
			quant.x = quant.y = quant.z = 0;
			quantw = 0;
		}

		quant.normalize();
		if( quantw != 0 )
			glRotated( 57.2958*2 * acos( quantw ), quant.x, quant.y, quant.z );
//		yrot += mousedeltax / 2.0;
//		zrot += mousedeltay / 2.0;
		mousedeltax = 0;
		mousedeltay = 0;
	} else {
		if( leftdown ) {
			glPushMatrix();
			glLoadIdentity();
			if( quantw != 0 )
				glRotated( 57.2958*2 * acos( quantw ), quant.x, quant.y, quant.z );
			glMultMatrixd( matrix );
			glGetDoublev( GL_MODELVIEW_MATRIX, matrix );
			glPopMatrix();

		}
		leftdown = false;
	}

	glMultMatrixd( matrix );
	static double scale = 10;
	glScaled( scale, scale, scale );
	static double r = 0.8, g = 0.8, b = 0.8;
	glColor3d( r, g, b );
	if( r < 1 && keys[SDLK_r] )
		r += 0.001;
	if( r > 0 && keys[SDLK_e] )
		r -= 0.001;
	
	if( g < 1 && keys[SDLK_g] )
		g += 0.001;
	if( g > 0 && keys[SDLK_f] )
		g -= 0.001;
	
	if( b < 1 && keys[SDLK_b] )
		b += 0.001;
	if( b > 0 && keys[SDLK_v] )
		b -= 0.001;

	obj.draw();
	/*glTranslatef( -3.5*0.6, 0, -1.5*0.6 );
	for( int j = 0; j < 4; j++ ) {
		for( int i = 0; i < 8; i++ ) {
			glCallList( objdisplist );
			glTranslatef( 0.6, 0, 0 );
		}
		glTranslatef( -8*0.6, 0, 0.6 );
		if( j == 1 )
			glColor3d( 0.2, 0.2, 0.2 );
	}*/
	//obj.draw();


	if( mouseb[SDL_BUTTON_RIGHT] ) {
		scale += mousedeltay / 2.0;
		mousedeltay = 0;
		mousedeltax = 0;
	}

	if( keys[SDLK_UP] )
		zoom += 0.05;
	
	if( keys[SDLK_DOWN] )
		zoom -= 0.05;


	// Draw it to the screen
	SDL_GL_SwapBuffers( );

	// Gather our frames per second
	Frames++;
	{
		GLint t = SDL_GetTicks();
		if (t - T0 >= 5000) {
		    GLfloat seconds = (t - T0) / 1000.0;
		    fps = Frames / seconds;
		    cout << Frames << " frames in " << seconds << " seconds = " << fps << " FPS" << endl;
		    T0 = t;
		    Frames = 0;
		}
	}
	return( true );
}

int main( int argc, char **argv )
{
	int x;
	vector<string> args(argc);
	for(x=0; x < argc; x++) 
		args[x]=argv[x];

	if( argc < 2 ) {
		cout << "Please give an obj file to load" << endl;
		return 1;
	}
	
	bool usetext = false;
	Texture tex;
	if( argc > 2 ) {
		if(!tex.load(args[2])) {
			cout << "Failed to load " << args[2] << endl;
			return 3;
		} else {
			usetext = true;
			obj.enableTexture();
		}
	}

	
	if( !obj.load( args[1].c_str() ) ) {
		cout << "Failed to load " << args[1] << endl;
		return 2;
	}


	obj.findNorms();
	
	// Flags to pass to SDL_SetVideoMode
    	int videoFlags;
	// main loop variable
	int done = false;
	// used to collect events
	SDL_Event event;
	// this holds some info about our display
	const SDL_VideoInfo *videoInfo;
	// whether or not the window is active
	int isActive = true;
	
	// initialize SDL
	if ( SDL_Init( SDL_INIT_VIDEO ) < 0 )
	{
		cerr << "Video initialization failed:" << SDL_GetError() << endl;
		quit( 1 );
	}


	// Fetch the video info
	videoInfo = SDL_GetVideoInfo( );

    	if ( !videoInfo )
	{
		cerr << "Video query failed: " << SDL_GetError() << endl;
	 	quit( 1 );
	}

	// the flags to pass to SDL_SetVideoMode
	videoFlags  = SDL_OPENGL;          // Enable OpenGL in SDL
	videoFlags |= SDL_GL_DOUBLEBUFFER; // Enable double buffering
	videoFlags |= SDL_HWPALETTE;       // Store the palette in hardware
	videoFlags |= SDL_RESIZABLE;       // Enable window resizing

	// This checks to see if surfaces can be stored in memory
	if ( videoInfo->hw_available )
		videoFlags |= SDL_HWSURFACE;
	else
		videoFlags |= SDL_SWSURFACE;

	// This checks if hardware blits can be done
	if ( videoInfo->blit_hw )
		videoFlags |= SDL_HWACCEL;

	// Sets up OpenGL double buffering
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

	// get a SDL surface
	SDL_WM_SetCaption(WINDOW_TITLE,NULL);
	surface = SDL_SetVideoMode( SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP,
			videoFlags );

	// Verify there is a surface
	if ( !surface )
	{
		cerr << "Video mode set failed: " << SDL_GetError( ) << endl;
		quit( 1 );
	}

	// initialize OpenGL
	initGL( );

	// resize the initial window
	resizeWindow( SCREEN_WIDTH, SCREEN_HEIGHT );

	SDL_GetMouseState(&mousex, &mousey);
	// wait for events
	int frameCount = 0;

	if(usetext) {
		glEnable(GL_TEXTURE_2D);
		tex.loadGL();
		tex.use();
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		cout << "Texturing" << endl;
	}

	obj.build();

	while ( !done )
	{
	
		// handle the events in the queue

		while ( SDL_PollEvent( &event ) )
		{
			switch( event.type )
			{
				case SDL_MOUSEBUTTONDOWN:
					mousex = event.button.x;
					mousey = event.button.y;	
					mouseb[event.button.button] = true;
					break;
				case SDL_MOUSEBUTTONUP:
					mousex = event.button.x;
					mousey = event.button.y;
					mouseb[event.button.button] = false;
					break;
				case SDL_MOUSEMOTION:	
			        //printf("Mouse moved to (%d,%d)\n", event.motion.x, event.motion.y);
					mousex = event.motion.x;
					if( event.motion.state ) {
						mousedeltax += event.motion.xrel;
						mousedeltay += event.motion.yrel;
					}
					mousey = event.motion.y;
					break;
				case SDL_ACTIVEEVENT:
					// Something's happend with our focus
				    // If we lost focus or we are iconified, 					 
					// we shouldn't draw the screen
					if ( event.active.gain == 0 && event.active.state!=SDL_APPMOUSEFOCUS) {
						cout << "inact" <<endl;
						isActive = false;
					} else
						isActive = true;

					if ( event.active.gain == 0 && 
						event.active.state ==
						SDL_APPMOUSEFOCUS ) {
						mouseout = true;
						cout << "mouseout" << endl;
					} else if ( event.active.gain == 1 && 
						event.active.state ==
						SDL_APPMOUSEFOCUS ) {
						mouseout = false;
						cout << "mousein" << endl;
					}
					break;			    
				case SDL_VIDEORESIZE:
					// handle resize event
					surface = SDL_SetVideoMode( 
							event.resize.w,
							event.resize.h,
							16, videoFlags );
					if ( !surface )
					{
				    		cerr << "Could not get a surface after resize: " << SDL_GetError( ) << endl;
						quit( 1 );
					}
					obj.unbuild();
					if(usetext)
						tex.unloadGL();
			    	resizeWindow( event.resize.w, event.resize.h );
					if(usetext) {
						tex.loadGL();
						glEnable(GL_TEXTURE_2D);
						glEnable(GL_BLEND);
						glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
						tex.use();
					}
					obj.build();
					width = event.resize.w;
					height = event.resize.h;
					break;
				case SDL_KEYDOWN:
					keys[ event.key.keysym.sym ] = true;
					// handle key presses
					//handleKeyPress( &event.key.keysym );
					break;
				case SDL_KEYUP:
					keys[ event.key.keysym.sym ] = false;
					break;
				case SDL_QUIT:
					// handle quit requests
					done = true;
					break;
				default:
				    break;
			}
		}
		if(keys[SDLK_ESCAPE] || keys[SDLK_q])
			quit( 0 );
		if(keys[SDLK_F1]) {
			//This only works with X
			//In order to get it to work with Win32 the surface must be recreated
			SDL_WM_ToggleFullScreen( surface );
			keys[SDLK_F1]=false;
		}
		if ( isActive )
			drawGLScene( );
	}

	/* clean ourselves up and exit */
	quit( 0 );

	/* Should never get here */
	return( 0 );
}
