//*******************************************************************************
// ZX Tape Reviver
//-----------------
//
// Author: Leonid Golouz
// E-mail: lgolouz@list.ru
//*******************************************************************************

import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.3

import WaveformControl 1.0
import com.enums.zxtapereviver 1.0
import com.models.zxtapereviver 1.0
import com.core.zxtapereviver 1.0

ApplicationWindow {
    id: mainWindow

    readonly property int mainAreaWidth: width * 0.75
    property var suspiciousPoints: SuspiciousPointsModel.suspiciousPoints

    visible: true
    width: 1600
    height: 800
    title: qsTr("ZX Tape Reviver")

    function getWaveShiftIndex(wfWidth, wfXScale) {
        return wfWidth * wfXScale / 2;
    }

    function getSelectedWaveform() {
        return channelsComboBox.currentIndex == 0 ? waveformControlCh0 : waveformControlCh1;
    }

    function restoreWaveformView() {
        waveformControlCh0.xScaleFactor = 1;
        waveformControlCh0.yScaleFactor = 80000;
        waveformControlCh0.wavePos = 0;

        waveformControlCh1.xScaleFactor = 1;
        waveformControlCh1.yScaleFactor = 80000;
        waveformControlCh1.wavePos = 0;
    }

    menuBar: MenuBar {
        Menu {
            title: "File"

            MenuItem {
                text: "Open WAV file..."
                onTriggered: {
                    console.log("Opening WAV file");
                    openFileDialog.isWavOpening = true;
                    openFileDialog.open();
                }
            }

            MenuItem {
                text: "Open Waveform file..."
                onTriggered:  {
                    console.log("Opening Waveform file");
                    openFileDialog.isWavOpening = false;
                    openFileDialog.open();
                }
            }

            MenuSeparator { }

            Menu {
                title: "Save"

                Menu {
                    title: "Parsed"

                    MenuItem {
                        text: "Left channel..."

                        onTriggered: {
                            saveFileDialog.saveParsed = true;
                            saveFileDialog.channelNumber = 0;
                            saveFileDialog.open();
                        }
                    }

                    MenuItem {
                        text: "Right channel..."

                        onTriggered: {
                            saveFileDialog.saveParsed = true;
                            saveFileDialog.channelNumber = 1;
                            saveFileDialog.open();
                        }
                    }
                }

                MenuItem {
                    text: "Waveform..."

                    onTriggered: {
                        saveFileDialog.saveParsed = false;
                        saveFileDialog.open();
                    }
                }
            }

            MenuSeparator { }

            MenuItem {
                text: "Exit"
                onTriggered: {
                    mainWindow.close();
                }
            }
        }

        Menu {
            title: "Waveform"

            MenuItem {
                text: "Restore view"
                onTriggered: {
                    restoreWaveformView();
                }
            }

            MenuItem {
                text: "Reparse"
            }
        }
    }

    FileDialog {
        id: openFileDialog

        property bool isWavOpening: true

        title: "Please choose WAV file"
        selectMultiple: false
        sidebarVisible: true
        defaultSuffix: isWavOpening ? "wav" : "wfm"
        nameFilters: isWavOpening ? [ "WAV files (*.wav)" ] : [ "Waveform files (*.wfm)" ]

        onAccepted: {
            var filetype = isWavOpening ? "WAV" : "Waveform";
            console.log("Selected %1 file: ".arg(filetype) + openFileDialog.fileUrl);
            var res = (isWavOpening
                        ? FileWorkerModel.openWavFileByUrl(openFileDialog.fileUrl)
                        : FileWorkerModel.openWaveformFileByUrl(openFileDialog.fileUrl));
            console.log("Open %1 file result: ".arg(filetype) + res);
            if (res === 0) {
                if (isWavOpening) {
                    SuspiciousPointsModel.clearSuspiciousPoints();
                }
                restoreWaveformView();
            }
        }

        onRejected: {
            console.log("No WAV file selected");
        }
    }

    FileDialog {
        id: saveFileDialog

        property bool saveParsed: true
        property int channelNumber: 0

        title: saveParsed ? "Save TAP file..." : "Save WFM file..."
        selectExisting: false
        selectMultiple: false
        sidebarVisible: true
        defaultSuffix: saveParsed ? "tap" : "wfm"
        nameFilters: saveParsed ? [ "TAP tape files (*.tap)" ] : [ "WFM waveform files (*.wfm)" ]

        onAccepted: {
            if (saveParsed) {
                if (channelNumber == 0) {
                    waveformControlCh0.saveTap(saveFileDialog.fileUrl);
                }
                else {
                    waveformControlCh1.saveTap(saveFileDialog.fileUrl);
                }
                console.log("Tap saved: " + saveFileDialog.fileUrl)
            }
            else {
                FileWorkerModel.saveWaveformFileByUrl(saveFileDialog.fileUrl);
                console.log("Waveform saved: " + saveFileDialog.fileUrl);
            }
        }
    }

    Connections {
        target: FileWorkerModel
        function onWavFileNameChanged() {
            waveformControlCh0.reparse();
            waveformControlCh1.reparse();
        }
    }

    Rectangle {
        id: mainArea

        anchors {
            left: parent.left
            top: parent.top
            bottom: parent.bottom
        }
        width: mainAreaWidth
        color: "black"

        readonly property int spacerHeight: ~~(parent.height * 0.0075);

        WaveformControl {
            id: waveformControlCh0

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: vZoomInButton.left
            anchors.rightMargin: 5

            channelNumber: 0
            width: parent.width - (parent.width * 0.11)
            height: parent.height - parent.height / 2 - parent.spacerHeight / 2

            onDoubleClick: {
                SuspiciousPointsModel.addSuspiciousPoint(idx);
            }
        }

        WaveformControl {
            id: waveformControlCh1

            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: vZoomInButton.left
            anchors.rightMargin: 5

            channelNumber: 1
            width: parent.width - (parent.width * 0.11)
            height: waveformControlCh0.height

            onDoubleClick: {
                SuspiciousPointsModel.addSuspiciousPoint(idx);
            }
        }

        Button {
            id: vZoomInButton

            text: "Vertical Zoom IN"
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.rightMargin: 5
            anchors.topMargin: 5
            width: hZoomOutButton.width

            onClicked: {
                var yfactor = waveformControlCh0.yScaleFactor;
                if (yfactor > 1000) {
                    waveformControlCh0.yScaleFactor = yfactor / 2;
                }

                yfactor = waveformControlCh1.yScaleFactor;
                if (yfactor > 1000) {
                    waveformControlCh1.yScaleFactor = yfactor / 2;
                }
            }
        }

        Button {
            id: vZoomOutButton

            text: "Vertical Zoom OUT"
            anchors.top: vZoomInButton.bottom
            anchors.right: parent.right
            anchors.rightMargin: 5
            anchors.topMargin: 5
            width: hZoomOutButton.width

            onClicked: {
                var yfactor = waveformControlCh0.yScaleFactor;
                if (yfactor < 320000) {
                    waveformControlCh0.yScaleFactor = yfactor * 2;
                }

                yfactor = waveformControlCh1.yScaleFactor;
                if (yfactor < 320000) {
                    waveformControlCh1.yScaleFactor = yfactor * 2;
                }
            }
        }

        Button {
            id: hZoomInButton

            text: "Horizontal Zoom IN"
            anchors.top: vZoomOutButton.bottom
            anchors.right: parent.right
            anchors.rightMargin: 5
            anchors.topMargin: 10
            width: hZoomOutButton.width

            onClicked: {
                waveformControlCh0.xScaleFactor = waveformControlCh0.xScaleFactor / 2;
                waveformControlCh1.xScaleFactor = waveformControlCh1.xScaleFactor / 2;
            }
        }

        Button {
            id: hZoomOutButton

            text: "Horizontal Zoom OUT"
            anchors.top: hZoomInButton.bottom
            anchors.right: parent.right
            anchors.rightMargin: 5
            anchors.topMargin: 5

            onClicked: {
                waveformControlCh0.xScaleFactor = waveformControlCh0.xScaleFactor * 2;
                waveformControlCh1.xScaleFactor = waveformControlCh1.xScaleFactor * 2;
            }
        }

        Button {
            id: shiftWaveRight

            text: "<<"
            anchors.bottom: waveformControlCh0.bottom
            anchors.left: hZoomOutButton.left
            width: 40

            onClicked: {
                waveformControlCh0.wavePos -= getWaveShiftIndex(waveformControlCh0.width, waveformControlCh0.xScaleFactor);
                waveformControlCh1.wavePos -= getWaveShiftIndex(waveformControlCh1.width, waveformControlCh1.xScaleFactor);
            }
        }

        Button {
            id: shiftWaveLeft

            text: ">>"
            anchors.bottom: shiftWaveRight.bottom
            anchors.right: parent.right
            anchors.rightMargin: 5
            width: 40

            onClicked: {
                waveformControlCh0.wavePos += getWaveShiftIndex(waveformControlCh0.width, waveformControlCh0.xScaleFactor);
                waveformControlCh1.wavePos += getWaveShiftIndex(waveformControlCh1.width, waveformControlCh1.xScaleFactor);
            }
        }

        Button {
            id: reparseButton

            text: "Reparse"
            anchors.top: shiftWaveLeft.bottom
            anchors.right: parent.right
            anchors.rightMargin: 5
            anchors.topMargin: 15
            width: hZoomOutButton.width

            onClicked: {
                var parsedDataViewIdx = parsedDataView.currentRow;
                waveformControlCh0.reparse();
                waveformControlCh1.reparse();
                parsedDataView.selection.select(parsedDataViewIdx);
                parsedDataView.currentRow = parsedDataViewIdx;
            }
        }

        Button {
            id: saveParsedDataButton

            text: "Save parsed"
            anchors.top: reparseButton.bottom
            anchors.right: parent.right
            anchors.rightMargin: 5
            anchors.topMargin: 15
            width: hZoomOutButton.width

            onClicked: {
                waveformControlCh0.saveTap();
                //waveformControlCh1.saveTap();
            }
        }

        Button {
            id: saveWaveformButton

            text: "Save waveform"
            anchors.top: saveParsedDataButton.bottom
            anchors.right: parent.right
            anchors.rightMargin: 5
            anchors.topMargin: 15
            width: hZoomOutButton.width

            onClicked: {
                waveformControlCh0.saveWaveform();
                //waveformControlCh1.saveWaveform();
            }
        }

        Button {
            id: repairRestoreButton

            text: "%1 waveform".arg(getSelectedWaveform().isWaveformRepaired ? "Restore" : "Repair")
            anchors.top: saveParsedDataButton.bottom
            anchors.right: parent.right
            anchors.rightMargin: 5
            anchors.topMargin: 15
            width: hZoomOutButton.width

            onClicked: {
                if (getSelectedWaveform().isWaveformRepaired) {
                    getSelectedWaveform().restoreWaveform();
                }
                else {
                    getSelectedWaveform().repairWaveform();
                }
            }
        }

        Button {
            id: shiftWaveform

            text: "Shift waveform"
            anchors {
                top: repairRestoreButton.bottom
                topMargin: 5
                right: parent.right
                rightMargin: 5
            }
            width: hZoomOutButton.width

            onClicked: {
                getSelectedWaveform().shiftWaveform();
            }
        }

        Button {
            id: gotoAddressButton

            text: "Goto address..."
            anchors {
                top: shiftWaveform.bottom
                topMargin: 5
                right: parent.right
                rightMargin: 5
            }
            width: hZoomOutButton.width

            onClicked: {
                gotoAddressDialog.open();
            }
        }

        Button {
            id: selectionModeToggleButton

            text: "Selection mode"
            anchors.right: parent.right
            anchors.bottom: waveformControlCh1.bottom
            anchors.bottomMargin: 5
            anchors.rightMargin: 5
            width: hZoomOutButton.width
            checkable: true
            visible: !measurementModeToggleButton.checked

            onCheckedChanged: {
                waveformControlCh0.operationMode = waveformControlCh1.operationMode = checked ? WaveformControlOperationModes.WaveformSelectionMode : WaveformControlOperationModes.WaveformRepairMode;
            }
        }

        Button {
            id: measurementModeToggleButton

            text: "Measurement mode"
            anchors {
                right: parent.right
                rightMargin: 5
                bottom: selectionModeToggleButton.top
                bottomMargin: 5
            }
            width: hZoomOutButton.width
            checkable: true
            visible: !selectionModeToggleButton.checked

            onCheckedChanged: {
                waveformControlCh0.operationMode = waveformControlCh1.operationMode = checked ? WaveformControlOperationModes.WaveformMeasurementMode : WaveformControlOperationModes.WaveformRepairMode;
            }
        }

        states: State {
            when: measurementModeToggleButton.checked
            AnchorChanges {
                target: measurementModeToggleButton
                anchors.bottom: waveformControlCh1.bottom
            }
        }

        Button {
            id: copyFromRigthToLeftChannel

            anchors.right: parent.right
            anchors.bottom: selectionModeToggleButton.top
            anchors.bottomMargin: 15
            anchors.rightMargin: 5
            width: hZoomOutButton.width

            text: "Copy from R to L (­▲)"
            visible: selectionModeToggleButton.checked
            onClicked: {
                waveformControlCh1.copySelectedToAnotherChannel();
                waveformControlCh0.update();
            }
        }

        Button {
            id: copyFromLeftToRightChannel

            anchors.right: parent.right
            anchors.bottom: copyFromRigthToLeftChannel.top
            anchors.bottomMargin: 5
            anchors.rightMargin: 5
            width: hZoomOutButton.width

            text: "Copy from L to R (▼)"
            visible: selectionModeToggleButton.checked
            onClicked: {
                waveformControlCh0.copySelectedToAnotherChannel();
                waveformControlCh1.update();
            }
        }
    }

    Rectangle {
        id: rightArea

        anchors {
            left: mainArea.right
            top: parent.top
            bottom: parent.bottom
        }

        width: parent.width - mainAreaWidth

        color: "transparent"

        ComboBox {
            id: channelsComboBox

            model: ["Left Channel", "Right Channel"]
            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
            }
        }

        Button {
            id: toBlockBeginningButton

            text: "<< To the beginning of the block"
            anchors {
                top: channelsComboBox.bottom
                left: parent.left
                rightMargin: 2
                topMargin: 2
                bottomMargin: 2
            }
            width: parent.width / 2

            onClicked: {
                if (parsedDataView.currentRow !== -1) {
                    var idx = WaveformParser.getBlockDataStart(channelsComboBox.currentIndex, parsedDataView.currentRow) - getWaveShiftIndex(waveformControlCh0.width, waveformControlCh0.xScaleFactor);
                    if (idx < 0) {
                        idx = 0;
                    }

                    waveformControlCh0.wavePos = waveformControlCh1.wavePos = idx;
                }
            }
        }

        Button {
            id: toBlockEndButton

            text: "To the end of the block >>"
            anchors {
                top: channelsComboBox.bottom
                right: parent.right
                left: toBlockBeginningButton.right
                leftMargin: 2
                topMargin: 2
                bottomMargin: 2
            }

            onClicked: {
                if (parsedDataView.currentRow !== -1) {
                    var idx = WaveformParser.getBlockDataEnd(channelsComboBox.currentIndex, parsedDataView.currentRow) - getWaveShiftIndex(waveformControlCh0.width, waveformControlCh0.xScaleFactor);
                    if (idx < 0) {
                        idx = 0;
                    }

                    waveformControlCh0.wavePos = waveformControlCh1.wavePos = idx;
                }
            }
        }

        TableView {
            id: parsedDataView

            height: parent.height * 0.4
            anchors {
                top: toBlockEndButton.bottom
                left: parent.left
                right: parent.right
                topMargin: 2
            }

            TableViewColumn {
                title: "#"
                width: rightArea.width * 0.07
                role: "blockNumber"
            }

            TableViewColumn {
                title: "Type"
                width: rightArea.width * 0.23
                role: "blockType"
            }

            TableViewColumn {
                title: "Name"
                width: rightArea.width * 0.3
                role: "blockName"
            }

            TableViewColumn {
                title: "Size (to be read)"
                width: rightArea.width * 0.25
                role: "blockSize"
            }

            TableViewColumn {
                title: "Status"
                width: rightArea.width * 0.15
                role: "blockStatus"
            }

            selectionMode: SelectionMode.SingleSelection
            model: channelsComboBox.currentIndex === 0 ? WaveformParser.parsedChannel0 : WaveformParser.parsedChannel1
            itemDelegate: Text {
                text: styleData.value
                color: modelData.state === 0 ? "black" : "red"
            }
        }

        Button {
            id: gotoPointButton

            anchors {
                top: parsedDataView.bottom
                left: parent.left
                rightMargin: 2
                topMargin: 2
            }
            width: parent.width / 2
            text: "Goto Suspicious point"

            onClicked: {
                if (suspiciousPointsView.currentRow < 0 || suspiciousPointsView.currentRow >= SuspiciousPointsModel.size) {
                    return;
                }

                var idx = SuspiciousPointsModel.getSuspiciousPoint(suspiciousPointsView.currentRow) - getWaveShiftIndex(waveformControlCh0.width, waveformControlCh0.xScaleFactor);
                if (idx < 0) {
                    idx = 0;
                }
                console.log("Go to point: " + idx);

                waveformControlCh0.wavePos = waveformControlCh1.wavePos = idx;
            }
        }

        Button {
            id: removePointButton

            anchors {
                top: parsedDataView.bottom
                left: gotoPointButton.right
                right: parent.right
                leftMargin: 2
                topMargin: 2
            }

            text: "Remove Suspicious point"

            onClicked: {
                if (suspiciousPointsView.currentRow >= 0 && suspiciousPointsView.currentRow < SuspiciousPointsModel.size) {
                    console.log("Removing suspicious point: " + SuspiciousPointsModel.getSuspiciousPoint(suspiciousPointsView.currentRow));
                    SuspiciousPointsModel.removeSuspiciousPoint(suspiciousPointsView.currentRow);
                }
            }
        }

        TableView {
            id: suspiciousPointsView

            anchors {
                top: gotoPointButton.bottom
                bottom: parent.bottom
                left: parent.left
                right: parent.right
                topMargin: 2
            }

            selectionMode: SelectionMode.SingleSelection
            model: suspiciousPoints
            itemDelegate: Text {
                text: styleData.column === 0 ? styleData.row + 1 : styleData.value
            }

            TableViewColumn {
                title: "#"
                width: rightArea.width * 0.1
            }

            TableViewColumn {
                title: "Position"
                width: rightArea.width * 0.9
            }
        }
    }

    GoToAddress {
        id: gotoAddressDialog
        onGotoAddress: {
            console.log("Goto address: " + adr);
            var pos = WaveformParser.getPositionByAddress(channelsComboBox.currentIndex, parsedDataView.currentRow, adr);
            if (pos !== 0) {
                var idx = pos - getWaveShiftIndex(waveformControlCh0.width, waveformControlCh0.xScaleFactor);
                if (idx < 0) {
                    idx = 0;
                }

                waveformControlCh0.wavePos = waveformControlCh1.wavePos = idx;
            }
        }
    }

    Frequency {
        id: frequencyDialog
        Component.onCompleted: {
            var func = function(fr) { frequency = fr; frequencyDialog.open(); };
            waveformControlCh0.frequency.connect(func);
            waveformControlCh1.frequency.connect(func);
        }
    }
}
