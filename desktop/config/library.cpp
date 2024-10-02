#include "library.hpp"

#include <QDebug>
#include <string>

KotoLibraryConfig::KotoLibraryConfig(std::string name, fs::path path) {
  this->i_name = name;
  this->i_path = path;
  qDebug() << "Library: " << this->i_name.c_str() << " at " << this->i_path.c_str();
}

KotoLibraryConfig::~KotoLibraryConfig() {}

KotoLibraryConfig::KotoLibraryConfig(const toml::value& v) {
  this->i_name = toml::find<std::string>(v, "name");
  this->i_path = toml::find<std::string>(v, "path");
}

std::string KotoLibraryConfig::getName() {
  return this->i_name;
}

fs::path KotoLibraryConfig::getPath() {
  return this->i_path;
}
