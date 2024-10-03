import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.GlobalDrawer {
    id: primaryNavigation

    property Kirigami.ApplicationWindow windowRef

    function isMobile(width) {
        return width < 800;
    }
    function onWindowSizeChanged(width) {
        drawerOpen = !isMobile(width);
        modal = isMobile(width);
    }

    collapseButtonVisible: false
    drawerOpen: !isMobile()
    edge: Qt.LeftEdge
    height: parent.height
    modal: isMobile()

    actions: [
        Kirigami.Action {
            icon.name: "go-home"
            text: "Home"

            onTriggered: console.log("Home triggered")
        },
        Kirigami.Action {
            expandible: true
            icon.name: "bookmark"
            text: "Audiobooks"

            onTriggered: console.log("Audiobooks triggered")
        },
        Kirigami.Action {
            expandible: true
            icon.name: "emblem-music-symbolic"
            text: "Music"

            children: [
                Kirigami.Action {
                    text: "Local Library"

                    onTriggered: console.log("Music Local Library triggered")
                },
                Kirigami.Action {
                    text: "Radio"

                    onTriggered: console.log("Music Radio triggered")
                }
            ]
        },
        Kirigami.Action {
            expandible: true
            icon.name: "application-rss+xml-symbolic"
            text: "Podcasts"

            children: [
                Kirigami.Action {
                    text: "Library"

                    onTriggered: console.log("Podcasts Library triggered")
                },
                Kirigami.Action {
                    text: "Find new podcasts"

                    onTriggered: console.log("Podcasts Find new podcasts triggered")
                }
            ]
        },
        Kirigami.Action {
            expandible: true
            icon.name: "music-playlist-symbolic"
            text: "Playlists"

            children: [
                Kirigami.Action {
                    text: "Library"

                    onTriggered: console.log("Playlists Library triggered")
                }
            ]
            // TODO: Generate list of playlists
        }
    ]
    header: Kirigami.SearchField {
        id: searchEntry

        placeholderText: qsTr("Search")
    }

    Component.onCompleted: {
        if (Kirigami.Settings.isMobile)
            return;
        if (windowRef)
            windowRef.onWidthChanged.connect(() => onWindowSizeChanged(windowRef.width));
    }
}
