#include <filesystem>

namespace osr {

void extract(bool with_platforms,
             std::filesystem::path const& in,
             std::filesystem::path const& out);

void extract(bool with_platforms,
             std::filesystem::path const& in,
             std::filesystem::path const& elev,
             std::filesystem::path const& out);

}  // namespace osr