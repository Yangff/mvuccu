var shit = require("modapi")

x = shit.doc("import QtQuick 2.3\nimport QtQuick.Controls 1.2\nAction {\nid: root\nproperty string hint:\"haha\"\n}\n")
h = x.imports
console.log(x.imports)  //y
x.imports.push("FuckYou qt") //y
x.imports[0] //y
console.log(x.imports) //y
try {
	x.imports = ["fuck", {"qml":"sb"}] //n
} catch (e) {
	console.log("y");
}
console.log(x.imports)  //y
x.imports = ["fuck", "shit"] //y
console.log(x.imports)  //y
try {
	x.imports = h
} catch (e) {
	// 123
	console.log("y")
}
console.log(x.toString())
x.root.makeProperty("id", "string")
console.log(x.toString())
x.root.set("fuck", "huh")
console.log(x.root.get("shit")) //n
console.log(x.root.get("hint")) // "haha"
x.root.set("hint", "123")
console.log(x.root.get("hint")) // 123
console.log(x.toString())
console.log("=======RET========")
console.log(x.root.ret("hint"))
x.root.ret("hint", "fuck")
x.root.readonly("hint")
console.log(x.root.ret("hint"))
console.log(x.toString())
console.log("==================")

console.log("=======DEF========")

x.root.def("yoo", "Signal", "XX YY, AA BB")
console.log(x.toString())
console.log("=================")

console.log(x.root.names)