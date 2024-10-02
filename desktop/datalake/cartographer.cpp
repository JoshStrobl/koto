#include "cartographer.hpp"

Cartographer::Cartographer()
    : i_albums(QHash<QUuid, KotoAlbum*>()),
      i_artists(QHash<QUuid, KotoArtist*>()),
      i_artists_by_name(QHash<QString, KotoArtist*>()),
      i_tracks(QHash<QUuid, KotoTrack*>()) {}

Cartographer& Cartographer::instance() {
  static Cartographer _instance;
  return _instance;
}

void Cartographer::addAlbum(KotoAlbum* album) {
  this->i_albums.insert(album->uuid, album);
}

void Cartographer::addArtist(KotoArtist* artist) {
  this->i_artists.insert(artist->uuid, artist);
  this->i_artists_by_name.insert(artist->getName(), artist);
}

void Cartographer::addTrack(KotoTrack* track) {
  this->i_tracks.insert(track->uuid, track);
}

std::optional<KotoAlbum*> Cartographer::getAlbum(QUuid uuid) {
  auto album = this->i_albums.value(uuid, nullptr);
  return album ? std::optional {album} : std::nullopt;
}

std::optional<KotoArtist*> Cartographer::getArtist(QUuid uuid) {
  auto artist = this->i_artists.value(uuid, nullptr);
  return artist ? std::optional {artist} : std::nullopt;
}

std::optional<KotoArtist*> Cartographer::getArtist(QString name) {
  auto artist = this->i_artists_by_name.value(name, nullptr);
  return artist ? std::optional {artist} : std::nullopt;
}

std::optional<KotoTrack*> Cartographer::getTrack(QUuid uuid) {
  auto track = this->i_tracks.value(uuid, nullptr);
  return track ? std::optional {track} : std::nullopt;
}
