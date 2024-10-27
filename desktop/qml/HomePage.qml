import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
    Component {
        id: listDelegate

        Controls.ItemDelegate {
            required property string name

            text: name
            width: ListView.view.width
        }
    }
    // ListModel {
    //     id: blah
    //
    //     ListElement {
    //         name: "blah1"
    //     }
    //     ListElement {
    //         name: "blah2"
    //     }
    //     ListElement {
    //         name: "blah3"
    //     }
    // }
    ListView {
        Layout.fillHeight: true
        Layout.fillWidth: true
        delegate: listDelegate
        //model: blah
        model: Cartographer.artists
    }
}
