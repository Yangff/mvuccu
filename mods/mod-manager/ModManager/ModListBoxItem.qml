import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import Tkool.rpg 1.0
import "../BasicControls"
import "../BasicLayouts"
import "../Singletons"
import "../ModManager"

Item {
    property int implicitWidth: sizehint.paintedWidth + 20
    property var item: model.get(styleData.row)

    Palette { id: pal }

    Column {
        anchors.fill: parent
        anchors.margins: 6
        spacing: 6
        Item {
            id: firstRow
            height: Math.max(label.height, version.height)
            width: parent.width

            Text {
                anchors.left: firstRow.left
                anchors.right: version.left
                id: label
                objectName: "label"
                elide: Text.ElideRight
                text: styleData.value || ""
                color: styleData.textColor
                opacity: 1
                font.family: pal.normalFont
                font.pixelSize: pal.fontSize
                renderType: pal.renderType
                textFormat: Text.PlainText
                horizontalAlignment: Text.AlignLeft
            }
            Text {
                anchors.right: firstRow.right
                id: version
                objectName: "version"
                elide: Text.ElideRight
                text: item.version || ""
                color: styleData.textColor
                opacity: 1
                font.family: pal.normalFont
                font.pixelSize: pal.fontSize
                renderType: pal.renderType
                textFormat: Text.PlainText
                horizontalAlignment: Text.AlignRight
            }
        }
        Item {
            id: secondRow
            height: Math.max(description.height, status.height, newStatusArrow.height, newStatus.height)
            width: parent.width
            Text {
                anchors.left: secondRow.left
                anchors.right: status.left
                anchors.verticalCenter: secondRow.verticalCenter
                id: description
                objectName: "description"
                elide: Text.ElideRight
                text: item.description || "(no description)"
                color: styleData.textColor
                opacity: 1
                font.family: pal.normalFont
                font.pixelSize: pal.fontSize * 0.8
                renderType: pal.renderType
                textFormat: Text.PlainText
                maximumLineCount: 1
            }
            Text {
                anchors.right: newStatusArrow.left
                anchors.verticalCenter: secondRow.verticalCenter

                id: status
                objectName: "status"
                text: item.loaded ? "✓ON" : "✖OFF"
                color: styleData.selected ? styleData.textColor : (item.loaded ? "#22B14C" : "#FF0000")
                font.family: pal.normalFont
                font.pixelSize: pal.fontSize * 0.8
                renderType: pal.renderType
                textFormat: Text.PlainText
            }
            Text {
                anchors.right: newStatus.left
                anchors.verticalCenter: secondRow.verticalCenter

                id: newStatusArrow
                objectName: "newStatusArrow"
                text: (item.enabled != item.loaded) ? " -> " : ""
                color: styleData.textColor
                font.family: pal.normalFont
                font.pixelSize: pal.fontSize * 0.8
                renderType: pal.renderType
                textFormat: Text.PlainText
            }
            Text {
                anchors.right: secondRow.right
                anchors.verticalCenter: secondRow.verticalCenter

                id: newStatus
                objectName: "newStatus"
                text: (item.enabled != item.loaded) ? (item.enabled ? "✓ON" : "✖OFF") : ""
                color: styleData.selected ? styleData.textColor : (item.enabled ? "#22B14C" : "#FF0000")
                font.family: pal.normalFont
                font.pixelSize: pal.fontSize * 0.8
                renderType: pal.renderType
                textFormat: Text.PlainText
            }
        }
    }
    Text {
        id: sizehint
        font: label.font
        text: label.text
        visible: false
        renderType: pal.renderType
        textFormat: Text.PlainText
    }
}
