import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.1
import Tkool.rpg 1.0
import "../BasicControls"
import "../BasicLayouts"
import "../Singletons"
import "."

ModalWindow {
    id: root

    title: qsTr("Mod Manager")

    DialogBox {
        id: dialogBox

        okVisible: true
        cancelVisible: true
        applyVisible: true

        property var modsData: null
        property var managerData: null
        property var settingsPath: null

        onInit: {
            loadModsData();
            loadManagerData();

            modList.setData(modsData.mods);
            modList.currentIndex = 0;
        }

        onOk: {
            saveModSettings();
        }

        onApply: {
            saveModSettings();
            applyEnabled = false;
        }

        Palette { id: pal }

        function buildModSettingsUrl(name) {
            return settingsPath + encodeURIComponent(name) + ".json";
        }

        function loadModSetting(name) {
            var settings = { enabled: true, settings: {} };
            var path = buildModSettingsUrl(name);
            try {
                var json = TkoolAPI.readFile(path);
                if (json) {
                    var data = JSON.parse(json);
                    if ( (typeof data === 'function') || (typeof data === 'object')) {
                        settings = data;
                        settings.enabled = !!settings.enabled;
                    }
                }
            } catch (e) {
                console.warn(e);
            }
            return settings;
        }

        function saveModSettings() {
            for (var i = 0; i < modsData.mods.length; i++) {
                saveModSetting(modsData.mods[i].name, modsData.mods[i].settings);
            }
        }

        function saveModSetting(name, settings) {
            var path = buildModSettingsUrl(name);
            try {
                TkoolAPI.writeFile(path, JSON.stringify(settings));
            } catch (e) {
                console.error(e);
            }
        }

        function findDependent(mod) {
            var result = [];

            for (var i = 0; i < modsData.mods.length; i++) {
                var dependencies = {};
                if (modsData.mods[i].manifest && modsData.mods[i].manifest.dependencies)
                    dependencies = modsData.mods[i].manifest.dependencies;

                if (dependencies[mod.name])
                    result.push(modsData.mods[i]);
            }

            return result;
        }

        function findDependency(mod) {
            var result = [];
            var dependencies = {};

            if (mod.manifest && mod.manifest.dependencies)
                dependencies = mod.manifest.dependencies;

            for (var i = 0; i < modsData.mods.length; i++) {
                if (dependencies[modsData.mods[i].name])
                    result.push(modsData.mods[i]);
            }

            return result;
        }

        function setModStatus(i, status) {
            var mod = modsData.mods[i];
            if (mod.settings.enabled == status) return;

            var updateStatus = function(mod) {
                var dependencies = [];
                if (status)
                    dependencies = findDependency(mod);
                else
                    dependencies = findDependent(mod);

                if (dependencies.length > 0) {
                    for (var i = 0; i < dependencies.length; i++) {
                        updateStatus(dependencies[i]);
                    }
                }
                mod.settings.enabled = status;
            }
            updateStatus(mod);
            modList.refresh();
            applyEnabled = true;
        }

        function loadModsData() {
            modsData = loadRawModsData();

            settingsPath = TkoolAPI.pathToUrl(modsData.path) + "/.settings/"
            if (!TkoolAPI.isDirectoryExists(settingsPath)) {
                TkoolAPI.createDirectories(settingsPath);
            }

            for (var i = 0; i < modsData.mods.length; i++) {
                modsData.mods[i].settings = loadModSetting(modsData.mods[i].name);
                saveModSetting(modsData.mods[i].name, modsData.mods[i].settings);
            }
        }

        property var currentMod: modsData ? modsData.mods[modList.currentIndex] : null
        property var currentModFixed: null

        function switchToMod(index) {
            var mod = modsData.mods[index];
            for (var i = 4; i < tabView.count; i++) {
                tabView.removeTab(i);
            }

            currentModFixed = mod;

            var tabs = managerData.settingsPanel[mod.name] || [];
            onTabViewLoadedConnections.model = tabs.length;
            for (var i = 0; i < tabs.length; i ++) {
                var tab = tabs[i];
                var loader = Qt.createComponent("Settings/" + mod.name + "/" + tab.describe + ".qml");
                onTabViewLoadedConnections.itemAt(i).conn.target = tabView.addTab(tab.name, loader);
            }
        }

        function refreshUserSettingPanels() {
          for (var i = 4; i < tabView.count; i++) {
              var tab = tabView.getTab(i);
              if (!tab) continue;

              var item = tab.item;
              if (!item) continue;
              if (!item.refresh) continue;

              item.refresh();
          }
        }

        Repeater {
            id: onTabViewLoadedConnections
            model: 0
            Item {
                property var conn: Connections {
                    target: null
                    onLoaded: {
                        target.item.mod = currentModFixed;
                    }
                }
            }
        }

        function loadManagerData() {
            var path = ":/qml/ModManager/data.json";
            try {
                var json = TkoolAPI.readResource(path);
                if (json) {
                    return managerData = JSON.parse(json);
                } else {
                    return managerData = null;
                }
            } catch (e) {
                console.warn(e);
                return managerData = null;
            }
        }

        function loadRawModsData() {
            var path = ":/qml/ModManager/mods.json";
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

        DialogBoxColumn {
            DialogBoxRow {
                ListBox {
                    id: modList
                    width: 200
                    height: tabView.height

                    itemHeight: 48

                    headerVisible: false
                    multiSelect: false
                    showSelectionAlways: true
                    wantReturn: false

                    itemDelegate: ModListBoxItem { }

                    contextMenu: Menu {
                        id: contextMenu

                        MenuItem {
                            text: qsTr("Turn ON")
                            onTriggered: {
                                modList.changeSelectedItemStatuses(true);
                            }
                        }
                        MenuItem {
                            text: qsTr("Turn OFF")
                            onTriggered: {
                                modList.changeSelectedItemStatuses(false);
                            }
                        }
                    }

                    model: ListModel {
                        id: listModel
                    }

                    property var data: null

                    function changeSelectedItemStatuses(status) {
                        for (var i = 0; i < data.length; i++) {
                            if (isSelected(i)) {
                                dialogBox.setModStatus(i, status);
                            }
                        }
                    }

                    function refresh() {
                        for (var i = 0; i < data.length; i++) {
                            listModel.set(i, makeModelItem(data[i]));
                        }
                    }

                    function setData(_data) {
                        data = _data;
                        build();
                    }

                    function build() {
                        listModel.clear();
                        for (var i = 0; i < data.length; i++) {
                            listModel.append(makeModelItem(data[i]));
                        }
                    }

                    ListBoxColumn {
                        role: "name"
                    }

                    function makeModelItem(data) {
                        return {
                            name: data.name,
                            version: data.manifest.version,
                            loaded: data.loaded,
                            enabled: data.settings.enabled,
                            description: data.manifest.description
                        }
                    }

                    onCurrentIndexChanged: {
                        dialogBox.switchToMod(currentIndex);
                    }
                }
                TabView {
                    id: tabView
                    width: 600
                    height: 400
                    Tab {
                        title: "Info"
                        id: infoTab

                        Tab_ModInfo {
                            mod: dialogBox.currentMod
                            manager: dialogBox
                        }
                    }
                    Tab {
                        title: "Help"
                        id: helpTab

                        Tab_ModFileReader {
                            mod: dialogBox.currentMod
                            tryFileList: ["README.md", "README.markdown", "README.txt", "README"]
                        }
                    }
                    Tab {
                        title: "License"
                        id: licenseTab

                        Tab_ModFileReader {
                            mod: dialogBox.currentMod
                            tryFileList: ["LICENSE.md", "LICENSE.markdown", "LICENSE.txt", "LICENSE"]
                        }
                    }
                    Tab {
                        title: "Settings"
                        id: settingsTab

                        Tab_ModSettings {
                            mod: dialogBox.currentMod
                            onModified: {
                                dialogBox.refreshUserSettingPanels();
                            }
                        }
                    }
                }
            }
        }
    }
}
