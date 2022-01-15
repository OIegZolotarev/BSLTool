#include <windows.h>
#include "BSLToken.h"
#include <algorithm>
#include "Utils.h"

constexpr auto CR = '\n';

namespace BSL
{

void TokenStream::DoLexModule(std::wstring& sourceCode)
{
	size_t dataLength = sourceCode.length();

	if (!dataLength)
		return;

	size_t offset = 0;
	bool inStringLiteral = false;
	bool inComment = false;

	size_t tokenStartRow = 0;
	size_t tokenStartColumn = 0;
	size_t tokenStartOffset = 0;
	
	size_t currentRow = 1;
	size_t currentColumn = 1;
		
	std::wstring tokenValue = L"";

	auto peekSymbol = [&](size_t peekOffset) -> wchar_t {

		size_t calculatedOffset = offset + peekOffset;

		if (calculatedOffset < dataLength)
			return sourceCode[calculatedOffset];
		else
			return NULL;
	};

	auto pushCurrentTokenAndStartNext = [&]()
	{
		PushToken(tokenValue, tokenStartRow, tokenStartColumn, tokenStartOffset, inStringLiteral);

		tokenStartOffset = offset;
		tokenValue = L"";
	};

	while (true)
	{
		if (offset == dataLength)
			break;

		wchar_t curSymbol = peekSymbol(0);
		wchar_t nextSymbol = peekSymbol(1);

		if (curSymbol == CR)
		{
			currentColumn = 1;
			currentRow++;
			inComment = false;
		}

		if (curSymbol == '/' && nextSymbol == '/' && !(inComment || inStringLiteral))
		{
			pushCurrentTokenAndStartNext();
			inComment = true;
		}

		if (inComment)
		{
			tokenValue += curSymbol;
		}
		else if (IsWhitespaceSymbol(curSymbol) && !inStringLiteral)
		{
			pushCurrentTokenAndStartNext();
		}
		else if (IsTokenDivider(curSymbol) && !inStringLiteral)
		{
			pushCurrentTokenAndStartNext();

			tokenValue += curSymbol;

			pushCurrentTokenAndStartNext();
		}
		else if (curSymbol == '\"')
		{
			if (inStringLiteral)
			{
				if (nextSymbol == '\"')
				{
					tokenValue += '"';
					offset++;
				}
				else
				{
					pushCurrentTokenAndStartNext();
					inStringLiteral = false;
				}
			}
			else
			{
				inStringLiteral = true;
				tokenStartOffset = offset;
				tokenValue = L"";

			}
		}
		else
		{
			if (tokenValue == L"")
			{
				tokenStartOffset = offset;
				tokenStartRow = currentRow;
				tokenStartColumn = currentColumn;
			}

			tokenValue += curSymbol;
		}

		currentColumn++;
		offset++;

	}


	if (tokenValue != L"")
		pushCurrentTokenAndStartNext();
}

TokenStream::TokenStream(std::wstring& sourceCode)
{
	m_Position = 0;
	m_Data.clear();

	DoLexModule(sourceCode);
}

TokenStream::~TokenStream()
{
	m_Data.clear();
	m_Data.shrink_to_fit();
}

void TokenStream::Reset()
{
	m_Position = 0;
}

BSL::tokenStreamElement_t* TokenStream::PeekNextToken()
{
	if (m_Position + 1 == m_Data.size())
	{
		throw new UnexcpectedEndOfTokenStream;
		return nullptr;
	}

	tokenStreamElement_t* el = &m_Data[m_Position + 1];
	return el;
}

BSL::tokenStreamElement_t* TokenStream::ReadToken(bool expectingToHaveAny)
{
	if (m_Position == m_Data.size())
	{
		if (expectingToHaveAny)
			throw new UnexcpectedEndOfTokenStream;

			return nullptr;
	}

	tokenStreamElement_t* el = &m_Data[m_Position];
	m_Position++;
	return el;
}

void TokenStream::CheckToken(TokenTypes type)
{
	tokenStreamElement_t* el = ReadToken(true);

	if (el->type != type)
		throw new UnexcpectedToken(type, el->type);

}

BSL::tokenStreamElement_t* TokenStream::CurrentToken()
{
	if (m_Position == m_Data.size())
	{
		throw new UnexcpectedEndOfTokenStream;
		return nullptr;
	}

	tokenStreamElement_t* el = &m_Data[m_Position];	
	return el;
}

bool TokenStream::HasToken(TokenTypes type)
{
	for (auto token : m_Data)
		if (token.type == type)
			return true;

	return false;
}

TokenStream* TokenStream::ExtractSubstream(TokenTypes blockStartToken, TokenTypes blockEndToken)
{
	TokenStream* pResult = new TokenStream;
	//pResult->m_Data.push_back(*currentToken);

	int level = 0;

	while (true)
	{
		tokenStreamElement_t* nextToken = ReadToken();

		if (!nextToken)
			throw new UnexcpectedEndOfTokenStream;

		if (nextToken->type == blockStartToken)
			level++;

		if (nextToken->type == blockEndToken)
		{
			if (level == 0)
			{
				//pResult->m_Data.push_back(*nextToken);
				break;
			}
			
			level--;
		}

		pResult->m_Data.push_back(*nextToken);
		
	}

	return pResult;
}

TokenStream* TokenStream::ExtractExpressionSubstream()
{
	if (m_Position == m_Data.size())
		return nullptr;

	TokenStream* pResult = new TokenStream;
	
	while (true)
	{
		tokenStreamElement_t* el = ReadToken();

		if (el == nullptr)
			break;

		if (el->type == TokenTypes::EndExpression)
			break;

		pResult->m_Data.push_back(*el);

	}

	return pResult;
}

void TokenStream::PushToken(std::wstring& tokenValue, size_t tokenStartRow, size_t tokenStartColumn, size_t offset, bool isStringLiteral)
{
	if (tokenValue == L"")
		return;

	if (tokenValue[0] == 0xFEFF)
		return;

//  	if (trim(tokenValue) == L"")
//  		return;

	tokenStreamElement_t elem;

	elem.value = tokenValue;	
	elem.textPosition.row = tokenStartRow;
	elem.textPosition.column = tokenStartColumn;
	elem.sourceOffset = offset;
	elem.sourceLength = tokenValue.length();
	elem.isStringLiteral = isStringLiteral;

	wchar_t* p;
	long converted = wcstol(tokenValue.c_str(), &p, 10);


	if (isStringLiteral)
		elem.type = TokenTypes::StringConst;
	else if (tokenValue[0] == L'&')
		elem.type = TokenTypes::Annotation;
	else if (*p == 0)
		elem.type = TokenTypes::NumericConst;
	else if (tokenValue.length() >= 2 && tokenValue[0] == '/' && tokenValue[1] == '/')
		elem.type = TokenTypes::Comment;
	else
		elem.type = TokenTypeFromValue(tokenValue);
	
	elem.isFunctionCallHint = false;
	m_Data.push_back(elem);

}

tokenDictionary_t g_TokenDictionary[] =
{
	{TokenTypes::BeginProcedure        ,L"œ–Œ÷≈ƒ”–¿"                     ,L"PROCEDURE"},
	{TokenTypes::BeginFunction         ,L"‘”Õ ÷»ﬂ"                       ,L"FUNCTION"},
	{TokenTypes::EndProcedure          ,L" ŒÕ≈÷œ–Œ÷≈ƒ”–€"                ,L"ENDPROCEDURE"},
	{TokenTypes::EndFunction           ,L" ŒÕ≈÷‘”Õ ÷»»"                  ,L"ENDFUNCTION"},
	{TokenTypes::EqualsSign            ,L"="                             ,L"="},
	{TokenTypes::OpeningBracket        ,L"("                             ,L"("},
	{TokenTypes::ClosingBracket        ,L")"                             ,L")"},
	{TokenTypes::ExportKeyword         ,L"› —œŒ–“"                       ,L"EXPORT"},
	{TokenTypes::Comma                 ,L","		                     ,L","},
	{TokenTypes::EndExpression         ,L";"                             ,L";"},
	{TokenTypes::PlusSign              ,L"+"                             ,L"+"},
	{TokenTypes::MinusSign             ,L"-"                             ,L"-"},
	{TokenTypes::MultiplySign          ,L"*"                             ,L"*"},
	{TokenTypes::DivisionSign          ,L"/"                             ,L"/"},
	{TokenTypes::DotSign               ,L"."                             ,L"."},
	{TokenTypes::BooleanConst          ,L"ÀŒ∆‹"                          ,L"FALSE"},
	{TokenTypes::BooleanConst          ,L"»—“»Õ¿"                        ,L"TRUE"},
	{TokenTypes::OperatorNew           ,L"ÕŒ¬€…"                         ,L"NEW"},
	{TokenTypes::OperatorIf            ,L"≈—À»"                          ,L"IF"},
	{TokenTypes::OperatorThen          ,L"“Œ√ƒ¿"                         ,L"THEN"},
	{TokenTypes::OperatorElse          ,L"»Õ¿◊≈"                         ,L"ELSE"},
	{TokenTypes::OperatorElseIf        ,L"»Õ¿◊≈≈—À»"                     ,L"ELSEIF"},
	{TokenTypes::OperatorEndIf         ,L" ŒÕ≈÷≈—À»"                     ,L"ENDIF"},
	{TokenTypes::LessSign              ,L"<"                             ,L"<"},
	{TokenTypes::GreaterSign           ,L">"                             ,L">"},
	{TokenTypes::OperatorFor           ,L"ƒÀﬂ"                           ,L"FOR"},
	{TokenTypes::OperatorWhile         ,L"œŒ ¿"                          ,L"WHILE"},
	{TokenTypes::OperatorEndLoop       ,L" ŒÕ≈÷÷» À¿"                    ,L"ENDLOOP"},
	{TokenTypes::OperatorTry           ,L"œŒœ€“ ¿"                       ,L"TRY"},
	{TokenTypes::OperatorEndTry        ,L" ŒÕ≈÷œŒœ€“ »"                  ,L"ENDTRY"},
	{TokenTypes::DirectiveIf           ,L"#≈ÒÎË"                         ,L"#IF"},
	{TokenTypes::DirectiveThen         ,L"#“Ó„‰‡"                        ,L"#THEN"},
	{TokenTypes::DirectiveElseIf       ,L"#»Ì‡˜Â≈ÒÎË"                    ,L"#ELSEIF"},
	{TokenTypes::DirectiveElse         ,L"#»Ì‡˜Â"                        ,L"#ELSE"},
	{TokenTypes::DirectiveEndIf        ,L"# ÓÌÂˆ≈ÒÎË"                    ,L"#ENDIF"},
	{TokenTypes::DirectiveInsert       ,L"#¬ÒÚ‡‚Í‡"                      ,L"#INSERT"},
	{TokenTypes::DirectiveEndInsert    ,L"# ÓÌÂˆ¬ÒÚ‡‚ÍË"                 ,L"#ENDINSERT"},
	{TokenTypes::DirectiveDelete       ,L"#”‰‡ÎÂÌËÂ"                     ,L"#DELETE"},
	{TokenTypes::DirectiveEndDelete    ,L"# ÓÌÂˆ”‰‡ÎÂÌËˇ"                ,L"#ENDDELETE"},
	{TokenTypes::DirectiveRegion       ,L"#Œ·Î‡ÒÚ¸"                      ,L"#REGION"},
	{TokenTypes::DirectiveEndRegion    ,L"# ÓÌÂˆŒ·Î‡ÒÚË"                 ,L"#ENDREGION"},
	{TokenTypes::KeywordAnd            ,L"»"                             ,L"AND"},
	{TokenTypes::KeywordOr             ,L"»À»"                           ,L"OR"},
	{TokenTypes::KeywordNot            ,L"Õ≈"                            ,L"NOT"},
	{TokenTypes::KeywordVar            ,L"œ≈–≈Ã"                         ,L"VAR"},
	{TokenTypes::KeywordLoop           ,L"÷» À"                          ,L"LOOP"},
	{TokenTypes::KeywordEach           ,L" ¿∆ƒŒ√Œ"                       ,L"EACH"},
	{TokenTypes::KeywordVal            ,L"«Õ¿◊"                          ,L"VAL"},
	{TokenTypes::OpeningSquareBracket  ,L"["                             ,L"["},
	{TokenTypes::ClosingSquareBracket  ,L"]"                             ,L"]"}
};

BSL::TokenTypes TokenTypeFromValue(std::wstring tokenValue)
{
	std::transform(tokenValue.begin(), tokenValue.end(),tokenValue.begin(), ::towupper);

	for (auto dict : g_TokenDictionary)
	{
		if (dict.russian == tokenValue)
			return dict.tokenType;
		else if (dict.english == tokenValue)
			return dict.tokenType;
	}

	return TokenTypes::Identifier;
}

bool TokenStream::IsWhitespaceSymbol(wchar_t curSymbol)
{
	return curSymbol < 33;
}

bool TokenStream::IsTokenDivider(wchar_t curSymbol)
{
	const wchar_t* dividers = L"\\/%()-=+;.,<>[]";
	return wcschr(dividers, curSymbol) != nullptr;
}

UnexcpectedToken::UnexcpectedToken(TokenTypes expectedToken, TokenTypes recivedToken)
{
	m_ExpectedType = expectedToken;
	m_RecivedType = recivedToken;
}

}



