find_package(Qt6 6.4 REQUIRED COMPONENTS Quick QuickControls2 Sql)
find_package(ECM REQUIRED NO_MODULE)
find_package(KF6Baloo)
find_package(KF6FileMetaData)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

include(KDEInstallDirs)
include(ECMFindQmlModule)
include(ECMQmlModule)

qt_standard_project_setup()

qt_add_executable(com.github.joshstrobl.koto
        config/config.cpp
        config/library.cpp
        config/ui_prefs.cpp
        datalake/album.cpp
        datalake/artist.cpp
        datalake/cartographer.cpp
        datalake/database.cpp
        datalake/indexer.cpp
        datalake/models.cpp
        datalake/track.cpp
        datalake/cartographer.hpp
        datalake/structs.hpp
        main.cpp
        datalake/models.cpp
)

target_include_directories(com.github.joshstrobl.koto PUBLIC datalake includes)
ecm_add_qml_module(com.github.joshstrobl.koto URI "com.github.joshstrobl.koto" GENERATE_PLUGIN_SOURCE)

ecm_target_qml_sources(com.github.joshstrobl.koto
        SOURCES
        qml/PlayerBar/PlayerBar.qml
        qml/PrimaryNavigation.qml
        qml/HomePage.qml
        qml/Main.qml
        qml/Root.qml
)

target_link_libraries(com.github.joshstrobl.koto
        PRIVATE KF6::Baloo KF6::FileMetaData Qt6::Quick Qt6::QuickControls2 Qt6::Sql
)

install(FILES com.github.joshstrobl.koto.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(TARGETS com.github.joshstrobl.koto ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

ecm_finalize_qml_module(com.github.joshstrobl.koto)
