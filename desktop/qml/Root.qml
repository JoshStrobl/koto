import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.Page {
    id: rootPage

    ColumnLayout {
        id: rootLayout

        Layout.fillWidth: true

        Controls.StackView {
            id: rootStack

            initialItem: HomePage {
            }
        }
    }
}