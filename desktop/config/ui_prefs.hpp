#pragma once
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "includes/toml.hpp"

class KotoUiPreferences {
  public:
    KotoUiPreferences();
    KotoUiPreferences(std::optional<toml::value> v);
    ~KotoUiPreferences();

    bool  getAlbumInfoShowDescription();
    bool  getAlbumInfoShowGenre();
    bool  getAlbumInfoShowNarrator();
    bool  getAlbumInfoShowYear();
    float getLastUsedVolume();

    toml::ordered_value serialize();

    void setAlbumInfoShowDescription(bool show);
    void setAlbumInfoShowGenre(bool show);
    void setAlbumInfoShowNarrator(bool show);
    void setAlbumInfoShowYear(bool show);
    void setLastUsedVolume(float volume);

  private:
    bool  i_albumInfoShowDescription;
    bool  i_albumInfoShowGenre;
    bool  i_albumInfoShowNarrator;
    bool  i_albumInfoShowYear;
    float i_lastUsedVolume;
};
