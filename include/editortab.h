#ifndef EDITORTAB_H
#define EDITORTAB_H

#include "common.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#include <dear/imgui.h>
#pragma GCC diagnostic pop

struct EditorRoom;
class EditorState;
class GraphicsManager;

class EditorTab {
public:
    EditorTab(EditorState*, GraphicsManager*);
    virtual ~EditorTab();
    virtual void init();
    virtual void main_loop(EditorRoom*) = 0;
    virtual void handle_left_click(EditorRoom*, Point3);
    virtual void handle_right_click(EditorRoom*, Point3);

protected:
    EditorState* editor_;
    GraphicsManager* gfx_;
};

ImVec4 unpack_color(glm::vec4 v);

#endif // EDITORTAB_H
