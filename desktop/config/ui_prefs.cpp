#include "ui_prefs.hpp"

KotoUiPreferences::KotoUiPreferences()
    : i_albumInfoShowDescription(true), i_albumInfoShowGenre(true), i_albumInfoShowNarrator(true), i_albumInfoShowYear(true), i_lastUsedVolume(0.5) {}

KotoUiPreferences::KotoUiPreferences(std::optional<toml::value> v) {
  // No UI prefs provided
  if (!v.has_value()) return;
  toml::value& uiPrefs         = v.value();
  auto         showDescription = toml::find_or<bool>(uiPrefs, "album_info_show_description", false);
  auto         showGenre       = toml::find_or<bool>(uiPrefs, "album_info_show_genre", false);
  auto         showNarrator    = toml::find_or<bool>(uiPrefs, "album_info_show_narrator", false);
  auto         showYear        = toml::find_or<bool>(uiPrefs, "album_info_show_year", false);
  auto         lastUsedVolume  = toml::find_or<float>(uiPrefs, "last_used_volume", 0.5);

  this->setAlbumInfoShowDescription(showDescription);
  this->setAlbumInfoShowGenre(showGenre);
  this->setAlbumInfoShowNarrator(showNarrator);
  this->setAlbumInfoShowYear(showYear);
  this->setLastUsedVolume(lastUsedVolume);
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

toml::ordered_value KotoUiPreferences::serialize() {
  toml::ordered_value ui_prefs_table(toml::ordered_table {});
  ui_prefs_table["album_info_show_description"] = this->i_albumInfoShowDescription;
  ui_prefs_table["album_info_show_genre"]       = this->i_albumInfoShowGenre;
  ui_prefs_table["album_info_show_narrator"]    = this->i_albumInfoShowNarrator;
  ui_prefs_table["album_info_show_year"]        = this->i_albumInfoShowYear;
  ui_prefs_table["last_used_volume"]            = this->i_lastUsedVolume;
  return ui_prefs_table;
}

void KotoUiPreferences::setAlbumInfoShowDescription(bool show) {
  this->i_albumInfoShowDescription = show;
}

void KotoUiPreferences::setAlbumInfoShowGenre(bool show) {
  this->i_albumInfoShowGenre = show;
}

void KotoUiPreferences::setAlbumInfoShowNarrator(bool show) {
  this->i_albumInfoShowNarrator = show;
}

void KotoUiPreferences::setAlbumInfoShowYear(bool show) {
  this->i_albumInfoShowYear = show;
}

void KotoUiPreferences::setLastUsedVolume(float volume) {
  this->i_lastUsedVolume = volume;
}
