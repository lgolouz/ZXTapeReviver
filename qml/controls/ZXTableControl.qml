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

import QtQuick
import QtQuick.Controls
import Qt.labs.qmlmodels

Item {
    id: zxTableControlRoot

    implicitHeight: 50
    implicitWidth: 50

    property alias topCornerRectColor: topCornerRect.color
    property alias topCornerRectBorder: topCornerRect.border
    property alias topCornerRectRadius: topCornerRect.radius

    property var horizontalHeaderModel: ["a", "b", "c", "d"]
    property var verticalHeaderModel: ["1", "2", "3"]
    property alias model: tableControlRoot.tableModel

    Rectangle {
        id: tableControlRoot

        property var tableModel

        anchors.fill: parent

        Rectangle {
            id: topCornerRect

            border {
                width: 1
                color: "black"
            }

            anchors {
                top: tableControlRoot.top
                left: tableControlRoot.left
            }

            color: "transparent"

            implicitHeight: 30
            implicitWidth: 50

            width: verticalHeaderView.columnWidth(0) > 0 ? verticalHeaderView.columnWidth(0) : topCornerRect.implicitWidth
            height: horizontalHeaderView.rowHeight(0) > 0 ? horizontalHeaderView.rowHeight(0) : topCornerRect.implicitHeight
        }

        VerticalHeaderView {
            id: verticalHeaderView

            anchors.top: topCornerRect.bottom
            anchors.bottom: tableControlRoot.bottom

            model: zxTableControlRoot.verticalHeaderModel
            syncView: zxTableView
            textRole: "verticalHeader"
            clip: true
        }

        HorizontalHeaderView {
            id: horizontalHeaderView

            anchors {
                top: tableControlRoot.top
                bottom: topCornerRect.bottom
                left: topCornerRect.right
                right: tableControlRoot.right
            }

            model: zxTableControlRoot.horizontalHeaderModel
            syncView: zxTableView
            textRole: "horizontalHeader"
            clip: true
        }

        TableView {
            id: zxTableView
            model: tableControlRoot.tableModel
            anchors.top: topCornerRect.bottom
            anchors.left: topCornerRect.right
            delegate: Rectangle {
                implicitHeight: 20
                implicitWidth: 60
                Text {
                    text: model.display
                }
                color: "yellow"
            }
        }

        color: "red"

        function initTableModel() {
            var modelStr = "import Qt.labs.qmlmodels;TableModel{"
            for (var i = 0; i < zxTableControlRoot.horizontalHeaderModel.length; ++i) {
                modelStr += "TableModelColumn{display:'col_" + i + "';}"
            }
            modelStr += "}"

            tableModel = Qt.createQmlObject(modelStr, tableControlRoot)
            for (var j = 0; j < zxTableControlRoot.verticalHeaderModel.length; ++j) {
                var obj = {}
                for (var k = 0; k < zxTableControlRoot.horizontalHeaderModel.length; ++k) {
                    obj['col_' + k] = "" + j + "; " + k
                }
                tableModel.appendRow(obj)
            }
        }

        Connections {
            target: zxTableControlRoot

            function onHorizontalHeaderModelChanged() {
                initTableModel();
            }

            function onVerticalHeaderModelChanged() {
                initTableModel();
            }
        }

        Component.onCompleted: _ => initTableModel()
    }
}
