#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <random>
#include <vector>

namespace {
constexpr float Pi = 3.14159265359f;
constexpr int ParticleCount = 4200;

struct Particle {
    float angle;
    float radius;
    float speed;
    float size;
    float brightness;
};

struct Simulation {
    float mass = 8.6f;
    float spin = 0.42f;
    float timeScale = 1.0f;
    bool paused = false;
    bool showGrid = true;
    std::vector<Particle> particles;
};

Simulation simulation;
double lastTime = 0.0;
int frameCount = 0;
float cameraYaw = -24.0f;
float cameraPitch = 18.0f;
float cameraDistance = 7.5f;
bool draggingCamera = false;
double lastMouseX = 0.0;
double lastMouseY = 0.0;

void seedParticles() {
    std::mt19937 generator(static_cast<unsigned>(std::time(nullptr)));
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);
    simulation.particles.clear();
    simulation.particles.reserve(ParticleCount);

    for (int index = 0; index < ParticleCount; ++index) {
        const float ring = unit(generator);
        simulation.particles.push_back({
            unit(generator) * 2.0f * Pi,
            0.22f + std::pow(ring, 0.62f) * 0.92f,
            0.25f + unit(generator) * 0.9f,
            1.0f + unit(generator) * 2.2f,
            0.22f + unit(generator) * 0.78f
        });
    }
}

void drawBackground(float width, float height) {
    glDisable(GL_BLEND);
    glBegin(GL_QUADS);
    glColor3f(0.018f, 0.022f, 0.03f); glVertex2f(0.0f, 0.0f);
    glColor3f(0.035f, 0.028f, 0.025f); glVertex2f(width, 0.0f);
    glColor3f(0.012f, 0.016f, 0.024f); glVertex2f(width, height);
    glColor3f(0.02f, 0.023f, 0.03f); glVertex2f(0.0f, height);
    glEnd();

}

void drawSpacetimeGrid(float centerX, float centerY, float width, float height) {
    if (!simulation.showGrid) return;
    glColor4f(0.17f, 0.16f, 0.14f, 0.15f);
    glLineWidth(1.0f);
    for (int line = -12; line <= 12; ++line) {
        const float offset = line * 0.055f;
        glVertex2f(0.0f, centerY + offset * height); glVertex2f(width, centerY + offset * height);
        glVertex2f(centerX + offset * width, 0.0f); glVertex2f(centerX + offset * width, height);
    }
}

void setPerspective(float fieldOfView, float aspect, float nearPlane, float farPlane) {
    const float top = std::tan(fieldOfView * Pi / 360.0f) * nearPlane;
    const float right = top * aspect;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-right, right, -top, top, nearPlane, farPlane);
    glMatrixMode(GL_MODELVIEW);
}

float warpedHeight(float x, float z) {
    const float radius = std::sqrt(x * x + z * z);
    return -0.72f / (radius + 0.34f);
}

void drawWarpedSpacetime() {
    glDisable(GL_BLEND);
    glColor3f(0.16f, 0.14f, 0.12f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    for (int line = -12; line <= 12; ++line) {
        const float offset = line * 0.32f;
        glBegin(GL_LINE_STRIP);
        for (int step = -24; step <= 24; ++step) {
            const float position = step * 0.32f;
            glVertex3f(position, warpedHeight(position, offset), offset);
        }
        glEnd();
        glBegin(GL_LINE_STRIP);
        for (int step = -24; step <= 24; ++step) {
            const float position = step * 0.32f;
            glVertex3f(offset, warpedHeight(offset, position), position);
        }
        glEnd();
    }
    glEnd();
}

void drawLensRays() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glLineWidth(2.0f);
    for (int ray = 0; ray < 7; ++ray) {
        const float height = (ray - 3) * 0.18f;
        glColor4f(0.2f, 0.65f, 0.95f, 0.42f);
        glBegin(GL_LINE_STRIP);
        for (int step = -40; step <= 40; ++step) {
            const float x = step * 0.14f;
            const float bend = 0.38f / (std::abs(x) + 0.22f);
            glVertex3f(x, height + 0.35f + bend * 0.12f, 1.35f + bend * 0.32f);
        }
        glEnd();
    }
    const float objectX = std::fmod(static_cast<float>(glfwGetTime()) * 0.8f, 11.0f) - 5.5f;
    const float objectBend = 0.38f / (std::abs(objectX) + 0.22f);
    glPointSize(8.0f);
    glColor4f(0.72f, 0.9f, 1.0f, 1.0f);
    glBegin(GL_POINTS);
    glVertex3f(objectX, 0.35f + objectBend * 0.12f, 1.35f + objectBend * 0.32f);
    glEnd();
}

void drawBlackHole3D(float delta) {
    const float rotation = static_cast<float>(glfwGetTime()) * simulation.timeScale * (0.7f + simulation.spin * 2.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    for (Particle& particle : simulation.particles) {
        if (!simulation.paused) particle.angle += delta * particle.speed * simulation.timeScale * (1.0f + simulation.spin * 2.0f) / particle.radius;
        const float radius = 0.42f + particle.radius * 2.25f;
        const float x = std::cos(particle.angle) * radius;
        const float z = std::sin(particle.angle) * radius;
        const float heat = std::clamp(1.0f - particle.radius, 0.0f, 1.0f);
        const float approach = 0.5f + std::sin(particle.angle) * 0.5f;
        glPointSize(particle.size + heat * 2.5f);
        glColor4f(0.86f + approach * 0.14f, 0.22f + heat * 0.48f + approach * 0.2f, 0.04f + approach * 0.52f, particle.brightness);
        glBegin(GL_POINTS); glVertex3f(x, 0.06f + heat * 0.04f, z); glEnd();
    }

    glLineWidth(3.0f);
    for (int arc = 0; arc < 10; ++arc) {
        const float start = rotation + static_cast<float>(arc) * Pi / 5.0f;
        const float radius = 0.48f;
        glColor4f(1.0f, 0.38f + (arc % 2) * 0.25f, 0.05f, 0.8f);
        glBegin(GL_LINE_STRIP);
        for (int point = 0; point < 6; ++point) {
            const float angle = start + point * 0.035f;
            glVertex3f(std::cos(angle) * radius, 0.02f, std::sin(angle) * radius);
        }
        glEnd();
    }

    glDisable(GL_BLEND);
    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0.0f, 0.04f, 0.0f);
    for (int segment = 0; segment <= 64; ++segment) {
        const float angle = static_cast<float>(segment) / 64.0f * 2.0f * Pi;
        glVertex3f(std::cos(angle) * 0.38f, 0.04f, std::sin(angle) * 0.38f);
    }
    glEnd();
}

void drawSimulation3D(float delta) {
    glLoadIdentity();
    glTranslatef(0.0f, -0.1f, -cameraDistance);
    glRotatef(-cameraPitch, 1.0f, 0.0f, 0.0f);
    glRotatef(-cameraYaw, 0.0f, 1.0f, 0.0f);
    drawWarpedSpacetime();
    drawLensRays();
    drawBlackHole3D(delta);
}

void drawSimulation(float width, float height, float delta) {
    const float centerX = width * 0.52f;
    const float centerY = height * 0.51f;
    const float scale = std::min(width, height) * 0.52f;
    const float horizon = 0.105f + simulation.mass * 0.0045f;

    drawSpacetimeGrid(centerX, centerY, width, height);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glPointSize(2.0f);
    glBegin(GL_POINTS);
    for (Particle& particle : simulation.particles) {
        if (!simulation.paused) particle.angle += delta * particle.speed * simulation.timeScale * (1.0f + simulation.spin * 2.0f) / particle.radius;
        const float radius = particle.radius * scale;
        const float bend = 1.0f + horizon / std::max(particle.radius, 0.08f);
        const float x = centerX + std::cos(particle.angle) * radius * bend;
        const float y = centerY + std::sin(particle.angle) * radius * 0.29f;
        const float heat = std::clamp(1.0f - (particle.radius - 0.22f), 0.0f, 1.0f);
        const float approach = std::clamp(0.5f + std::sin(particle.angle) * 0.5f, 0.0f, 1.0f);
        glPointSize(particle.size + heat * 2.0f);
        const float red = 0.82f + approach * 0.18f;
        const float green = 0.28f + heat * 0.42f + approach * 0.16f;
        const float blue = 0.05f + approach * 0.5f;
        glColor4f(red, green, blue, particle.brightness);
        glVertex2f(x, y);
    }
    glEnd();

    for (int ring = 5; ring >= 1; --ring) {
        glLineWidth(static_cast<float>(ring));
        glColor4f(1.0f, 0.20f + ring * 0.08f, 0.03f, 0.16f + ring * 0.12f);
        glBegin(GL_LINE_LOOP);
        for (int segment = 0; segment < 96; ++segment) {
            const float angle = static_cast<float>(segment) / 96.0f * 2.0f * Pi;
            const float radius = horizon * scale * (1.18f + ring * 0.07f);
            glVertex2f(centerX + std::cos(angle) * radius, centerY + std::sin(angle) * radius);
        }
        glEnd();
    }

    glLineWidth(2.0f);
    glColor4f(1.0f, 0.56f, 0.12f, 0.95f);
    glBegin(GL_LINE_LOOP);
    for (int segment = 0; segment < 96; ++segment) {
        const float angle = static_cast<float>(segment) / 96.0f * 2.0f * Pi;
        const float radius = horizon * scale * 1.08f;
        glVertex2f(centerX + std::cos(angle) * radius, centerY + std::sin(angle) * radius);
    }
    glEnd();

    const float rotation = static_cast<float>(glfwGetTime()) * simulation.timeScale * (0.7f + simulation.spin * 2.0f);
    glLineWidth(3.0f);
    for (int arc = 0; arc < 8; ++arc) {
        const float start = rotation + static_cast<float>(arc) * Pi / 4.0f;
        const float radius = horizon * scale * 1.12f;
        glColor4f(1.0f, 0.42f + (arc % 2) * 0.22f, 0.04f, 0.72f);
        glBegin(GL_LINES);
        glVertex2f(centerX + std::cos(start) * radius, centerY + std::sin(start) * radius);
        glVertex2f(centerX + std::cos(start + 0.16f) * radius, centerY + std::sin(start + 0.16f) * radius);
        glEnd();
    }

    glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
    glBegin(GL_TRIANGLE_FAN); glVertex2f(centerX, centerY);
    for (int segment = 0; segment <= 64; ++segment) {
        const float angle = static_cast<float>(segment) / 64.0f * 2.0f * Pi;
        glVertex2f(centerX + std::cos(angle) * horizon * scale, centerY + std::sin(angle) * horizon * scale);
    }
    glEnd();
}

void drawHud(float width, float height) {
    glDisable(GL_BLEND);
    glLineWidth(2.0f);
    glColor3f(0.82f, 0.78f, 0.68f);
    glBegin(GL_LINES);
    glVertex2f(28.0f, height - 28.0f); glVertex2f(250.0f, height - 28.0f);
    glVertex2f(28.0f, height - 36.0f); glVertex2f(150.0f, height - 36.0f);
    glVertex2f(width - 245.0f, height - 28.0f); glVertex2f(width - 28.0f, height - 28.0f);
    glEnd();
    glColor3f(0.95f, 0.35f, 0.12f);
    glBegin(GL_QUADS);
    glVertex2f(28.0f, height - 56.0f); glVertex2f(95.0f, height - 56.0f);
    glVertex2f(95.0f, height - 52.0f); glVertex2f(28.0f, height - 52.0f);
    glEnd();
    glColor3f(0.58f, 0.58f, 0.56f);
    glBegin(GL_LINES);
    for (int mark = 0; mark < 8; ++mark) {
        const float x = 28.0f + mark * 18.0f;
        glVertex2f(x, 30.0f); glVertex2f(x + 9.0f, 30.0f);
    }
    glEnd();
}

void handleKey(GLFWwindow* window, int key, int, int action, int) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;
    if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, GLFW_TRUE);
    if (key == GLFW_KEY_SPACE) simulation.paused = !simulation.paused;
    if (key == GLFW_KEY_G) simulation.showGrid = !simulation.showGrid;
    if (key == GLFW_KEY_R) seedParticles();
    if (key == GLFW_KEY_UP) simulation.mass = std::min(16.0f, simulation.mass + 0.2f);
    if (key == GLFW_KEY_DOWN) simulation.mass = std::max(3.0f, simulation.mass - 0.2f);
    if (key == GLFW_KEY_RIGHT) simulation.spin = std::min(0.98f, simulation.spin + 0.02f);
    if (key == GLFW_KEY_LEFT) simulation.spin = std::max(0.0f, simulation.spin - 0.02f);
}

void handleMouseButton(GLFWwindow* window, int button, int action, int) {
    if (button != GLFW_MOUSE_BUTTON_LEFT) return;
    draggingCamera = action == GLFW_PRESS;
    glfwGetCursorPos(window, &lastMouseX, &lastMouseY);
}

void handleMouseMove(GLFWwindow*, double x, double y) {
    if (!draggingCamera) return;
    cameraYaw += static_cast<float>(x - lastMouseX) * 0.22f;
    cameraPitch = std::clamp(cameraPitch + static_cast<float>(y - lastMouseY) * 0.18f, -8.0f, 72.0f);
    lastMouseX = x;
    lastMouseY = y;
}
}

int main() {
    if (!glfwInit()) { std::cerr << "Could not initialize GLFW\n"; return EXIT_FAILURE; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* videoMode = glfwGetVideoMode(monitor);
    if (monitor == nullptr || videoMode == nullptr) {
        glfwTerminate();
        std::cerr << "Could not find a primary monitor\n";
        return EXIT_FAILURE;
    }
    GLFWwindow* window = glfwCreateWindow(videoMode->width, videoMode->height, "Event Horizon Observatory", monitor, nullptr);
    if (!window) { glfwTerminate(); std::cerr << "Could not create OpenGL window\n"; return EXIT_FAILURE; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetKeyCallback(window, handleKey);
    glfwSetMouseButtonCallback(window, handleMouseButton);
    glfwSetCursorPosCallback(window, handleMouseMove);
    seedParticles();
    lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        const double currentTime = glfwGetTime();
        const float delta = static_cast<float>(std::min(currentTime - lastTime, 0.05));
        lastTime = currentTime;
        glViewport(0, 0, width, height);
        glClearColor(0.008f, 0.01f, 0.018f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        setPerspective(52.0f, static_cast<float>(width) / static_cast<float>(height), 0.05f, 100.0f);
        drawSimulation3D(delta);
        glfwSwapBuffers(window); glfwPollEvents();
        ++frameCount;
    }
    glfwDestroyWindow(window); glfwTerminate(); return EXIT_SUCCESS;
}