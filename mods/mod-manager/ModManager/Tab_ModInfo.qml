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

    property var mod : null
    property var manager : null

    Component.onCompleted: reload()
    onModChanged: reload()

    function reload() {
        if (!mod) {
            return;
        }

        var text = "";
        var manifest = mod.manifest || {};

        text +=        String(mod.name) + " " + String(manifest.version);
        text += "\n" + String(manifest.description);
        text += "\n" + "";
        text += "\n" + "Author: " + String(manifest.author);
        text += "\n" + "Homepage: " + String(manifest.homepage);
        text += "\n" + "";

        var dependence = manifest.dependencies || {};
        text += "\n" + "Dependencies: ";
        for (var i in dependence) {
            if (dependence.hasOwnProperty(i)) {
                text += "\n" + "* " + String(i) + " (" + String(dependence[i]) + ")";
            }
        }
        text += "\n" + "";

        var dependent = manager.findDependent(mod);
        text += "\n" + "Dependents: ";
        for (var i = 0; i < dependent.length; i++) {
            text += "\n" + "* " + String(dependent[i].name) + "@" + String((dependent[i].manifest || {}).version);
        }
        text += "\n" + "";

        textArea.text = text;
    }
}
