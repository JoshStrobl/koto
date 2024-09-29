import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls

import org.kde.kirigami as Kirigami

Kirigami.ApplicationWindow {
    id: root
    width: 1000
    height: 600
    visible: true
    title: "Koto"

    // TODO: Implement an onboarding page
    pageStack.initialPage: PrimaryNavigation {
        Component.onCompleted: {
            pageStack.push(Qt.createComponent("HomePage.qml"), {});
        }
    }
}
