/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies). 
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:  
*
*/



// INCLUDE FILES
#include "SGSettingListContainer.h"
#include <ScreenGrabber2.rsg>
#include "SG.hrh"
#include "SGDocument.h"


#ifdef SCREENGRABBER_MULTIDRIVE_SUPPORT
  #include <caknmemoryselectionsettingitemmultidrive.h>
#else
  #include <caknmemoryselectionsettingitem.h>
#endif

#include <eikappui.h>
#include <akntitle.h>
#include <eikspane.h> 

#include <akndef.h>


#include <aknmessagequerydialog.h>
#include <aknmessagequerycontrol.h>
#include <aknnotewrappers.h>
#include <eikfrlbd.h>
#include <avkon.mbg>
#include <akniconarray.h>
#include <aknconsts.h>
#include <aknlists.h>
#include <aknpopup.h>
#include <bautils.h>
#include <akncommondialogs.h>
#include <caknfileselectiondialog.h>

class HotkeyPickerDialog: public CAknMessageQueryDialog
{

public:
    TUint iInternalKeyValue;
    TUint& iExternalKeyValue;

public:
    HotkeyPickerDialog(TUint &aKeyValue) : CAknMessageQueryDialog(ENoTone), iExternalKeyValue(aKeyValue)
    {
	iInternalKeyValue = 0;
    }

    void UpdateMessageTextL(TDesC &aText)
    {
	CAknMessageQueryControl* c = (CAknMessageQueryControl*)ControlOrNull(EAknMessageQueryContentId);
	if (c)
	{
	    c->SetMessageTextL(&aText);
	    DrawNow();
	}
    }

    void UpdateMessageTextL(TUint aKeyValue)
    {
	TBuf<64> msg;
	msg.Format(_L("scancode: 0x%x"), aKeyValue);
	UpdateMessageTextL(msg);

    }
   
    TBool OkToExitL(TInt aButton)
    {
	if ((aButton == EAknSoftkeyOk) && iInternalKeyValue) iExternalKeyValue = iInternalKeyValue;
	return CAknMessageQueryDialog::OkToExitL(aButton);
    }

    TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType)
    {
	TUint scanCode = aKeyEvent.iScanCode;
	if ((scanCode != EStdKeyDevice0) && (scanCode != EStdKeyDevice1) && (scanCode != EStdKeyDevice3) )
	{

	    if (aType == EEventKeyDown){
		iInternalKeyValue = scanCode;
		UpdateMessageTextL(scanCode); 
	    }
	}

	return EKeyWasConsumed;
    }

    static void ExecuteLD(TUint aCurrentKey, TUint &aKeyValue)
    {
	HotkeyPickerDialog* dlg = new (ELeave) HotkeyPickerDialog(aKeyValue);
	dlg->PrepareLC(R_SCREENGRABBER_HOTKEYPICKER_DIALOG);
	dlg->UpdateMessageTextL(aCurrentKey);
	dlg->RunLD();
    }
 
};


class CHotkeySettingItem: public CAknEnumeratedTextPopupSettingItem
{
public:    
    CHotkeySettingItem(TInt aResourceId, TInt& aValue, TGrabSettings& aGrabSettings, CScreenGrabberModel* aModel):
	CAknEnumeratedTextPopupSettingItem(aResourceId, aValue),
	iGrabSettings(aGrabSettings),
	iSGModel(aModel){};

    // From MAknSettingPageObserver
    virtual void HandleSettingPageEventL(CAknSettingPage* aSettingPage,TAknSettingPageEvent aEventType);


private:
    TGrabSettings& iGrabSettings;
    CScreenGrabberModel* iSGModel;
};


void CHotkeySettingItem::HandleSettingPageEventL(CAknSettingPage* aSettingPage,TAknSettingPageEvent aEventType)
{

    TInt currentIndex = QueryValue()->CurrentValueIndex();
    if (aEventType == EEventSettingOked  && currentIndex == ECustomHotkey)
    {

	iSGModel->StopCapturing();
	iSGModel->CaptureAllKeys();

	TUint currentKey = iSGModel->GrabSettings().iCustomCaptureHotkey;
	HotkeyPickerDialog::ExecuteLD(currentKey, iGrabSettings.iCustomCaptureHotkey);
	iSGModel->CancelCaptureAllKeys();

    }
}

class CSavePathSettingItem: public CAknTextSettingItem
{
public:    
    CSavePathSettingItem(TInt aResourceId, TDes &aValue):CAknTextSettingItem(aResourceId, aValue)
    {
	//iOldPath.Copy(aValue);
    };

    virtual void EditItemL(TBool aCalledFromMenu);
    virtual void HandleSettingPageEventL(CAknSettingPage* aSettingPage,TAknSettingPageEvent aEventType);
    TBool SelectDrivePathL(TDes &aPath);
    TBool SelectPathL(TDes &aPath);
private:
    TFileName iOldPath;

};


void CSavePathSettingItem::HandleSettingPageEventL(CAknSettingPage* aSettingPage, TAknSettingPageEvent aEventType)
{

    if (aEventType == EEventSettingCancelled)
    {
	InternalTextPtr().Copy(iOldPath);
    }
}



static void DoFormatSize(TDes &aDes, TInt64 aSize)
{
    wchar_t* sizeSnits[] = {L"B", L"KB", L"MB", L"GB", L"TB"};
    TReal size = (TReal)aSize;
    for (TInt i=0; i < 5; i++){
	if (size < 1024.0)
	{
            aDes.Format(_L("%.2f%s"), size, sizeSnits[i]);
	    break;
	}
        size /= 1024.0;
    } 
}

TBool CSavePathSettingItem::SelectDrivePathL(TDes &aPath)
{

    CAknDoubleLargeGraphicPopupMenuStyleListBox* list;

    list = new(ELeave) CAknDoubleLargeGraphicPopupMenuStyleListBox;  
    CleanupStack::PushL(list);

    CAknPopupList* popupList;
    popupList = CAknPopupList::NewL(list, R_AVKON_SOFTKEYS_SELECT_CANCEL, AknPopupLayouts::EMenuDoubleLargeGraphicWindow);   
    CleanupStack::PushL(popupList);

    HBufC* title = CCoeEnv::Static()->AllocReadResourceLC(R_SAVEPATHSELECTION_DIALOG_TITLE);
    popupList->SetTitleL(*title);
    CleanupStack::PopAndDestroy(title);

    TInt flags = 0; // Initialize flag
    list->ConstructL(popupList, flags);
    list->CreateScrollBarFrameL(ETrue);
    list->ScrollBarFrame()->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff,CEikScrollBarFrame::EAuto);
    //list->EnableExtendedDrawingL();
    list->ItemDrawer()->FormattedCellData()->EnableMarqueeL(ETrue);
    CDesCArrayFlat* items = new CDesCArrayFlat(3);
    CleanupStack::PushL(items);
 
    CArrayPtr<CGulIcon>* icons =  new ( ELeave ) CAknIconArray(3);
    CleanupStack::PushL(icons);

    // Add drives
    CDesCArrayFlat* driveList = new CDesCArrayFlat(3);
    RFs& fs = CCoeEnv::Static()->FsSession();
    BaflUtils::GetDiskListL(fs, *driveList);
 
    //CleanupStack::PushL(driveList); // KERN-EXEC3 when this function returns!!
   
    TInt i;
    TInt iconIndex = 0;
    
    if ( driveList->Find(_L("D"), i) == 0 )
    {
	    driveList->Delete(i);
    }
	
    if ( driveList->Find(_L("Z"), i) == 0 )
    {
	driveList->Delete(i);
    }

    for (i = 0; i < driveList->MdcaCount(); i++)
    {

	TPtrC d = driveList->MdcaPoint(i); 	
	TBuf<3> root(d);
	root.Append(_L(":\\"));
	TBool rootOK(EFalse);

	if (BaflUtils::PathExists(fs, root))
	{
	    
	    TBool isRO(EFalse); 
	    if (BaflUtils::DiskIsReadOnly(fs, root, isRO) == KErrNone)
	    {
		rootOK = !isRO;	
	    }
    
	}

	if (!rootOK)
	{
	    driveList->Delete(i);
	    continue;
	}

	TBuf<KMaxFileName+64> item;
	TBuf<64> sizeInfo1;
	//TBuf<64> sizeInfo2;
	TVolumeInfo driveInfo;
	TInt driveNumber;

 	fs.CharToDrive(d[0], driveNumber); 
        if (fs.Volume(driveInfo, driveNumber) != KErrNone)
	{
	    driveList->Delete(i);
	    continue;
	}
	
	TBuf<KMaxFileName> driveName = driveInfo.iName; // Optional name of the volume
	
	if (driveName.Length() == 0)
	{
	    TInt mediaType = driveInfo.iDrive.iType;
            if (mediaType == EMediaNANDFlash){ 
		if (d[0] == 'C') driveName = _L("Phone memory");
		else  driveName = _L("NAND flash");
	    }

	    if (mediaType == EMediaHardDisk) driveName = _L("Mass storage");
	    else if (mediaType == EMediaRemote) driveName = _L("Remote");
	}

	DoFormatSize(sizeInfo1, driveInfo.iFree);
        //DoFormatSize(sizeInfo2, driveInfo.iSize);
	item.Format(_L("%d\t%S: %S\tFree: %S\t"), iconIndex, &d, &driveName, &sizeInfo1);
	items->AppendL(item);
	iconIndex++;

	CFbsBitmap *bm, *mask;
	if (d[0] == 'F' || d[0] == 'E')
	{
	    AknIconUtils::CreateIconL(bm, mask, AknIconUtils::AvkonIconFileName(), EMbmAvkonQgn_prop_mmc_memc_large, EMbmAvkonQgn_prop_mmc_memc_large_mask);
  
	}

	else
	{
    
	    AknIconUtils::CreateIconL(bm, mask, AknIconUtils::AvkonIconFileName(), EMbmAvkonQgn_prop_phone_memc_large, EMbmAvkonQgn_prop_phone_memc_mask);
    
	}
	icons->AppendL(CGulIcon::NewL(bm, mask));	
    }

    CTextListBoxModel* model = list->Model();
    model->SetItemTextArray(items);
    model->SetOwnershipType(ELbmOwnsItemArray);
    CleanupStack::Pop(items);

    list->ItemDrawer()->FormattedCellData()->SetIconArrayL(icons);
    CleanupStack::Pop(icons);
    
    TInt popupOk = popupList->ExecuteLD();
    CleanupStack::Pop(popupList);

    if (popupOk)
    {

    	TInt index = list->CurrentItemIndex();
	aPath.Copy(driveList->MdcaPoint(index));
	aPath.Append(_L(":\\"));
	
    }

    CleanupStack::PopAndDestroy(list); // list
    delete driveList;
    //CleanupStack::PopAndDestroy(driveList); 
    return popupOk;   
}



TBool CSavePathSettingItem::SelectPathL(TDes& aPath)
{

    if (!SelectDrivePathL(aPath)) 
    {
	return EFalse;
    }

    CAknFileSelectionDialog* d = CAknFileSelectionDialog::NewL(ECFDDialogTypeSave, R_SAVEPATHSELECTION_DIALOG);
    CleanupStack::PushL(d);

    TBool ok = d->ExecuteL(aPath);
    CleanupStack::PopAndDestroy(d);
    return ok;
}



void CSavePathSettingItem::EditItemL(TBool aCalledFromMenu)
{
    
    TFileName path;   
    if (SelectPathL(path))
    {

	iOldPath.Copy(InternalTextPtr());
	InternalTextPtr().Copy(path);
	CAknTextSettingItem::EditItemL(aCalledFromMenu);
    }

}



// ================= MEMBER FUNCTIONS =======================

// ---------------------------------------------------------
// CScreenGrabberSettingListContainer::ConstructL(const TRect& aRect)
// EPOC two phased constructor
// ---------------------------------------------------------
//
void CScreenGrabberSettingListContainer::ConstructL(const TRect& aRect)
    {
    CreateWindowL();
    
    // set title of the app
    CEikStatusPane* statusPane = iEikonEnv->AppUiFactory()->StatusPane();
    CAknTitlePane* title = static_cast<CAknTitlePane*>( statusPane->ControlL( TUid::Uid( EEikStatusPaneUidTitle ) ) );

    HBufC* titleText = iCoeEnv->AllocReadResourceLC(R_SETTINGS_TITLE);

    title->SetTextL(*titleText);
    CleanupStack::PopAndDestroy(titleText);


    // get an instance of the engine
    iModel = static_cast<CScreenGrabberDocument*>(reinterpret_cast<CEikAppUi*>(iEikonEnv->AppUi())->Document())->Model();

    // get the settings from the engine
    iGrabSettings = iModel->GrabSettings();

    // construct the settings list from resources
    ConstructFromResourceL(R_SCREENGRABBER_SETTINGLIST);
    
    // set visibilities
    SetVisibilitiesOfSettingItems();

    // set correct drawing area for the listbox
    SetRect(aRect);
    HandleResourceChange(KEikDynamicLayoutVariantSwitch);

    ActivateL();
    }


// Destructor
CScreenGrabberSettingListContainer::~CScreenGrabberSettingListContainer()
    {
    }


// ---------------------------------------------------------
// CScreenGrabberSettingListView::CreateSettingItemL( TInt aIdentifier )
// ---------------------------------------------------------
//
CAknSettingItem* CScreenGrabberSettingListContainer::CreateSettingItemL( TInt aIdentifier )
    {
    CAknSettingItem* settingItem = NULL;

    switch (aIdentifier)
        {
        case ESettingListCaptureModeSelection:
            settingItem = new(ELeave) CAknEnumeratedTextPopupSettingItem(aIdentifier, iGrabSettings.iCaptureMode);
            break;
      
        case ESettingListScreenDeviceSelection:
            settingItem = new(ELeave) CAknEnumeratedTextPopupSettingItem(aIdentifier, iGrabSettings.iScreenDevice);
            break;

        case ESettingListSingleCaptureHotkeySelection:
            //settingItem = new(ELeave) CAknEnumeratedTextPopupSettingItem(aIdentifier, iGrabSettings.iSingleCaptureHotkey);
            settingItem = new (ELeave) CHotkeySettingItem(aIdentifier, iGrabSettings.iSingleCaptureHotkey, iGrabSettings, iModel);	    
	    break;
 
   
        case ESettingListSingleCaptureImageFormatSelection:
            settingItem = new(ELeave) CAknEnumeratedTextPopupSettingItem(aIdentifier, iGrabSettings.iSingleCaptureImageFormat);
            break;
        
        case ESettingListSingleCaptureFileNameSelection:
            settingItem = new(ELeave) CAknTextSettingItem(aIdentifier, iGrabSettings.iSingleCaptureFileName);
            break;


        case ESettingListSequantialCaptureHotkeySelection:
            //settingItem = new(ELeave) CAknEnumeratedTextPopupSettingItem(aIdentifier, iGrabSettings.iSequantialCaptureHotkey);
	    settingItem = new(ELeave) CHotkeySettingItem(aIdentifier, iGrabSettings.iSequantialCaptureHotkey, iGrabSettings, iModel);    
            break;
        
        case ESettingListSequantialCaptureImageFormatSelection:
            settingItem = new(ELeave) CAknEnumeratedTextPopupSettingItem(aIdentifier, iGrabSettings.iSequantialCaptureImageFormat);
            break;

        case ESettingListSequantialCaptureDelaySelection:
            settingItem = new(ELeave) CAknIntegerEdwinSettingItem(aIdentifier, iGrabSettings.iSequantialCaptureDelay);
            break;
        case ESettingListSequantialCaptureFileNameSelection:
            settingItem = new(ELeave) CAknTextSettingItem(aIdentifier, iGrabSettings.iSequantialCaptureFileName);
            break;


        case ESettingListVideoCaptureHotkeySelection:
            //settingItem = new(ELeave) CAknEnumeratedTextPopupSettingItem(aIdentifier, iGrabSettings.iVideoCaptureHotkey);
	    settingItem = new(ELeave) CHotkeySettingItem(aIdentifier, iGrabSettings.iVideoCaptureHotkey, iGrabSettings, iModel);    
            break;
        
        case ESettingListVideoCaptureVideoFormatSelection:
            settingItem = new(ELeave) CAknEnumeratedTextPopupSettingItem(aIdentifier, iGrabSettings.iVideoCaptureVideoFormat);
            break;
        
        case ESettingListVideoCaptureFileNameSelection:
            settingItem = new(ELeave) CAknTextSettingItem(aIdentifier, iGrabSettings.iVideoCaptureFileName);
            break;

	case ESettingListSavePathSelection:
	    settingItem = new (ELeave) CSavePathSettingItem(aIdentifier, iGrabSettings.iSavePath);
            break;
 
        }

    return settingItem;
    }


// ---------------------------------------------------------
// CScreenGrabberSettingListView::UpdateSettings()
// ---------------------------------------------------------
//
void CScreenGrabberSettingListContainer::UpdateSettingsL()
    {
    // get the modified settings to our own variables
    StoreSettingsL();  // from CAknSettingItemList

    // store new settings in engine and save the settings in disk
    TRAP_IGNORE(iModel->SaveSettingsL(iGrabSettings));
    
    // Change the keys used for capturing
    iModel->ActivateCaptureKeysL(ETrue);
    }


// ---------------------------------------------------------
// CScreenGrabberSettingListView::HandleListBoxEventL(CEikListBox* aListBox, TListBoxEvent aEventType)
// ---------------------------------------------------------
//
void CScreenGrabberSettingListContainer::HandleListBoxEventL(CEikListBox* /*aListBox*/, TListBoxEvent aEventType)
    {
    switch (aEventType) 
        {
        case EEventEnterKeyPressed:
        case EEventItemDoubleClicked:
            ShowSettingPageL(EFalse);
            break;
        default:
            break;
        }
    }

// ---------------------------------------------------------
// CScreenGrabberSettingListView::ShowSettingPageL(TInt aCalledFromMenu)
// ---------------------------------------------------------
//
void CScreenGrabberSettingListContainer::ShowSettingPageL(TInt aCalledFromMenu) 
    {
    // open the setting page
    TInt listIndex = ListBox()->CurrentItemIndex();
    TInt realIndex = SettingItemArray()->ItemIndexFromVisibleIndex(listIndex);
    EditItemL(realIndex, aCalledFromMenu);

    // update and save the settings
    UpdateSettingsL();
    
    // set visibilities
    SetVisibilitiesOfSettingItems();

    // refresh the screen 
    DrawNow();
    }

// ---------------------------------------------------------
// CScreenGrabberSettingListContainer::SetVisibilitiesOfSettingItems()
// ---------------------------------------------------------
//
void CScreenGrabberSettingListContainer::SetVisibilitiesOfSettingItems() 
{
    ((*SettingItemArray())[ESettingListScreenDeviceSelection])->SetHidden(!iModel->SecondScreenAvailable());

    switch (iGrabSettings.iCaptureMode)
        {
        case ECaptureModeSingleCapture:
            {
            ((*SettingItemArray())[ESettingListSingleCaptureHotkeySelection])->SetHidden(EFalse);
            ((*SettingItemArray())[ESettingListSingleCaptureImageFormatSelection])->SetHidden(EFalse);
           
	    ((*SettingItemArray())[ESettingListSingleCaptureFileNameSelection])->SetHidden(EFalse);
            ((*SettingItemArray())[ESettingListSequantialCaptureHotkeySelection])->SetHidden(ETrue);
            ((*SettingItemArray())[ESettingListSequantialCaptureImageFormatSelection])->SetHidden(ETrue);
            ((*SettingItemArray())[ESettingListSequantialCaptureDelaySelection])->SetHidden(ETrue);
            ((*SettingItemArray())[ESettingListSequantialCaptureFileNameSelection])->SetHidden(ETrue);
            ((*SettingItemArray())[ESettingListVideoCaptureHotkeySelection])->SetHidden(ETrue);
            ((*SettingItemArray())[ESettingListVideoCaptureVideoFormatSelection])->SetHidden(ETrue);
           
	    ((*SettingItemArray())[ESettingListVideoCaptureFileNameSelection])->SetHidden(ETrue);
            }
            break;
        
        case ECaptureModeSequantialCapture:
            {
            ((*SettingItemArray())[ESettingListSingleCaptureHotkeySelection])->SetHidden(ETrue);
            ((*SettingItemArray())[ESettingListSingleCaptureImageFormatSelection])->SetHidden(ETrue);

            ((*SettingItemArray())[ESettingListSingleCaptureFileNameSelection])->SetHidden(ETrue);
            ((*SettingItemArray())[ESettingListSequantialCaptureHotkeySelection])->SetHidden(EFalse);
            ((*SettingItemArray())[ESettingListSequantialCaptureImageFormatSelection])->SetHidden(EFalse);
            ((*SettingItemArray())[ESettingListSequantialCaptureDelaySelection])->SetHidden(EFalse);            ((*SettingItemArray())[ESettingListSequantialCaptureFileNameSelection])->SetHidden(EFalse);
            ((*SettingItemArray())[ESettingListVideoCaptureHotkeySelection])->SetHidden(ETrue);
            ((*SettingItemArray())[ESettingListVideoCaptureVideoFormatSelection])->SetHidden(ETrue);
           
	    ((*SettingItemArray())[ESettingListVideoCaptureFileNameSelection])->SetHidden(ETrue);
            }
            break;            

        case ECaptureModeVideoCapture:
            {
            ((*SettingItemArray())[ESettingListSingleCaptureHotkeySelection])->SetHidden(ETrue);
            ((*SettingItemArray())[ESettingListSingleCaptureImageFormatSelection])->SetHidden(ETrue);
                       
	    ((*SettingItemArray())[ESettingListSingleCaptureFileNameSelection])->SetHidden(ETrue);
            ((*SettingItemArray())[ESettingListSequantialCaptureHotkeySelection])->SetHidden(ETrue);
            ((*SettingItemArray())[ESettingListSequantialCaptureImageFormatSelection])->SetHidden(ETrue);
            ((*SettingItemArray())[ESettingListSequantialCaptureDelaySelection])->SetHidden(ETrue);
            ((*SettingItemArray())[ESettingListSequantialCaptureFileNameSelection])->SetHidden(ETrue);
            ((*SettingItemArray())[ESettingListVideoCaptureHotkeySelection])->SetHidden(EFalse);
            ((*SettingItemArray())[ESettingListVideoCaptureVideoFormatSelection])->SetHidden(EFalse);

            ((*SettingItemArray())[ESettingListVideoCaptureFileNameSelection])->SetHidden(EFalse);
            }
            break;             

        default:
            User::Panic(_L("Inv.capt.mode"), 50);
            break;
        }
        
    HandleChangeInItemArrayOrVisibilityL(); 
    }

// ---------------------------------------------------------
// CScreenGrabberSettingListView::HandleResourceChange(TInt aType)
// ---------------------------------------------------------
//
void CScreenGrabberSettingListContainer::HandleResourceChange(TInt aType)
    {
    if ( aType == KEikDynamicLayoutVariantSwitch )
        {
        TRect mainPaneRect;
        AknLayoutUtils::LayoutMetricsRect(AknLayoutUtils::EMainPane, mainPaneRect);
        SetRect(mainPaneRect);

        TSize outputRectSize;
        AknLayoutUtils::LayoutMetricsSize(AknLayoutUtils::EMainPane, outputRectSize);
        TRect outputRect(outputRectSize);
        ListBox()->SetRect(outputRect);
        }
    else
        {
        CCoeControl::HandleResourceChange(aType);
        }
    }

// End of File  
