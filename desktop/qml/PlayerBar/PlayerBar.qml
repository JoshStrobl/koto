import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

ColumnLayout {
    id: playerBar

    Layout.fillWidth: true

    Controls.Slider {
        id: seekSlider

        Layout.fillWidth: true
    }
    RowLayout {
        Layout.fillWidth: true

        Controls.Button {
            icon.height: Kirigami.Units.iconSizes.small
            icon.name: "media-seek-backward"
        }
    }
}