/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QIBUSTYPES_H
#define QIBUSTYPES_H

#include <qvector.h>
#include <qevent.h>
#include <QDBusArgument>
#include <QTextCharFormat>
#include <QLoggingCategory>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qtQpaInputMethods)
Q_DECLARE_LOGGING_CATEGORY(qtQpaInputMethodsSerialize)

class QIBusSerializable
{
public:
    QIBusSerializable();
    ~QIBusSerializable();

    void serializeTo(QDBusArgument &argument) const;
    void deserializeFrom(const QDBusArgument &argument);

    QString name;
    QHash<QString, QDBusArgument>  attachments;
};

class QIBusAttribute : private QIBusSerializable
{
public:
    enum Type {
        Invalid = 0,
        Underline = 1,
        Foreground = 2,
        Background = 3,
    };

    enum Underline {
        UnderlineNone = 0,
        UnderlineSingle  = 1,
        UnderlineDouble  = 2,
        UnderlineLow = 3,
        UnderlineError = 4,
    };

    QIBusAttribute();
    ~QIBusAttribute();

    QTextCharFormat format() const;

    void serializeTo(QDBusArgument &argument) const;
    void deserializeFrom(const QDBusArgument &argument);

    Type type;
    quint32 value;
    quint32 start;
    quint32 end;
};

class QIBusAttributeList : private QIBusSerializable
{
public:
    QIBusAttributeList();
    ~QIBusAttributeList();

    QList<QInputMethodEvent::Attribute> imAttributes() const;

    void serializeTo(QDBusArgument &argument) const;
    void deserializeFrom(const QDBusArgument &argument);

    QVector<QIBusAttribute> attributes;
};

class QIBusText : private QIBusSerializable
{
public:
    QIBusText();
    ~QIBusText();

    void serializeTo(QDBusArgument &argument) const;
    void deserializeFrom(const QDBusArgument &argument);

    QString text;
    QIBusAttributeList attributes;
};

class QIBusEngineDesc : private QIBusSerializable
{
public:
    QIBusEngineDesc();
    ~QIBusEngineDesc();

    void serializeTo(QDBusArgument &argument) const;
    void deserializeFrom(const QDBusArgument &argument);

    QString engine_name;
    QString longname;
    QString description;
    QString language;
    QString license;
    QString author;
    QString icon;
    QString layout;
    unsigned int rank;
    QString hotkeys;
    QString symbol;
    QString setup;
    QString layout_variant;
    QString layout_option;
    QString version;
    QString textdomain;
    QString iconpropkey;
};

inline QDBusArgument &operator<<(QDBusArgument &argument, const QIBusAttribute &attribute)
{ attribute.serializeTo(argument); return argument; }
inline const QDBusArgument &operator>>(const QDBusArgument &argument, QIBusAttribute &attribute)
{ attribute.deserializeFrom(argument); return argument; }

inline QDBusArgument &operator<<(QDBusArgument &argument, const QIBusAttributeList &attributeList)
{ attributeList.serializeTo(argument); return argument; }
inline const QDBusArgument &operator>>(const QDBusArgument &argument, QIBusAttributeList &attributeList)
{ attributeList.deserializeFrom(argument); return argument; }

inline QDBusArgument &operator<<(QDBusArgument &argument, const QIBusText &text)
{ text.serializeTo(argument); return argument; }
inline const QDBusArgument &operator>>(const QDBusArgument &argument, QIBusText &text)
{ text.deserializeFrom(argument); return argument; }

inline QDBusArgument &operator<<(QDBusArgument &argument, const QIBusEngineDesc &desc)
{ desc.serializeTo(argument); return argument; }
inline const QDBusArgument &operator>>(const QDBusArgument &argument, QIBusEngineDesc &desc)
{ desc.deserializeFrom(argument); return argument; }

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QIBusAttribute)
Q_DECLARE_METATYPE(QIBusAttributeList)
Q_DECLARE_METATYPE(QIBusText)
Q_DECLARE_METATYPE(QIBusEngineDesc)

#endif
