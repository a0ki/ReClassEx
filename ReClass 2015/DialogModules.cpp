#include "stdafx.h"
#include "ReClass2015.h"
#include "DialogModules.h"

#include "MainFrm.h"
#include "ChildFrm.h"
#include "SDK.h"

#include "afxdialogex.h"

IMPLEMENT_DYNAMIC(CDialogModules, CDialogEx)

CDialogModules::CDialogModules(CWnd* pParent) : CDialogEx(CDialogModules::IDD, pParent)
{
}

CDialogModules::~CDialogModules()
{
}

void CDialogModules::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MODULELIST, m_ModuleViewList);
	DDX_Control(pDX, IDC_MODULENAME, m_Edit);
}

void CDialogModules::OnSize(UINT nType, int cx, int cy)
{

}

BEGIN_MESSAGE_MAP(CDialogModules, CDialogEx)
	ON_EN_CHANGE(IDC_MODULENAME, &CDialogModules::OnEnChangeModuleName)
END_MESSAGE_MAP()


void CDialogModules::BuildList()
{
	for (UINT i = 0; i < MemMapModule.size(); i++)
	{
		MemMapInfo moduleInfo = MemMapModule[i];

		SHFILEINFO    sfi;
		SHGetFileInfo(MemMapModule[i].Path, FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_USEFILEATTRIBUTES);
		m_ImageList.Add(sfi.hIcon);

		CString name = moduleInfo.Name;
		if (m_Filter.GetLength() != 0 && name.MakeUpper().Find(m_Filter.MakeUpper()) == -1)
			continue;

		TCHAR strStart[64];
		_stprintf(strStart, _T("0x%IX"), moduleInfo.Start);
		TCHAR strEnd[64];
		_stprintf(strEnd, _T("0x%IX"), moduleInfo.End);
		TCHAR strSize[64];
		_stprintf(strSize, _T("0x%X"), moduleInfo.Size);

		AddData(i, (LPTSTR)name.GetString(), strStart, strEnd, strSize, static_cast<LPARAM>(moduleInfo.Start));
	}
}

BOOL CDialogModules::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_ImageList.Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ILC_COLOR32, 1, 1);
	m_ImageList.SetBkColor(RGB(255, 255, 255));

	m_ModuleViewList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

	m_ModuleViewList.InsertColumn(COLUMN_MODULE, _T("Module"), LVCFMT_LEFT, 200);
	m_ModuleViewList.InsertColumn(COLUMN_START, _T("Start"), LVCFMT_LEFT, 200);
	m_ModuleViewList.InsertColumn(COLUMN_END, _T("End"), LVCFMT_LEFT, 200);
	m_ModuleViewList.InsertColumn(COLUMN_SIZE, _T("Size"), LVCFMT_LEFT, 200);

	m_ModuleViewList.SetImageList(&m_ImageList, LVSIL_SMALL);

	BuildList();

	return TRUE;
}

__inline int FindModuleByName(const TCHAR* szName)
{
	for (int id = 0; id < MemMapModule.size(); id++)
	{
		MemMapInfo moduleInfo = MemMapModule[id];
		if (_tcsicmp(moduleInfo.Name, szName) == 0)
			return id;
	}
	return -1;
};

void CDialogModules::OnOK()
{
	unsigned numselected = m_ModuleViewList.GetSelectedCount();
	POSITION pos = m_ModuleViewList.GetFirstSelectedItemPosition();
	//while (pos)
	//{
	//	int nItem = m_ModuleViewList.GetNextSelectedItem(pos);
	//	CString szBuffer = m_ModuleViewList.GetItemText(nItem, 0);
	//
	//	#ifdef _DEBUG
	//	_tprintf(_T("nitem %d\n"), nItem);
	//	#endif
	//
	//	nItem = FindModuleByName(szBuffer.GetBuffer());
	//
	//	//printf( "szBuffer %s new %d\n", szBuffer.GetBuffer( ), nItem );
	//	CMainFrame*  pFrame = static_cast<CMainFrame*>(AfxGetApp()->m_pMainWnd);
	//	CChildFrame* pChild = (CChildFrame*)pFrame->CreateNewChild(RUNTIME_CLASS(CChildFrame), IDR_ReClass2015TYPE, theApp.m_hMDIMenu, theApp.m_hMDIAccel);
	//	pChild->m_wndView.m_pClass = theApp.Classes[nItem];
	//
	//	// This will get overwritten for each class that is opened
	//	pChild->SetTitle(MemMapModule[nItem].Name);
	//	pChild->SetWindowText(MemMapModule[nItem].Name);
	//	pFrame->UpdateFrameTitleForDocument(MemMapModule[nItem].Name);
	//}

	CDialogEx::OnOK();
}

int CDialogModules::AddData(int Index, LPTSTR ModuleName, LPTSTR StartAddress, LPTSTR EndAddress, LPTSTR ModuleSize, LPARAM lParam)
{
	LVITEM lvi = { 0 };

	lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;

	lvi.pszText = (LPTSTR)ModuleName;
	lvi.cchTextMax = static_cast<int>(_tcslen(ModuleName)) + 1;
	lvi.iImage = Index;
	lvi.lParam = lParam;
	lvi.iItem = ListView_GetItemCount(m_ModuleViewList.GetSafeHwnd());

	int pos = ListView_InsertItem(m_ModuleViewList.GetSafeHwnd(), &lvi);

	m_ModuleViewList.SetItemText(pos, COLUMN_START, (LPTSTR)StartAddress);
	m_ModuleViewList.SetItemText(pos, COLUMN_END, (LPTSTR)EndAddress);
	m_ModuleViewList.SetItemText(pos, COLUMN_SIZE, (LPTSTR)ModuleSize);

	return pos;
}

void CDialogModules::OnEnChangeModuleName()
{
	m_Edit.GetWindowText(m_Filter);
	m_ModuleViewList.DeleteAllItems();
	BuildList();
}