#include "ui_prefs.hpp"

KotoUiPreferences::KotoUiPreferences(std::optional<toml::value> v) {
    this->i_albumInfoShowDescription = true;
    this->i_albumInfoShowGenre = true;
    this->i_albumInfoShowNarrator = true;
    this->i_albumInfoShowYear = true;
    this->i_lastUsedVolume = 0.5;

    // No UI prefs provided
    if (!v.has_value()) return;
    toml::value& uiPrefs = v.value();
    this->i_albumInfoShowDescription = toml::find_or<bool>(uiPrefs, "album_info_show_description", false);
    this->i_albumInfoShowGenre = toml::find_or<bool>(uiPrefs, "album_info_show_genre", false);
    this->i_albumInfoShowNarrator = toml::find_or<bool>(uiPrefs, "album_info_show_narrator", false);
    this->i_albumInfoShowYear = toml::find_or<bool>(uiPrefs, "album_info_show_year", false);
    this->i_lastUsedVolume = toml::find_or<float>(uiPrefs, "last_used_volume", 0.5);
}

bool KotoUiPreferences::getAlbumInfoShowDescription() {
    return this->i_albumInfoShowDescription;
}

bool KotoUiPreferences::getAlbumInfoShowGenre() {
    return this->i_albumInfoShowGenre;
}

bool KotoUiPreferences::getAlbumInfoShowNarrator() {
    return this->i_albumInfoShowNarrator;
}

bool KotoUiPreferences::getAlbumInfoShowYear() {
    return this->i_albumInfoShowYear;
}

float KotoUiPreferences::getLastUsedVolume() {
    return this->i_lastUsedVolume;
}
