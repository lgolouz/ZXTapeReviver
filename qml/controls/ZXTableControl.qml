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
import QtQml.Models
import com.models.zxtapereviver 1.0

Control {
    id: root

    property var model
    property int currentRow: -1
    property alias selection: selectionApi
    property int modelRevision: 0
    property bool userResizedColumns: false
    property var fallbackHeaders: []
    property var fallbackColumnWidths: []

    readonly property int rowHeight: 28
    readonly property int headerHeight: 30
    readonly property int defaultColumnWidth: 120
    readonly property int minimumColumnWidth: 40

    padding: 0

    background: Rectangle {
        color: root.palette.base
        border.color: root.palette.mid
    }

    onModelChanged: {
        currentRow = -1;
        userResizedColumns = false;
        invalidate();
        Qt.callLater(applyDefaultColumnWidths);
    }

    QtObject {
        id: selectionApi

        function select(row) {
            root.selectRow(row);
        }

        function deselect(row) {
            if (root.currentRow === row) {
                root.selectRow(-1);
            }
        }

        function forEach(callback) {
            if (root.currentRow !== -1) {
                callback(root.currentRow);
            }
        }
    }

    function selectRow(row) {
        if (row < 0 || row >= rowCount()) {
            currentRow = -1;
            itemSelectionModel.clear();
            return;
        }

        currentRow = row;
        var index = tableView.index(row, 0);
        itemSelectionModel.setCurrentIndex(index, ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Rows);
        tableView.positionViewAtRow(row, TableView.Contain);
    }

    function invalidate() {
        ++modelRevision;
    }

    function hasTableModel() {
        return model !== undefined
            && model !== null
            && typeof model.rowCount === "function"
            && typeof model.columnCount === "function"
            && typeof model.index === "function"
            && typeof model.data === "function";
    }

    function rowCount() {
        modelRevision;
        if (model === undefined || model === null) {
            return 0;
        }

        return hasTableModel() ? model.rowCount() : 0;
    }

    function columnCount() {
        modelRevision;
        if (hasTableModel()) {
            return model.columnCount();
        }

        return Math.max(fallbackHeaders.length, fallbackColumnWidths.length);
    }

    function baseWidthForColumn(column) {
        if (hasTableModel() && typeof model.columnWidthProvider === "function") {
            var width = model.columnWidthProvider(column);
            if (width > 0) {
                return width;
            }
        }

        if (column >= 0 && column < fallbackColumnWidths.length && fallbackColumnWidths[column] > 0) {
            return fallbackColumnWidths[column];
        }

        if (column === 0) {
            return 70;
        }

        if (columnCount() === 2) {
            return Math.max(defaultColumnWidth, width - 70);
        }

        return defaultColumnWidth;
    }

    function defaultWidthForColumn(column) {
        var count = columnCount();
        if (count <= 0) {
            return defaultColumnWidth;
        }

        var totalBaseWidth = 0;
        for (var i = 0; i < count; ++i) {
            totalBaseWidth += Math.max(minimumColumnWidth, baseWidthForColumn(i));
        }

        var availableWidth = Math.max(0, tableView.width);
        var baseWidth = Math.max(minimumColumnWidth, baseWidthForColumn(column));
        if (availableWidth <= totalBaseWidth) {
            return baseWidth;
        }

        if (column === count - 1) {
            var usedWidth = 0;
            for (var j = 0; j < column; ++j) {
                usedWidth += defaultWidthForColumn(j);
            }
            return Math.max(minimumColumnWidth, availableWidth - usedWidth);
        }

        return Math.max(minimumColumnWidth, Math.floor(baseWidth * availableWidth / totalBaseWidth));
    }

    function cellText(row, column) {
        TranslationManager.translationChanged;
        modelRevision;
        if (!hasTableModel()) {
            return "";
        }

        var value = model.data(model.index(row, column), 0);
        return value === undefined || value === null ? "" : value;
    }

    function headerText(column) {
        TranslationManager.translationChanged;
        modelRevision;
        if (!hasTableModel()) {
            if (column >= 0 && column < fallbackHeaders.length) {
                return fallbackHeaders[column];
            }
            return "";
        }

        var value = model.headerData(column, Qt.Horizontal, 0);
        return value === undefined || value === null ? "" : value;
    }

    function applyDefaultColumnWidths() {
        if (!tableView || columnCount() <= 0) {
            return;
        }

        tableView.clearColumnWidths();
        for (var column = 0; column < columnCount(); ++column) {
            tableView.setColumnWidth(column, Math.max(minimumColumnWidth, defaultWidthForColumn(column)));
        }
        tableView.forceLayout();
        invalidate();
    }

    function columnWidth(column) {
        if (!tableView || column < 0 || column >= columnCount()) {
            return defaultColumnWidth;
        }

        var explicitWidth = tableView.explicitColumnWidth(column);
        return explicitWidth > 0 ? explicitWidth : defaultWidthForColumn(column);
    }

    function contentWidth() {
        modelRevision;
        var result = 0;
        for (var column = 0; column < columnCount(); ++column) {
            result += columnWidth(column);
        }
        return result;
    }

    Connections {
        target: root.hasTableModel() ? root.model : null
        ignoreUnknownSignals: true

        function onModelReset() {
            root.currentRow = -1;
            root.userResizedColumns = false;
            itemSelectionModel.clear();
            root.invalidate();
            Qt.callLater(root.applyDefaultColumnWidths);
        }

        function onRowsInserted() { root.invalidate(); }
        function onRowsRemoved() {
            if (root.currentRow >= root.rowCount()) {
                root.selectRow(-1);
            }
            root.invalidate();
        }
        function onColumnsInserted() {
            root.invalidate();
            Qt.callLater(root.applyDefaultColumnWidths);
        }
        function onColumnsRemoved() {
            root.invalidate();
            Qt.callLater(root.applyDefaultColumnWidths);
        }
        function onDataChanged() { root.invalidate(); }
        function onHeaderDataChanged() { root.invalidate(); }
    }

    ItemSelectionModel {
        id: itemSelectionModel
        model: tableView.model
    }

    contentItem: Item {
        clip: true

        Item {
            id: headerView

            anchors.left: parent.left
            anchors.top: parent.top
            anchors.right: parent.right
            height: root.headerHeight
            clip: true

            Rectangle {
                anchors.fill: parent
                color: root.palette.button
                border.color: root.palette.mid
            }

            Item {
                x: -tableView.contentX
                width: Math.max(root.contentWidth(), headerView.width)
                height: parent.height

                Row {
                    height: parent.height

                    Repeater {
                        model: root.columnCount()

                        Button {
                            id: headerCell

                            width: root.columnWidth(index)
                            height: headerView.height
                            text: root.headerText(index)
                            padding: 6
                            font: root.font
                            horizontalPadding: 8

                            MouseArea {
                                id: resizeHandle

                                anchors {
                                    top: parent.top
                                    right: parent.right
                                    bottom: parent.bottom
                                }
                                width: 8
                                cursorShape: Qt.SplitHCursor
                                hoverEnabled: true
                                acceptedButtons: Qt.LeftButton

                                property real startX: 0
                                property real startWidth: 0

                                onPressed: (mouse) => {
                                    root.userResizedColumns = true;
                                    startX = mouse.x;
                                    startWidth = root.columnWidth(index);
                                    mouse.accepted = true;
                                }

                                onPositionChanged: (mouse) => {
                                    if (!pressed) {
                                        return;
                                    }

                                    var nextWidth = Math.max(root.minimumColumnWidth, startWidth + mouse.x - startX);
                                    tableView.setColumnWidth(index, nextWidth);
                                    root.invalidate();
                                    tableView.forceLayout();
                                }
                            }
                        }
                    }
                }
            }
        }

        TableView {
            id: tableView

            anchors {
                left: parent.left
                top: headerView.bottom
                right: parent.right
                bottom: parent.bottom
            }
            clip: true
            model: root.model
            boundsBehavior: Flickable.StopAtBounds
            alternatingRows: true
            selectionModel: itemSelectionModel
            selectionBehavior: TableView.SelectRows
            selectionMode: TableView.SingleSelection
            resizableColumns: true
            resizableRows: false
            rowHeightProvider: function(row) { return root.rowHeight; }
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
            ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }
            onWidthChanged: {
                if (!root.userResizedColumns) {
                    Qt.callLater(root.applyDefaultColumnWidths);
                }
            }

            delegate: Rectangle {
                id: cell

                required property int row
                required property int column

                implicitWidth: root.defaultWidthForColumn(column)
                implicitHeight: root.rowHeight
                color: row === root.currentRow
                       ? root.palette.highlight
                       : tableView.alternatingRows && row % 2 !== 0
                         ? root.palette.alternateBase
                         : root.palette.base

                Label {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    text: root.cellText(cell.row, cell.column)
                    color: cell.row === root.currentRow
                           ? root.palette.highlightedText
                           : root.palette.text
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                }

                Rectangle {
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: 1
                    color: root.palette.midlight
                }

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: 1
                    color: root.palette.midlight
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    onClicked: root.selectRow(cell.row)
                }
            }

            Component.onCompleted: Qt.callLater(root.applyDefaultColumnWidths)
        }
    }
}
