#include <iostream>
#include <fstream>
#include <cmath>
#include <unordered_map>
#include <unordered_set>
#include <thread>
#include <chrono>

#include "common.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma GCC diagnostic ignored "-Wswitch-default"

#include <dear/imgui.h>
#include <dear/imgui_impl_glfw.h>
#include <dear/imgui_impl_opengl3.h>

#pragma GCC diagnostic pop

#include "room.h"
#include "roommap.h"
#include "graphicsmanager.h"
#include "editorstate.h"

#include "gameobject.h"
#include "block.h"
#include "switch.h"

bool window_init(GLFWwindow*&);

void init_switch_room(RoomMap*);

// Code copied from https://stackoverflow.com/questions/1739259/how-to-use-queryperformancecounter for profiling!
#include <windows.h>

double PCFreq = 0.0;
__int64 CounterStart = 0;

void StartCounter()
{
    LARGE_INTEGER li;
    if(!QueryPerformanceFrequency(&li))
    std::cout << "QueryPerformanceFrequency failed!\n";

    PCFreq = double(li.QuadPart)/1000000.0;

    QueryPerformanceCounter(&li);
    CounterStart = li.QuadPart;
}
double GetCounter()
{
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return double(li.QuadPart-CounterStart)/PCFreq;
}
// End copied code

int main(void) {
    GLFWwindow* window;
    if (!window_init(window)) {
        return -1;
    }

    GraphicsManager gfx(window);

    std::unique_ptr<GameState> current_state = std::make_unique<EditorState>(&gfx);
    current_state->set_csp(&current_state);

    /*
    RoomManager mgr(window, &shader);
    mgr.init_make("default_", DEFAULT_BOARD_WIDTH, DEFAULT_BOARD_HEIGHT);
    Editor editor(window, &mgr);
    mgr.set_editor(&editor); */

    // Hardcoded Object Testing

    /*
    RoomMap* room_map = mgr.room_map();
    init_switch_room(room_map);
    //*/

    // ImGui init

    const char* glsl_version = "#version 330";

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    ImGui::StyleColorsDark();

    // It's convenient to keep the demo code in here,
    // for when we want to explore ImGui features
    bool show_demo_window = true;

    glfwSwapInterval(0);

    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        int display_w, display_h;
        glfwMakeContextCurrent(window);
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.2, 0, 0.3, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        current_state->main_loop();

        if (show_demo_window) {
            ImGui::ShowDemoWindow(&show_demo_window);
        }

        ImGui::Render();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwMakeContextCurrent(window);
        glfwSwapBuffers(window);

        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    glfwTerminate();
    return 0;
}

bool window_init(GLFWwindow*& window) {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, false);

    window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Sokoban 3D", nullptr, nullptr);
    if (!window) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    return true;
}

void init_switch_room(RoomMap* room_map) {
    auto sw = std::make_unique<Switch>(4,4,5,false);
    auto swp = std::make_unique<Switch>(4,6,6,true);
    auto g1 = std::make_unique<Gate>(7,4,false);
    auto g2 = std::make_unique<Gate>(8,4,true);
    auto g3 = std::make_unique<Gate>(7,6,false);
    auto g4 = std::make_unique<Gate>(8,6,true);

    Signaler s1(1, false);
    Signaler s2(1, false);

    sw->push_signaler(&s1);
    swp->push_signaler(&s2);

    s1.push_switchable(g1.get());
    s1.push_switchable(g2.get());
    s2.push_switchable(g3.get());
    s2.push_switchable(g4.get());

    g1->check_waiting(room_map, nullptr);
    g2->check_waiting(room_map, nullptr);
    g3->check_waiting(room_map, nullptr);
    g4->check_waiting(room_map, nullptr);

    room_map->put_quiet(std::move(sw));
    room_map->put_quiet(std::move(swp));
    room_map->put_quiet(std::move(g1));
    room_map->put_quiet(std::move(g2));
    room_map->put_quiet(std::move(g3));
    room_map->put_quiet(std::move(g4));


    auto sw1 = std::make_unique<Switch>(4,8,2,false);
    auto sw2 = std::make_unique<Switch>(5,8,3,false);
    auto sw3 = std::make_unique<Switch>(6,8,4,false);
    auto sw4 = std::make_unique<Switch>(7,8,5,false);
    auto sw5 = std::make_unique<Switch>(8,8,6,true);

    Signaler sa(1, false);
    Signaler sb(5, false);
    Signaler sc(5, true);

    auto gany = std::make_unique<Gate>(10,6,false);
    auto gall = std::make_unique<Gate>(10,8,false);
    auto gallp = std::make_unique<Gate>(10,10,false);

    sw1->push_signaler(&sa);
    sw1->push_signaler(&sb);
    sw1->push_signaler(&sc);

    sw2->push_signaler(&sa);
    sw2->push_signaler(&sb);
    sw2->push_signaler(&sc);

    sw3->push_signaler(&sa);
    sw3->push_signaler(&sb);
    sw3->push_signaler(&sc);

    sw4->push_signaler(&sa);
    sw4->push_signaler(&sb);
    sw4->push_signaler(&sc);

    sw5->push_signaler(&sa);
    sw5->push_signaler(&sb);
    sw5->push_signaler(&sc);

    sa.push_switchable(gany.get());
    sb.push_switchable(gall.get());
    sc.push_switchable(gallp.get());

    gany->check_waiting(room_map, nullptr);
    gall->check_waiting(room_map, nullptr);
    gallp->check_waiting(room_map, nullptr);

    room_map->put_quiet(std::move(sw1));
    room_map->put_quiet(std::move(sw2));
    room_map->put_quiet(std::move(sw3));
    room_map->put_quiet(std::move(sw4));
    room_map->put_quiet(std::move(sw5));
    room_map->put_quiet(std::move(gany));
    room_map->put_quiet(std::move(gall));
    room_map->put_quiet(std::move(gallp));
}
