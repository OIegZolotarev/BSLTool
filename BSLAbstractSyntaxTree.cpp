#include "BSLToken.h"
#include "BSLAbstractSyntaxTree.h"
#include <stack>



namespace BSL
{

void ReduceNodesByParsingSubscriptExpression(std::list<IAbstractSyntaxTreeNode*> m_tempNodes)
{
	if (m_tempNodes.size() < 3)
		return;

	//for (int i = 1; i < m_tempNodes.size() - 2; i++)
	size_t i = m_tempNodes.size() - 1;
	auto it = m_tempNodes.end();
	std::advance(it, -1);

	while (i > 0)
	{
		IAbstractSyntaxTreeNode* pRight = *it; std::advance(it, -1); i--;		
		

		if (pRight->Type() == ASTNodeTypes::Unparsed)
		{
			UnparsedNode* bracketNodeRight = dynamic_cast<UnparsedNode*>(pRight);
			if (NULL == bracketNodeRight)
				continue;

			auto rightNodeStart = it;
			std::advance(it, 1);

			if (bracketNodeRight->TokenType() == TokenTypes::ClosingSquareBracket)
			{
				while (i >= 0)
				{
					IAbstractSyntaxTreeNode* pLeft = *it; std::advance(it, -1); i--;
					if (pLeft->Type() == ASTNodeTypes::Unparsed)
					{
						UnparsedNode* bracketNodeLeft = dynamic_cast<UnparsedNode*>(pLeft);
						if (NULL == bracketNodeRight)
							continue;

						if (bracketNodeLeft->TokenType() == TokenTypes::OpeningSquareBracket)
						{
							auto rightNodeStart = it;
							std::advance(it, 1);
						}
					}
				}

			}
		}

	}
}

void ReduceNodesByParsingMemberExpressions(std::list<IAbstractSyntaxTreeNode*> m_tempNodes)
{
	
	if (m_tempNodes.size() < 3)
		return;

	//for (int i = 1; i < m_tempNodes.size() - 2; i++)
	size_t i = m_tempNodes.size() - 1;
	auto it = m_tempNodes.end();
	std::advance(it, -1);

	while(i > 0)
	{
		IAbstractSyntaxTreeNode* pRight = *it; std::advance(it, -1); i--;
		IAbstractSyntaxTreeNode* pMid  = *it; std::advance(it, -1); i--;
		IAbstractSyntaxTreeNode* pLeft = *it; std::advance(it, -1); i--;

		if (pMid->Type() == ASTNodeTypes::Unparsed)
		{
			UnparsedNode *dotNode = dynamic_cast<UnparsedNode *>(pMid);
			if (NULL != dotNode)
			{
				if (dotNode->TokenType() == TokenTypes::DotSign)
				{
					MemberExpressionNode* newNode = new MemberExpressionNode(pLeft, pRight);

					m_tempNodes.remove(pLeft);
					m_tempNodes.remove(pMid);
					m_tempNodes.remove(pRight);

					auto newPos = m_tempNodes.begin();
					std::advance(newPos, (i + 2));
					m_tempNodes.insert(newPos, newNode);

				}
			}
		}
	
	}

}

IAbstractSyntaxTreeNode::IAbstractSyntaxTreeNode(ASTNodeTypes type) : m_nodeType(type)
{
	m_Nodes.clear();
}

IAbstractSyntaxTreeNode::~IAbstractSyntaxTreeNode()
{
	for (auto item : m_Nodes)
		delete item;

	m_Nodes.clear();
}

int Precedence(TokenTypes token)
{
	switch (token)
	{
	case TokenTypes::PlusSign:
		return 1;
	case TokenTypes::MinusSign:
		return 1;
	case TokenTypes::MultiplySign:
		return 2;
	case TokenTypes::DivisionSign:
		return 2;
	case TokenTypes::DotSign:
		return 0;
	}

	return -1;
}

void ShuntAlgo(TokenStream* source);

IAbstractSyntaxTreeNode* BSL::BuildAbstractSyntaxTree(TokenStream* source)
{
	IAbstractSyntaxTreeNode* pResult = new IAbstractSyntaxTreeNode(ASTNodeTypes::Module);

	std::vector<std::wstring> annotations;

	while (true)
	{
		tokenStreamElement_t* token = source->ReadToken();

		if (!token)
			break;

		switch(token->type)
		{
		case TokenTypes::Annotation:
			annotations.push_back(token->value);
			continue;
			break;
		case TokenTypes::Comment:
			continue;
			break;
		case TokenTypes::BeginProcedure:
			{
				auto tokenStream = source->ExtractSubstream(TokenTypes::BeginProcedure, TokenTypes::EndProcedure);
				SubprogramTreeNode* pNode = new SubprogramTreeNode(tokenStream, ASTNodeTypes::Procedure, annotations);
				pResult->AddNode(pNode);
				delete tokenStream;
			}
			break;
		case TokenTypes::EndFunction:
			{
				auto tokenStream = source->ExtractSubstream(TokenTypes::BeginFunction, TokenTypes::EndFunction);
				SubprogramTreeNode* pNode = new SubprogramTreeNode(tokenStream, ASTNodeTypes::Function, annotations);
				pResult->AddNode(pNode);
				delete tokenStream;
			}
			break;
		case TokenTypes::Identifier:
			{

			
			source->Reset();
			ShuntAlgo(source);

// 				int a = 1;
// 
// 				std::list<IAbstractSyntaxTreeNode*> m_tempNodes;
// 
// 				while (true)
// 				{
// 					tokenStreamElement_t* el = source->ReadToken();
// 					if (!el)
// 						break;
// 
// 					m_tempNodes.push_back(new UnparsedNode(el));
// 				}
// 
// 				ReduceNodesByParsingMemberExpressions(m_tempNodes);
// 				ReduceNodesByParsingSubscriptExpression(m_tempNodes);
				
				
			}
			break;
		}

	}

	return pResult;
}

void ShuntAlgo(TokenStream * source)
{
	//tokenStreamElement_t* nextToken = source->PeekNextToken();

	std::stack<tokenStreamElement_t*> op_stack;
	std::vector<tokenStreamElement_t*> output;

	while (true)
	{
		tokenStreamElement_t* token = source->ReadToken();

		if (!token)
			break;

		switch (token->type)
		{
		case TokenTypes::NumericConst:
			output.push_back(token);
			break;
		case TokenTypes::Identifier:
			output.push_back(token);
			break;
		case TokenTypes::OpeningBracket:

			if (output.back()->type == TokenTypes::Identifier)
				token->isFunctionCallHint = true;

			op_stack.push(token);
			break;
		case TokenTypes::OpeningSquareBracket:
			op_stack.push(token);
			break;
		case TokenTypes::PlusSign:
		case TokenTypes::MinusSign:
		case TokenTypes::DotSign:
		case TokenTypes::EqualsSign:		
		case TokenTypes::MultiplySign:
		case TokenTypes::DivisionSign:

			if (!op_stack.empty())
			{
				while (Precedence(op_stack.top()->type) >= Precedence(token->type))
				{
					if (Precedence(op_stack.top()->type) == -1)
						break;

					output.push_back(op_stack.top());
					op_stack.pop();

					if (op_stack.empty())
						break;
				}
			}

			op_stack.push(token);
			break;
		case TokenTypes::ClosingBracket:

			while (true)
			{
				if (op_stack.empty())
					throw new std::exception("Mismatched parenthesis");


				if (op_stack.top()->type == TokenTypes::OpeningBracket)
				{
					tokenStreamElement_t* top = op_stack.top();

					if (top->isFunctionCallHint)
						output.push_back(top);

					op_stack.pop();
					break;
				}

				output.push_back(op_stack.top());
				op_stack.pop();
			}
			break;
		case TokenTypes::ClosingSquareBracket:

			while (true)
			{
				if (op_stack.empty())
					throw new std::exception("Mismatched parenthesis");


				if (op_stack.top()->type == TokenTypes::OpeningSquareBracket)
				{
					output.push_back(op_stack.top());
					op_stack.pop();
					break;
				}

				output.push_back(op_stack.top());
				op_stack.pop();
			}
			break;
		}

	}

	while (!op_stack.empty())
	{
		if (op_stack.top()->type == TokenTypes::OpeningBracket)
			throw new std::exception("Mismatched parenthesis");

		output.push_back(op_stack.top());
		op_stack.pop();
	}

	for (auto item : output)
	{
		wprintf(L"%s ", item->value.c_str());
	}

}

SubprogramTreeNode::SubprogramTreeNode(TokenStream* stream, ASTNodeTypes type, std::vector<std::wstring> annotations): IAbstractSyntaxTreeNode(type)
{
	m_Annotations.clear();

	for (auto annotation : annotations)
		m_Annotations.push_back(annotation);

	tokenStreamElement_t* programName = stream->ReadToken(true);
	stream->CheckToken(TokenTypes::OpeningBracket);

	m_Name = programName->value;

	while (true)
	{
		tokenStreamElement_t* token = stream->ReadToken(true);

		if (token->type == TokenTypes::ClosingBracket)
			break;

		
		argumentDescriptor_t desc;
		desc.byValue = false;
		desc.hasDefaultValue = false;
		desc.defaultValue = L"";

		if (token->type == TokenTypes::KeywordVal)
		{
			desc.byValue = true;
			
			token = stream->ReadToken(true);
			desc.name = token->value;
		}		
		else
			desc.name = token->value;

		token = stream->ReadToken(true);

		switch (token->type)
		{
			case TokenTypes::ClosingBracket:
				m_Arguments.push_back(desc);
				break;
			case TokenTypes::EqualsSign:
				token = stream->ReadToken(true);
				desc.defaultValue = token->value;
				desc.hasDefaultValue = true;
				m_Arguments.push_back(desc);
				break;
			case TokenTypes::Comma:
				m_Arguments.push_back(desc);
				break;
		}

	}

	if (stream->CurrentToken()->type == TokenTypes::ExportKeyword)
	{
		m_Export = true;		
	}

	while (true)
	{
		TokenStream* pExpressionStream = stream->ExtractExpressionSubstream();

		if (!pExpressionStream)
			break;

		IAbstractSyntaxTreeNode* pNode = BuildAbstractSyntaxTree(pExpressionStream);
		AddNode(pNode);
	}

}

SubprogramTreeNode::~SubprogramTreeNode()
{
	m_Annotations.clear();
	m_Annotations.shrink_to_fit();

	m_Arguments.clear();
	m_Arguments.shrink_to_fit();
}

}