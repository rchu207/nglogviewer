
#include "stdafx.h"
#include "NGLogViewApp.h"

#include "NGLogViewAppDoc.h"
#include "NGLogViewAppView.h"
#include "MainFrm.h"
#include "NGPropertySheet.h"

#define APP_SETTING_PATH (L"SOFTWARE\\NG\\LOGViewer")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CNGLogViewAppView

IMPLEMENT_DYNCREATE(CNGLogViewAppView, CNGListViewEx)

BEGIN_MESSAGE_MAP(CNGLogViewAppView, CNGListViewEx)
	ON_WM_DESTROY()
	ON_NOTIFY_REFLECT(LVN_DELETEITEM, OnDeleteitem)
	ON_NOTIFY_REFLECT(LVN_GETDISPINFO, OnGetDispInfo)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	ON_COMMAND(ID_FILE_SAVE_AS, &CNGLogViewAppView::OnFileSaveAs)
	ON_COMMAND(ID_EDIT_FONT, &CNGLogViewAppView::OnEditFont)
	ON_COMMAND(ID_EDIT_PROPERTIES, &CNGLogViewAppView::OnProperties)
END_MESSAGE_MAP()

// CNGLogViewAppView construction/destruction

CNGLogViewAppView::CNGLogViewAppView()
{
	// TODO: add construction code here
	m_pLogFileLoader = NULL;
	m_pRecSetting = new CRegSetting(APP_SETTING_PATH);
	m_strPath = _T("");
	ZeroMemory(&m_props, sizeof(PROPINFO));
	
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


	// TODO: You may populate your ListView with items by directly accessing
	//  its list control through a call to GetListCtrl().
	
	// Add column
	GetListCtrl().InsertColumn(0, _T("Index"), LVCFMT_CENTER, 55);
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

	// Add data
	//	Refresh();
	
}
void CNGLogViewAppView::OnGetDispInfo(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*) pNMHDR;
	
	if (pDispInfo->item.mask & LVIF_TEXT) 
	{
		ITEMINFO* pItem = (ITEMINFO*)pDispInfo->item.lParam;
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
				CString string;
				if (pItem->m_cLineBuffer->m_tTime!= 0 || (pItem->m_cLineBuffer->m_tTime==0) )
					string.Format(_T("%f"), pItem->m_cLineBuffer->m_fTime);
				else
				{
					struct tm * timeinfo;
					timeinfo = localtime(&pItem->m_cLineBuffer->m_tTime);
					string.Format(_T("%d:%d:%d"), timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
				}
				::lstrcpy (pDispInfo->item.pszText, string);			
			}
			break;
		case 4: // message
			{
				::lstrcpyn(pDispInfo->item.pszText, pItem->m_cLineBuffer->m_wstrMessage.c_str(), _MAX_PATH);
			}
			break;
			}
		}
	}
	*pResult = 0;
}

void CNGLogViewAppView::OnDeleteitem(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	ITEMINFO* pItem = (ITEMINFO*) pNMListView->lParam;

	if (pItem) delete pItem;

	*pResult = 0;
}

BOOL CNGLogViewAppView::AddItem(int nIndex)
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
	
	m_pLogFileLoader->GetResultLine(nIndex, &(pItem->m_cLineBuffer));
	pItem->nIndex = nIndex;


	LV_ITEM lvi;
	lvi.mask = LVIF_TEXT | LVIF_PARAM;
	lvi.iItem = nIndex;
	lvi.iSubItem = 0;
	lvi.iImage = 0;
	lvi.pszText = LPSTR_TEXTCALLBACK;
	lvi.lParam = (LPARAM)pItem;
	
	if(GetListCtrl().InsertItem(&lvi) == -1)
		return FALSE;

	return TRUE;
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
	}
}
// CNGLogViewAppView diagnostics

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
		ZeroMemory(&m_props, sizeof(PROPINFO));
		m_props.bEnableEmptyString = ps.m_FilterPage.m_bEnableEmptyString;
		wcscpy(m_props.wszExcludeList, ps.m_FilterPage.m_wszExcludeList);
		wcscpy(m_props.wszIncludeList, ps.m_FilterPage.m_wszIncludeList);
		
		Refresh();
		Invalidate ();
	}
	
}
void CNGLogViewAppView::SetProperties(PROPINFO* props)
{
	if(m_pLogFileLoader)
	{
		m_pLogFileLoader->SetEnableFilterEmptyMessage(props->bEnableEmptyString);
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