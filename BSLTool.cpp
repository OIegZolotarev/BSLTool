// BSLTool.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <iostream>
#include "BSLToken.h"
#include "BSLAbstractSyntaxTree.h"


wchar_t* ReadFile(const char* fileName)
{
    FILE* fp = fopen(fileName, "rb");

    fseek(fp, 0, SEEK_END);
    size_t dataLength = ftell(fp) + 2;
    fseek(fp, 0, SEEK_SET);


    char* data = new char[dataLength];
    memset(data, 0, dataLength);
    fread(data, dataLength, 1, fp);
    fclose(fp);

    int reformatedSize = MultiByteToWideChar(CP_UTF8, 0, (char*)data, -1, 0, 0);

    wchar_t* newData = new wchar_t[reformatedSize];

    MultiByteToWideChar(CP_UTF8, 0, (char*)data, -1, newData, reformatedSize);

    delete[] data;

    return newData;

}

int main()
{
    setlocale(LC_ALL, "");
    wchar_t* data = ReadFile("ModuleSimple.txt");
    auto data_str = std::wstring(data);

    BSL::TokenStream* stream = new BSL::TokenStream(data_str);

    BSL::IAbstractSyntaxTreeNode* pTree = BSL::BuildAbstractSyntaxTree(stream);

    delete[] data;
    delete stream;
}

