#pragma once

#include "QmlNode.h"
#include "QmlDocument.h"

namespace Qml {
	class QmlProcessor {
	public:
		QmlProcessor(QString code);
		QSharedPointer<QmlNode> GenerateNode();
		Document* GenerateDocument();
		QString GetCode();
		QString * GetCodeRef();
	private:
		QString m_sCode;
	};
};
