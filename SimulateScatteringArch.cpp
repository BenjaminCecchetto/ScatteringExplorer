/*
 * TestArch.cpp
 *
 *  Created on: Dec 13, 2013
 *      Author: root
 */

#include <GL/glew.h>

#include <iostream>
#include "SimulateScatteringArch.h"
#include "cgl/cgl.h"

#include <cmath>
#include <sstream>


#include "PathVector.h"

//shortest path
#include <iostream>
#include <vector>
#include <string>
#include <list>

#include <limits> // for numeric_limits

#include <queue>
#include <utility> // for pair
#include <algorithm>
#include <iterator>
 // end shortest path
#include <fstream>
#include <iomanip>

namespace cgl {


void SimulateScatteringArch::Update(float dt) {
  (void)dt;
  //m_pv->SimulatePaths(10);
}

void SimulateScatteringArch::updateCameraMatrix() {

  while (m_theta >  glm::pi<float>()) m_theta -= 2.0f*glm::pi<float>();
  while (m_theta < -glm::pi<float>()) m_theta += 2.0f*glm::pi<float>();

  float epsilon = 0.001;
  m_phi = glm::clamp<float>(m_phi, epsilon, glm::pi<float>()-epsilon);

  if (!m_orthoMode) {
    m_FOV = glm::clamp<float>(m_FOV, 30.0f, 180.0f);
  } else {
    m_FOV = glm::clamp<float>(m_FOV, 1.0, 100.0);
  }
  glm::vec3 cartesian(cos(m_theta)*sin(m_phi), sin(m_theta)*sin(m_phi), cos(m_phi));

  //Modelview Projection Matrix
  glm::vec3 cameraPosition = 60.0f*cartesian;
  glm::vec3 cameraTarget = glm::vec3();
  glm::vec3 cameraUp  = glm::vec3(0,0,1);

  glm::mat4 viewMatrix = glm::lookAt(cameraPosition, cameraTarget, cameraUp);

  float near = 0.01f;
  float far = 100.0f;

  glm::mat4 projectionMatrix;

  if (!m_orthoMode) {
    projectionMatrix = glm::perspective(
            glm::radians(m_FOV),         // The horizontal Field of View, in degrees : the amount of "zoom". Think "camera lens". Usually between 90° (extra wide) and 30° (quite zoomed in)
            1.0f, // Aspect Ratio. Depends on the size of your window. Notice that 4/3 == 800/600 == 1280/960, sounds familiar ?
            near,        // Near clipping plane. Keep as big as possible, or you'll get precision issues.
            far       // Far clipping plane. Keep as little as possible.
        );
  } else {
    projectionMatrix = glm::ortho(-m_FOV, m_FOV, -m_FOV, m_FOV, near, far);
  }

  glm::mat4 MVP = projectionMatrix * viewMatrix;

  //update programs using it
  m_pvProg->UniformMat4("MVP", MVP);
  m_cube->GetProgram()->UniformMat4("MVP", MVP);
  m_cube->GetProgram()->UniformVec3f("cubeBound", glm::vec3(m_boxwidth, m_boxwidth, m_boxheight));
}

void SimulateScatteringArch::Simulate(uint32_t numPaths) {

   // m_pv->SimulateBidirectionalPathTracing(m_startPosition, m_startDirection, m_endPosition, m_endDirection, numPaths);
	m_pv->SimulatePaths(m_boxwidth, m_boxheight, m_startPosition, m_startDirection, m_sigma_t, m_sigma_s, m_henyey_g, numPaths);
  m_pvEnd->SimulatePaths(m_boxwidth, m_boxheight, m_endPosition, -m_endDirection, m_sigma_t, m_sigma_s, m_henyey_g, numPaths);

  m_pathDensities = 1.0/numPaths;
  m_pvProg->UniformFloat("pathDensities", m_pathDensities);


}



//Dijkstra from https://rosettacode.org/wiki/Dijkstra%27s_algorithm#C.2B.2B

typedef int vertex_t;
typedef double weight_t;

const weight_t max_weight = std::numeric_limits<double>::infinity();

struct neighbor {
    vertex_t target;
    weight_t weight;
    neighbor(vertex_t arg_target, weight_t arg_weight)
        : target(arg_target), weight(arg_weight) { }
};

typedef std::vector<std::vector<neighbor> > adjacency_list_t;
typedef std::pair<weight_t, vertex_t> weight_vertex_pair_t;

void DijkstraComputePaths(vertex_t source,
                          const adjacency_list_t &adjacency_list,
                          std::vector<weight_t> &min_distance,
                          std::vector<vertex_t> &previous)
{
    int n = adjacency_list.size();
    min_distance.clear();
    min_distance.resize(n, max_weight);
    min_distance[source] = 0;
    previous.clear();
    previous.resize(n, -1);
    // we use greater instead of less to turn max-heap into min-heap
    std::priority_queue<weight_vertex_pair_t,
			std::vector<weight_vertex_pair_t>,
			std::greater<weight_vertex_pair_t> > vertex_queue;
    vertex_queue.push(std::make_pair(min_distance[source], source));

    while (!vertex_queue.empty())
    {
        weight_t dist = vertex_queue.top().first;
        vertex_t u = vertex_queue.top().second;
        vertex_queue.pop();

	// Because we leave old copies of the vertex in the priority queue
	// (with outdated higher distances), we need to ignore it when we come
	// across it again, by checking its distance against the minimum distance
	if (dist > min_distance[u])
	    continue;

        // Visit each edge exiting u
	const std::vector<neighbor> &neighbors = adjacency_list[u];
        for (std::vector<neighbor>::const_iterator neighbor_iter = neighbors.begin();
             neighbor_iter != neighbors.end();
             neighbor_iter++)
        {
            vertex_t v = neighbor_iter->target;
            weight_t weight = neighbor_iter->weight;
            weight_t distance_through_u = dist + weight;
	    if (distance_through_u < min_distance[v]) {
	        min_distance[v] = distance_through_u;
	        previous[v] = u;
	        vertex_queue.push(std::make_pair(min_distance[v], v));

	    }

        }
    }
}

std::list<vertex_t> DijkstraGetShortestPathTo(
    vertex_t vertex, const std::vector<vertex_t> &previous)
{
    std::list<vertex_t> path;
    for ( ; vertex != -1; vertex = previous[vertex])
        path.push_front(vertex);
    return path;
}


void SavePath(std::vector<glm::dvec2> path, std::string filename) {
  std::ofstream myfile(filename.c_str());
  if (myfile.is_open())
  {

    for (glm::dvec2 v : path) {
      myfile << v.x << "\t" << v.y << std::endl;
    }
    myfile.close();
  }
}

double ShortestPath(std::string filename, FloatImagePtr im, glm::dvec2 start, glm::dvec2 end) {


    int32_t h = im->Height();
    int32_t w = im->Width();


    double minI = std::numeric_limits<double>::infinity();
    double maxI = 0.0;

    int maxIi = 0;
    int maxIj = 0;

    for (int32_t i = 0; i < int32_t(im->Height()); i++) {
      for (int32_t j = 0; j < int32_t(im->Width()); j++) {
          if (im->At(i, j, 0) < minI)
            minI = im->At(i, j, 0);

          if (im->At(i, j, 0) > maxI) {
            maxI = im->At(i, j, 0);
            maxIi = i;
            maxIj = j;
          }
      }

    }

    std::cout << "Width: " << w << " Height: " << h << std::endl;
    std::cout << " Min " << minI << " Max " << maxI << "("<< maxIi <<"," << maxIj << " )" << std::endl;
    std::cout << "start " << start.x << "\t" << start.y << std::endl;
    std::cout << "end " << end.x << "\t" << end.y << std::endl;

    //image to probability
    double epsilon = 0.00001; //without this it may favour physically longer paths
    std::vector<double>probabilityImage(h*w);
    for (int32_t i = 0; i < h; i++) {
      for (int32_t j = 0; j < w; j++) {

        //opengl's image format is strange
        double P = (im->At((w-1)-j, i, 0)-minI)/(maxI-minI);

        probabilityImage[i*w + j] = -log(P)+epsilon;
      }
    }

    //build adjacency list
    adjacency_list_t adjacency_list(w*h);

    for (int32_t i = 0; i < h; i++) {
      for (int32_t j = 0; j < w; j++) {

        if (j < w-1)
          adjacency_list[i*w + j ].push_back(neighbor((i+0)*w+(j+1),probabilityImage[(i+0)*w+(j+1)] ));
        if (j > 0)
          adjacency_list[i*w + j ].push_back(neighbor((i+0)*w+(j-1), probabilityImage[(i+0)*w+(j-1)] ));
        if (i < h-1)
          adjacency_list[i*w + j ].push_back(neighbor((i+1)*w+(j+0), probabilityImage[(i+1)*w+(j+0)] ));
        if (i > 0)
          adjacency_list[i*w + j ].push_back(neighbor((i-1)*w+(j+0), probabilityImage[(i-1)*w+(j+0)] ));


        if (j < w-1 && i < h-1)
          adjacency_list[i*w + j ].push_back(neighbor((i+1)*w+(j+1), sqrt(2)*probabilityImage[(i+1)*w+(j+1)] ));
        if (j > 0 && i < h-1)
          adjacency_list[i*w + j ].push_back(neighbor((i+1)*w+(j-1), sqrt(2)*probabilityImage[(i+1)*w+(j-1)] ));
        if (j < w-1 && i > 0)
          adjacency_list[i*w + j ].push_back(neighbor((i-1)*w+(j+1), sqrt(2)*probabilityImage[(i-1)*w+(j+1)] ));
        if (j > 0 && i > 0)
          adjacency_list[i*w + j ].push_back(neighbor((i-1)*w+(j-1), sqrt(2)*probabilityImage[(i-1)*w+(j-1)] ));

      }

    }

    std::vector<weight_t> min_distance;
    std::vector<vertex_t> previous;

    uint32_t startx = round(glm::clamp(start.x, 0.0, double(w)));
    uint32_t starty = round(glm::clamp(start.y, 0.0, double(h)));
    uint32_t endx = round(glm::clamp(end.x, 0.0, double(w)));
    uint32_t endy = round(glm::clamp(end.y, 0.0, double(h)));

    uint32_t startIndex = starty*w + startx;
    uint32_t endIndex   =   endy*w +   endx;
    DijkstraComputePaths(startIndex, adjacency_list, min_distance, previous);

    std::cout << "Distance from start to end: " << min_distance[endIndex] << std::endl;
    std::list<vertex_t> path = DijkstraGetShortestPathTo(endIndex, previous);

    std::vector<glm::dvec2> shortestPath;
    for (vertex_t p : path) {
          glm::dvec2 v(p % w, floor(p/w));
          shortestPath.push_back(v);
    }
    SavePath(shortestPath, filename.c_str());

    return min_distance[endIndex];
}

void MinimumVarianceCurve(glm::dvec2 startPos, glm::dvec2 startDir,
                          glm::dvec2 endPos,   glm::dvec2 endDir) {
  (void)startDir;
  (void)endDir;

  uint32_t numPoints = 200;

  //initialize with simple curve
  std::vector<glm::dvec2> minimumVariancePath(numPoints);
  std::vector<glm::dvec2> tangents(numPoints);
  std::vector<double> curvatures(numPoints);

  //set initial points
  for (uint32_t i = 0; i < numPoints; i++) {
      double t = static_cast<double>(i)/static_cast<double>(numPoints);

      //linear
      glm::dvec2 p = (1.0-t)*startPos + t*endPos;

      minimumVariancePath[i] = p;
  }

  //iterate and move towards less curvature
  std::cout << "Computing MVC" << std::endl;
  uint32_t numIterations = 10;
//  double delta = 0.01;
  for (uint32_t iter = 0; iter < numIterations; iter++) {
      //calculate tangents and circles
      for (uint32_t t = 0; t < numPoints; t++) {
        if (t == 0)
          tangents[t] = startDir;
        else if (t == numPoints-1)
          tangents[t] = endDir;
        else
          tangents[t] = minimumVariancePath[t+1]-minimumVariancePath[t-1];


          //calculate curvatures
          glm::dvec3 p1;
          glm::dvec3 p2;
          glm::dvec3 p3;
          if (t==0)
            p1 = glm::dvec3(minimumVariancePath[t]-startDir, 0.0);
          else
            p1 = glm::dvec3(minimumVariancePath[t-1], 0.0);

            p2 = glm::dvec3(minimumVariancePath[t], 0.0);

          if (t==numPoints-1)
            p3 = glm::dvec3(minimumVariancePath[t]+endDir, 0.0);
          else
            p3 = glm::dvec3(minimumVariancePath[t+1], 0.0);

          //get circle center
          double numerator = glm::length(p1-p2)*glm::length(p2-p3)*glm::length(p3-p1);
          double denom = 2.0 * glm::length(glm::cross(p1-p2, p2-p3));

          double curvature;
          if (denom == 0)
            curvature = 0.0;
          else
            curvature = 1.0/(numerator/denom);

          curvatures[t] = curvature;

      }

      //move slowly in direction to agree with neighbours tangents
      for (uint32_t p = 1; p < numPoints-1; p++) {
          double avgCurvature = 0.5*(curvatures[p+1]+curvatures[p-1]);
          double dCurvature = 0.5*(avgCurvature - curvatures[p]);

          glm::dvec3 displacement = dCurvature*glm::cross(glm::dvec3(0.0, 0.0, 1.0), glm::dvec3(tangents[p], 0.0));
          minimumVariancePath[p] += glm::dvec2(displacement.x, displacement.y);



      }
  }

  SavePath(minimumVariancePath, "mvcpath.csv");
}


void SimulateScatteringArch::Save(std::string filenamebase) {
 FloatImagePtr floatIm = m_blendedFBO->GetColorBuffer(0)->GetImageF();
 floatIm->Save(filenamebase + "image.bin");


 glm::dvec3 start = glm::dvec3(((float)m_resolution-1.0f)*(0.5f*m_startPosition + glm::vec3(0.5f, 0.5f, 0.5f)));
 glm::dvec2 start2 = glm::dvec2(start.x, start.y);
 glm::dvec3 end = glm::dvec3(((float)m_resolution-1.0f)*(0.5f*m_endPosition + glm::vec3(0.5f, 0.5f, 0.5f)));
 glm::dvec2 end2 = glm::dvec2(end.x, end.y);

 m_shortestPathDistance = ShortestPath(filenamebase + "shortestpath.csv", floatIm, start2, end2);
 //double splineStrength = 50.0;
 //glm::dvec2 startDir = splineStrength*glm::dvec2(m_startDirection.x, m_startDirection.y);
 //glm::dvec2 endDir = splineStrength*glm::dvec2(m_endDirection.x, m_endDirection.y);
 //MinimumVarianceCurve(start2, startDir, end2, endDir);

 //ShortestPath(floatIm, glm::uvec2(127, 63), glm::uvec2(0,95));
 //m_pvfbo->GetColorBuffer(0)->GetImageF()->Save(std::string("start-").append(filename));
 //m_pvfboEnd->GetColorBuffer(0)->GetImageF()->Save(std::string("end-").append(filename));

}

//fragment shader to blend between two textures
const char* fragBlendPDFs = "#version 430 core\n"
        "layout(binding=0) uniform sampler2D tex0;\n"
        "layout(binding=1) uniform sampler2D tex1;\n"
        "uniform float weight0;\n"
        "uniform float weight1;\n"
        "in vec2 texCoord;\n"
        "layout(location=0) out vec4 fragColor;\n" //between [-1,-1]x[1,1]
        "void main() {\n"
        "    fragColor = weight0*texture(tex0,texCoord)*weight1*texture(tex1,texCoord);\n" //blend between
        "}\n";

void SimulateScatteringArch::SetSimulationParameters(SimulationParameters sp) {
  m_startPosition   = glm::vec3(sp.startX, sp.startY, 0);
  m_startDirection  = glm::vec3(sp.startDirX, sp.startDirY, 0);

  m_endPosition     = glm::vec3(sp.endX, sp.endY, 0.5);
  m_endDirection    = glm::vec3(sp.endDirX, sp.endDirY, 0);
  m_sigma_t         = sp.sigma_t;
  m_sigma_s         = sp.sigma_s;
  m_henyey_g        = sp.henyey_g;
  m_resolution      = sp.resolution;
  m_boxwidth        = sp.boxwidth;
  m_boxheight       = sp.boxheight;

}

SimulateScatteringArch::SimulateScatteringArch(SimulationParameters sp) {
    m_phi = 0.0f;
    m_theta = 0.0f;
    m_FOV = 50.0f;
    m_orthoMode = true;

    using namespace glm;

    //TEST PATH DATA TO RENDER
    m_pv = PathVector::New();
    m_pvEnd = PathVector::New();

    m_pvfbo = nullptr;
    m_pvfboEnd = nullptr;

    //INPUTS
    SetSimulationParameters(sp);


    m_pvProg = Program::New("shaders/simulatePaths-vertex.glsl",
                            "shaders/simulatePaths-frag.glsl");

    m_pvProg->UniformFloat("pathDensities", m_pathDensities);

    //Pass 0 - Draw starting paths
    VertexSpecificationPtr pvSpec = VertexSpecification::New();
    pvSpec->Append(m_pv->GetVBO());
    pvSpec->Set();

    m_pvfbo = DrawFramebuffer::New(1, GL_RGB32F, m_resolution, m_resolution);
    ClearPassPtr pvClearBuffer = ClearPass::New(m_pvfbo,
                                          GL_COLOR_BUFFER_BIT,
                                          0.0, 0.0, 0.0, 1.0);
    DrawModePtr pvDraw = m_pv->MultiDrawArrays();
    DrawPassPtr pvPass = DrawPass::New(m_pvProg, pvSpec, pvDraw, true);

    BlendParametersPtr pvBlend = BlendParameters::New(GL_ONE, GL_ONE,
                                                  GL_FUNC_ADD);
    pvPass->AppendBlendParameters(pvBlend);
    pvPass->SetFramebufferOutput(m_pvfbo); // Write to this FBO

    //Pass 1 - Draw End paths
    VertexSpecificationPtr pvSpecEnd = VertexSpecification::New();
    pvSpecEnd->Append(m_pvEnd->GetVBO());
    pvSpecEnd->Set();

    m_pvfboEnd = DrawFramebuffer::New(1, GL_RGB32F, m_resolution, m_resolution);
    ClearPassPtr pvClearBufferEnd = ClearPass::New(m_pvfboEnd,
                                          GL_COLOR_BUFFER_BIT,
                                          0.0, 0.0, 0.0, 1.0);
    DrawModePtr pvDrawEnd = m_pvEnd->MultiDrawArrays();
    DrawPassPtr pvPassEnd = DrawPass::New(m_pvProg, pvSpecEnd, pvDrawEnd, true);

    BlendParametersPtr pvBlendEnd = BlendParameters::New(GL_ONE, GL_ONE,
                                                  GL_FUNC_ADD);
    pvPassEnd->AppendBlendParameters(pvBlendEnd);
    pvPassEnd->SetFramebufferOutput(m_pvfboEnd); //write to this FBO

    //Blend two textures
    FragmentShaderPtr fragPtr = FragmentShader::New(fragBlendPDFs);
    m_blendedFBO = DrawFramebuffer::New(1, GL_RGB32F, m_resolution, m_resolution);
    ClearPassPtr pvClearBufferBlended = ClearPass::New(m_blendedFBO,
                                                        GL_COLOR_BUFFER_BIT,
                                                        0.0, 0.0, 0.0, 1.0);
    RenderTexturePassPtr rtp = RenderTexturePass::New(m_pvfbo->GetColorBuffer(0), fragPtr);
    rtp->AppendTexture(m_pvfboEnd->GetColorBuffer(0));
    rtp->GetProgram()->UniformFloat("weight0", 1.0);
    rtp->GetProgram()->UniformFloat("weight1", 1.0);
    rtp->SetFramebufferOutput(m_blendedFBO);

    //CLEAR PASS
    ClearPassPtr pvClearScreen = ClearPass::New(0,
                                                GL_COLOR_BUFFER_BIT,
                                                0.0, 0.0, 0.0, 1.0);

    //render texture to screen
    RenderTexturePassPtr rtpscreen = RenderTexturePass::New(m_blendedFBO->GetColorBuffer(0));


    m_cube = Cube::New();

    updateCameraMatrix();

    AddPass(pvClearBuffer);
    AddPass(pvPass);

    AddPass(pvClearBufferEnd);
    AddPass(pvPassEnd);

    AddPass(pvClearBufferBlended);
    AddPass(rtp);

    AddPass(pvClearScreen);
    AddPass(rtpscreen);
    //AddPass(m_cube->GetDrawPass());

    return;
}

}
