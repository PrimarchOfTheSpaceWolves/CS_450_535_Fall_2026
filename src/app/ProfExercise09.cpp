#include <iostream>
#include <string>
#define PRO_USE_GLFW
#include "pro/Prometheus.hpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
using namespace std;

struct ForgeVertex {
    glm::vec3 pos;
};

bool didWindowResize = false;

static void window_resize_callback( GLFWwindow *window, 
                                    int width, int height) {
    didWindowResize = true;
}

void printRM(string name, glm::mat2 &M) {
    cout << name << ":" << endl;
    for(int i = 0; i < 2; i++) {
        for(int j = 0; j < 2; j++) {
            cout << M[j][i] << ", ";
        }
        cout << endl;
    }
}

void printRM(string name, glm::mat3 &M) {
    cout << name << ":" << endl;
    for(int i = 0; i < 3; i++) {
        for(int j = 0; j < 3; j++) {
            cout << M[j][i] << ", ";
        }
        cout << endl;
    }
}

void printRM(string name, glm::mat4 &M) {
    cout << name << ":" << endl;
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            cout << M[j][i] << ", ";
        }
        cout << endl;
    }
}

int main(int argc, char **argv) {

    /*
    cout << argc << " ARGUMENTS" << endl;
    for(int i = 0; i < argc; i++) {
        cout << i << ": " << argv[i] << endl;
    }

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        "sampleData/models/bunnyteatime.glb",
        aiProcess_Triangulate | aiProcess_FlipUVs |
        aiProcess_GenNormals | aiProcess_JoinIdenticalVertices
    );

    if(!scene || 
        scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
        !scene->mRootNode
    ) {
        cerr << "ERROR: " << importer.GetErrorString() << endl;
        exit(1);
    }

    for(unsigned int i = 0; i < scene->mNumMeshes; i++) {
        cout << "MESH " << i << ":" << endl;
        cout << "Num verts: " << scene->mMeshes[i]->mNumVertices << endl;
    }

    cout << "BEGIN GLM EXERCISE" << endl;
    glm::vec3 a = {1,4,0};
    glm::vec3 b = {2,3,2};
    cout << "a.x/r: " << a.x << " " << a.r << endl;
    cout << "a: " << glm::to_string(a) << endl;
    cout << "b: " << glm::to_string(b) << endl;
    glm::vec3 c = b - a;
    cout << "c: " << glm::to_string(c) << endl;

    float alen = glm::length(a);
    cout << "Length a: " << alen << endl;

    a = 5.0f*a;
    alen = glm::length(a);
    cout << "New a: " << glm::to_string(a) << endl;
    cout << "New length a: " << alen << endl;
    
    glm::vec3 normA = glm::normalize(a);
    float anormlen = glm::length(normA);
    cout << "normA: " << glm::to_string(normA) << endl;
    cout << "normA len: " << anormlen << endl;

    glm::vec3 normB = glm::normalize(b);
    float dotAB = glm::dot(normA, normB);
    float degAB = glm::degrees(glm::acos(dotAB));
    cout << "dotAB: " << dotAB << endl;
    cout << "degAB: " << degAB << endl;

    glm::vec3 d = {0,0,1};
    float dotAD = glm::dot(normA, d);
    float degAD = glm::degrees(glm::acos(dotAD));
    cout << "dotAD: " << dotAD << endl;
    cout << "degAD: " << degAD << endl;

    glm::vec3 e = {7,8,0};
    cout << "a: " << glm::to_string(a) << endl;
    cout << "e: " << glm::to_string(e) << endl;
    glm::vec3 crossAE = glm::cross(a,e);
    glm::vec3 crossEA = glm::cross(e,a);
    cout << "A x E = " << glm::to_string(crossAE) << endl;
    cout << "E x A = " << glm::to_string(crossEA) << endl;

    glm::vec3 D = {0,1,-1};
    D = glm::normalize(D);
    glm::vec3 W = -D;
    glm::vec3 T = {0,1,0};
    glm::vec3 U = glm::normalize(glm::cross(T,W));
    glm::vec3 V = glm::normalize(glm::cross(W,U));
    cout << "U = " << glm::to_string(U) << endl;
    cout << "V = " << glm::to_string(V) << endl;
    cout << "W = " << glm::to_string(W) << endl;
    cout << glm::dot(U,V) << " "
         << glm::dot(U,W) << " "
         << glm::dot(V,W) << endl;

    glm::vec3 P = {2,0,-3};
    float u = glm::dot(P,U);
    float v = glm::dot(P,V);
    float w = glm::dot(P,W);
    cout << u << "," << v << "," << w << endl;
    */

    glm::mat3 M = glm::mat3(1,2,3,4,5,6,7,8,9);
    printRM("M", M);
    glm::mat3 I = glm::mat3(1.0);
    glm::mat3 Z = glm::mat3(0.0);
    glm::mat3 F = glm::mat3(5.0);
    printRM("I", I);
    printRM("Z", Z);
    printRM("F", F);
    glm::vec3 U = glm::normalize(glm::vec3(1,1,0));
    glm::vec3 V = glm::normalize(glm::vec3(-1,1,0));
    glm::vec3 W = glm::normalize(glm::cross(U,V));
    glm::mat3 B = glm::mat3(U,V,W);
    printRM("B", B);

    glm::mat3 Mt = glm::transpose(M);
    printRM("Mt", Mt);

    glm::mat3 A = glm::mat3(1,0,0,0,2,0,0,0,1);
    glm::mat3 C = glm::mat3(0.5,0,0,0,1,0,0,0,1);
    printRM("A", A);
    printRM("C", C);
    glm::mat3 T = C*A;
    printRM("T", T);
    glm::vec3 p = glm::vec3(2,4,8);
    glm::vec3 np = T*p;
    cout << "p: " << glm::to_string(p) << endl;
    cout << "np: " << glm::to_string(np) << endl;

    glm::vec2 g = glm::vec2(5,1);
    glm::vec2 h = glm::vec2(-2,6);
    glm::mat2 K = glm::mat2(g,h);
    printRM("K", K);
    float areaT = glm::determinant(K)/2.0f;
    cout << "Area triangle: " << areaT << endl;
    K = glm::mat2(h,g);
    areaT = glm::determinant(K)/2.0f;
    cout << "Area triangle now: " << areaT << endl;


    cout << "BEGIN VULKAN EXERCISE" << endl;

    if(!glfwInit()) {
        cerr << "Error: GLFW did not init!" << endl;
        exit(1);
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, true);

    string appName = "ProfExercise09";
    string windowTitle = appName + ": realemj";
    GLFWwindow *window = glfwCreateWindow(800, 600, windowTitle.c_str(),
                                            nullptr, nullptr);

    if(!window) {
        cerr << "Error: Could not create window!" << endl;
        glfwTerminate();
        exit(1);
    }

    glfwSetFramebufferSizeCallback(window, window_resize_callback);

    {
        pro::VulkanCoreCreateInfo coreCreateInfo {};
        coreCreateInfo.appName = appName;

        pro::prepareVulkanInitGLFWFunctions(coreCreateInfo, window);

        pro::VulkanCore vkCore(coreCreateInfo);

        pro::CommandData frameCmd = pro::createFrameCommandData(vkCore);

        pro::VulkanPipelineCreateInfo pipelineCreateInfo(vkCore);

        pipelineCreateInfo.shaderInfo = {
            pro::VulkanShaderCreateInfo(
                "build/compiledshaders/" + appName + "/shader.vert.spv",
                vk::ShaderStageFlagBits::eVertex
            ),
            pro::VulkanShaderCreateInfo(
                "build/compiledshaders/" + appName + "/shader.frag.spv",
                vk::ShaderStageFlagBits::eFragment
            )
        };

        pipelineCreateInfo.bindDesc = vk::VertexInputBindingDescription(
            0, sizeof(ForgeVertex), vk::VertexInputRate::eVertex
        );

        pipelineCreateInfo.attribDesc = {
            vk::VertexInputAttributeDescription(
                0, // location
                0, // binding
                vk::Format::eR32G32B32Sfloat, // (x,y,z)
                offsetof(ForgeVertex, pos)
            )
        };

        pro::VulkanPipelineData pipelineData
         = pro::createVulkanPipeline(vkCore.device(), pipelineCreateInfo);

        vector<pro::HostMesh<ForgeVertex>> allHostMeshes {};
        pro::HostMesh<ForgeVertex> hostMesh {};
        hostMesh.vertices = {
            {{-0.5f, -0.5f, 0.5f}},
            {{0.5f, -0.5f, 0.5f}},
            {{0.5f, 0.5f, 0.5f}},
            {{-0.5f, 0.5f, 0.5f}}
        };
        hostMesh.indices = {
            0,2,3,
            2,0,1
        };
        allHostMeshes.push_back(hostMesh);

        vector<pro::VulkanMesh> allMeshes {};
        allMeshes.resize(allHostMeshes.size());

        // Host-visible version
        for(int i = 0; i < allHostMeshes.size(); i++) {
            allMeshes[i] = pro::createVulkanMesh(
                vkCore.allocator(),
                allHostMeshes[i],
                false
            );
            pro::copyToHostVisibleVulkanMesh(
                vkCore.allocator(),
                allMeshes[i],
                allHostMeshes[i]
            );
        }

        uint32_t framesRendered = 0;
        int numberFramesInFlight = 1;

        vk::QueryPoolCreateInfo qi {};
        qi.queryType = vk::QueryType::eTimestamp;
        qi.queryCount = 2;
        auto qPool = vk::raii::QueryPool(vkCore.device(), qi);

        while(!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            if(didWindowResize) {
                cout << "Resized!" << endl;
                vkCore.doWindowResize();
                didWindowResize = false;
            }

            uint32_t flightIndex = framesRendered % numberFramesInFlight;
            uint32_t swapIndex = pro::acquireNextSwapImage(vkCore, frameCmd);
            frameCmd.beginRecording();
            frameCmd.buffer().resetQueryPool(qPool, 0, 2);
            frameCmd.buffer().writeTimestamp2(
                vk::PipelineStageFlagBits2::eTopOfPipe, qPool, 0);

            pro::performImageTransition(
                frameCmd.buffer(),
                vkCore.swapchain().swaps[swapIndex].image,
                pro::IMAGE_STATE_TYPE::UNDEF,
                pro::IMAGE_STATE_TYPE::COLOR
            );

            auto colorAtt = pro::createColorAttachment(
                vkCore.swapchain().swaps[swapIndex].view,
                {1.0f, 1.0f, 0.0f, 1.0f}
            );

            vk::RenderingInfoKHR ri {};
            ri.setRenderArea(vk::Rect2D({0,0}, vkCore.swapchain().extent));
            ri.setLayerCount(1);
            ri.setColorAttachments(colorAtt);
            frameCmd.buffer().beginRendering(ri);
            frameCmd.buffer().bindPipeline(
                vk::PipelineBindPoint::eGraphics,
                pipelineData.pipeline
            );

            vk::Viewport vps[] = { pro::makeDefaultViewport(vkCore) };
            frameCmd.buffer().setViewport(0, vps);

            vk::Rect2D scs[] = { pro::makeDefaultScissors(vkCore) };
            frameCmd.buffer().setScissor(0, scs);

            for(int i = 0; i < allMeshes.size(); i++) {
                pro::recordDrawVulkanMesh(frameCmd.buffer(), allMeshes[i]);
            }

            frameCmd.buffer().endRendering();

            pro::performImageTransition(
                frameCmd.buffer(),
                vkCore.swapchain().swaps[swapIndex].image,
                pro::IMAGE_STATE_TYPE::COLOR,
                pro::IMAGE_STATE_TYPE::PRESENT
            );

            frameCmd.buffer().writeTimestamp2(
                vk::PipelineStageFlagBits2::eBottomOfPipe, qPool, 1);

            frameCmd.endRecording();
            pro::submitForFrame(vkCore, frameCmd, swapIndex);

            if(!pro::presentSwapImage(vkCore, swapIndex)) {
                cerr << "Warning: Present failed." << endl;
            }

            framesRendered++;

            auto poolResult = qPool.getResults<uint64_t>(
                0, 2, 2*sizeof(uint64_t), sizeof(uint64_t),
                vk::QueryResultFlagBits::e64 | vk::QueryResultFlagBits::eWait
            );
            vector<uint64_t> results = poolResult.value;
            auto props = vkCore.physicalDevice().getProperties();
            double nsPerTick = props.limits.timestampPeriod;
            double timePassed = (results[1] - results[0])*nsPerTick;
            //cout << "TIME PASSED: " << timePassed << endl;
        }
        
        vkCore.device().waitIdle();
    } // Cleanup starts here

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
