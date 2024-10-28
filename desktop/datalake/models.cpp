#include "structs.hpp"

KotoArtistModel::KotoArtistModel(const QList<KotoArtist*>& artists, QObject* parent) : QAbstractListModel(parent), m_artists(artists) {}

KotoArtistModel::~KotoArtistModel() {
  this->beginResetModel();
  this->m_artists.clear();
  this->endResetModel();
}

void KotoArtistModel::addArtist(KotoArtist* artist) {
  this->beginInsertRows(QModelIndex(), this->m_artists.count(), this->m_artists.count());
  this->m_artists.append(artist);
  this->endInsertRows();
}

int KotoArtistModel::rowCount(const QModelIndex& parent) const {
  return this->m_artists.count();
}

QVariant KotoArtistModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid()) { return {}; }

  if (index.row() >= this->m_artists.size()) { return {}; }

  if (role == Qt::DisplayRole) {
    return this->m_artists.at(index.row())->getName();
  } else if (role == KotoArtistRoles::NameRole) {
    return this->m_artists.at(index.row())->getName();
  } else if (role == KotoArtistRoles::PathRole) {
    return this->m_artists.at(index.row())->getPath();
  } else if (role == KotoArtistRoles::UuidRole) {
    return this->m_artists.at(index.row())->uuid;
  } else {
    return {};
  }
}

QList<KotoArtist*> KotoArtistModel::getArtists() {
  return this->m_artists;
}

QHash<int, QByteArray> KotoArtistModel::roleNames() const {
  QHash<int, QByteArray> roles;
  roles[NameRole] = QByteArrayLiteral("name");
  roles[PathRole] = QByteArrayLiteral("path");
  roles[UuidRole] = QByteArrayLiteral("uuid");
  return roles;
}
