import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import com.github.joshstrobl.koto

Kirigami.ScrollablePage {
    Component {
        id: listDelegate

        Controls.ItemDelegate {
            required property string name

            text: name
            width: ListView.view.width
        }
    }
    ListView {
        Layout.fillHeight: true
        Layout.fillWidth: true
        delegate: listDelegate
        model: Cartographer.artists
    }
}
