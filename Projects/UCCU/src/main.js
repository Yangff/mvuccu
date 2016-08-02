require('console').error = function () {
	console.err(require('util').formart(arguments))
}

var ml = require('modloader');
ml.mods = {}

function safe_require(name){
	var mod;
	try {
		mod = require(name)
	} catch (e) {
		console.log(e.message)
		ml.mods[name] = {
			name: name,
			loaded: false,
			reason: e.message
		}
		return false;
	}
	ml.mods[name] = {
		name: name,
		mod: mod,
		loaded: true
	}
	return true
}

try {
	exports.ok = ml.start(safe_require)
} catch (e) {
	console.log(e.message)
	exports.ok = false
}
