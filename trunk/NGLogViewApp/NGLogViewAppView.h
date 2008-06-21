// NGLogViewAppView.h : interface of the CNGLogViewAppView class
//

#pragma once

#include "..\NGLogViewer\CLogFileLoader.h"
#include "..\Utility\CRegSetting.h"
#include "NGLogViewAppDoc.h"
#include "CNGListViewEx.h"
#include "FilterPage.h"

typedef struct tagITEMINFO {
	INT         nIndex;
	CLineBuffer *m_cLineBuffer;
} ITEMINFO;

class CNGLogViewAppView : public CNGListViewEx, public CLogFileLoaderCallback
{
protected: // create from serialization only
	CNGLogViewAppView();
	DECLARE_DYNCREATE(CNGLogViewAppView)

// Attributes
public:
	CNGLogViewAppDoc* GetDocument() const;
	bool OnPercentCallback(float fInput);
// Operations

// Overrides
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual void OnInitialUpdate(); // called first time after construct

// Implementation
public:
	int Refresh();
	virtual ~CNGLogViewAppView();
	bool SetStatusBarString(const char *szInput);
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	CLogFileLoader* m_pLogFileLoader;
	CRegSetting* m_pRecSetting;
	CString m_strPath;
	CFont   m_ft;
	PROPINFO m_props;
	BOOL AddItem(int nIndex);
	bool OnAddListPercentCallback(float fInput);
	void SetProperties(PROPINFO* props);
	afx_msg void OnDestroy();
	afx_msg void OnGetDispInfo(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnFileOpen();
	afx_msg void OnDeleteitem(NMHDR* pNMHDR, LRESULT* pResult);

// Generated message map functions
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnFileSaveAs();
	afx_msg void OnEditFont();
	afx_msg void OnProperties();
};

#ifndef _DEBUG  // debug version in NGLogViewAppView.cpp
inline CNGLogViewAppDoc* CNGLogViewAppView::GetDocument() const
   { return reinterpret_cast<CNGLogViewAppDoc*>(m_pDocument); }
#endif

