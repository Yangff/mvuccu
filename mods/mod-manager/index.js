var ModAPI = require('modapi')
var _ = require('lodash')
var fs = require('fs')
var path = require('path')

var readLocalFile = function(name) {
  return fs.readFileSync(path.join(__dirname, name))
}


;[
  "Main/Dialog_AboutMod.qml",
  "ModManager/Dialog_ModManager.qml",
  "ModManager/ModListBoxItem.qml",
  "ModManager/Tab_ModFileReader.qml",
  "ModManager/Tab_ModInfo.qml",
  "ModManager/Tab_ModSettings.qml",
].forEach(function(i) {
  if (path.extname(i) == ".js" && path.extname(path.basename(i, ".js")) == ".qml") {
    require("./" + i)
  } else {
    ModAPI.add(i, readLocalFile(i))
  }
})

var addMainMenu = function() {
  var qml = ModAPI.qml("Main/MainMenu.qml")
  var rootNode = qml.root

  var rootMenus = rootNode.get("menuBar").obj()

  var helpMenu = _.find(rootMenus, function(i) {
    return i.get("title") == 'qsTr("Help")'
  }) || _.last(rootMenus)

  var aboutModMenu = ModAPI.node([
    "MenuItem {",
      "text: qsTr(\"About\") + \" mvUCCU...\"",
      "enabled: true",
      "onTriggered: root.aboutMod()",
    "}"
  ].join("\n"))

  helpMenu.add(aboutModMenu)

  rootNode.def("aboutMod", "Signal", "")

  var toolsMenu = _.find(rootMenus, function(i) {
    return i.get("title") == 'qsTr("Tools")'
  }) || rootMenus[5]


  var openModManagerMenu = ModAPI.node([
    "MenuItem {",
      "text: \"Mod Manager...\"",
      "enabled: true",
      "onTriggered: root.openModManager()",
    "}",
  ].join("\n"))

  toolsMenu.add(openModManagerMenu)

  rootNode.def("openModManager", "Signal", "")

  qml.save()
}

var addMainAction = function() {
  var qml = ModAPI.qml("Main/MainWindow.qml")
  var rootNode = qml.root

  var mainMenu = rootNode.getObjectById("mainMenu")
  mainMenu.set("onAboutMod", "aboutModBox.open()")
  mainMenu.set("onOpenModManager", "modManagerBox.item.open()")

  var aboutModBox = ModAPI.node("Dialog_AboutMod { id: aboutModBox }")
  rootNode.add(aboutModBox)

  var modManagerBox = ModAPI.node("Loader { id:modManagerBox \n source: \"../ModManager/Dialog_ModManager.qml\"}")
  rootNode.add(modManagerBox)

  qml.save()
}

addMainMenu()
addMainAction()

var data = {
  settingsPanel: {}
}
var dumped = false
var ModManager = {}
var dumpData = function() {
  (dumped ? ModAPI.update : ModAPI.add).apply(ModAPI, ["ModManager/data.json", JSON.stringify(data)])
}

ModManager.registerSettingPanel = function(name, describe) {
  var modName = ModAPI.currentModName()
  data.settingsPanel[modName] = data.settingsPanel[modName] || []
  data.settingsPanel[modName].push({name: name, describe: describe})
  dumpData()
}

dumpData()
module.exports = ModManager
