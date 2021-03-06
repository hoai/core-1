/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#include "dlgunit.hxx"
#include <vcl/fixed.hxx>
#include <vcl/graph.hxx>
#include <vcl/graphicfilter.hxx>
#include <vcl/lstbox.hxx>
#include <vcl/virdev.hxx>
#include <vcl/svapp.hxx>
#include <vcl/settings.hxx>
#include <vcl/slider.hxx>
#include <svx/strings.hrc>
#include <svx/svdograf.hxx>
#include <svx/sdgcpitm.hxx>
#include <svx/dialmgr.hxx>
#include <svx/compressgraphicdialog.hxx>
#include <sfx2/dispatch.hxx>
#include <comphelper/fileformat.h>
#include <comphelper/processfactory.hxx>
#include <com/sun/star/uno/Sequence.hxx>
#include <com/sun/star/beans/XPropertySet.hpp>

using namespace com::sun::star::uno;
using namespace com::sun::star::beans;

CompressGraphicsDialog::CompressGraphicsDialog( vcl::Window* pParent, SdrGrafObj* pGraphicObj, SfxBindings& rBindings ) :
    ModalDialog       ( pParent, "CompressGraphicDialog", "svx/ui/compressgraphicdialog.ui" ),
    m_pGraphicObj     ( pGraphicObj ),
    m_aGraphic        ( pGraphicObj->GetGraphicObject().GetGraphic() ),
    m_aViewSize100mm  ( pGraphicObj->GetLogicRect().GetSize() ),
    m_rBindings       ( rBindings ),
    m_dResolution     ( 96.0 )
{
    const SdrGrafCropItem& rCrop = m_pGraphicObj->GetMergedItem(SDRATTR_GRAFCROP);
    m_aCropRectangle = tools::Rectangle(rCrop.GetLeft(), rCrop.GetTop(), rCrop.GetRight(), rCrop.GetBottom());

    Initialize();
}

CompressGraphicsDialog::CompressGraphicsDialog( vcl::Window* pParent, Graphic const & rGraphic, Size rViewSize100mm, tools::Rectangle const & rCropRectangle, SfxBindings& rBindings ) :
    ModalDialog       ( pParent, "CompressGraphicDialog", "svx/ui/compressgraphicdialog.ui" ),
    m_pGraphicObj     ( nullptr ),
    m_aGraphic        ( rGraphic ),
    m_aViewSize100mm  ( rViewSize100mm ),
    m_aCropRectangle  ( rCropRectangle ),
    m_rBindings       ( rBindings ),
    m_dResolution     ( 96.0 )
{
    Initialize();
}

CompressGraphicsDialog::~CompressGraphicsDialog()
{
    disposeOnce();
}

void CompressGraphicsDialog::dispose()
{
    m_pLabelGraphicType.clear();
    m_pFixedText2.clear();
    m_pFixedText3.clear();
    m_pFixedText5.clear();
    m_pFixedText6.clear();
    m_pReduceResolutionCB.clear();
    m_pMFNewWidth.clear();
    m_pMFNewHeight.clear();
    m_pResolutionLB.clear();
    m_pLosslessRB.clear();
    m_pJpegCompRB.clear();
    m_pCompressionMF.clear();
    m_pQualityMF.clear();
    m_pBtnCalculate.clear();
    m_pInterpolationCombo.clear();
    m_pCompressionSlider.clear();
    m_pQualitySlider.clear();
    ModalDialog::dispose();
}

void CompressGraphicsDialog::Initialize()
{
    get(m_pLabelGraphicType,    "label-graphic-type");
    get(m_pFixedText2,          "label-original-size");
    get(m_pFixedText3,          "label-view-size");
    get(m_pFixedText5,          "label-image-capacity");
    get(m_pFixedText6,          "label-new-capacity");
    get(m_pJpegCompRB,          "radio-jpeg");
    get(m_pCompressionMF,       "spin-compression");
    get(m_pCompressionSlider,   "scale-compression");
    get(m_pLosslessRB,          "radio-lossless");
    get(m_pQualityMF,           "spin-quality");
    get(m_pQualitySlider,       "scale-quality");
    get(m_pReduceResolutionCB,  "checkbox-reduce-resolution");
    get(m_pMFNewWidth,          "spin-new-width");
    get(m_pMFNewHeight,         "spin-new-height");
    get(m_pResolutionLB,        "combo-resolution");
    get(m_pBtnCalculate,        "calculate");
    get(m_pInterpolationCombo,  "interpolation-method-combo");

    m_pInterpolationCombo->SelectEntry( "Lanczos" );

    m_pInterpolationCombo->SetSelectHdl( LINK( this, CompressGraphicsDialog, NewInterpolationModifiedHdl ));

    m_pMFNewWidth->SetModifyHdl( LINK( this, CompressGraphicsDialog, NewWidthModifiedHdl ));
    m_pMFNewHeight->SetModifyHdl( LINK( this, CompressGraphicsDialog, NewHeightModifiedHdl ));

    m_pResolutionLB->SetModifyHdl( LINK( this, CompressGraphicsDialog, ResolutionModifiedHdl ));
    m_pBtnCalculate->SetClickHdl(  LINK( this, CompressGraphicsDialog, CalculateClickHdl ) );

    m_pLosslessRB->SetToggleHdl( LINK( this, CompressGraphicsDialog, ToggleCompressionRB ) );
    m_pJpegCompRB->SetToggleHdl( LINK( this, CompressGraphicsDialog, ToggleCompressionRB ) );

    m_pReduceResolutionCB->SetToggleHdl( LINK( this, CompressGraphicsDialog, ToggleReduceResolutionRB ) );

    m_pQualitySlider->SetLinkedField(m_pQualityMF);
    m_pQualitySlider->SetEndSlideHdl( LINK( this, CompressGraphicsDialog, EndSlideHdl ));
    m_pCompressionSlider->SetLinkedField(m_pCompressionMF);
    m_pCompressionSlider->SetEndSlideHdl( LINK( this, CompressGraphicsDialog, EndSlideHdl ));
    m_pQualityMF->SetModifyHdl( LINK( this, CompressGraphicsDialog, NewQualityModifiedHdl ));
    m_pCompressionMF->SetModifyHdl( LINK( this, CompressGraphicsDialog, NewCompressionModifiedHdl ));

    m_pJpegCompRB->Check();
    m_pReduceResolutionCB->Check();

    UpdateNewWidthMF();
    UpdateNewHeightMF();
    UpdateResolutionLB();
    Update();
}

void CompressGraphicsDialog::Update()
{
    GfxLinkType aLinkType = m_aGraphic.GetLink().GetType();
    OUString aGraphicTypeString;
    switch(aLinkType)
    {
        case GfxLinkType::NativeGif:
            aGraphicTypeString = SvxResId(STR_IMAGE_GIF);
            break;
        case GfxLinkType::NativeJpg:
            aGraphicTypeString = SvxResId(STR_IMAGE_JPEG);
            break;
        case GfxLinkType::NativePng:
            aGraphicTypeString = SvxResId(STR_IMAGE_PNG);
            break;
        case GfxLinkType::NativeTif:
            aGraphicTypeString = SvxResId(STR_IMAGE_TIFF);
            break;
        case GfxLinkType::NativeWmf:
            aGraphicTypeString = SvxResId(STR_IMAGE_WMF);
            break;
        case GfxLinkType::NativeMet:
            aGraphicTypeString = SvxResId(STR_IMAGE_MET);
            break;
        case GfxLinkType::NativePct:
            aGraphicTypeString = SvxResId(STR_IMAGE_PCT);
            break;
        case GfxLinkType::NativeSvg:
            aGraphicTypeString = SvxResId(STR_IMAGE_SVG);
            break;
        case GfxLinkType::NativeBmp:
            aGraphicTypeString = SvxResId(STR_IMAGE_BMP);
            break;
        default:
            aGraphicTypeString = SvxResId(STR_IMAGE_UNKNOWN);
            break;
    }
    m_pLabelGraphicType->SetText(aGraphicTypeString);

    const FieldUnit eFieldUnit = m_rBindings.GetDispatcher()->GetModule()->GetFieldUnit();
    const LocaleDataWrapper& rLocaleWrapper( Application::GetSettings().GetLocaleDataWrapper() );
    sal_Unicode cSeparator = rLocaleWrapper.getNumDecimalSep()[0];

    ScopedVclPtrInstance<VirtualDevice> pDummyVDev;
    pDummyVDev->EnableOutput( false );
    pDummyVDev->SetMapMode( m_aGraphic.GetPrefMapMode() );

    Size aPixelSize = m_aGraphic.GetSizePixel();
    Size aOriginalSize100mm(pDummyVDev->PixelToLogic(m_aGraphic.GetSizePixel(), MapMode(MapUnit::Map100thMM)));

    OUString aBitmapSizeString = SvxResId(STR_IMAGE_ORIGINAL_SIZE);
    OUString aWidthString  = GetUnitString( aOriginalSize100mm.Width(),  eFieldUnit, cSeparator );
    OUString aHeightString = GetUnitString( aOriginalSize100mm.Height(), eFieldUnit, cSeparator );
    aBitmapSizeString = aBitmapSizeString.replaceAll("$(WIDTH)",  aWidthString);
    aBitmapSizeString = aBitmapSizeString.replaceAll("$(HEIGHT)", aHeightString);
    aBitmapSizeString = aBitmapSizeString.replaceAll("$(WIDTH_IN_PX)",  OUString::number(aPixelSize.Width()));
    aBitmapSizeString = aBitmapSizeString.replaceAll("$(HEIGHT_IN_PX)", OUString::number(aPixelSize.Height()));
    m_pFixedText2->SetText(aBitmapSizeString);

    int aValX = static_cast<int>(aPixelSize.Width() / GetViewWidthInch());

    OUString aViewSizeString = SvxResId(STR_IMAGE_VIEW_SIZE);

    aWidthString  = GetUnitString( m_aViewSize100mm.Width(),  eFieldUnit, cSeparator );
    aHeightString = GetUnitString( m_aViewSize100mm.Height(), eFieldUnit, cSeparator );
    aViewSizeString = aViewSizeString.replaceAll("$(WIDTH)",  aWidthString);
    aViewSizeString = aViewSizeString.replaceAll("$(HEIGHT)", aHeightString);
    aViewSizeString = aViewSizeString.replaceAll("$(DPI)", OUString::number(aValX));
    m_pFixedText3->SetText(aViewSizeString);

    SvMemoryStream aMemStream;
    aMemStream.SetVersion( SOFFICE_FILEFORMAT_CURRENT );
    m_aGraphic.ExportNative(aMemStream);
    aMemStream.Seek( STREAM_SEEK_TO_END );
    sal_Int32 aNativeSize = aMemStream.Tell();

    OUString aNativeSizeString = SvxResId(STR_IMAGE_CAPACITY);
    aNativeSizeString = aNativeSizeString.replaceAll("$(CAPACITY)",  OUString::number(aNativeSize / 1024));
    m_pFixedText5->SetText(aNativeSizeString);

    m_pFixedText6->SetText("??");
}

void CompressGraphicsDialog::UpdateNewWidthMF()
{
    int nPixelX = static_cast<sal_Int32>( GetViewWidthInch() * m_dResolution );
    m_pMFNewWidth->SetText( OUString::number( nPixelX ));
}

void CompressGraphicsDialog::UpdateNewHeightMF()
{
    int nPixelY = static_cast<sal_Int32>( GetViewHeightInch() * m_dResolution );
    m_pMFNewHeight->SetText( OUString::number( nPixelY ));
}

void CompressGraphicsDialog::UpdateResolutionLB()
{
    m_pResolutionLB->SetText( OUString::number( static_cast<sal_Int32>(m_dResolution) ) );
}

double CompressGraphicsDialog::GetViewWidthInch()
{
    return static_cast<double>(MetricField::ConvertValue(m_aViewSize100mm.Width(),  2, MapUnit::Map100thMM, FUNIT_INCH)) / 100.0;
}

double CompressGraphicsDialog::GetViewHeightInch()
{
    return static_cast<double>(MetricField::ConvertValue(m_aViewSize100mm.Height(),  2, MapUnit::Map100thMM, FUNIT_INCH)) / 100.0;
}

BmpScaleFlag CompressGraphicsDialog::GetSelectedInterpolationType()
{
    OUString aSelectionText = m_pInterpolationCombo->GetSelectedEntry();

    if( aSelectionText == "Lanczos" ) {
        return BmpScaleFlag::Lanczos;
    } else if( aSelectionText == "Bilinear" ) {
        return BmpScaleFlag::BiLinear;
    } else if( aSelectionText == "Bicubic" ) {
        return BmpScaleFlag::BiCubic;
    } else if ( aSelectionText == "None" ) {
        return BmpScaleFlag::Fast;
    }
    return BmpScaleFlag::BestQuality;
}

void CompressGraphicsDialog::Compress(SvStream& aStream)
{
    BitmapEx aBitmap = m_aGraphic.GetBitmapEx();
    if ( m_pReduceResolutionCB->IsChecked() )
    {
        long nPixelX = static_cast<long>( GetViewWidthInch() * m_dResolution );
        long nPixelY = static_cast<long>( GetViewHeightInch() * m_dResolution );

        aBitmap.Scale( Size( nPixelX, nPixelY ), GetSelectedInterpolationType() );
    }
    Graphic aScaledGraphic( aBitmap );
    GraphicFilter& rFilter = GraphicFilter::GetGraphicFilter();

    Sequence< PropertyValue > aFilterData( 3 );
    aFilterData[ 0 ].Name = "Interlaced";
    aFilterData[ 0 ].Value <<= sal_Int32(0);
    aFilterData[ 1 ].Name = "Compression";
    aFilterData[ 1 ].Value <<= static_cast<sal_Int32>(m_pCompressionMF->GetValue());
    aFilterData[ 2 ].Name = "Quality";
    aFilterData[ 2 ].Value <<= static_cast<sal_Int32>(m_pQualityMF->GetValue());

    OUString aGraphicFormatName = m_pLosslessRB->IsChecked() ? OUString( "png" ) : OUString( "jpg" );

    sal_uInt16 nFilterFormat = rFilter.GetExportFormatNumberForShortName( aGraphicFormatName );
    rFilter.ExportGraphic( aScaledGraphic, "none", aStream, nFilterFormat, &aFilterData );
}

IMPL_LINK_NOARG( CompressGraphicsDialog, NewWidthModifiedHdl, Edit&, void )
{
    fprintf(stderr, "NewWidthModifiedHdl\n");

    m_dResolution =  m_pMFNewWidth->GetValue() / GetViewWidthInch();

    UpdateNewHeightMF();
    UpdateResolutionLB();
    Update();
}

IMPL_LINK_NOARG( CompressGraphicsDialog, EndSlideHdl, Slider*, void )
{
    Update();
}

IMPL_LINK_NOARG( CompressGraphicsDialog, NewInterpolationModifiedHdl, ListBox&, void )
{
    Update();
}

IMPL_LINK_NOARG( CompressGraphicsDialog, NewQualityModifiedHdl, Edit&, void )
{
    m_pQualitySlider->SetThumbPos(m_pQualityMF->GetValue());
    Update();
}

IMPL_LINK_NOARG( CompressGraphicsDialog, NewCompressionModifiedHdl, Edit&, void )
{
    m_pCompressionSlider->SetThumbPos(m_pCompressionMF->GetValue());
    Update();
}

IMPL_LINK_NOARG( CompressGraphicsDialog, NewHeightModifiedHdl, Edit&, void )
{
    m_dResolution =  m_pMFNewHeight->GetValue() / GetViewHeightInch();

    UpdateNewWidthMF();
    UpdateResolutionLB();
    Update();
}

IMPL_LINK_NOARG( CompressGraphicsDialog, ResolutionModifiedHdl, Edit&, void )
{
    m_dResolution = static_cast<double>(m_pResolutionLB->GetText().toInt32());

    UpdateNewWidthMF();
    UpdateNewHeightMF();
    Update();
}

IMPL_LINK_NOARG( CompressGraphicsDialog, ToggleCompressionRB, RadioButton&, void )
{
    bool choice = m_pLosslessRB->IsChecked();
    m_pCompressionMF->Enable(choice);
    m_pCompressionSlider->Enable(choice);
    m_pQualityMF->Enable(!choice);
    m_pQualitySlider->Enable(!choice);
    Update();
}

IMPL_LINK_NOARG( CompressGraphicsDialog, ToggleReduceResolutionRB, CheckBox&, void )
{
    bool choice = m_pReduceResolutionCB->IsChecked();
    m_pMFNewWidth->Enable(choice);
    m_pMFNewHeight->Enable(choice);
    m_pResolutionLB->Enable(choice);
    m_pInterpolationCombo->Enable(choice);
    Update();
}

IMPL_LINK_NOARG( CompressGraphicsDialog, CalculateClickHdl, Button*, void )
{
    sal_Int32 aSize = 0;

    if ( m_dResolution > 0.0  )
    {
        SvMemoryStream aMemStream;
        aMemStream.SetVersion( SOFFICE_FILEFORMAT_CURRENT );
        Compress( aMemStream );
        aMemStream.Seek( STREAM_SEEK_TO_END );
        aSize = aMemStream.Tell();
    }

    if ( aSize > 0 )
    {
        OUString aSizeAsString = OUString::number(aSize / 1024);

        OUString aNewSizeString = SvxResId(STR_IMAGE_CAPACITY);
        aNewSizeString = aNewSizeString.replaceAll("$(CAPACITY)", aSizeAsString);
        m_pFixedText6->SetText(aNewSizeString);
    }
}

tools::Rectangle CompressGraphicsDialog::GetScaledCropRectangle()
{
    if ( m_pReduceResolutionCB->IsChecked() )
    {
        long nPixelX = static_cast<long>( GetViewWidthInch()  * m_dResolution );
        long nPixelY = static_cast<long>( GetViewHeightInch() * m_dResolution );
        Size aSize = m_aGraphic.GetBitmapEx().GetSizePixel();
        double aScaleX = nPixelX / static_cast<double>(aSize.Width());
        double aScaleY = nPixelY / static_cast<double>(aSize.Height());

        return tools::Rectangle(
            m_aCropRectangle.Left()  * aScaleX,
            m_aCropRectangle.Top()   * aScaleY,
            m_aCropRectangle.Right() * aScaleX,
            m_aCropRectangle.Bottom()* aScaleY);
    }
    else
    {
        return m_aCropRectangle;
    }
}

Graphic CompressGraphicsDialog::GetCompressedGraphic()
{
    if ( m_dResolution > 0.0  )
    {
        SvMemoryStream aMemStream;
        aMemStream.SetVersion( SOFFICE_FILEFORMAT_CURRENT );
        Compress( aMemStream );
        aMemStream.Seek( STREAM_SEEK_TO_BEGIN );
        Graphic aResultGraphic;
        GraphicFilter& rFilter = GraphicFilter::GetGraphicFilter();
        rFilter.ImportGraphic( aResultGraphic, OUString("import"), aMemStream );

        return aResultGraphic;
    }
    return Graphic();
}

SdrGrafObj* CompressGraphicsDialog::GetCompressedSdrGrafObj()
{
    if ( m_dResolution > 0.0  )
    {
        SdrGrafObj* pNewObject = m_pGraphicObj->Clone();

        if ( m_pReduceResolutionCB->IsChecked() )
        {
            tools::Rectangle aScaledCropedRectangle = GetScaledCropRectangle();
            SdrGrafCropItem aNewCrop(
                aScaledCropedRectangle.Left(),
                aScaledCropedRectangle.Top(),
                aScaledCropedRectangle.Right(),
                aScaledCropedRectangle.Bottom());

            pNewObject->SetMergedItem(aNewCrop);
        }
        pNewObject->SetGraphic( GetCompressedGraphic() );

        return pNewObject;
    }
    return nullptr;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
