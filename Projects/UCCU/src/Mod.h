#pragma once

#include <QtCore/QString>
#include <QtCore/qdir.h>
#include <QtCore/Qlist>

enum Requirement {
	Equal, Less, Greater, Range
};

class Version {
public:
	int ver[3]; // -1 for any
	
public:
	bool match(const Requirement&, const Version a) const;
	bool range(const Version a, const Version b) const;
};

class Dependence {
	Requirement req;
	Version a, b;
};

class Mod {
public:
	QString modName;
	Version modVersion;
	QDir modPath;
	QList<Dependence> deps;

public:
	bool check();
};