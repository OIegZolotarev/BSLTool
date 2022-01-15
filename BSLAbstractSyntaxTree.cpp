#include "BSLToken.h"
#include "BSLAbstractSyntaxTree.h"
#include <stack>

namespace BSL
{

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
	}

	return -1;
}


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
				//tokenStreamElement_t* nextToken = source->PeekNextToken();

				std::stack<tokenStreamElement_t*> op_stack;
				std::vector<tokenStreamElement_t*> output;

				while (true)
				{
					token = source->ReadToken();

					if (!token)
						break;

					switch (token->type)
					{
					case TokenTypes::NumericConst:
						output.push_back(token);						
						break;
					case TokenTypes::OpeningBracket:
						op_stack.push(token);
						break;
					case TokenTypes::PlusSign:						
					case TokenTypes::MinusSign:
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

				
			}
			break;
		}

	}

	return pResult;
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