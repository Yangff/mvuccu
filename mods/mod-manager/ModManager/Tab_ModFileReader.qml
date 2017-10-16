import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.1
import Tkool.rpg 1.0
import "../BasicControls"
import "../BasicLayouts"
import "../Singletons"
import "."

TextArea {
    id: textArea
    readOnly: true
    selectAllOnFocus: false
    anchors.fill: parent
    anchors.margins: 8
    font.pixelSize: pal.labelFontSize * 0.9
    font.family: pal.fixedFont

    property var mod: null
    property var tryFileList: []

    Component.onCompleted: reload()
    onModChanged: reload()

    function reload() {
      if (!mod) {
          textArea.text = ""
          return;
      }

      var modPath = TkoolAPI.pathToUrl(mod.location);

      try {
          var body = null;
          for (var i = 0; i < tryFileList.length; i++) {
              body = TkoolAPI.readFile(modPath + "/" + tryFileList[i]);
              if (body) break;
          }
          if (!body) {
              textArea.text = "Cannot found following files in this mod." + "\n* " + tryFileList.join("\n* ");
              return;
          }

          textArea.text = body;
      } catch(e) {
          textArea.text = e.toString() + "\n" + e.stack.toString();
      }
    }
}
