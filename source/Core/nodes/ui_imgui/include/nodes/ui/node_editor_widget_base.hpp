#pragma once
#define IMGUI_DEFINE_MATH_OPERATORS

#include "GUI/widget.h"
#include "imgui.h"
#include "nodes/core/node_link.hpp"
#include "nodes/core/node_tree.hpp"
#include "nodes/core/socket.hpp"
#include "nodes/system/node_system.hpp"
#include "nodes/ui/imgui.hpp"

USTC_CG_NAMESPACE_OPEN_SCOPE
class NodeEditorWidgetBase : public IWidget {
   public:
    NodeEditorWidgetBase(const NodeWidgetSettings& desc)
        : tree_(desc.system->get_node_tree())
    {
    }

   protected:
    void connectLinks();
    static ImColor GetIconColor(SocketType type);
    void DrawPinIcon(const NodeSocket& pin, bool connected, int alpha);
    NodeTree* tree_;

    static const int m_PinIconSize = 20;

    unsigned _level = 0;
};

USTC_CG_NAMESPACE_CLOSE_SCOPE