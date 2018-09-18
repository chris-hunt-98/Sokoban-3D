#include "common.h"
#include "editor.h"
#include "gameobject.h"
#include "block.h"

Editor::Editor(GLFWwindow* window): window_ {window}, type_ {1}, pos_ {Point{0,0}}, valid_pos_ {false} {}

void Editor::handle_input(DeltaFrame* delta_frame, WorldMap* world_map, Point cam_pos) {
    for (int i = 0; i < 10; ++i) {
        if (glfwGetKey(window_, GLFW_KEY_0 + i) == GLFW_PRESS) {
            type_ = i;
            break;
        }
    }

    if (glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        get_pos(world_map, cam_pos);
        if (!valid_pos_) {
            return;
        }
        create_obj(delta_frame, world_map);
    } else if (glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        get_pos(world_map, cam_pos);
        if (!valid_pos_) {
            return;
        }
        destroy_obj(delta_frame, world_map);
    }
}

void Editor::get_pos(WorldMap* world_map, Point cam_pos) {
    double xpos, ypos;
    glfwGetCursorPos(window_, &xpos, &ypos);
    valid_pos_ = false;
    if (xpos >= 0 && xpos < SCREEN_WIDTH && ypos >= 0 && ypos < SCREEN_HEIGHT) {
        int x = ((int)xpos + MESH_SIZE*cam_pos.x - (SCREEN_WIDTH - MESH_SIZE) / 2) / MESH_SIZE;
        int y = ((int)ypos + MESH_SIZE*cam_pos.y - (SCREEN_HEIGHT - MESH_SIZE) / 2) / MESH_SIZE;
        pos_ = Point{x, y};
        valid_pos_ = world_map->valid(pos_);
    }
}

void Editor::create_obj(DeltaFrame* delta_frame, WorldMap* world_map) {
    if (world_map->view(pos_, Layer::Solid)) {
        return;
    }
    int x = pos_.x;
    int y = pos_.y;
    switch (type_) {
    case 1: world_map->put(std::move(std::make_unique<Wall>(x,y)), delta_frame);
        break;
    case 2: world_map->put(std::move(std::make_unique<PushBlock>(x,y,false,StickyLevel::None)), delta_frame);
        break;
    case 3: world_map->put(std::move(std::make_unique<PushBlock>(x,y,false,StickyLevel::Weak)), delta_frame);
        break;
    case 4: world_map->put(std::move(std::make_unique<PushBlock>(x,y,false,StickyLevel::Strong)), delta_frame);
        break;
    case 5: world_map->put(std::move(std::make_unique<PushBlock>(x,y,true,StickyLevel::None)), delta_frame);
        break;
    case 6: world_map->put(std::move(std::make_unique<PushBlock>(x,y,true,StickyLevel::Weak)), delta_frame);
        break;
    case 7: world_map->put(std::move(std::make_unique<PushBlock>(x,y,true,StickyLevel::Strong)), delta_frame);
        break;
    case 8: world_map->put(std::move(std::make_unique<SnakeBlock>(x,y,false,2)), delta_frame);
        break;
    case 9: world_map->put(std::move(std::make_unique<SnakeBlock>(x,y,false,1)), delta_frame);
        break;
    default:
        break;
    }
}

void Editor::destroy_obj(DeltaFrame* delta_frame, WorldMap* world_map) {
    if (!world_map->view(pos_, Layer::Solid)) {
        return;
    }
    world_map->take(pos_, Layer::Solid, delta_frame);
}