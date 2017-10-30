#include "Injector.h"

#include <QtCore/qtranslator.h>
#include <QtCore/qstring.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qresource.h>
#include <QtCore/qdir.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qdiriterator.h>
#include <QtCore/qloggingcategory.h>
//#include <QtCore/5.4.1/QtCore/private/qhooks_p.h>

// #include "QmlErrorHandler.h"
#include "uccuConfig.h"
#include "LogManager.h"
#include "ModManager.h"

Injector* Injector::ins = NULL;
// QmlErrorHandler *qeh;
QMap<QString, int> *categoryEnabler;
//QSet<QObject*> *objects;
bool bQappTriggered;

struct fingerprint {
	bool v, vp;
	int version;
	const unsigned char *tree;
	const unsigned char *name;
	const unsigned char *data;
} fp;

bool QApplicationReady() {
	if (fp.v && !fp.vp) {
		fp.vp = true;
		LogManager::instance().log("Start mods");
		Injector::instance().Wrapper->Call_qRegisterResourceData(fp.version, fp.tree, fp.name, fp.data);
		if (unsigned char * new_data = ModManager::instance().RunMods()) {
			Injector::instance().Wrapper->Call_qUnregisterResourceData(fp.version, fp.tree, fp.name, fp.data);
			if (!QResource::registerResource(new_data)) {
				LogManager::instance().log("Failed to modify resource");
			}
			return Injector::instance().Wrapper->Call_qRegisterResourceData(fp.version, fp.tree, fp.name, fp.data);
		} else {
			LogManager::instance().err("Cannot Load Mods");
		}
	}
	return false;
}

bool wrap_qRegisterResourceData(
	int version,
	const unsigned char *tree,
	const unsigned char *name,
	const unsigned char *data
	) {
	bool succ = Injector::instance().Wrapper->Call_qRegisterResourceData(version, tree, name, data);
	if (succ && QDir(":/qml").exists() && ModManager::instance().WaitingForRes()) {
		ModManager::instance().MarkFound();
		fp.v = true; fp.vp = false; fp.version = version; fp.tree = tree; fp.name = name; fp.data = data;
		LogManager::instance().log("Resource hooked");
		Injector::instance().Wrapper->Call_qUnregisterResourceData(version, tree, name, data);
		/*
		if (unsigned char * new_data = ModManager::instance().RunMods()) {
			Injector::instance().Wrapper->Call_qUnregisterResourceData(version, tree, name, data);
			if (!QResource::registerResource(new_data)) {
				LogManager::instance().log("Failed to modify resource");
			}
			return Injector::instance().Wrapper->Call_qRegisterResourceData(version, tree, name, data);
		} else {
			LogManager::instance().err("Cannot Load Mods");
		}
		*/
	}
	return succ;
}

void categoryFilterForceLog(QLoggingCategory *category) {
	int x = uccuConfig::instance().GetCategoryMode(category->categoryName());
	category->setEnabled(QtDebugMsg, x & 1);
	category->setEnabled(QtCriticalMsg, x & 2);
	category->setEnabled(QtWarningMsg, x & 4);
	category->setEnabled(QtFatalMsg, x & 8);
}
/*
static void objectAddHook(QObject* o) {
	objects->insert(o);
}

static void objectRemoveHook(QObject* o) {
	objects->remove(o);
}
*/

QMap<QString, QString> _repMap;
QMap<QString, QTranslator*> _newTrans;
bool Injector::addTranslator(QString fn) {
	if (_repMap.contains(fn))
		_repMap.erase(_repMap.find(fn));
	if (!_newTrans.contains(fn))
		_newTrans[fn] = (new QTranslator());
	return Injector::instance().Wrapper->Call_QTranslator_load(_newTrans[fn], fn, QString(), QString(), QString());
	
}

void Injector::replaceTranslator(QString a, QString b) {
	_repMap[a] = b;
}

class WrappedLoad {
public:
	bool load(const QString & filename,
		const QString & directory,
		const QString & search_delimiters,
		const QString & suffix) {
		QString new_name = filename;

		if (_repMap.find(filename) != _repMap.end()) {
			new_name = _repMap[filename];
		}
		if (!Injector::instance().bQappTriggered) {
			Injector::instance().bQappTriggered = true;
			LogManager::instance().log("Translate hooked");
			/*for (auto x : *objects) {
				if (QQmlApplicationEngine *qapp = qobject_cast<QQmlApplicationEngine*>(x)) {
					qeh->Start(qapp);
				}
			}*/

			//objects->clear();

			//qtHookData[QHooks::AddQObject] = qtHookData[QHooks::RemoveQObject] = 0;
			//qtHookData[QHooks::RemoveQObject] = qtHookData[QHooks::RemoveQObject] = 0;

			qInstallMessageHandler(&LogManager::qtMessageHandler);
		}
		if (!filename.endsWith("cocoa")) {
			// loading language pack
			bool b = false;
			if (uccuConfig::instance().enableLanguageFix())
				b = Injector::instance().Wrapper->Call_QTranslator_load(this, uccuConfig::instance().GetLanguageFile(), directory, search_delimiters, suffix);
			else {
				b = Injector::instance().Wrapper->Call_QTranslator_load(this, new_name, directory, search_delimiters, suffix);
			}
			for (auto& kv = _newTrans.begin(); kv != _newTrans.end(); kv++) {
				auto &x = kv.value();
				if (_repMap.find(kv.key()) != _repMap.end()) {
					Injector::instance().Wrapper->Call_QTranslator_load(x, kv.key(), QString(), QString(), QString());
				}
				QCoreApplication::installTranslator(x);
			}
			return b;
		}
		return Injector::instance().Wrapper->Call_QTranslator_load(this, new_name, directory, search_delimiters, suffix);
	}
};

bool Injector::Init(IQt5Wrapper* wrapper) {
	// step1. init hooks
	wrapper->Set_qRegisterResourceData(wrap_qRegisterResourceData);
	auto addr = &WrappedLoad::load;
	wrapper->Set_QTranslator_load(*reinterpret_cast<p_QTranslator_load*>(&addr)); // mdzz
	wrapper->Set_AfterQCoreApplication_setOrganizationDomain((p_Callback)QApplicationReady);
	Wrapper = wrapper;
	
	// step2. init
	qInstallMessageHandler(&platformQtMessageHandler);

	QLoggingCategory::installFilter(categoryFilterForceLog);

//	qeh = new QmlErrorHandler;
	//objects = new QSet<QObject*>;
	//categoryEnabler = new QMap<QString, int>;

	//qtHookData[QHooks::AddQObject] = reinterpret_cast<quintptr>(&objectAddHook);
	//qtHookData[QHooks::RemoveQObject] = reinterpret_cast<quintptr>(&objectRemoveHook);

	bQappTriggered = false;

	return true;
}

bool Injector::OnExit() {
//	delete qeh;
//	delete objects;
	return true;
}