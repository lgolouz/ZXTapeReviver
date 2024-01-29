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

Item {
    id: zxTableControlRoot

    implicitHeight: 50
    implicitWidth: 50

    property alias topCornerRectColor: topCornerRect.color
    property alias topCornerRectBorder: topCornerRect.border
    property alias topCornerRectRadius: topCornerRect.radius

    property var horizontalHeaderModel: ["a", "b", "c", "d"]
    property var verticalHeaderModel: ["1", "2", "3"]

    Rectangle {
        id: tableControlRoot

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

            width: verticalHeaderView.columnAtIndex(0).width > 0 ? verticalHeaderView.columnAtIndex(0).width : topCornerRect.implicitWidth
            height: horizontalHeaderView.rowAtIndex(0).height > 0 ? horizontalHeaderView.rowAtIndex(0).height : topCornerRect.implicitHeight
        }

        VerticalHeaderView {
            id: verticalHeaderView

            anchors.top: topCornerRect.bottom
            anchors.bottom: tableControlRoot.bottom

            //model: zxTableControlRoot.verticalHeaderModel
            syncView: zxTableView
            textRole: "verticalHeader"
        }

        HorizontalHeaderView {
            id: horizontalHeaderView

            anchors {
                top: tableControlRoot.top
                bottom: topCornerRect.bottom
                left: topCornerRect.right
                right: tableControlRoot.right
            }

            //model: zxTableControlRoot.horizontalHeaderModel
            syncView: zxTableView
            textRole: "horizontalHeader"
        }

        TableView {
            id: zxTableView
            model: TableModel {
                rows: ["q", "w", "e", "r", "t"]
            }
        }

        color: "red"
    }
}
