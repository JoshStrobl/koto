import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

// Kirigami.Page {
//     ColumnLayout {
//         Controls.TextArea {
//             Layout.alignment: Qt.AlignTop
//             id: searchEntry
//             placeholderText: qsTr("Search")
//         }
//     }
// }

Kirigami.GlobalDrawer {
    width: 200
    height: parent.height
    edge: Qt.LeftEdge
    modal: false

    header: Kirigami.SearchField {
        id: searchEntry
        placeholderText: qsTr("Search")
    }

    actions: [
                Kirigami.Action {
                    text: "Home"
                    icon.name: "go-home"
                    onTriggered: console.log("Home triggered")
                },
                Kirigami.Action {
                    text: "Audiobooks"
                    expandible: true
                    icon.name: "bookmark"
                    onTriggered: console.log("Audiobooks triggered")
                },
                Kirigami.Action {
                    text: "Music"
                    expandible: true
                    icon.name: "emblem-music-symbolic"
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
                    text: "Podcasts"
                    expandible: true
                    icon.name: "application-rss+xml-symbolic"
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
                    text: "Playlists"
                    expandible: true
                    icon.name: "music-playlist-symbolic"
                    children: [
                        Kirigami.Action {
                            text: "Library"
                            onTriggered: console.log("Playlists Library triggered")
                        },
                        Kirigami.Action {
                            text: "Find new playlists"
                            onTriggered: console.log("Playlists Find new playlists triggered")
                        }
                    ]
                    // TODO: Generate list of playlists
                }
            ]

}
