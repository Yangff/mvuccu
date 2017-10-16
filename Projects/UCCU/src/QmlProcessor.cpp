#include "QmlProcesser.h"
#include "LogManager.h"

#include <QtQml/5.4.2/QtQml/private/qqmljslexer_p.h> 
#include <QtQml/5.4.2/QtQml/private/qqmljsparser_p.h>
#include <QtQml/5.4.2/QtQml/private/qqmljsengine_p.h>
#include <QtQml/5.4.2/QtQml/private/qqmljsastvisitor_p.h>
#include <QtCore/qpointer.h>

#include <QtQml/qjsvalue.h>

#include <QtCore/qstack>

using namespace Qml;
using namespace QQmlJS;


QString _loc2str(const QString *a, AST::SourceLocation s) {
	return QStringRef(a, s.offset, s.length).toString();
}

#define loc2str(x) (_loc2str(m_pProcessor->GetCodeRef(), x))

template<class T>
QString node2str(const QString *a, T *x) {
	if (x) {
		return QStringRef(a, x->firstSourceLocation().offset, x->firstSourceLocation().length).toString();
	}
	else
		return "";
}

template<class T> QString completeId(T *id, QString d = ".") {
	QString result;
	while (id) {
		result += id->name.toString();
		id = id->next;
		if (id) result += d;
	}
	return result;
}

template<class T> QString _getAll(QString *a, T *x) {
	return QStringRef(a, x->firstSourceLocation().offset, x->lastSourceLocation().offset + x->lastSourceLocation().length - x->firstSourceLocation().offset).toString();
}

#define getAll(x) (_getAll(m_pProcessor->GetCodeRef(), x))

class InnerVisitor : public AST::Visitor {
public:
	InnerVisitor(Document *document, QmlProcessor *processor):m_pDocument(document), m_pProcessor(processor) {
		QSharedPointer<QmlNode> p(new QmlNode("__[PlaceHolder]__"));
		p->SetSelf(p);
		current.node = placeholder = p;
	};

	virtual bool visit(AST::UiPragma * pragma);
	virtual bool visit(AST::UiImport * import);
	virtual bool visit(AST::UiPublicMember * member);
	virtual bool visit(AST::UiSourceElement * sele);
	virtual bool visit(AST::UiObjectDefinition * o);
	virtual void endVisit(AST::UiObjectDefinition * o);
	virtual void endVisit(AST::UiObjectBinding * o);
	virtual bool visit(AST::UiObjectBinding * o);
	virtual bool visit(AST::UiScriptBinding * o);
	virtual bool visit(AST::UiArrayBinding * o);
	virtual void endVisit(AST::UiArrayBinding * o);

public:
	QSharedPointer<QmlNode> GetResult() {
		if (!placeholder.isNull()) {
			QSharedPointer<QmlNode> x = placeholder->GetObjects()[0];
			placeholder.clear();
			return x;
		}
		return QSharedPointer<QmlNode>(0);
	}
private:
	Document *m_pDocument; // do NOT delete this.
	QmlProcessor *m_pProcessor;
	QSharedPointer<QmlNode> placeholder;

	struct WorkingSet {
		QSharedPointer<QmlNode> node;

		QmlNode::Property p;
		QString name;
		QmlNode::Value val;

		void next() {
			if (node)
				if (!name.isEmpty() && !name.isNull())
					node->AddNameValueProperty(name, val, p);
				else if (p.eSymbolType == QmlNode::NoNameObject)
					node->AddUnnamedObject(val.o);
			val.clear(); p.clear(); name.clear();
		}

		void clear() {
			node.clear(); val.clear(); p.clear(); name.clear();
		}

		WorkingSet() :node(0) { p.clear(); val.clear(); }
	};

	WorkingSet current;
	QStack<WorkingSet> stack;

	void push(QSharedPointer<QmlNode> node) {
		stack.push(current);
		current.clear();
		current.node = node;
	}

	void pop() {
		current = (stack.pop());
	}
};

Qml::QmlProcessor::QmlProcessor(QString code):m_sCode(code) { }

Document* _GenerateDocument(QmlProcessor *_this, QString code) {
	Document *d = new Document;
	InnerVisitor * visitor = new InnerVisitor(d, _this);
	auto engine = new Engine();
	auto lexer = new Lexer(engine);
	engine->setLexer(lexer);
	lexer->setCode(code, 1);
	auto parser = new Parser(engine);
	bool x = parser->parse();

	for (auto m : parser->diagnosticMessages()) {
		LogManager::instance().log(m.message);
	}

	if (!x)
		return nullptr;

	auto ast = parser->ast();
	ast->accept0(reinterpret_cast<AST::Visitor*>(visitor));

	d->root = visitor->GetResult();

	delete parser;
	delete lexer;
	delete engine;
	delete visitor;

	return d;
}

QSharedPointer<QmlNode> Qml::QmlProcessor::GenerateNode() {
	Document* d = _GenerateDocument(this, m_sCode);
	QSharedPointer<QmlNode> n = d->root;
	delete d;
	return n;
}

Document * Qml::QmlProcessor::GenerateDocument() {
	Document* d = _GenerateDocument(this, m_sCode);
	return d;
}

QString Qml::QmlProcessor::GetCode() {
	return m_sCode;
}

QString* Qml::QmlProcessor::GetCodeRef() {
	return &m_sCode;
}

bool InnerVisitor::visit(AST::UiPragma * pragma) {
	m_pDocument->vParmas.append(getAll(pragma));
	return true;
}

bool InnerVisitor::visit(AST::UiImport * import) {
	m_pDocument->vImports.append(getAll(import));
	return true;
}

#define SetType(x) do { current.val.type = (QmlNode::ValueType::x); \
current.p.eValueType = QmlNode::ValueType::x; } while (0);

#define SetPType(x) do {current.p.eSymbolType = QmlNode::SymbolType::x; } while(0);

bool InnerVisitor::visit(AST::UiPublicMember * member) {
	current.name = member->name.toString();
	if (member->type) {
		// property
		SetPType(Prop);
		current.p.p.retType = loc2str(member->typeToken);
		current.p.p.bReadOnly = member->isReadonlyMember;
		current.p.p.bDefault = member->isDefaultMember;
	}
	else SetPType(Signal);

	if (member->type == 0 && member->parameters) {
		SetType(RawCode);
		current.val.s = getAll(member->parameters);
	} else if (member->statement) {
		SetType(RawCode);
		current.val.s = getAll(member->statement);
	} else if (member->binding) {
		SetType(BindingObject);
		return true; // wait to binding
	} else {
		SetType(NoValue);
	}

	current.next();

	return true;
}

bool InnerVisitor::visit(AST::UiSourceElement * sele) {
	if (AST::FunctionDeclaration *funDecl = AST::cast<AST::FunctionDeclaration *>(sele->sourceElement)) {
		SetType(RawCode);
		SetPType(Function);
		current.val.s = getAll(funDecl);
		current.name = funDecl->name.toString();
		current.next();
	} else if (AST::VariableStatement *varStmt = AST::cast<AST::VariableStatement *>(sele->sourceElement)) {
		current.node->AddVar(getAll(sele));
	}

	return false;
}

bool InnerVisitor::visit(AST::UiObjectDefinition * o) {
	QSharedPointer<QmlNode> next(0);
	if (current.p.eValueType != QmlNode::ValueType::Array) {
		SetType(BindingObject); SetPType(NoNameObject);
		current.val.o = next = QSharedPointer<QmlNode>(new QmlNode(completeId(o->qualifiedTypeNameId)));
	} else {
		// TODO: Check if array can include another array (for object)
		current.val.l.push_back(next = QSharedPointer<QmlNode>(new QmlNode(completeId(o->qualifiedTypeNameId))));
	}
	next->SetSelf(next);
	push(next);
	return true;
}

bool InnerVisitor::visit(AST::UiObjectBinding * o) {

	QSharedPointer<QmlNode> next(0);
	SetType(BindingObject);
	if (current.p.eSymbolType == QmlNode::SymbolType::NoSymbol)
		SetPType(Object);
	current.val.o = next = QSharedPointer<QmlNode>(new QmlNode(completeId(o->qualifiedTypeNameId)));
	next->SetSelf(next);
	current.name = completeId(o->qualifiedId);
	if (current.p.eSymbolType == QmlNode::SymbolType::Object) {
		current.p.o.bHasOnToken = o->hasOnToken;
	}
	push(next);
	return true;
}

bool InnerVisitor::visit(AST::UiScriptBinding * o) {
	SetType(RawCode);
	if (current.p.eSymbolType == QmlNode::SymbolType::NoSymbol)
		SetPType(Object);
	current.val.s = getAll(o->statement);
	current.name = completeId(o->qualifiedId);
	current.next();
	return true;
}

bool InnerVisitor::visit(AST::UiArrayBinding * o) {
	if (current.p.eSymbolType == QmlNode::SymbolType::NoSymbol)
		SetPType(Object);
	current.name = completeId(o->qualifiedId);
	if (o->members->member->uiObjectMemberCast()) {
		SetType(Array);
		return true;
	} else {
		SetType(RawCode);
		current.val.s = getAll(o->members);
		return false;
	}
}

void InnerVisitor::endVisit(AST::UiArrayBinding * o) {
	current.next();
}

void InnerVisitor::endVisit(AST::UiObjectDefinition * o) {
	pop();
	current.next();
}

void InnerVisitor::endVisit(AST::UiObjectBinding * o) {
	pop();
	current.next();
}