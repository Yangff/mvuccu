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

	QString toStr();
};

class Dependence {
public:
	Requirement req;
	Version a;

public:
	QString toStr();
};

class Mod {
public:
	QString name;
	Version version;
	QDir path;
	QList<Dependence> deps;
	Dependence uccu;
public:
	bool check();
	Mod(QDir modPath);
	bool isMod();
};

class Loader {
private:
	QMap<QString, Mod> m_mMods;
public:
	Loader(QMap<QString, Mod>);
	bool next(QString, bool, QString&);
	bool first(QString&);
};