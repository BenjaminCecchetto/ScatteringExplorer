#ifndef SCATTERING_CONSOLE_H
#define  SCATTERING_CONSOLE_H

/* This is the main class for the text console, which is an interactive
   console to add/remove/save/load scattering simulations */

#include <SDL2/SDL.h>
#include <vector>
#include <string>
#include "cgl/cgl.h"
#include "SimulateScatteringArch.h"

namespace cgl {


class ScatteringConsole : public TextConsole {
  SimulateScatteringArchPtr m_ssaptr;
  void simulate(std::string);
  void save(std::string);
  
public:
    ScatteringConsole(SimulateScatteringArchPtr ssaptr);

    void Return();
};




}

#endif
