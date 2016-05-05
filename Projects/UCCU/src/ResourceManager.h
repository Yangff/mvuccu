#pragma once

#include <QtCore/qmap.h>
#include <QtCore/qdir.h>


class filenode;

class ResourceManager {
private:
	static ResourceManager * _instance;
    // QMap<QString, ModResourceInfo> _files;

	QByteArray m_data;
	quint32 m_treeOffset, m_dataOffset, m_namesOffset;
	filenode *m_root;
    
	ResourceManager();

public:
	static ResourceManager & instance() {
		if (!_instance) {
			return *(_instance = new ResourceManager());
		} else return *_instance;
	};

	const QByteArray GetFileContent(QString path);
	// bool LockQResource(QString path, int lockId = 0);
	// bool UnlockQResource(QString path, int lockId);
	bool UpdateFileContent(QString path, const QByteArray &content /* , int lockId = 0 */);
	bool AddFile(QString path, const QByteArray &data /* , int lockId = 0 */ );
	
	////// rcc
	
	void write16(quint16 number);
	void write32(quint32 number);
	void append(const QByteArray &x) {
		m_data.append(x);
	}
	void writeChar(char ch) { m_data.append(ch); }
	void writeStr(const char *str);

	filenode* GetFileInfo(QString path);

	QByteArray& finish();


	bool writeDataBlobs();
	bool writeDataNames();
	bool writeDataStructure();

};