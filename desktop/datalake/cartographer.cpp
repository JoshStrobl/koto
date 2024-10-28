#include "cartographer.hpp"

#include <iostream>

Cartographer::Cartographer(QObject* parent)
    : QObject(parent),
      i_albums(QHash<QUuid, KotoAlbum*>()),
      i_artists_model(new KotoArtistModel(QList<KotoArtist*>())),
      i_artists_by_name(QHash<QString, KotoArtist*>()),
      i_tracks(QHash<QUuid, KotoTrack*>()) {}

Cartographer& Cartographer::instance() {
  static Cartographer _instance(nullptr);
  return _instance;
}

void Cartographer::addAlbum(KotoAlbum* album) {
  this->i_albums.insert(album->uuid, album);
}

void Cartographer::addArtist(KotoArtist* artist) {
  this->i_artists_model->addArtist(artist);
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
  for (auto artist : this->i_artists_model->getArtists()) {
    if (artist->uuid == uuid) { return std::optional {artist}; }
  }
  return std::nullopt;
}

QList<KotoArtist*> Cartographer::getArtists() {
  return this->i_artists_model->getArtists();
}

KotoArtistModel* Cartographer::getArtistsModel() {
  // if (this->i_artists_model == nullptr) { this->i_artists_model = new KotoArtistModel(this->i_artists); }
  return this->i_artists_model;
}

std::optional<KotoArtist*> Cartographer::getArtist(QString name) {
  auto artist = this->i_artists_by_name.value(name, nullptr);
  return artist ? std::optional {artist} : std::nullopt;
}

std::optional<KotoTrack*> Cartographer::getTrack(QUuid uuid) {
  auto track = this->i_tracks.value(uuid, nullptr);
  return track ? std::optional {track} : std::nullopt;
}

QList<KotoTrack*> Cartographer::getTracks() {
  return this->i_tracks.values();
}
