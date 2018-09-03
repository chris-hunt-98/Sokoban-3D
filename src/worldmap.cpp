#include "worldmap.h"
#include <iostream>
#include <unordered_map>

const glm::vec4 GREEN = glm::vec4(0.6f, 0.9f, 0.7f, 1.0f);
const glm::vec4 PINK = glm::vec4(0.9f, 0.6f, 0.7f, 1.0f);
const glm::vec4 BLACK = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
const glm::vec4 ORANGE = glm::vec4(1.0f, 0.7f, 0.3f, 1.0f);

// This is just a temporary thing to throw in the drawing function.
// Obviously, must be fixed when a more dynamic camera is implemented.

bool operator==(const Point& a, const Point& b)
{
    return a.x == b.x && a.y == b.y;
}

unsigned int GameObject::GLOBAL_ID_COUNT = 0;

unsigned int GameObject::gen_id() {
    return GLOBAL_ID_COUNT++;
}

GameObject::GameObject(int x, int y): id_ {GameObject::gen_id()}, pos_ {x, y}, sticky_ {false}, weak_sticky_ {false}, sticky_links_ {}, weak_sticky_links_ {} {}

GameObject::~GameObject() {}

unsigned int GameObject::id() const {
    return id_;
}

Point GameObject::pos() const {
    return pos_;
}

Layer GameObject::layer() const {
    return Layer::Solid;
}

void GameObject::shift_pos(Point d, DeltaFrame* delta_frame) {
    pos_.x += d.x;
    pos_.y += d.y;
    if (delta_frame) {
        delta_frame->push(std::make_unique<MotionDelta>(*this, d));
    }
}

bool GameObject::sticky() {
    return sticky_;
}

bool GameObject::weak_sticky() {
    return weak_sticky_;
}

// By default, GameObjects don't have any links
// But we still need to be able to "check for them"!
PosIdMap const& GameObject::get_strong_links(){
    return sticky_links_;
}

void GameObject::insert_strong_link(Point p, unsigned int id) {
    sticky_links_.insert(std::make_pair(p, id));
}

PosIdMap const& GameObject::get_weak_links(){
    return weak_sticky_links_;
}

void GameObject::insert_weak_link(Point p, unsigned int id) {
    weak_sticky_links_.insert(std::make_pair(p, id));
}

Car::Car(int x, int y): GameObject(x, y) {}

Car::~Car() {}

bool Car::pushable() const { return true; }

Layer Car::layer() const { return Layer::Solid; }

void Car::draw(Shader* shader) {
    Point p = pos();
    glm::mat4 model = glm::translate(glm::mat4(), glm::vec3(p.x - BOARD_SIZE/2, 0.5f, p.y - BOARD_SIZE/2));
    shader->setMat4("model", model);
    shader->setVec4("color", PINK);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
}


Block::Block(int x, int y): GameObject(x, y) {}

Block::~Block() {}

bool Block::pushable() const { return true; }

Layer Block::layer() const { return Layer::Solid; }

void Block::draw(Shader* shader) {
    Point p = pos();
    glm::mat4 model = glm::translate(glm::mat4(), glm::vec3(p.x - BOARD_SIZE/2, 0.5f, p.y - BOARD_SIZE/2));
    shader->setMat4("model", model);
    shader->setVec4("color", GREEN);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
}

StickyBlock::StickyBlock(int x, int y): Block(x, y) {
    sticky_ = true;
}

void StickyBlock::draw(Shader* shader) {
    Point p = pos();
    glm::mat4 model = glm::translate(glm::mat4(), glm::vec3(p.x - BOARD_SIZE/2, 0.5f, p.y - BOARD_SIZE/2));
    shader->setMat4("model", model);
    shader->setVec4("color", ORANGE);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
}


Wall::Wall(int x, int y): GameObject(x, y) {}

Wall::~Wall() {}

bool Wall::pushable() const { return false; }

Layer Wall::layer() const { return Layer::Solid; }

void Wall::draw(Shader* shader) {
    Point p = pos();
    glm::mat4 model = glm::translate(glm::mat4(), glm::vec3(p.x - BOARD_SIZE/2, 0.5f, p.y - BOARD_SIZE/2));
    shader->setMat4("model", model);
    shader->setVec4("color", BLACK);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
}

UndoStack::UndoStack(unsigned int max_depth): max_depth_ {max_depth}, size_ {0}, frames_ {} {}

void UndoStack::push(std::unique_ptr<DeltaFrame> delta_frame) {
    if (!(*delta_frame).trivial()) {
        if (size_ == max_depth_) {
            frames_.pop_front();
        } else {
            ++size_;
        }
        frames_.push_back(std::move(delta_frame));
    }
}

void UndoStack::pop(WorldMap* world_map) {
    if (size_ > 0) {
        (*frames_.back()).revert(world_map);
        frames_.pop_back();
        --size_;
    }
}

DeltaFrame::DeltaFrame(): deltas_ {} {}

void DeltaFrame::revert(WorldMap* world_map) {
    for (auto& delta : deltas_) {
        (*delta).revert(world_map);
    }
}

void DeltaFrame::push(std::unique_ptr<Delta> delta) {
    deltas_.push_back(std::move(delta));
}

bool DeltaFrame::trivial() {
    return deltas_.empty();
}

DeletionDelta::DeletionDelta(std::unique_ptr<GameObject> object): object_ {std::move(object)} {}

void DeletionDelta::revert(WorldMap* world_map) {
    world_map->put_quiet(std::move(object_));
}

CreationDelta::CreationDelta(GameObject const& object): pos_ {object.pos()}, layer_ {object.layer()}, id_ {object.id()} {}

void CreationDelta::revert(WorldMap* world_map) {
    world_map->take_quiet(pos_, layer_, id_);
}

MotionDelta::MotionDelta(GameObject const& object, Point d): pos_ {object.pos()}, layer_ {object.layer()}, id_ {object.id()}, d_ {d} {}

void MotionDelta::revert(WorldMap* world_map) {
    auto object = world_map->take_quiet(pos_, layer_, id_);
    // We don't want to create any deltas while undoing a delta (yet, at least)
    object->shift_pos(Point{-d_.x, -d_.y}, nullptr);
    world_map->put_quiet(std::move(object));
}

MapCell::MapCell(): layers_(std::array<std::vector<std::unique_ptr<GameObject>>, static_cast<unsigned int>(Layer::COUNT)>()) {}

// nullptr means this Layer of this Cell was empty (this may happen often)
// Note: this method only really "makes sense" (has non-arbitrary behavior) on Single Object Layers
GameObject * MapCell::view(Layer layer) {
    if (layers_[static_cast<unsigned int>(layer)].empty()) {
        return nullptr;
    } else {
        return layers_[static_cast<unsigned int>(layer)].front().get();
    }
}

// If we reach the throw, the object with the given id wasn't where we thought it was
// (and this is probably a sign of a bug, so we'll throw an exception for now)
GameObject * MapCell::view_id(Layer layer, unsigned int id) {
    for (auto const& object : layers_[static_cast<unsigned int>(layer)]) {
        if (object.get()->id() == id) {
            return object.get();
        }
    }
    throw "Object not found in call to view_id!";
}

void MapCell::take(Layer layer, unsigned int id, DeltaFrame* delta_frame) {
    auto &vec = layers_[static_cast<unsigned int>(layer)];
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        if ((*it).get()->id() == id) {
            delta_frame->push(static_cast<std::unique_ptr<Delta>>(std::make_unique<DeletionDelta>(std::move(*it))));
            vec.erase(it);
            return;
        }
    }
    throw "Object not found in call to take!";
}

std::unique_ptr<GameObject> MapCell::take_quiet(Layer layer, unsigned int id) {
    auto &vec = layers_[static_cast<unsigned int>(layer)];
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        if ((*it).get()->id() == id) {
            auto obj = std::move(*it);
            vec.erase(it);
            return obj;
        }
    }
    throw "Object not found in call to take_quiet!";
}

void MapCell::put(std::unique_ptr<GameObject> object, DeltaFrame* delta_frame) {
    delta_frame->push(std::make_unique<CreationDelta>(*object));
    layers_[static_cast<unsigned int>(object->layer())].push_back(std::move(object));
}

void MapCell::put_quiet(std::unique_ptr<GameObject> object) {
    layers_[static_cast<unsigned int>(object->layer())].push_back(std::move(object));
}

void MapCell::draw(Shader* shader) {
    for (auto& layer : layers_) {
        for (auto& object : layer) {
            object->draw(shader);
        }
    }
}

WorldMap::WorldMap(int width, int height): width_ {width}, height_ {height}, map_ {} {
    for (int i = 0; i != width; ++i) {
        map_.push_back({});
        for (int j = 0; j != height; ++j) {
            map_[i].push_back(MapCell());
        }
    }
}

bool WorldMap::valid(Point pos) {
    return (0 <= pos.x) && (pos.x < width_) && (0 <= pos.y) && (pos.y < height_);
}

GameObject* WorldMap::view(Point pos, Layer layer) {
    if (valid(pos)) {
        return map_[pos.x][pos.y].view(layer);
    } else {
        // This is acceptable: we should already be considering the case
        // that there wasn't an object at the location we checked.
        return nullptr;
    }
}

GameObject* WorldMap::view_id(Point pos, Layer layer, unsigned int id) {
    if (valid(pos)) {
        return map_[pos.x][pos.y].view_id(layer, id);
    } else {
        throw "Tried to view a (specific!!) object from an invalid location!";
    }
}

void WorldMap::take(Point pos, Layer layer, unsigned int id, DeltaFrame* delta_frame) {
    if (valid(pos)) {
        map_[pos.x][pos.y].take(layer, id, delta_frame);
    } else {
        throw "Tried to take an object from an invalid location!";
    }
}

std::unique_ptr<GameObject> WorldMap::take_quiet(Point pos, Layer layer, unsigned int id) {
    if (valid(pos)) {
        return map_[pos.x][pos.y].take_quiet(layer, id);
    } else {
        throw "Tried to (quietly) take an object from an invalid location!";
    }
}

void WorldMap::put(std::unique_ptr<GameObject> object, DeltaFrame* delta_frame) {
    auto pos = object->pos();
    if (valid(pos)) {
        map_[pos.x][pos.y].put(std::move(object), delta_frame);
    } else {
        throw "Tried to put an object in an invalid location!";
    }
}

void WorldMap::put_quiet(std::unique_ptr<GameObject> object) {
    auto pos = object->pos();
    if (valid(pos)) {
        map_[pos.x][pos.y].put_quiet(std::move(object));
    } else {
        throw "Tried to (quietly) put an object in an invalid location!";
    }
}

// Note: many things are predicated on the assumption that, in any consistent game state,
// certain layers allow at most one object per MapCell.  These include Solid and Player.
void WorldMap::try_move(PosIdMap& to_move, Point dir, DeltaFrame* delta_frame) {
    PosIdVec to_check;
    for (auto pos_id : to_move) {
        to_check.push_back(pos_id);
    }
    Point cur_pos, new_pos;
    unsigned int id;
    while (!to_check.empty()) {
        std::tie(cur_pos, id) = to_check.back();
        to_check.pop_back();
        // First, check whether the thing you'll be displacing can move
        new_pos = Point {cur_pos.x + dir.x, cur_pos.y + dir.y};
        if (!valid(new_pos)) {
            return;
        }
        GameObject* new_obj = view(new_pos, Layer::Solid);
        if (new_obj) {
            if (new_obj->pushable()) {
                if (to_move.count(new_obj->pos()) == 0) {
                    auto pos_id = std::make_pair(new_obj->pos(), new_obj->id());
                    to_move.insert(pos_id);
                    to_check.push_back(pos_id);
                }
            } else {
                return;
            }
        }
        // Now check strong links (like stickiness)
        GameObject* cur_obj = view(cur_pos, Layer::Solid);
        for (auto p : cur_obj->get_strong_links()) {
            new_pos = Point {cur_pos.x + p.first.x, cur_pos.y + p.first.y};
            if (to_move.count(new_pos) == 0) {
                auto pos_id = std::make_pair(new_pos, p.second);
                to_move.insert(pos_id);
                to_check.push_back(pos_id);
            }
        }
    }
    for (auto pos_id : to_move) {
        std::tie(cur_pos, id) = pos_id;
        // Hardcode layer for now - it might be more complicated later.
        auto object = take_quiet(cur_pos, Layer::Solid, id);
        object->shift_pos(dir, delta_frame);
        put_quiet(std::move(object));
    }
}

void WorldMap::draw(Shader* shader) {
    for (int i = 0; i < width_; ++i) {
        for (int j = 0; j < height_; ++j) {
            map_[i][j].draw(shader);
        }
    }
}

// Note: this should maybe use a dynamic_cast, but let's wait until there are more objects.
void WorldMap::init_sticky() {
    for (int i = 0; i < width_; ++i) {
        for (int j = 0; j < height_; ++j) {
            GameObject* obj = map_[i][j].view(Layer::Solid);
            if (obj && obj->sticky()) {
                for (int di : {1,-1}) {
                    GameObject* adj_obj = view(Point{i+di,j}, Layer::Solid);
                    if (adj_obj && adj_obj->sticky()) {
                        obj->insert_strong_link(Point {di,0}, adj_obj->id());
                        adj_obj->insert_strong_link(Point {-di,0}, obj->id());
                    }
                }
                for (int dj : {1,-1}) {
                    GameObject* adj_obj = view(Point{i,j+dj}, Layer::Solid);
                    if (adj_obj && adj_obj->sticky()) {
                        obj->insert_strong_link(Point {0,dj}, adj_obj->id());
                        adj_obj->insert_strong_link(Point {0,-dj}, obj->id());
                    }
                }
            }
        }
    }
}
