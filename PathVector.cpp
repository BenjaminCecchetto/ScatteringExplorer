#include "PathVector.h"
#include <sstream>
#include <glm.hpp>
#include <gtc/random.hpp>
#include <algorithm>
#include <limits>

namespace cgl {

PathVector::PathVector() {
	m_pathInputVBO =
					VertexBuffer::New(true,  GL_DYNAMIC_DRAW, GL_FLOAT, sizeof(float)*3*2, m_maximumPathVertices );
	m_pathInputVBO->AddAttribute(3, false); //vector xyz
	m_pathInputVBO->AddAttribute(3, false); //vector intensities

	m_multiDrawArrays = MultiDrawArrays::New(GL_LINE_STRIP, 0);

	m_pathIndexOffset = 0;
	m_gpuOffset = 0;
}

double HenyeyGreensteinPhase(double g, double costheta) {
    return (1.0 - g * g)
            / (4.0 * glm::pi<double>() * pow(1.0 + g * g - 2.0 * g * costheta, 1.5));
}

double HenyeyGreensteinPhaseSampler(double g, double R) {
	double s = 2.0*R-1.0;
	double sqterm = (1.0 - g*g)/(1.0+g*s);

	return (1.0/(2.0*g)) * (1.0 + g*g - sqterm*sqterm);
}

/*********************** Indicatrix *********************
*New direction cosines after scattering by angle theta, fi.
* mux new=(sin(theta)*(mux*muz*cos(fi)-muy*sin(fi)))/sqrt(1-muz^2)+mux*cos(theta)
* muy new=(sin(theta)*(muy*muz*cos(fi)+mux*sin(fi)))/sqrt(1-muz^2)+muy*cos(theta)
* muz new= - sqrt(1-muz^2)*sin(theta)*cos(fi)+muz*cos(theta)
*---------------------------------------------------------
*Input:
* muxs,muys,muzs - direction cosine before collision
* mutheta, fi - cosine of polar angle and the azimuthal angle
*---------------------------------------------------------
*Output:
*  muxd,muyd,muzd - direction cosine after collision
*---------------------------------------------------------
*/
void Indicatrix(double muxs, double muys, double muzs,
								double mutheta, double fi,
								double *muxd, double *muyd, double *muzd)
{
 double costheta = mutheta;
 double sintheta = sqrt(1.0-costheta*costheta); // sin(theta)
 double sinfi = sin(fi);
 double cosfi = cos(fi);
 if (muzs == 1.0) {
   *muxd = sintheta*cosfi;
   *muyd = sintheta*sinfi;
   *muzd = costheta;
 } else if (muzs == -1.0) {
   *muxd = sintheta*cosfi;
   *muyd = -sintheta*sinfi;
   *muzd = -costheta;
 } else {
   double denom = sqrt(1.0-muzs*muzs);
   double muzcosfi = muzs*cosfi;
   *muxd = sintheta*(muxs*muzcosfi-muys*sinfi)/denom + muxs*costheta;
   *muyd = sintheta*(muys*muzcosfi+muxs*sinfi)/denom + muys*costheta;
   *muzd = -denom*sintheta*cosfi + muzs*costheta;
 }
}

//simulate path without taking into account intensities
Path PathVector::SimulatePath(double boxwidth, double boxheight, glm::vec3 start, glm::vec3 startDir, double sigma_t, double sigma_s, double henyey_g) {
		start = glm::vec3(0.5f*boxwidth*start.x,
											0.5f*boxwidth*start.y,
											0.5f*boxheight*start.z);


		Path p;

		using namespace glm;

		vec3 pos = start;
		vec3 dir = startDir;

		double sigma_a = sigma_t - sigma_s;
		double henyeyCoefficient = henyey_g;

		//while it's in the box
		while (all(greaterThanEqual(pos, (-0.5f)*vec3(boxwidth, boxwidth, boxheight))) &&
					 all(lessThanEqual(pos, (0.5f)*vec3(boxwidth, boxwidth, boxheight)))) {
			//add old position to path
			PathVertex pv;
			pv.pos = pos;
			pv.intensity = 1.0;
			p.push_back(pv);

			//move particle
			float dist = -log(linearRand(0.0,1.0))/sigma_t;
			pos += dist*dir;

			double u = linearRand(0.0,1.0);

			//terminate if photon is absorbed - determine during BDPT
		//	if (u < dist*sigma_a)
			//	break;

			//sample phase if probability says to scatter in new direction
			if (dist*sigma_a < u && u < dist*sigma_t) {
				double sampledCosTheta = HenyeyGreensteinPhaseSampler(henyeyCoefficient, linearRand(0.0, 1.0));
				double sampledPhi = linearRand(0.0, 2.0*glm::pi<double>());

				double mu_xn, mu_yn, mu_zn;
				Indicatrix(dir.x, dir.y, dir.z, sampledCosTheta, sampledPhi,
										&mu_xn, &mu_yn, &mu_zn);

				dir = vec3(mu_xn, mu_yn, mu_zn);
			}
		}

		return p;
}

void PathVector::AddPath(const Path& p) {
		if (p.size() + TotalVertices(0) > m_maximumPathVertices) {
			std::cout << "Trying to add too many vertices, exiting..." << std::endl;
		} else {
			m_paths.push_back(p);
		}
}

void PathVector::CalculateIntensities(Path& p) {

		double initialIntensity = 1.0;
		double intensity = initialIntensity;

		double sigma_t = 57.298;
//		double sigma_s = 57.291;
//		double sigma_a = sigma_t - sigma_s;
		double henyeyCoefficient = 0.99;

		p[0].intensity = intensity;

		for (uint32_t i = 1; i < p.size()-1; i++) {
				glm::dvec3 p0 = p[i-1].pos;
				glm::dvec3 p1 = p[i  ].pos;
				glm::dvec3 p2 = p[i+1].pos;

				double dist = glm::length(p1-p0);
				double expValue = sigma_t*glm::exp(-sigma_t*dist);

				glm::dvec3 d1 = glm::normalize(p0-p1);
				glm::dvec3 d2 = glm::normalize(p2-p1);
				double costheta = glm::dot(d1,d2);

				double gvalue = HenyeyGreensteinPhase(henyeyCoefficient, costheta);

				p[i].intensity = 0.99*p[i-1].intensity;//p[i-1].intensity*expValue*gvalue;
				(void)expValue;
				(void)gvalue;
		}

		glm::dvec3 p0 = p[p.size()-1].pos;
		glm::dvec3 p1 = p[p.size()-2].pos;

		double dist = glm::length(p1-p0);
		double expValue = glm::exp(-sigma_t*dist);
		p[p.size()-1].intensity = p[p.size()-2].intensity*expValue;
}

double calculatePathProbability(const Path& p, uint32_t start, uint32_t end) {

	double sigma_t = 57.298;
	double sigma_s = 57.291;
//		double sigma_a = sigma_t - sigma_s;
	double henyeyCoefficient = 0.99;

	double probability = 1.0;

	if (start < 2) start = 2;
	if (end > p.size()-1) end = p.size()-1;

	for (uint32_t i = start; i < end; i++) {
		  glm::vec3 p0 = p[i-2].pos;
			glm::vec3 p1 = p[i-1].pos;
			glm::vec3 p2 =   p[i].pos;

			double dist = glm::length(p2-p1);
			double pMedium = sigma_t*glm::exp(-sigma_t*dist);

			double costheta = glm::dot(glm::normalize(p1-p0), glm::normalize(p2-p1));
			double pScattering = (sigma_s/sigma_t)*HenyeyGreensteinPhase(henyeyCoefficient, costheta);

			probability = probability*pMedium*pScattering/(dist*dist);
	}

	probability = glm::clamp(probability, 0.0, 1.0);

	return probability;
}


void ClosestPointBetweenPaths(const Path& p1, const Path& p2, double& mindist,
															uint32_t& iout, uint32_t& jout) {
		mindist = std::numeric_limits<double>::max();
		iout = 0;
		jout = 0;

		for (uint32_t i = 0; i < p1.size(); i++) {
			for (uint32_t j = 0; j < p2.size(); j++) {
					double l = glm::length(p2[j].pos-p1[i].pos);

					if (l < mindist) {
						mindist = l;

						iout = i;
						jout = j;
					}
			}
		}

}

glm::vec3 GetPathDirection(const Path& p, uint32_t i) {
	if (i == 0)
		i = 1;

	return glm::normalize(p[i].pos-p[i-1].pos);
}

Path GetSubpath(const Path& p, uint32_t i, uint32_t j) {
	 Path pOut(p.begin() + i, p.begin() + j);
   return pOut;
}

Path JoinPaths(const Path& p1, const Path& p2) {
	Path p12;
	p12.reserve( p1.size() + p2.size() ); // preallocate memory
	p12.insert( p12.end(), p1.begin(), p1.end() );
	p12.insert( p12.end(), p2.begin(), p2.end() );

	return p12;
}

void PathVector::SimulateBidirectionalPathTracing(glm::vec3 start, glm::vec3 startDir,
	 																								glm::vec3 end,	 glm::vec3 endDir, uint32_t numPaths) {

	(void)endDir;
	(void)end;
	(void) startDir;
	(void) start;
	(void)numPaths;

}

void PathVector::SimulatePaths(double boxwidth, double boxheight, glm::vec3 start, glm::vec3 startDir,  double sigma_t, double sigma_s, double henyey_g, uint32_t numPaths) {
	std::cout << "Simulating " << numPaths << std::endl;

	for (uint32_t i = 0; i < numPaths; i++) {
		Path p = SimulatePath(boxwidth, boxheight, start, startDir, sigma_t, sigma_s, henyey_g);

		if (p.size() + TotalVertices(0) > m_maximumPathVertices) {
				std::cout << "Trying to add too many vertices, exiting..." << std::endl;
				UpdateGLData();
				return;
		}

		m_paths.push_back(p);
	}

	UpdateGLData();
}

void PathVector::TestPaths(uint32_t pathRes) {
	using namespace glm;

	float w = pathRes;

	float bound = 1.0;
	float boundBox = (2.0*bound/(w-1));

	vec3 start(-bound, 0.0, 0.0);
	uint32_t pathLengths = 2;

	for (float u = -bound; u < bound; u += boundBox ) {
		for (float v = -bound; v < bound; v += boundBox ) {
			vec3 end(bound, u, v );

			vec3 jitter = glm::linearRand(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, boundBox, boundBox));

			end = end + jitter;

			//interpolate between start and end pts
			Path path;
			for (uint32_t i = 0; i < pathLengths; i++) {
				float a = static_cast<float>(i)/(pathLengths-1);


				PathVertex pv;
				vec3 pt = mix(start, end, a);
				pv.pos = pt;
				pv.intensity = 1.0;

				path.push_back(pv);
			}

			AddPath(path);

		}
	}

	UpdateGLData();
}

uint32_t PathVector::TotalVertices(uint32_t offset) {
		uint32_t size = 0;
		for (uint32_t i = offset; i < m_paths.size(); i++) {
				size += m_paths[i].size();
		}
		return size;
}

uint32_t PathVector::TotalSize(uint32_t offset) {
	return TotalVertices(offset)*3*2;
}

FloatBufferPtr PathVector::GetFloatBuffer(uint32_t offset) {
		uint32_t floatElements = TotalSize(offset);

		FloatBufferPtr fbp = FloatBuffer::New(floatElements);

		uint32_t index = 0;
		for (uint32_t i = offset; i < m_paths.size(); i++) {
			for (uint32_t j = 0; j < m_paths[i].size(); j++) {
					fbp->_data[index++] = m_paths[i][j].pos.x;
					fbp->_data[index++] = m_paths[i][j].pos.y;
					fbp->_data[index++] = m_paths[i][j].pos.z;

					fbp->_data[index++] = m_paths[i][j].intensity;
					fbp->_data[index++] = m_paths[i][j].intensity;
					fbp->_data[index++] = m_paths[i][j].intensity;
			}
		}

		return fbp;
}


std::string PathVector::ToString() {
	std::stringstream ss;

	for (uint32_t i = 0; i < m_paths.size(); i++) {
		ss << "Path " << i << std::endl;

		for (uint32_t j =0 ;j < m_paths[i].size(); j++) {
			ss << "(" << m_paths[i][j].pos.x << ","
								<< m_paths[i][j].pos.y << ","
								<< m_paths[i][j].pos.z << ")" << std::endl;
		}
		ss << std::endl;

	}

	return ss.str();
}

void PathVector::UpdateGLData() {
	std::cout << "total vertices: " << TotalVertices(0) << std::endl;

	//if need to add more data, do it
	if (m_pathIndexOffset != m_paths.size()) {
		//Update BufferData on the gpu
		FloatBufferPtr fbp = GetFloatBuffer(m_pathIndexOffset);
		m_pathInputVBO->UploadOffset(m_gpuOffset, fbp);

		std::cout << "gpu offset " << m_gpuOffset << " indexoffset " << m_pathIndexOffset << std::endl;
		std::cout << "gpu size " << fbp->Bytes() << " pathsize " << m_paths.size() << std::endl;;

		//start the offset at the end of index as well as buffer
		m_pathIndexOffset = m_paths.size();
		m_gpuOffset += fbp->Bytes();

	}

	m_multiDrawArrays->Resize(m_paths.size());

	GLint index = 0;
	for (uint32_t i = 0; i < m_paths.size(); i++) {
			m_multiDrawArrays->m_first[i] = index;
			m_multiDrawArrays->m_count[i] = m_paths[i].size();
			index += m_paths[i].size();
	}
}

//draws a series of line strips
DrawModePtr PathVector::MultiDrawArrays() {
		return m_multiDrawArrays;
}


}
