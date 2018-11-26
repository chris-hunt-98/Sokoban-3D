#include "roommap.h"

#include "gameobject.h"
#include "delta.h"
#include "block.h"
#include "switch.h"
#include "mapfile.h"

RoomMap::RoomMap(int width, int height): width_ {width}, height_ {height}, layers_ {} {}

bool RoomMap::valid(Point pos) {
    return (0 <= pos.x) && (pos.x < width_) && (0 <= pos.y) && (pos.y < height_);
}

int RoomMap::width() const {
    return width_;
}

int RoomMap::height() const {
    return height_;
}

void RoomMap::push_full() {
    layers_.push_back(std::make_unique<FullMapLayer>(this, width_, height_));
}

void RoomMap::push_sparse() {
    layers_.push_back(std::make_unique<SparseMapLayer>(this));
}

void RoomMap::serialize(MapFileO& file) const {
    // Serialize layer types
    for (auto& layer : layers_) {
        file << layer->type();
    }
    // Serialize raw object data
    std::vector<GameObject*> rel_check;
    file << static_cast<unsigned char>(MapCode::Objects);
    for (auto& layer : layers_) {
        layer->serialize(file, rel_check);
    }
    file << static_cast<unsigned char>(ObjCode::NONE);
    // Serialize relational data
    for (auto& object : rel_check) {
        object->relation_serialize(file);
    }
}

GameObject* RoomMap::view(Point3 pos) {
    return valid(pos.h()) ? layers_[pos.z]->view(pos.h()) : nullptr;
}

void RoomMap::take(Point3 pos, DeltaFrame* delta_frame) {
    layers_[pos.z]->take(pos.h(), delta_frame);
}

std::unique_ptr<GameObject> RoomMap::take_quiet(Point3 pos) {
    return layers_[pos.z]->take_quiet(pos.h());
}

std::unique_ptr<GameObject> RoomMap::take_quiet(GameObject* obj) {
    Point3 pos {obj->pos()};
    return layers_[obj->pos().z]->take_quiet(pos.h());
}

void RoomMap::put(std::unique_ptr<GameObject> obj, DeltaFrame* delta_frame) {
    layers_[obj->pos().z]->put(std::move(obj), delta_frame);
}

void RoomMap::put_quiet(std::unique_ptr<GameObject> obj) {
    layers_[obj->pos().z]->put_quiet(std::move(obj));
}

void RoomMap::draw(GraphicsManager* gfx) {
    for (auto& layer : layers_) {
        layer->draw(gfx);
    }
}

void RoomMap::set_initial_state(bool editor_mode) {
    /*for (int x = 0; x != width_; ++x) {
        for (int y = 0; y != height_; ++y) {
            auto pb = dynamic_cast<PushBlock*>(view(Point{x,y}));
            if (pb) {
                pb->check_add_local_links(this, nullptr);
            }
            if (editor_mode) {
                continue;
            }
            auto gate = dynamic_cast<Gate*>(view(Point{x,y}));
            if (gate) {
                gate->check_waiting(this, nullptr);
            }
        }
    }*/
}
