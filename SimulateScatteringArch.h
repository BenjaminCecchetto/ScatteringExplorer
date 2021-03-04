/*
 * TestArch.h
 *
 *  Created on: Dec 13, 2013
 *      Author: root
 */

#ifndef CGL_SIMULATESCATTERINGARCH_H_
#define CGL_SIMULATESCATTERINGARCH_H_

#include <memory>
#include "cgl/cgl.h"
#include <gtc/constants.hpp>
#include "PathVector.h"
#include <fstream>

namespace cgl {

class SimulateScatteringArch;
typedef std::shared_ptr<SimulateScatteringArch> SimulateScatteringArchPtr;

//everything needed to simulate a banana scan
struct SimulationParameters {
  double sigma_t;
  double sigma_s;
  double henyey_g;

  double startX;
  double startY;
  double startDirX;
  double startDirY;

  double endX;
  double endY;
  double endDirX;
  double endDirY;

  uint32_t resolution;
  double boxwidth;
  double boxheight;

  void Print() {
    std::cout << "henyeyCoeffs (sigma_t, sigma_s, henyey_g):" << "\n" << sigma_t << "\n" << sigma_s << "\n" << henyey_g << std::endl;
    std::cout << "startParams (x,y,dirx,diry):" << "\n" << startX << "\n" << startY << "\n" << startDirX << "\n" << startDirY << std::endl;;
    std::cout << "endParams (x,y,dirx,diry):" << "\n" << endX << "\n" << endY << "\n" << endDirX << "\n" << endDirY << std::endl;;
    std::cout << "boxParams (resolution,width,height):" << "\n" << resolution << "\n" << boxwidth << "\n" << boxheight << std::endl;;
  }
  void Save(std::string filename) {
    std::ofstream myfile(filename.c_str());
    if (myfile.is_open())
    {
      myfile << "henyeyCoeffs:" << "\n" << sigma_t << "\n" << sigma_s << "\n" << henyey_g << std::endl;
      myfile << "startParams:" << "\n" << startX << "\n" << startY << "\n" << startDirX << "\n" << startDirY << std::endl;;
      myfile << "endParams:" << "\n" << endX << "\n" << endY << "\n" << endDirX << "\n" << endDirY << std::endl;;
      myfile << "boxParams:" << "\n" << resolution << "\n" << boxwidth << "\n" << boxheight << std::endl;;

      myfile.close();
    }
  }

  void Load(std::string filename) {
    std::ifstream myfile(filename.c_str());
    std::string line;

    if (myfile.is_open())
    {
      getline(myfile,line);
      getline(myfile,line);
      sigma_t = std::stod(line);
      getline(myfile,line);
      sigma_s = std::stod(line);
      getline(myfile,line);
      henyey_g = std::stod(line);

      getline(myfile,line);
      getline(myfile,line);
      startX = std::stod(line);
      getline(myfile,line);
      startY = std::stod(line);
      getline(myfile,line);
      startDirX = std::stod(line);
      getline(myfile,line);
      startDirY = std::stod(line);

      getline(myfile,line);
      getline(myfile,line);
      endX = std::stod(line);
      getline(myfile,line);
      endY = std::stod(line);
      getline(myfile,line);
      endDirX = std::stod(line);
      getline(myfile,line);
      endDirY = std::stod(line);

      getline(myfile,line);
      getline(myfile,line);
      resolution= std::stoul(line);
      getline(myfile,line);
      boxwidth = std::stod(line);
      getline(myfile,line);
      boxheight = std::stod(line);

      myfile.close();
    }
  }
};

class SimulateScatteringArch : public Architecture {
    float m_theta;
    float m_phi;
    float m_FOV;

    bool m_orthoMode;
    bool m_opaqueMode;

    float m_pathDensities;

    // pathvector pointer
    ProgramPtr m_pvProg; //needed to set MVP

    PathVectorPtr m_pv; //pointer to path vector/simulation
    PathVectorPtr m_pvEnd;

    DrawFramebufferPtr m_pvfbo;
    DrawFramebufferPtr m_pvfboEnd;
    DrawFramebufferPtr m_blendedFBO;

    //start/end position are the cube coordinates of the box
    glm::vec3 m_startPosition;
    glm::vec3 m_startDirection;

    glm::vec3 m_endPosition;
    glm::vec3 m_endDirection;

    double m_sigma_t;
    double m_sigma_s;
    double m_henyey_g;

    uint32_t m_resolution;
    double m_boxwidth;
    double m_boxheight;


    double m_shortestPathDistance;

    CubePtr m_cube;


    void updateCameraMatrix();
public:
    SimulateScatteringArch(SimulationParameters sp);

    static SimulateScatteringArchPtr
      New(SimulationParameters sp) {
          return std::make_shared<SimulateScatteringArch>(sp);
      }

      PathVectorPtr GetPathVector() {
        return m_pv;
      }

    void Simulate(uint32_t numPaths);
    void Save(std::string filename);

    double GetShortestPathDistance() { return m_shortestPathDistance; }

    void SetSimulationParameters(SimulationParameters sp);

    void SetOpaqueMode() {
        m_opaqueMode = !m_opaqueMode;

        if (!m_opaqueMode) {
            m_pvProg->UniformFloat("pathDensities", m_pathDensities);
        } else {
            m_pvProg->UniformFloat("pathDensities", 1.0f);
        }
    }

    void SetOrthoMode() {
        m_orthoMode = !m_orthoMode;

        if (m_orthoMode) {
          m_FOV = 10.0f;
        } else {
          m_FOV = 45.0f;
        }

        updateCameraMatrix();
    }

    void SetTopView() {
      m_orthoMode = true;
      m_FOV = m_boxwidth/2.0;
      m_theta = 0.0;
      m_phi = 0.0;
      updateCameraMatrix();
    }

    void SetCubeMode() {
      bool cubeVisible = m_cube->GetDrawPass()->GetVisible();
      m_cube->GetDrawPass()->SetVisible(!cubeVisible);
    }

    void SetPathDensities(int mouseYrel) {
      m_pathDensities += static_cast<float>(mouseYrel)*0.0001f;
      m_pathDensities = glm::clamp<float>(m_pathDensities, 0.0f, 1.0f);

      m_pvProg->UniformFloat("pathDensities", m_pathDensities);
    }

    void SetCameraFOV (int mouseWheelY) {
      if (!m_orthoMode)
        m_FOV += mouseWheelY;
      else
        m_FOV += 0.1f*mouseWheelY;

       updateCameraMatrix();
    }

    void SetCameraPosition(int xrel, int yrel) {
      m_theta += 0.01*xrel;
      m_phi += 0.01*yrel;

      updateCameraMatrix();
    }

    void Update(float dt);
};

}

#endif /* TESTARCH_H_ */
