#include "components/ui_root.hpp"

namespace violet
{
ui_root::ui_root()
{
    m_container = std::make_unique<widget>();
}
} // namespace violet