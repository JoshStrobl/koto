#pragma once

#include <QHash>
#include <QString>
#include <QUuid>
#include <optional>

#include "structs.hpp"

class Cartographer {
  public:
    Cartographer();
    static Cartographer& instance();
    static Cartographer* create() { return &instance(); }

    void                      addAlbum(KotoAlbum* album);
    void                      addArtist(KotoArtist* artist);
    void                      addTrack(KotoTrack* track);
    std::optional<KotoAlbum*> getAlbum(QUuid uuid);
    //.std::optional<KotoAlbum*>  getAlbum(QString name);
    std::optional<KotoArtist*> getArtist(QUuid uuid);
    std::optional<KotoArtist*> getArtist(QString name);
    std::optional<KotoTrack*>  getTrack(QUuid uuid);

  private:
    QHash<QUuid, KotoAlbum*>    i_albums;
    QHash<QUuid, KotoArtist*>   i_artists;
    QHash<QString, KotoArtist*> i_artists_by_name;
    QHash<QUuid, KotoTrack*>    i_tracks;
};
