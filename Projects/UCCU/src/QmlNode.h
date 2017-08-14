#pragma once

#include <QtCore/qstring>
#include <QtCore/qmap>
#include <QtCore/qlist>
#include <QtCore/qjsonobject>
#include <QtCore/QSet>
#include <QtCore/qpointer>
#include <QtCore/qdebug>

namespace Qml {

	static bool PAssert(bool b, QString s) { if (!b) { qDebug() << QString("[Assert] %1").arg(s); } return b; };

	class QmlNode {
	public:
		enum ValueType {
			NoValue, BindingObject, RawCode, Array
		};
		enum SymbolType {
			NoSymbol, Prop, Signal, Object, Function, NoNameObject /*placeholder while ast scan*/
		};
		struct Property {
			SymbolType eSymbolType;
			ValueType eValueType;
			struct Object {
				bool bHasOnToken;
			};
			struct Prop {
				bool bReadOnly;
				bool bDefault;
				QString retType;
			};
			struct Signal {};
			struct Function {};
			Object o; Prop p; Signal s; Function f;
			bool Assert(bool rt = false) {
				if (rt && eSymbolType == NoNameObject)
					return PAssert(false, "eSymbolType cannot be NoNameObject while runtime.");
				if (eSymbolType == SymbolType::NoSymbol) return PAssert(false, "eSymbolType cannot be No.");
				if (eSymbolType == SymbolType::Signal) return PAssert(eValueType == RawCode || eValueType == NoValue, "eValueType should equal RawCode or NoValue");
				if (eSymbolType == SymbolType::Function) return PAssert(eValueType == ValueType::RawCode, "eValueType should equal Function");
				if (eSymbolType == SymbolType::Object || eSymbolType == SymbolType::NoNameObject) return PAssert(eValueType != ValueType::NoValue, "eValueType should not be No");
				// if (eSymbolType == SymbolType::Array) return PAssert(eValueType == Objects || eValueType == RawCode, "eValueType should be bindings");
				return true;
			};

			void clear() {
				eSymbolType = SymbolType::NoSymbol;
				eValueType = ValueType::NoValue;
				o.bHasOnToken = false; p.bReadOnly = p.bDefault = false; p.retType.clear();
			}
		};
		struct Value {
			ValueType type;
			QSharedPointer<QmlNode> o;
			QList<QSharedPointer<QmlNode>> l;
			QString s;
			// QJsonObject* j;

			void clear() {
				o.clear(); l.clear(); s.clear();
				type = NoValue;
			}
		};

	private:
		// symbol table
		QMap<QString, int> m_mNames;
		QMap<int, QString> m_mNameIds;

		// Properties
		QMap<int, Property> m_mProperties;

		// objects
		QSet<QSharedPointer<QmlNode>> m_sObjects;

		// to erase a binding with its name
		QMap<QmlNode*, int> m_mBindings;

		// objects
		QList<QSharedPointer<QmlNode>> m_lObjects;

		// valuetype == object/objects
		QMap<int, QList<QSharedPointer<QmlNode>>> m_mlObjects;
		QMap<int, QSharedPointer<QmlNode>> m_mObjects;
		QMap<int, QList<QSharedPointer<QmlNode>>> m_lOnObjects;
		
		// valuetype == RawCode
		QMap<int, QString> m_mValues;

		// to select m_lFunctions;
		QList<int> m_lFunctions;

		/*
		// valuetype == JSON
		QMap<int, QJsonObject*> m_mJSONs;
		*/

		// variables list
		QList<QString> m_lVars;

		QString m_sTypeId;

		int m_iNameCnt;

		QWeakPointer<QmlNode> m_pParent;
		QWeakPointer<QmlNode> self;

	public:
		QString GenCode();
		
		QmlNode(QString typeId);
		void SetSelf(QWeakPointer<QmlNode> self) { this->self = self; }

	public:
		QString GetTypeId();
		void SetTypeId(QString);

		void AddObject(QSharedPointer<QmlNode>);

		/* will erase all element relate to the binding */
		void EraseObject(QSharedPointer<QmlNode>);

		void AddOnObject(QSharedPointer<QmlNode>, QSharedPointer<QmlNode>);

		void AddNameValueProperty(QString, Value, Property);

		void AddVar(QString);
		void EraseVar(QString);

		void AddUnnamedObject(QSharedPointer<QmlNode>);
		void AddUnnamedObjectBefore(QSharedPointer<QmlNode>, QSharedPointer<QmlNode>);

		void EraseByName(QString);
		QString GetNameByObject(QSharedPointer<QmlNode>);

		Value GetValueByName(QString);
		Property& GetPropertyByName(QString);
		Property& GetPropertyByObject(QSharedPointer<QmlNode>);
		void SetPropertyByName(QString, Property p);
		
		void ModifyValueByName(QString, Value);

		const QList<QSharedPointer<QmlNode>> GetObjects(); // xxx:XXX{} + XXX{} + [XXX{},YYY{}] on xxx
		const QList<QSharedPointer<QmlNode>> GetUnnamedObjects(); // XXX {} 

		const QList<QString> GetArrays(); // [XXX{}, YYY{}]
		/*const*/ QList<QString> GetVars();
		void SetVars(QList<QString> l);
		const QList<QString> GetFunctions();

		const QList<QSharedPointer<QmlNode>> GetOnObjects(QSharedPointer<QmlNode>);

		const QList<QString> GetNames(); // all possible names

		bool NameExists(QString name);
		bool ValueExists(QString name);

		int GetNameId(QString name);

		QWeakPointer<QmlNode> GetParent() { return m_pParent; };

	private:
		QString ProcessName(QString name);

	public:
		static QString ObjectWN(QSharedPointer<QmlNode> x);
		static QString ObjectsWN(QList<QSharedPointer<QmlNode>> x);
		QString GenCode(int id);
		bool HasCode(int id);
		void EraseFromSet(QSharedPointer<QmlNode> n);

	public:
		~QmlNode();
	};

}