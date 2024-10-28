#pragma once
#include <QtQml/qqmlregistration.h>

#include <KFileMetaData/SimpleExtractionResult>
#include <QAbstractListModel>
#include <QFileInfo>
#include <QList>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QString>
#include <QUuid>

class KotoArtist;
class KotoArtistModel;
class KotoAlbum;
class KotoAlbumModel;
class KotoTrack;
class KotoTrackModel;

class KotoArtist : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString name READ getName WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString path READ getPath WRITE setPath NOTIFY pathChanged)
    Q_PROPERTY(QList<KotoAlbum*> albums READ getAlbums NOTIFY albumsChanged)
    Q_PROPERTY(QList<KotoTrack*> tracks READ getTracks NOTIFY tracksChanged)
    Q_PROPERTY(QUuid uuid READ getUuid)

  public:
    KotoArtist();
    static KotoArtist* fromDb(const QSqlQuery& query, const QSqlRecord& record);
    virtual ~KotoArtist();

    QUuid uuid;

    void                      addAlbum(KotoAlbum* album);
    void                      addTrack(KotoTrack* track);
    void                      commit();
    QList<KotoAlbum*>         getAlbums();
    std::optional<KotoAlbum*> getAlbumByName(QString name);
    QString                   getName();
    QString                   getPath();
    QList<KotoTrack*>         getTracks();
    QUuid                     getUuid();
    void                      removeAlbum(KotoAlbum* album);
    void                      removeTrack(KotoTrack* track);
    void                      setName(QString str);
    void                      setPath(QString str);

  signals:
    void albumsChanged();
    void nameChanged();
    void pathChanged();
    void tracksChanged();

  private:
    QString path;
    QString name;

    QList<KotoAlbum*> albums;
    QList<KotoTrack*> tracks;
};

class KotoArtistModel : public QAbstractListModel {
    Q_OBJECT

  public:
    explicit KotoArtistModel(const QList<KotoArtist*>& artists, QObject* parent = nullptr);

    void                   addArtist(KotoArtist* artist);
    int                    rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant               data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    virtual ~KotoArtistModel();

    QList<KotoArtist*> getArtists();

    enum KotoArtistRoles {
      NameRole = Qt::UserRole + 1,
      PathRole,
      UuidRole,
    };

  private:
    QList<KotoArtist*> m_artists;
};

class KotoAlbum : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString albumArtPath READ getAlbumArtPath WRITE setAlbumArtPath NOTIFY albumArtChanged)
    Q_PROPERTY(QString description READ getDescription WRITE setDescription NOTIFY descriptionChanged)
    Q_PROPERTY(QList<QString> genres READ getGenres WRITE setGenres NOTIFY genresChanged)
    Q_PROPERTY(QString narrator READ getNarrator WRITE setNarrator NOTIFY narratorChanged)
    Q_PROPERTY(QString path READ getPath WRITE setPath NOTIFY pathChanged)
    Q_PROPERTY(QString title READ getTitle WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(QList<KotoTrack*> tracks READ getTracks NOTIFY tracksChanged)
    Q_PROPERTY(int year READ getYearQml WRITE setYear NOTIFY yearChanged)

  public:
    KotoAlbum();
    static KotoAlbum* fromDb(const QSqlQuery& query, const QSqlRecord& record);
    virtual ~KotoAlbum();

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
    int                getYearQml();

    void addTrack(KotoTrack* track);
    void removeTrack(KotoTrack* track);
    void setAlbumArtPath(QString str);
    void setDescription(QString str);
    void setGenres(QList<QString> list);
    void setNarrator(QString str);
    void setPath(QString str);
    void setTitle(QString str);
    void setYear(int num);

  signals:
    void albumArtChanged();
    void descriptionChanged();
    void genresChanged();
    void narratorChanged();
    void pathChanged();
    void titleChanged();
    void tracksChanged();
    void yearChanged();

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

class KotoTrack : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString album_uuid READ getAlbumUuid NOTIFY albumChanged)
    Q_PROPERTY(QUuid artist_uuid READ getArtistUuid NOTIFY artistChanged)
    Q_PROPERTY(QUuid uuid READ getUuid)
    Q_PROPERTY(int discNumber READ getDiscNumber WRITE setDiscNumber NOTIFY discNumberChanged)
    Q_PROPERTY(int duration READ getDuration WRITE setDuration NOTIFY durationChanged)
    Q_PROPERTY(QStringList genres READ getGenres WRITE setGenres NOTIFY genresChanged)
    Q_PROPERTY(QString lyrics READ getLyrics WRITE setLyrics NOTIFY lyricsChanged)
    Q_PROPERTY(QString narrator READ getNarrator WRITE setNarrator NOTIFY narratorChanged)
    Q_PROPERTY(QString path READ getPath WRITE setPath NOTIFY pathChanged)
    Q_PROPERTY(QString title READ getTitle WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(int trackNumber READ getTrackNumber WRITE setTrackNumber NOTIFY trackNumberChanged)
    Q_PROPERTY(int year READ getYear WRITE setYear NOTIFY yearChanged)

  public:
    KotoTrack();  // No-op constructor
    static KotoTrack* fromDb(const QSqlQuery& query, const QSqlRecord& record);
    static KotoTrack* fromMetadata(const KFileMetaData::SimpleExtractionResult& metadata, const QFileInfo& info);
    virtual ~KotoTrack();

    std::optional<QUuid> album_uuid;
    QUuid                artist_uuid;
    QUuid                uuid;

    void        commit();
    QString     getAlbumUuid();
    QUuid       getArtistUuid();
    int         getDiscNumber();
    int         getDuration();
    QStringList getGenres();
    QString     getLyrics();
    QString     getNarrator();
    QString     getPath();
    QString     getTitle();
    int         getTrackNumber();
    QUuid       getUuid();
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

  signals:
    void albumChanged();
    void artistChanged();
    void discNumberChanged();
    void durationChanged();
    void genresChanged();
    void lyricsChanged();
    void narratorChanged();
    void pathChanged();
    void titleChanged();
    void trackNumberChanged();
    void yearChanged();

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
