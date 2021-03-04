#include <GL/glew.h>

#include <iostream>
#include "SimulateScatteringArch.h"
#include "cgl/cgl.h"

#include <cmath>
#include <sstream>

namespace cgl {

const double pi = 3.141592653589793238462643383279;

//parameters mu    - mean
//           sigma - standard deviation
//            x    - sample position

double IsotropicPhaseFunction() {
    return 1.0 / (4.0 * pi);
}

double RaleighScatteringPhaseFunction(double theta) {
    return (1.0 + cos(theta) * cos(theta)) * (3.0 / (16.0 * pi));
}

//g \in (-1,1)
//g < 0 backward scattering
//g == 0 isotropic scattering
//g > 0 forward scattering
double HenyeyGreensteinPhaseFunction(double g, double theta) {
    return (1.0 - g * g)
            / (4.0 * pi * pow(1.0 + g * g - 2.0 * g * cos(theta), 1.5));
}

double SchlickPhaseFunction(double k, double theta) {
    return (1.0 - k * k)
            / (4.0 * pi * (1.0 + k * cos(theta)) * (1.0 + k * cos(theta)));
}
double Gaussian(double mu, double sigma, double x) {
    double x_mu = x - mu;
    return std::exp(-x_mu * x_mu / (2.0 * sigma * sigma))
            / (sigma * std::sqrt(2.0 * pi));
}

double sumBuffer(DoubleBufferPtr buffer) {
    double sum = 0.0f;
    for (uint32_t i = 0; i < buffer->Size(); i++)
        sum += buffer->_data[i];
    return sum;
}

//sum modifier is such that sum(A_i*dx) = RHS
DoubleBufferPtr normalizeBuffer(DoubleBufferPtr buffer, float dx = 1.0,
        float RHS = 1.0) {
    DoubleBufferPtr normalizedBuffer = DoubleBuffer::New(buffer->Size());

    double integral = sumBuffer(buffer) * dx;

    for (uint32_t i = 0; i < buffer->Size(); i++) {
        normalizedBuffer->_data[i] = RHS * buffer->_data[i] / integral;
    }

   // std::cout << "integral " << integral << std::endl;
    return normalizedBuffer;
}

const bool isotropic = false;

//generates a phase function in range [0, pi] at [0,n)
//normalized so 2pi*integral_0^pi p(theta) dtheta = 1
DoubleBufferPtr generatePhase(uint32_t n, double henyeyCoefficient) {
    DoubleBufferPtr phase = DoubleBuffer::New(n);

    for (uint32_t i = 0; i < n; i++) {

        double theta = (pi * i) / (n - 1.0f);

        if (isotropic)
            phase->_data[i] = IsotropicPhaseFunction();
        else
            phase->_data[i] = HenyeyGreensteinPhaseFunction(henyeyCoefficient, theta);
    }

    // std::cout << "phase " << phase->ToString() << std::endl;
    phase = normalizeBuffer(phase, pi / n, 4.0 * pi);
    //std::cout << "phase " << phase->ToString() << std::endl;

    return phase;
}

DoubleBufferPtr CDFfromBuffer(DoubleBufferPtr buffer) {
    DoubleBufferPtr pdf = normalizeBuffer(buffer);

    DoubleBufferPtr cdf = DoubleBuffer::New(buffer->Size());

    double sum = 0.0;
    for (uint32_t i = 0; i < buffer->Size(); i++) {
        cdf->_data[i] = sum;
        sum += pdf->_data[i];
    }

    return cdf;
}

DoubleBufferPtr invCDFfromBuffer(DoubleBufferPtr buffer) {
    DoubleBufferPtr cdf = CDFfromBuffer(buffer);
    DoubleBufferPtr invCDF = DoubleBuffer::New(buffer->Size());

    uint32_t n = buffer->Size();
    for (uint32_t i = 0; i < n - 1; i++) {
        double x1 = i / (n - 1.0f);
        double x2 = (i + 1) / (n - 1.0f);

        double y1 = cdf->_data[i];
        double y2 = cdf->_data[i + 1];

        uint32_t a1 = floor(y1 * n);
        uint32_t a2 = floor(y2 * n);
        for (uint32_t j = a1; j < a2; j++) {
            double interp = (double(j - a1) / double(a2 - a1));
            invCDF->_data[j] = (1.0 - interp) * x1 + interp * x2;
        }
    }

    return invCDF;
}

DoubleBufferPtr randomBuffer(uint32_t size) {
    DoubleBufferPtr rbuf = DoubleBuffer::New(size);

    for (uint32_t i = 0; i < size; i++)
        rbuf->_data[i] = Random::drand();

    return rbuf;
}

}
