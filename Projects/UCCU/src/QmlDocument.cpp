#include "QmlDocument.h"

#include <QtCore/qstringbuilder>

namespace Qml {
	QString padding(QString org, int l) {
		QString s = ""; while (l-- > 0) s = s + "  ";
		return s + org;
	}
}

QString Qml::Document::GenCode() {
	QString code;
	for (auto x : vParmas) {
		code = code % x % "\n";
	}

	for (auto x : vImports) {
		code = code % x % "\n";
	}

	code = code % (root ? QmlNode::ObjectWN(root) : "");

	auto lines = code.splitRef("\n");
	int status = 0, depth = 0;

	QStringList ret; int inComment = 0, inStr = 0;

	for (auto x : lines) {
		if (x.trimmed().isEmpty()) continue;
		auto y = x.trimmed();
		if (!inComment && y.startsWith("}"))
			ret << padding(y.toString(), depth - 1);
		else
			ret << padding(y.toString(), depth);
		for (auto z : y) {
			if (inComment) {
				if (inComment == 2 && z == '*') {
					inComment = 3;
				}
				else
					if (inComment == 3 && z != '/') {
						inComment = 2;
					}
					else
						if (inComment == 3 && z == '/') {
							inComment = 0;
						}
						else if (z == '/' && inComment == 1) {
							inComment = 0; break;
						}
						else if (z == '*' && inComment == 1) {
							inComment = 2;
						}
						else inComment = 0;
						if (inComment)
							continue;
			}

			if (status == 0) {
				if (z == '{') {
					depth++;
				}
				if (z == '}') {
					depth--;
				}
				if (z == '/' && inComment == 0)
					inComment = 1;

			}
			if (status == 2) {
				status = 0; continue;
			}
			if (z == '\\' && status == 1) {
				status = 2;
				continue;
			}
			if (z == '\'' || z == '\"') {
				if (inStr == 0) {
					inStr = (z == '\'' ? 1 : 2);
					status = 1;
				}
				else {
					status ^= (inStr == ((z == '\'') ? 1 : 2));
					inStr = 0;
				}
				continue;
			}
		}
		
	}
	return ret.join("\n");
}
