#pragma once

class ScriptCore {
private:
	ScriptCore();
	static ScriptCore *_ins;
public:
	static ScriptCore& instance() { return *(_ins? _ins: _ins = new ScriptCore()); }
public:
	void RunScript();
};