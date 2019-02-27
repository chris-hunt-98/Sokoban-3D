#ifndef PUSHBLOCK_H
#define PUSHBLOCK_H

#include "gameobject.h"
#include "common.h"

class PushBlock: public GameObject {
public:
    PushBlock(Point3 pos, unsigned char color, bool pushable, bool gravitable, Sticky sticky);
    virtual ~PushBlock();

    virtual ObjCode obj_code();
    virtual void serialize(MapFileO& file);
    static std::unique_ptr<GameObject> deserialize(MapFileI& file);

    void collect_sticky_links(RoomMap*, Sticky, std::vector<GameObject*>& links);

    virtual void draw(GraphicsManager*, Point3);

    Sticky sticky();

    Sticky sticky_;
};

#endif // PUSHBLOCK_H