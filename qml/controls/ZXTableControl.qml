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

    // implicitHeight: 50
    // implicitWidth: 50

    // property alias topCornerRectColor: topCornerRect.color
    // property alias topCornerRectBorder: topCornerRect.border
    // property alias topCornerRectRadius: topCornerRect.radius

    //property var verticalHeaderModel//: ["1", "2", "3"]
    property var columnsModel: [ { name: "a", display: "col_0", delegate: null },
                                 { name: "b", display: "col_1", delegate: null },
                                 { name: "c", display: "col_2", delegate: null },
                                 { name: "d", display: "col_3", delegate: null } ]
    property alias model: tableControlRoot.tableModel

    Rectangle {
        id: tableControlRoot

        property alias tableModel: zxTableView.model
        property var horizontalHeaderModel: []

        anchors.fill: parent

        // Rectangle {
        //     id: topCornerRect

        //     border {
        //         width: 1
        //         color: "black"
        //     }

        //     anchors {
        //         top: tableControlRoot.top
        //         left: tableControlRoot.left
        //     }

        //     color: "transparent"

        //     implicitHeight: 30
        //     implicitWidth: 50

        //     width: verticalHeaderView.columnWidth(0) > 0 ? verticalHeaderView.columnWidth(0) : topCornerRect.implicitWidth
        //     height: horizontalHeaderView.rowHeight(0) > 0 ? horizontalHeaderView.rowHeight(0) : topCornerRect.implicitHeight
        // }

        VerticalHeaderView {
            id: verticalHeaderView

            anchors.top: zxTableView.top
            anchors.left: parent.left
            // anchors.top: topCornerRect.bottom
            // anchors.bottom: tableControlRoot.bottom
            // anchors.left: tableControlRoot.left
            // anchors.right: topCornerRect.right

            //model: //zxTableControlRoot.verticalHeaderModel
            syncView: zxTableView
            //textRole: "verticalHeader"
            clip: true
        }

        HorizontalHeaderView {
            id: horizontalHeaderView

            anchors.left: zxTableView.left
            anchors.top: parent.top
            //height: 20
            // anchors {
            //     top: tableControlRoot.top
            //     bottom: topCornerRect.bottom
            //     left: topCornerRect.right
            //     right: tableControlRoot.right
            // }

            model: tableControlRoot.horizontalHeaderModel
            syncView: zxTableView
            //textRole: "horizontalHeader"
            clip: true
            // delegate: Rectangle {
            //     color: "transparent"
            //     Text {
            //         anchors.fill: parent
            //         text: model.modelData
            //     }
            // }
        }

        TableView {
            id: zxTableView
            model: tableControlRoot.tableModel
            anchors.left: verticalHeaderView.right
            anchors.top: horizontalHeaderView.bottom
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            // anchors {
            //     top: topCornerRect.bottom
            //     left: topCornerRect.right
            //     right: tableControlRoot.right
            //     bottom: tableControlRoot.bottom
            // }

            columnSpacing: 1
            rowSpacing: 1
            clip: true

            delegate: Rectangle {
                implicitWidth: 100
                implicitHeight: 20
                Text {
                    text: model.display
                }
                color: "yellow"
            }
        }

        color: "red"

        function initTableModel() {
            var hdrModel = []
            var modelStr = "import Qt.labs.qmlmodels;TableModel{"
            for (var i = 0; i < zxTableControlRoot.columnsModel.length; ++i) {
                modelStr += "TableModelColumn{display:'" + zxTableControlRoot.columnsModel[i].display + "';}"
                hdrModel.push(zxTableControlRoot.columnsModel[i].name)
            }
            modelStr += "}"
            tableControlRoot.horizontalHeaderModel = hdrModel;

            tableModel = Qt.createQmlObject(modelStr, tableControlRoot)
            for (var j = 0; j < 4; ++j) {
                var obj = {}
                for (var k = 0; k < zxTableControlRoot.columnsModel.length; ++k) {
                    obj[zxTableControlRoot.columnsModel[k].display] = "" + j + "; " + k
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
