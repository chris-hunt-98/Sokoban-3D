#include "switch.h"
#include "roommap.h"
#include "delta.h"
#include "graphicsmanager.h"
#include "mapfile.h"

Switchable::Switchable(Point3 pos, bool default_state, bool initial_state):
GameObject(pos),
default_ {default_state},
active_ {(bool)(default_state ^ initial_state)},
waiting_ {(bool)(default_state ^ initial_state)} {}

Switchable::~Switchable() {}

void Switchable::set_aw(bool active, bool waiting) {
    active_ = active;
    waiting_ = waiting;
}

bool Switchable::state() {
    return default_ ^ active_;
}

void Switchable::receive_signal(bool signal, RoomMap* room_map, DeltaFrame* delta_frame) {
    if (active_ ^ waiting_ == signal) {
        return;
    }
    if (delta_frame) {
        delta_frame->push(std::make_unique<SwitchableDelta>(this, active_, waiting_));
    }
    waiting_ = !can_set_state(default_ ^ signal, room_map);
    active_ = waiting_ ^ signal;
}

void Switchable::check_waiting(RoomMap* room_map, DeltaFrame* delta_frame) {
    if (waiting_ && can_set_state(!(default_ ^ active_), room_map)) {
        if (delta_frame) {
            delta_frame->push(std::make_unique<SwitchableDelta>(this, active_, waiting_));
        }
        waiting_ = false;
        active_ = !active_;
    }
}

// Gates should be initialized down in case they are "covered" at load time
Gate::Gate(Point3 pos, bool def): Switchable(pos, def, false) {}

Gate::~Gate() {}

ObjCode Gate::obj_code() {
    return ObjCode::Gate;
}

void Gate::serialize(MapFileO& file) {
    file << default_;
}

GameObject* Gate::deserialize(MapFileI& file) {
    return nullptr;
    //return new Gate(b[0], b[1], (bool)b[2]);
}

bool Gate::can_set_state(bool state, RoomMap* room_map) {
    // You can always set state to false, but setting it to true requires there be
    // nothing above the gate
    return !state || (room_map->view(shifted_pos({0,0,1})) == nullptr);
}

void Gate::draw(GraphicsManager* gfx) {
    Point3 p = pos();
    glm::mat4 model = glm::translate(glm::mat4(), glm::vec3(p.x, .9f * state() - 0.45f, p.y));
    model = glm::scale(model, glm::vec3(0.7f, 1.0f, 0.7f));
    gfx->set_model(model);
    gfx->set_color(COLORS[DARK_PURPLE]);
    gfx->draw_cube();
}

Signaler::Signaler(unsigned char threshold, bool persistent, bool active):
count_ {0}, threshold_ {threshold},
active_ {active}, persistent_ {persistent},
switches_ {}, switchables_ {} {}

Signaler::~Signaler() {}

void Signaler::push_switchable(Switchable* obj) {
    switchables_.push_back(obj);
}

void Signaler::push_switch(Switch* obj) {
    switches_.push_back(obj);
    obj->push_signaler(this);
}

void Signaler::receive_signal(bool signal) {
    if (signal) {
        ++count_;
    } else {
        --count_;
    }
}

void Signaler::toggle() {
    active_ = !active_;
}

void Signaler::check_send_signal(RoomMap* room_map, DeltaFrame* delta_frame) {
    if (!(active_ && persistent_) && ((count_ >= threshold_) != active_)) {
        delta_frame->push(std::make_unique<SignalerToggleDelta>(this));
        active_ = !active_;
        for (Switchable* obj : switchables_) {
            obj->receive_signal(active_, room_map, delta_frame);
        }
    }
}

void Signaler::serialize(MapFileO& file) {
    file << MapCode::Signaler;
    file << threshold_;
    file << (persistent_ | (active_ << 1));
    file << switches_.size();
    file << switchables_.size();
    for (auto& obj : switches_) {
        file << obj->pos();
        file << obj->obj_code();
    }
    for (auto& obj : switchables_) {
        file << obj->pos();
        file << obj->obj_code();
    }
}

Switch::Switch(Point3 pos, bool persistent, bool active): GameObject(pos),
persistent_ {persistent}, active_ {active}, signalers_ {} {}

Switch::~Switch() {}

void Switch::push_signaler(Signaler* signaler) {
    signalers_.push_back(signaler);
}

void Switch::toggle() {
    active_ = !active_;
    for (auto& signaler : signalers_) {
        signaler->receive_signal(active_);
    }
}

PressSwitch::PressSwitch(Point3 pos, unsigned char color, bool persistent, bool active):
Switch(pos, persistent, active), color_ {color} {}

PressSwitch::~PressSwitch() {}

ObjCode PressSwitch::obj_code() {
    return ObjCode::PressSwitch;
}

void PressSwitch::serialize(MapFileO& file) {
    file << color_;
    file << (persistent_ | (active_ << 1));
}

GameObject* PressSwitch::deserialize(MapFileI& file) {
    Point3 pos {file.read_point3()};
    unsigned char b[2];
    file.read(b, 2);
    return new PressSwitch(pos, b[0], b[1] & 1, b[1] & 2);
}

void PressSwitch::check_send_signal(RoomMap* room_map, DeltaFrame* delta_frame, std::unordered_set<Signaler*>& check) {
    if (active_ && persistent_) {
        return;
    }
    if (should_toggle(room_map)) {
        delta_frame->push(std::make_unique<SwitchToggleDelta>(this));
        toggle();
        for (Signaler* signaler : signalers_) {
            check.insert(signaler);
        }
    }
}

bool PressSwitch::should_toggle(RoomMap* room_map) {
    return active_ ^ (room_map->view(shifted_pos({0,0,1})) != nullptr);
}

void PressSwitch::draw(GraphicsManager* gfx) {
    Point3 p = pos();
    glm::mat4 model = glm::translate(glm::mat4(), glm::vec3(p.x, -0.4f, p.y));
    model = glm::scale(model, glm::vec3(0.9f, 1.0f, 0.9f));
    gfx->set_model(model);
    gfx->set_color(COLORS[color_]);
    if (persistent_) {
        if (active_) {
            gfx->set_tex(glm::vec2(2,1));
        } else {
            gfx->set_tex(glm::vec2(1,1));
        }
    } else {
        gfx->set_tex(glm::vec2(0,1));
    }
    gfx->draw_cube();
    gfx->set_tex(glm::vec2(0,0));
}