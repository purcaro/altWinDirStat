// treemap.cpp	- Implementation of CColorSpace, CTreemap and CTreemapPreview
//
// WinDirStat - Directory Statistics
// Copyright (C) 2003-2004 Bernhard Seifert
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Author: bseifert@users.sourceforge.net, bseifert@daccord.net
//
// Last modified: $Date$

#include "stdafx.h"
#include "treemap.h"
//#include <afxwin.h>
//#include <stdio.h>
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// I define the "brightness" of an rgb value as (r+b+g)/3/255.
// The EqualizeColors() method creates a palette with colors all having the same brightness of 0.6
// Later in RenderCushion() this number is used again to scale the colors.

static const double PALETTE_BRIGHTNESS = 0.6;

bool CTreemap::m_IsSystem256Colors = false;

/////////////////////////////////////////////////////////////////////////////

_Ret_range_( 0, 1 ) double CColorSpace::GetColorBrightness( _In_ const COLORREF color ) {
	return ( GetRValue( color ) + GetGValue( color ) + GetBValue( color ) ) / 255.0 / 3.0;
	}

COLORREF CColorSpace::MakeBrightColor( _In_ const COLORREF color, _In_ _In_range_( 0, 1 ) const DOUBLE brightness ) {
	ASSERT( brightness >= 0.0 );
	ASSERT( brightness <= 1.0 );

	DOUBLE dred   = GetRValue( color ) / 255.0;
	DOUBLE dgreen = GetGValue( color ) / 255.0;
	DOUBLE dblue  = GetBValue( color ) / 255.0;

	DOUBLE f = 3.0 * brightness / ( dred + dgreen + dblue );
	dred   *= f;
	dgreen *= f;
	dblue  *= f;

	INT red   = ( INT ) ( dred * 255 );
	INT green = ( INT ) ( dgreen * 255 );
	INT blue  = ( INT ) ( dblue * 255 );
	
	NormalizeColor(red, green, blue);
	ASSERT( RGB( red, green, blue ) != 0 );
	return RGB( red, green, blue );
	}

bool CColorSpace::Is256Colors( ) {
	/*
	 Returns true, if the System has 256 Colors or less.
	 In this case options.brightness is ignored (and the slider should be disabled).
	*/
	CClientDC dc( CWnd::GetDesktopWindow( ) );
	return ( dc.GetDeviceCaps( NUMCOLORS ) != -1 );
	}

void CColorSpace::NormalizeColor( _Inout_ _Out_range_(0, 255) INT& red, _Inout_ _Out_range_(0, 255) INT& green, _Inout_ _Out_range_(0, 255) INT& blue ) {
	ASSERT( red + green + blue <= 3 * 255 );

	if ( red > 255 ) {
		DistributeFirst( red, green, blue );
		}
	else if ( green > 255 ) {
		DistributeFirst( green, red, blue );
		}
	else if ( blue > 255 ) {
		DistributeFirst( blue, red, green );
		}
	}

void CColorSpace::DistributeFirst( _Inout_ _Out_range_(0, 255) INT& first, _Inout_ _Out_range_(0, 255) INT& second, _Inout_ _Out_range_(0, 255) INT& third ) {
	INT h = ( first - 255 ) / 2;
	first = 255;
	second += h;
	third += h;

	if ( second > 255 ) {
		INT h2 = second - 255;
		second = 255;
		third += h2;
		}
	else if ( third > 255 ) {
		INT h3 = third - 255;
		third = 255;
		second += h3;
		}
	ASSERT( second <= 255 );
	ASSERT( third <= 255 );
	}

/////////////////////////////////////////////////////////////////////////////

const CTreemap::Options CTreemap::_defaultOptions =    { KDirStatStyle, false, RGB( 0, 0, 0 ), 0.88, 0.38, 0.91, 0.13, -1.0, -1.0 };

const CTreemap::Options CTreemap::_defaultOptionsOld = { KDirStatStyle, false, RGB( 0, 0, 0 ), 0.85, 0.40, 0.90, 0.15, -1.0, -1.0 };

const COLORREF CTreemap::_defaultCushionColors[ ] = { RGB( 0, 0, 255 ), RGB( 255, 0, 0 ), RGB( 0, 255, 0 ), RGB( 0, 255, 255 ), RGB( 255, 0, 255 ), RGB( 255, 255, 0 ), RGB( 150, 150, 255 ), RGB( 255, 150, 150 ), RGB( 150, 255, 150 ), RGB( 150, 255, 255 ), RGB( 255, 150, 255 ), RGB( 255, 255, 150 ), RGB( 255, 255, 255 ) };

const COLORREF CTreemap::_defaultCushionColors256[ ] = { RGB( 0, 0, 255 ), RGB( 255, 0, 0 ), RGB( 0, 255, 0 ), RGB( 0, 255, 255 ), RGB( 255, 0, 255 ), RGB( 255, 255, 0 ), RGB( 100, 100, 100 ) };

std::vector<COLORREF> CTreemap::GetDefaultPaletteAsVector( ) {
	std::vector<COLORREF> colorVector;
	colorVector.reserve( defaultColorVec.size() + 1 );
	for ( std::vector<COLORREF>::size_type i = 0; i < defaultColorVec.size(); ++i ) {
		colorVector.emplace_back( CColorSpace::MakeBrightColor( defaultColorVec.at( i ), PALETTE_BRIGHTNESS ) );
		}
	return std::move( colorVector );
	}

//void CTreemap::EqualizeColors( _In_ _In_reads_( count ) const COLORREF* colors, _In_ const INT count, _Inout_ CArray<COLORREF, COLORREF&>& out ) {
//	out.SetSize( count );
//	for ( INT i = 0; i < count; i++ ) {
//		out[ i ] = CColorSpace::MakeBrightColor( colors[ i ], PALETTE_BRIGHTNESS );
//		}
//	}

CTreemap::Options CTreemap::GetDefaultOptions( ) {
	return _defaultOptions;
	}

CTreemap::Options CTreemap::GetOldDefaultOptions( ) {
	return _defaultOptionsOld;
	}

CTreemap::CTreemap( Callback *callback ) {
	m_callback = callback;
	m_IsSystem256Colors = CColorSpace::Is256Colors( );
	SetOptions( &_defaultOptions );
	SetBrightnessFor256( );
	IsCushionShading_current = IsCushionShading( );
#ifdef GRAPH_LAYOUT_DEBUG
	//std::vector<std::vector<bool>> x(3000, std::vector<bool>(3000, false))
	bitSetMask = std::make_unique<std::vector<std::vector<bool>>>( 3000, std::vector<bool>( 3000, false ) );//what a mouthful
	numCalls = 0;
#endif
	}

void CTreemap::UpdateCushionShading( _In_ const bool newVal ) { 
	IsCushionShading_current = newVal;
	}

void CTreemap::SetOptions( _In_ const Options* options ) {
	ASSERT( options != NULL );
	m_options = *options;

	// Derive normalized vector here for performance
	const DOUBLE  lx = m_options.lightSourceX;			// negative = left
	const DOUBLE ly = m_options.lightSourceY;			// negative = top
	static const DOUBLE lz = 10;

	const DOUBLE len = sqrt( lx*lx + ly*ly + lz*lz );
	m_Lx = lx / len;
	m_Ly = ly / len;
	m_Lz = lz / len;

	SetBrightnessFor256( );
	}

CTreemap::Options CTreemap::GetOptions( ) {
	return m_options;
	}

void CTreemap::SetBrightnessFor256( ) {
	if ( m_IsSystem256Colors ) {
		m_options.brightness = PALETTE_BRIGHTNESS;
		}
	}

#ifdef _DEBUG
void CTreemap::RecurseCheckTree( _In_ Item *item ) {
 
	item;//do we need???
	if ( item == NULL ) {
		return;
		}

	//TRACE(_T("RecurseCheckTree!\r\n") );
	if ( item->TmiIsLeaf( ) ) {
		//item doesn't have children, nothing to check
		ASSERT( item->TmiGetChildrenCount( ) == 0 );
		}
	else {
		validateRectangle( item, item->TmiGetRectangle( ) );
		for ( auto i = 0; i < item->TmiGetChildrenCount( ); i++ ) {
			//translate into ranged for?
			auto child = item->TmiGetChild( i );
			//validateRectangle( child, item->TmiGetRectangle( ) );
			if ( i > 0 ) {
				auto child_2 = item->TmiGetChild( i - 1 );
				if ( ( child_2 != NULL ) && ( child != NULL ) ) {
					//TRACE( _T( "child 2: %lld, child 1: %lld\r\n" ), child_2->TmiGetSize( ), child->TmiGetSize( ) );
					//ASSERT( ( child_2->TmiGetSize( ) <= child->TmiGetSize( ) ) );
					}
				else {	
					AfxCheckMemory( );
					ASSERT( false );
					}
				}
			
			RecurseCheckTree( child );
			}
		}
	return;
}

#else

void CTreemap::RecurseCheckTree( _In_ Item* item ) {
	( void ) item;
	CString msg = _T( "RecurseCheckTree was called in the release build! This shouldn't happen!" );
	AfxMessageBox( msg );
	//abort( );
	}

#endif

void CTreemap::compensateForGrid( _Inout_ CRect& rc, _In_ CDC* pdc ) {
	if ( m_options.grid ) {
		rc.NormalizeRect( );
		pdc->FillSolidRect( rc, m_options.gridColor );
		}
	else {
		// We shrink the rectangle here, too. If we didn't do this, the layout of the treemap would change, when grid is switched on and off.
		CPen pen { PS_SOLID, 1, GetSysColor( COLOR_3DSHADOW ) };
		CSelectObject sopen { pdc, &pen };
		pdc->MoveTo( rc.right - 1, rc.top );
		pdc->LineTo( rc.right - 1, rc.bottom );
		pdc->MoveTo( rc.left,      rc.bottom - 1 );
		pdc->LineTo( rc.right,     rc.bottom - 1 );
		}
	rc.right--;
	rc.bottom--;

	}

void CTreemap::DrawTreemap( _In_ CDC* pdc, _In_ CRect& rc, _In_ Item* root, _In_opt_ const Options* options ) {
	ASSERT( ( rc.Height( ) + rc.Width( ) ) > 0 );
	if ( root == NULL ) {//should never happen! Ever!
		ASSERT( root != NULL );
		}

	if ( ( rc.Width( ) ) <= 0 || ( rc.Height( ) ) <= 0 ) {
		ASSERT( false );
		return;
		}

	if ( options != NULL ) {
		SetOptions( options );
		}

	compensateForGrid( rc, pdc );

	if ( ( rc.Width( ) ) <= 0 || ( rc.Height( ) ) <= 0 ) {
		ASSERT( false );
		return;
		}
	if ( root->TmiGetSize( ) > 0 ) {//root can be null on zooming out??
		DOUBLE surface[ 4 ] = { 0.00, 0.00, 0.00, 0.00 };
		rc.NormalizeRect( );
		RecurseDrawGraph( pdc, root, rc, true, surface, m_options.height, 0 );
		}
	else {
		rc.NormalizeRect( );
		pdc->FillSolidRect( rc, RGB( 0, 0, 0 ) );
		}
	validateRectangle( root, root->TmiGetRectangle( ) );
	}

void CTreemap::DrawTreemapDoubleBuffered( _In_ CDC *pdc, _In_ const CRect& rc, _In_ Item *root, _In_opt_ const Options *options ) {
	ASSERT_VALID( pdc );
	ASSERT( ( rc.right - rc.left ) == rc.Width( ) );
	ASSERT( ( rc.bottom - rc.top ) == rc.Height( ) );
	ASSERT( ( rc.Height( ) + rc.Width( ) ) > 0 );
	if ( options != NULL ) {
		SetOptions( options );
		}

	if ( ( rc.Width( ) ) <= 0 || ( rc.Height( ) ) <= 0 ) {
		return;
		}

	CDC dc;
	VERIFY( dc.CreateCompatibleDC( pdc ) );

	CBitmap bm;
	VERIFY( bm.CreateCompatibleBitmap( pdc, ( rc.Width( ) ), ( rc.Height( ) ) ) );

	CSelectObject sobmp { &dc, &bm };

	CRect rect{ CPoint( 0, 0 ), rc.Size( ) };

	DrawTreemap( &dc, rect, root, NULL );

	VERIFY( pdc->BitBlt( rc.left, rc.top, ( rc.Width( ) ), ( rc.Height( ) ), &dc, 0, 0, SRCCOPY ) );
	}

void CTreemap::validateRectangle( _In_ const Item* child, _In_ const CRect& rc ) const {
#ifdef _DEBUG
	ASSERT( child->TmiGetSize( ) >= 0 );//really should be greater than ( not greater than or equal ) as we shouldn't be drawing zero-size rectangles!
	auto rcChild = child->TmiGetRectangle( );

	ASSERT(   rc.bottom < 32767 );
	ASSERT(   rc.left   < 32767 );
	ASSERT(   rc.right  < 32767 );
	ASSERT(   rc.top    < 32767 );

	ASSERT( ( ( 0-32768 ) < rc.left   ) );
	ASSERT( ( ( 0-32768 ) < rc.top    ) );
	ASSERT( ( ( 0-32768 ) < rc.right  ) );
	ASSERT( ( ( 0-32768 ) < rc.bottom ) );

	
	ASSERT(   rcChild.right      >=   rcChild.left );
	ASSERT(   rcChild.bottom     >=   rcChild.top );
	ASSERT( ( rcChild.left + 1 ) >=   rc.left );
	ASSERT( ( rc.right + 1 )     >=   rc.left );
	ASSERT(   rc.bottom          >=   rc.top );
	ASSERT(   rcChild.top        >=   rc.top );

	ASSERT(   rcChild.right      <= ( rc.right + 1 ) );
	ASSERT(   rcChild.bottom     <=   rc.bottom );
	
	rcChild.NormalizeRect( );
	ASSERT(   rcChild.Width( )   < 32767 );
	ASSERT(   rcChild.Height( )  < 32767 );
#else
	UNREFERENCED_PARAMETER( child );
	UNREFERENCED_PARAMETER( rc );
#endif
	}

_Success_( return != NULL ) _Must_inspect_result_ CTreemap::Item *CTreemap::FindItemByPoint( _In_ Item* item, _In_ const CPoint point ) {
	/*
	  In the resulting treemap, find the item below a given coordinate. Return value can be NULL - the only case that this function returns NULL is that point is not inside the rectangle of item.

	  `item` (First parameter) MUST NOT BE NULL! I'm serious.

	  Take notice of
	     (a) the very right an bottom lines, which can be "grid" and are not covered by the root rectangle,
	     (b) the fact, that WM_MOUSEMOVEs can occur after WM_SIZE but before WM_PAINT.
	
	*/
	//CRect& rc_a = item->TmiGetRectangle( );
	auto rc = item->TmiGetRectangle( );
	rc.NormalizeRect( );

	if ( !rc.PtInRect( point ) ) {
		return NULL;
		}

	ASSERT( rc.PtInRect( point ) );

	auto gridWidth = m_options.grid ? 1 : 0;
	
	if ( ( ( rc.Width( ) ) <= gridWidth ) || ( ( rc.Height( ) ) <= gridWidth ) || item->TmiIsLeaf( ) ) {
		return item;
		}
	ASSERT( item->TmiGetSize( ) > 0 );
	ASSERT( item->TmiGetChildrenCount( ) > 0 );

	auto countOfChildren = item->TmiGetChildrenCount( );
	for ( INT i = 0; i < countOfChildren; i++ ) {
		auto child = item->TmiGetChild( i );
		if ( child != NULL ) {
			if ( child->TmiGetRectangle( ).PtInRect( point ) ) {
				validateRectangle( child, rc );
				auto ret = FindItemByPoint( child, point );
				if ( ret != NULL ) {
					validateRectangle( ret, rc );
					return ret;
					}
				}
			}
		}
	return item;
	}

void CTreemap::DrawColorPreview( _In_ CDC* pdc, _In_ const CRect& rc, _In_ const COLORREF color, _In_ const Options* options ) {
	ASSERT_VALID( pdc );
	if ( options != NULL ) {
		SetOptions( options );
		}

	DOUBLE surface[ 4 ] = { 0.00, 0.00, 0.00, 0.00 };

	AddRidge( rc, surface, m_options.height * m_options.scaleFactor );

	RenderRectangle( pdc, rc, surface, color );
	if ( m_options.grid ) {
		CPen pen { PS_SOLID, 1, m_options.gridColor };
		CSelectObject sopen{ pdc, &pen };
		CSelectStockObject sobrush { pdc, NULL_BRUSH };
		pdc->Rectangle( rc );
		}
	}

void CTreemap::RecurseDrawGraph( _In_ CDC* pdc, _In_ Item* item, _In_ const CRect& rc, _In_ const bool asroot, _In_ _In_reads_( 4 ) const DOUBLE* psurface, _In_ const DOUBLE height, _In_ const DWORD flags ) {
	ASSERT_VALID( pdc );
	ASSERT( item != NULL );
	if ( item->TmiIsLeaf( ) ) {
		if ( !( item->TmiGetSize( ) > 0 ) ) {
			return;
			}
		}
	else if ( item->TmiGetChildrenCount( ) == 0 ) {
		if ( !( item->TmiGetSize( ) > 0 ) ) {
			return;
			}
		}
#ifdef GRAPH_LAYOUT_DEBUG
	TRACE( _T( " RecurseDrawGraph working on rect left: %li, right: %li, top: %li, bottom: %li, isroot: %i\r\n" ), rc.left, rc.right, rc.top, rc.bottom, ( asroot ? 1 : 0 ) );
#endif

	if ( m_callback != NULL ) {
		m_callback->TreemapDrawingCallback( );
		}
	item->TmiSetRectangle( rc );
	validateRectangle( item, rc );
	auto gridWidth = m_options.grid ? 1 : 0;
	auto childCount = item->TmiGetChildrenCount( );
	//empty directory is a valid possibility!
	if ( ( ( ( rc.Width( ) ) < gridWidth ) || ( ( rc.Height( ) ) < gridWidth ) ) ) {
		return;
		}
	DOUBLE surface[ 4 ] = { 0.00, 0.00, 0.00, 0.00 };

	if ( IsCushionShading_current ) {
		surface[ 0 ] = psurface[ 0 ];
		surface[ 1 ] = psurface[ 1 ];
		surface[ 2 ] = psurface[ 2 ];
		surface[ 3 ] = psurface[ 3 ];
		if ( !asroot ) {
			AddRidge( rc, surface, height );
			validateRectangle( item, rc );
			}
		}
	if ( item->TmiIsLeaf( ) ) {
		RenderLeaf( pdc, item, surface );
		}
	else {
		//empty directory is a valid possibility!
		//ASSERT( item->TmiGetChildrenCount( ) > 0 );
		//ASSERT( item->TmiGetSize( ) > 0 );
		if ( ( !( item->TmiGetChildrenCount( ) > 0 ) ) ||  ( !( item->TmiGetSize( ) > 0 ) ) ) {
			return;
			}
		DrawChildren( pdc, item, surface, height, flags );
		}
	validateRectangle( item, rc );
	}

void CTreemap::DrawChildren( _In_ CDC* pdc, _In_ Item* parent, _In_ _In_reads_( 4 ) const DOUBLE* surface, _In_ const DOUBLE height, _In_ const DWORD flags ) {
	/*
	  My first approach was to make this member pure virtual and have three classes derived from CTreemap. The disadvantage is then, that we cannot simply have a member variable of type CTreemap but have to deal with pointers, factory methods and explicit destruction. It's not worth.
	*/
	ASSERT_VALID( pdc );
	switch ( m_options.style )
	{
		case KDirStatStyle:
			KDirStat_DrawChildren( pdc, parent, surface, height, flags );
			break;

		case SequoiaViewStyle:
			SequoiaView_DrawChildren( pdc, parent, surface, height, flags );
			break;

		case SimpleStyle:
			Simple_DrawChildren( pdc, parent, surface, height, flags );
			break;
	}
	}

CRect CTreemap::KDirStat_buildrcChildVerticalOrHorizontalRow( _In_ const bool horizontalRows, _In_ _In_range_( 0, 32767 ) const LONG left, _In_ _In_range_( 0, 32767 ) const LONG right, _In_ _In_range_( 0, 32767 ) const LONG top, _In_ _In_range_( 0, 32767 ) const LONG bottom ) {
	ASSERT( ( right + 1 ) >= left );
	ASSERT( bottom >= top );
	CRect rcChild;
	if ( horizontalRows ) {
		rcChild.left   = left;
		rcChild.right  = right;
		rcChild.top    = top;
		rcChild.bottom = bottom;
		}
	else {
		rcChild.left   = top;
		rcChild.top    = left;
		rcChild.right  = bottom;
		rcChild.bottom = right;
		}
	ASSERT( ( rcChild.right + 1 ) >= rcChild.left );
	ASSERT( rcChild.bottom >= rcChild.top );
	return std::move( rcChild );
	}

void CTreemap::KDirStat_DrawChildrenInThisRow( _In_ const std::vector<INT_PTR>& childrenPerRow, _Inout_ INT_PTR& c, _In_ const Item* parent, _Inout_ LONG& left, _In_ const INT& width, _In_ const std::vector<DOUBLE>& childWidth, _In_ const bool horizontalRows, _In_ const LONG top, _In_ const LONG bottom, _In_ _In_reads_( 4 ) const DOUBLE* surface, _In_ const DOUBLE h, _In_ CDC* pdc, _In_ const INT row ) {
	for ( INT i = 0; i < childrenPerRow.at( row ); i++, c++ ) {
		auto child = parent->TmiGetChild( c );
		ASSERT( ( c < childWidth.size( ) ) && ( childWidth.at( c ) >= 0 ) );
		auto cChildWidth = childWidth.at( c );
		auto fRight      = left + cChildWidth * width;
		auto right       = std::lround( fRight );
		ASSERT( right >= left );
		//ASSERT( (c+1) < childWidth.GetSize( ) );
		bool lastChild = false;

		if ( ( c + 1 ) < childWidth.size( ) ) {
			lastChild = ( ( i == childrenPerRow.at( row ) - 1 ) || ( childWidth.at( c + 1 ) == 0 ) );
			}
		if ( lastChild ) {
			//right = horizontalRows ? rc.right : rc.bottom;
			right = horizontalRows ? right : bottom;
			ASSERT( ( right + 1 ) >= left );
			if ( !( right >= left ) ) {
				right = left;
				}
			}
		auto rcChild = KDirStat_buildrcChildVerticalOrHorizontalRow( horizontalRows, left, right, top, bottom );
#ifdef GRAPH_LAYOUT_DEBUG
		TRACE( _T( "left: %ld, right: %ld, top: %ld, bottom: %ld\r\n" ), rcChild.left, rcChild.right, rcChild.top, rcChild.bottom );
#endif
		RecurseDrawGraph( pdc, child, rcChild, false, surface, h * m_options.scaleFactor, 0 );
		if ( lastChild ) {
			i++, c++;
			if ( i < childrenPerRow.at( row ) ) {
				auto childToSet = parent->TmiGetChild( c );
				if ( childToSet != NULL ) {
					childToSet->TmiSetRectangle( CRect { -1, -1, -1, -1 } );//WTF
					//RenderRectangle( pdc, childToSet->TmiGetRectangle( ), surface, childToSet->TmiGetGraphColor( ) );
					}
				}
			c += childrenPerRow.at( row ) - i;
			break;
			}
		left = std::lround( fRight );
		}

	}

bool CTreemap::KDirStat_ArrangeChildren( _In_ const Item* parent, _Inout_ CArray<double, double>& childWidth, _Inout_ CArray<double, double>& rows, _Inout_ CArray<int, int>& childrenPerRow ) {
	/*
	  return: whether the rows are horizontal.
	*/
	ASSERT( !parent->TmiIsLeaf( ) );
	ASSERT( parent->TmiGetChildrenCount( ) > 0 );

	if ( parent->TmiGetSize( ) == 0 ) {
		rows.Add( 1.0 );
		childrenPerRow.Add( parent->TmiGetChildrenCount( ) );
		for ( int i = 0; i < parent->TmiGetChildrenCount( ); i++ ) {
			childWidth[ i ] = 1.0 / parent->TmiGetChildrenCount( );
			}
#ifdef DEBUG
	for ( int i = 0; i < childWidth.GetSize( ); ++i ) {
		ASSERT( childWidth[ i ] >= 0 );
		}
#endif
		return true;
		}

#ifdef DEBUG
	for ( int i = 0; i < childWidth.GetSize( ); ++i ) {
		ASSERT( childWidth[ i ] >= 0 );
		}
#endif
	bool horizontalRows = ( parent->TmiGetRectangle( ).Width( ) >= parent->TmiGetRectangle( ).Height( ) );

	double width = 1.0;
	if ( horizontalRows ) {
		if ( parent->TmiGetRectangle( ).Height( ) > 0 ) {
			width = ( double ) parent->TmiGetRectangle( ).Width( ) / parent->TmiGetRectangle( ).Height( );
			}
		}
	else {
		if ( parent->TmiGetRectangle( ).Width( ) > 0 ) {
			width = ( double ) parent->TmiGetRectangle( ).Height( ) / parent->TmiGetRectangle( ).Width( );
			}
		}

	int nextChild = 0;
	while ( nextChild < parent->TmiGetChildrenCount( ) ) {
		int childrenUsed;
		rows.Add( KDirStat_CalcutateNextRow( parent, nextChild, width, childrenUsed, childWidth ) );
		childrenPerRow.Add( childrenUsed );
		nextChild += childrenUsed;
#ifdef DEBUG
	for ( int i = 0; i < childWidth.GetSize( ); ++i ) {
		ASSERT( childWidth[ i ] >= 0 );
		}
#endif
		}

#ifdef DEBUG
	for ( int i = 0; i < childWidth.GetSize( ); ++i ) {
		ASSERT( childWidth[ i ] >= 0 );
		}
#endif

	return horizontalRows;

	}

void CTreemap::KDirStat_DrawChildren( _In_ CDC* pdc, _In_ const Item* parent, _In_ _In_reads_( 4 ) const DOUBLE* surface, _In_ const DOUBLE h, _In_ const DWORD /*flags*/ ) {
	/*
	  I learned this squarification style from the KDirStat executable. It's the most complex one here but also the clearest, imho.
	*/
	ASSERT( parent->TmiGetChildrenCount( ) > 0 );

	const CRect& rc = parent->TmiGetRectangle( );

	CArray<double, double> rows;    // Our rectangle is divided into rows, each of which gets this height (fraction of total height).
	CArray<int, int> childrenPerRow;// childrenPerRow[i] = # of children in rows[i]

	CArray<double, double> childWidth; // Widths of the children (fraction of row width).
	childWidth.SetSize( parent->TmiGetChildrenCount( ) );

	bool horizontalRows = KDirStat_ArrangeChildren( parent, childWidth, rows, childrenPerRow );

	const int width = horizontalRows ? rc.Width( ) : rc.Height( );
	const int height = horizontalRows ? rc.Height( ) : rc.Width( );
	ASSERT( width >= 0 );
	ASSERT( height >= 0 );

	int c = 0;
	auto top = horizontalRows ? rc.top : rc.left;
	for ( int row = 0; row < rows.GetSize( ); row++ ) {
		double fBottom = top + rows[ row ] * height;
		int bottom = ( int ) fBottom;
		if ( row == rows.GetSize( ) - 1 ) {
			bottom = horizontalRows ? rc.bottom : rc.right;
			}
		auto left = horizontalRows ? rc.left : rc.top;
		for ( int i = 0; i < childrenPerRow[ row ]; i++, c++ ) {
			auto child = parent->TmiGetChild( c );
			ASSERT( childWidth[ c ] >= 0 );
			ASSERT( left > -2 );
			double fRight = left + childWidth[ c ] * width;
			int right = std::lround( fRight );

			bool lastChild = ( i == childrenPerRow[ row ] - 1 || childWidth[ c + 1 ] == 0 );

			if ( lastChild ) {
				right = horizontalRows ? rc.right : rc.bottom;
				}
			
			CRect rcChild;
			if ( horizontalRows ) {
				rcChild.left = left;
				rcChild.right = right;
				rcChild.top =  top;
				rcChild.bottom = bottom;
				}
			else {
				rcChild.left = top;
				rcChild.right = bottom;
				rcChild.top = left;
				rcChild.bottom = right;
				}

#ifdef _DEBUG
			if ( rcChild.Width( ) > 0 && rcChild.Height( ) > 0 ) {
				CRect test;
				test.IntersectRect( parent->TmiGetRectangle( ), rcChild );
				ASSERT( test == rcChild );
				}
#endif

				RecurseDrawGraph( pdc, child, rcChild, false, surface, h * m_options.scaleFactor, 0 );

			if ( lastChild ) {
				i++, c++;

				if ( i < childrenPerRow[ row ] ) {
					parent->TmiGetChild( c )->TmiSetRectangle( CRect( -1, -1, -1, -1 ) );
					}

				c += childrenPerRow[ row ] - i;
				break;
				}

			left = fRight;
			}
		// This asserts due to rounding error: ASSERT(left == (horizontalRows ? rc.right : rc.bottom));
		top = fBottom;
		}
	// This asserts due to rounding error: ASSERT(top == (horizontalRows ? rc.bottom : rc.right));
	}

DOUBLE CTreemap::KDirStat_GetWidth( _In_ const Item* parent, _In_ const bool horizontalRows ) {
	/*
	  Returns the Width/Height OR Height/Width ratio of parent, depending on horizontalRows
	*/
	if ( horizontalRows ) {
		if ( parent->TmiGetRectangle( ).Height( ) > 0 ) {
			return DOUBLE( parent->TmiGetRectangle( ).Width( ) ) / DOUBLE( parent->TmiGetRectangle( ).Height( ) );
			}
		}
	else {
		if ( parent->TmiGetRectangle( ).Width( ) > 0 ) {
			return DOUBLE( parent->TmiGetRectangle( ).Height( ) ) / DOUBLE( parent->TmiGetRectangle( ).Width( ) );
			}
		}
	return 0.00;
	}

void CTreemap::KDirStat_IterateOverAllChilrenInParent( _In_ const Item* parent, _In_ _In_range_( 0, INT_MAX ) const INT nextChild, _Inout_ DOUBLE& sizeUsed, _In_ _In_range_( 0, 32767 ) const DOUBLE width, const _In_ DOUBLE mySize, _In_ const DOUBLE _minProportion, _Inout_ DOUBLE& rowHeight, _Inout_ INT& i  ) {
	const auto parent_tmiGetChildCount = parent->TmiGetChildrenCount( );
	for ( i = nextChild; i < parent_tmiGetChildCount; ++i ) { //iterate over all children of parent, starting at nextChild,
		const auto childOfParent = parent->TmiGetChild( i );
		DOUBLE childSize = 0.00;
		if ( childOfParent != NULL ) {
			childSize += DOUBLE( childOfParent->TmiGetSize( ) );
			}
		ASSERT( childOfParent != NULL );
		if ( std::lround( childSize ) == 0 ) {
//			ASSERT( i > nextChild );	// first child has size > 0
			break;
			}
		ASSERT( ( std::lround( childSize ) != 0 ) && ( mySize != 0 ) );
		sizeUsed += childSize;
		auto virtualRowHeight = sizeUsed / mySize;

		if ( virtualRowHeight > 1.00 ) {
			TRACE( _T( "sizeUsed(%f) / mySize(%f) = %f\r\n\tTHAT'S BIGGER THAN 1!\r\n" ), sizeUsed, mySize, virtualRowHeight );
			}
		ASSERT( ( virtualRowHeight > 0 ) && ( virtualRowHeight <= 1.00 ) && ( childSize <= mySize ) && ( mySize != 0 ) );
		const auto aChildWidth = ( ( childSize * width ) / mySize ) / virtualRowHeight;//WTF
		
		if ( aChildWidth / virtualRowHeight < _minProportion ) {
			ASSERT( ( ( aChildWidth / virtualRowHeight ) < _minProportion ) && ( width >= 1.0 ) && ( i >= nextChild ) );// i >= nextChild because width >= 1 and _minProportion < 1.
			++i;
			break;
			}

#ifdef GRAPH_LAYOUT_DEBUG
		TRACE( _T( "i: %i, nextChild: %i, childSize: %f, virtualRowHeight: %f\r\n" ), i, nextChild, childSize, virtualRowHeight );
#endif
		rowHeight = virtualRowHeight;
		}

	}

void CTreemap::KDirStat_OperateOnSingleChild( _In_ const Item* parent, _In_ _In_range_( 0, INT_MAX ) const INT nextChild, _In_ const DOUBLE mySize, _Inout_ DOUBLE& rowHeight, _Inout_ std::vector<DOUBLE>& childWidth, _In_ _In_range_( 0, 32767 ) const DOUBLE width, _Inout_ DOUBLE& cwTotal, _Inout_ DOUBLE& sizeSoFar, _In_ const INT j ) {
	// Rectangle(1.0 * 1.0) = mySize
	DOUBLE rowSize = { mySize * rowHeight };
	//rowSize += 1;//Fuck you floating point!
	sizeSoFar += mySize;
	auto childOfParent = parent->TmiGetChild( nextChild + j );
	if ( childOfParent != NULL ) {
#ifdef GRAPH_LAYOUT_DEBUG
		TRACE( _T( "Iter J:%i, Calculating rectangle for %i\r\n" ), j, ( nextChild + j ) );
#endif
		const auto childSize = DOUBLE( childOfParent->TmiGetSize( ) );
		ASSERT( std::fmod( childSize, 1 ) == 0 );
		//childSize -= 1;//Fuck you floating point!
		//ASSERT( ( ( childSize / rowSize ) + cwTotal ) <= width );
		//ASSERT( childSize == childOfParent->TmiGetSize( ) );
		ASSERT( childSize <= ( rowSize + 0.01 ) );//Fuck you, floating point!
		DOUBLE cw = childSize / rowSize;
		if ( ( cw + cwTotal ) > width ) {
			cw += ( width - ( cw + cwTotal ) );//ugly hack to deal with floating point madness!
			}
		ASSERT( ( cwTotal <= width ) && ( cw <= width ) && ( ( cw + cwTotal ) <= width ) );
		//cw = cw * ( sizeSoFar/rowSize );//
		//ASSERT( cwTotal >= shit );
		//ASSERT( cw <= ( ( width - cwTotal ) + 0.01 ) );//Fuck you, floating point!
			
		ASSERT( cw >= 0 );
			
		if ( childWidth.size( ) == ( nextChild + j ) ) {
			ASSERT( ( cw < 32767 ) && ( ( cw + cwTotal ) <= width ) && ( cw <= 1.00001 ) );//Fuck you, floating point!
			childWidth.push_back( cw );
			cwTotal += cw;
			ASSERT( cwTotal <= ( parent->TmiGetRectangle( ).right - parent->TmiGetRectangle( ).left ) );
			}
		else {
			ASSERT( ( childWidth.size( ) >= ( nextChild + j ) ) && ( cw <= 1 ) && ( ( nextChild + j ) < childWidth.size( ) ) && ( ( cw + cwTotal ) <= width ) );
			childWidth[ nextChild + j ] = cw;
			cwTotal += cw;
			ASSERT( cwTotal <= ( parent->TmiGetRectangle( ).right - parent->TmiGetRectangle( ).left ) );
			}
		}
	ASSERT( childOfParent != NULL );
	}

DOUBLE CTreemap::KDirStat_CalcutateNextRow( _In_ const Item* parent, _In_ _In_range_( 0, INT_MAX ) const INT nextChild, _In_ _In_range_( 0, 32767 ) const DOUBLE width, _Inout_ INT& childrenUsed, _Inout_ CArray<DOUBLE, DOUBLE>& childWidth ) {
	int i = 0;
	static const double _minProportion = 0.4;
	ASSERT( _minProportion < 1 );

	ASSERT( nextChild < parent->TmiGetChildrenCount( ) );
	ASSERT( width >= 1.0 );

	const double mySize = ( double ) parent->TmiGetSize( );
	ASSERT( mySize > 0 );
	ULONGLONG sizeUsed = 0;
	double rowHeight = 0;

	for ( i = nextChild; i < parent->TmiGetChildrenCount( ); i++ )
		{
		ULONGLONG childSize = parent->TmiGetChild(i)->TmiGetSize();
		if ( childSize == 0 ) {
			//WEAK_ASSERT( i > nextChild );  // first child has size > 0
			break;
			}

		sizeUsed += childSize;
		double virtualRowHeight = sizeUsed / mySize;
		ASSERT( virtualRowHeight > 0 );
		ASSERT( virtualRowHeight <= 1 );

		// Rectangle(mySize)    = width * 1.0
		// Rectangle(childSize) = childWidth * virtualRowHeight
		// Rectangle(childSize) = childSize / mySize * width

		double childWidth = childSize / mySize * width / virtualRowHeight;

		if ( childWidth / virtualRowHeight < _minProportion ) {
			ASSERT( i > nextChild ); // because width >= 1 and _minProportion < 1.
			// For the first child we have:
			// childWidth / rowHeight
			// = childSize / mySize * width / rowHeight / rowHeight
			// = childSize * width / sizeUsed / sizeUsed * mySize
			// > childSize * mySize / sizeUsed / sizeUsed
			// > childSize * childSize / childSize / childSize
			// = 1 > _minProportion.
			break;
			}
		rowHeight = virtualRowHeight;
		ASSERT( rowHeight != 0.00 );
	}
	//ASSERT( i > nextChild );

	// Now i-1 is the last child used
	// and rowHeight is the height of the row.

	// We add the rest of the children, if their size is 0.
	while ( i < parent->TmiGetChildrenCount( ) && parent->TmiGetChild( i )->TmiGetSize( ) == 0 ) {
		i++;
		}

	childrenUsed = i - nextChild;
	ASSERT( rowHeight != 0.00 );
	// Now as we know the rowHeight, we compute the widths of our children.
	for ( i = 0; i < childrenUsed; i++ ) {
		// Rectangle(1.0 * 1.0) = mySize
		double rowSize = mySize * rowHeight;
		double childSize = ( double ) parent->TmiGetChild( nextChild + i )->TmiGetSize( );
		ASSERT( rowSize != 0.00 );
		double cw = childSize / rowSize;
		ASSERT( cw >= 0 );
		childWidth[ nextChild + i ] = cw;
		}

#ifdef DEBUG
	for ( int i = 0; i < childWidth.GetSize( ); ++i ) {
		ASSERT( childWidth[ i ] >= 0 );
		}
#endif

	return rowHeight;

	}

//layoutrow() == PlaceChildren?
void CTreemap::SequoiaView_PlaceChildren( _In_ CDC* pdc, _In_ const Item* parent, _In_ _In_reads_( 4 ) const DOUBLE* surface, _In_ const DOUBLE h, _In_ const INT_PTR rowBegin, _In_ const INT_PTR rowEnd, _In_ DOUBLE fBegin, _In_ const std::uint64_t sum, _In_ const bool horizontal, _In_ const CRect& remaining, _Inout_ CRect& rc, _In_ const INT height ) {

	// Now put the children into their places
	for ( auto i = rowBegin; i < rowEnd; i++ ) {
		const auto    begin    = std::lround( fBegin );
		const auto   childAtI = parent->TmiGetChild( i );
		DOUBLE       fraction = DBL_MAX;

		if ( childAtI != NULL ) {
			fraction = DOUBLE( childAtI->TmiGetSize( ) ) / sum;
			}
		ASSERT( childAtI != NULL );
		ASSERT( fraction != DBL_MAX );
		const auto fEnd = fBegin + fraction * DOUBLE( height );
		auto end        = std::lround( fEnd );

		ASSERT( end > begin );

		const auto lastChild = ( i == rowEnd - 1 || parent->TmiGetChild( i + 1 )->TmiGetSize( ) == 0 );

		if ( lastChild ) { // Use up the whole height
			end = ( horizontal ? ( remaining.top + height -1) : ( remaining.left + height -1) );
			}
		
		ASSERT( end > begin );

		if ( horizontal ) {
			rc.top    = begin;
			rc.bottom = end;
			}
		else {
			rc.left  = begin;
			rc.right = end;
			}

		rc.NormalizeRect( );

		ASSERT( rc.left   < rc.right );
		ASSERT( rc.top    < rc.bottom );
		ASSERT( rc.left   >= remaining.left );
		ASSERT( rc.right  <= remaining.right );
		ASSERT( rc.top    >= remaining.top );
		ASSERT( rc.bottom <= remaining.bottom );

		ASSERT( childAtI == parent->TmiGetChild( i ) );
		if ( childAtI != NULL ) {
			if ( childAtI->TmiGetSize( ) == 0 ) {
				AfxDebugBreak( );
				}
			RecurseDrawGraph( pdc, childAtI, rc, false, surface, h * m_options.scaleFactor, 0 );
			}
		ASSERT( childAtI != NULL );
		if ( lastChild ) {
			break;
			}

		fBegin = fEnd;
		}
	
	}

bool WillGetWorse( _In_ const std::uint64_t sumOfSizeOfChildrenInThisRow, _In_ const LONGLONG minSizeOfChildrenInThisRow, _In_ const LONGLONG maxSizeOfChildrenInThisRow, _In_ const DOUBLE worstRatioSoFar, _In_ const DOUBLE hh, _Out_ DOUBLE& nextWorst ) {

	const auto ss = ( DOUBLE( sumOfSizeOfChildrenInThisRow + minSizeOfChildrenInThisRow ) ) * ( DOUBLE( sumOfSizeOfChildrenInThisRow + minSizeOfChildrenInThisRow ) );
	ASSERT( ss != 0 );
	ASSERT( hh != 0 );
	ASSERT( minSizeOfChildrenInThisRow != 0 );

	const DOUBLE ratio1 = hh * maxSizeOfChildrenInThisRow / ss;
	const DOUBLE ratio2 = ss / hh / minSizeOfChildrenInThisRow;
	nextWorst = max( ratio1, ratio2 );
	return nextWorst > worstRatioSoFar;
	}

void assign_rc_and_fBegin_horizontalOrVertical( _In_ const CRect& remainingRectangleToFill, _Inout_ CRect& rc, _Inout_ DOUBLE& fBegin, _In_ const bool divideHorizontally, _In_ const int widthOfThisRow ) {
	if ( divideHorizontally ) {
		rc.left   = remainingRectangleToFill.left;
		rc.right  = remainingRectangleToFill.left + widthOfThisRow;
		fBegin    = remainingRectangleToFill.top;
		ASSERT( rc.left   < rc.right );
		}
	else {
		rc.top    = remainingRectangleToFill.top;
		rc.bottom = remainingRectangleToFill.top + widthOfThisRow;
		fBegin    = remainingRectangleToFill.left;
		ASSERT( rc.top    < rc.bottom );
		}
	}

void addChild_rowEnd_toRow( _Inout_ std::uint64_t& sumOfSizeOfChildrenInThisRow, _In_ const LONGLONG minSizeOfChildrenInThisRow, _Inout_ INT_PTR& rowEnd, _In_ const DOUBLE& nextWorst, _Inout_ DOUBLE& worstRatioSoFar ) {

	
	}

void CTreemap::checkVirtualRowOf_rowBegin_to_rowEnd__thenAdd( _In_ Item* parent, _Inout_ INT_PTR& rowEnd, _Out_ std::uint64_t& sumOfSizeOfChildrenInThisRow, _In_ const LONGLONG maxSizeOfChildrenInThisRow, _Inout_ DOUBLE& worstRatioSoFar, _In_ const DOUBLE hh ) {

	TRACE( _T( "rowEnd: %i, parent->TmiGetChildrenCount( ): %i\r\n" ), int( rowEnd ), int( parent->TmiGetChildrenCount( ) ) );
	// This condition will hold at least once.
	while ( rowEnd < parent->TmiGetChildrenCount( ) ) { // We check a virtual row made up of child(rowBegin)...child(rowEnd) here.
		//

		// Minimum size of child in virtual row
		LONGLONG minSizeOfChildrenInThisRow = 0;
		auto childRowEnd = parent->TmiGetChild( rowEnd );
		ASSERT( childRowEnd != NULL );
		if ( childRowEnd != NULL ) {
			minSizeOfChildrenInThisRow = childRowEnd->TmiGetSize( );
			}
		// If sizes of the rest of the children is zero, we add all of them
		if ( minSizeOfChildrenInThisRow == 0 ) {
			rowEnd = parent->TmiGetChildrenCount( );
			break;
			}

		// Calculate the worst ratio in virtual row. Formula taken from the "Squarified Treemaps" paper: (http://http://www.win.tue.nl/~vanwijk/)

		DOUBLE nextWorst = DBL_MAX;

		if ( WillGetWorse( sumOfSizeOfChildrenInThisRow, minSizeOfChildrenInThisRow, maxSizeOfChildrenInThisRow, worstRatioSoFar, hh, nextWorst ) ) {
			ASSERT( nextWorst != DBL_MAX );
			break;// Yes. Don't take the virtual row, but the real row (child(rowBegin)..child(rowEnd - 1)) made so far.
			}

		ASSERT( nextWorst != DBL_MAX );

		// Here we have decided to add child( rowEnd ) to the row.
		TRACE( _T( "sumSizeRow: %i\r\n" ), int( sumOfSizeOfChildrenInThisRow ) );
		//addChild_rowEnd_toRow( sumOfSizeOfChildrenInThisRow, minSizeOfChildrenInThisRow, rowEnd, nextWorst, worstRatioSoFar );
		sumOfSizeOfChildrenInThisRow += minSizeOfChildrenInThisRow;
		ASSERT( sumOfSizeOfChildrenInThisRow >= 0 );
		worstRatioSoFar = nextWorst;
		++rowEnd;
		TRACE( _T( "rowEnd: %i, parent->TmiGetChildrenCount( ): %i\r\n" ), int( rowEnd ), int( parent->TmiGetChildrenCount( ) ) );
		}
	TRACE( _T( "rowEnd: %i, parent->TmiGetChildrenCount( ): %i\r\n" ), int( rowEnd ), int( parent->TmiGetChildrenCount( ) ) );
	ASSERT( sumOfSizeOfChildrenInThisRow > 0 );
	}

// The classical squarification method.
void CTreemap::SequoiaView_DrawChildren( _In_ CDC* pdc, _In_ Item* parent, _In_ _In_reads_( 4 ) const DOUBLE* surface, _In_ const DOUBLE h, _In_ const DWORD flags ) {
	// Rest rectangle to fill
	CRect remaining( parent->TmiGetRectangle( ) );

	ASSERT( remaining.Width( ) > 0 );
	ASSERT( remaining.Height( ) > 0 );

	// Size of rest rectangle
	ULONGLONG remainingSize = parent->TmiGetSize( );
	ASSERT( remainingSize > 0 );

	// Scale factor
	const double sizePerSquarePixel = ( double ) parent->TmiGetSize( ) / remaining.Width( ) / remaining.Height( );

	// First child for next row
	int head = 0;

	// At least one child left
	while ( head < parent->TmiGetChildrenCount( ) ) {
		ASSERT( remaining.Width( ) > 0 );
		ASSERT( remaining.Height( ) > 0 );

		// How we divide the remaining rectangle
		bool horizontal = ( remaining.Width( ) >= remaining.Height( ) );

		// Height of the new row
		const int height = horizontal ? remaining.Height( ) : remaining.Width( );

		// Square of height in size scale for ratio formula
		const double hh = ( height * height ) * sizePerSquarePixel;
		ASSERT( hh > 0 );

		// Row will be made up of child(rowBegin)...child(rowEnd - 1)
		int rowBegin = head;
		int rowEnd = head;

		// Worst ratio so far
		double worst = DBL_MAX;

		// Maximum size of children in row
		ULONGLONG rmax = parent->TmiGetChild( rowBegin )->TmiGetSize( );

		// Sum of sizes of children in row
		ULONGLONG sum = 0;

		// This condition will hold at least once.
		while ( rowEnd < parent->TmiGetChildrenCount( ) ) {
			// We check a virtual row made up of child(rowBegin)...child(rowEnd) here.

			// Minimum size of child in virtual row
			ULONGLONG rmin = parent->TmiGetChild( rowEnd )->TmiGetSize( );

			// If sizes of the rest of the children is zero, we add all of them
			if ( rmin == 0 ) {
				rowEnd = parent->TmiGetChildrenCount( );
				break;
				}

			// Calculate the worst ratio in virtual row.
			// Formula taken from the "Squarified Treemaps" paper.
			// (http://http://www.win.tue.nl/~vanwijk/)

			const double ss = ( ( double ) sum + rmin ) * ( ( double ) sum + rmin );
			const double ratio1 = hh * rmax / ss;
			const double ratio2 = ss / hh / rmin;

			const double nextWorst = max( ratio1, ratio2 );

			// Will the ratio get worse?
			if ( nextWorst > worst ) {
				// Yes. Don't take the virtual row, but the
				// real row (child(rowBegin)..child(rowEnd - 1))
				// made so far.
				break;
				}

			// Here we have decided to add child(rowEnd) to the row.
			sum += rmin;
			rowEnd++;

			worst = nextWorst;
			}

		// Row will be made up of child(rowBegin)...child(rowEnd - 1).
		// sum is the size of the row.

		// As the size of parent is greater than zero, the size of
		// the first child must have been greater than zero, too.
		ASSERT( sum > 0 );

		// Width of row
		int width = ( horizontal ? remaining.Width( ) : remaining.Height( ) );
		ASSERT( width > 0 );

		if ( sum < remainingSize )
			width = ( int ) ( ( double ) sum / remainingSize * width );
		// else: use up the whole width
		// width may be 0 here.

		// Build the rectangles of children.
		CRect rc;
		double fBegin;
		if ( horizontal ) {
			rc.left = remaining.left;
			rc.right = remaining.left + width;
			fBegin = remaining.top;
			}
		else {
			rc.top = remaining.top;
			rc.bottom = remaining.top + width;
			fBegin = remaining.left;
			}

		// Now put the children into their places
		for ( int i = rowBegin; i < rowEnd; i++ ) {
			int begin = ( int ) fBegin;
			double fraction = ( double ) ( parent->TmiGetChild( i )->TmiGetSize( ) ) / sum;
			double fEnd = fBegin + fraction * height;
			int end = ( int ) fEnd;

			bool lastChild = ( i == rowEnd - 1 || parent->TmiGetChild( i + 1 )->TmiGetSize( ) == 0 );

			if ( lastChild ) {
				// Use up the whole height
				end = ( horizontal ? remaining.top + height : remaining.left + height );
				}

			if ( horizontal ) {
				rc.top = begin;
				rc.bottom = end;
				}
			else {
				rc.left = begin;
				rc.right = end;
				}

			ASSERT( rc.left <= rc.right );
			ASSERT( rc.top <= rc.bottom );

			ASSERT( rc.left >= remaining.left );
			ASSERT( rc.right <= remaining.right );
			ASSERT( rc.top >= remaining.top );
			ASSERT( rc.bottom <= remaining.bottom );

			RecurseDrawGraph( pdc, parent->TmiGetChild( i ), rc, false, surface, h * m_options.scaleFactor, 0 );

			if ( lastChild )
				break;

			fBegin = fEnd;
			}

		// Put the next row into the rest of the rectangle
		if ( horizontal ) {
			remaining.left += width;
			}
		else {
			remaining.top += width;
			}

		remainingSize -= sum;

		ASSERT( remaining.left <= remaining.right );
		ASSERT( remaining.top <= remaining.bottom );

		ASSERT( remainingSize >= 0 );

		head += ( rowEnd - rowBegin );

		if ( remaining.Width( ) <= 0 || remaining.Height( ) <= 0 ) {
			if ( head < parent->TmiGetChildrenCount( ) ) {
				parent->TmiGetChild( head )->TmiSetRectangle( CRect( -1, -1, -1, -1 ) );
				}

			break;
			}
		}
	ASSERT( remainingSize == 0 );
	ASSERT( remaining.left == remaining.right || remaining.top == remaining.bottom );

	}

// No squarification. Children are arranged alternately horizontally and vertically.
void CTreemap::Simple_DrawChildren( _In_ const CDC* pdc, _In_ const Item* parent, _In_ _In_reads_( 4 ) const DOUBLE* surface, _In_ const DOUBLE h, _In_ const DWORD flags ) {
	ASSERT_VALID( pdc );
	ASSERT( false ); // Not used in Windirstat.
	UNREFERENCED_PARAMETER( pdc );
	UNREFERENCED_PARAMETER( parent );
	UNREFERENCED_PARAMETER( surface );
	UNREFERENCED_PARAMETER( h );
	UNREFERENCED_PARAMETER( flags );
	}

bool CTreemap::IsCushionShading( ) const {
	return m_options.ambientLight < 1.0 && m_options.height > 0.0 && m_options.scaleFactor > 0.0;
	}

void CTreemap::RenderLeaf( _In_ CDC* pdc, _In_ Item* item, _In_ _In_reads_( 4 ) const DOUBLE* surface ) {
	auto rc = item->TmiGetRectangle( );
	if ( m_options.grid ) {
		rc.top++;
		rc.left++;
		if ( ( rc.Width( ) ) <= 0 || ( rc.Height( ) ) <= 0 ) {
			return;
			}
		}
	rc.NormalizeRect( );
	auto colorOfItem = item->TmiGetGraphColor( );
	ASSERT( colorOfItem != 0 );
	RenderRectangle( pdc, rc, surface, colorOfItem );
	}

void CTreemap::RenderRectangle( _In_ CDC* pdc, _In_ const CRect& rc, _In_ _In_reads_( 4 ) const DOUBLE* surface, _In_ DWORD color ) {
	//ASSERT( ( rc.Width( ) > 0 ) || ( rc.Height( ) > 0 ) );
	if ( ( ( rc.Width( ) ) == 0 ) || ( ( rc.Height( ) ) ) ) {
		//TRACE( _T( "Huh?\r\n" ) );
		//return;
		}
	auto brightness = m_options.brightness;
	if ( ( color & COLORFLAG_MASK ) != 0 ) {
		auto flags = ( color & COLORFLAG_MASK );
		color = CColorSpace::MakeBrightColor( color, PALETTE_BRIGHTNESS );
		if ( ( flags & COLORFLAG_DARKER ) != 0 ) {
			brightness *= 0.66;
			}
		else {
			brightness *= 1.2;
			if ( brightness > 1.0 ) {
				brightness = 1.0;
				}
			}
		}
	ASSERT( color != 0 );
	if ( IsCushionShading_current ) {
		DrawCushion( pdc, rc, surface, color, brightness );
		}
	else {
		DrawSolidRect( pdc, rc, color, brightness );
		}
	}

void CTreemap::DrawSolidRect( _In_ CDC* pdc, _In_ const CRect& rc, _In_ const COLORREF col, _In_ _In_range_( 0, 1 ) const DOUBLE brightness ) {
	INT red   = GetRValue( col );
	INT green = GetGValue( col );
	INT blue  = GetBValue( col );
	
	const DOUBLE factor = brightness / PALETTE_BRIGHTNESS;

	red = INT( std::lround( red * factor ) );
	green = INT( std::lround( green * factor ));
	blue  = INT( std::lround( blue * factor ) );

	CColorSpace::NormalizeColor( red, green, blue );

	pdc->FillSolidRect( rc, RGB( red, green, blue ) );
	}

//void setPix( CDC* pdc, std::mutex* pixlesMutex, std::mutex* pdcMutex, std::queue<setPixStruct>& pixles, std::condition_variable* isDataReady, std::atomic_bool* isDone) {
//	while ( !std::atomic_load( isDone ) ) {
//		std::unique_lock<std::mutex> lck( *pixlesMutex );
//		if ( std::atomic_load( isDone ) ) {
//			return;
//			}
//
//		isDataReady->wait( lck );
//		{
//		if ( pixles.size( ) > 0 ) {
//			auto aPixle = std::move( pixles.front( ) );
//			pixles.pop( );
//			lck.unlock( );
//			pdcMutex->lock( );
//
//			TRACE( _T( "Setting color %i\r\n" ), aPixle.color );
//			pdc->SetPixel( aPixle.ix, aPixle.iy, aPixle.color );
//			pdcMutex->unlock( );
//			}
//		else {
//			return;
//			}
//		}
//		}
//	return;
//	}


void CTreemap::DrawCushion( _In_ CDC *pdc, const _In_ CRect& rc, _In_ _In_reads_( 4 ) const DOUBLE* surface, _In_ const COLORREF col, _In_ _In_range_(0, 1) const DOUBLE brightness ) {
	//ASSERT( ( rc.Width()  > 0 ) || ( rc.Height() > 0 ) );
	// Cushion parameters
	const DOUBLE Ia = m_options.ambientLight;

	// Derived parameters
	const DOUBLE Is = 1 - Ia;			// shading

	const DOUBLE colR = GetRValue( col );//THIS does NOT get vectorized!
	const DOUBLE colG = GetGValue( col );//THIS gets vectorized
	const DOUBLE colB = GetBValue( col );//THIS does NOT get vectorized!
	
	std::vector<setPixStruct> xPixles;
	xPixles.reserve( size_t( ( rc.Width( ) ) + 1 ) );
#ifdef GRAPH_LAYOUT_DEBUG
	TRACE( _T( "DrawCushion drawing rectangle    left: %li, right: %li, top: %li, bottom: %li\r\n" ), rc.left, rc.right, rc.top, rc.bottom );
#endif
	for ( INT iy = rc.top; iy < rc.bottom; iy++ ) {
		xPixles.reserve( size_t( ( rc.Width( ) ) + 1 ) );
		for ( INT ix = rc.left; ix < rc.right; ix++ ) {
			/*
			  BOTH for initializations get vectorized
			  EVERYTHING until (NOT including) NormalizeColor gets vectorized :)
			  THAT SAID, there are still two branches (iy < rc.botton, ix < rc.right)
			  */
			auto nx = -( 2.00 * surface[ 0 ] * ( ix + 0.5 ) + surface[ 2 ] );
			auto ny = -( 2.00 * surface[ 1 ] * ( iy + 0.5 ) + surface[ 3 ] );
			auto cosa = ( nx*m_Lx + ny*m_Ly + m_Lz ) / sqrt( nx*nx + ny*ny + 1.0 );

			ASSERT( cosa <= 1.0 );

			auto pixel = Is * cosa;
			if ( pixel < 0 ) {
				pixel = 0;
				}

			pixel += Ia;
			ASSERT( pixel <= 1.0 );

			// Now, pixel is the brightness of the pixel, 0...1.0.
			// Apply contrast.
			// Not implemented.
			// Costs performance and nearly the same effect can be made width the m_options->ambientLight parameter.
			// pixel= pow(pixel, m_options->contrast);
			// Apply "brightness"
			pixel *= brightness / PALETTE_BRIGHTNESS;

			/*
			  for some reason, issues a "Convert two packed signed doubleword integers from xmm2/mem to two packed double-precision floating-point values in xmm1" (vcvtdq2pd) instruction, an vmulsd ("Multiply the low double-precision floating-point value in xmm3/mem64 by low double precision floating-point value in xmm2") instruction, then a "Convert one double-precision floating-point value from xmm1/m64 to one signed doubleword integer in r32 using truncation" (vcvttsd2si) instruction. Idiotic.

			  INT red   = ( INT ) ( colR * pixel );
			  INT green = ( INT ) ( colG * pixel );
			  INT blue  = ( INT ) ( colB * pixel );
			  */


			// Make color value
			auto red   = INT( colR * pixel );
			auto green = INT( colG * pixel );
			auto blue  = INT( colB * pixel );
			if ( red >= 256 ) {
				red = 255;
				}
			if ( green >= 256 ) {
				green = 255;
				}
			if ( blue >= 256 ) {
				blue = 255;
				}
			//TRACE( _T( "red: %i, green: %i, blue: %i\r\n" ), red, green, blue );
			//#pragma omp critical
			CColorSpace::NormalizeColor( red, green, blue );
			if ( red == 0 ) {
				red++;
				}
			if ( green == 0 ) {
				green++;
				}
			if ( blue == 0 ) {
				blue++;
				}
			// ... and set!
			ASSERT( RGB( red, green, blue ) != 0 );
			static_assert( sizeof( INT ) == sizeof( std::int_fast32_t ), "setPixStruct bad point type!!" );
			static_assert( sizeof( std::int_fast32_t ) == sizeof( COLORREF ), "setPixStruct bad color type!!" );
			xPixles.emplace_back( setPixStruct ( ix, iy, RGB( red, green, blue ) ) );//TODO fix implicit conversion!
			}
		for ( INT ix = rc.left; ix < rc.right; ix++ ) {
			setPixStruct& setP = xPixles.at( ix - rc.left );
#ifdef GRAPH_LAYOUT_DEBUG
			debugSetPixel( pdc, setP.ix, setP.iy, setP.color );//debug version that detects drawing collisions
#else
			pdc->SetPixel( setP.ix, setP.iy, setP.color );//this is the sole remaining bottleneck here. The drawing time is a direct function of the drawing area - i.e. how many times we have to setPixel!
#endif
			}
		xPixles.clear( );
		}
	}

#ifdef GRAPH_LAYOUT_DEBUG
void CTreemap::debugSetPixel( CDC* pdc, int x, int y, COLORREF c ) {
	++numCalls;
	//This function detects drawing collisions!
	if ( !( bitSetMask->at( x ).at( y ) ) ) {
		( *bitSetMask )[ x ][ y ] = true;//we already know that we're in bounds.
		pdc->SetPixel( x, y, c );
		}
	else {

		ASSERT( false );
		AfxDebugBreak( );
		}
	}
#endif

void CTreemap::AddRidge( _In_ const CRect& rc, _Inout_ _Inout_updates_( 4 ) DOUBLE* surface, _In_ const DOUBLE h ) {
	auto width  = ( rc.Width( ) );
	auto height = ( rc.Height( ) );

	ASSERT( width > 0 && height > 0 );

	DOUBLE h4 = 4 * h;

	DOUBLE wf   = h4 / width;
	surface[ 2 ] += wf * ( rc.right + rc.left );
	surface[ 0 ] -= wf;

	DOUBLE hf   = h4 / height;
	surface[ 3 ] += hf * ( rc.bottom + rc.top );
	surface[ 1 ] -= hf;
	}


/////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CTreemapPreview, CStatic)
	ON_WM_PAINT()
END_MESSAGE_MAP()

CTreemapPreview::CTreemapPreview( ) {
	m_root = NULL;
	BuildDemoData();
	}

CTreemapPreview::~CTreemapPreview( ) {
	if ( m_root != NULL ) {
		delete m_root;
		m_root = NULL;
		}
	}

void CTreemapPreview::SetOptions(_In_ const CTreemap::Options *options)
{
	m_treemap.SetOptions(options);
	Invalidate();
}

void CTreemapPreview::BuildDemoData( ) {
	//CTreemap::GetDefaultPalette( m_colors );
	m_vectorOfColors = CTreemap::GetDefaultPaletteAsVector( );
	size_t col = 0;
	COLORREF color;
	INT i = 0;

#ifdef CHILDVEC
	std::vector<CItem*> c4;
	std::vector<CItem*> c1;
	std::vector<CItem*> c2;
	std::vector<CItem *> c3;
	std::vector<CItem *> c10;
	std::vector<CItem*> c0;
#else
	CArray<CItem *, CItem *> c4;
	CArray<CItem *, CItem *> c0;
	CArray<CItem *, CItem *> c1;
	CArray<CItem *, CItem *> c2;
	CArray<CItem *, CItem *> c3;
	CArray<CItem *, CItem *> c10;
#endif
	
	color = GetNextColor( col );
	for ( i = 0; i < 20; i++ ) {
		c4.polyAdd( new CItem { 7 * i, color } );
		}
	for ( i = 0; i < 9; i++ ) {
		c0.polyAdd( new CItem { 13 * i, GetNextColor( col ) } );
		}

	color = GetNextColor( col );
	for ( i = 0; i < 7; i++ ) {
		c1.polyAdd( new CItem { 23 * i, color } );
		}
	c0.polyAdd( new CItem { c1 } );

	color = GetNextColor( col );
	for ( i = 0; i < 53; i++ ) {
		c2.polyAdd( new CItem { 1 + i, color } );
		}
	c3.polyAdd( new CItem { 457, GetNextColor( col ) } );
	c3.polyAdd( new CItem { c4 } );
	c3.polyAdd( new CItem { c2 } );
	c3.polyAdd( new CItem { 601, GetNextColor( col ) } );
	c3.polyAdd( new CItem { 151, GetNextColor( col ) } );

	
	c10.polyAdd( new CItem { c0 } );
	c10.polyAdd( new CItem { c3 } );;
	m_root = new CItem { c10 };
	}

COLORREF CTreemapPreview::GetNextColor( _Inout_ size_t& i ) {
	++i;
	i %= m_vectorOfColors.size( );
	return m_vectorOfColors.at( i );
	}

void CTreemapPreview::OnPaint( ) {
	CPaintDC dc( this );
	CRect rc;
	GetClientRect( rc );
	m_treemap.DrawTreemapDoubleBuffered( &dc, rc, m_root );
	}


// $Log$
// Revision 1.6  2004/11/05 16:53:08  assarbad
// Added Date and History tag where appropriate.
//
