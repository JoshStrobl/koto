#include "config.hpp"
#include <QDir>
#include <QStandardPaths>
#include <filesystem>
namespace fs = std::filesystem;

KotoConfig::KotoConfig() {
    // Define our application's config location
    auto configDir = QDir(QStandardPaths::writableLocation(QStandardPaths::StandardLocation::AppConfigLocation));
    auto configDirPath = configDir.absolutePath();
    fs::path filePath {};
    auto configPathStd = configDirPath.toStdString();
    filePath /= configPathStd;
    filePath /= "config.toml";

    auto data = toml::parse(filePath);
    std::optional<toml::value> ui_prefs;

    if (data.contains("preferences.ui")) {
      auto ui_prefs_at = data.at("preferences.ui");
      if (ui_prefs_at.is_table()) ui_prefs = ui_prefs_at.as_table();
    }

    auto prefs = KotoUiPreferences(ui_prefs);
    this->i_uiPreferences = &prefs;

    this->i_libraries = {};
    for (const auto& lib_value : toml::find<std::vector<toml::value>>(data, "libraries")) {
        auto lib = KotoLibraryConfig(lib_value);
        this->i_libraries.push_back(lib);
    }
}

KotoUiPreferences * KotoConfig::getUiPreferences() {
    return this->i_uiPreferences;
}

std::vector<KotoLibraryConfig> KotoConfig::getLibraries() {
    return this->i_libraries;
}
