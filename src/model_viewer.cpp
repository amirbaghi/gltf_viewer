// Model viewer code for the assignments in Computer Graphics 1TD388/1MD150.
//
// Modify this and other source files according to the tasks in the instructions.
//

#include "gltf_io.h"
#include "gltf_scene.h"
#include "gltf_render.h"
#include "cg_utils.h"
#include "cg_trackball.h"

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdlib>
#include <iostream>

// Struct for representing a shadow casting point light
struct ShadowCastingLight {
    glm::vec3 position;      // Light source position
    glm::mat4 shadowMatrix;  // Camera matrix for shadowmap
    GLuint shadowmap;        // Depth texture
    GLuint shadowFBO;        // Depth framebuffer
    float shadowBias;        // Bias for depth comparison
};

// Struct for our application context
struct Context {
    int width = 512;
    int height = 512;
    GLFWwindow *window;
    gltf::GLTFAsset asset;
    gltf::DrawableList drawables;
    cg::Trackball trackball;
    GLuint program;
    GLuint emptyVAO;
    float elapsedTime;
    std::string gltfFilename = "armadillo.gltf";
    glm::vec4 backgroundColor = glm::vec4(0);
    
    // Shadow Mapping
    ShadowCastingLight light;
    GLuint shadowProgram;
    bool showShadowMap = false;

    // Lighting Parameters
    glm::vec3 ambientColor = glm::vec3(0.01);
    glm::vec3 lightPosition;
    glm::vec3 diffuseColor = glm::vec3(0.01);
    glm::vec3 specularColor = glm::vec3(0.04);
    float specularPower = 2.0f;

    // Textures
    gltf::TextureList textures;
    int baseColorTextureId = 9;
    int normalMapTextureId = 10;

    // Cube Map Active Texture ID
    int cubemapId;

    // Camera Parameters
    glm::mat4 projectionMatrix;
    float fov = 45.0f;
    float zoomFactor = 0.0f;
    float orthographicScale = 3.0f;

    // Flags
    bool useLighting = true;
    bool useDiffuseLighting = true;
    bool useAmbientLighting = true;
    bool useSpecularLighting = true;
    bool useNormalsAsColor = false;
    bool useOrthographicProjection = false;
    bool useGammaCorrection = true;
    bool useCubemap = false;

    bool visualiseTextureCoords = false;
    bool useDiffuseTexture = true;
    bool useNormalTexture = true;

    // Add more variables here...
};

// Returns the absolute path to the src/shader directory
std::string shader_dir(void)
{
    std::string rootDir = cg::get_env_var("MODEL_VIEWER_ROOT");
    if (rootDir.empty()) {
        std::cout << "Error: MODEL_VIEWER_ROOT is not set." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return rootDir + "/src/shaders/";
}

// Returns the absolute path to the assets/cubemaps directory
std::string cubemap_dir(void)
{
    std::string rootDir = cg::get_env_var("MODEL_VIEWER_ROOT");
    if (rootDir.empty()) {
        std::cout << "Error: MODEL_VIEWER_ROOT is not set." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return rootDir + "/assets/cubemaps/";
}

// Returns the absolute path to the assets/gltf directory
std::string gltf_dir(void)
{
    std::string rootDir = cg::get_env_var("MODEL_VIEWER_ROOT");
    if (rootDir.empty()) {
        std::cout << "Error: MODEL_VIEWER_ROOT is not set." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return rootDir + "/assets/gltf/";
}

void load_cubemaps(Context &ctx, std::string cubemap_name)
{
    // Load basic cubemap
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cg::load_cubemap(cubemap_dir() + cubemap_name));

    // Load pre-filtered cubemaps
    float filters[] = {0.125, 0.5, 2, 8, 32, 128, 512, 2048};
    for (int i = 7, idOffset = 0; i >= 0; i--, idOffset++)
    {
        glActiveTexture(GL_TEXTURE1 + idOffset);
        std::string filename = std::to_string(filters[i]);
        filename.erase ( filename.find_last_not_of('0') + 1, std::string::npos);
        filename.erase ( filename.find_last_not_of('.') + 1, std::string::npos);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cg::load_cubemap(cubemap_dir() + cubemap_name + "/prefiltered/" + filename));
    }
}

void initialize_shadow_map(Context &ctx)
{
    ctx.shadowProgram =
        cg::load_shader_program(shader_dir() + "shadow.vert", shader_dir() + "shadow.frag");

    ctx.light.shadowmap = cg::create_depth_texture(512, 512);
    ctx.light.shadowFBO = cg::create_depth_framebuffer(ctx.light.shadowmap);
    ctx.light.shadowBias = 0;
    ctx.light.shadowMatrix = glm::mat4(1.0f);
    ctx.light.position = glm::vec3(0, 5, 0);
}

void calculate_projection(Context &ctx)
{
    // If orthographic projection
    if (ctx.useOrthographicProjection)
    {
        // Calculate the aspect ratio
        float aspect = static_cast<float>(ctx.width) / static_cast<float>(ctx.height);
        // Create orthographic projection matrix using the context scale and aspect ratio
        ctx.projectionMatrix = glm::ortho(-aspect * ctx.orthographicScale, aspect * ctx.orthographicScale
                                          , -ctx.orthographicScale, ctx.orthographicScale, 0.1f, 100.0f);
    }
    // If prespective projection
    else
    {
        // Create prespective projection matrix using the context FOV
        ctx.projectionMatrix = glm::perspective(glm::radians(ctx.fov), (float)ctx.width/ctx.height, 1.0f, 100.0f);
    }    
}

// Update the shadowmap and shadow matrix for a light source
void update_shadowmap(Context &ctx, ShadowCastingLight &light, GLuint shadowFBO)
{
    // // Set up rendering to shadowmap framebuffer
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, shadowFBO);
    if (shadowFBO) glViewport(0, 0, 512, 512);  // Set viewport to shadowmap size
    glClear(GL_DEPTH_BUFFER_BIT);               // Clear depth values to 1.0

    // Set up pipeline
    glUseProgram(ctx.shadowProgram);
    // glUseProgram(ctx.program);
    glEnable(GL_DEPTH_TEST);  // Enable Z-buffering

    // Define view and projection matrices for the shadowmap camera. The
    // view matrix should be a lookAt-matrix computed from the light source
    // position, and the projection matrix should be a frustum that covers the
    // parts of the scene that shall recieve shadows.
    glm::mat4 view = glm::lookAt(glm::vec3(2, 2, 2), glm::vec3(0,0,0), glm::vec3(0,1,0)) * glm::mat4(ctx.trackball.orient);
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)512/512, 1.0f, 100.0f);

    glUniformMatrix4fv(glGetUniformLocation(ctx.shadowProgram, "u_view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(ctx.shadowProgram, "u_proj"), 1, GL_FALSE, &proj[0][0]);

    // Store updated shadow matrix for use in draw_scene()
    light.shadowMatrix = proj * view;

    // Draw scene
    for (unsigned i = 0; i < ctx.asset.nodes.size(); ++i) {
        const gltf::Node &node = ctx.asset.nodes[i];
        const gltf::Drawable &drawable = ctx.drawables[node.mesh];

        // Define the model matrix for the drawable
        glm::mat4 model = glm::scale(glm::toMat4(node.rotation) * glm::translate(glm::mat4(1.0), node.translation), node.scale);
        glUniformMatrix4fv(glGetUniformLocation(ctx.shadowProgram, "u_model"), 1, GL_FALSE, &model[0][0]);

        // Draw object
        glBindVertexArray(drawable.vao);
        glDrawElements(GL_TRIANGLES, drawable.indexCount, drawable.indexType,
                       (GLvoid *)(intptr_t)drawable.indexByteOffset);
        glBindVertexArray(0);
    }

    // Clean up
    cg::reset_gl_render_state();
    glUseProgram(0);
    glViewport(0, 0, ctx.width, ctx.height);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void do_initialization(Context &ctx)
{
    ctx.program = cg::load_shader_program(shader_dir() + "mesh.vert", shader_dir() + "mesh.frag");

    load_cubemaps(ctx, "Forrest");
    initialize_shadow_map(ctx);

    gltf::load_gltf_asset(ctx.gltfFilename, gltf_dir(), ctx.asset);
    gltf::create_drawables_from_gltf_asset(ctx.drawables, ctx.asset);
    gltf::create_textures_from_gltf_asset(ctx.textures, ctx.asset);
}

void draw_scene(Context &ctx)
{
    // Activate shader program
    glUseProgram(ctx.program);

    // Set render state
    glEnable(GL_DEPTH_TEST);  // Enable Z-buffering

    // Define per-scene uniforms
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::lookAt(glm::vec3(0, 0, 5), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0)) * glm::mat4(ctx.trackball.orient);
    calculate_projection(ctx);

    // Model and View matrices
    glUniformMatrix4fv(glGetUniformLocation(ctx.program, "u_view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(ctx.program, "u_projection"), 1, GL_FALSE, &ctx.projectionMatrix[0][0]);

    glm::mat4 shadowFromView = ctx.light.shadowMatrix * glm::inverse(view);
    glUniformMatrix4fv(glGetUniformLocation(ctx.program, "u_shadowFromView"), 1, GL_FALSE, &shadowFromView[0][0]);

    // Elapsed time
    glUniform1f(glGetUniformLocation(ctx.program, "u_time"), ctx.elapsedTime);

    // Textures and Cubemaps
    glUniform1i(glGetUniformLocation(ctx.program, "u_baseColorTexture"), ctx.baseColorTextureId);
    glUniform1i(glGetUniformLocation(ctx.program, "u_normalTexture"), ctx.normalMapTextureId);
    glUniform1i(glGetUniformLocation(ctx.program, "u_cubemap"), ctx.cubemapId);

    glActiveTexture(GL_TEXTURE11);
    glBindTexture(GL_TEXTURE_2D, ctx.light.shadowmap);
    glUniform1i(glGetUniformLocation(ctx.program, "u_shadowmap"), 11);

    // Lighting Parameters
    glUniform3fv(glGetUniformLocation(ctx.program, "u_lightPosition"), 1, &ctx.lightPosition[0]);
    glUniform3fv(glGetUniformLocation(ctx.program, "u_diffuseColor"), 1, &ctx.diffuseColor[0]);
    glUniform3fv(glGetUniformLocation(ctx.program, "u_specularColor"), 1, &ctx.specularColor[0]);
    glUniform1f(glGetUniformLocation(ctx.program, "u_specularPower"), ctx.specularPower);
    glUniform3fv(glGetUniformLocation(ctx.program, "u_ambientColor"), 1, &ctx.ambientColor[0]);

    // Flags
    glUniform1i(glGetUniformLocation(ctx.program, "u_useLighting"), ctx.useLighting);
    glUniform1i(glGetUniformLocation(ctx.program, "u_useDiffuseLighting"), ctx.useDiffuseLighting);
    glUniform1i(glGetUniformLocation(ctx.program, "u_useAmbientLighting"), ctx.useAmbientLighting);
    glUniform1i(glGetUniformLocation(ctx.program, "u_useSpecularLighting"), ctx.useSpecularLighting);
    glUniform1i(glGetUniformLocation(ctx.program, "u_useNormalsAsColor"), ctx.useNormalsAsColor);
    glUniform1i(glGetUniformLocation(ctx.program, "u_useGammaCorrection"), ctx.useGammaCorrection);
    glUniform1i(glGetUniformLocation(ctx.program, "u_useCubemap"), ctx.useCubemap);

    glUniform1i(glGetUniformLocation(ctx.program, "u_visualiseTexCoords"), ctx.visualiseTextureCoords);
    glUniform1i(glGetUniformLocation(ctx.program, "u_useDiffuseTexture"), ctx.useDiffuseTexture);
    glUniform1i(glGetUniformLocation(ctx.program, "u_useNormalTexture"), ctx.useNormalTexture);
    glUniform1i(glGetUniformLocation(ctx.program, "u_hasDiffuseTexture"), false);
    glUniform1i(glGetUniformLocation(ctx.program, "u_hasNormalTexture"), false);

    // ...

    // Draw scene
    for (unsigned i = 0; i < ctx.asset.nodes.size(); ++i) {
        const gltf::Node &node = ctx.asset.nodes[i];
        const gltf::Drawable &drawable = ctx.drawables[node.mesh];

        // Define per-object uniforms   
        model = glm::scale(glm::toMat4(node.rotation) * glm::translate(model, node.translation), node.scale);
        glUniformMatrix4fv(glGetUniformLocation(ctx.program, "u_model"), 1, GL_FALSE, &model[0][0]);
        // ...

        // Get Texture data for this node if it has any
        const gltf::Mesh &mesh = ctx.asset.meshes[node.mesh];
        if (mesh.primitives[0].hasMaterial)
        {
            const gltf::Primitive &primitive = mesh.primitives[0];
            const gltf::Material &material = ctx.asset.materials[primitive.material];
            const gltf::PBRMetallicRoughness &pbr = material.pbrMetallicRoughness;

            // Define material textures and uniforms
            // If there is a base color texture
            if (pbr.hasBaseColorTexture)
            {
                GLuint texture_id = ctx.textures[pbr.baseColorTexture.index];
                glActiveTexture(GL_TEXTURE0 + ctx.baseColorTextureId);
                glBindTexture(GL_TEXTURE_2D, texture_id);
                glUniform1i(glGetUniformLocation(ctx.program, "u_hasDiffuseTexture"), true);
            }
            //If there is a normal/bump map texture
            if (material.hasNormalTexture)
            {
                GLuint texture_id = ctx.textures[material.normalTexture.index];
                glActiveTexture(GL_TEXTURE0 + ctx.normalMapTextureId);
                glBindTexture(GL_TEXTURE_2D, texture_id);
                glUniform1i(glGetUniformLocation(ctx.program, "u_hasNormalTexture"), true);
            }

        }

        // Draw object
        glBindVertexArray(drawable.vao);
        glDrawElements(GL_TRIANGLES, drawable.indexCount, drawable.indexType,
                       (GLvoid *)(intptr_t)drawable.indexByteOffset);
        glBindVertexArray(0);
    }

    // Clean up
    cg::reset_gl_render_state();
    glUseProgram(0);
}

void do_rendering(Context &ctx)
{
    // Clear render states at the start of each frame
    cg::reset_gl_render_state();

    // Clear color and depth buffers
    glClearColor(ctx.backgroundColor.r, ctx.backgroundColor.g, ctx.backgroundColor.b, ctx.backgroundColor[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Update Shadow Map
    update_shadowmap(ctx, ctx.light, ctx.light.shadowFBO);
    draw_scene(ctx);

    if (ctx.showShadowMap)
    {
        glClearColor(1.0, 1.0, 1.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        update_shadowmap(ctx, ctx.light, 0);
    }
}

void reload_shaders(Context *ctx)
{
    glDeleteProgram(ctx->program);
    ctx->program = cg::load_shader_program(shader_dir() + "mesh.vert", shader_dir() + "mesh.frag");
}

void error_callback(int /*error*/, const char *description)
{
    std::cerr << description << std::endl;
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    // Forward event to ImGui
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
    if (ImGui::GetIO().WantCaptureKeyboard) return;

    Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
    if (key == GLFW_KEY_R && action == GLFW_PRESS) { reload_shaders(ctx); }
}

void char_callback(GLFWwindow *window, unsigned int codepoint)
{
    // Forward event to ImGui
    ImGui_ImplGlfw_CharCallback(window, codepoint);
    if (ImGui::GetIO().WantTextInput) return;
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    // Forward event to ImGui
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
    if (ImGui::GetIO().WantCaptureMouse) return;

    double x, y;
    glfwGetCursorPos(window, &x, &y);

    Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        ctx->trackball.center = glm::vec2(x, y);
        ctx->trackball.tracking = (action == GLFW_PRESS);
    }
}

void cursor_pos_callback(GLFWwindow *window, double x, double y)
{
    // Forward event to ImGui
    if (ImGui::GetIO().WantCaptureMouse) return;

    Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
    cg::trackball_move(ctx->trackball, float(x), float(y));
}

void update_fov(Context &ctx, double yOffset)
{
    // Update the zoom factor
    ctx.zoomFactor = yOffset;

    // Update the current FOV of the camera
    ctx.fov -= ctx.zoomFactor * 1.3f;

    // Limit the FOV between 1 to 100 degrees
    if (ctx.fov < 1)
    {
        ctx.fov = 1.0f;
    }
    else if (ctx.fov > 100.0f)
    {
        ctx.fov = 100.0f;
    }
}

void scroll_callback(GLFWwindow *window, double x, double y)
{
    // Forward event to ImGui
    ImGui_ImplGlfw_ScrollCallback(window, x, y);
    if (ImGui::GetIO().WantCaptureMouse) return;

    // Update the zoom parameter and camera's FOV
    Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
    update_fov(*ctx, y);
}

void resize_callback(GLFWwindow *window, int width, int height)
{
    // Update window size and viewport rectangle
    Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
    ctx->width = width;
    ctx->height = height;
    glViewport(0, 0, width, height);
}

void show_gui_widgets(Context& ctx)
{
    // Misc
    if (ImGui::CollapsingHeader("Projection"))
    {
        ImGui::Checkbox("Use Orthographic Projection", &ctx.useOrthographicProjection);
    }

    // Background Color
    if (ImGui::CollapsingHeader("Background"))
    {
        ImGui::ColorEdit4("Background Color", &ctx.backgroundColor[0]);
    }

    // Lighting
    if (ImGui::CollapsingHeader("Lighting"))
    {
        ImGui::Checkbox("Use Lighting", &ctx.useLighting);
        ImGui::ColorEdit3("Ambient Color", &ctx.ambientColor[0]);
        ImGui::Checkbox("Use Ambient Lighting", &ctx.useAmbientLighting);
        ImGui::ColorEdit3("Diffuse Color", &ctx.diffuseColor[0]);
        ImGui::Checkbox("Use Diffuse Lighting", &ctx.useDiffuseLighting);
        ImGui::ColorEdit3("Specular Color", &ctx.specularColor[0]);
        ImGui::InputFloat("Specular Power", &ctx.specularPower);
        ImGui::Checkbox("Use Specular Lighting", &ctx.useSpecularLighting);
        ImGui::SliderFloat3("Light Source Position", &ctx.lightPosition[0], 0.0f, 255.0f);
    }

    // Shadow Mapping
    if (ImGui::CollapsingHeader("Shadow Mapping"))
    {
        ImGui::Checkbox("Show Shadow Map", &ctx.showShadowMap);
    }

    // Enivronment Mapping
    if (ImGui::CollapsingHeader("Environment Mapping"))
    {
        ImGui::Checkbox("Use Environment Mapping", &ctx.useCubemap);
        ImGui::SliderInt("Cubemap Index", &ctx.cubemapId, 0, 8);
    }

    // Texture Mapping
    if (ImGui::CollapsingHeader("Texture Mapping"))
    {
        ImGui::Checkbox("Use Diffuse (Base Color) Texture", &ctx.useDiffuseTexture);
        ImGui::Checkbox("Use Normal Texture", &ctx.useNormalTexture);
        ImGui::Checkbox("Visualise Texture Coordinates", &ctx.visualiseTextureCoords);
    }

    // Misc
    if (ImGui::CollapsingHeader("Misc."))
    {
        ImGui::Checkbox("Use Normals as RGB Colors", &ctx.useNormalsAsColor);
        ImGui::Checkbox("Use Gamma Correction", &ctx.useGammaCorrection);
    }
}

int main(int argc, char *argv[])
{
    Context ctx = Context();
    if (argc > 1) { ctx.gltfFilename = std::string(argv[1]); }

    // Create a GLFW window
    glfwSetErrorCallback(error_callback);
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    ctx.window = glfwCreateWindow(ctx.width, ctx.height, "Model viewer", nullptr, nullptr);
    glfwMakeContextCurrent(ctx.window);
    glfwSetWindowUserPointer(ctx.window, &ctx);
    glfwSetKeyCallback(ctx.window, key_callback);
    glfwSetCharCallback(ctx.window, char_callback);
    glfwSetMouseButtonCallback(ctx.window, mouse_button_callback);
    glfwSetCursorPosCallback(ctx.window, cursor_pos_callback);
    glfwSetScrollCallback(ctx.window, scroll_callback);
    glfwSetFramebufferSizeCallback(ctx.window, resize_callback);

    // Load OpenGL functions
    if (gl3wInit() || !gl3wIsSupported(3, 3) /*check OpenGL version*/) {
        std::cerr << "Error: failed to initialize OpenGL" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;

    // Initialize ImGui
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(ctx.window, false /*do not install callbacks*/);
    ImGui_ImplOpenGL3_Init("#version 330" /*GLSL version*/);

    // Initialize rendering
    glGenVertexArrays(1, &ctx.emptyVAO);
    glBindVertexArray(ctx.emptyVAO);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    do_initialization(ctx);

    // Start rendering loop
    while (!glfwWindowShouldClose(ctx.window)) {
        glfwPollEvents();
        ctx.elapsedTime = glfwGetTime();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::ShowDemoWindow();
        show_gui_widgets(ctx);
        do_rendering(ctx);
        calculate_projection(ctx);
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());


        glfwSwapBuffers(ctx.window);
    }

    // Shutdown
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(ctx.window);
    glfwTerminate();
    std::exit(EXIT_SUCCESS);
}
