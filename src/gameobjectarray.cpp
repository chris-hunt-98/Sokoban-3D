#include "gameobjectarray.h"

#include "wall.h"
#include "objectmodifier.h"
#include "animation.h"

GameObjectArray::GameObjectArray(): array_ {} {
    array_.push_back(nullptr);
    array_.push_back(std::make_unique<Wall>());
    array_[1]->id_ = 1;
}

GameObjectArray::~GameObjectArray() {}

void GameObjectArray::push_object(std::unique_ptr<GameObject> obj) {
    obj->id_ = array_.size();
    array_.push_back(std::move(obj));
}

GameObject* GameObjectArray::operator[](int id) {
    return array_[id].get();
}

// TODO: use a minheap of "deleted IDs" to potentially reuse object IDs.
void GameObjectArray::destroy(GameObject* obj) {
    array_[obj->id_].reset(nullptr);
}