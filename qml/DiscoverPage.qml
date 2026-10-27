import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: page
    property var shell
    property int categoryIndex: 0
    property bool searchPending: false
    property bool started: false
    property var detailProject: ({})
    property bool syncingResults: false
    property bool moreScheduled: false
    ListModel {
        id: projectModel
        dynamicRoles: true
    }

    function projectKey(project) {
        return (project.provider || "") + "/" + (project.projectId || "");
    }
    function syncResults() {
        syncingResults = true;
        const incoming = launcher.results;
        let appendOnly = projectModel.count <= incoming.length;
        for (let i = 0; appendOnly && i < projectModel.count; ++i)
            appendOnly = projectKey(projectModel.get(i).project) === projectKey(incoming[i]);
        if (!appendOnly) {
            projectModel.clear();
            projectGrid.contentY = 0;
        }
        // Keep the existing rows and scroll position while the next page arrives.
        for (let i = projectModel.count; i < incoming.length; ++i)
            projectModel.append({
                project: incoming[i]
            });
        syncingResults = false;
    }
    readonly property bool searching: launcher.searching
    readonly property var categories: ["Popular", "Performance", "Adventure", "Tech", "Magic", "Utility"]

    function syncType() {
        if (!shell)
            return;
        const desired = shell.discoverType || "modpack";
        for (let i = 0; i < typeChoice.model.length; ++i)
            if (typeChoice.model[i].value === desired) {
                typeChoice.currentIndex = i;
                break;
            }
    }
    function runSearch() {
        if (!shell)
            return;
        if (launcher.busy) {
            searchPending = true;
            return;
        }
        searchPending = false;
        started = true;
        launcher.search(sourceChoice.currentText, typeChoice.currentValue, searchField.text.trim(), categories[categoryIndex]);
    }
    function maybeLoadMore() {
        if (moreScheduled || syncingResults || !started || !launcher.canLoadMore || launcher.searching || launcher.loadingMore)
            return;
        if (projectGrid.count === 0 || projectGrid.contentY + projectGrid.height < projectGrid.contentHeight - 320)
            return;
        moreScheduled = true;
        Qt.callLater(function() {
            page.moreScheduled = false;
            if (!page.syncingResults && launcher.canLoadMore && !launcher.searching && !launcher.loadingMore && projectGrid.contentY + projectGrid.height >= projectGrid.contentHeight - 320)
                launcher.loadMore();
        });
    }
    function openDetail(project) {
        detailProject = project;
        detailDrawer.open();
    }
    function downloadCount(value) {
        const n = Number(value || 0);
        if (n >= 1000000)
            return (n / 1000000).toFixed(n >= 10000000 ? 0 : 1) + "M";
        if (n >= 1000)
            return (n / 1000).toFixed(n >= 10000 ? 0 : 1) + "K";
        return n.toString();
    }
    onShellChanged: if (shell) {
        syncType();
        if(shell){searchField.text=shell.discoverQuery||"";sourceChoice.currentIndex=shell.discoverProvider==="CurseForge"?1:0}
        syncResults();
        Qt.callLater(runSearch);
    }
    Connections {
        target: page.shell || null
        ignoreUnknownSignals: true
        function onDiscoverTypeChanged() {
            page.syncType();
            page.runSearch();
        }
    }
    Connections {
        target: launcher
        function onBusyChanged() {
            if (!launcher.busy && page.searchPending)
                Qt.callLater(page.runSearch);
        }
        function onResultsChanged() {
            page.syncResults();
            page.started = true;
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16
        RowLayout {
            Layout.fillWidth: true
            ColumnLayout {
                spacing: 4
                Text {
                    text: "Discover"
                    color: Theme.text
                    font.family: Theme.fontFamily
                    font.pixelSize: 26
                    font.weight: Font.Bold
                    font.letterSpacing: -0.5
                }
                Text {
                    text: "Browse Minecraft content from Modrinth and CurseForge."
                    color: Theme.textSecondary
                    font.pixelSize: 12
                }
            }
            Item {
                Layout.fillWidth: true
            }
            MButton {
                text: "Import file"
                implicitHeight: 34
                enabled: !launcher.busy
                onClicked: shell.chooseFile(typeChoice.currentValue === "modpack" ? "pack" : "content", typeChoice.currentValue)
            }
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            MField {
                id: searchField
                Layout.fillWidth: true
                implicitHeight: 40
                placeholderText: "Search " + typeChoice.currentText.toLowerCase() + "…"
                Accessible.name: "Search projects"
                onAccepted: page.runSearch()
            }
            MComboBox {
                id: sourceChoice
                objectName: "providerChoice"
                Layout.preferredWidth: 160
                implicitHeight: 40
                providerIcons: true
                model: ["Modrinth", "CurseForge"]
                Accessible.name: "Content source"
                onActivated: page.runSearch()
            }
            MComboBox {
                id: typeChoice
                Layout.preferredWidth: 150
                implicitHeight: 40
                textRole: "text"
                valueRole: "value"
                model: [
                    {
                        text: "Modpacks",
                        value: "modpack"
                    },
                    {
                        text: "Mods",
                        value: "mod"
                    },
                    {
                        text: "Resource packs",
                        value: "resourcepack"
                    },
                    {
                        text: "Shaders",
                        value: "shader"
                    },
                    {
                        text: "Data packs",
                        value: "datapack"
                    }
                ]
                Accessible.name: "Content type"
                onActivated: {
                    if (shell.discoverType !== currentValue)
                        shell.discoverType = currentValue;
                    else
                        page.runSearch();
                }
            }
            MButton {
                text: "Search"
                kind: "primary"
                implicitHeight: 40
                enabled: !launcher.busy
                onClicked: page.runSearch()
            }
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            Repeater {
                model: page.categories
                delegate: Button {
                    id: chip
                    required property string modelData
                    required property int index
                    implicitHeight: 30
                    implicitWidth: chipText.implicitWidth + 24
                    hoverEnabled: true
                    padding: 0
                    onClicked: {
                        if (page.categoryIndex === index)
                            return;
                        page.categoryIndex = index;
                        page.runSearch();
                    }
                    Accessible.name: modelData + " projects"
                    background: Rectangle {
                        radius: 15
                        color: page.categoryIndex === chip.index ? Theme.accentSoft : chip.hovered ? Theme.surface2 : Theme.surface
                        border.color: page.categoryIndex === chip.index ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.35) : chip.activeFocus ? Theme.accent : Theme.border
                        border.width: chip.activeFocus ? 2 : 1
                        Behavior on color {
                            ColorAnimation {
                                duration: Theme.motion ? 150 : 0
                            }
                        }
                    }
                    contentItem: Text {
                        id: chipText
                        text: chip.modelData
                        color: page.categoryIndex === chip.index ? Theme.text : Theme.textSecondary
                        font.pixelSize: 12
                        font.weight: page.categoryIndex === chip.index ? Font.DemiBold : Font.Normal
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        Behavior on color {
                            ColorAnimation {
                                duration: Theme.motion ? 150 : 0
                            }
                        }
                    }
                }
            }
            Item {
                Layout.fillWidth: true
            }
        }
        Rectangle {
            visible: sourceChoice.currentText === "CurseForge" && !launcher.curseConnected
            Layout.fillWidth: true
            implicitHeight: connectionHint.implicitHeight + 24
            radius: 8
            color: Theme.surface
            border.color: Theme.border
            Text {
                id: connectionHint
                anchors.fill: parent
                anchors.margins: 12
                text: "Add a CurseForge API key in Settings → Connections to use this source."
                color: Theme.textSecondary
                font.pixelSize: 12
                wrapMode: Text.Wrap
            }
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Text {
                text: page.searching ? "Searching…" : searchField.text || page.categoryIndex !== 0 ? "Search results" : "Popular " + typeChoice.currentText.toLowerCase()
                color: Theme.text
                font.pixelSize: 16
                font.weight: Font.DemiBold
            }
            Text {
                visible: launcher.results.length > 0
                text: (launcher.resultTotal > launcher.results.length ? launcher.results.length + " of " + launcher.resultTotal : launcher.results.length) + " projects"
                color: Theme.textMuted
                font.pixelSize: 12
            }
            Item {
                Layout.fillWidth: true
            }
            Text {
                visible: typeChoice.currentValue !== "modpack" && !!launcher.selected.version
                text: (launcher.selected.name || "") + " · " + (launcher.selected.loader || "") + " " + (launcher.selected.version || "")
                color: Theme.textMuted
                font.pixelSize: 11
                elide: Text.ElideRight
                Layout.maximumWidth: 310
            }
        }
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            GridView {
                id: projectGrid
                anchors.fill: parent
                visible: launcher.results.length > 0
                opacity: page.searching ? 0.4 : 1
                Behavior on opacity {
                    NumberAnimation {
                        duration: Theme.motion ? 180 : 0
                    }
                }
                clip: true
                model: projectModel
                cellWidth: width / Math.max(2, Math.floor(width / 232))
                cellHeight: cellWidth * 0.5625 + 162
                boundsBehavior: Flickable.StopAtBounds
                cacheBuffer: cellHeight * 2
                ScrollBar.vertical: ScrollBar {}
                footer: Item {
                    width: projectGrid.width
                    height: projectModel.count > 0 ? 58 : 0
                    Row {
                        anchors.centerIn: parent
                        spacing: 10
                        BusyIndicator {
                            width: 24
                            height: 24
                            visible: launcher.loadingMore && Theme.motion
                            running: visible
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text {
                            width: Math.min(implicitWidth, Math.max(180, projectGrid.width - 140))
                            text: launcher.loadingMore ? "Loading more projects…" : launcher.searchError ? launcher.searchError : launcher.canLoadMore ? "Scroll for more projects" : "All matching projects loaded"
                            color: launcher.searchError ? Theme.textSecondary : Theme.textMuted
                            font.pixelSize: 12
                            elide: Text.ElideRight
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        MButton {
                            visible: !!launcher.searchError && !launcher.loadingMore
                            text: "Retry"
                            compact: true
                            onClicked: launcher.canLoadMore ? page.maybeLoadMore() : page.runSearch()
                        }
                    }
                }
                onAtYEndChanged: if (atYEnd)
                    page.maybeLoadMore()
                onContentYChanged: if (contentY + height > contentHeight - 320)
                    page.maybeLoadMore()
                onContentHeightChanged: Qt.callLater(page.maybeLoadMore)

                delegate: Item {
                    id: cardItem
                    required property var project
                    readonly property var modelData: project
                    required property int index
                    property bool imagesRequested: false
                    function requestVisibleImages() {
                        if (!imagesRequested && y + height >= projectGrid.contentY - projectGrid.cellHeight && y <= projectGrid.contentY + projectGrid.height + projectGrid.cellHeight)
                            imagesRequested = true;
                    }
                    onYChanged: requestVisibleImages()
                    Connections {
                        target: projectGrid
                        function onContentYChanged() {
                            cardItem.requestVisibleImages();
                        }
                        function onHeightChanged() {
                            cardItem.requestVisibleImages();
                        }
                    }
                    width: projectGrid.cellWidth
                    height: projectGrid.cellHeight
                    opacity: 0
                    transform: Translate {
                        id: enterShift
                        y: 14
                    }
                    Component.onCompleted: {
                        requestVisibleImages();
                        enterAnim.start();
                    }
                    SequentialAnimation {
                        id: enterAnim
                        PauseAnimation {
                            duration: Theme.motion ? Math.min(index, 5) * 20 : 0
                        }
                        ParallelAnimation {
                            NumberAnimation {
                                target: cardItem
                                property: "opacity"
                                to: 1
                                duration: Theme.motion ? 220 : 0
                                easing.type: Easing.OutCubic
                            }
                            NumberAnimation {
                                target: enterShift
                                property: "y"
                                to: 0
                                duration: Theme.motion ? 260 : 0
                                easing.type: Easing.OutQuint
                            }
                        }
                    }
                    Rectangle {
                        id: card
                        anchors.fill: parent
                        anchors.rightMargin: 12
                        anchors.bottomMargin: 12
                        radius: 17
                        color: cardHover.hovered ? Theme.surface2 : Theme.surface
                        border.color: cardHover.hovered ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.4) : Theme.border
                        scale: cardHover.hovered && Theme.motion ? 1.012 : 1
                        Behavior on scale {
                            NumberAnimation {
                                duration: Theme.motion ? 190 : 0
                                easing.type: Easing.OutCubic
                            }
                        }
                        Behavior on color {
                            ColorAnimation {
                                duration: Theme.motion ? 160 : 0
                            }
                        }
                        Behavior on border.color {
                            ColorAnimation {
                                duration: Theme.motion ? 160 : 0
                            }
                        }
                        HoverHandler {
                            id: cardHover
                        }
                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 0
                            ItemDelegate {
                                Layout.fillWidth: true
                                Layout.preferredHeight: (projectGrid.cellWidth - 12) * 0.5625
                                padding: 0
                                onClicked: page.openDetail(modelData)
                                Accessible.name: "Details for " + modelData.title
                                background: Rectangle {
                                    color: Theme.surface2
                                    radius: 11
                                    border.color: parent.activeFocus ? Theme.accent : Theme.border
                                    border.width: parent.activeFocus ? 2 : 1
                                }
                                contentItem: Item {
                                    clip: true
                                    Image {
                                        anchors.fill: parent
                                        source: cardItem.imagesRequested ? modelData.thumbnail || "" : ""
                                        fillMode: Image.PreserveAspectCrop
                                        asynchronous: true
                                        cache: true
                                        sourceSize.width: Math.ceil(projectGrid.cellWidth * 1.5)
                                        visible: !!modelData.thumbnail
                                        opacity: status === Image.Ready ? 1 : 0
                                        Behavior on opacity {
                                            NumberAnimation {
                                                duration: Theme.motion ? 260 : 0
                                            }
                                        }
                                    }
                                    Rectangle {
                                        anchors.fill:parent;visible:!modelData.thumbnail;color:Theme.surface2
                                    }
                                    Rectangle {
                                        anchors.bottom: parent.bottom
                                        width: parent.width
                                        height: 34
                                        visible: !!modelData.thumbnail
                                        gradient: Gradient {
                                            GradientStop {
                                                position: 0
                                                color: "transparent"
                                            }
                                            GradientStop {
                                                position: 1
                                                color: Qt.rgba(Theme.app.r, Theme.app.g, Theme.app.b, 0.55)
                                            }
                                        }
                                    }
                                    Rectangle {
                                        anchors.left:parent.left;anchors.bottom:parent.bottom;anchors.leftMargin:12;anchors.bottomMargin:10
                                        width:46;height:46;radius:12;color:Theme.surface;border.color:Theme.border
                                        Image {id:projectMark;anchors.fill:parent;anchors.margins:5;source:cardItem.imagesRequested?modelData.icon||"":"";sourceSize.width:92;sourceSize.height:92;asynchronous:true;fillMode:Image.PreserveAspectFit;visible:status===Image.Ready}
                                        Icon {anchors.centerIn:parent;name:"box";size:22;visible:projectMark.status!==Image.Ready;color:Theme.textMuted}
                                    }
                                }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.margins: 12
                                spacing: 7
                                Text {
                                    text: modelData.title
                                    color: Theme.text
                                    font.pixelSize: 14
                                    font.weight: Font.DemiBold
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                                Text {
                                    text: (modelData.author ? modelData.author + " · " : "") + page.downloadCount(modelData.downloads) + " downloads"
                                    color: Theme.textMuted
                                    font.pixelSize: 11
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                                Text {
                                    text: modelData.description || "No description provided."
                                    color: Theme.textSecondary
                                    font.pixelSize: 12
                                    wrapMode: Text.Wrap
                                    maximumLineCount: 2
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    verticalAlignment: Text.AlignTop
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    Text {
                                        text: modelData.provider
                                        color: Theme.textMuted
                                        font.pixelSize: 10
                                        Layout.fillWidth: true
                                    }
                                    MButton {
                                        text: "Install"
                                        kind: "primary"
                                        implicitHeight: 30
                                        implicitWidth: 70
                                        enabled: !launcher.busy
                                        onClicked: shell.installProject(modelData)
                                    }
                                }
                            }
                        }
                    }
                }
            }
            GridView {
                id: skeletonGrid
                anchors.fill: parent
                visible: page.searching && launcher.results.length === 0
                interactive: false
                clip: true
                model: 6
                cellWidth: width / Math.max(2, Math.floor(width / 232))
                cellHeight: cellWidth * 0.5625 + 162
                delegate: Rectangle {
                    width: skeletonGrid.cellWidth - 12
                    height: skeletonGrid.cellHeight - 12
                    radius: 12
                    color: Theme.surface
                    border.color: Theme.border
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 12
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            radius: 8
                            color: Theme.surface2
                        }
                        Rectangle {
                            Layout.preferredWidth: parent.width * 0.72
                            height: 12
                            radius: 3
                            color: Theme.surface2
                        }
                        Rectangle {
                            Layout.preferredWidth: parent.width * 0.45
                            height: 8
                            radius: 3
                            color: Theme.surface2
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            height: 8
                            radius: 3
                            color: Theme.surface2
                        }
                    }
                    SequentialAnimation on opacity {
                        running: page.searching && !launcher.settings.reducedMotion
                        loops: Animation.Infinite
                        NumberAnimation {
                            from: 0.55
                            to: 1
                            duration: 620
                            easing.type: Easing.InOutQuad
                        }
                        NumberAnimation {
                            from: 1
                            to: 0.55
                            duration: 620
                            easing.type: Easing.InOutQuad
                        }
                    }
                }
            }
            ColumnLayout {
                visible: page.started && !page.searching && launcher.searchError.length === 0 && launcher.results.length === 0
                anchors.centerIn: parent
                spacing: 10
                Icon {
                    name: "search"
                    size: 30
                    color: Theme.textMuted
                    Layout.alignment: Qt.AlignHCenter
                }
                Text {
                    text: "No matching projects"
                    color: Theme.text
                    font.pixelSize: 16
                    font.weight: Font.DemiBold
                    Layout.alignment: Qt.AlignHCenter
                }
                Text {
                    text: "Try another search, source, or content type."
                    color: Theme.textSecondary
                    font.pixelSize: 13
                    Layout.alignment: Qt.AlignHCenter
                }
                MButton {
                    text: "Show popular"
                    implicitHeight: 32
                    Layout.alignment: Qt.AlignHCenter
                    onClicked: {
                        searchField.clear();
                        page.categoryIndex = 0;
                        page.runSearch();
                    }
                }
            }
            ColumnLayout {
                visible: page.started && !page.searching && launcher.searchError.length > 0 && projectModel.count === 0
                anchors.centerIn: parent
                width: Math.min(430, page.width - 80)
                spacing: 10
                Icon {
                    name: "close"
                    size: 28
                    color: Theme.danger
                    Layout.alignment: Qt.AlignHCenter
                }
                Text {
                    text: "Search failed"
                    color: Theme.text
                    font.pixelSize: 16
                    font.weight: Font.DemiBold
                    Layout.alignment: Qt.AlignHCenter
                }
                Text {
                    text: launcher.searchError
                    color: Theme.textSecondary
                    font.pixelSize: 13
                    wrapMode: Text.Wrap
                    horizontalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                }
                MButton {
                    text: "Try again"
                    kind: "primary"
                    implicitHeight: 32
                    Layout.alignment: Qt.AlignHCenter
                    onClicked: page.runSearch()
                }
            }
            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 10
                visible: launcher.loadingMore
                width: loadingRow.implicitWidth + 28
                height: 34
                radius: 17
                color: Theme.surface2
                border.color: Theme.border
                RowLayout {
                    id: loadingRow
                    anchors.centerIn: parent
                    spacing: 10
                    MProgressBar {
                        Layout.preferredWidth: 70
                        indeterminate: true
                        value: 0
                    }
                    Text {
                        text: "Loading more…"
                        color: Theme.textSecondary
                        font.pixelSize: 11
                    }
                }
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 12
                visible: launcher.results.length > 0 && !launcher.searching && !launcher.loadingMore && !launcher.canLoadMore
                text: "That's everything for this search"
                color: Theme.textMuted
                font.pixelSize: 11
            }
        }
    }
    Drawer {
        id: detailDrawer
        parent: page
        edge: Qt.RightEdge
        width: Math.min(400, page.width - 40)
        height: page.height
        modal: false
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        padding: 20
        background: Rectangle {
            color: Theme.surface
            border.color: Theme.border
        }
        contentItem: ColumnLayout {
            spacing: 16
            RowLayout {
                Layout.fillWidth: true
                Rectangle {
                    width: 56
                    height: 56
                    radius: 12
                    color: Theme.surface2
                    border.color: Theme.border
                    Image {
                        anchors.fill: parent
                        anchors.margins: 6
                        source: page.detailProject.icon || ""
                        fillMode: Image.PreserveAspectFit
                        asynchronous: true
                    }
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    Text {
                        text: page.detailProject.title || "Project"
                        color: Theme.text
                        font.pixelSize: 20
                        font.weight: Font.Bold
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }
                    Text {
                        text: page.detailProject.author || page.detailProject.provider || ""
                        color: Theme.textSecondary
                        font.pixelSize: 12
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }
                MButton {
                    text: "×"
                    implicitWidth: 30
                    implicitHeight: 30
                    onClicked: detailDrawer.close()
                    Accessible.name: "Close project details"
                }
            }
            Text {
                text: page.downloadCount(page.detailProject.downloads) + " downloads · " + (page.detailProject.provider || "")
                color: Theme.textMuted
                font.pixelSize: 12
            }
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.border
            }
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                contentWidth: availableWidth
                Text {
                    width: parent.width
                    text: page.detailProject.body || page.detailProject.description || "No description provided."
                    textFormat: Text.MarkdownText
                    color: Theme.textSecondary
                    linkColor: Theme.accent
                    font.pixelSize: 14
                    lineHeight: 1.5
                    wrapMode: Text.Wrap
                    onLinkActivated: function (link) {
                        launcher.openLink(link);
                    }
                }
            }
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: versionInfo.implicitHeight + 24
                radius: 8
                color: Theme.surface2
                Text {
                    id: versionInfo
                    anchors.fill: parent
                    anchors.margins: 12
                    text: "Choose a compatible version and target instance in the install dialog. Required dependencies are included automatically."
                    color: Theme.textSecondary
                    font.pixelSize: 12
                    wrapMode: Text.Wrap
                }
            }
            RowLayout {
                Layout.fillWidth: true
                MButton {
                    text: "Project page"
                    implicitHeight: 36
                    onClicked: launcher.openLink(page.detailProject.page || "")
                }
                MButton {
                    text: "Install"
                    kind: "primary"
                    Layout.fillWidth: true
                    implicitHeight: 36
                    enabled: !launcher.busy
                    onClicked: {
                        detailDrawer.close();
                        shell.installProject(page.detailProject);
                    }
                }
            }
        }
    }
}
