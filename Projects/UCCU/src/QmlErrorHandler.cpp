#include "QmlErrorHandler.h"

#include "QmlErrorHandler.moc"

#include <QtCore/qdebug.h>

QmlErrorHandler::QmlErrorHandler(QObject *parent) :QObject(parent) {}

void QmlErrorHandler::handleObjectCreated(QObject * object, const QUrl & url) {
	if (!object) {
		qDebug() << QString("%1 seems load failed").arg(url.toString());
	}
}

void QmlErrorHandler::handleQmlErrors(const QList<QQmlError>& qmlErrors) {
	QStringList errors;
	for (const QQmlError& error : qmlErrors) {
		// Special case for bug in QtComponents 1.1
		// https://bugreports.qt-project.org/browse/QTCOMPONENTS-1217
		if (error.url().toString().endsWith("PageStackWindow.qml") && error.line() == 70)
			continue;
		errors.append(error.toString());
	}
	qDebug() << errors.join("\n");
}

void QmlErrorHandler::Start(QQmlApplicationEngine *engine) {
	connect(engine, &QQmlApplicationEngine::warnings, this, &QmlErrorHandler::handleQmlErrors);
	connect(engine, &QQmlApplicationEngine::objectCreated, this, &QmlErrorHandler::handleObjectCreated);
}