#include "block.h"
#include "graphicsmanager.h"
#include "delta.h"
#include "roommap.h"
#include "moveprocessor.h"
#include "component.h"
#include "mapfile.h"
#include "animation.h"

#include "graphicsmanager.h"


Block::Block(Point3 pos, ColorCycle color, bool car):
GameObject(pos), animation_ {}, comp_ {}, color_ {color}, car_ {car} {}

Block::~Block() {}

void Block::serialize(MapFileO& file) {
    file << color_;
    file << car_;
}

unsigned char Block::color() {
    return color_.color();
}

void Block::insert_color(unsigned char color) {
    color_.insert_color(color);
}

bool Block::cycle_color(bool undo) {
    if (color_.size_ == 1) {
        return false;
    }
    color_.cycle(undo);
    return true;
}

// Most block types don't form strong components
std::unique_ptr<StrongComponent> Block::make_strong_component(RoomMap* room_map) {
    auto unique_comp = std::make_unique<SingletonComponent>(this);
    comp_ = unique_comp.get();
    return std::move(unique_comp);
}

// Some block types have trivial weak components, some don't
// Using sticky(), we can avoid rewriting this method
std::unique_ptr<WeakComponent> Block::make_weak_component(RoomMap* room_map) {
    auto unique_comp = std::make_unique<WeakComponent>();
    comp_ = unique_comp.get();
    if (sticky()) {
        std::vector<Block*> to_check {this};
        while (!to_check.empty()) {
            Block* cur = to_check.back();
            to_check.pop_back();
            unique_comp->add_block(cur);
            for (Point3 d : DIRECTIONS) {
                Block* adj = dynamic_cast<Block*>(room_map->view(cur->shifted_pos(d)));
                if (adj && !adj->comp_ && adj->sticky() && color() == adj->color()) {
                    adj->comp_ = comp_;
                    to_check.push_back(adj);
                }
            }
        }
    } else {
        unique_comp->add_block(this);
    }
    return std::move(unique_comp);
}

StrongComponent* Block::s_comp() {
    return static_cast<StrongComponent*>(comp_);
}

WeakComponent* Block::w_comp() {
    return static_cast<WeakComponent*>(comp_);
}

void Block::reset_comp() {
    comp_ = nullptr;
}

void Block::set_z(int z) {
    pos_.z = z;
}

bool Block::sticky() {
    return false;
}

bool Block::car() {
    return car_;
}

void Block::get_weak_links(RoomMap*, std::vector<Block*>&) {}

bool Block::has_weak_neighbor(RoomMap* room_map) {
    if (!sticky()) {
        return false;
    }
    for (auto d : H_DIRECTIONS) {
        Block* adj = dynamic_cast<Block*>(room_map->view(shifted_pos(d)));
        if (adj && sticky() && color() == adj->color()) {
            return true;
        }
    }
    return false;
}

void Block::reset_animation() {
    animation_.reset(nullptr);
}


void Block::set_linear_animation(Point3 d) {
    animation_ = std::make_unique<LinearAnimation>(d);
}

void Block::update_animation() {
    if (animation_ && animation_->update()) {
        animation_.reset(nullptr);
    }
}

void Block::shift_pos_from_animation() {
    pos_ = animation_->shift_pos(pos_);
}

FPoint3 Block::real_pos() {
    if (animation_) {
        return pos_ + animation_->dpos();
    } else {
        return pos_;
    }
}

void Block::draw(GraphicsManager* gfx) {
    if (car_) {
        FPoint3 p {real_pos()};
        glm::mat4 model = glm::translate(glm::mat4(), glm::vec3(p.x, p.z + 0.5, p.y));
        model = glm::scale(model, glm::vec3(0.7f, 0.1f, 0.7f));
        gfx->set_model(model);
        gfx->set_color(COLORS[LIGHT_GREY]);
        gfx->draw_cube();
    }
}

NonStickBlock::NonStickBlock(Point3 pos, ColorCycle color, bool car): Block(pos, color, car) {}

NonStickBlock::~NonStickBlock() {}

ObjCode NonStickBlock::obj_code() {
    return ObjCode::NonStickBlock;
}

GameObject* NonStickBlock::deserialize(MapFileI& file) {
    Point3 pos {file.read_point3()};
    ColorCycle color {file.read_color_cycle()};
    unsigned char b[1];
    file.read(b,1);
    return new NonStickBlock(pos, color, b[0]);
}

void NonStickBlock::draw(GraphicsManager* gfx) {
    Block::draw(gfx);
    gfx->set_tex(glm::vec2(2,0));
    FPoint3 p {real_pos()};
    gfx->set_model(glm::translate(glm::mat4(), glm::vec3(p.x, p.z, p.y)));
    gfx->set_color(COLORS[color()]);
    gfx->draw_cube();
    gfx->set_tex(glm::vec2(0,0));
}

WeakBlock::WeakBlock(Point3 pos, ColorCycle color, bool car):
Block(pos, color, car) {}

WeakBlock::~WeakBlock() {}

ObjCode WeakBlock::obj_code() {
    return ObjCode::WeakBlock;
}

bool WeakBlock::sticky() {
    return true;
}

void WeakBlock::get_weak_links(RoomMap* room_map, std::vector<Block*>& links) {
    for (Point3 d : DIRECTIONS) {
        Block* adj = dynamic_cast<Block*>(room_map->view(shifted_pos(d)));
        if (adj && adj->sticky() && color() == adj->color()) {
            links.push_back(adj);
        }
    }
}

GameObject* WeakBlock::deserialize(MapFileI& file) {
    Point3 pos {file.read_point3()};
    ColorCycle color {file.read_color_cycle()};
    unsigned char b[1];
    file.read(b,1);
    return new WeakBlock(pos, color, b[0]);
}

void WeakBlock::draw(GraphicsManager* gfx) {
    Block::draw(gfx);
    gfx->set_tex(glm::vec2(1,0));
    FPoint3 p {real_pos()};
    gfx->set_model(glm::translate(glm::mat4(), glm::vec3(p.x, p.z, p.y)));
    gfx->set_color(COLORS[color()]);
    gfx->draw_cube();
    gfx->set_tex(glm::vec2(0,0));
}


StickyBlock::StickyBlock(Point3 pos, ColorCycle color, bool car):
Block(pos, color, car) {}

StickyBlock::~StickyBlock() {}

ObjCode StickyBlock::obj_code() {
    return ObjCode::StickyBlock;
}

std::unique_ptr<StrongComponent> StickyBlock::make_strong_component(RoomMap* room_map) {
    auto unique_comp = std::make_unique<ComplexComponent>();
    comp_ = unique_comp.get();
    std::vector<StickyBlock*> to_check {this};
    while (!to_check.empty()) {
        StickyBlock* cur = to_check.back();
        to_check.pop_back();
        unique_comp->add_block(cur);
        for (Point3 d : DIRECTIONS) {
            StickyBlock* adj = dynamic_cast<StickyBlock*>(room_map->view(cur->shifted_pos(d)));
            if (adj && !adj->comp_ && color() == adj->color()) {
                adj->comp_ = comp_;
                to_check.push_back(adj);
            }
        }
    }
    return std::move(unique_comp);
}

void StickyBlock::get_weak_links(RoomMap* room_map, std::vector<Block*>& links) {
    for (Point3 d : DIRECTIONS) {
        // Two StickyBlocks can never have a weak link!
        WeakBlock* adj = dynamic_cast<WeakBlock*>(room_map->view(shifted_pos(d)));
        if (adj && color() == adj->color()) {
            links.push_back(adj);
        }
    }
}

bool StickyBlock::sticky() {
    return true;
}

GameObject* StickyBlock::deserialize(MapFileI& file) {
    Point3 pos {file.read_point3()};
    ColorCycle color {file.read_color_cycle()};
    unsigned char b[1];
    file.read(b,1);
    return new StickyBlock(pos, color, b[0]);
}

void StickyBlock::draw(GraphicsManager* gfx) {
    Block::draw(gfx);
    FPoint3 p {real_pos()};
    gfx->set_model(glm::translate(glm::mat4(), glm::vec3(p.x, p.z, p.y)));
    gfx->set_color(COLORS[color()]);
    gfx->draw_cube();
}
