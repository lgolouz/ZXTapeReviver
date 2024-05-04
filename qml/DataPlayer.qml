//*******************************************************************************
// ZX Tape Reviver
//-----------------
//
// Author: Leonid Golouz
// E-mail: lgolouz@list.ru
// YouTube channel: https://www.youtube.com/channel/UCz_ktTqWVekT0P4zVW8Xgcg
// YouTube channel e-mail: computerenthusiasttips@mail.ru
//
// Code modification and distribution of any kind is not allowed without direct
// permission of the Author.
//*******************************************************************************

import QtQuick 2.15
import QtQuick.Controls 1.3
import QtQuick.Dialogs 1.3
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.12

import com.models.zxtapereviver 1.0
import "."

Dialog {
    id: dataPlayerDialog

    property int selectedChannel: 0
    property var parsedChannel: undefined

    visible: false
    title: Translations.id_playing_parsed_data_window_header
    standardButtons: StandardButton.Close
    modality: Qt.WindowModal
    width: 500
    height: 400

    Connections {
        target: DataPlayerModel
        function onCurrentBlockChanged() {
            var cb = DataPlayerModel.currentBlock;
            parsedDataView.selection.forEach(function(rowIndex) { parsedDataView.selection.deselect(rowIndex); });
            parsedDataView.selection.select(cb);
        }
    }

    TableView {
        id: parsedDataView

        width: parent.width
        //height: parent.height * 0.9
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            bottom: progressBarItem.top
            bottomMargin: 5
        }

        TableViewColumn {
            title: Translations.id_block_number
            width: rightArea.width * 0.07
            role: "block"
            delegate: Item {
                property bool blkSelected: styleData.value.blockSelected
                property int blkNumber: styleData.value.blockNumber

                Rectangle {
                    anchors.fill: parent
                    border.width: 0
                    color: parent.blkSelected ? "#A00000FF" : "transparent"
                    Text {
                        anchors.centerIn: parent
                        color: parent.parent.blkSelected ? "white" : "black"
                        text: blkNumber + 1
                    }
                }

//                MouseArea {
//                    anchors.fill: parent
//                    onClicked: {
//                        WaveformParser.toggleBlockSelection(blkNumber);
//                    }
//                }
            }
        }

        TableViewColumn {
            title: Translations.id_block_type
            width: rightArea.width * 0.23
            role: "blockType"
        }

        TableViewColumn {
            title: Translations.id_block_name
            width: rightArea.width * 0.3
            role: "blockName"
        }

        TableViewColumn {
            title: Translations.id_block_size
            width: rightArea.width * 0.25
            role: "blockSize"
        }

        TableViewColumn {
            title: Translations.id_block_status
            width: rightArea.width * 0.45
            role: "blockStatus"
        }

        selectionMode: SelectionMode.SingleSelection
        model: parsedChannel
        itemDelegate: Text {
            text: styleData.value
            color: modelData.state === 0 ? "black" : "red"
        }
    }

    Button {
        id: playParsedData

        text: DataPlayerModel.stopped ? Translations.id_play_parsed_data : Translations.id_stop_playing_parsed_data
        anchors.bottom: parent.bottom
        anchors.left: parent.left

        onClicked: {
            if (DataPlayerModel.stopped) {
                DataPlayerModel.playParsedData(selectedChannel, parsedDataView.currentRow === -1 ? 0 : parsedDataView.currentRow);
            } else {
                DataPlayerModel.stop();
            }
        }
    }

    Item {
        id: progressBarItem
        anchors {
            bottom:playParsedData.top
            left: parent.left
            right: parent.right
            bottomMargin: 5
        }
        height: progressBarRect.height + startDurationText.height

        Rectangle {
            id: progressBarRect

            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
            }
            height: textFontMetrics.height * 1.25

            color: "transparent"
            border.color: "black"

            Rectangle {
                anchors {
                    top: parent.top
                    bottom: parent.bottom
                    left: parent.left
                    topMargin: parent.border.width
                    bottomMargin: parent.border.width
                    leftMargin: parent.border.width
                }
                width: (parent.width - 2 * parent.border.width) * (DataPlayerModel.processedTime / DataPlayerModel.blockTime)
                LinearGradient {
                    anchors.fill: parent
                    gradient: Gradient {
                            GradientStop { position: 0.0; color: "#1B94EF" }
                            GradientStop { position: 1.0; color: "#92C1E4" }
                        }
                    start: Qt.point(0, 0)
                    end: Qt.point(parent.width, 0)
                }
            }

            Text {
                id: playingRecordText
                font.pixelSize: 12
                anchors.fill: parent
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                text: {
                    var b = DataPlayerModel.blockData;
                    return b == undefined ? "" : (b.block.blockNumber + 1) + ": " + b.blockType + " " + b.blockName;
                }
            }
            FontMetrics {
                id: textFontMetrics
                font: playingRecordText.font
            }
        }

        function getTimeText(t) {
            var bt_s = ~~(t / 1000);
            var btm = ~~(bt_s / 60);
            var bts = ~~(bt_s - (btm * 60));
            return btm + ":" + String(bts).padStart(2, "0");
        }

        Text {
            id: startDurationText
            text: progressBarItem.getTimeText(0)
            anchors {
                top: progressBarRect.bottom
                left: parent.left
            }
        }
        Text {
            id: endDurationText
            text: progressBarItem.getTimeText(DataPlayerModel.blockTime)
            anchors {
                top: progressBarRect.bottom
                right: parent.right
            }
        }
        Text {
            id: currentDurationText
            text: progressBarItem.getTimeText(DataPlayerModel.processedTime)
            anchors {
                top: progressBarRect.bottom
                left: parent.left
                right: parent.right
            }
            horizontalAlignment: Text.AlignHCenter
        }
    }

    onVisibleChanged: {
        if (!visible) {
            DataPlayerModel.stop();
        }
    }
}
