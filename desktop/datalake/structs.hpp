#pragma once
#include <KFileMetaData/SimpleExtractionResult>
#include <QFileInfo>
#include <QList>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QString>
#include <QUuid>

class KotoArtist;
class KotoAlbum;
class KotoTrack;

class KotoArtist {
  public:
    KotoArtist();
    static KotoArtist* fromDb(const QSqlQuery& query, const QSqlRecord& record);
    ~KotoArtist();

    QUuid uuid;

    void                      addAlbum(KotoAlbum* album);
    void                      addTrack(KotoTrack* track);
    void                      commit();
    QList<KotoAlbum*>         getAlbums();
    std::optional<KotoAlbum*> getAlbumByName(QString name);
    QString                   getName();
    QString                   getPath();
    QList<KotoTrack*>         getTracks();
    void                      removeAlbum(KotoAlbum* album);
    void                      removeTrack(KotoTrack* track);
    void                      setName(QString str);
    void                      setPath(QString str);

  private:
    QString path;
    QString name;

    QList<KotoAlbum*> albums;
    QList<KotoTrack*> tracks;
};

class KotoAlbum {
  public:
    KotoAlbum();
    static KotoAlbum* fromDb(const QSqlQuery& query, const QSqlRecord& record);
    ~KotoAlbum();

    QUuid uuid;
    QUuid artist_uuid;

    void               commit();
    QString            getAlbumArtPath();
    QString            getDescription();
    QList<QString>     getGenres();
    QString            getNarrator();
    QString            getPath();
    QString            getTitle();
    QList<KotoTrack*>  getTracks();
    std::optional<int> getYear();

    void addTrack(KotoTrack* track);
    void removeTrack(KotoTrack* track);
    void setAlbumArtPath(QString str);
    void setDescription(QString str);
    void setGenres(QList<QString> list);
    void setNarrator(QString str);
    void setPath(QString str);
    void setTitle(QString str);
    void setYear(int num);

  private:
    QString            title;
    QString            description;
    QString            narrator;
    std::optional<int> year;

    QList<QString>    genres;
    QList<KotoTrack*> tracks;

    QString path;
    QString album_art_path;
};

class KotoTrack {
  public:
    KotoTrack();  // No-op constructor
    static KotoTrack* fromDb(const QSqlQuery& query, const QSqlRecord& record);
    static KotoTrack* fromMetadata(const KFileMetaData::SimpleExtractionResult& metadata, const QFileInfo& info);
    ~KotoTrack();

    std::optional<QUuid> album_uuid;
    QUuid                artist_uuid;
    QUuid                uuid;

    void        commit();
    int         getDuration();
    QStringList getGenres();
    QString     getLyrics();
    QString     getNarrator();
    QString     getPath();
    QString     getTitle();
    int         getTrackNumber();
    int         getYear();

    void setAlbum(KotoAlbum* album);
    void setArtist(KotoArtist* artist);
    void setDiscNumber(int num);
    void setDuration(int num);
    void setGenres(QList<QString> list);
    void setLyrics(QString str);
    void setNarrator(QString str);
    void setPath(QString path);
    void setTitle(QString str);
    void setTrackNumber(int num);
    void setYear(int num);

  private:
    int         disc_number;
    int         duration;
    QStringList genres;
    QString     lyrics;
    QString     narrator;
    QString     path;
    QString     title;
    int         track_number;
    int         year;
};
