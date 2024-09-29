#include "library.hpp"
#include <string>

KotoLibraryConfig::KotoLibraryConfig(const toml::value &v) {
    this->i_name = toml::find<std::string>(v, "name");
    this->i_path = toml::find<std::string>(v, "path");
}

std::string KotoLibraryConfig::getName() {
    return this->i_name;
}

fs::path KotoLibraryConfig::getPath() {
    return this->i_path;
}
