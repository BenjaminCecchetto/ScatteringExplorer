//============================================================================
// Name        : VisualGL.cpp
// Author      : Benjamin T. Cecchetto
// Version     :
// Copyright   : (c) 2013 Benjamin T. Cecchetto
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>

#include <GL/glew.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <stddef.h>
#include <list>
#include <chrono>

#include "cgl/cgl.h"
//#include "TestArch.h"
#include "SimulateScatteringArch.h"
#include "ScatteringConsole.h"

#include <random>

#define PROGRAM_NAME "Scattering Explorer"

/* A simple function that prints a message, the error code returned by SDL,
 * and quits the application */
void sdldie(const char *msg) {
    printf("%s: %s\n", msg, SDL_GetError());
    SDL_Quit();
    exit(1);
}

void checkSDLError(int line = -1) {
#ifndef NDEBUG
    const char *error = SDL_GetError();
    if (*error != '\0') {
        printf("SDL Error: %s\n", error);
        if (line != -1)
            printf(" + line: %i\n", line);
        SDL_ClearError();
    }
#endif
}


void RandomSideDirection(std::uniform_real_distribution<double>& unif,
                         std::default_random_engine& generator,
                        double& x, double& y, double& dirx, double& diry) {
  //randomize output direction/position
  double side = unif(generator);
  double placement = unif(generator);

  //between -pi/2 and pi/2
  double theta = unif(generator)*M_PI - M_PI/2.0;

  if (side < 1.0/4.0) {           // left side
    x = -1.0;
    y = placement*2.0-1.0;
    theta += M_PI;
  } else if (side < 2.0/4.0) {    //right side
    x = 1.0;
    y = placement*2.0-1.0;
  } else if (side < 3.0/4.0) {    //bottom side
    x = placement*2.0-1.0;
    y = -1.0;
    theta -= M_PI/2.0;
  } else {                        //top side
    x = placement*2.0-1.0;
    y = 1.0;
    theta += M_PI/2.0;
  }
  dirx = cos(theta);
  diry = sin(theta);
}

void InitializeSimulationParameters(std::uniform_real_distribution<double>& unif,
                                   std::default_random_engine& generator,
                                   cgl::SimulationParameters& sp, uint32_t index, uint32_t maxI) {
//  sp.sigma_t = 50;//unif(generator)*50 + 10.0;//50.0;
//  sp.sigma_s = 25;//unif(generator)*(sp.sigma_t-10.0)+10.0;//25.0; //sigma_s <= sigma_t
//  sp.henyey_g = .99;//unif(generator);//.99;

  double t = ((double)index)/((double)maxI-1.0);

  (void)unif;
  (void)generator;

  //look at sigma_s is 0 to 100
  sp.sigma_s = t*100.0;//unif(generator)*70 + 30  ; //the lower, the less scattering in a given area
  double sigma_a = 0.0;// no absorption0.2*unif(generator);
  sp.sigma_t = sp.sigma_s + sigma_a;
  sp.henyey_g = .5;//  unif(generator);

  sp.startX = -1.0;
  sp.startY = 0.0;
  sp.startDirX = 1.0;
  sp.startDirY = 0.0;

  sp.endX = 1.0;
  sp.endY = -0.5;
  sp.endDirX = 1.0;//cos(M_PI/2);
  sp.endDirY = 0.0;//sin(M_PI/2);

  //RandomSideDirection(unif, generator, sp.startX, sp.startY, sp.startDirX, sp.startDirY);
  //RandomSideDirection(unif, generator, sp.endX, sp.endY, sp.endDirX, sp.endDirY);

  sp.resolution = 128;
  sp.boxwidth = 20.0; //between 5  and 30mm
  sp.boxheight = 10.0;//sp.boxwidth/4.0; //always 1/4 the width
}


/* Our program's entry point */
int main(int argc, char *argv[]) {
    (void)(argc);
    (void)(argv);

   // cgl::FileLoaderPtr flp = cgl::FileLoader::New("Data/fileloadertest.txt");

    SDL_Window *mainwindow; /* Our window handle */
    SDL_GLContext maincontext; /* Our opengl context handle */

    if (SDL_Init(SDL_INIT_VIDEO) < 0) { /* Initialize SDL's Video subsystem */
        std::cerr << "Unable to initialize SDL Video" << std::endl;
        exit(1);
    }

    if (TTF_Init() < 0) {
       std::cerr << "Unable to initialize SDL_TTF" << std::endl;
       exit(1);
    }

    /* Request opengl 4.4 context.
     * SDL doesn't have the ability to choose which profile at this time of writing,
     * but it should default to the core profile */
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 4);

    /* Turn on double buffering with a 24bit Z buffer.
     * You may need to change this to 16 or 32 for your system */
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    uint32_t windowWidth = 1080;
    uint32_t windowHeight = 1080;
    mainwindow = SDL_CreateWindow(PROGRAM_NAME, SDL_WINDOWPOS_CENTERED,
    SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!mainwindow) /* Die if creation failed */
        sdldie("Unable to create window");

    checkSDLError(__LINE__);

    /* Create our OpenGL context and attach it to our window */
    maincontext = SDL_GL_CreateContext(mainwindow);
    checkSDLError(__LINE__);

    SDL_GL_MakeCurrent(mainwindow, maincontext);

    //Initialize GLEW
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	cgl::GLInfo::CheckGLError(__FILE__, __LINE__);

	if (GLEW_OK != err)
	{
	  /* Problem: glewInit failed, something is seriously wrong. */
	  fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
	}
	fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

	if(GLEW_VERSION_1_1)
	{
	  printf("----------------------------------------------------------------\n");
	  printf("Graphics Successfully Initialized\n");
	  printf("OpenGL Info\n");
	  printf("    Version: %s\n", glGetString(GL_VERSION));
	  printf("     Vendor: %s\n", glGetString(GL_VENDOR));
	  printf("   Renderer: %s\n", glGetString(GL_RENDERER));
	  printf("    Shading: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
	  printf("----------------------------------------------------------------\n");
	}
	else
	{
	  printf("Unable to get any OpenGL version from GLEW!");
	}

  using namespace cgl;

  //initialize random number generator
  double lower_bound = 0;
   double upper_bound = 1;
   unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
   std::default_random_engine generator(seed);
   std::uniform_real_distribution<double> unif(lower_bound,upper_bound);

// This makes our buffer swap synchronized with the monitor's vertical refresh
    SDL_GL_SetSwapInterval(1);

    bool quit = false;
    uint32_t lastTick = SDL_GetTicks();

//one shot simulation
  //  double shortestPathDistance = std::numeric_limits<double>::infinity();
    double inf = std::numeric_limits<double>::infinity();


    uint32_t maxI = 10; //number of samples over space
    uint32_t maxTests = 10; //max restarts looking for non-inf paths

    for (uint32_t index = 0; index < maxI; index++) {
      bool foundPath = false;
      uint32_t currTest = 0;

      while (!foundPath) {
        std::stringstream filenamebase;
        filenamebase << "henyey-0.5/bananascan-" << index << "-";

        SimulationParameters sp;
        InitializeSimulationParameters( unif, generator,sp, index, maxI);
        sp.Print();
        sp.Save(filenamebase.str() + "simulationParams.csv");

        SimulateScatteringArchPtr oneshotSSAptr = SimulateScatteringArch::New(sp);

        oneshotSSAptr->Simulate(50000);
        oneshotSSAptr->SetTopView();

        //UPDATE AND DRAW FOR ARCHITECTURE
        oneshotSSAptr->Update(1);
        oneshotSSAptr->Loop(1);
        oneshotSSAptr->Save(filenamebase.str());

        double shortestPathDistance = oneshotSSAptr->GetShortestPathDistance();

        currTest++;
        if (currTest == maxTests || shortestPathDistance != inf) {
          foundPath = true;
        }
      }
    }
    quit = true;

    SimulateScatteringArchPtr ssaptr;

    ScatteringConsole tconsole(ssaptr);
    bool consoleMode = false;
    SDL_StopTextInput();

    while (!quit) {
        uint32_t tick = SDL_GetTicks();
        double dt = (tick-lastTick) / 1000.0;
        lastTick = tick;

        //MAIN EVENT POLLING
        int returncode = 0;
        static bool mouseLDown = false;
        static bool mouseRDown = false;
        int xrel = 0;
        int yrel = 0;
        int mouseWheelY = 0;
        bool orthoMode = false;
        bool opaquePaths = false;
        bool toggleCube = false;
        bool topMode = false;

        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_KEYDOWN:
                if (consoleMode) {
                  if (event.key.keysym.sym == SDLK_RETURN) {
                      tconsole.Return();
                  }
                  if (event.key.keysym.sym == SDLK_BACKSPACE) {
                      tconsole.Backspace();
                  }
                  if (event.key.keysym.sym == SDLK_UP) {
                    tconsole.Up();
                  }
                  if (event.key.keysym.sym == SDLK_DOWN) {
                      tconsole.Down();
                  }
                  if (event.key.keysym.sym == SDLK_TAB) {
                      tconsole.Tab();
                  }
                } else {
                  if (event.key.keysym.sym == SDLK_o) {
                    orthoMode = true;
                  }
                  if (event.key.keysym.sym == SDLK_t) {
                    topMode = true;
                  }

                  if (event.key.keysym.sym == SDLK_p) {
                    opaquePaths = true;
                  }
                  if (event.key.keysym.sym == SDLK_c ) {
                      toggleCube = true;
                  }
                }

                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    returncode =-1;
                }

                if (event.key.keysym.sym == SDLK_BACKQUOTE) {
                   consoleMode = !consoleMode;

                   if (!consoleMode) {
                      SDL_StopTextInput();
                   } else {
                     SDL_StartTextInput();
                   }
                }
                break;
           case SDL_TEXTINPUT:
                if (consoleMode) {
                  tconsole.AddText(event.text.text);
                }

                break;
            case SDL_TEXTEDITING:
                break;
            case SDL_QUIT:
                returncode =-1;
        		case SDL_MOUSEMOTION:
                xrel = event.motion.xrel;
                yrel = event.motion.yrel;
                break;
        		case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT)
                  mouseLDown = true;
                if (event.button.button == SDL_BUTTON_RIGHT)
                  mouseRDown = true;
                break;
        		case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_LEFT)
                  mouseLDown = false;
                if (event.button.button == SDL_BUTTON_RIGHT)
                  mouseRDown = false;
                break;
            case SDL_MOUSEWHEEL:
                mouseWheelY = event.wheel.y;
                break;
            }
        }

        if (opaquePaths) {
          ssaptr->SetOpaqueMode();
        }

        if (orthoMode) {
          ssaptr->SetOrthoMode();
        }

        if (toggleCube) {
          ssaptr->SetCubeMode();
        }
        if (topMode) {
          ssaptr->SetTopView();
        }
        if (mouseLDown) {
            ssaptr->SetCameraPosition(xrel, yrel);
        }

        if (mouseRDown) {
            ssaptr->SetPathDensities(yrel);
        }

        if (mouseWheelY != 0) {
            ssaptr->SetCameraFOV(mouseWheelY);
        }


        ssaptr->Simulate(50000);
        ssaptr->SetTopView();

        //UPDATE AND DRAW FOR ARCHITECTURE
        ssaptr->Update(dt);
        ssaptr->Loop(dt);

        ssaptr->Save("bananascan.bin");
        returncode = -1;

        if (consoleMode) {
          tconsole.Display(dt);
        }

        //std::cout << "dt: " << dt << std::endl;
        /* Swap our back buffer to the front */
        SDL_GL_SwapWindow(mainwindow);

        if (returncode == -1)
            quit = true;
    }



    /* Wait 2 seconds */
    SDL_Delay(100);

    /* Delete our opengl context, destroy our window, and shutdown SDL */
    SDL_GL_DeleteContext(maincontext);
    SDL_DestroyWindow(mainwindow);
    SDL_Quit();

    return 0;
}
