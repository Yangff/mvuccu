var ModAPI = require('modapi')
var _ = require('lodash')
var qml = ModAPI.qml("BasicControls/ComboBox.qml")

var node, objects, obj
qml.root.remove("Keys.onLeftPressed")
.remove("Keys.onRightPressed")
.set("editable", "true")

qml.root.getObjectById("mouseArea")
.remove("anchors.fill")
.set("anchors.right", "parent.right").end()

qml.root.getObjectById("mouseArea")
.set("anchors.top", "parent.top").end()
.set("anchors.bottom", "parent.bottom").end()
.set("width", "20").end()

//require('buffer').fromString('a')

ModAPI.add("BasicControls/EditableComboBox.qml", qml.toString())
console.log("a")