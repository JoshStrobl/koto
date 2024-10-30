import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

ColumnLayout {
    id: playerBar

    Layout.fillWidth: true
    Layout.leftMargin: Kirigami.Units.largeSpacing
    Layout.rightMargin: Kirigami.Units.largeSpacing
    spacing: 4

    RowLayout {
        Layout.fillWidth: true
        Layout.margins: Kirigami.Units.largeSpacing

        Controls.Slider {
            id: seekSlider

            Layout.fillWidth: true
        }
    }
    RowLayout {
        Layout.fillWidth: true
        Layout.margins: Kirigami.Units.largeSpacing
        Layout.maximumWidth: parent.width - 2 * Kirigami.Units.largeSpacing
        Layout.minimumWidth: parent.width - 2 * Kirigami.Units.largeSpacing

        RowLayout {
            anchors.left: parent.left

            Controls.Button {
                flat: true
                icon.height: Kirigami.Units.iconSizes.small
                icon.name: "media-seek-backward"
            }
            Controls.Button {
                flat: true
                icon.height: Kirigami.Units.iconSizes.medium
                icon.name: "media-playback-start"
                icon.width: Kirigami.Units.iconSizes.medium
            }
            Controls.Button {
                flat: true
                icon.height: Kirigami.Units.iconSizes.small
                icon.name: "media-seek-forward"
            }
        }
        RowLayout {
            anchors.right: parent.right

            Controls.Button {
                flat: true
                icon.height: Kirigami.Units.iconSizes.small
                icon.name: "media-playlist-repeat"
            }
            Controls.Button {
                flat: true
                icon.height: Kirigami.Units.iconSizes.small
                icon.name: "media-playlist-shuffle"
            }
            Controls.Button {
                flat: true
                icon.height: Kirigami.Units.iconSizes.small
                icon.name: "playlist-symbolic"
            }
            Controls.Button {
                flat: true
                icon.height: Kirigami.Units.iconSizes.small
                icon.name: "audio-volume-medium"
            }
        }
    }
}