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

    Connections {
        target: FileWorkerModel
        function onWavFileNameChanged() {
            waveformControl.reparse();
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "black"

        WaveformControl {
            id: waveformControl

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: vZoomInButton.left
            anchors.rightMargin: 5

            width: parent.width - (parent.width * 0.11)
            height: parent.height - (parent.height * 0.4)
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
                var yfactor = waveformControl.yScaleFactor;
                if (yfactor > 1000) {
                    waveformControl.yScaleFactor = yfactor / 2;
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
                var yfactor = waveformControl.yScaleFactor;
                if (yfactor < 320000) {
                    waveformControl.yScaleFactor = yfactor * 2;
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
                waveformControl.xScaleFactor = waveformControl.xScaleFactor / 2;
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
                waveformControl.xScaleFactor = waveformControl.xScaleFactor * 2;
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
                waveformControl.xScaleFactor = 1;
                waveformControl.yScaleFactor = 80000;
                waveformControl.wavePos = 0;
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
                waveformControl.reparse();
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
                waveformControl.saveTap();
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
                waveformControl.saveWaveform();
            }
        }

        Button {
            id: repairRestoreButton

            text: "%1 waveform".arg(waveformControl.isWaveformRepaired ? "Restore" : "Repair")
            anchors.top: saveParsedDataButton.bottom
            anchors.right: parent.right
            anchors.rightMargin: 5
            anchors.topMargin: 15
            width: hZoomOutButton.width

            onClicked: {
                if (waveformControl.isWaveformRepaired) {
                    waveformControl.restoreWaveform();
                }
                else {
                    waveformControl.repairWaveform();
                }
            }
        }

        Button {
            id: shiftWaveRight

            text: "<<"
            anchors.bottom: waveformControl.bottom
            anchors.left: restoreButton.left
            width: 40

            onClicked: {
                waveformControl.wavePos -= waveformControl.width * waveformControl.xScaleFactor / 2;
            }
        }

        Button {
            id: shiftWaveLeft

            text: ">>"
            anchors.bottom: waveformControl.bottom
            anchors.right: parent.right
            anchors.rightMargin: 5
            width: 40

            onClicked: {
                waveformControl.wavePos += waveformControl.width * waveformControl.xScaleFactor / 2;
            }
        }
    }
}
