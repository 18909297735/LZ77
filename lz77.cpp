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
		cout << "打开文件失败" << endl;
		return;
	}

	//文件大小小于三个字符  不用压缩
	//获取文件大小
	fseek(fIn,0,SEEK_END);
	ULL fileSize = ftell(fIn);
	fseek(fIn, 0, SEEK_SET);

	if (fileSize < 3)//文件大小小于三个字节
	{
		fclose(fIn);
		return;
	}
	//先读取一个缓冲区（64K）的数据
	//_lookAheead待压缩数据的个数
	_lookAhead = fread(_pWin, 1, 2 * WSIZE, fIn);
	USH hashAddr = 0;
	for (size_t i = 0; i < MIN_MATCH - 1; i++)
	{
		_ht.HashFunc(hashAddr,_pWin[i]);
	}

	//写源文件的后缀
	//fOutD保存文件压缩数据
	FILE* fOutD = fopen("1.lzp", "wb");
	assert(fOutD);
	string postFix = filePath.substr(filePath.rfind('.'));
	postFix += '\n';

	fwrite(postFix.c_str(), 1, postFix.size(), fOutD);
	//保存标记位的
	FILE* fOutF = fopen("2.lzp", "wb");
	assert(fOutF);

	USH matchHead = 0;
	UCH chFlag = 0;
	UCH bitCount = 0;
	while (_lookAhead)
	{
		//将start为首的三个字符插入到hash表中
		_ht.Insert(hashAddr, _pWin[_start + 2], _start, matchHead);

		USH curMatchDist = 0;
		UCH curMatchLen = 0;
		if (matchHead && _lookAhead > MIN_LOOKAHEAD)
		{
			curMatchLen = LongMatch(matchHead, curMatchDist);
		}

		if (curMatchLen < MIN_MATCH)
		{
			//没有匹配
			//没有找到匹配,写源字符
			fputc(_pWin[_start], fOutD);
			++_start;
			--_lookAhead;
			//写标记---源字符用0--false，距离对用1标记
			//写标记
			WriteFlag(fOutF, chFlag, bitCount, false);
		}
		else
		{
			//找最长匹配
			//写距离长度对
			fwrite(&curMatchDist, 2, 1, fOutD);
			fputc(curMatchLen, fOutD);

			//写标记位
			//长度距离对用1标记----true

			WriteFlag(fOutF, chFlag, bitCount, true);
			_lookAhead -= curMatchLen;

			//更新哈希表
			curMatchLen -= 1;
			while (curMatchLen)
			{
				++_start;
				_ht.Insert(hashAddr, _pWin[_start + 2], _start, matchHead);
				curMatchLen--;
			}
			_start++;
		}
		//窗口中数据如果不够，向窗口中填充数据
		if (_lookAhead <= MIN_LOOKAHEAD)
		{
			FillWindow(fIn);
		}
	}
	//最后一个标记不满八个比特位，补满
	if (bitCount > 0 && bitCount < 8)
	{
		chFlag <<= (8 - bitCount);
		fputc(chFlag, fOutF);
	}
	fclose(fIn);
	fclose(fOutF);
	//将标记文件中的内容搬移到压缩文件中
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
	//检测当前标记是否为距离对
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

//matchHead----->哈希匹配链的起始位置
UCH lz77::LongMatch(USH matchHead, USH& curMatchDist)
{	
	UCH curMatchLen = 0;
	UCH maxLen = 0;
	USH pos = 0;
	UCH Matchchainlen = 255;
	//只搜索_start左边MAX_DIST范围内的串
	USH limit = _start > MAX_DIST ? _start - MAX_DIST : 0;
	do
	{
		UCH* pStart = _pWin + _start;
		UCH* pEnd = pStart + MAX_MATCH;

		//在查找缓冲区里找到匹配串的起始位置
		UCH* pCurStart = _pWin + matchHead;
		curMatchLen = 0;
		//找单条链的匹配长度
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
		cout << "压缩文件不支持" << endl;
		return;
	}

	FILE* fInD = fopen(filePath.c_str(), "rb");
	if (nullptr == fInD)
	{
		cout << "打开压缩文件失败" << endl;
		return;
	}

	//获取标记的大小
	size_t flagSize = 0;
	int offset = 0 - sizeof(flagSize);
	fseek(fInD, offset, SEEK_END);
	fread(&flagSize, sizeof(flagSize), 1, fInD);

	//获取源文件的大小
	ULL fileSize = 0;
	offset = 0 - sizeof(flagSize) - sizeof(fileSize);
	fseek(fInD, offset, SEEK_END);
	fread(&fileSize, sizeof(fileSize), 1, fInD);

	//fInF作用：读取标记数据
	FILE* fInF = fopen(filePath.c_str(), "rb");
	assert(fInF);
    offset = 0 - (sizeof(fileSize) + sizeof(flagSize) + flagSize);
	fseek(fInF, offset, SEEK_END);

	//打开解压缩的文件
	fseek(fInD,0,SEEK_SET);
	string strUNComFileName("UNCompreseFile");
	strPostFix = "";
	GetLine(fInD, strPostFix);
	strUNComFileName += strPostFix;

	//fOut作用：写压缩数据
	FILE* fOut = fopen(strUNComFileName.c_str(), "wb");
	assert(fOut);

	//处理长度距离对
	FILE* fWin = fopen(strUNComFileName.c_str(), "rb");
	assert(fWin);

	UCH charFlag = 0;
	char bitCount = -1;

	while (fileSize)
	{
		//读取标记
		if (bitCount < 0)
		{
			charFlag = fgetc(fInF);
			bitCount = 7;
		}

		if (charFlag & (1 << bitCount))//1--距离对
		{
			//长度距离对
			UCH dist = fgetc(fInD);
			UCH length = fgetc(fInD);

			fflush(fOut);//缓存区数据刷到磁盘
			fseek(fWin, 0 - dist, SEEK_END);
			fileSize -= length;

			while (length)
			{
				UCH ch = fgetc(fWin);
				fputc(ch, fOut);
				length--;
			}

		}
		else//0--源字符
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
	//将右窗口数据搬移到左边的窗口
	if (_start >= WSIZE + MAX_DIST)
	{
		memcpy(_pWin, _pWin + WSIZE, WSIZE);
		memset(_pWin + WSIZE, 0, WSIZE);
		_start -= WSIZE;

	    //更新哈希表
		_ht.Updata();
	}

	//向右窗口中填充数据
	if (!feof(fIn))
	{
		_lookAhead += fread(_pWin + WSIZE, 1, WSIZE, fIn);
	}
}
