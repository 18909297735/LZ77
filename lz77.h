#pragma once

#include"HashTable.h"

class lz77
{
public:
	lz77();
	~lz77();
	void CompressFile(const string& filePath);
	void UNCompressFile(const string& filePath);
	void GetLine(FILE* fIn, string& strContent);
private:
	void FillWindow(FILE* fIn);
	UCH LongMatch(USH matchHead, USH& curMatchDist);
	void WriteFlag(FILE* fOutF, UCH& chFlag, UCH& bitCount, bool IsChar);
private:
	USH _start;
	UCH * _pWin;
	HashTable _ht;
	size_t _lookAhead;
};