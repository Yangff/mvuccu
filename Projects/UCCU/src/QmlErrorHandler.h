#pragma once
#include <QtCore/qobject.h>
#include <QtQml/qqmlapplicationengine.h>
#include <QtCore/qcoreapplication.h>

class QmlErrorHandler : public QObject
{
	Q_OBJECT
public:
	explicit QmlErrorHandler(QObject *parent = 0);

	private slots:
	void handleQmlErrors(const QList<QQmlError>& qmlErrors);
	void handleObjectCreated(QObject *object, const QUrl &url);

public:
	void Start(QQmlApplicationEngine *engine);
};