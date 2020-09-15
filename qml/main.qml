//*******************************************************************************
// ZX Tape Reviver
//-----------------
//
// Author: Leonid Golouz
// E-mail: lgolouz@list.ru
//*******************************************************************************

import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs 1.3

import WaveformControl 1.0
import com.enums.zxtapereviver 1.0
import com.models.zxtapereviver 1.0

ApplicationWindow {
    id: mainWindow

    visible: true
    width: 1200
    height: 800
    title: qsTr("ZX Tape Reviver")

    menuBar: MenuBar {
        Menu {
            title: "File"

            MenuItem {
                text: "Open WAV file..."
                onTriggered: {
                    console.log("Opening WAV file");
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
    }

    FileDialog {
        id: openFileDialog

        title: "Please choose WAV file"
        selectMultiple: false
        sidebarVisible: true
        defaultSuffix: "wav"
        nameFilters: [ "WAV files (*.wav)" ]

        onAccepted: {
            console.log("Selected WAV file: " + openFileDialog.fileUrl);
            console.log("Open WAV file result: " + FileWorkerModel.openWavFileByUrl(openFileDialog.fileUrl));
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
                    waveformControlCh0.saveTap();
                }
                else {
                    waveformControlCh1.saveTap();
                }
            }
            else {
                //wavReader.saveWaveform();
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
        anchors.fill: parent
        color: "black"

        WaveformControl {
            id: waveformControlCh0

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: vZoomInButton.left
            anchors.rightMargin: 5

            channelNumber: 0
            width: parent.width - (parent.width * 0.11)
            height: parent.height - (parent.height * 0.6)
        }

        WaveformControl {
            id: waveformControlCh1

            anchors.top: waveformControlCh0.bottom
            anchors.left: parent.left
            anchors.right: vZoomInButton.left
            anchors.rightMargin: 5
            anchors.topMargin: 5

            channelNumber: 1
            width: parent.width - (parent.width * 0.11)
            height: parent.height - (parent.height * 0.6)
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
            id: restoreButton

            text: "Restore"
            anchors.top: hZoomOutButton.bottom
            anchors.right: parent.right
            anchors.rightMargin: 5
            anchors.topMargin: 15
            width: hZoomOutButton.width

            onClicked: {
                waveformControlCh0.xScaleFactor = 1;
                waveformControlCh0.yScaleFactor = 80000;
                waveformControlCh0.wavePos = 0;

                waveformControlCh1.xScaleFactor = 1;
                waveformControlCh1.yScaleFactor = 80000;
                waveformControlCh1.wavePos = 0;
            }
        }

        Button {
            id: reparseButton

            text: "Reparse"
            anchors.top: restoreButton.bottom
            anchors.right: parent.right
            anchors.rightMargin: 5
            anchors.topMargin: 15
            width: hZoomOutButton.width

            onClicked: {
                waveformControlCh0.reparse();
                waveformControlCh1.reparse();
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

            text: "%1 waveform".arg(waveformControlCh0.isWaveformRepaired ? "Restore" : "Repair")
            anchors.top: saveParsedDataButton.bottom
            anchors.right: parent.right
            anchors.rightMargin: 5
            anchors.topMargin: 15
            width: hZoomOutButton.width

            onClicked: {
                if (waveformControlCh0.isWaveformRepaired) {
                    waveformControlCh0.restoreWaveform();
                }
                else {
                    waveformControlCh0.repairWaveform();
                }
            }
        }

        Button {
            id: shiftWaveRight

            text: "<<"
            anchors.bottom: waveformControlCh0.bottom
            anchors.left: restoreButton.left
            width: 40

            onClicked: {
                waveformControlCh0.wavePos -= waveformControlCh0.width * waveformControlCh0.xScaleFactor / 2;
                waveformControlCh1.wavePos -= waveformControlCh1.width * waveformControlCh1.xScaleFactor / 2;
            }
        }

        Button {
            id: shiftWaveLeft

            text: ">>"
            anchors.bottom: waveformControlCh0.bottom
            anchors.right: parent.right
            anchors.rightMargin: 5
            width: 40

            onClicked: {
                waveformControlCh0.wavePos += waveformControlCh0.width * waveformControlCh0.xScaleFactor / 2;
                waveformControlCh1.wavePos += waveformControlCh1.width * waveformControlCh1.xScaleFactor / 2;
            }
        }
    }
}
