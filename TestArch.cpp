/*
 * TestArch.cpp
 *
 *  Created on: Dec 13, 2013
 *      Author: root
 */

#include <iostream>
#include "TestArch.h"
#include "cgl/cgl.h"

namespace cgl {

//storage structures for what the
// transform feedback outputs are
struct xfb0 {
    float position[4];
    float velocity[4];
};

struct xfb1 {
    float xfbTexCoord[2];
};

//Test update callback class
class Test0;
typedef std::shared_ptr<Test0> Test0Ptr;
class Test0: public cgl::UpdateCallbackClass {
    float _boogaloo;
public:
    Test0(float boogaloo) {
        _boogaloo = boogaloo;
    }
    static Test0Ptr New(float boogaloo) {
        return std::make_shared<Test0>(boogaloo);
    }

    void Update(double dt) {
        (void) (dt);
    }
};

class SimulationUpdateCallback;
typedef std::shared_ptr<SimulationUpdateCallback> SimulationUpdateCallbackPtr;
class SimulationUpdateCallback: public cgl::UpdateCallbackClass {
    TexturePtr _input;
    TexturePtr _output;
public:
    SimulationUpdateCallback(TexturePtr inputTexture,
            TexturePtr outputTexture) {
        _input = inputTexture;
        _output = outputTexture;
    }

    static SimulationUpdateCallbackPtr New(TexturePtr inputTexture,
            TexturePtr outputTexture) {
        return std::make_shared<SimulationUpdateCallback>(inputTexture,
                outputTexture);
    }

    void Update(double dt) {
        (void)dt;

        GLuint swapId = _input->GetID();
        _input->SetID(_output->GetID());
        _output->SetID(swapId);
    }
};

TestArch::TestArch() {
    //Texture Load image-------------------------------------------------------
    UnsignedByteImagePtr imgptr = UnsignedByteImage::New("Data/img_test.png");
    Texture2DPtr imtex0 = Texture2D::NewRandom(256, 256, GL_LINEAR, GL_LINEAR,
    GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    //Texture2D::New(0, GL_RGBA16F, imgptr, GL_LINEAR,
    // GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    Texture2DPtr imtex1 = Texture2D::New(0, GL_RGBA8, imgptr, GL_LINEAR,
    GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

    Texture2DPtr tex = Texture2D::New(0, GL_RGBA8, imgptr, GL_LINEAR,
    GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

    // Compile shader----------------------------------------------------------
    VertexShaderPtr vs = VertexShader::NewFromFile("shaders/TestArch/vertexShader.glsl");
    VertexShaderPtr xfb_vs = VertexShader::NewFromFile(
            "shaders/TestArch/transformFeedbackVtx.glsl");

    TessControlShaderPtr tcs = TessControlShader::NewFromFile(
            "shaders/TestArch/tessControlShader.glsl");
    TessEvaluationShaderPtr tes = TessEvaluationShader::NewFromFile(
            "shaders/TestArch/tessEval.glsl");

    FragmentShaderPtr fs = FragmentShader::NewFromFile(
            "shaders/TestArch/fragmentShader.glsl");
    FragmentShaderPtr fst = FragmentShader::NewFromFile(
            "shaders/TestArch/fragmentShaderWithTess.glsl");

    ComputeShaderPtr computeShader = ComputeShader::NewFromFile(
            "shaders/TestArch/computeShader.glsl");
    ProgramPtr computeProgram = Program::New(computeShader);

    ProgramPtr prog = Program::New(vs, fs);
    ProgramPtr xfbprog = Program::New(xfb_vs, fs);
    ProgramPtr progTess = Program::New(vs, tcs, tes, fst);

    //Vertex Buffers ----------------------------------------------------------
    FloatBufferPtr vdata = FloatBuffer::New(8);
    {
        vdata->_data[0] = -0.5;
        vdata->_data[1] = -0.5;

        vdata->_data[2] = 0.5;
        vdata->_data[3] = -0.5;

        vdata->_data[4] = -0.5;
        vdata->_data[5] = 0.5;

        vdata->_data[6] = 0.5;
        vdata->_data[7] = 0.5;
    }
    //(mutable, data, usage, stride, vertices)
    VertexBufferPtr vbuff = VertexBuffer::New(true, vdata, GL_STATIC_DRAW,
            sizeof(float) * 2, 4);
    //(numcomponents, normalized)
    vbuff->AddAttribute(2, false);

    FloatBufferPtr tdata = FloatBuffer::New(8);
    {
        tdata->_data[0] = 0.0;
        tdata->_data[1] = 0.0;

        tdata->_data[2] = 1.0;
        tdata->_data[3] = 0.0;

        tdata->_data[4] = 0.0;
        tdata->_data[5] = 1.0;

        tdata->_data[6] = 1.0;
        tdata->_data[7] = 1.0;
    }
    VertexBufferPtr tbuff = VertexBuffer::New(true, tdata, GL_STATIC_DRAW,
            sizeof(float) * 2, 4);
    tbuff->AddAttribute(2, false);

    VertexSpecificationPtr vspec = VertexSpecification::New();
    vspec->Append(vbuff);
    vspec->Append(tbuff);
    vspec->Set();

    //UNIFORMS-----------------------------------------------------------------
    _fbuf = FloatBuffer::New(9);
    {
        _fbuf->_data[0] = 0.0;
        _fbuf->_data[1] = 1.0;
        _fbuf->_data[2] = 0.0;
        _fbuf->_data[3] = 1.0;

        _fbuf->_data[4] = 0.0;
        _fbuf->_data[5] = 0.0;
        _fbuf->_data[6] = 1.0;
        _fbuf->_data[7] = 1.0;

        _fbuf->_data[8] = 0.0;
    }
    //(mutable, data, usage, program, buffername, binding point)
    _ubuf = UniformBuffer::New(true, _fbuf, GL_DYNAMIC_DRAW, prog, "TestBuffer",
            0);

    FloatBufferPtr udata2 = FloatBuffer::New(8);
    {
        udata2->_data[0] = 1.0;
        udata2->_data[1] = 0.0;
        udata2->_data[2] = 0.0;
        udata2->_data[3] = 1.0;

        udata2->_data[4] = 0.0;
        udata2->_data[5] = 0.0;
        udata2->_data[6] = 0.0;
        udata2->_data[7] = 1.0;
    }
    UniformBufferPtr ubuf2 = UniformBuffer::New(true, udata2,
    GL_DYNAMIC_DRAW, prog, "TestBuffer2", 1);

    //SSBOs
    uint32_t numPaths = 10000;
    uint32_t pathLength = 128;

    UnsignedIntBufferPtr pathDataBuffer = UnsignedIntBuffer::New(2);
    {
        pathDataBuffer->_data[0] = numPaths;
        pathDataBuffer->_data[1] = pathLength;
    }

    FloatBufferPtr pathOutputBuffer = FloatBuffer::New(
            numPaths * pathLength * 2);
    {
        for (uint32_t i = 0; i < pathOutputBuffer->Size(); i++)
            pathOutputBuffer->_data[i] = 0.0;
    }
    //(mutable, data, usage, bindingpoint)
    ShaderStorageBufferPtr pathDataSSBO = ShaderStorageBuffer::New(true,
            pathDataBuffer,
            GL_STATIC_READ);
    ShaderStorageBufferPtr pathOutputSSBO = ShaderStorageBuffer::New(true,
            pathOutputBuffer,
            GL_DYNAMIC_DRAW);

    //Transform feedback-------------------------------------------------------
    //make VBOs for transform feedback object (TFO) to render into
    VertexBufferPtr xfvbo0 = VertexBuffer::New(true, GL_FLOAT,
    GL_DYNAMIC_COPY, sizeof(xfb0), 4);
    {
        xfvbo0->AddAttribute(4, false);
        xfvbo0->AddAttribute(4, false);
    }
    VertexBufferPtr xfvbo1 = VertexBuffer::New(true, GL_FLOAT,
    GL_DYNAMIC_COPY, sizeof(xfb1), 4);
    {
        xfvbo1->AddAttribute(2, false);
    }

    //set varyings for output buffers, xfbPosition/Velocity will go to outputbuffer 0
    // interleaved and texcoord to outputbuffer 1
    static const char* const varyings[] = { "xfbPosition", "xfbVelocity",
            "gl_NextBuffer", "xfbTexCoord" };
    TransformFeedbackObjectPtr tfo = TransformFeedbackObject::New(xfbprog,
            varyings);
    {
        tfo->Append(xfvbo0); //output buffer 0
        tfo->Append(xfvbo1); //output buffer 1
    }
    tfo->Set();

    //make a vertex specification to render from
    VertexSpecificationPtr vspectfo = VertexSpecification::New();
    vspectfo->Append(xfvbo0); //2 buffer attributes - binding 0, 1
    vspectfo->Append(xfvbo1); //1 buffer attribute -  binding 2
    vspectfo->Set();

    //Pass 0 - Compute pass to generate texture
    std::cout << "Width x Height " << imgptr->Width() << "x" << imgptr->Height()
            << std::endl;
    ComputePassPtr pass0 = ComputePass::New(computeProgram, 16, 16, 1);
    pass0->AppendBuffer(pathDataSSBO, 0);
    pass0->AppendBuffer(pathOutputSSBO, 1);

    //Pass 1 - Render points, transforms points into other points via transform feedback
    DrawModePtr drawModePts = DrawArrays::New(GL_POINTS, 0, 4);
    DrawPassPtr pass1 = DrawPass::New(xfbprog, vspec, drawModePts, true);
    {
        pass1->SetTransformFeedbackOutput(tfo);

        Test0Ptr t0 = Test0::New(5.0);
        pass1->SetUpdateCallback(t0);
    }

    // Framebuffer object ---------------------------------------------------------
    DrawFramebufferPtr fbo = DrawFramebuffer::New(2, GL_RGBA, 1920, 1080);

    ClearPassPtr clearBuffer1 = ClearPass::New(fbo, GL_COLOR_BUFFER_BIT, 0.0,
            1.0, 1.0, 1.0);

    DrawModePtr drawMode = DrawArrays::New(GL_TRIANGLE_STRIP, 0, 4);
    DrawPassPtr pass2 = DrawPass::New(prog, vspectfo, drawMode, false);
    {
        pass2->AppendTexture(imtex1); //binding 0
        pass2->AppendTexture(imtex1); //binding 1

        pass2->SetFramebufferOutput(fbo);
        pass2->SetTransformFeedbackInput(tfo);

        Test0Ptr t1 = Test0::New(1.0);
        pass2->SetUpdateCallback(t1);
    }

    ClearPassPtr clearBuffer2 = ClearPass::New(nullptr, GL_COLOR_BUFFER_BIT,
            1.0, 0.0, 0.0, 1.0);

    DrawModePtr drawModePatch = DrawArrays::New(GL_PATCHES, 0, 4);
    DrawPassPtr pass3 = DrawPass::New(progTess, vspectfo, drawModePatch, false);
    {
        pass3->AppendTexture(fbo->GetColorBuffer(0));
        pass3->AppendTexture(fbo->GetColorBuffer(1));
        pass3->Disable(GL_CULL_FACE);
        pass3->SetPatchVertices(4);

        pass3->SetTransformFeedbackInput(tfo);
    }

    AddPass(pass0);
    AddPass(pass1);
    AddPass(clearBuffer1);
    AddPass(pass2);
    AddPass(clearBuffer2);
    AddPass(pass3);

    _interp = 0.0;
}

void TestArch::Update(double dt) {
    _interp += dt;
    if (_interp > 1.0)
        _interp = 0.0;

    //_fbuf->_data[8] = _interp;
    //_ubuf->UploadAll();
}

}
