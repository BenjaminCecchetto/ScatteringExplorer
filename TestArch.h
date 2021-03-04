/*
 * TestArch.h
 *
 *  Created on: Dec 13, 2013
 *      Author: root
 */

#ifndef CGL_TESTARCH_H_
#define CGL_TESTARCH_H_

#include <memory>
#include "cgl/cgl.h"


namespace cgl {

class TestArch;
typedef std::shared_ptr<TestArch> TestArchPtr;

class TestArch : public Architecture {
    UniformBufferPtr _ubuf;
    FloatBufferPtr _fbuf;
    float _interp;

public:
    TestArch();

	static TestArchPtr New() { return std::make_shared<TestArch>(); }

	void Update(double dt);
};

}

#endif /* TESTARCH_H_ */
