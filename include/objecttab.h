#ifndef OBJECTTAB_H
#define OBJECTTAB_H

#include "common.h"
#include "editortab.h"

class ObjectTab: public EditorTab {
public:
    ObjectTab(EditorState*, GraphicsManager*);
    virtual ~ObjectTab();
    void main_loop(EditorRoom*);
    void handle_left_click(EditorRoom*, Point3);
    void handle_right_click(EditorRoom*, Point3);
};

#endif // OBJECTTAB_H