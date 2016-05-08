#pragma once

#include <QtCore/QString>
#include <QtCore/qdir.h>
#include <QtCore/Qlist>

enum Requirement {
	Equal, Less, Greater
};

class Version {
public:
	int ver[3]; // -1 for any
	
public:
	bool match(const Requirement&, const Version a) const;
};

class Dependence {
	Requirement req;
	Version a, b;
};

class Mod {
public:
	QString name;
	Version version;
	QDir path;
	QList<Dependence> deps;
	
public:
	bool check();
	Mod(QDir modPath);
	bool isMod();
};
