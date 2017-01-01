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

bool wrap_qRegisterResourceData(
	int version,
	const unsigned char *tree,
	const unsigned char *name,
	const unsigned char *data
	) {
	bool succ = Injector::instance().Wrapper->Call_qRegisterResourceData(version, tree, name, data);
	if (succ && QDir(":/qml").exists() && ModManager::instance().WaitingForRes()) {
		if (unsigned char * new_data = ModManager::instance().RunMods()) {
			Injector::instance().Wrapper->Call_qUnregisterResourceData(version, tree, name, data);
			if (!QResource::registerResource(new_data)) {
				LogManager::instance().log("Failed to modify resource");
			}
			return Injector::instance().Wrapper->Call_qRegisterResourceData(version, tree, name, data);
		} else {
			LogManager::instance().err("Cannot Load Mods");
		}
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
class WrappedLoad {
public:
	bool load(const QString & filename,
		const QString & directory,
		const QString & search_delimiters,
		const QString & suffix) {
		if (!Injector::instance().bQappTriggered) {
			Injector::instance().bQappTriggered = true;
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

		/*if (filename.endsWith("en_US") && uccuConfig::instance().enableLanguageFix()) {
			bool b = Injector::instance().Wrapper->Call_QTranslator_load(this, uccuConfig::instance().GetLanguageFile(), directory, search_delimiters, suffix);
			return b;
		}*/
		return Injector::instance().Wrapper->Call_QTranslator_load(this, filename, directory, search_delimiters, suffix);
	}
};

bool Injector::Init(IQt5Wrapper* wrapper) {
	// step1. init hooks
	wrapper->Set_qRegisterResourceData(wrap_qRegisterResourceData);
	auto addr = &WrappedLoad::load;
	wrapper->Set_QTranslator_load(*reinterpret_cast<p_QTranslator_load*>(&addr)); // mdzz
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