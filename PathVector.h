/*
 * PathVector.h
 *
 *  Created on: Nov 1, 2013
 *      Author: cecchett
 */

#ifndef CGL_PATHVECTOR_H_
#define CGL_PATHVECTOR_H_

#include <memory>
#include <vector>
#include <glm.hpp>
#include <string>
#include "cgl/cgl.h"

namespace cgl {

class PathVector;
typedef std::shared_ptr<PathVector> PathVectorPtr;

typedef struct {
		glm::vec3 pos;
	  double intensity;
} PathVertex;

typedef std::vector<PathVertex> Path;

class PathVector {
	std::vector<Path> m_paths;

  //opengl data
	MultiDrawArraysPtr m_multiDrawArrays;
	const uint32_t m_maximumPathVertices = 100000000; //every 100k is 1.2MB

	VertexBufferPtr m_pathInputVBO;

	//index into paths to start uploading from
	uint32_t m_pathIndexOffset;
	uint32_t m_gpuOffset;

public:
    PathVector();

    static PathVectorPtr New() {
        return std::make_shared<PathVector>();
    }


    Path SimulatePath(double boxwidth, double boxheight, glm::vec3 start, glm::vec3 startDir, double sigma_t, double sigma_s, double henyey_g);
    void SimulatePaths(double boxwidth, double boxheight, glm::vec3 start, glm::vec3 startDir, double sigma_t, double sigma_s, double henyey_g, uint32_t numPaths);


		void CalculateIntensities(Path& p);

		void SimulateBidirectionalPathTracing(glm::vec3 start, glm::vec3 startDir,
			 																		glm::vec3 end,		glm::vec3 endDir, uint32_t numPaths);

		void AddPath(const Path& p) ;

		uint32_t GetMaximumPathVertices() {
			return m_maximumPathVertices;
		}

		VertexBufferPtr GetVBO() {
			return m_pathInputVBO;
		}

    /*
    	Creates a set of paths starting at (0,0,-bound) ending at (x,y,bound)
    		x \in [-bound, bound]
    		y \in [-bound, bound]
    	With path lengths 2 to numpaths+1
    */
    void TestPaths(uint32_t numPaths);

		uint32_t TotalPaths() {
			return m_paths.size();
		}

		uint32_t TotalVertices(uint32_t offset);

    uint32_t TotalSize(uint32_t offset);
    FloatBufferPtr GetFloatBuffer(uint32_t offset);
    std::string ToString();

    void UpdateGLData();
    DrawModePtr MultiDrawArrays();
};

}

#endif /* SCE_PATHVECTOR_H_ */
