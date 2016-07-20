#pragma once

#include <QtCore/QString>
#include <QtCore/qdir.h>
#include <QtCore/qmap>
#include <QtCore/Qlist>

enum Requirement {
	Equal, Less, Greater
};

class Version {
public:
	int ver[3]; // -1 for any
	
public:
	bool match(const Requirement&, const Version a) const;
	Version(int a=0, int b=0, int c=0);
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
	Mod();
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