// ---------------------------------------------------------------------------
// IMLAB
// ---------------------------------------------------------------------------
#include <algorithm>
#include <sstream>
#include <fstream>
#include <unordered_map>
#include <boost/filesystem.hpp>
#include "gflags/gflags.h"
#include "imlab/buffer_manager.h"
// ---------------------------------------------------------------------------------------------------
namespace filesystem = boost::filesystem;
// ---------------------------------------------------------------------------------------------------
namespace {
// ---------------------------------------------------------------------------------------------------
bool ValidateWritable(const char *flagname, const std::string &value) {
    std::ofstream out(value);
    return out.good();
}
// ---------------------------------------------------------------------------------------------------
bool ValidateDirectory(const char *flagname, const std::string &value) {
    filesystem::path path(value);
    return filesystem::is_directory(path);
}
// ---------------------------------------------------------------------------------------------------
}  // namespace
// ---------------------------------------------------------------------------------------------------
int main(int argc, char *argv[]) {
    imlab::BufferManager<1024> manager{10};
    return 0;
}
// ---------------------------------------------------------------------------------------------------
