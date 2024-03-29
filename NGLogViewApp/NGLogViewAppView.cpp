
#include "stdafx.h"
#include "NGLogViewApp.h"

#include "NGLogViewAppDoc.h"
#include "NGLogViewAppView.h"
#include "MainFrm.h"
#include "NGPropertySheet.h"

#include "Clipboard.h"
#include <string>
using namespace std;

#define APP_SETTING_PATH (L"SOFTWARE\\NG\\LOGViewer")
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CNGLogViewAppView

IMPLEMENT_DYNCREATE(CNGLogViewAppView, CNGListViewEx)
static UINT WM_FINDREPLACE = ::RegisterWindowMessage( FINDMSGSTRING );

BEGIN_MESSAGE_MAP(CNGLogViewAppView, CNGListViewEx)
	ON_WM_DESTROY()
	ON_NOTIFY_REFLECT(LVN_DELETEITEM, OnDeleteItem)
	ON_NOTIFY_REFLECT(LVN_GETDISPINFO, OnGetDispInfo)
	ON_COMMAND(HOT_KEY_FIND_DLG, OnFindDialog)
	ON_COMMAND(HOT_KEY_FIND_STRING, OnFindString)
	ON_COMMAND(HOT_KEY_SET_BOOKMARK, OnSetBookmark)
	ON_COMMAND(HOT_KEY_FIND_NEXT_BOOKMARK, OnFindNextBookmark)
	ON_COMMAND(HOT_KEY_OPEN_PROPERTY_PAGE, OnProperties)
	ON_REGISTERED_MESSAGE(WM_FINDREPLACE,OnFindDialogMessage)
	ON_COMMAND(ID_FILE_OPEN, &CNGLogViewAppView::OnFileOpen)
	ON_COMMAND(ID_FILE_SAVE_AS, &CNGLogViewAppView::OnFileSaveAs)
	ON_COMMAND(ID_EDIT_FONT, &CNGLogViewAppView::OnEditFont)
	ON_COMMAND(ID_EDIT_PROPERTIES, &CNGLogViewAppView::OnProperties)
	ON_COMMAND(ID_EDIT_CLEARBOOKMARK, &CNGLogViewAppView::OnClearBookmark)
	ON_COMMAND(ID_EDIT_COPY, &CNGLogViewAppView::OnEditCopy)
END_MESSAGE_MAP()

// CNGLogViewAppView construction/destruction

CNGLogViewAppView::CNGLogViewAppView()
{
	// TODO: add construction code here
	m_pLogFileLoader = NULL;
	m_pRecSetting = new CRegSetting(APP_SETTING_PATH);
	m_strPath = _T("");
	m_pFindDialog = NULL;
	m_bRemoveShowSelAlwaysAtFindDialogExit = false;
	ZeroMemory(&m_props, sizeof(PROPINFO));
	
	//Get Default Value	
	CNGPropertySheet ps(_T("Properties"));
	m_props = ps.m_FilterPage.m_PropInfo;
	m_mapHighLightString = ps.m_FilterPage.GetMapStringToColors();

}

CNGLogViewAppView::~CNGLogViewAppView()
{
	if(m_pLogFileLoader)
	{
		delete m_pLogFileLoader;
		m_pLogFileLoader = NULL;
	}

	if (m_pRecSetting)
	{
		delete m_pRecSetting;
		m_pRecSetting = NULL;
	}
}

BOOL CNGLogViewAppView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs
	if (!CListView::PreCreateWindow (cs))
		return FALSE;

	cs.style &= ~LVS_TYPEMASK;
	cs.style |= LVS_REPORT | LVS_OWNERDRAWFIXED;	
	return TRUE;

}

void CNGLogViewAppView::OnInitialUpdate()
{
	CNGListViewEx::OnInitialUpdate();
	CNGLogViewAppDoc *pDoc = GetDocument();
	CString cstr = pDoc->GetPathName();

	// TODO: You may populate your ListView with items by directly accessing
	//  its list control through a call to GetListCtrl().
	if (cstr =="") // original
	{
		m_ilImage.Create(16, 16, TRUE | ILC_COLOR8, 2, 2);
		m_ilImage.Add(AfxGetApp()->LoadIcon(IDI_BOOKMARK));
		m_ilImage.Add(AfxGetApp()->LoadIcon(IDI_BOOKMARK1));		
		GetListCtrl().SetImageList(&m_ilImage, LVSIL_STATE);
		
		// Add column
		GetListCtrl().InsertColumn(0, _T("#"), LVCFMT_CENTER, 55);
		GetListCtrl().InsertColumn(1, _T("Line"), LVCFMT_CENTER, 55);
		GetListCtrl().InsertColumn(2, _T ("PID"), LVCFMT_CENTER, 45);
		GetListCtrl().InsertColumn(3, _T("Time"), LVCFMT_CENTER, 100);
		GetListCtrl().InsertColumn(4, _T("Message"), LVCFMT_LEFT, 2048);

		//Set Font 
		LOGFONT lf;
		if (m_pRecSetting->ReadRegKey(L"ListCtrlFont", sizeof(LOGFONT), (BYTE*)&lf))
		{
			m_ft.CreateFontIndirect(&lf);   
			GetListCtrl().SetFont(&m_ft, TRUE);
		}
	}
	else //drag and drop to load file
	{
		runOpenFile(cstr);
	}
}
void CNGLogViewAppView::OnGetDispInfo(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*) pNMHDR;
	
	if (pDispInfo->item.mask & LVIF_TEXT) 
	{
		ITEMINFO* pItem = (ITEMINFO*)pDispInfo->item.lParam;
		dprintf("[OnGetDispInfo] %d",pItem->nIndex  );
		
		if ((m_pLogFileLoader->m_bEnableRunBuffer) && (pItem->m_cLineBuffer==NULL))
		{
			m_pLogFileLoader->GetResultLine(pItem->nIndex , &(pItem->m_cLineBuffer));
			GetColors(pItem);
		}

		//new method:
		
		else if(!m_pLogFileLoader->m_bEnableRunBuffer)
		{
			
			pItem->m_cLineBuffer= new CLineBuffer();
    		m_pLogFileLoader->GetResultLine(pItem->nIndex , pItem->m_cLineBuffer);
			GetColors(pItem);
		}

		if (pItem->m_cLineBuffer!=NULL)
		{
			switch (pDispInfo->item.iSubItem) {

		case 0: // nIndex
			{
				CString string;
				string.Format (_T ("%u"), pItem->nIndex);
				::lstrcpy (pDispInfo->item.pszText, string);			
			}
			break;
		case 1: // line number
			{
				CString string;
				string.Format (_T ("%u"), pItem->m_cLineBuffer->m_nLineNumber);
				::lstrcpy (pDispInfo->item.pszText, string);
			}
			break;
		case 2: // PID
			{
				CString string;
				string.Format (_T ("%u"), pItem->m_cLineBuffer->m_nProcess);
				::lstrcpy (pDispInfo->item.pszText, string);			
			}
			break;
		case 3: // Time
			{
				::lstrcpyn(pDispInfo->item.pszText, pItem->m_cLineBuffer->m_wstrTimeString.c_str(), _MAX_PATH);
			}
			break;
		case 4: // message
			{
				::lstrcpyn(pDispInfo->item.pszText, pItem->m_cLineBuffer->m_wstrMessage.c_str(), _MAX_PATH);
			}
			break;
			}
		}
		if (!m_pLogFileLoader->m_bEnableRunBuffer)
			pItem->m_cLineBuffer = NULL;
	}
	*pResult = 0;
}

void CNGLogViewAppView::OnDeleteItem(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	ITEMINFO* pItem = (ITEMINFO*) pNMListView->lParam;

	if (pItem) delete pItem;

	*pResult = 0;
}
bool CNGLogViewAppView::AddItem(int nIndex)
{
	ITEMINFO* pItem;
	try {
		pItem = new ITEMINFO;
		pItem->m_cLineBuffer = NULL;
	}
	catch (CMemoryException* e) {
		e->Delete ();
		return FALSE;
	}
	
	//m_pLogFileLoader->GetResultLine(nIndex, &(pItem->m_cLineBuffer));
	//GetColors(pItem);
	pItem->nIndex = nIndex;


	LV_ITEM lvi;
	lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
	lvi.iItem = nIndex;
	lvi.iSubItem = 0;
	lvi.iImage = 0;
	lvi.pszText = LPSTR_TEXTCALLBACK;
	lvi.lParam = (LPARAM)pItem;
	lvi.stateMask = LVIS_STATEIMAGEMASK;

	if (m_setBookmarkIndex.find(nIndex) != m_setBookmarkIndex.end())
		lvi.state = INDEXTOSTATEIMAGEMASK(2);
	else
		lvi.state = INDEXTOSTATEIMAGEMASK(1);

	if(GetListCtrl().InsertItem(&lvi) == -1)
		return FALSE;

	return TRUE;
}

void CNGLogViewAppView::GetColors(ITEMINFO* pItem)
{
	map<std::wstring,  COLORPAIR>::iterator it;
	pItem->colors.m_cBkColor=::GetSysColor(COLOR_WINDOW);
	pItem->colors.m_cTextColor=::GetSysColor(COLOR_WINDOWTEXT);
	for (it=m_mapHighLightString.begin(); it!= m_mapHighLightString.end();++it)
	{
		wstring wstr = it->first;
		if( CheckSubString( pItem->m_cLineBuffer->m_wstrMessage.c_str(), wstr.c_str()) != NULL)
		{
			pItem->colors = it->second;
		}
	}

}

const wchar_t * CNGLogViewAppView::CheckSubString(const wchar_t *s1, const wchar_t *s2)
{
	if (m_props.bEnableMatchCase)
		return wcsstr(s1,s2);
	else
		return wcsistr(s1,s2); 
}

void CNGLogViewAppView::OnDestroy()
{
	if(m_pLogFileLoader)
	{
		delete m_pLogFileLoader;
		m_pLogFileLoader = NULL;
	}
	CNGListViewEx::OnDestroy ();
}
void CNGLogViewAppView::OnFileOpen()
{
	CFileDialog dlg (TRUE, _T("txt, log"), _T("all.LOG"), OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, _T("Log Files(*.log)|*.log|Text Files(*.txt)|*.txt|All Files(*.*)|*.*||"));
	if(dlg.DoModal() == IDOK)
	{
		CString str = dlg.GetPathName();
		if(str.GetLength() > 0)
		{
			runOpenFile(str);
		}
	}
}
// CNGLogViewAppView diagnostics
void CNGLogViewAppView::runOpenFile(CString str)
{
	SetStatusBarString("Loading...");
	TCHAR tszBuffer[LINE_BUFFER_SIZE];
	wsprintf(tszBuffer,TEXT("%s"),str);
	// Create file loader
	if(m_pLogFileLoader)
	{
		delete m_pLogFileLoader;
		m_pLogFileLoader = NULL;				
	}
	m_strPath = str;
	m_pLogFileLoader = new CLogFileLoader(tszBuffer);

	// Set title
	CString strTitle = _T ("NGLogViewer - ");
	strTitle += str;
	AfxGetMainWnd ()->SetWindowText (strTitle);

	Refresh();
	SetStatusBarString("Ready");
}
#ifdef _DEBUG
void CNGLogViewAppView::AssertValid() const
{
	CListView::AssertValid();
}

void CNGLogViewAppView::Dump(CDumpContext& dc) const
{
	CListView::Dump(dc);
}

CNGLogViewAppDoc* CNGLogViewAppView::GetDocument() const // non-debug version is inline
{
	return (CNGLogViewAppDoc*)m_pDocument;
}
#endif //_DEBUG


// CNGLogViewAppView message handlers
int CNGLogViewAppView::Refresh()
{
	if(m_pLogFileLoader == NULL)
		return 0;

	GetListCtrl ().DeleteAllItems ();

	// pre-processing
	//m_pLogFileLoader->PreProcessing();
	
	// Set properties
	SetProperties(&m_props);
	m_pLogFileLoader->SetCallbackPercentFunction((CLogFileLoaderCallback *)this);
	m_pLogFileLoader->RunFilterResult();
	int nTotal = m_pLogFileLoader->GetResultSize();
	dprintf(L"[NGLogViewAppView] nTotal =%d", nTotal);
	
	for (int i=0; i < nTotal; i++)
	{
		AddItem(i);
		OnAddListPercentCallback((float)i/(float)nTotal);
	}
	return nTotal;
}
void CNGLogViewAppView::OnFileSaveAs()
{
	CFileDialog dlg (FALSE, _T("txt, log"), _T("all.LOG"), OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, _T("Log Files(*.log)|*.log|Text Files(*.txt)|*.txt|All Files(*.*)|*.*||"));
	if(dlg.DoModal() == IDOK)
	{
		CString str = dlg.GetPathName();
		if(str.GetLength() > 0)
		{
			TCHAR tszBuffer[LINE_BUFFER_SIZE];
			wsprintf(tszBuffer,TEXT("%s"),str);
			if (m_pLogFileLoader)
				m_pLogFileLoader->SaveResultAs(tszBuffer);
		}
	}
}

void CNGLogViewAppView::OnEditFont()
{
	LOGFONT   lf;  
	CFontDialog cfd;
	if (cfd.DoModal()==IDOK)
	{
		cfd.GetCurrentFont(&lf);   
		m_pRecSetting->WriteRegKey(L"ListCtrlFont", sizeof(LOGFONT), (BYTE*)&lf);
		m_ft.CreateFontIndirect(&lf);   
		GetListCtrl().SetFont(&m_ft, TRUE);
	}
}

bool CNGLogViewAppView::SetStatusBarString(const char *szInput)
{
	CNGLogViewAppApp *pApp = (CNGLogViewAppApp *) AfxGetApp();   
	CMainFrame *pMFrame = (CMainFrame *)pApp->m_pMainWnd;   
	CString str(szInput);
	pMFrame->m_wndStatusBar.SetPaneText(0,str);   
	return true;
}

void CNGLogViewAppView::OnProperties()
{
	CNGPropertySheet ps(_T("Properties"));
	if (ps.DoModal() == IDOK)
	{
		// Reset Properties
		m_props = ps.m_FilterPage.m_PropInfo;
		m_mapHighLightString = ps.m_FilterPage.GetMapStringToColors();
		Refresh();
		Invalidate ();
	}
	
}
void CNGLogViewAppView::SetProperties(PROPINFO* props)
{
	if(m_pLogFileLoader)
	{
		m_pLogFileLoader->SetEnableFilterEmptyMessage(props->bEnableEmptyString);
		m_pLogFileLoader->SetEnableMatchCaseStringCompare(props->bEnableMatchCase);
		m_pLogFileLoader->SetKeyWordExcludeFilter(props->wszExcludeList);
		m_pLogFileLoader->SetKeyWordIncludeFilter(props->wszIncludeList);
	}
}

bool CNGLogViewAppView::OnPercentCallback(float fInput)
{
	char szBuffer[128] = {0};
	sprintf(szBuffer,"Loading ... %.02f%%", fInput*100);
	this->SetStatusBarString(szBuffer);
	return true;
}

bool CNGLogViewAppView::OnAddListPercentCallback(float fInput)
{
	char szBuffer[128] = {0};
	sprintf(szBuffer,"Displaying ... %.02f%%", fInput*100);
	this->SetStatusBarString(szBuffer);
	return true;
}
BOOL CNGLogViewAppView::PreTranslateMessage(MSG* pMsg) 
{
	if(GetFocus() == this)
	{
		LPTSTR lpszResourceName = MAKEINTRESOURCE(IDR_ACCELERATOR1);
		HINSTANCE hInst = AfxFindResourceHandle(lpszResourceName, RT_ACCELERATOR);
		HACCEL hAccel = ::LoadAccelerators(hInst, lpszResourceName);
		if(hAccel != NULL && ::TranslateAccelerator(m_hWnd, hAccel, pMsg))
			return TRUE;
	}
	return CNGListViewEx::PreTranslateMessage(pMsg);
}
void CNGLogViewAppView::OnFindDialog()
{
	if(GetWindowLong(GetSafeHwnd(), GWL_STYLE) & TVS_SHOWSELALWAYS)
	{
		m_bRemoveShowSelAlwaysAtFindDialogExit = false;
	}
	else
	{
		SetWindowLong(GetSafeHwnd(), GWL_STYLE,
			GetWindowLong(GetSafeHwnd(), GWL_STYLE) | TVS_SHOWSELALWAYS);
		m_bRemoveShowSelAlwaysAtFindDialogExit = true; //return back at dialog exit

		if(GetFocus() != this)
		{
			RedrawWindow();
		}
	}

	if(m_pFindDialog != NULL)
	{
		m_pFindDialog->SetFocus();
		return;
	}

	ASSERT(m_pFindDialog == NULL);
	m_pFindDialog = new CNGFindDialog();
	m_pFindDialog->m_fr.lpTemplateName=MAKEINTRESOURCE(IDD_NGFINDDIALOG);
	m_pFindDialog->Create(TRUE, m_fdinfo.strFind, _T(""), FR_DOWN | FR_ENABLETEMPLATE, this);

}
LONG CNGLogViewAppView::OnFindDialogMessage(WPARAM wParam,LPARAM lParam)
{
	ASSERT(m_pFindDialog != NULL);

	// If the FR_DIALOGTERM flag is set,
	// invalidate the handle identifying the dialog box.
	if (m_pFindDialog->IsTerminating())
	{
		if (m_bRemoveShowSelAlwaysAtFindDialogExit)
		{
			SetWindowLong(GetSafeHwnd(), GWL_STYLE,
				GetWindowLong(GetSafeHwnd(), 
				GWL_STYLE) & (~((LONG)TVS_SHOWSELALWAYS)));

			RedrawWindow();
		}

		m_pFindDialog = NULL;
		return 0;
	}

	if(wParam == WM_USER_SET_BOOKMARK_ALL)
	{
		FINDREPLACE* fr = (FINDREPLACE*)lParam;
		m_fdinfo.strFind = fr->lpstrFindWhat;
		m_fdinfo.bMatchCase = fr->Flags & FR_MATCHCASE;
		m_fdinfo.bMatchWholeWord = fr->Flags & FR_WHOLEWORD;

		OnSetBookmarkAll();
	}
	// If the FR_FINDNEXT flag is set,
	// call the application-defined search routine
	// to search for the requested string.
	else if(m_pFindDialog->FindNext())
	{
		// Read data from dialog
		m_fdinfo.strFind = m_pFindDialog->GetFindString();
		m_fdinfo.bMatchCase = m_pFindDialog->MatchCase() == TRUE;
		m_fdinfo.bMatchWholeWord = m_pFindDialog->MatchWholeWord() == TRUE;
		m_fdinfo.bSearchDown = m_pFindDialog->SearchDown() == TRUE;

		//Start find string
		OnFindString();
	}

	
	return 0;
}

void CNGLogViewAppView::OnFindString()
{
	if(m_fdinfo.strFind == _T(""))
		return ;

	//with given name do search
	FindWhatYouNeed(m_fdinfo.strFind, m_fdinfo.bMatchCase, m_fdinfo.bMatchWholeWord, m_fdinfo.bSearchDown);
}

bool CNGLogViewAppView::FindWhatYouNeed(CString strFind, bool bMatchCase, bool bMatchWholeWord, bool bSearchDown)
{
	CListCtrl& ListCtrl=GetListCtrl();
	if(bMatchCase == false)
		strFind.MakeUpper();

	bool bContinueSearch = true;

	// Clean current select
	int nStartPos = GetSelectItem();
	int nEndPos   = ListCtrl.GetItemCount() - 1;
	if (nStartPos != -1)
	{
		CleanItemState(nStartPos);
	}

	//Is startPos outside the list (happens if last item is selected)
	if(nStartPos > nEndPos || nStartPos == -1)
		nStartPos = 0;

	int nCurPos = nStartPos;
	do{
		if(bSearchDown)
			nCurPos++;
		else
			nCurPos--;

		// Find item from begin
		if(nCurPos > nEndPos)
			nCurPos = 0;
		
		// Find item from end
		else if(nCurPos < 0)
			nCurPos = nEndPos;
		
		// Not found
		if(nStartPos == nCurPos)
		{
			bContinueSearch = false;
			WCHAR wszMsg[_MAX_PATH];
			ZeroMemory(wszMsg, 0, _MAX_PATH);
			swprintf(wszMsg, L"%s\n%s", L"The following specified text was not found:", m_fdinfo.strFind);
			MessageBoxW(wszMsg, L"NGLogViewer", MB_ICONINFORMATION | MB_OK);

			break;
		}		
		ITEMINFO* pItem = (ITEMINFO*)ListCtrl.GetItemData(nCurPos);
		if(IsFindString(pItem))
		{
			UpdateItemState(nCurPos);
			ListCtrl.EnsureVisible(nCurPos, FALSE);
			SetFocus();
			return true;
		}
	}while(bContinueSearch);
	
	return false;
}

void CNGLogViewAppView::OnSetBookmark()
{
	CListCtrl& ListCtrl=GetListCtrl();
	int nIndex = GetSelectItem();
	if(SetBookmark(nIndex))
	{
		ListCtrl.SetItemState(nIndex,
			INDEXTOSTATEIMAGEMASK(2), LVIS_STATEIMAGEMASK);
	}
	else
	{
		ListCtrl.SetItemState(nIndex,
			INDEXTOSTATEIMAGEMASK(1), LVIS_STATEIMAGEMASK);
	}
	Invalidate();
	return;
}

void CNGLogViewAppView::OnFindNextBookmark()
{
	CListCtrl& ListCtrl=GetListCtrl();
	int nCurPos = GetSelectItem();
	int nRet=-1;
	set<int>::iterator it;
	for (it = m_setBookmarkIndex.begin();it!=m_setBookmarkIndex.end();++it)
	{
		if (*it ==nCurPos)
		{
			it++;
			if (it!=m_setBookmarkIndex.end())
			{
				nRet = *it;
				break;
			}
			else
			{
				nRet=-1;
				break;
			}
		}
		else if (*it>nCurPos)
		{
			nRet = *it;
			break;
		}
	}

	if(nRet==-1 && (m_setBookmarkIndex.size()!=0))
	{
		it = m_setBookmarkIndex.begin();
		nRet= *it;
	}

	if (nRet != -1)
	{
		CleanItemState(nCurPos);
		UpdateItemState(nRet);
		ListCtrl.EnsureVisible(nRet, FALSE);
		SetFocus();
	}
	return;
}
// Kill item focus and restore item state
void CNGLogViewAppView::CleanItemState(int nItem , UINT uStateFlag /* = LVIS_SELECTED | LVIS_FOCUSED */, UINT uStateMask /* = LVIF_STATE | LVIS_STATEIMAGEMASK */)
{
	CListCtrl& ListCtrl=GetListCtrl();
	UINT uCurState = ListCtrl.GetItemState(nItem, 0xFFFF);
	uCurState &= ~uStateFlag;
	
	ListCtrl.SetItem(nItem, 0, uStateMask, NULL, 0, 0, 0xFFFF, 0);
	ListCtrl.SetItemState(nItem, uCurState, uStateMask);
	return;
}
// Set item focus and restore item state
void CNGLogViewAppView::UpdateItemState(int nItem, UINT uStateFlag /* = LVIS_SELECTED | LVIS_FOCUSED */, UINT uStateMask /* = LVIF_STATE | LVIS_STATEIMAGEMASK */)
{
	CListCtrl& ListCtrl=GetListCtrl();
	UINT uCurState = ListCtrl.GetItemState(nItem, 0xFFFF);
	uCurState |= uStateFlag;
	ListCtrl.SetItem(nItem, 0, uStateMask, NULL, 0, uCurState, uCurState, 0);
	return;
}
void CNGLogViewAppView::OnSetBookmarkAll()
{
	if(m_fdinfo.strFind == _T(""))
		return;

	bool bFind = false;
	CListCtrl& ListCtrl=GetListCtrl();
	int nRet = -1;

	for(int i = 0; i < ListCtrl.GetItemCount(); i++)
	{
		ITEMINFO* pItem = (ITEMINFO*)ListCtrl.GetItemData(i);
		if (IsFindString(pItem))
		{
			bFind = true;
			if(nRet == -1)
				nRet = i;
			if (m_setBookmarkIndex.find(i) == m_setBookmarkIndex.end())
			{
				m_setBookmarkIndex.insert(i);
				ListCtrl.SetItemState(i, INDEXTOSTATEIMAGEMASK(2), LVIS_STATEIMAGEMASK);
			}
		}
	}
	
	if(!bFind)
	{
		WCHAR wszMsg[_MAX_PATH];
		ZeroMemory(wszMsg, 0, _MAX_PATH);
		swprintf(wszMsg, L"%s\n%s", L"The following specified text was not found:", m_fdinfo.strFind);
		MessageBoxW(wszMsg, L"NGLogViewer", MB_ICONINFORMATION | MB_OK);
	}

	if(nRet != -1)
	{
		CleanItemState(GetSelectItem());
		UpdateItemState(nRet);
		ListCtrl.EnsureVisible(nRet, FALSE);
		SetFocus();
	}
	return;

}
bool CNGLogViewAppView::IsFindString(ITEMINFO* pItem)
{
	bool bRet = false;
	bool bNewInThisFunction = false;

	if (pItem->m_cLineBuffer == NULL)
	{
		bNewInThisFunction = true;
		pItem->m_cLineBuffer= new CLineBuffer();
		m_pLogFileLoader->GetResultLine(pItem->nIndex , pItem->m_cLineBuffer);
	}

	CString strMsg = pItem->m_cLineBuffer->m_wstrMessage.c_str();
	
	CString strFind = m_fdinfo.strFind;

	if(!m_fdinfo.bMatchCase)
	{
		strMsg.MakeUpper();
		strFind.MakeUpper();
	}

	if((m_fdinfo.bMatchWholeWord && strMsg == m_fdinfo.strFind) || (!m_fdinfo.bMatchWholeWord && (strMsg.Find(strFind) >= 0)))
		bRet = true;
	else
		bRet = false;
	
	if (bNewInThisFunction)
	{
		delete pItem->m_cLineBuffer;
		pItem->m_cLineBuffer=NULL;
	}
	
	return bRet;
}
void CNGLogViewAppView::OnClearBookmark()
{
	CListCtrl& ListCtrl=GetListCtrl();
	set<int>::iterator it;
	
	for (it = m_setBookmarkIndex.begin() ; it!=m_setBookmarkIndex.end() ; ++it)
		ListCtrl.SetItemState(*it, INDEXTOSTATEIMAGEMASK(1), LVIS_STATEIMAGEMASK);
	
	m_setBookmarkIndex.clear();
	Invalidate();

	return;
}
void CNGLogViewAppView::OnEditCopy()
{
	UpdateData();

	CListCtrl& ListCtrl=GetListCtrl();
	int nCount = ListCtrl.GetSelectedCount();
	int nItem = -1;
	int nLength = nCount*_MAX_PATH+1;
	char *buf = new char[nLength];
	// Update all of the selected items.
	if (nCount > 0)
	{
		CString CStrTemp=_T("");
		for(int i=0;i < nCount;i++)
		{
			nItem = ListCtrl.GetNextItem(nItem, LVNI_SELECTED);
			ASSERT(nItem != -1);
			CString CStrText[5];
			for(int n=0;n<5;n++) 
			{
				CStrText[n] = ListCtrl.GetItemText(nItem,n);
				if( n != 4 )CStrTemp += CStrText[n] + _T("\t");
				else		CStrTemp += CStrText[n] + _T("\n");
			}
	   }
	   sprintf(buf, "%S", CStrTemp);
	}
	//SelectedListViewItemCollection& Selected = this->SelectedItems;
	CClipboard::SetText(buf);

	delete[] buf;
}

