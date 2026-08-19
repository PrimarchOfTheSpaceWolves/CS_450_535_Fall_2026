#pragma once
#include "ProBase.hpp"

namespace pro {
    ///////////////////////////////////////////////////////////////////////////
    // FUNCTIONS 
    ///////////////////////////////////////////////////////////////////////////

    inline glm::mat4 createInfiniteFarReverseZPerspective(float fovY, float aspect, float near) {
        // Compute focal length
        float focalLen = 1.0f / glm::tan(fovY * 0.5f);
    
        // Initialize projection matrix to all zeros
        glm::mat4 projMat(0.0f);

        // Set values
        projMat[0][0] = focalLen / aspect;
        projMat[1][1] = focalLen;
        projMat[3][2] = near;
        projMat[2][3] = -1.0f;

        // Return result
        return projMat;
    };
}