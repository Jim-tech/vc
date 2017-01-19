#include "stdafx.h"
#include "ColorListCtrl.h"

/////////////////////////////////////////////////////////////////////////////  
// CColorListCtrl  
  
CColorListCtrl::CColorListCtrl()  
{  

	m_NextItemId = 0;
}
  
CColorListCtrl::~CColorListCtrl()  
{  
	
}  
  
  
BEGIN_MESSAGE_MAP(CColorListCtrl, CListCtrl)  
    //{{AFX_MSG_MAP(CLineColorListCtrl)  
        // NOTE - the ClassWizard will add and remove mapping macros here.  
    //}}AFX_MSG_MAP  
    ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnNMCustomDraw)  
END_MESSAGE_MAP()
  
/////////////////////////////////////////////////////////////////////////////  
// CLineColorListCtrl message handlers  
  
  
void CColorListCtrl::OnNMCustomDraw(NMHDR *pNMHDR, LRESULT *pResult)    
{    
    // TODO: Add your control notification handler code here     
    *pResult = CDRF_DODEFAULT;    
    NMLVCUSTOMDRAW * lplvdr=(NMLVCUSTOMDRAW*)pNMHDR;    
    NMCUSTOMDRAW &nmcd = lplvdr->nmcd;    
    switch(lplvdr->nmcd.dwDrawStage)//判断状态     
    {    
    case CDDS_PREPAINT:    
        {    
            *pResult = CDRF_NOTIFYITEMDRAW;    
            break;    
        }    
    case CDDS_ITEMPREPAINT://如果为画ITEM之前就要进行颜色的改变     
        {    
            TEXT_BK tb;  
  
            if(MapItemColor.Lookup(nmcd.dwItemSpec, tb))    
                // 根据在 SetItemColor(DWORD iItem, COLORREF color) 设置的     
                // ITEM号和COLORREF 在摸板中查找，然后进行颜色赋值。     
            {    
                lplvdr->clrText = tb.colText;     
                lplvdr->clrTextBk = tb.colTextBk;    
                *pResult = CDRF_DODEFAULT;    
            }    
        }    
        break;    
    }    
}    
  
void CColorListCtrl::ClearColor()  
{  
    MapItemColor.RemoveAll();  
}  
  
void CColorListCtrl::SetItemColor(DWORD iItem, COLORREF TextColor, COLORREF TextBkColor)  
{    
    TEXT_BK tb;  
    tb.colText = TextColor;  
    tb.colTextBk = TextBkColor;  
  
  
    MapItemColor.SetAt(iItem, tb);//设置某行的颜色。     
    this->RedrawItems(iItem, iItem);//重新染色     
      
    //this->SetCheck(iItem,1);     
    this->SetFocus();    //设置焦点     
    UpdateWindow();    
}    
  
  
void CColorListCtrl::SetAllItemColor(DWORD iItem, COLORREF TextColor, COLORREF TextBkColor)    
{    
    //INT_PTR ncount = this->GetItemCount();  
    TEXT_BK tb;  
    tb.colText = TextColor;  
    tb.colTextBk = TextBkColor;  
  
    if(iItem > 0)    
    {    
        for(DWORD numItem = 0; numItem < iItem ;numItem ++)    
        {    
            //iItem = i;     
            MapItemColor.SetAt(numItem, tb);    
            this->RedrawItems(numItem, numItem);    
        }    
    }    
      
    return;    
      
}    

int CColorListCtrl::AddLog(CString strLog, COLORREF color, COLORREF bkcolor)
{
	InsertItem(m_NextItemId, strLog);
	SetItemColor(m_NextItemId, color, bkcolor);
	EnsureVisible(m_NextItemId, true);
	m_NextItemId++;
	return 0;
}

int CColorListCtrl::EmptyLog()
{
	DeleteAllItems();
	m_NextItemId = 0;
	UpdateWindow();
	return 0;
}
