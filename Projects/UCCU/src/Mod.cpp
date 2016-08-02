// TODO: Give up all these f******* code and move to js

#include "Mod.h"

Loader::Loader(QMap<QString, Mod>)
{
}

bool Loader::next(QString, bool, QString &)
{
	return false;
}

bool Loader::first(QString &)
{
	return false;
}

bool Mod::check()
{
	return false;
}

Mod::Mod(QString modPath) {
	QFile f(modPath + "/mod.json");
	if (f.exists() && f.open(QIODevice::ReadOnly)) {
		f.readAll();
	}
}

Mod::Mod()
{
}

bool Mod::isMod()
{
	return false;
}

bool Version::match(const Requirement &, const Version a) const
{
	return false;
}

Version::Version(int a, int b, int c)
{
	ver[0] = a, ver[1] = b, ver[2] = c;
}

QString Version::toStr()
{
	return QString();
}

QString Dependence::toStr()
{
	return QString();
}
