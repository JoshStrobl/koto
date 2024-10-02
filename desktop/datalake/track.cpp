#include <iostream>

#include "cartographer.hpp"
#include "structs.hpp"

KotoTrack::KotoTrack() {
  this->uuid = QUuid::createUuid();
}

KotoTrack* KotoTrack::fromDb() {
  return new KotoTrack();
}

KotoTrack::~KotoTrack() {}

KotoTrack* KotoTrack::fromMetadata(const KFileMetaData::SimpleExtractionResult& metadata) {
  auto       props   = metadata.properties();
  KotoTrack* track   = new KotoTrack();
  track->disc_number = props.value(KFileMetaData::Property::DiscNumber, 0).toInt();
  track->duration    = props.value(KFileMetaData::Property::Duration, 0).toInt();

  QStringList genres;
  for (auto v : props.values(KFileMetaData::Property::Genre)) { genres.append(v.toString()); }

  track->genres = genres;

  track->lyrics       = props.value(KFileMetaData::Property::Lyrics).toString();
  track->narrator     = props.value(KFileMetaData::Property::Performer).toString();
  track->title        = props.value(KFileMetaData::Property::Title).toString();
  track->track_number = props.value(KFileMetaData::Property::TrackNumber, 0).toInt();
  track->year         = props.value(KFileMetaData::Property::ReleaseYear, 0).toInt();

  auto artistResult   = props.value(KFileMetaData::Property::Artist);
  auto artistOptional = std::optional<KotoArtist*>();
  if (artistResult.isValid()) {
    artistOptional = Cartographer::instance().getArtist(artistResult.toString());

    if (artistOptional.has_value()) {
      auto artist        = artistOptional.value();
      track->artist_uuid = QUuid(artist->uuid);
      artist->addTrack(track);
    }
  }

  auto albumResult = props.value(KFileMetaData::Property::Album);
  if (albumResult.isValid() && artistOptional.has_value()) {
    auto artist = artistOptional.value();
    std::cout << "Album: " << albumResult.toString().toStdString() << std::endl;

    auto albumMetaName = albumResult.toString();
    auto albumOptional = artist->getAlbumByName(albumMetaName);

    if (albumOptional.has_value()) {
      auto album        = albumOptional.value();
      track->album_uuid = QUuid(album->uuid);
      album->addTrack(track);
      if (album->getTitle() != albumMetaName) album->setTitle(albumMetaName);
    }
  }

  return track;
}

int KotoTrack::getDuration() {
  return this->duration;
}

QList<QString> KotoTrack::getGenres() {
  return QList {this->genres};
}

QString KotoTrack::getLyrics() {
  return QString {this->lyrics};
}

QString KotoTrack::getNarrator() {
  return QString {this->narrator};
}

QString KotoTrack::getPath() {
  return QString {this->path};
}

QString KotoTrack::getTitle() {
  return QString {this->title};
}

int KotoTrack::getTrackNumber() {
  return this->track_number;
}

int KotoTrack::getYear() {
  return this->year;
}

void KotoTrack::setAlbum(KotoAlbum* album) {
  this->album_uuid = QUuid(album->uuid);

  if (this->artist_uuid.isNull()) QUuid(album->artist_uuid);
}

void KotoTrack::setArtist(KotoArtist* artist) {
  this->artist_uuid = QUuid(artist->uuid);
}

void KotoTrack::setDiscNumber(int num) {
  this->disc_number = num;
}

void KotoTrack::setDuration(int num) {
  this->duration = num;
}

void KotoTrack::setGenres(QList<QString> list) {
  this->genres = QList {list};
}

void KotoTrack::setLyrics(QString str) {
  this->lyrics = QString {str};
}

void KotoTrack::setNarrator(QString str) {
  this->narrator = QString {str};
}

void KotoTrack::setPath(QString str) {
  this->path = QString {str};
}

void KotoTrack::setTitle(QString str) {
  this->title = QString {str};
}

void KotoTrack::setTrackNumber(int num) {
  this->track_number = num;
}

void KotoTrack::setYear(int num) {
  this->year = num;
}
