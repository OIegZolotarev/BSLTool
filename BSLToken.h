#pragma once
#include <string>
#include <vector>
#include <exception>

namespace BSL
{

enum class TokenTypes
{	
	Identifier,
	BeginProcedure,
	BeginFunction,
	EndProcedure,
	EndFunction,
	EqualsSign,
	OpeningBracket,
	ClosingBracket,
	ExportKeyword,
	Comma,
	EndExpression,
	PlusSign,
	MinusSign,
	MultiplySign,
	DivisionSign,
	DotSign,
	BooleanConst,
	OperatorNew,
	OperatorIf,
	OperatorThen,
	OperatorElse,
	OperatorElseIf,
	OperatorEndIf,
	LessSign,
	GreaterSign,
	OperatorFor,
	OperatorWhile,
	OperatorEndLoop,
	OperatorTry,
	OperatorEndTry,
	DirectiveIf,
	DirectiveThen,
	DirectiveElseIf,
	DirectiveElse,
	DirectiveEndIf,
	DirectiveInsert,
	DirectiveEndInsert,
	DirectiveDelete,
	DirectiveEndDelete,
	DirectiveRegion,
	DirectiveEndRegion,
	KeywordAnd,
	KeywordOr,
	KeywordNot,
	KeywordVar,
	KeywordLoop,
	KeywordEach,
	KeywordVal,
	OpeningSquareBracket,
	ClosingSquareBracket,
	StringConst,
	Comment,
	NumericConst,
	Annotation,
};

typedef struct
{
	TokenTypes tokenType;
	const wchar_t* russian;
	const wchar_t* english;

}tokenDictionary_t;

//tokenDictionary_t* LookupTokenDictionary(const std::wstring& value);
TokenTypes TokenTypeFromValue(std::wstring tokenValue);

typedef struct  
{
	size_t row, column;
}textHumanPosition_t;

class UnexcpectedEndOfTokenStream : public std::exception
{

};

class UnexcpectedToken : public std::exception
{
	TokenTypes m_ExpectedType;
	TokenTypes m_RecivedType;
public:
	UnexcpectedToken(TokenTypes expectedToken, TokenTypes recivedToken);
};

typedef struct
{
	TokenTypes type;
	std::wstring value;

	size_t sourceOffset;
	size_t sourceLength;

	textHumanPosition_t textPosition;

	bool isStringLiteral;
}tokenStreamElement_t;

class TokenStream
{
	std::vector<tokenStreamElement_t> m_Data;
	size_t m_Position;

	void DoLexModule(std::wstring & sourceCode);
public:
	TokenStream(std::wstring & sourceCode);
	~TokenStream();

	void Reset();
	tokenStreamElement_t* PeekNextToken();
	tokenStreamElement_t* ReadToken(bool expectingToHaveAny = false);
	void CheckToken(TokenTypes type);	
	tokenStreamElement_t* CurrentToken();
	
	bool HasToken(TokenTypes type);

	TokenStream* ExtractSubstream(TokenTypes blockStartToken, TokenTypes blockEndToken);	
	TokenStream* ExtractExpressionSubstream();
private:
	void PushToken(std::wstring& tokenValue, size_t tokenStartRow, size_t tokenStartColumn, size_t offset, bool isStringLiteral);
	
	bool IsWhitespaceSymbol(wchar_t curSymbol);
	bool IsTokenDivider(wchar_t curSymbol);

	TokenStream()
	{
		m_Position = 0;
	}
};



};