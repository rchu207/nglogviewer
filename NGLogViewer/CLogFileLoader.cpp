#include "CLogFileLoader.h"
#include <time.h>

using namespace std;

#include <string.h>
#include <malloc.h>
#include <tchar.h>
#include <Windows.h>
#include "..\Utility\Debug.h"
#include "psapi.h"


void PrintMemoryInfo( DWORD processID )
{
	HANDLE hProcess;
	PROCESS_MEMORY_COUNTERS pmc;

	// Print the process identifier.

	dprintf( "\nProcess ID: %u\n", processID );

	// Print information about the memory usage of the process.

	hProcess = OpenProcess(  PROCESS_QUERY_INFORMATION,  FALSE, processID );
	if (NULL == hProcess)
		return;

	if ( GetProcessMemoryInfo( hProcess, &pmc, sizeof(pmc)) )
	{
		dprintf( "\tPageFaultCount: 0x%08X\n", pmc.PageFaultCount );
		dprintf( "\tPeakWorkingSetSize: 0x%08X\n",
			pmc.PeakWorkingSetSize );
		dprintf( "\tWorkingSetSize: %d K\nn", pmc.WorkingSetSize / 1024 );
		dprintf( "\tQuotaPeakPagedPoolUsage: 0x%08X\n",
			pmc.QuotaPeakPagedPoolUsage );
		dprintf( "\tQuotaPagedPoolUsage: 0x%08X\n",
			pmc.QuotaPagedPoolUsage );
		dprintf( "\tQuotaPeakNonPagedPoolUsage: 0x%08X\n",
			pmc.QuotaPeakNonPagedPoolUsage );
		dprintf( "\tQuotaNonPagedPoolUsage: 0x%08X\n",
			pmc.QuotaNonPagedPoolUsage );
		dprintf( "\tPagefileUsage: 0x%08X\n", pmc.PagefileUsage );
		dprintf( "\tPeakPagefileUsage: 0x%08X\n",
			pmc.PeakPagefileUsage );
	}

	CloseHandle( hProcess );
}


wchar_t *wcsistr
(
 const wchar_t *  szStringToBeSearched,
 const wchar_t *  szSubstringToSearchFor
 )
{
	wchar_t   *  pPos = NULL;
	wchar_t   *  szCopy1 = NULL;
	wchar_t   *  szCopy2 = NULL;


	// verify parameters
	if ( szStringToBeSearched == NULL ||
		szSubstringToSearchFor == NULL )
	{
		return NULL;
	}

	// empty substring - return input (consistent with strstr)
	if ( wcslen(szSubstringToSearchFor) == 0 ) {
		return NULL;
	}

	szCopy1 = _wcslwr(_wcsdup(szStringToBeSearched));
	szCopy2 = _wcslwr(_wcsdup(szSubstringToSearchFor));

	if ( szCopy1 == NULL || szCopy2 == NULL  ) {
		// another option is to raise an exception here
		free((void*)szCopy1);
		free((void*)szCopy2);
		return NULL;
	}

	pPos = wcsstr(szCopy1, szCopy2);

	if ( pPos != NULL ) {
		// map to the original string
		pPos = (wchar_t *)szStringToBeSearched + (pPos - szCopy1);
	}

	free((void*)szCopy1);
	free((void*)szCopy2);

	return pPos;
} // stristr(...)



int CLogFileLoader::PreProcessing()
{
	this->ClearAll();
	m_wiFile.seekg(0);
	if (!m_wiFile)
	{
		return 0;
	}

	wchar_t wszLineBuffer[LINE_BUFFER_SIZE];
	class CLineBuffer *pCLineBuffer = new CLineBuffer();
	int nIndex = 0;
	
	while(GetlineN(m_wiFile, wszLineBuffer, LINE_BUFFER_SIZE))
	{
		if (IsHeaderLineOfLogfile(wszLineBuffer))
			continue;
		GetLineBufferData(wszLineBuffer, pCLineBuffer);
		//1. Preprocess the Max/min line number
		{
			if (nIndex ==0)
			{
				m_nMaxLineNumber = pCLineBuffer->m_nLineNumber;
				m_nMinLineNumber = pCLineBuffer->m_nLineNumber;
				m_fMinTime = pCLineBuffer->m_fTime;
				m_fMaxTime = pCLineBuffer->m_fTime;
			}
			else
			{
				if (pCLineBuffer->m_nLineNumber> m_nMaxLineNumber)
					m_nMaxLineNumber = pCLineBuffer->m_nLineNumber;
				if (pCLineBuffer->m_fTime> m_fMaxTime)
					m_fMaxTime = pCLineBuffer->m_fTime;
			}
		}

		//2. Preprocess tags and process number
		m_setWstrTags.insert(pCLineBuffer->m_wstrTag);
		m_setIntProcessNumber.insert(pCLineBuffer->m_nProcess);
		nIndex++;
	}
	m_nTotalLines = nIndex;

	delete pCLineBuffer;
	pCLineBuffer = NULL;
	m_wiFile.clear();
	return 0;
}

CLogFileLoader::CLogFileLoader(wstring wstrFileName)
{
	ClearAll();
	m_wstrFilename = wstrFileName;
	m_wiFile.open(m_wstrFilename.c_str());
	if (m_wiFile.bad())
	{
		printf("fail to load!");
	}
	m_CallbackObject = NULL;
}

wstring CLogFileLoader::GetTag(wchar_t *wszBuffer)
{
	int nNumber;
	float fNumber;
	wchar_t wszSkip[LINE_BUFFER_SIZE]={0};
	wchar_t wszString[LINE_BUFFER_SIZE]={0};
	swscanf(wszBuffer, L"%d %f %s %s", &nNumber, &fNumber, wszSkip, wszString);
	if (wszString[0] !='[')
		return L"";
	wstring wstr(wszString);
	return wstr;
}

CLogFileLoader::~CLogFileLoader()
{
	ClearAll();
}

void CLogFileLoader::PrintInfo()
{
	//Prinet member var
	wprintf(L"Path:%s\n", m_wstrFilename.c_str());
	wprintf(L"Min Line number:%d\n", m_nMinLineNumber);
	wprintf(L"Max Line number:%d\n", m_nMaxLineNumber);
	wprintf(L"Min Time:%f\n", GetMinTime());
	wprintf(L"Max Time:%f\n", GetMaxTime());
	wprintf(L"====================================\n");

	//List All Tags
	vector<wstring>::iterator vecWstrIter;
	int i =0;
	vector<wstring> vecWstrTags=GetVecWstrTags();
	for ( vecWstrIter = vecWstrTags.begin(); vecWstrIter != vecWstrTags.end(); ++vecWstrIter )
	{
		i++;
		wchar_t *wstrA;
		wstrA =(wchar_t *) (vecWstrIter->c_str());
		wprintf(L"%d:%s\n", i, wstrA);
	}

	//List All Process
	vector<int> vecProcessNumber= this->GetVecIntProcessNumber();
	vector<int>::iterator veciIter;
	i=0;
	for (veciIter = vecProcessNumber.begin(); veciIter != vecProcessNumber.end(); ++veciIter)
	{
		i++;
		wprintf(L"%d:[%d]\n", i, *veciIter);
	}

}

int CLogFileLoader::GetLineBufferData(wchar_t *wszBuffer, class CLineBuffer *pCLineBuffer, bool bTag)
{
	wchar_t wszString[LINE_BUFFER_SIZE]={0};

	wchar_t wszDelims[]		= L"\t";
	wchar_t wszDelims2[]	= L"\0"; 
	wchar_t *resultLineNumber = NULL;
	wchar_t *resultTime = NULL;
	
	resultLineNumber = wcstok(wszBuffer, wszDelims );
	resultTime = wcstok(NULL, wszDelims);
	pCLineBuffer->m_wstrTimeString = resultTime;	
	wszBuffer = wcstok(NULL, wszDelims2);
	if (NULL!=wcschr(resultTime, ':'))
	{
		struct tm temp;
		int byte = 0;
		memset(&temp, byte,sizeof(struct tm));
		temp.tm_mday =1;
		temp.tm_year = 90;
		swscanf(resultTime, L"%d:%d:%d", &(temp.tm_hour), &(temp.tm_min), &(temp.tm_sec));
		pCLineBuffer->m_tTime = mktime(&temp);
		pCLineBuffer->m_fTime = 0 ;
	}
	else
	{
		swscanf(resultTime, L"%f", &pCLineBuffer->m_fTime);
		pCLineBuffer->m_tTime =0;
	}
	 
	//swscanf(wszBuffer, L"%d %f [%d]", &pCLineBuffer->m_nLineNumber, &pCLineBuffer->m_fTime, &pCLineBuffer->m_nProcess);
	swscanf(resultLineNumber, L"%d", &pCLineBuffer->m_nLineNumber);
	swscanf(wszBuffer, L"[%d]",&pCLineBuffer->m_nProcess);

	if (bTag)
	{
		wchar_t *pWstr;
		pWstr = wcschr(wszBuffer, '[');
		if (pCLineBuffer->m_nProcess != 0)
		{
			pWstr = wcschr(wszBuffer, ']');
			if (pWstr ==NULL)
			{
				wszString[0] = '\0';
			}
			else
			{
				pWstr++;	//empty char
				pWstr++;	//'['
				pCLineBuffer->m_wstrMessage = pWstr;
				if (pWstr!=NULL)
				{
					//wcscpy(wszString,pWstr);
					CleanAsTag(pWstr);
				}
			}
		}
		pCLineBuffer->m_wstrTag = pWstr;
	}
	else
	{
		wchar_t *pWstr;
		pWstr = wcschr(wszBuffer, '[');
		if (pWstr!=NULL)
			pCLineBuffer->m_wstrMessage = pWstr;
		else 
			pCLineBuffer->m_wstrMessage = L"";
		pCLineBuffer->m_wstrTag =L"";
	}
	
	return 0;
}

void CLogFileLoader::CleanAsTag(wchar_t *wszString)
{
	//Clean tag String:
	if (wszString[0] !='[')
		wszString[0] = '\0';
	else
	{
		wchar_t *pWstr=wszString;
		do 
		{
			pWstr = wcschr(pWstr, ']');
			if (pWstr==NULL)
			{
				break;
			}
			else
			{
				pWstr++;
				if (*pWstr == '[')
				{
					continue;
				}
				else
				{
					*pWstr='\0';
					break;
				}
			}
		} while (true);

	}
}

vector<int> CLogFileLoader::GetVecIntProcessNumber()
{
	vector<int> vecIntProcessNumber;
	set<int>::iterator sIter;
	int i =0;
	for ( sIter = m_setIntProcessNumber.begin(); sIter != m_setIntProcessNumber.end(); ++sIter )
	{
		vecIntProcessNumber.push_back(*sIter);
	}
	return vecIntProcessNumber;
}

vector<wstring> CLogFileLoader::GetVecWstrTags()
{
	vector<wstring> vecWstrTags;
	hash_set<wstring>::iterator hsIter;
	int i =0;
	for ( hsIter = m_setWstrTags.begin(); hsIter != m_setWstrTags.end(); ++hsIter )
	{
		vecWstrTags.push_back(*hsIter);
	}
	sort(vecWstrTags.begin(), vecWstrTags.end() );     
	return vecWstrTags;
}

int CLogFileLoader::ClearAll()
{
	m_setIntProcessNumber.clear();
	m_setWstrTags.clear();
	m_nMinLineNumber=0;
	m_nMaxLineNumber=0;
	m_fMinTime=0.0;
	m_fMaxTime=0.0;
	m_bEnableLineNumberFilter = false;
	m_bEnableTimeFilter = false;
	m_bEnableOutputNoProcessNumber = false;
	m_bEnableProcessFilter = false;
	m_bEnableOutputEmptyFilter = false;
	m_bEnableExcludeKeywordsFilter = false;
	m_pVecIntFilterProcessNumber = NULL;
	m_pVecWstrFilterTags = NULL;
	m_bReFilterDirtyBit = 0;
	m_bEnableRunBuffer = false;
	m_bPrintDebugMessage = true;
	m_vecWstrFilterKeyWords.clear();
	
	for (int i = 0;i<m_vecpRunBuffer.size();++i)
	{
		delete m_vecpRunBuffer[i];
		m_vecpRunBuffer[i]=NULL;
	}
	m_vecpRunBuffer.clear();
	
	return 0;
}


int CLogFileLoader::GetMinLineNumber()
{
	return m_nMinLineNumber;
}

int CLogFileLoader::GetMaxLineNumber()
{
	return m_nMaxLineNumber;
}

int CLogFileLoader::GetResultSize()
{
	return m_setResultLine.size();
}

int CLogFileLoader::RunFilterResult()
{
	if (!IsNeedDirtyReFilterData())
	{
		return 0;
	}
	m_setResultLine.clear();
	m_vecResultLinePos.clear();
	m_wiFile.seekg(0);
	
	for (int i = 0;i<m_vecpRunBuffer.size();++i)
	{
		delete m_vecpRunBuffer[i];
		m_vecpRunBuffer[i]=NULL;
	}
	m_vecpRunBuffer.clear();

	wchar_t wszLineBuffer[LINE_BUFFER_SIZE];
	int nIndex = 0;

	m_wiFile.seekg (0, ios::end);
	int nlength = m_wiFile.tellg();
	m_wiFile.seekg (0, ios::beg);

	int llpos = m_wiFile.tellg();

	while(GetlineN(m_wiFile, wszLineBuffer, LINE_BUFFER_SIZE))
	{
		if (IsHeaderLineOfLogfile(wszLineBuffer))
			continue;
		class CLineBuffer *pCLineBuffer = new CLineBuffer();
		GetLineBufferData(wszLineBuffer, pCLineBuffer, false);
		if (! IsFilterLine(pCLineBuffer))
		{
			m_setResultLine.insert(nIndex);
			m_vecResultLinePos.push_back(llpos);
			if (m_bEnableRunBuffer)
				m_vecpRunBuffer.push_back(pCLineBuffer);
			else
				delete pCLineBuffer;
		}
		else
		{
			delete pCLineBuffer;
		}

		nIndex++;
		llpos = m_wiFile.tellg();
		if (m_CallbackObject)
		{
			float fInput = ((float)llpos/nlength); 
			m_CallbackObject->OnPercentCallback(fInput);
		}

		if (m_bPrintDebugMessage && (nIndex%100 ==0))
		{
			//Print memory

			DWORD dwCurPID;
			dwCurPID = GetCurrentProcessId();
			dprintf("index = %d Current process id : %d\n", nIndex, dwCurPID);
			PrintMemoryInfo( dwCurPID );
		}
	}

	m_wiFile.clear();

	if (m_CallbackObject)
		m_CallbackObject->OnPercentCallback(1.0f);

	ReSetDirtyBit();
	return 0;
}

bool CLogFileLoader::IsFilterLine(class CLineBuffer *pCLineBuffer)
{
	if (m_bEnableIncludeKeeywordsFilter && IsFilterLineByIncludeKeyWords(pCLineBuffer) )
		return true;
	if (m_bEnableOutputEmptyFilter && IsEmptyString(pCLineBuffer->m_wstrMessage.c_str()))
		return true;
	if (m_bEnableLineNumberFilter && IsFilterLineByLineNumber(pCLineBuffer))
		return true;
	if (m_bEnableTimeFilter && IsFilterLineByTime(pCLineBuffer))
		return true;
	if (m_bEnableProcessFilter && IsFilterLineByProcess(pCLineBuffer))
		return true;
	if (m_bEnableTagsFilter && IsFilterLineByTags(pCLineBuffer))
		return true;
	if (m_bEnableExcludeKeywordsFilter && IsFilterLineByExcludeKeyWords(pCLineBuffer))
		return true;
	return false;
}

bool CLogFileLoader::IsFilterLineByTags(class CLineBuffer *pCLineBuffer)
{
	if (m_pVecWstrFilterTags==NULL)
		return false;
	vector<wstring>::iterator pos;
	for (pos = m_pVecWstrFilterTags->begin();pos != m_pVecWstrFilterTags->end();++pos)
	{
		if (CheckSubString(pCLineBuffer->m_wstrMessage.c_str(), pos->c_str()) != NULL)
		{
			return true;
		}
	}

	return false;
}

bool CLogFileLoader::IsFilterLineByExcludeKeyWords(class CLineBuffer *pCLineBuffer)
{
	vector<wstring>::iterator pos;
	for (pos = m_vecWstrFilterKeyWords.begin();pos != m_vecWstrFilterKeyWords.end();++pos)
	{
		if (CheckSubString(pCLineBuffer->m_wstrMessage.c_str(), pos->c_str()) != NULL)
		{
			return true;
		}
	}
	return false;
}

bool CLogFileLoader::IsFilterLineByIncludeKeyWords(class CLineBuffer *pCLineBuffer)
{
	vector<wstring>::iterator pos;
	for (pos = m_vecWstrIncludeKerWords.begin();pos != m_vecWstrIncludeKerWords.end();++pos)
	{
		if (CheckSubString(pCLineBuffer->m_wstrMessage.c_str(), pos->c_str()) != NULL)
		{
			return false;
		}
	}
	return true;
}

bool CLogFileLoader::IsFilterLineByLineNumber(class CLineBuffer *pCLineBuffer)
{
	if (pCLineBuffer->m_nLineNumber< this->m_nStartLineNumber)
		return true;
	if (pCLineBuffer->m_nLineNumber> this->m_nEndLineNumber)
		return true;
	return false;
}

bool CLogFileLoader::IsFilterLineByTime(class CLineBuffer *pCLineBuffer)
{
	if (pCLineBuffer->m_fTime< this->m_fStartTime)
		return true;
	if (pCLineBuffer->m_fTime> this->m_fEndTime)
		return true;
	return false;
}

bool CLogFileLoader::IsFilterLineByProcess(class CLineBuffer *pCLineBuffer)
{
	if(m_pVecIntFilterProcessNumber == NULL)
		return false;
	if((m_bEnableOutputNoProcessNumber) && (pCLineBuffer->m_nLineNumber == 0))
		return false;
	vector<int>::iterator pos;
	for (pos = m_pVecIntFilterProcessNumber->begin();pos!= m_pVecIntFilterProcessNumber->end() ;pos++)
	{
		if (*pos == pCLineBuffer->m_nProcess)
			return true;
	}
	return false;
}

int CLogFileLoader::GetResultLine(int nLine, CLineBuffer *pCLineBuffer)
{
	wchar_t wszLineBuffer[LINE_BUFFER_SIZE];
	GetResultLine(nLine,wszLineBuffer);
	GetLineBufferData(wszLineBuffer, pCLineBuffer, false);
	return 0;
}

int CLogFileLoader::GetResultLine(int nLine, wchar_t *wszLineBuffer)
{
	int llPos = m_vecResultLinePos[nLine];
	m_wiFile.seekg(llPos);
	GetlineN(m_wiFile, wszLineBuffer, LINE_BUFFER_SIZE);
	m_wiFile.clear();
	return 0;
}

bool CLogFileLoader::GetlineN(wifstream &fin, wchar_t *wszLineBuffer, size_t nSize)
{
    wstring wstrBuffer;
    bool bRet =(bool) (getline(m_wiFile, wstrBuffer)!=NULL);
    wcsncpy(wszLineBuffer,wstrBuffer.c_str(), nSize-1);
    wszLineBuffer[nSize-1] =0;  
    return bRet;
}

int CLogFileLoader::SetLineNumberFilter(int nStartLineNumber, int nEndLineNumber)
{
	m_bEnableLineNumberFilter = true;
	m_nStartLineNumber = nStartLineNumber;
	m_nEndLineNumber = nEndLineNumber;
	return 0;
}

void CLogFileLoader::PrintResult()
{
	int nTotlal = this->GetResultSize();
	wchar_t wstrBuffer[LINE_BUFFER_SIZE];
	for (int i=0;i<nTotlal;++i)
	{
		this->GetResultLine(i, wstrBuffer);
		wprintf(L"%s\n", wstrBuffer);
	}
}

float CLogFileLoader::GetMinTime()
{
	return m_fMinTime;
}

float CLogFileLoader::GetMaxTime()
{
	return m_fMaxTime;
}


int CLogFileLoader::SetTimeFilter(float fStartTime, float fEndTime)
{
	m_bEnableTimeFilter = true;
	m_fStartTime = fStartTime;
	m_fEndTime = fEndTime;
	return 0;
}

int CLogFileLoader::SetProcessFilter(vector<int> *pProcessFilter, bool bIncludeNoProcessFilter/* = false*/)
{
	m_bEnableProcessFilter = true;
	m_bEnableOutputNoProcessNumber = bIncludeNoProcessFilter;
	m_pVecIntFilterProcessNumber = pProcessFilter;
	return 0;
}

int CLogFileLoader::SetTagsFilter(vector<wstring> *pTagsFilter)
{
	m_bEnableTagsFilter = true;
	m_pVecWstrFilterTags = pTagsFilter;
	return 0;
}

bool CLogFileLoader::IsEmptyString(const wchar_t * pwszStr)
{
	const wchar_t * pwszStrAfterProcess;
	pwszStrAfterProcess = wcschr(pwszStr, ']');
	if (pwszStrAfterProcess!=NULL)
		pwszStrAfterProcess++;
	else
		pwszStrAfterProcess = pwszStr;
	for(unsigned int i =0;i<wcslen(pwszStrAfterProcess);++i)
	{
		if (isspace(pwszStrAfterProcess[i])==0)
		{
			return false;
		}
	}
	return true;
}

bool CLogFileLoader::SaveResultAs(wstring wstrPathName)
{
	wofstream fwoFile(wstrPathName.c_str());
	if (fwoFile.fail())
		return false;
	int nCount = this->GetResultSize();
	wchar_t wszBuffer [LINE_BUFFER_SIZE];
	for (int i = 0; i<nCount ;i++)
	{
		this->GetResultLine(i, wszBuffer);
		fwoFile<< wszBuffer << endl;
	}
	fwoFile.close();
	return true;
}

int CLogFileLoader::SetEnableFilterEmptyMessage(bool bInput)
{
	m_bEnableOutputEmptyFilter = bInput;
	return 0;
}

int CLogFileLoader::SetKeyWordExcludeFilter(const wchar_t *wstrKeyWords)
{
	if (CompareStringBySelf(wstrKeyWords, m_wstrExclude.c_str())==0)
	{
		return 0;
	}
	SetDirtyBit(CLOGFILELOADER_DIRTYBIT_EXCLUDE);
	m_wstrExclude=wstrKeyWords;
	ClearKeyWordExclude();
	if (wstrKeyWords == NULL)
	{
		return 0;
	}

	m_bEnableExcludeKeywordsFilter = true;
	wchar_t *pwszBuffer = new wchar_t [wcslen(wstrKeyWords)+1];
	
	wcscpy(pwszBuffer, wstrKeyWords);

	wchar_t wszDelims[] = L";";
	wchar_t *result = NULL;
	result = wcstok(pwszBuffer, wszDelims );
	while (result != NULL)
	{
		m_vecWstrFilterKeyWords.push_back(result);
		result = wcstok(NULL, wszDelims);
	}
	delete [] pwszBuffer;
	pwszBuffer = NULL;
	return 0;
}

int CLogFileLoader::SetKeyWordIncludeFilter(const wchar_t *wstrKeyWords)
{
	if (CompareStringBySelf(wstrKeyWords, m_wstrInclude.c_str())==0)
	{
		return 0;
	}
	SetDirtyBit(CLOGFILELOADER_DIRTYBIT_INCLUDE);
	m_wstrInclude=wstrKeyWords;
	ClearKeyWordInclude();
	if (wstrKeyWords == NULL || 
		(CompareStringBySelf(wstrKeyWords, L"*")==0) ||
		(CompareStringBySelf(wstrKeyWords, L"") ==0)
		)
	{
		return 0;
	}

	m_bEnableIncludeKeeywordsFilter = true;
	wchar_t *pwszBuffer = new wchar_t [wcslen(wstrKeyWords)+1];

	wcscpy(pwszBuffer, wstrKeyWords);

	wchar_t wszDelims[] = L";";
	wchar_t *result = NULL;
	result = wcstok(pwszBuffer, wszDelims );
	while (result != NULL)
	{
		m_vecWstrIncludeKerWords.push_back(result);
		result = wcstok(NULL, wszDelims);
	}
	delete [] pwszBuffer;
	pwszBuffer = NULL;
	return 0;
}

void CLogFileLoader::SetCallbackPercentFunction(CLogFileLoaderCallback *pObj)
{
	m_CallbackObject = pObj;
}

int CLogFileLoader::GetResultLine(int nLine, CLineBuffer **pCLineBuffer)
{
	if (nLine<m_vecResultLinePos.size())
	{
		*pCLineBuffer = (CLineBuffer *)m_vecpRunBuffer[nLine];
		return 0;
	}


	*pCLineBuffer=NULL;
	return -1;
}

void CLogFileLoader::SetEnableMatchCaseStringCompare(bool bInput)
{
	m_bEnableMatchCaseStringCompare = bInput;
}

int CLogFileLoader::CompareStringBySelf(const wchar_t *s1, const wchar_t *s2)
{
	if (m_bEnableMatchCaseStringCompare)
		return wcscmp(s1,s2);
	else
		return wcsicmp(s1,s2);
}

const wchar_t * CLogFileLoader::CheckSubString(const wchar_t *s1, const wchar_t *s2)
{
	if (m_bEnableMatchCaseStringCompare)
		return wcsstr(s1,s2);
	else
		return wcsistr(s1,s2); //TODO: NEED To Find a function for ignore case w strstr.
}

bool CLogFileLoader::IsHeaderLineOfLogfile(const wchar_t *wsz)
{
	if (wsz[0]=='[')
	{
		return true;
	}
	return false;
}

bool CLogFileLoader::ClearKeyWordInclude()
{
	m_bEnableIncludeKeeywordsFilter = false;
	m_vecWstrIncludeKerWords.clear();
	return true;
}

bool CLogFileLoader::ClearKeyWordExclude()
{
	m_bEnableExcludeKeywordsFilter = false;
	m_vecWstrFilterKeyWords.clear();
	return true;
}

bool CLogFileLoader::IsNeedDirtyReFilterData()
{
	return (m_bReFilterDirtyBit!=0);
}

void CLogFileLoader::SetDirtyBit(int bit)
{
	m_bReFilterDirtyBit|=bit;
}

void CLogFileLoader::ReSetDirtyBit(int bit/* =0xffffffff */)
{
	m_bReFilterDirtyBit&=(~bit);
}
