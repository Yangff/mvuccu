#include <QtCore/qfile.h>
#include <QtCore/qxmlstream.h>
#include <QtCore/qstringbuilder.h>
#include <QtCore/qdebug.h>
#include <QtQml/qjsengine>
#include <QtCore/qbytearray.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qdir.h>
#include <QtCore/qlocale.h>
#include <QtCore/qstack.h>
#include <QtCore/qbytearray.h>


#include "ResourceManager.h"
#include "ModLogger.h"
#include "ModManager.h"

ResourceManager *ResourceManager::_instance;


class filenode
{
public:
	enum Flags
	{
		NoFlags = 0x00,
		Directory = 0x02
	};

	filenode(const QString &name = QString(), const QByteArray &data = QByteArray(),
		uint flags = Directory);
	~filenode();

public:
	qint64 writeDataBlob(ResourceManager &lib, qint64 offset);
	qint64 writeDataName(ResourceManager &, qint64 offset);
	void writeDataInfo(ResourceManager &lib);

	int m_flags;
	QString m_name;
	QByteArray m_data;
	filenode *m_parent;
	QHash<QString, filenode*> m_children;

	qint64 m_nameOffset;
	qint64 m_dataOffset;
	qint64 m_childOffset;
};

/* Outside API */

unsigned char* ResourceManager::startResourceHook()
{
    _waitingForRes = false;
    
	AddFile(":/mod/version", QByteArray("2001", 4));

	ModManager::GetInstance().RunLoader();
    
    QByteArray & x = finish();

	int size = x.size();
    
    unsigned char * buff = new unsigned char[size];
	memset(buff, 0, size);
    memcpy(buff, x.constData(), sizeof(char) * x.size());

	if (m_root) {
		delete m_root;
		m_root = NULL;
	}
    return buff;
}

const QByteArray ResourceManager::GetFileContent(QString path)
{
	path = ":" + QDir::cleanPath(path.right(path.length() - 1));
    if (GetFileInfo(path) != NULL) {
        return GetFileInfo(path)->m_data;
    } else {
        QFile f(path);
        if (f.exists()) {
            f.open(QFile::ReadOnly);
            AddFile(path, f.readAll());
			f.close();
			return GetFileInfo(path)->m_data;
        }
    }
    return QByteArray();
}

bool ResourceManager::UpdateFileContent(QString path, const QByteArray &content)
{
    path = ":" + QDir::cleanPath(path.right(path.length() - 1));
	auto _file = GetFileInfo(path);
	if (!_file) {
		GetFileContent(path);
		if (!(_file = GetFileInfo(path))) return false;
	}
	_file->m_data = content;
	return true;
}

bool ResourceManager::AddFile(QString path, const QByteArray &data)
{
	path = ":" + QDir::cleanPath(path.right(path.length() - 1));

	if (!m_root)
		m_root = new filenode();

	auto parent = m_root;
	const QStringList nodes = path.split(QLatin1Char('/'));
	for (int i = 1; i < nodes.size() - 1; ++i) {
		auto node = nodes.at(i);
		if (node.isEmpty())
			continue;

		if (!parent->m_children.contains(node)) {
			auto ni = new filenode(node);
			ni->m_parent = parent; parent->m_children.insert(node, ni);
			parent = ni;
		}
		else parent = parent->m_children[node];
	}

	const QString filename = nodes.at(nodes.size() - 1);
	auto s = new filenode(filename, data, 0);
	s->m_parent = parent;
	parent->m_children.insertMulti(filename, s);
	return true;
}

bool ResourceManager::WaitingForRes()
{
    return _waitingForRes;
}


filenode * ResourceManager::GetFileInfo(QString path)
{
	if (!m_root)
		m_root = new filenode();

	auto parent = m_root;
	const QStringList nodes = path.split(QLatin1Char('/'));
	for (int i = 1; i < nodes.size(); ++i) {
		auto node = nodes.at(i);
		if (node.isEmpty())
			continue;

		if (!parent->m_children.contains(node)) {
			return nullptr;
		}
		else parent = parent->m_children[node];
	}
	if (parent->m_flags != 0)
		return nullptr;
	return parent;
}

/* Packager */

bool ResourceManager::writeDataBlobs()
{
	m_dataOffset = m_data.size();
	if (!m_root)
		return false;
	QStack<filenode*> pending;
	pending.push(m_root);
	qint64 offset = 0;
	while (!pending.isEmpty()) {
		auto file = pending.pop();
		for (auto x : file->m_children) {
			if (x->m_flags == filenode::Directory) {
				pending.push(x);
			}
			else {
				offset = x->writeDataBlob(*this, offset);
				if (offset == 0)
					return false;
			}

		}
	}
	return true;
}

bool ResourceManager::writeDataNames()
{
	m_namesOffset = m_data.size();
	if (!m_root)
		return false;
	QHash<QString, int> names;
	QStack<filenode*> pending;
	pending.push(m_root);
	qint64 offset = 0;
	while (!pending.isEmpty()) {
		auto file = pending.pop();
		for (auto x : file->m_children) {
			if (x->m_flags == filenode::Directory) {
				pending.push(x);
			}
			if (names.contains(x->m_name)) {
				x->m_nameOffset = names.value(x->m_name);
			}
			else {
				names.insert(x->m_name, offset);
				x->m_nameOffset = offset;
				offset = x->writeDataName(*this, offset);
			}
		}
	}
	return true;
}

static bool qt_rcc_compare_hash(const filenode *lsh, const filenode *rsh)
{
	return qt_hash(lsh->m_name) < qt_hash(rsh->m_name);
}

bool ResourceManager::writeDataStructure()
{
	m_treeOffset = m_data.size();
	if (!m_root)
		return false;

	QStack<filenode*> pending;
	pending.push(m_root);
	qint64 offset = 1;
	while (!pending.isEmpty()) {
		auto file = pending.pop();
		file->m_childOffset = offset;
		QList<filenode*> m_children = file->m_children.values();
		std::sort(m_children.begin(), m_children.end(), qt_rcc_compare_hash);
        for (auto x : m_children) {
            offset++;
			if (x->m_flags == filenode::Directory) {
				pending.push(x);
			}
		}
	}

	pending.push(m_root);
	m_root->writeDataInfo(*this);
	while (!pending.isEmpty()) {
		auto file = pending.pop();
		QList<filenode*> m_children = file->m_children.values();
		std::sort(m_children.begin(), m_children.end(), qt_rcc_compare_hash);
		for (auto x : m_children) {
			x->writeDataInfo(*this);
			if (x->m_flags == filenode::Directory) {
				pending.push(x);
			}
		}
	}
	return true;
}



void ResourceManager::write16(quint16 number)
{
	writeChar(number >> 8);
	writeChar(number);
}

void ResourceManager::write32(quint32 number)
{
	writeChar(number >> 24);
	writeChar(number >> 16);
	writeChar(number >> 8);
	writeChar(number);
}

void ResourceManager::writeStr(const char *str)
{
	int n = m_data.size();
	m_data.resize(n + strlen(str));
	memcpy(m_data.data() + n, str, strlen(str));
}


QByteArray & ResourceManager::finish()
{

	writeDataBlobs();
	writeDataNames();
	writeDataStructure();

	////...
	int i = 4;
	char *p = m_data.data();
	p[i++] = 0; // 0x01
	p[i++] = 0;
	p[i++] = 0;
	p[i++] = 1;

	p[i++] = (m_treeOffset >> 24) & 0xff;
	p[i++] = (m_treeOffset >> 16) & 0xff;
	p[i++] = (m_treeOffset >> 8) & 0xff;
	p[i++] = (m_treeOffset >> 0) & 0xff;

	p[i++] = (m_dataOffset >> 24) & 0xff;
	p[i++] = (m_dataOffset >> 16) & 0xff;
	p[i++] = (m_dataOffset >> 8) & 0xff;
	p[i++] = (m_dataOffset >> 0) & 0xff;

	p[i++] = (m_namesOffset >> 24) & 0xff;
	p[i++] = (m_namesOffset >> 16) & 0xff;
	p[i++] = (m_namesOffset >> 8) & 0xff;
	p[i++] = (m_namesOffset >> 0) & 0xff;

	return m_data;
}

ResourceManager::ResourceManager(): _waitingForRes(true) {
	m_root = NULL;
	m_treeOffset = m_dataOffset = m_namesOffset = 0;
	writeStr("qres");
	write32(0);
	write32(0);
	write32(0);
	write32(0);
}

/* Filenode */

filenode::filenode(const QString &name, const QByteArray &data, uint flags) {
	m_name = name;
	m_data = data;
	m_flags = flags;
	m_parent = 0;
	m_nameOffset = 0;
	m_dataOffset = 0;
	m_childOffset = 0;
}

filenode::~filenode()
{
	qDeleteAll(m_children);
}

void filenode::writeDataInfo(ResourceManager &lib)
{
	//pointer data
	if (m_flags & filenode::Directory) {
		// name offset
		lib.write32(m_nameOffset);
		// flags
		lib.write16(m_flags);
		// child count
		lib.write32(m_children.size());
		// first child offset
		lib.write32(m_childOffset);
	}
	else {
		// name offset
		lib.write32(m_nameOffset);
		// flags
		lib.write16(m_flags);
		// locale
		lib.write16(QLocale::AnyCountry);
		lib.write16(QLocale::C);
		//data offset
		lib.write32(m_dataOffset);
	}
}


qint64 filenode::writeDataBlob(ResourceManager &lib, qint64 offset)
{
	const bool binary = true;
	m_dataOffset = offset;
	QByteArray data = m_data;
	lib.write32(data.size());
	offset += 4;
	const char *p = data.constData();
	lib.append(data);
	offset += data.size();
	return offset;
}

qint64 filenode::writeDataName(ResourceManager &lib, qint64 offset)
{
	// capture the offset
	m_nameOffset = offset;

	// write the length
	lib.write16(m_name.length());

	offset += 2;

	// write the hash
	lib.write32(qt_hash(m_name));

	offset += 4;

	// write the m_name
	const QChar *unicode = m_name.unicode();
	for (int i = 0; i < m_name.length(); ++i) {
		lib.write16(unicode[i].unicode());
	}
	offset += m_name.length() * 2;
	return offset;
}
