var ModAPI = require('modapi')
var _ = require('lodash')
var fs = require('fs')
var path = require('path')

var readLocalFile = function(name) {
  return fs.readFileSync(path.join(__dirname, name))
}

// Add Essential GUI Components
;[
  "BasicControls/EditableComboBox.qml.js",
  "BasicLayouts/LabeledEditableComboBox.qml",
  "Controls/DialogAddon.qml"
].forEach(function(i) {
  if (path.extname(i) == ".js" && path.extname(path.basename(i, ".js")) == ".qml") {
    require("./" + i)
  } else {
    ModAPI.add(i, readLocalFile(i))
  }
})

var Essentials = {}

Essentials.addSingleton = function(name, content) {
  ModAPI.add("Singletons/" + name + ".qml", content)

  var qmldir = ModAPI.get("Singletons/qmldir")
  qmldir += "\n"
  qmldir += "singleton " + name + " " + name + ".qml\n"
  ModAPI.update("Singletons/qmldir", qmldir)
}


module.exports = Essentials
