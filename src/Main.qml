import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtMultimedia

ApplicationWindow {
    id: win
    width: 1020
    height: 720
    minimumWidth: 640
    minimumHeight: 480
    visible: true
    title: "sift"
    color: theme.background

    Material.theme: theme.isDark ? Material.Dark : Material.Light
    Material.background: theme.background
    Material.foreground: theme.foreground
    Material.accent: theme.accent
    Material.roundedScale: theme.radius

    property bool helpVisible: false
    property bool newFolderVisible: false
    property bool quitConfirmVisible: false
    readonly property bool modalOpen: helpVisible || newFolderVisible || quitConfirmVisible

    Shortcut {
        sequence: "?"
        context: Qt.ApplicationShortcut
        enabled: !win.newFolderVisible && !win.quitConfirmVisible
        onActivated: win.helpVisible = !win.helpVisible
    }
    Shortcut {
        sequence: "Escape"
        context: Qt.ApplicationShortcut
        enabled: win.modalOpen
        onActivated: {
            win.helpVisible = false
            win.newFolderVisible = false
            win.quitConfirmVisible = false
        }
    }
    Shortcut {
        sequence: "Q"
        context: Qt.ApplicationShortcut
        enabled: !win.modalOpen
        onActivated: {
            if (backend.pendingMoveCount > 0 && !backend.movesApplied)
                win.quitConfirmVisible = true
            else
                Qt.quit()
        }
    }
    Shortcut {
        sequence: "O"
        context: Qt.ApplicationShortcut
        enabled: !win.modalOpen
        onActivated: {
            if (backend.isSessionActive)
                backend.resetSession()
            else
                backend.requestSourceFolder()
        }
    }
    Shortcut {
        sequence: "D"
        context: Qt.ApplicationShortcut
        enabled: !win.modalOpen && !backend.isSessionActive
        onActivated: backend.requestDiscardFolder()
    }
    Shortcut {
        sequence: "Return"
        context: Qt.ApplicationShortcut
        enabled: !win.modalOpen && !backend.isSessionActive && backend.canStart
        onActivated: backend.startSifting()
    }
    Shortcut {
        sequence: "N"
        context: Qt.ApplicationShortcut
        enabled: !win.modalOpen && backend.isSessionActive
        onActivated: {
            newFolderField.text = ""
            win.newFolderVisible = true
            newFolderField.forceActiveFocus()
        }
    }
    Shortcut {
        sequences: ["Space", "Right", "L"]
        context: Qt.ApplicationShortcut
        enabled: !win.modalOpen && backend.isSessionActive && backend.hasImages && !backend.isFinished
        onActivated: backend.keep()
    }
    Shortcut {
        sequences: ["Left", "H", "Backspace", "Delete"]
        context: Qt.ApplicationShortcut
        enabled: !win.modalOpen && backend.isSessionActive && backend.hasImages && !backend.isFinished
        onActivated: backend.discard()
    }
    Shortcut {
        sequences: [StandardKey.Undo, "Z"]
        context: Qt.ApplicationShortcut
        enabled: !win.modalOpen && backend.isSessionActive && backend.canUndo
        onActivated: backend.undo()
    }
    Shortcut {
        sequences: [StandardKey.Copy]
        context: Qt.ApplicationShortcut
        enabled: !win.modalOpen && backend.isSessionActive && backend.hasImages && !backend.isFinished
        onActivated: backend.copyCurrent()
    }
    Shortcut {
        sequence: "P"
        context: Qt.ApplicationShortcut
        enabled: !win.modalOpen && backend.isSessionActive && backend.currentIsVideo && !backend.isFinished
        onActivated: player.playbackState === MediaPlayer.PlayingState ? player.pause() : player.play()
    }
    Shortcut {
        sequence: "M"
        context: Qt.ApplicationShortcut
        enabled: !win.modalOpen && backend.isSessionActive && backend.currentIsVideo && !backend.isFinished
        onActivated: audioOut.muted = !audioOut.muted
    }
    Shortcut { sequence: "1"; context: Qt.ApplicationShortcut; enabled: !win.modalOpen && backend.isSessionActive && backend.buckets.length >= 1 && !backend.isFinished; onActivated: backend.moveToFolder(0) }
    Shortcut { sequence: "2"; context: Qt.ApplicationShortcut; enabled: !win.modalOpen && backend.isSessionActive && backend.buckets.length >= 2 && !backend.isFinished; onActivated: backend.moveToFolder(1) }
    Shortcut { sequence: "3"; context: Qt.ApplicationShortcut; enabled: !win.modalOpen && backend.isSessionActive && backend.buckets.length >= 3 && !backend.isFinished; onActivated: backend.moveToFolder(2) }
    Shortcut { sequence: "4"; context: Qt.ApplicationShortcut; enabled: !win.modalOpen && backend.isSessionActive && backend.buckets.length >= 4 && !backend.isFinished; onActivated: backend.moveToFolder(3) }
    Shortcut { sequence: "5"; context: Qt.ApplicationShortcut; enabled: !win.modalOpen && backend.isSessionActive && backend.buckets.length >= 5 && !backend.isFinished; onActivated: backend.moveToFolder(4) }
    Shortcut { sequence: "6"; context: Qt.ApplicationShortcut; enabled: !win.modalOpen && backend.isSessionActive && backend.buckets.length >= 6 && !backend.isFinished; onActivated: backend.moveToFolder(5) }
    Shortcut { sequence: "7"; context: Qt.ApplicationShortcut; enabled: !win.modalOpen && backend.isSessionActive && backend.buckets.length >= 7 && !backend.isFinished; onActivated: backend.moveToFolder(6) }
    Shortcut { sequence: "8"; context: Qt.ApplicationShortcut; enabled: !win.modalOpen && backend.isSessionActive && backend.buckets.length >= 8 && !backend.isFinished; onActivated: backend.moveToFolder(7) }
    Shortcut { sequence: "9"; context: Qt.ApplicationShortcut; enabled: !win.modalOpen && backend.isSessionActive && backend.buckets.length >= 9 && !backend.isFinished; onActivated: backend.moveToFolder(8) }

    Item {
        anchors.fill: parent
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true

        // Preload the next image off-screen so stepping through photos is instant.
        Image {
            visible: false
            width: 1
            height: 1
            asynchronous: true
            cache: true
            source: backend.nextImageUrl
            sourceSize.width: win.width
            sourceSize.height: win.height
        }

        Connections {
            target: backend
            function onCopied() {
                copiedToast.visible = true
                copiedTimer.restart()
            }
        }

        Timer {
            id: copiedTimer
            interval: 1200
            onTriggered: copiedToast.visible = false
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 12

            // Top bar
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Button {
                    text: backend.isSessionActive ? ("← " + qsTr("Change folders")) : qsTr("sift")
                    flat: !backend.isSessionActive
                    enabled: backend.isSessionActive
                    onClicked: backend.resetSession()
                }

                Label {
                    visible: backend.isSessionActive
                    text: backend.sourceFolderName + "  →  " + backend.discardFolderName
                    color: theme.mutedForeground
                    font.pixelSize: 13
                    elide: Text.ElideMiddle
                    Layout.maximumWidth: 360
                }

                Button {
                    text: "+ " + qsTr("New folder")
                    visible: backend.isSessionActive
                    onClicked: {
                        newFolderField.text = ""
                        win.newFolderVisible = true
                        newFolderField.forceActiveFocus()
                    }
                }

                Item { Layout.fillWidth: true }

                RowLayout {
                    visible: backend.isSessionActive && backend.hasImages
                    spacing: 12

                    Label {
                        text: qsTr("%n remaining", "", backend.remainingCount)
                        color: theme.foreground
                        font.pixelSize: 13
                    }
                    Label {
                        text: "·"
                        color: theme.mutedForeground
                    }
                    Label {
                        text: qsTr("%n kept", "", backend.keptCount)
                        color: theme.keepColor
                        font.pixelSize: 13
                    }
                    Label {
                        text: qsTr("%n discarded", "", backend.discardedCount)
                        color: theme.discardColor
                        font.pixelSize: 13
                    }
                    Label {
                        visible: backend.movedCount > 0
                        text: qsTr("%n moved", "", backend.movedCount)
                        color: theme.accent
                        font.pixelSize: 13
                    }
                }

                ToolButton {
                    text: "?"
                    font.pixelSize: 15
                    font.bold: true
                    onClicked: win.helpVisible = !win.helpVisible
                }
            }

            // Error banner
            Rectangle {
                Layout.fillWidth: true
                visible: backend.errorMessage.length > 0
                color: theme.surface
                border.color: theme.discardColor
                border.width: 1
                radius: Math.min(4, theme.radius)
                implicitHeight: errorRow.implicitHeight + 16

                RowLayout {
                    id: errorRow
                    anchors.fill: parent
                    anchors.margins: 8
                    Label {
                        Layout.fillWidth: true
                        text: backend.errorMessage
                        color: theme.discardColor
                        wrapMode: Text.Wrap
                    }
                    ToolButton {
                        text: "×"
                        onClicked: backend.clearError()
                    }
                }
            }

            // Main stage
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                // Folder setup screen (before session starts)
                ColumnLayout {
                    anchors.centerIn: parent
                    width: Math.min(580, parent.width - 32)
                    spacing: 16
                    visible: !backend.isSessionActive

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("Choose folders to start")
                        color: theme.foreground
                        font.pixelSize: 22
                        font.bold: true
                    }

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.Wrap
                        text: qsTr("Discarded photos are moved to the destination folder, never deleted.")
                        color: theme.mutedForeground
                        font.pixelSize: 13
                    }

                    // Source folder selector card
                    Rectangle {
                        Layout.fillWidth: true
                        color: theme.surface
                        border.color: theme.border
                        border.width: 1
                        radius: theme.radius
                        implicitHeight: srcCol.implicitHeight + 28

                        ColumnLayout {
                            id: srcCol
                            anchors.fill: parent
                            anchors.margins: 14
                            spacing: 8

                            Label {
                                text: qsTr("Source folder")
                                color: theme.mutedForeground
                                font.pixelSize: 12
                                font.bold: true
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 10
                                Label {
                                    Layout.fillWidth: true
                                    text: backend.sourceFolderPath.length > 0
                                          ? backend.sourceFolderPath
                                          : qsTr("No folder selected")
                                    color: backend.sourceFolderPath.length > 0 ? theme.foreground : theme.mutedForeground
                                    elide: Text.ElideMiddle
                                    font.pixelSize: 14
                                }
                                Button {
                                    text: qsTr("Choose") + " (O)"
                                    onClicked: backend.requestSourceFolder()
                                }
                            }
                        }
                    }

                    // Discard destination selector card
                    Rectangle {
                        Layout.fillWidth: true
                        color: theme.surface
                        border.color: theme.border
                        border.width: 1
                        radius: theme.radius
                        implicitHeight: dstCol.implicitHeight + 28

                        ColumnLayout {
                            id: dstCol
                            anchors.fill: parent
                            anchors.margins: 14
                            spacing: 8

                            Label {
                                text: qsTr("Discard destination")
                                color: theme.mutedForeground
                                font.pixelSize: 12
                                font.bold: true
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 10
                                Label {
                                    Layout.fillWidth: true
                                    text: backend.discardFolderPath.length > 0
                                          ? backend.discardFolderPath
                                          : qsTr("No folder selected")
                                    color: backend.discardFolderPath.length > 0 ? theme.foreground : theme.mutedForeground
                                    elide: Text.ElideMiddle
                                    font.pixelSize: 14
                                }
                                Button {
                                    text: qsTr("Choose") + " (D)"
                                    onClicked: backend.requestDiscardFolder()
                                }
                            }
                        }
                    }

                    Button {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.topMargin: 4
                        text: qsTr("Start sifting")
                        highlighted: true
                        enabled: backend.canStart
                        onClicked: backend.startSifting()
                    }
                }

                // Empty folder state (session active, 0 images)
                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 12
                    visible: backend.isSessionActive && !backend.hasImages

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("No images found in this folder.")
                        color: theme.foreground
                        font.pixelSize: 18
                    }
                    Button {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("Change folders")
                        highlighted: true
                        onClicked: backend.resetSession()
                    }
                }

                // Finished state
                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 14
                    visible: backend.isFinished

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: backend.movesApplied || backend.pendingMoveCount === 0
                              ? qsTr("All done")
                              : qsTr("Review complete")
                        color: theme.accent
                        font.pixelSize: 24
                        font.bold: true
                    }
                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("%n kept", "", backend.keptCount) + "  ·  "
                              + qsTr("%n discarded", "", backend.discardedCount)
                              + (backend.movedCount > 0 ? "  ·  " + qsTr("%n moved", "", backend.movedCount) : "")
                        color: theme.foreground
                        font.pixelSize: 14
                    }
                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        visible: !backend.movesApplied && backend.pendingMoveCount > 0
                        text: qsTr("No files have been moved yet. Click below to move them now.")
                        color: theme.mutedForeground
                        font.pixelSize: 13
                    }
                    RowLayout {
                        Layout.alignment: Qt.AlignHCenter
                        spacing: 10
                        Button {
                            text: qsTr("Undo")
                            visible: !backend.movesApplied
                            enabled: backend.canUndo
                            onClicked: backend.undo()
                        }
                        Button {
                            text: qsTr("Move %n files", "", backend.pendingMoveCount)
                            highlighted: true
                            visible: !backend.movesApplied && backend.pendingMoveCount > 0
                            onClicked: backend.applyMoves()
                        }
                        Button {
                            text: qsTr("Change folders")
                            highlighted: backend.movesApplied || backend.pendingMoveCount === 0
                            onClicked: backend.resetSession()
                        }
                    }
                }

                // Image card with drag/swipe support
                Rectangle {
                    id: card
                    anchors.fill: parent
                    visible: backend.isSessionActive && backend.hasImages && !backend.isFinished
                    color: theme.surface
                    border.color: theme.border
                    border.width: 1
                    radius: theme.radius
                    clip: true

                    property real dragOffsetX: 0
                    readonly property real swipeThreshold: 110

                    Image {
                        id: photo
                        anchors.fill: parent
                        anchors.margins: 12
                        visible: !backend.currentIsVideo
                        fillMode: Image.PreserveAspectFit
                        asynchronous: true
                        cache: true
                        smooth: true
                        source: backend.currentIsVideo ? "" : backend.currentUrl
                        x: 12 + card.dragOffsetX
                        rotation: card.dragOffsetX * 0.02

                        Behavior on x {
                            enabled: !dragArea.pressed
                            NumberAnimation { duration: 140; easing.type: Easing.OutCubic }
                        }
                        Behavior on rotation {
                            enabled: !dragArea.pressed
                            NumberAnimation { duration: 140; easing.type: Easing.OutCubic }
                        }
                    }

                    MediaPlayer {
                        id: player
                        source: backend.currentIsVideo && !backend.isFinished ? backend.currentUrl : ""
                        loops: MediaPlayer.Infinite
                        audioOutput: AudioOutput { id: audioOut; muted: false }
                        videoOutput: videoOut
                        onSourceChanged: {
                            if (source.toString().length > 0)
                                play()
                            else
                                stop()
                        }
                    }

                    VideoOutput {
                        id: videoOut
                        anchors.fill: parent
                        anchors.margins: 12
                        visible: backend.currentIsVideo
                        fillMode: VideoOutput.PreserveAspectFit
                        x: 12 + card.dragOffsetX
                        rotation: card.dragOffsetX * 0.02
                    }

                    // Video controls pill
                    Row {
                        anchors.bottom: parent.bottom
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottomMargin: 20
                        spacing: 8
                        z: 2
                        visible: backend.currentIsVideo

                        Button {
                            text: player.playbackState === MediaPlayer.PlayingState ? qsTr("Pause") + " (P)" : qsTr("Play") + " (P)"
                            onClicked: player.playbackState === MediaPlayer.PlayingState ? player.pause() : player.play()
                        }
                        Button {
                            text: audioOut.muted ? qsTr("Unmute") + " (M)" : qsTr("Mute") + " (M)"
                            onClicked: audioOut.muted = !audioOut.muted
                        }
                    }

                    // Copied toast badge
                    Rectangle {
                        id: copiedToast
                        visible: false
                        anchors.top: parent.top
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.topMargin: 20
                        z: 3
                        color: theme.elevated
                        border.color: theme.accent
                        border.width: 1
                        radius: Math.min(4, theme.radius)
                        implicitWidth: copiedLabel.implicitWidth + 24
                        implicitHeight: copiedLabel.implicitHeight + 12

                        Label {
                            id: copiedLabel
                            anchors.centerIn: parent
                            text: qsTr("Copied")
                            color: theme.accent
                            font.pixelSize: 13
                            font.bold: true
                        }
                    }

                    // Swipe visual indicator
                    Rectangle {
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.margins: 24
                        visible: card.dragOffsetX > 30
                        opacity: Math.min(1.0, (card.dragOffsetX - 30) / 80.0)
                        color: theme.elevated
                        border.color: theme.keepColor
                        border.width: 2
                        radius: Math.min(4, theme.radius)
                        implicitWidth: keepBadge.implicitWidth + 24
                        implicitHeight: keepBadge.implicitHeight + 12
                        Label {
                            id: keepBadge
                            anchors.centerIn: parent
                            text: qsTr("Keep").toUpperCase()
                            color: theme.keepColor
                            font.pixelSize: 16
                            font.bold: true
                        }
                    }

                    Rectangle {
                        anchors.top: parent.top
                        anchors.right: parent.right
                        anchors.margins: 24
                        visible: card.dragOffsetX < -30
                        opacity: Math.min(1.0, (-card.dragOffsetX - 30) / 80.0)
                        color: theme.elevated
                        border.color: theme.discardColor
                        border.width: 2
                        radius: Math.min(4, theme.radius)
                        implicitWidth: discardBadge.implicitWidth + 24
                        implicitHeight: discardBadge.implicitHeight + 12
                        Label {
                            id: discardBadge
                            anchors.centerIn: parent
                            text: qsTr("Discard").toUpperCase()
                            color: theme.discardColor
                            font.pixelSize: 16
                            font.bold: true
                        }
                    }

                    MouseArea {
                        id: dragArea
                        anchors.fill: parent
                        property real startX: 0
                        onPressed: (mouse) => { startX = mouse.x }
                        onPositionChanged: (mouse) => {
                            if (pressed)
                                card.dragOffsetX = mouse.x - startX
                        }
                        onReleased: {
                            if (card.dragOffsetX <= -card.swipeThreshold) {
                                card.dragOffsetX = 0
                                backend.discard()
                            } else if (card.dragOffsetX >= card.swipeThreshold) {
                                card.dragOffsetX = 0
                                backend.keep()
                            } else {
                                card.dragOffsetX = 0
                            }
                        }
                        onCanceled: card.dragOffsetX = 0
                    }
                }
            }

            // Custom subfolder quick-move bar
            Flow {
                Layout.fillWidth: true
                spacing: 8
                visible: backend.isSessionActive && backend.hasImages && !backend.isFinished && backend.buckets.length > 0

                Repeater {
                    model: backend.buckets
                    delegate: Button {
                        required property string modelData
                        required property int index
                        text: (index < 9 ? (index + 1) + " · " : "") + modelData
                        onClicked: backend.moveToFolder(index)
                    }
                }
            }

            // Bottom bar
            RowLayout {
                Layout.fillWidth: true
                visible: backend.isSessionActive && backend.hasImages && !backend.isFinished
                spacing: 12

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Label {
                        text: backend.currentName
                        color: theme.foreground
                        font.pixelSize: 14
                        elide: Text.ElideMiddle
                        Layout.fillWidth: true
                    }
                    Label {
                        text: [backend.currentResolution, backend.currentSizeText].filter(Boolean).join("  ·  ")
                        color: theme.mutedForeground
                        font.pixelSize: 12
                    }
                }

                Button {
                    text: "← " + qsTr("Discard")
                    onClicked: backend.discard()
                }
                Button {
                    text: qsTr("Undo")
                    enabled: backend.canUndo
                    onClicked: backend.undo()
                }
                Button {
                    text: qsTr("Keep") + " →"
                    highlighted: true
                    onClicked: backend.keep()
                }
            }
        }

        // New folder dialog overlay
        Rectangle {
            anchors.fill: parent
            visible: win.newFolderVisible
            color: "#aa000000"

            MouseArea { anchors.fill: parent; onClicked: win.newFolderVisible = false }

            Rectangle {
                anchors.centerIn: parent
                width: Math.min(420, parent.width - 48)
                implicitHeight: folderCol.implicitHeight + 36
                color: theme.surface
                border.color: theme.border
                border.width: 1
                radius: theme.radius

                MouseArea { anchors.fill: parent }

                ColumnLayout {
                    id: folderCol
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 12

                    Label {
                        text: qsTr("New folder")
                        color: theme.foreground
                        font.pixelSize: 16
                        font.bold: true
                    }

                    TextField {
                        id: newFolderField
                        Layout.fillWidth: true
                        placeholderText: qsTr("Folder name")
                        onAccepted: {
                            if (backend.createFolder(text))
                                win.newFolderVisible = false
                        }
                    }

                    RowLayout {
                        Layout.alignment: Qt.AlignRight
                        spacing: 8
                        Button {
                            text: qsTr("Cancel")
                            onClicked: win.newFolderVisible = false
                        }
                        Button {
                            text: qsTr("Create")
                            highlighted: true
                            onClicked: {
                                if (backend.createFolder(newFolderField.text))
                                    win.newFolderVisible = false
                            }
                        }
                    }
                }
            }
        }

        // Quit confirmation overlay when moves are pending
        Rectangle {
            anchors.fill: parent
            visible: win.quitConfirmVisible
            color: "#aa000000"

            MouseArea { anchors.fill: parent; onClicked: win.quitConfirmVisible = false }

            Rectangle {
                anchors.centerIn: parent
                width: Math.min(440, parent.width - 48)
                implicitHeight: quitCol.implicitHeight + 36
                color: theme.surface
                border.color: theme.border
                border.width: 1
                radius: theme.radius

                MouseArea { anchors.fill: parent }

                ColumnLayout {
                    id: quitCol
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 12

                    Label {
                        text: qsTr("Unapplied changes")
                        color: theme.foreground
                        font.pixelSize: 16
                        font.bold: true
                    }

                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        text: qsTr("Pending moves have not been applied yet. Quit without moving files?")
                        color: theme.mutedForeground
                        font.pixelSize: 13
                    }

                    RowLayout {
                        Layout.alignment: Qt.AlignRight
                        spacing: 8
                        Button {
                            text: qsTr("Cancel")
                            onClicked: win.quitConfirmVisible = false
                        }
                        Button {
                            text: qsTr("Quit")
                            highlighted: true
                            onClicked: Qt.quit()
                        }
                    }
                }
            }
        }

        // Shortcuts overlay
        Rectangle {
            anchors.fill: parent
            visible: win.helpVisible
            color: "#bb000000"

            MouseArea { anchors.fill: parent; onClicked: win.helpVisible = false }

            Rectangle {
                anchors.centerIn: parent
                width: Math.min(480, parent.width - 48)
                implicitHeight: helpCol.implicitHeight + 40
                color: theme.surface
                border.color: theme.border
                border.width: 1
                radius: theme.radius

                MouseArea { anchors.fill: parent }

                ColumnLayout {
                    id: helpCol
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 10

                    Label {
                        text: qsTr("Shortcuts")
                        color: theme.accent
                        font.pixelSize: 18
                        font.bold: true
                    }

                    GridLayout {
                        columns: 2
                        columnSpacing: 20
                        rowSpacing: 8

                        Label { text: "Space / → / L"; color: theme.foreground; font.bold: true }
                        Label { text: qsTr("Keep photo"); color: theme.mutedForeground }

                        Label { text: "← / H / Backspace"; color: theme.foreground; font.bold: true }
                        Label { text: qsTr("Move to discard folder"); color: theme.mutedForeground }

                        Label { text: "Ctrl+Z / Z"; color: theme.foreground; font.bold: true }
                        Label { text: qsTr("Undo last action"); color: theme.mutedForeground }

                        Label { text: "Ctrl+C"; color: theme.foreground; font.bold: true }
                        Label { text: qsTr("Copy current file"); color: theme.mutedForeground }

                        Label { text: "P / M"; color: theme.foreground; font.bold: true }
                        Label { text: qsTr("Play/Pause or Mute video"); color: theme.mutedForeground }

                        Label { text: "N"; color: theme.foreground; font.bold: true }
                        Label { text: qsTr("Create subfolder"); color: theme.mutedForeground }

                        Label { text: "1–9"; color: theme.foreground; font.bold: true }
                        Label { text: qsTr("Move to subfolder 1-9"); color: theme.mutedForeground }

                        Label { text: "O / D"; color: theme.foreground; font.bold: true }
                        Label { text: qsTr("Choose source or discard folder"); color: theme.mutedForeground }

                        Label { text: "?"; color: theme.foreground; font.bold: true }
                        Label { text: qsTr("Toggle shortcuts"); color: theme.mutedForeground }

                        Label { text: "Q"; color: theme.foreground; font.bold: true }
                        Label { text: qsTr("Quit"); color: theme.mutedForeground }
                    }
                }
            }
        }
    }
}
