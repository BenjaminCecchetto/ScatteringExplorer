#version 440 
#extension GL_ARB_gpu_shader_fp64 : enable

//BEGIN INPUTS-----------------------------------------------
layout(local_size_x=64) in;

//atomic counters
layout(binding=0, offset=0) uniform atomic_uint randomSeed;
layout(binding=1, offset=0) uniform atomic_uint numRays;

//buffers
layout(std430, binding=0) buffer PathDataBuffer {
    uint numPaths;
    uint pathLength;
    uint numIterations;
    uint numRandomNumbers;
};

layout(std430, binding=1) buffer PathBuffer {
    dvec4 paths[];
};

#define phaseSize 200
#define numMaterials 4
layout(std430, binding=2) buffer ScatteringParameters { 
    double sigma_s_R[numMaterials];
    double sigma_a_R[numMaterials];
    double sigma_t_R[numMaterials];
    
    double sigma_s_G[numMaterials];
    double sigma_a_G[numMaterials];
    double sigma_t_G[numMaterials];
    
    double sigma_s_B[numMaterials];
    double sigma_a_B[numMaterials];
    double sigma_t_B[numMaterials];

    double phase[phaseSize*numMaterials];
    double phaseCDFinv[phaseSize*numMaterials];
};

layout(std430, binding=3) buffer RandomBuffer {
    double randomNumber[];
};

#define volumeSize 256
layout(std430, binding=4) buffer VolumeData {
    double volumeData[volumeSize*volumeSize*numMaterials];
};


//END INPUTS------------------------------------------------

struct ScatteringParameter {
    double sigma_s;
    double sigma_a;
    double sigma_t;
        
    double phase[phaseSize];
    double phaseCDFinv[phaseSize];
};

double random() {
    uint rand = atomicCounterIncrement(randomSeed);
    uint modulo = uint(mod(rand, numRandomNumbers));
       
    return randomNumber[modulo];
}

//between -pi, pi
dvec2 samplePhaseAngle() {
    double rand = random();
    double randsign = sign(random()-0.5);
    
    uint idx = uint(clamp(phaseSize*rand, 0, phaseSize-1));
    double val = phaseCDFinv[idx]; 
    
    double angleSample = randsign*val*3.1415926;
    double phaseSample = phase[uint(val*phaseSize)];
    
    dvec2 phaseAngle = dvec2(phaseSample, angleSample);
    return phaseAngle;
}

dvec4 volumeLoad(uint i, uint j) {
    i = clamp(i, 0, volumeSize-1);
    j = clamp(j, 0, volumeSize-1);
    return dvec4(volumeData[i + volumeSize*j + volumeSize*volumeSize*0],
                 volumeData[i + volumeSize*j + volumeSize*volumeSize*1],
                 volumeData[i + volumeSize*j + volumeSize*volumeSize*2],
                 volumeData[i + volumeSize*j + volumeSize*volumeSize*3]);
}

dvec4 sampleImage(double u, double v) {
   u = u * volumeSize - 0.5;
   v = v * volumeSize - 0.5;
   
   uint x = uint(floor(u));
   uint y = uint(floor(v));
   
   double u_ratio = u - x;
   double v_ratio = v - y;
   double u_opposite = 1 - u_ratio;
   double v_opposite = 1 - v_ratio;
   
   dvec4 result = (volumeLoad( x  ,y  )* u_opposite  + 
                   volumeLoad( x+1,y  )* u_ratio) * v_opposite +
                  (volumeLoad( x  ,y+1)* u_opposite  + 
                   volumeLoad( x+1,y+1)* u_ratio) * v_ratio;

   return result;
}

//assume sum(weights)=1
ScatteringParameter getParameterCombination(double x, double y, uint color) {
    ScatteringParameter sp;
    
    double weights[numMaterials];
        
    dvec4 vweights = sampleImage(x,y);
    weights[0] = vweights.x;
    weights[1] = vweights.y;
    weights[2] = vweights.z;   
    weights[3] = vweights.w;
    
    //initialize scatteringparameters
    sp.sigma_s = 0.0;
    sp.sigma_a = 0.0;
    sp.sigma_t = 0.0;
        
    for (uint j = 0; j < phaseSize; j++) {
        sp.phase[j] = 0.0;
        sp.phaseCDFinv[j] = 0.0;
    }
        
    for (uint i = 0; i < numMaterials; i++) {
        if (color == 0) {
            sp.sigma_s += sigma_s_R[i]*weights[i];
            sp.sigma_a += sigma_a_R[i]*weights[i];
            sp.sigma_t += sigma_t_R[i]*weights[i];
        } else if (color == 1) {
            sp.sigma_s += sigma_s_G[i]*weights[i];
            sp.sigma_a += sigma_a_G[i]*weights[i];
            sp.sigma_t += sigma_t_G[i]*weights[i];
        } else {
            sp.sigma_s += sigma_s_B[i]*weights[i];
            sp.sigma_a += sigma_a_B[i]*weights[i];
            sp.sigma_t += sigma_t_B[i]*weights[i];
        }

        for (uint j = 0; j < phaseSize; j++) {
            sp.phase[j]      += phase[j+phaseSize*i]*weights[i];
            sp.phaseCDFinv[j]+= phase[j+phaseSize*i]*weights[i];
        }
    }
        
    return sp;
}

//simulates a path in [0, -5]x[10,5]
void SimulatePath(double L_0, uint bufIndex, uint color) {
 //start at origin, go to the right
    dvec2 x_0 = dvec2(0.0, 0.0);
    dvec2 d_0 = dvec2(1.0, 0.0);
    
    double randYrange = 0;
    dvec2 x = x_0 + dvec2(0.0,randYrange*(2.0*random()-1.0));
    dvec2 xPrev = x_0;
    dvec2 d = d_0;
    bool simulatePath = true;
    double totalpathlength = 0.0;
      
    atomicCounterIncrement(numRays);
    
    double L = L_0;

    for (uint i = 0; i < pathLength; i++) {   
        if (simulatePath) {
            paths[bufIndex+i].xy = x;
            paths[bufIndex+i].z  = L;
            paths[bufIndex+i].w = 1.0;
            
            //get parameters
            ScatteringParameter sp = getParameterCombination(clamp(x.x/10.0, 0.0, 1.0), 
                                                             clamp((x.y+5.0)/10.0, 0.0,1.0),
                                                             color);
    
            //propagate
            //want it to go in half pixels so
            //dist in mm - 10.0mm / (volumeSize*2.0)
            double dist = 5.0/double(volumeSize);//-double(log(float(random())))/sp.sigma_t;
                  
            double u = random();
            //absorbed
            if (u < dist*sp.sigma_a) {
                //terminate
                simulatePath = false;
                for (uint j = 0; j < i; j++) {
                    paths[bufIndex+i].xy = xPrev;
                    paths[bufIndex+j].z = 0.0;
                    paths[bufIndex+j].w = -1.0;
                } 
                continue;
                //xPrev = x;
                // paths[bufIndex+i].w = 1;
               // continue;
            //scatter
            } else if (dist*sp.sigma_a < u && u < dist*sp.sigma_t) {
                double angle = atan(float(d.y), float(d.x));
                dvec2 phaseAngle = samplePhaseAngle();       
                
                double sampledPhase = phaseAngle.x;
                double sampledAngle = phaseAngle.y;
                               
                d = dvec2(cos(float(angle+sampledAngle)), sin(float(angle+sampledAngle)));
                          
                paths[bufIndex+i].w = 2; 
            } 
            
            //beer-lambert law for absorption
            L = L*exp(-float(dist*sp.sigma_a));
            
            xPrev = x;
            x += d*dist;
            
            totalpathlength += dist;
                                
            //if outside boundary
            if (x.x < 0.0 || x.x > 10.0 || x.y < -5.0 || x.y > 5.0) {
                simulatePath = false;
                
                float thetatest = 0.0;
                dvec2 dtest = dvec2(cos(thetatest), sin(thetatest));
                
                //in right exit region, exit direction, total length
                if (x.x > 10.0 &&  x.y > -0.25 && x.y < 0.25 && 
                    d.y > -0.2 && d.y < 0.2 &&
                     totalpathlength > 10.1 && totalpathlength < 10.15 ) {
                    x += d*1000000.0; //make ray go off to infinity to not affect image
                    xPrev = x;
                    L = 0.0;
                } else { //in wrong region kill it
                    //xPrev = vec2(-100.0, -100.0);
                    
                    for (uint j = 0; j < pathLength; j++) {                        
                        paths[bufIndex+j].xy = xPrev;
                        paths[bufIndex+j].z = 0.0;
                        paths[bufIndex+j].w = 0.0;
                    } 
                }
                continue;
            }
            
            //if ends inside boundary kill it or total path length is too long
            if ((i == pathLength-1)) {
                simulatePath = false;
                
               // xPrev = vec2(-100.0, -100.0);
                    
                for (uint j = 0; j < pathLength; j++) {                        
                    paths[bufIndex+j].xy = xPrev;
                    paths[bufIndex+j].z = 0.0;
                    paths[bufIndex+j].w = 0.0;
                } 
            }
        }
        else {       
            paths[bufIndex+i].xy = xPrev;
            paths[bufIndex+i].z = 0.0;
            paths[bufIndex+i].w = 0.0;
        }     
    }
}

void main()
{
    uint bufIndex = gl_GlobalInvocationID.x*pathLength; //get index to start of path buffer
    
    dvec3 laserColor = dvec3(1.0, 1.0, 1.0);
    
    //red laser has 3.1966643 × 10^18 photons/s
    //double intensity = 3.2e18LF;
    
    if (gl_GlobalInvocationID.x < numPaths) //red
        SimulatePath(laserColor.x, bufIndex, 0);
    else if (gl_GlobalInvocationID.x < 2*numPaths) //green
        SimulatePath(laserColor.y, bufIndex, 1);
    else // blue
        SimulatePath(laserColor.z, bufIndex, 2);
}