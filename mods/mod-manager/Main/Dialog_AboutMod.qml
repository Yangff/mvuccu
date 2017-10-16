import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.1
import Tkool.rpg 1.0
import "../BasicControls"
import "../BasicLayouts"
import "../Singletons"

ModalWindow {
    id: root

    title: qsTr("About %1").arg("mvUCCU")

    DialogBox {
        okVisible: false
        cancelVisible: false
        applyVisible: false
        closeVisible: true

        onInit: {
          var versions = {};
          var path = ":/mod/version.json";
          try {
              var json = TkoolAPI.readResource(path);
              if (json) {
                  versions = JSON.parse(json);
              }
          } catch (e) {
              console.warn(e);
          }

          label1.text = "mvUCCU " + String(versions.gitTag) + " @" + String(versions.gitBranch);

          docList.refresh();
          docList.currentIndex = 0;
        }

        Palette { id: pal }

        DialogBoxColumn {
            Column {
                Label {
                    id: label1
                    text: "mvUCCU"
                    font.pixelSize: pal.labelFontSize * 1.5
                }
            }
            DialogBoxRow {
                ListBox {
                    id: docList
                    width: 200
                    height: 360

                    headerVisible: false
                    multiSelect: false
                    showSelectionAlways: true
                    wantReturn: false

                    model: ListModel {
                        id: listModel
                    }

                    function refresh() {
                        var array = getDataArray();
                        listModel.clear();
                        for (var i = 0; i < array.length; i++) {
                            listModel.append(makeModelItem(array[i]));
                        }
                    }

                    ListBoxColumn {
                        role: "name"
                    }

                    function getDataArray() {
                        var path = ":/mod/texts.json";
                        try {
                            var json = TkoolAPI.readResource(path);
                            if (json) {
                                return JSON.parse(json);
                            } else {
                                return null;
                            }
                        } catch (e) {
                            console.warn(e);
                            return null;
                        }
                    }

                    function makeModelItem(data) {
                        return data;
                    }

                    onCurrentIndexChanged: {
                        textArea.text = model.get(currentIndex).body;
                    }
                }
                TextArea {
                    id: textArea
                    readOnly: true
                    selectAllOnFocus: false
                    width: 560
                    height: 360
                    font.pixelSize: pal.labelFontSize * 0.9
                    font.family: pal.fixedFont
                }
            }
        }
    }
}
