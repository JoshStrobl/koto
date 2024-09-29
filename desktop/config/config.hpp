#pragma once

#include <vector>
#include "library.hpp"
#include "ui_prefs.hpp"

class KotoConfig {
    public:
        KotoConfig();
        ~KotoConfig();
        std::vector<KotoLibraryConfig> getLibraries();
        KotoUiPreferences * getUiPreferences();

    private:
        std::vector<KotoLibraryConfig> i_libraries;
        KotoUiPreferences * i_uiPreferences;

};
