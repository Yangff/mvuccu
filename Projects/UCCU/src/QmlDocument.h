#pragma once

#include "QmlNode.h"

namespace Qml {
	struct Document {
		QSharedPointer<QmlNode> root;
		QList<QString> vImports;
		QList<QString> vParmas;

		QString GenCode();
	};
};