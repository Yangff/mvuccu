import QtQuick 2.3
import QtQuick.Controls 1.2
import Tkool.rpg 1.0
import "../BasicControls"
import "../BasicLayouts"
import "../Controls"
import "../ObjControls"
import "../Singletons"
import "../Main"
import "../Plugin"

ListBox {
    id: root
    anchors.fill: parent
    anchors.margins: 8

    property var mod : null
    property var manager : null
    property var parameters: ({})

    signal modified()

    Component.onCompleted: loadScript()
    onModChanged: loadScript()

    function reset() {
        parameters = {};
    }

    wantReturn: true
    showSelectionAlways: false

    DialogBoxHelper { id: helper }

    ListBoxColumn {
        title: qsTr("Name")
        role: "name"
        width: 160
    }
    ListBoxColumn {
        title: qsTr("Value")
        role: "value"
        width: 350
    }

    model: ListModel {
        id: parameterListModel
    }

    onDoubleClicked: editParameter()
    Keys.onReturnPressed: editParameter()
    Keys.onEnterPressed: editParameter()

    Dialog_PluginParameter {
        id: parameterDialog
        onOk: finishEdit(value)
    }

    property var paramNames: []
    property var paramDescs: ({})
    property var paramDefaults: ({})

    function loadScript() {
        if (!mod) return;

        var url = TkoolAPI.pathToUrl(mod.location) + "/settings.js";
        var script = TkoolAPI.readFile(url);
        var re = /\/\*\:([a-zA-Z_]*)([\s\S]*?)\*\//mg;
        var locale = TkoolAPI.locale();
        var englishComments = "";
        var localComments = "";
        paramNames = [];
        paramDescs = {};
        paramDefaults = {};
        for (;;) {
            var match = re.exec(script);
            if (match) {
                var lang = match[1];
                if (!lang || lang === "en") {
                    englishComments = match[2];
                } else if (lang.length >= 2 && locale.indexOf(lang) === 0) {
                    localComments = match[2];
                }
            } else {
                break;
            }
        }
        if (localComments) {
            processCommentBlock(localComments);
        } else {
            processCommentBlock(englishComments);
        }
        buildParameterListModel();
    }

    function processCommentBlock(comments) {
        var currentParam = null;
        var re = /@(\w+)([^@]*)/g;
        for (;;) {
            var match = re.exec(comments);
            if (!match) {
                break;
            }
            var keyword = match[1];
            var text = match[2];
            text = text.replace(/[ ]*\n[ ]*\*?[ ]?/g, "\n");
            text = text.trim();
            var text2 = text.split("\n")[0];
            switch (keyword) {
            case 'param':
                paramNames.push(text2);
                currentParam = text2;
                break;
            case 'desc':
                paramDescs[currentParam] = text;
                break;
            case 'default':
                paramDefaults[currentParam] = text2;
                break;
            }
        }
    }

    function buildParameterListModel() {
        parameterListModel.clear();
        for (var i = 0; i < paramNames.length; i++) {
            var name = paramNames[i];
            var item = {};
            item.name = name;
            item.desc = paramDescs[name] || "";
            item.value = paramDefaults[name] || "";
            if (mod.settings.settings[name] !== undefined) {
                item.value = String(mod.settings.settings[name]);
            }
            parameterListModel.append(item);
        }
    }

    function editParameter() {
        var item = parameterListModel.get(currentIndex);
        if (item) {
            parameterDialog.name = item.name;
            parameterDialog.value = item.value;
            parameterDialog.description = item.desc;
            parameterDialog.open();
        }
    }

    function finishEdit(value) {
        var item = parameterListModel.get(currentIndex);
        if (item) {
            item.value = value;
            mod.settings.settings[item.name] = item.value;
            helper.setModified();
            modified();
        }
    }
}
