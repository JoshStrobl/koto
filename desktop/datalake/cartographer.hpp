#pragma once

#include <QtQml/qqmlregistration.h>

#include <QHash>
#include <QQmlEngine>
#include <QQmlListProperty>
#include <QString>
#include <QUuid>
#include <optional>

#include "structs.hpp"

class Cartographer : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    // Q_PROPERTY(QQmlListProperty<KotoAlbum*> albums READ getAlbumsQml)
    Q_PROPERTY(KotoArtistModel* artists READ getArtistsModel CONSTANT)
    // Q_PROPERTY(QQmlListProperty<KotoTrack*> tracks READ getTracksQml)

  public:
    Cartographer(QObject* parent);
    static Cartographer& instance();
    static Cartographer* create(QQmlEngine*, QJSEngine*) {
      QQmlEngine::setObjectOwnership(&instance(), QQmlEngine::CppOwnership);
      return &instance();
    }

    void addAlbum(KotoAlbum* album);
    void addArtist(KotoArtist* artist);
    void addTrack(KotoTrack* track);

    // QQmlListProperty<KotoAlbum*> getAlbumsQml();
    KotoArtistModel* getArtistsModel();
    // QQmlListProperty<KotoTrack*> getTracksQml();

    std::optional<KotoAlbum*>  getAlbum(QUuid uuid);
    QList<KotoAlbum*>          getAlbums();
    std::optional<KotoArtist*> getArtist(QUuid uuid);
    QList<KotoArtist*>         getArtists();
    std::optional<KotoArtist*> getArtist(QString name);
    std::optional<KotoTrack*>  getTrack(QUuid uuid);
    QList<KotoTrack*>          getTracks();

  private:
    QHash<QUuid, KotoAlbum*>    i_albums;
    KotoArtistModel*            i_artists_model;
    QHash<QString, KotoArtist*> i_artists_by_name;
    QHash<QUuid, KotoTrack*>    i_tracks;
};
