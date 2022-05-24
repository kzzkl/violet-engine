#include "ui/layout.hpp"
#include <yoga/Yoga.h>

namespace ash::ui
{
class layout_impl_yoga : public layout_impl
{
public:
    layout_impl_yoga()
    {
        m_config = YGConfigNew();
        YGNodeRef root = YGNodeNewWithConfig(m_config);
        YGNodeStyleSetFlexDirection(root, YGFlexDirectionRow);
        YGNodeStyleSetPadding(root, YGEdgeAll, 20);
        YGNodeStyleSetMargin(root, YGEdgeAll, 20);

        YGNodeRef image = YGNodeNew();
        YGNodeStyleSetWidth(image, 80);
        YGNodeStyleSetHeight(image, 80);
        YGNodeStyleSetAlignSelf(image, YGAlignCenter);
        YGNodeStyleSetMargin(image, YGEdgeEnd, 20);

        YGNodeRef text = YGNodeNew();
        YGNodeStyleSetHeight(text, 25);
        YGNodeStyleSetAlignSelf(text, YGAlignCenter);
        YGNodeStyleSetFlexGrow(text, 1);

        YGNodeInsertChild(root, image, 0);
        YGNodeInsertChild(root, text, 1);

        YGNodeCalculateLayout(root, 1300, 800, YGDirectionLTR);

        int a;
    }

private:
    YGConfigRef m_config;
};

layout::layout()
{
    m_impl = std::make_unique<layout_impl_yoga>();
}
} // namespace ash::ui