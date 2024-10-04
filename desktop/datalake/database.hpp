#pragma once

#include <QSqlDatabase>

class KotoDatabase {
  public:
    KotoDatabase();
    static KotoDatabase& instance();
    static KotoDatabase* create() { return &instance(); }

    void         connect();
    void         disconnect();
    QSqlDatabase getDatabase();
    void         load();
    bool         requiredBootstrap();

  private:
    void         bootstrap();
    bool         shouldBootstrap;
    QSqlDatabase db;
};
