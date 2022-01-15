#pragma once
#include <algorithm>
#include <list>
#include "BSLToken.h"

namespace BSL
{

enum class ASTNodeTypes
{
	Module,
	Function,
	Procedure,
	ConditionalOperator,
	ArithmeticExpression,
	AssigmentExpression,
	MemberExpression,
	SubscriptExpression,
	Comment,
	ForLoop,
	WhileLoop,
	SubprogramCall,
	NumericConstant
};

class IAbstractSyntaxTreeNode
{
protected:
	
	std::list<IAbstractSyntaxTreeNode*>	m_Nodes;

	size_t m_sourceCodeStartingOffset;
	size_t m_sourceCodeLength;

	textHumanPosition_t m_startingPosition;
	textHumanPosition_t m_endingPosition;

public:
	IAbstractSyntaxTreeNode(ASTNodeTypes type);
	virtual ~IAbstractSyntaxTreeNode();

	void AddNode(IAbstractSyntaxTreeNode* pNode)
	{
		m_Nodes.push_back(pNode);
	}

protected:
	ASTNodeTypes m_nodeType;
};

typedef struct  
{
	std::wstring name;
	bool byValue;
	bool hasDefaultValue;
	std::wstring defaultValue;
}argumentDescriptor_t;

class SubprogramTreeNode: public IAbstractSyntaxTreeNode
{
	std::vector<std::wstring> m_Annotations;
	std::wstring	m_Name;
	std::vector<argumentDescriptor_t> m_Arguments;
	bool m_Export;
public:
	SubprogramTreeNode(TokenStream* stream, ASTNodeTypes type,std::vector<std::wstring> annotations);
	~SubprogramTreeNode();
};

class NumericConstantTreeNode : public IAbstractSyntaxTreeNode
{
	double m_Value;
public:
	NumericConstantTreeNode(double value) : IAbstractSyntaxTreeNode(ASTNodeTypes::NumericConstant)
	{
		m_Value = value;
	}
};

IAbstractSyntaxTreeNode* BuildAbstractSyntaxTree(TokenStream* source);

}
