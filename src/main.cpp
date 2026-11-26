/*

cmake -S . -B build
cmake --build build
./build/allc

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/allc

*/

#include <iostream>

#include <GLFW/glfw3.h>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "../imgui/imgui.h"
#include "../imgui/backends/imgui_impl_glfw.h"
#include "../imgui/backends/imgui_impl_opengl3.h"

// what do i want this to be??

// jank physics engine??
// simulation of 3d bodies??
// surviving??


// built in

float vertices[] = {
     0.5f,  0.5f, 0.0f,
     0.5f, -0.5f, 0.0f,
    -0.5f, -0.5f, 0.0f,
    -0.5f,  0.5f, 0.0f
};

unsigned int indices[] = {
    0, 1, 3,
    1, 2, 3
};

// outside shader sources
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main() {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;

    void main() {
        FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
    }
)";

class projectApplication {
public:

    // important class variables

    // window
    GLFWwindow *window;

    // gl
    GLuint shader;

    // camera
    glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f, 3.0f);
    glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f,  0.0f);

    float sensitivity = 0.5f;

    float pitch = 0.0f;
    float yaw = 0.0f;

    glm::dvec2 lastMouse = glm::dvec2(0.0f);

    // time measures
    float dt = 0.0f;
    float lastTime = 0.0f;

    // call in main to run EVERYTHING
    void run() 
    {
        initWindow();
        mainLoop();
        cleanUp();
    }

    // everything else

    void initWindow()
    {
        printf("Initializing Window.\n");
        glfwInit();

        // setup opengl
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        // make window
        window = glfwCreateWindow(500, 500, "Puzzling!", nullptr, nullptr);
        glfwMakeContextCurrent(window);

        // init shader program
        shader = createShaderProgram(vertexShaderSource, fragmentShaderSource);

        // enable depth test
        glEnable(GL_DEPTH_TEST);

        // other inits
        createSimpleModel();
    }


    void mainLoop()
    {
        printf("Started The Loop.\n");

        // for no jumpy startups
        glfwGetCursorPos(window, &lastMouse.x, &lastMouse.y);

        // imgui io
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();

        while (!glfwWindowShouldClose(window))
        {
            // events
            glfwSwapBuffers(window);
            glfwPollEvents();

            // clear screen
            glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // delta time calculations
            float thisTime = static_cast<float>(glfwGetTime());
            dt = thisTime - lastTime;
            lastTime = thisTime;

            // mouse movement calculations
            glm::dvec2 thisMouse;
            glfwGetCursorPos(window, &thisMouse.x, &thisMouse.y);
            glm::dvec2 deltaMouse = thisMouse - lastMouse;
            lastMouse = thisMouse;

            // LOCK MOUSE
            if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !io.WantCaptureMouse) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_RAW_MOUSE_MOTION);

                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }

            // if press escape unlock
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
            
            // if mouse locked you can look around
            if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
                yaw += deltaMouse.x * sensitivity;
                pitch += -deltaMouse.y * sensitivity;
                pitch = glm::clamp(pitch, -89.9f, 89.9f);
            }
            
            // rebuild the front vector
            glm::vec3 Gfront;
            Gfront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
            Gfront.y = sin(glm::radians(pitch));
            Gfront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
            front = glm::normalize(Gfront);


            // update camera pos and matrices
            updateCamera(window);

            glm::mat4 model = glm::mat4(1.0f);
            glm::mat4 view = glm::lookAt(cameraPos, cameraPos + front, up);
            
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);
            glViewport(0, 0, width, height);
            float aspect = (height > 0) ? static_cast<float>(width) / static_cast<float>(height) : 1.0f;
            glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);

            glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
            glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));


            // rendering
            glUseProgram(shader);
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }

        printf("Ended The Loop.\n");
    }

    void cleanUp()
    {
        printf("Cleaning Up.\n");

        glDeleteProgram(shader);

        glfwDestroyWindow(window);
        glfwTerminate();
    }

    // OTHER FUNCTIONS AHEAD.

    // modeling
    unsigned int VAO, VBO, EBO;

    void createSimpleModel() 
    {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        // upload vertex
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        // upload index
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        // vertex attrib 0
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // unbinding
        glBindVertexArray(0);
    }

    // build the shader program
    GLuint createShaderProgram(const char* vertSrc, const char* fragSrc) 
    {
        // VERTEX
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertSrc, nullptr);
        glCompileShader(vertexShader);

        GLint success;
        char infoLog[512];
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
            std::cerr << "Vertex Shader Error:\n" << infoLog << '\n';
        }

        // FRAGMENT
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragSrc, nullptr);
        glCompileShader(fragmentShader);

        // ERRORS?
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
            std::cerr << "Fragment Shader Error:\n" << infoLog << '\n';
        }

        // LINK
        GLuint shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);

        // ERRORS? (fragment edition!)
        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
            std::cerr << "Shader Program Link Error:\n" << infoLog << '\n';
        }

        // DELETE
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        return shaderProgram;
    }

    // update camera based on key input and other camera stuff
    void updateCamera(GLFWwindow* window) {
        float speed = 2.5f * dt;
        glm::vec3 movedirection = glm::vec3(0.0f);

        // get right vector
        glm::vec3 right = glm::cross(front, up);
        
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            movedirection += glm::vec3(0.0f, 0.0f, 1.0f);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            movedirection += glm::vec3(0.0f, 0.0f, -1.0f);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            movedirection += glm::vec3(-1.0f, 0.0f, 0.0f);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            movedirection += glm::vec3(1.0f, 0.0f, 0.0f);

        if (glm::length(movedirection) != 0.0f) {
            glm::vec3 upM = up * movedirection.y;
            glm::vec3 rightM = right * movedirection.x;
            glm::vec3 frontM = front * movedirection.z;

            glm::vec3 wantDirection = glm::normalize(upM + rightM + frontM) * speed;
            cameraPos += wantDirection;
        }
    }

    // copy and pasted from another place
    void drawImGui(ImGuiIO& io) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Window");
        ImGui::Text("Welcome, OpenGL!");
        ImGui::Text("FPS: %.1f", io.Framerate);

        //ImGui::SliderInt("Renderdist", &rd, 2, 64);
        //ImGui::SliderFloat("FOV", &FOV, 30.0f, 256.0f);
        //ImGui::SliderFloat("Speed", &walkSpeed, 16.0f, 256.0f);

        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
};

int main() {
    
    projectApplication app;

    app.run();

    return 0;
}