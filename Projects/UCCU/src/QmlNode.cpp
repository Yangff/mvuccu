#include <QtCore/qmap.h>
#include "QmlNode.h"

#include <QtCore/qstringbuilder>

using namespace Qml;

struct Sorter {
	QMap<QString, int> &mm;
	bool operator () (const QString &l, const QString &r) {
		return mm[l] < mm[r];
	}
	Sorter(QMap<QString, int> &mm) :mm(mm) {};
};

Qml::QmlNode::QmlNode(QString typeId):m_sTypeId(typeId), m_iNameCnt(0), m_pParent(){ }

QString Qml::QmlNode::GetTypeId() {
	return m_sTypeId;
}

void Qml::QmlNode::SetTypeId(QString s) {
	m_sTypeId = s;
}

void Qml::QmlNode::AddObject(QSharedPointer<QmlNode> n) {
	if (m_sObjects.find(n) != m_sObjects.end()) {
		return;
	}
	if (n->m_pParent && n->m_pParent.data() != this) {
		n->m_pParent.data()->EraseObject(n);
	}
	if (m_sTypeId != "__[PlaceHolder]__")
		n->m_pParent = self;
	m_sObjects.insert(n);
}

#define FindAndErase(x,y) do {auto _ = x.find(y); if (_ != x.end()) x.erase(_); } while(0);
#define FindAndEraseObj(x,y) do {auto _ = x.find(y); if (_ != x.end()) {EraseFromSet(*_);x.erase(_);} } while(0);
void Qml::QmlNode::EraseObject(QSharedPointer<QmlNode> n) {
	EraseFromSet(n);

	auto _ni = m_mBindings.find(n.data());
	if (_ni != m_mBindings.end()) {
		// named
		int ni = *_ni;
		auto _s = m_mNameIds.find(ni);
		if (_s != m_mNameIds.end()) {
			QString s = *_s;
			// erase all object on it
			if (m_mObjects.find(ni) != m_mObjects.end() && m_mObjects[ni] == n) {
				// name's first binding is obj
				FindAndErase(m_lOnObjects, ni);
				EraseByName(s);
			} else {
				// or is array or on object.
				auto l = m_lOnObjects.find(ni);
				if (l != m_lOnObjects.end()) {
					for (auto z = l->begin(); z != l->end(); z++)
						if (*z == n) {
							l->erase(z);
							return;
						}
				}

				auto q = m_mlObjects.find(ni);
				if (q != m_mlObjects.end()) {
					for (auto z = q->begin(); z != q->end(); z++)
						if (*z == n) {
							q->erase(z);
							return;
						}
				}
			}
		}
	} else {
		// unnamed

		for (auto x = m_lObjects.begin(); x != m_lObjects.end(); x++) {
			if (*x == n) {
				m_lObjects.erase(x);
				return;
			}
		}
	}
}


void Qml::QmlNode::AddOnObject(QSharedPointer<QmlNode> x, QSharedPointer<QmlNode> y){
	auto id = m_mBindings.find(x.data());
	if (id != m_mBindings.end()) {
		AddObject(y);
		m_lOnObjects[*id].append(y);
		m_mBindings[y.data()] = *id;
	}
}

void Qml::QmlNode::AddVar(QString v) {
	m_lVars.push_back(v);
}

void Qml::QmlNode::EraseVar(QString v) {
	for (QList<QString>::iterator x = m_lVars.begin(); x != m_lVars.end(); x++) {
		if (*x == v) {
			m_lVars.erase(x);
			return;
		}
	}
}

void Qml::QmlNode::AddUnnamedObject(QSharedPointer<QmlNode> x) {
	EraseObject(x);
	AddObject(x);
	m_lObjects.push_back(x);
	return ;
}

void Qml::QmlNode::AddUnnamedObjectBefore(QSharedPointer<QmlNode> x, QSharedPointer<QmlNode> y)
{
	for (auto it = m_lObjects.begin(); it != m_lObjects.end(); it++)
		if (*it == y) {
			EraseObject(x);
			AddObject(x);
			m_lObjects.insert(it, x);
			return;
		}
}

void Qml::QmlNode::EraseByName(QString name) {
	auto _id = m_mNames.find(name);
	if (_id != m_mNames.end()) {
		int id = *_id;
		// name
		m_mNames.erase(_id);
		FindAndErase(m_mNameIds, id);

		// property
		FindAndErase(m_mProperties, id);

		// binding
		if (m_mObjects.find(id) != m_mObjects.end()) {
			EraseFromSet(m_mObjects[id]);
			FindAndErase(m_mBindings, m_mObjects[id].data());
		}

		// object 
		FindAndErase(m_mObjects, id);

		// on object
		if (m_lOnObjects.find(id) != m_lOnObjects.end()) {
			for (auto x : m_lOnObjects[id]) {
				EraseFromSet(x);
			}
		}
		
		FindAndErase(m_lOnObjects, id);

		// function
		for (auto o = m_lFunctions.begin(); o != m_lFunctions.end(); o++)
			if (*o == id) {
				m_lFunctions.erase(o);
				break;
			}

		// array
		auto x = m_mlObjects.find(id);
		if (x != m_mlObjects.end()) {
			for (auto y : *x)
				EraseFromSet(y);
			m_mlObjects.erase(x);
		}

		// value
		FindAndErase(m_mValues, id);
	}
}

QString Qml::QmlNode::GetNameByObject(QSharedPointer<QmlNode> x)
{
	return m_mNameIds[m_mBindings[x.data()]];
}

#define Find(x, y, t, c) do {auto _ = x.find(y); if (_ != x.end()) { \
 Qml::QmlNode::Value v; v.type = t; \
 c; \
 return v; \
} } while(0);

Qml::QmlNode::Value Qml::QmlNode::GetValueByName(QString name) {
	Qml::QmlNode::Value v; v.clear();  v.type = NoValue;
	auto _id = m_mNames.find(name);
	if (_id != m_mNames.end()) {
		int id = *_id;
		auto p = m_mProperties.find(*_id);
		if (p != m_mProperties.end()) {
			auto q = *p;
			Find(m_mObjects, id, q.eValueType, v.o = *_);
			Find(m_mValues, id, q.eValueType, v.s = *_);
			Find(m_mlObjects, id, q.eValueType, v.l = *_);
		}
	}
	return v;
}

Qml::QmlNode::Property& Qml::QmlNode::GetPropertyByName(QString name)
{
	Qml::QmlNode::Property pro; 
	pro.clear();
	pro.eSymbolType = NoSymbol; pro.eValueType = NoValue;
	auto _id = m_mNames.find(name);
	if (_id != m_mNames.end()) {
		int id = *_id;
		auto p = m_mProperties.find(*_id);
		if (p != m_mProperties.end()) {
			return *p;
		}
	}
	return pro;
}

Qml::QmlNode::Property& Qml::QmlNode::GetPropertyByObject(QSharedPointer<QmlNode> ptr) {
	Qml::QmlNode::Property pro; 
	pro.clear();
	pro.eSymbolType = NoSymbol; pro.eValueType = NoValue;
	auto _id = m_mBindings.find(ptr.data());
	if (_id != m_mBindings.end()) {
		int id = *_id;
		auto p = m_mProperties.find(*_id);
		if (p != m_mProperties.end()) {
			return *p;
		}
	}
	return pro;
}

void Qml::QmlNode::SetPropertyByName(QString name, Qml::QmlNode::Property p)
{
	auto _id = m_mNames.find(name);
	if (_id != m_mNames.end()) {
		int id = *_id;
		m_mProperties[id] = p;
	}
}

void Qml::QmlNode::ModifyValueByName(QString name, Value val) {
	auto _id = m_mNames.find(name);
	if (_id != m_mNames.end()) {
		auto _p = m_mProperties.find(*_id);
		if (_p != m_mProperties.end()) {
			auto p = *_p;
			if (p.eValueType != val.type) {
				p.eValueType = val.type;
				if (!p.Assert())
					return;
			}
			EraseByName(name);
			AddNameValueProperty(name, val, p);
		}
	}
}

const QList<QSharedPointer<QmlNode>> Qml::QmlNode::GetUnnamedObjects() {
	return m_lObjects;
}

const QList<QSharedPointer<QmlNode>> Qml::QmlNode::GetObjects() {
	return m_sObjects.toList();
}

const QList<QString> Qml::QmlNode::GetArrays() {
	QList<QString> s;
	for (auto x : m_mlObjects.keys()) {
		s.push_back(*m_mNameIds.find(x));
	}
	return s;
}

const QList<QString> Qml::QmlNode::GetVars(){
	return m_lVars;
}

void Qml::QmlNode::SetVars(QList<QString> l)
{
	m_lVars = l;
}

const QList<QString> Qml::QmlNode::GetFunctions() {
	QList<QString> s;
	for (auto x : m_lFunctions)
		s.push_back(*m_mNameIds.find(x));
	return s;
}


const QList<QSharedPointer<QmlNode>> Qml::QmlNode::GetOnObjects(QSharedPointer<QmlNode> x) {
	auto y = m_mBindings.find(x.data());
	if (y != m_mBindings.end())
		return m_lOnObjects[*y];
	return QList<QSharedPointer<QmlNode>>();
}

const QList<QString> Qml::QmlNode::GetNames() {
	QList<QString> nameList = m_mNames.keys();
	qSort(nameList.begin(), nameList.end(), Sorter(m_mNames));
	return nameList;
}

bool Qml::QmlNode::NameExists(QString name)
{
	return m_mNames.find(name) != m_mNames.end();
}

bool Qml::QmlNode::ValueExists(QString name)
{
	return NameExists(name) && GetValueByName(name).type != ValueType::NoValue;
}

int Qml::QmlNode::GetNameId(QString name)
{
	if (m_mNames.find(name) != m_mNames.end())
		return m_mNames[name];
	else
		return 0;
}

void Qml::QmlNode::AddNameValueProperty(QString name, QmlNode::Value v, Property p) {
	int cid;
	if (p.eValueType != v.type) {
		p.eValueType = v.type;
		qDebug() << "[WARN] AddNameValueProperty with p.eValueType != v.type, try fixit.";
		if (!p.Assert())
			return;
	}
	if (m_mNames.find(name) != m_mNames.end()) {
		cid = *m_mNames.find(name);
		if (!(p.eSymbolType == Object && p.o.bHasOnToken && v.type == BindingObject)) {
			EraseByName(name);
			m_mNames[name] = cid;
			m_mNameIds[cid] = name;
		}
	} else {
		cid = ++m_iNameCnt;
		m_mNames[name] = cid;
		m_mNameIds[cid] = name;
	}

	if (!(p.eSymbolType == Object && p.o.bHasOnToken))
		m_mProperties[cid] = p;

	if (v.type == QmlNode::ValueType::BindingObject) {
		AddObject(v.o);
		if (p.eSymbolType == Object && p.o.bHasOnToken && v.type == BindingObject)
			m_lOnObjects[cid].append(v.o);
		else 
			m_mObjects[cid] = v.o;
		m_mBindings[v.o.data()] = cid;
	}

	if (p.eSymbolType == Function) {
		m_lFunctions.push_back(cid);
	}

	if (v.type == QmlNode::ValueType::RawCode) {
		m_mValues[cid] = v.s;
	}

	if (v.type == QmlNode::ValueType::Array) {
		for (auto x : v.l) {
			AddObject(x);
			m_mBindings[x.data()] = cid;
		}
		m_mlObjects[cid] = v.l;
	}
}

QString Qml::QmlNode::ProcessName(QString name) {
	int id = m_mNames[name];
	auto p = m_mProperties[id];
	QString code = "";
	
	switch (p.eSymbolType)
	{
	case QmlNode::SymbolType::Prop:
		if (p.p.bDefault)
			code = "default ";
		if (p.p.bReadOnly)
			code = code % "readonly ";
		code = code % "property ";
		code = code % p.p.retType;
		code = code % " " % name;
		if (HasCode(id))
			code = code % ":" % GenCode(id);
		break;
	case QmlNode::SymbolType::Signal:
		code = "signal " % name % "(" % GenCode(id) % ")"; break;
	case QmlNode::SymbolType::Object: {
		code = name % " : " % GenCode(id);
		auto x = m_lOnObjects.find(id);
		if (x != m_lOnObjects.end()) {
			code = code % "\n";
			for (auto y : m_lOnObjects[id]) {
				code = code % (y->GetTypeId() % " on " % name % " " % "{\n" % y->GenCode() % "}") % "\n";
			}
		}
		break;
	}
	case QmlNode::SymbolType::Function:
		return GenCode(id);
	case QmlNode::SymbolType::NoSymbol:
	default:
		return "";
	}
	return code;
}

QString Qml::QmlNode::ObjectWN(QSharedPointer<QmlNode> x) {
	return x->GetTypeId() % " {\n" % (x)->GenCode() % "\n}";
}

QString Qml::QmlNode::ObjectsWN(QList<QSharedPointer<QmlNode>> x) {
	QStringList sl;
	for (auto y : x) {
		sl << ObjectWN(y);
	}
	return "[" + sl.join(", \n") + "]";
}

bool Qml::QmlNode::HasCode(int id) {
	return m_mProperties[id].eValueType != QmlNode::ValueType::NoValue;
}

QString Qml::QmlNode::GenCode(int id) {
	auto p = m_mProperties[id];
	switch (p.eValueType) {
	case QmlNode::ValueType::BindingObject:
		return ObjectWN(m_mObjects[id]);
	case QmlNode::ValueType::Array:
		return ObjectsWN(m_mlObjects[id]);
	case QmlNode::ValueType::RawCode:
		return m_mValues[id];
	case QmlNode::ValueType::NoValue:
	default:
		return "";
	}
}

void Qml::QmlNode::EraseFromSet(QSharedPointer<QmlNode> n){
	auto b = m_sObjects.find(n);
	// erase from set.
	if (b != m_sObjects.end()) {
		QSharedPointer<QmlNode> x = *b;
		x->m_pParent.clear();
		m_sObjects.erase(b);
	}

}

Qml::QmlNode::~QmlNode() {
	for (auto x : m_lObjects)
		x->m_pParent.clear();
}

QString Qml::QmlNode::GenCode() {
	QString code;
	for (auto x : m_lVars)
		code = code % x % "\n";
	QList<QString> nameList = GetNames();
	for (auto x : nameList) {
		code = code % ProcessName(x) % "\n";
	}
	for (auto x : m_lObjects)
		code = code % ObjectWN(x) % "\n";
	return code;
}
