#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS

#include"lz77.h"
#include<assert.h>
#include<stdlib.h>
using namespace std;


const USH MIN_LOOKAHEAD = MAX_MATCH + 1;
const USH MAX_DIST = WSIZE - MIN_LOOKAHEAD;

lz77::lz77()
	:_pWin(new UCH[WSIZE * 2])
	,_ht(WSIZE)
	,_start(0)
	,_lookAhead(0)
{}
lz77::~lz77()
{
	if (_pWin)
	{
		delete[] _pWin;
	}
	
}

void lz77::CompressFile(const string& filePath)
{
	FILE* fIn = fopen(filePath.c_str(), "rb");

	if (nullptr == fIn)
	{
		cout << "���ļ�ʧ��" << endl;
		return;
	}

	//�ļ���СС�������ַ�  ����ѹ��
	//��ȡ�ļ���С
	fseek(fIn,0,SEEK_END);
	ULL fileSize = ftell(fIn);
	fseek(fIn, 0, SEEK_SET);

	if (fileSize < 3)//�ļ���СС�������ֽ�
	{
		fclose(fIn);
		return;
	}
	//�ȶ�ȡһ����������64K��������
	//_lookAheead��ѹ�����ݵĸ���
	_lookAhead = fread(_pWin, 1, 2 * WSIZE, fIn);
	USH hashAddr = 0;
	for (size_t i = 0; i < MIN_MATCH - 1; i++)
	{
		_ht.HashFunc(hashAddr,_pWin[i]);
	}

	//дԴ�ļ��ĺ�׺
	//fOutD�����ļ�ѹ������
	FILE* fOutD = fopen("1.lzp", "wb");
	assert(fOutD);
	string postFix = filePath.substr(filePath.rfind('.'));
	postFix += '\n';

	fwrite(postFix.c_str(), 1, postFix.size(), fOutD);
	//������λ��
	FILE* fOutF = fopen("2.lzp", "wb");
	assert(fOutF);

	USH matchHead = 0;
	UCH chFlag = 0;
	UCH bitCount = 0;
	while (_lookAhead)
	{
		//��startΪ�׵������ַ����뵽hash����
		_ht.Insert(hashAddr, _pWin[_start + 2], _start, matchHead);

		USH curMatchDist = 0;
		UCH curMatchLen = 0;
		if (matchHead && _lookAhead > MIN_LOOKAHEAD)
		{
			curMatchLen = LongMatch(matchHead, curMatchDist);
		}

		if (curMatchLen < MIN_MATCH)
		{
			//û��ƥ��
			//û���ҵ�ƥ��,дԴ�ַ�
			fputc(_pWin[_start], fOutD);
			++_start;
			--_lookAhead;
			//д���---Դ�ַ���0--false���������1���
			//д���
			WriteFlag(fOutF, chFlag, bitCount, false);
		}
		else
		{
			//���ƥ��
			//д���볤�ȶ�
			fwrite(&curMatchDist, 2, 1, fOutD);
			fputc(curMatchLen, fOutD);

			//д���λ
			//���Ⱦ������1���----true

			WriteFlag(fOutF, chFlag, bitCount, true);
			_lookAhead -= curMatchLen;

			//���¹�ϣ��
			curMatchLen -= 1;
			while (curMatchLen)
			{
				++_start;
				_ht.Insert(hashAddr, _pWin[_start + 2], _start, matchHead);
				curMatchLen--;
			}
			_start++;
		}
		//��������������������򴰿����������
		if (_lookAhead <= MIN_LOOKAHEAD)
		{
			FillWindow(fIn);
		}
	}
	//���һ����ǲ����˸�����λ������
	if (bitCount > 0 && bitCount < 8)
	{
		chFlag <<= (8 - bitCount);
		fputc(chFlag, fOutF);
	}
	fclose(fIn);
	fclose(fOutF);
	//������ļ��е����ݰ��Ƶ�ѹ���ļ���
	FILE* fInF = fopen("2.lzp", "rb");
	assert(fInF);
	UCH* pReadBuff = new UCH[1024];
	size_t flagSize = 0;
	while (true)
	{
		size_t rdSize = fread(pReadBuff, 1, 1024, fInF);
			if (0 == rdSize)
				break;
		flagSize += rdSize;
		fwrite(pReadBuff, 1, rdSize, fOutD);
	}
	fclose(fInF);
	fwrite(&fileSize, sizeof(fileSize), 1, fOutD);
	fwrite(&flagSize, sizeof(flagSize), 1, fOutD);
	fclose(fOutD);
	
	remove("2.lzp");
}
void lz77::WriteFlag(FILE* fOutF, UCH& chFlag, UCH& bitCount, bool IsChar)
{
	chFlag <<= 1;
	//��⵱ǰ����Ƿ�Ϊ�����
	if( IsChar )
		chFlag |= 1;

	bitCount++;
	if (bitCount == 8)
	{
		fputc(chFlag, fOutF);
		chFlag = 0;
		bitCount = 0;
	}
}

//matchHead----->��ϣƥ��������ʼλ��
UCH lz77::LongMatch(USH matchHead, USH& curMatchDist)
{	
	UCH curMatchLen = 0;
	UCH maxLen = 0;
	USH pos = 0;
	UCH Matchchainlen = 255;
	//ֻ����_start���MAX_DIST��Χ�ڵĴ�
	USH limit = _start > MAX_DIST ? _start - MAX_DIST : 0;
	do
	{
		UCH* pStart = _pWin + _start;
		UCH* pEnd = pStart + MAX_MATCH;

		//�ڲ��һ��������ҵ�ƥ�䴮����ʼλ��
		UCH* pCurStart = _pWin + matchHead;
		curMatchLen = 0;
		//�ҵ�������ƥ�䳤��
		while (pStart < pEnd && *pStart == *pCurStart)
		{
			curMatchLen++;
			pStart++;
			pCurStart++;
		}
		if (curMatchLen > maxLen)
		{
			pos = matchHead;
			maxLen = curMatchLen;
		}		
	} while (((matchHead = _ht.GetNext(matchHead))> limit) 
			 && Matchchainlen--);
	curMatchDist = _start - pos;
	return maxLen;
}

void lz77::UNCompressFile(const string& filePath)
{
	string strPostFix = filePath.substr(filePath.rfind("."));
	if (strPostFix != ".lzp")
	{
		cout << "ѹ���ļ���֧��" << endl;
		return;
	}

	FILE* fInD = fopen(filePath.c_str(), "rb");
	if (nullptr == fInD)
	{
		cout << "��ѹ���ļ�ʧ��" << endl;
		return;
	}

	//��ȡ��ǵĴ�С
	size_t flagSize = 0;
	int offset = 0 - sizeof(flagSize);
	fseek(fInD, offset, SEEK_END);
	fread(&flagSize, sizeof(flagSize), 1, fInD);

	//��ȡԴ�ļ��Ĵ�С
	ULL fileSize = 0;
	offset = 0 - sizeof(flagSize) - sizeof(fileSize);
	fseek(fInD, offset, SEEK_END);
	fread(&fileSize, sizeof(fileSize), 1, fInD);

	//fInF���ã���ȡ�������
	FILE* fInF = fopen(filePath.c_str(), "rb");
	assert(fInF);
    offset = 0 - (sizeof(fileSize) + sizeof(flagSize) + flagSize);
	fseek(fInF, offset, SEEK_END);

	//�򿪽�ѹ�����ļ�
	fseek(fInD,0,SEEK_SET);
	string strUNComFileName("UNCompreseFile");
	strPostFix = "";
	GetLine(fInD, strPostFix);
	strUNComFileName += strPostFix;

	//fOut���ã�дѹ������
	FILE* fOut = fopen(strUNComFileName.c_str(), "wb");
	assert(fOut);

	//�����Ⱦ����
	FILE* fWin = fopen(strUNComFileName.c_str(), "rb");
	assert(fWin);

	UCH charFlag = 0;
	char bitCount = -1;

	while (fileSize)
	{
		//��ȡ���
		if (bitCount < 0)
		{
			charFlag = fgetc(fInF);
			bitCount = 7;
		}

		if (charFlag & (1 << bitCount))//1--�����
		{
			//���Ⱦ����
			UCH dist = fgetc(fInD);
			UCH length = fgetc(fInD);

			fflush(fOut);//����������ˢ������
			fseek(fWin, 0 - dist, SEEK_END);
			fileSize -= length;

			while (length)
			{
				UCH ch = fgetc(fWin);
				fputc(ch, fOut);
				length--;
			}

		}
		else//0--Դ�ַ�
		{
			UCH ch = fgetc(fInD); 
			fputc(ch, fOut);
			fileSize -= 1;
		}
		bitCount--;
	}
	fclose(fInD);
	fclose(fInF);
	fclose(fOut);
	fclose(fWin);
}

void lz77::GetLine(FILE* fIn, string& strContent)
{
	while (!feof(fIn))
	{
		char ch = fgetc(fIn);
		if ('\n' == ch)
		{
			return;
		}
		strContent += ch;
	}
}
void lz77::FillWindow(FILE* fIn)
{
	//���Ҵ������ݰ��Ƶ���ߵĴ���
	if (_start >= WSIZE + MAX_DIST)
	{
		memcpy(_pWin, _pWin + WSIZE, WSIZE);
		memset(_pWin + WSIZE, 0, WSIZE);
		_start -= WSIZE;

	    //���¹�ϣ��
		_ht.Updata();
	}

	//���Ҵ������������
	if (!feof(fIn))
	{
		_lookAhead += fread(_pWin + WSIZE, 1, WSIZE, fIn);
	}
}
