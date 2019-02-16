#ifndef MAPLAYER_H
#define MAPLAYER_H

#include "common.h"

class GameObject;
class DeltaFrame;
class RoomMap;
class MapFileO;
class GraphicsManager;


class MapLayerIterator {
public:
    virtual ~MapLayerIterator() = 0;

    virtual void advance() = 0;
    virtual bool done() = 0;

    virtual Point3 pos() = 0;
    virtual int id() = 0;

protected:
    MapLayerIterator() = default;
};

class FullMapLayerIterator: public MapLayerIterator {
public:
    FullMapLayerIterator(std::vector<std::vector<int>>& map, int z, int width, int height);
    ~FullMapLayerIterator();

    void advance();
    bool done();

    Point3 pos();
    int id();

private:
    std::vector<std::vector<int>>& map_;
    Point3 pos_;
    int width_;
    int height_;
};

class SparseMapLayerIterator: public MapLayerIterator {
public:
    SparseMapLayerIterator(std::unordered_map<Point, int, PointHash>& map, int z);
    ~SparseMapLayerIterator();

    void advance();
    bool done();

    Point3 pos();
    int id();

private:
    std::unordered_map<Point, int, PointHash>::iterator iter_;
    std::unordered_map<Point, int, PointHash>::iterator end_;
    Point3 pos_;
    int id_;

    void update_values();
};


class MapLayer {
public:
    MapLayer(RoomMap*, int z);
    virtual ~MapLayer() = 0;

    virtual int& at(Point pos) = 0;

    virtual MapCode type() = 0;
    virtual std::unique_ptr<MapLayerIterator> begin_iter() = 0;

protected:
    RoomMap* parent_map_;
    int z_;
};

class FullMapLayer: public MapLayer {
public:
    FullMapLayer(RoomMap*, int width, int height, int z);
    ~FullMapLayer();

    int& at(Point pos);

    MapCode type();
    std::unique_ptr<MapLayerIterator> begin_iter();

private:
    std::vector<std::vector<int>> map_;
    int width_;
    int height_;
};


class SparseMapLayer: public MapLayer {
public:
    SparseMapLayer(RoomMap*, int z);
    ~SparseMapLayer();

    int& at(Point pos);

    MapCode type();
    std::unique_ptr<MapLayerIterator> begin_iter();

private:
    std::unordered_map<Point, int, PointHash> map_;
};

#endif // MAPLAYER_H
