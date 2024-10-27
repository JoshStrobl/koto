import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.Page {
    id: rootPage

    ColumnLayout {
        id: rootLayout

        anchors.fill: parent

        Controls.StackView {
            id: rootStack

            Layout.fillHeight: true
            Layout.fillWidth: true

            initialItem: HomePage {
            }
        }
    }
}