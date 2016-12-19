#ifndef __COLORLISTCTRL_H__
#define __COLORLISTCTRL_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

typedef struct    
{  
    COLORREF colText;  
    COLORREF colTextBk;  
}TEXT_BK;


/////////////////////////////////////////////////////////////////////////////  
// CColorListCtrl window  
  
class CColorListCtrl : public CListCtrl  
{  
public:  
    void SetItemColor(DWORD iItem, COLORREF TextColor, COLORREF TextBkColor);   //����ĳһ�е�ǰ��ɫ�ͱ���ɫ  
    void SetAllItemColor(DWORD iItem, COLORREF TextColor, COLORREF TextBkColor);//����ȫ���е�ǰ��ɫ�ͱ���ɫ  
    void ClearColor();                                                          //�����ɫӳ���  
  
// Construction  
public:  
    CColorListCtrl();  
  
// Attributes  
public:  
    CMap<DWORD, DWORD&, TEXT_BK, TEXT_BK&> MapItemColor;  
      
  
// Operations  
public:  
  
// Overrides  
    // ClassWizard generated virtual function overrides  
    //{{AFX_VIRTUAL(CColorListCtrl)  
    //}}AFX_VIRTUAL  
  
// Implementation  
public:  
    virtual ~CColorListCtrl();  
  
    // Generated message map functions  
protected:  
    //{{AFX_MSG(CColorListCtrl)  
        // NOTE - the ClassWizard will add and remove member functions here.  
    //}}AFX_MSG  
    void CColorListCtrl::OnNMCustomDraw(NMHDR *pNMHDR, LRESULT *pResult);  
  
    DECLARE_MESSAGE_MAP()  
public:
	int m_NextItemId;
	int AddLog(CString strLog, COLORREF color = RGB(0, 0, 0), COLORREF bkcolor = RGB(0xff, 0xff, 0xff));
    int EmptyLog();
    CPoint old;
};

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __COLORLISTCTRL_H__ */
