import org.kde.kirigami as Kirigami

Kirigami.ApplicationWindow {
    id: root

    height: 600
    title: "Koto"
    visible: true
    width: 1000

    globalDrawer: PrimaryNavigation {
        windowRef: root
    }

    // TODO: Implement an onboarding page
    pageStack.initialPage: HomePage {
    }
}
