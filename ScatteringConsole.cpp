
#include <sstream>
#include <SDL2/SDL.h>
#include <iterator>
#include <algorithm>

#include "cgl/cgl.h"

#include "ScatteringConsole.h"

namespace cgl {

ScatteringConsole::ScatteringConsole(SimulateScatteringArchPtr ssaptr) :
        TextConsole(), m_ssaptr(ssaptr) {

  m_commands.push_back("simulate");
  m_commands.push_back("save");
}

void ScatteringConsole::save(std::string filename) {
    m_ssaptr->Save(filename);
}

void ScatteringConsole::simulate(std::string inputcommand) {
    uint32_t numPaths = std::stol(inputcommand.c_str());
    m_ssaptr->Simulate(numPaths);
}

void ScatteringConsole::Return() {
  using namespace std;

  if (!m_currentLine.empty()) {
    istringstream iss(m_currentLine);

    vector<string> tokens{istream_iterator<string>{iss},
                        istream_iterator<string>{}};

    string inputCommand = tokens[0];

    if (find(m_commands.begin(), m_commands.end(), inputCommand) != m_commands.end()) {
        if (inputCommand.compare("simulate")==0 && tokens.size() == 2) {
            simulate(tokens[1]);
        }
        if (inputCommand.compare("save")==0 && tokens.size() == 2) {
            save(tokens[1]);
        }
    }
  }

  TextConsole::Return();
}



}
