/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQMLCONTEXT_H
#define QQMLCONTEXT_H

#include <QtCore/qurl.h>
#include <QtCore/qobject.h>
#include <QtQml/qjsvalue.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE


class QString;
class QQmlEngine;
class QQmlRefCount;
class QQmlContextPrivate;
class QQmlCompositeTypeData;
class QQmlContextData;

class Q_QML_EXPORT QQmlContext : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQmlContext)

public:
    QQmlContext(QQmlEngine *parent, QObject *objParent=0);
    QQmlContext(QQmlContext *parent, QObject *objParent=0);
    virtual ~QQmlContext();

    bool isValid() const;

    QQmlEngine *engine() const;
    QQmlContext *parentContext() const;

    QObject *contextObject() const;
    void setContextObject(QObject *);

    QVariant contextProperty(const QString &) const;
    void setContextProperty(const QString &, QObject *);
    void setContextProperty(const QString &, const QVariant &);

    QString nameForObject(QObject *) const;

    QUrl resolvedUrl(const QUrl &);

    void setBaseUrl(const QUrl &);
    QUrl baseUrl() const;

private:
    friend class QQmlEngine;
    friend class QQmlEnginePrivate;
    friend class QQmlExpression;
    friend class QQmlExpressionPrivate;
    friend class QQmlComponent;
    friend class QQmlComponentPrivate;
    friend class QQmlScriptPrivate;
    friend class QQmlContextData;
    QQmlContext(QQmlContextData *);
    QQmlContext(QQmlEngine *, bool);
    Q_DISABLE_COPY(QQmlContext)
};
QT_END_NAMESPACE

Q_DECLARE_METATYPE(QList<QObject*>)

#endif // QQMLCONTEXT_H
