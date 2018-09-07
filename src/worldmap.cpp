#include "worldmap.h"
#include <iostream>
#include <unordered_map>

#include "gameobject.h"
#include "delta.h"


WorldMap::WorldMap(int width, int height): width_ {width}, height_ {height}, map_ {},
seen_ {}, not_move_ {}, moved_ {}, link_update_ {}, floor_update_ {}, snakes_ {}, pushed_snakes_ {} {
    for (int i = 0; i != width; ++i) {
        map_.push_back({});
        for (int j = 0; j != height; ++j) {
            map_[i].push_back(std::move(MapCell()));
        }
    }
}

bool WorldMap::valid(Point pos) {
    return (0 <= pos.x) && (pos.x < width_) && (0 <= pos.y) && (pos.y < height_);
}

GameObject* WorldMap::view(Point pos, Layer layer) {
    if (valid(pos)) {
        auto &vec = map_[pos.x][pos.y][static_cast<unsigned int>(layer)];
        if (!vec.empty()) {
            return vec.back().get();
        }
    }
    return nullptr;
}

void WorldMap::take(Point pos, Layer layer, DeltaFrame* delta_frame) {
    if (valid(pos)) {
        auto &vec = map_[pos.x][pos.y][static_cast<unsigned int>(layer)];
        if (!vec.empty()) {
            delta_frame->push(static_cast<std::unique_ptr<Delta>>(std::make_unique<DeletionDelta>(std::move(vec.back()))));
            vec.pop_back();
        }
    }
}

void WorldMap::take_id(Point pos, Layer layer, GameObject* id, DeltaFrame* delta_frame) {
    if (valid(pos)) {
        auto &vec = map_[pos.x][pos.y][static_cast<unsigned int>(layer)];
        for (auto&& it = vec.begin(); it != vec.end(); ++it) {
            if ((*it).get() == id) {
                delta_frame->push(static_cast<std::unique_ptr<Delta>>(std::make_unique<DeletionDelta>(std::move(*it))));
                vec.erase(it);
                return;
            }
        }
        throw std::runtime_error("Tried to take an object that wasn't there!");
    }
    throw std::runtime_error("Tried to take an object from an invalid location!");
}

std::unique_ptr<GameObject> WorldMap::take_quiet(Point pos, Layer layer) {
    if (valid(pos)) {
        auto &vec = map_[pos.x][pos.y][static_cast<unsigned int>(layer)];
        if (!vec.empty()) {
            auto obj = std::move(vec.back());
            vec.pop_back();
            return obj;
        }
    }
    return nullptr;
}

std::unique_ptr<GameObject> WorldMap::take_quiet_id(Point pos, Layer layer, GameObject* id) {
    if (valid(pos)) {
        auto &vec = map_[pos.x][pos.y][static_cast<unsigned int>(layer)];
        for (auto&& it = vec.begin(); it != vec.end(); ++it) {
            if ((*it).get() == id) {
                auto obj = std::move(*it);
                vec.erase(it);
                return obj;
            }
        }
        throw std::runtime_error("Tried to (quietly) take an object that wasn't there!");
    }
    throw std::runtime_error("Tried to (quietly) take an object from an invalid location!");
}

void WorldMap::put(std::unique_ptr<GameObject> object, DeltaFrame* delta_frame) {
    Point pos = object->pos();
    if (valid(pos)) {
        delta_frame->push(std::make_unique<CreationDelta>(object.get()));
        map_[pos.x][pos.y][static_cast<unsigned int>(object->layer())].push_back(std::move(object));
    } else {
        throw std::runtime_error("Tried to put an object in an invalid location!");
    }
}

void WorldMap::put_quiet(std::unique_ptr<GameObject> object) {
    Point pos = object->pos();
    if (valid(pos)) {
        map_[pos.x][pos.y][static_cast<unsigned int>(object->layer())].push_back(std::move(object));
    } else {
        throw std::runtime_error("Tried to (quietly) put an object in an invalid location!");
    }
}

void WorldMap::draw(Shader* shader) {
    for (auto& column : map_) {
        for (auto& cell : column) {
            for (auto& layer : cell) {
                for (auto& object : layer) {
                    object->draw(shader);
                }
            }
        }
    }
}


// Note: many things are predicated on the assumption that, in any consistent game state,
// certain layers allow at most one object per MapCell.  These include Solid and Player.
void WorldMap::move_solid(Point player_pos, Point dir, DeltaFrame* delta_frame) {
    reset_state();
    PosIdMap result {};
    if (!move_strong_component(result, player_pos, dir))
        return;
    pull_snakes(delta_frame);
    for (auto& pos_id : result) {
        Point pos;
        GameObject* obj;
        std::tie(pos, obj) = pos_id;
        auto obj_unique = take_quiet_id(pos, Layer::Solid, obj);
        static_cast<Block*>(obj)->shift_pos(dir, delta_frame);
        put_quiet(std::move(obj_unique));
        // Record the old and current positions
        floor_update_.insert(pos);
        floor_update_.insert(obj->pos());
        moved_.insert(obj);
    }
    update_snakes(delta_frame);
    update_links(delta_frame);
}

/** Part of the movement algorithm
 * Attempts to move the object in cur_move in direction dir
 * with the knowledge that the objects in seen have been seen
 * and the objects in not_move cannot move (even if they are not walls).
 * Returns whether the object in cur_move was able to move.
 */
bool WorldMap::move_strong_component(PosIdMap& result, Point start_point, Point dir) {
    if (not_move_.count(start_point)) {
        return false;
    } else if (seen_.count(start_point) > 0) {
        return true;
    }
    PointVec to_check {start_point};
    PosIdMap cur_group {};
    // First find all strongly linked objects, because they share the same fate.
    // Either they'll all move or none of them will.
    while (!to_check.empty()) {
        Point cur_pos = to_check.back();
        to_check.pop_back();
        // It's possible (likely even, with complex groups) that we've seen this before
        if (cur_group.count(cur_pos)) {
            continue;
        }
        GameObject* cur_obj = view(cur_pos, Layer::Solid);
        auto sb = dynamic_cast<SnakeBlock*>(cur_obj);
        //if (sb)
            //std::cout << "Moving a snake at " << sb->pos().x << "," << sb->pos().y << std::endl;
        cur_group.insert(std::make_pair(cur_pos, cur_obj));
        seen_.insert(cur_pos);
        for (auto& link : static_cast<Block*>(cur_obj)->get_strong_links()) {
            to_check.push_back(link->pos());
        }
    }
    // Now we determine whether they can all move.
    // If any can't, then we immediately return
    bool move_cancelled = false;
    ObjVec weak_connections;
    for (auto& cur : cur_group) {
        Point cur_pos = cur.first;
        // First make sure the object can actually move in direction dir
        Point new_pos {cur_pos.x + dir.x, cur_pos.y + dir.y};
        if (!valid(new_pos) || not_move_.count(new_pos)) {
            // We can't move forward; this whole branch is bad
            move_cancelled = true;
            result = {};
            break;
        }
        // This branch is potentially good; look at weak links
        for (auto& link : static_cast<Block*>(view(cur_pos, Layer::Solid))->get_weak_links()) {
            weak_connections.push_back(link);
        }
        if (seen_.count(new_pos)) {
            // If it was in seen but not in not_move, then it has already been verified ok
            continue;
        }
        GameObject* new_obj = view(new_pos, Layer::Solid);
        if (new_obj) {
            if (new_obj->wall()) {
                move_cancelled = true;
                result = {};
                break;
            }
            // If the thing we pushed was a snake block, we need to pull its neighbors
            auto sb = dynamic_cast<SnakeBlock*>(new_obj);
            if (sb) {
                sb->set_distance(0);
                sb->set_target(sb);
                pushed_snakes_.insert(sb);
                //std::cout << "setting distance to 0 at " << sb->pos().x << "," << sb->pos().y << std::endl;
                for (auto link : sb->links()) {
                    auto link_sb = static_cast<SnakeBlock*>(link);
                    if (!link_sb->target()) {
                        link_sb->set_distance(1);
                        link_sb->set_target(sb);
                        //std::cout << "setting distance to 1 at " << link_sb->pos().x << "," << link_sb->pos().y << std::endl;
                    }
                }
            }
            //std::cout << "Starting a push branch" << std::endl;
            // If this branch is good, insert it into the current result
            // If it's bad, we ONLY return the bad branch
            PosIdMap branch {};
            if (move_strong_component(branch, new_pos, dir)) {
                result.insert(branch.begin(), branch.end());
            } else {
                move_cancelled = true;
                result = branch;
                break;
            }
        }
    }
    if (move_cancelled) {
        for (auto& pos_id : cur_group) {
            result.insert(pos_id);
            not_move_.insert(pos_id.first);
        }
        return false;
    } else {
        result.insert(cur_group.begin(), cur_group.end());
        for (auto& link : weak_connections) {
            PosIdMap branch {};
            //std::cout << "Starting a weak branch" << std::endl;
            Point link_pos = link->pos();
            // If the block is a snake block with no pull target, we ignore it for now
            auto sb = dynamic_cast<SnakeBlock*>(link);
            if (sb && !(sb->target())) {
                //std::cout << "Ignoring a snake at " << sb->pos().x << "," << sb->pos().y << std::endl;
                continue;
            }
            if (move_strong_component(branch, link_pos, dir)) {
                result.insert(branch.begin(), branch.end());
            } else {
                // Failed weak links will need to be checked for updates post-move
                link_update_.insert(link);
            }
        }
        return true;
    }
}

void WorldMap::reset_state() {
    seen_ = {};
    not_move_ = {};
    moved_ = {};
    link_update_ = {};
    floor_update_ = {};
    pushed_snakes_ = {};
    for (auto sb : snakes_) {
        sb->set_distance(0);
        sb->set_target(nullptr);
    }
}

void WorldMap::set_initial_state() {
    for (int x = 0; x != width_; ++x) {
        for (int y = 0; y != height_; ++y) {
            auto obj = view(Point{x,y}, Layer::Solid);
            update_links_auxiliary(obj, false, nullptr);
            auto sb = dynamic_cast<SnakeBlock*>(obj);
            if (sb)
                snakes_.insert(sb);
        }
    }
    update_snakes(nullptr);
}

void WorldMap::update_snakes(DeltaFrame* delta_frame) {
    std::unordered_set<SnakeBlock*> available_snakes {};
    std::unordered_set<SnakeBlock*> updated_snakes {};
    for (auto sb : snakes_) {
        if (sb->links().size() == sb->ends()) {
            continue;
        }
        Point pos = sb->pos();
        ObjSet potential_links {};
        for (Point d : {Point{1,0}, Point{-1,0}, Point{0,1}, Point{0,-1}}) {
            auto adj = dynamic_cast<SnakeBlock*>(view(Point{pos.x + d.x, pos.y + d.y}, Layer::Solid));
            if (adj)
                potential_links.insert(adj);
        }
        if (potential_links.size() <= sb->ends()) {
            available_snakes.insert(sb);
        }
    }
    for (auto sb : available_snakes) {
        Point pos = sb->pos();
        ObjSet new_links {};
        for (Point d : {Point{1,0}, Point{-1,0}, Point{0,1}, Point{0,-1}}) {
            auto adj = dynamic_cast<SnakeBlock*>(view(Point{pos.x + d.x, pos.y + d.y}, Layer::Solid));
            if (available_snakes.count(adj))
                new_links.insert(adj);
        }
        ObjSet links = sb->links();
        links.insert(new_links.begin(), new_links.end());
        sb->set_links(links, delta_frame);
    }
}

void WorldMap::update_links(DeltaFrame* delta_frame) {
    for (auto& obj : moved_) {
        update_links_auxiliary(obj, true, delta_frame);
    }
    for (auto& obj : link_update_) {
        update_links_auxiliary(obj, false, delta_frame);
    }
}

void WorldMap::update_links_auxiliary(GameObject* obj, bool save_adj, DeltaFrame* delta_frame) {
    auto pb = dynamic_cast<PushBlock*>(obj);
    if (pb && pb->sticky() != StickyLevel::None) {
        Point p = obj->pos();
        ObjSet new_links {};
        //std::cout << "Updating links at " << p.x << "," << p.y << std::endl;
        for (Point d : {Point{1,0}, Point{-1,0}, Point{0,1}, Point{0,-1}}) {
            PushBlock* adj = dynamic_cast<PushBlock*>(view(Point {p.x + d.x, p.y + d.y}, Layer::Solid));
            if (adj && pb->sticky() == adj->sticky()) {
                new_links.insert(adj);
                if (save_adj && !moved_.count(adj))
                    link_update_.insert(adj);
            }
        }
        if (pb->links() != new_links) {
            pb->set_links(new_links, delta_frame);
        }
    }
}

void WorldMap::pull_snakes(DeltaFrame* delta_frame) {
    for (auto pushed : pushed_snakes_) {
        std::cout << "Snake at " << pushed->pos().x << "," << pushed->pos().y << " was pushed" << std::endl;
        for (auto obj : pushed->links()) {
            SnakeBlock* cur = static_cast<SnakeBlock*>(obj);
            if (not_move_.count(cur->pos())) {
                pushed->remove_link(cur);
                std::cout << "Snake at " << obj->pos().x << "," << obj->pos().y << " didn't move" << std::endl;
                continue;
            }
            if (!cur->distance()) {
                continue;
            }
            SnakeBlock* prev = pushed;
            int d = 1;
            while (true) {
                std::cout << "d is " << d << std::endl;
                std::cout << "Snake at " << cur->pos().x << "," << cur->pos().y << " is the current pulled snake" << std::endl;
                cur->set_target(prev);
                // If we reach the end of the snake, we can pull it
                if (cur->links().size() == 1) {
                    std::cout << "Reached end of snake at " << cur->pos().x << "," << cur->pos().y << "!" << std::endl;
                    pull_snakes_auxiliary(cur, delta_frame);
                    break;
                }
                // Progress down the snake
                for (auto link : cur->links()) {
                    if (link != prev) {
                        cur->set_distance(d++);
                        prev = cur;
                        cur = static_cast<SnakeBlock*>(link);
                        std::cout << "Snake at " << cur->pos().x << "," << cur->pos().y << " has distance " << cur->distance() << " and is pointing to a snake at " << prev->pos().x << "," << prev->pos().y << std::endl;
                        break;
                    }
                }
                // If we reach a block with a target and a shorter distance, we're done
                if (cur->target() && d >= cur->distance()) {
                    std::cout << "Snake at " << cur->pos().x << "," << cur->pos().y << " has target at " << cur->target()->pos().x << "," << cur->target()->pos().y << std::endl;
                    // The chain was so short that it didn't break (it was all pushed)!
                    if (cur->distance() <= 1) {
                        std::cout << "Short chain didn't break!!" << std::endl;
                        break;
                    }
                    // The chain was odd length; split the middle block!
                    else if (d == cur->distance()) {
                        std::cout << "Snake at " << cur->pos().x << "," << cur->pos().y << " split!!!" << std::endl;
                        Point pos = cur->pos();
                        snakes_.erase(cur);
                        for (auto link : cur->links()) {
                            cur->remove_link(static_cast<Block*>(link));
                        }
                        take_id(pos, Layer::Solid, cur, delta_frame);
                        auto half_a = std::make_unique<SnakeBlock>(pos.x, pos.y, 1);
                        auto a_ptr = half_a.get();
                        put(std::move(half_a), delta_frame);
                        a_ptr->set_target(prev);
                        a_ptr->insert_link(prev);
                        snakes_.insert(a_ptr);;
                        pull_snakes_auxiliary(a_ptr, delta_frame);
                        auto half_b = std::make_unique<SnakeBlock>(pos.x, pos.y, 1);
                        auto b_ptr = half_b.get();
                        put(std::move(half_b), delta_frame);
                        b_ptr->set_target(cur->target());
                        b_ptr->insert_link(cur->target());
                        snakes_.insert(b_ptr);
                        pull_snakes_auxiliary(b_ptr, delta_frame);
                        //delta_frame->push(std::make_unique<SnakeSplitDelta>(cur, a_ptr, b_ptr));
                    }
                    // The chain was even length; cut!
                    else {
                        std::cout << "Snake at " << cur->pos().x << "," << cur->pos().y << " got cut!!!" << std::endl;
                        cur->set_links({cur->target()}, delta_frame);
                        pull_snakes_auxiliary(cur, delta_frame);
                        prev->set_links({prev->target()}, delta_frame);
                        pull_snakes_auxiliary(prev, delta_frame);
                    }
                    break;
                }
            }
        }
    }
}

void WorldMap::pull_snakes_auxiliary(SnakeBlock* cur, DeltaFrame* delta_frame) {
    SnakeBlock* next = cur->target();
    std::cout << "Snake at " << cur->pos().x << "," << cur->pos().y << " getting pulled!" << std::endl;
    while (next->distance()) {
        auto obj_unique = take_quiet_id(cur->pos(), Layer::Solid, cur);
        cur->set_pos(next->pos(), delta_frame);
        put_quiet(std::move(obj_unique));
        cur = next;
        std::cout << "Now pulling the one at " << cur->pos().x << "," << cur->pos().y << std::endl;
        next = cur->target();
    }
}
