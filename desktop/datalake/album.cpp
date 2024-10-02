#include "structs.hpp"

KotoAlbum::KotoAlbum() {
  this->uuid   = QUuid::createUuid();
  this->tracks = QList<KotoTrack*>();
}

KotoAlbum* KotoAlbum::fromDb() {
  return new KotoAlbum();
}

KotoAlbum::~KotoAlbum() {
  for (auto track : this->tracks) { delete track; }
  this->tracks.clear();
}

void KotoAlbum::addTrack(KotoTrack* track) {
  this->tracks.append(track);
}

QString KotoAlbum::getAlbumArtPath() {
  return QString {this->album_art_path};
}

QString KotoAlbum::getDescription() {
  return QString {this->description};
}

QList<QString> KotoAlbum::getGenres() {
  return QList {this->genres};
}

QString KotoAlbum::getPath() {
  return this->path;
}

QString KotoAlbum::getNarrator() {
  return QString {this->narrator};
}

QString KotoAlbum::getTitle() {
  return QString {this->title};
}

QList<KotoTrack*> KotoAlbum::getTracks() {
  return QList {this->tracks};
}

int KotoAlbum::getYear() {
  return this->year;
}

void KotoAlbum::removeTrack(KotoTrack* track) {
  this->tracks.removeOne(track);
}

void KotoAlbum::setAlbumArtPath(QString str) {
  this->album_art_path = QString {path};
}

void KotoAlbum::setDescription(QString str) {
  this->description = QString {str};
}

void KotoAlbum::setGenres(QList<QString> list) {
  this->genres = QList {list};
}

void KotoAlbum::setNarrator(QString str) {
  this->narrator = QString {str};
}

void KotoAlbum::setPath(QString str) {
  this->path = QString {str};
}

void KotoAlbum::setTitle(QString str) {
  this->title = QString {str};
}

void KotoAlbum::setYear(int num) {
  this->year = num;
}
