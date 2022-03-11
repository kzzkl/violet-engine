#include <string>
#include <string_view>

namespace ash::graphics::d3d12
{
std::wstring string_to_wstring(std::string_view str);
std::string wstring_to_string(std::wstring_view str);
} // namespace ash::graphics::d3d12