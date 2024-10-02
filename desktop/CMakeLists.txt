find_package(Qt6 6.4 REQUIRED COMPONENTS Quick QuickControls2)
find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)
find_package(KF6Baloo)
find_package(KF6FileMetaData)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

include(KDEInstallDirs)
include(ECMFindQmlModule)
include(ECMQmlModule)

qt_standard_project_setup()

qt_add_executable(koto
        main.cpp
        config/config.cpp
        config/library.cpp
        config/ui_prefs.cpp
        datalake/indexer.cpp
        datalake/track.cpp
        datalake/album.cpp
        datalake/artist.cpp
        datalake/cartographer.cpp
        datalake/cartographer.hpp
)

ecm_add_qml_module(koto URI "com.github.joshstrobl.koto" GENERATE_PLUGIN_SOURCE)

ecm_target_qml_sources(koto
        SOURCES
        qml/PrimaryNavigation.qml
        qml/HomePage.qml
        qml/Main.qml
)

target_link_libraries(koto
        PRIVATE KF6::Baloo KF6::FileMetaData Qt6::Quick Qt6::QuickControls2
)

install(FILES com.github.joshstrobl.koto.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(TARGETS koto ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

ecm_finalize_qml_module(koto)
