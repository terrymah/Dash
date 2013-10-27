#include "DGui.h"
#include "atlbase.h"
#include "utils.h"

namespace tjm {
namespace dash {


struct SplitterImpl
{
	Orientation m_orientation;
	SplitterStyle m_style;
	D2D1::ColorF m_color;
	bool m_moveable;
	Object* m_leftTop;
	SplitLayoutType m_leftTopLayoutType;
	Object* m_rightBottom;
	SplitLayoutType m_rightBottomLayoutType;
	FLOAT m_splitterWidth;
	DOUBLE m_min;
	bool m_minIsPercent;
	DOUBLE m_max;
	bool m_maxIsPercent;
	
	DOUBLE m_oldPos;
	bool m_collapsed;

	DOUBLE m_pos; // always stored as a percent
	tjm::animation::AnimatedVar m_splitterPos;

	CComPtr<ID2D1SolidColorBrush> m_brush;

	SplitterImpl();
};

SplitterImpl::SplitterImpl() :
m_orientation(Orientation::Horizontal),
m_color(D2D1::ColorF::LightGray),
m_style(SplitterStyle::Dots),
m_moveable(true),
m_leftTop(nullptr),
m_leftTopLayoutType(SplitLayoutType::Variable),
m_rightBottom(nullptr),
m_rightBottomLayoutType(SplitLayoutType::Variable),
m_splitterWidth(15.0),
m_min(0.0),
m_minIsPercent(true),
m_max(1.0),
m_maxIsPercent(true),
m_collapsed(false),
m_pos(.5)
{
}

Splitter::Splitter() :
m_pImpl(new SplitterImpl())
{
}

Splitter::~Splitter()
{
	delete m_pImpl;
}

void Splitter::SetOrientation(Orientation o)
{
	m_pImpl->m_orientation = o;
}

Orientation Splitter::GetOrientation() const
{
	return m_pImpl->m_orientation;
}

void Splitter::SetStyle(SplitterStyle s)
{
	m_pImpl->m_style = s;
}

SplitterStyle Splitter::GetStyle() const
{
	return m_pImpl->m_style;
}

void Splitter::SetColor(D2D1::ColorF color)
{
	m_pImpl->m_color = color;
	m_pImpl->m_brush.Release();
}

D2D1::ColorF Splitter::GetColor() const
{
	return m_pImpl->m_color;
}

void Splitter::SetMoveable(bool moveable)
{
	m_pImpl->m_moveable = moveable;
}

bool Splitter::GetMoveable() const
{
	return m_pImpl->m_moveable;
}

void Splitter::SetLeftTop(Object* obj, SplitLayoutType layout)
{
	RemoveChild(GetLeftTop());
	AddChild(obj);
	m_pImpl->m_leftTop = obj;
	m_pImpl->m_leftTopLayoutType = layout;
}

Object* Splitter::GetLeftTop() const
{
	return m_pImpl->m_leftTop;
}

void Splitter::SetRightBottom(Object* obj, SplitLayoutType layout)
{
	RemoveChild(GetRightBottom());
	AddChild(obj);
	m_pImpl->m_rightBottom = obj;
	m_pImpl->m_rightBottomLayoutType = layout;
}

Object* Splitter::GetRightBottom() const
{
	return m_pImpl->m_rightBottom;
}

void Splitter::SetSplitterWidth(FLOAT width)
{
	if(width != m_pImpl->m_splitterWidth) 
	{
		m_pImpl->m_splitterWidth = width;
		DirtyLayout();
	}
}

FLOAT Splitter::GetSplitterWidth() const
{
	return m_pImpl->m_splitterWidth;
}

void Splitter::SetSplitterMin(DOUBLE min)
{
	m_pImpl->m_minIsPercent = false;
	m_pImpl->m_min = min;
	SetBounds();
}

void Splitter::SetSplitterMinPercent(DOUBLE min)
{
	m_pImpl->m_minIsPercent = true;
	m_pImpl->m_min = min;
	SetBounds();
}

void Splitter::SetSplitterMax(DOUBLE max)
{
	m_pImpl->m_maxIsPercent = false;
	m_pImpl->m_max = max;
	SetBounds();
}

void Splitter::SetSplitterMaxPercent(DOUBLE max)
{
	m_pImpl->m_maxIsPercent = true;
	m_pImpl->m_max = max;
	SetBounds();
}

void Splitter::SetSplitterPos(DOUBLE pos)
{
	m_pImpl->m_pos = pos / SplitLength();
	DirtyLayout();
}

void Splitter::SetSplitterPosPercent(DOUBLE posPercent)
{
	m_pImpl->m_pos = posPercent;
	DirtyLayout();
}

DOUBLE Splitter::GetSplitterPos() const
{
	return m_pImpl->m_splitterPos;
}

DOUBLE Splitter::GetSplitterPosFinal() const
{
	return m_pImpl->m_splitterPos.GetFinalValue();
}

bool Splitter::IsCollapsed() const
{
	return m_pImpl->m_collapsed;
}

void Splitter::Collapse(bool bottomLeft)
{
	m_pImpl->m_oldPos = (DOUBLE)m_pImpl->m_splitterPos;
	m_pImpl->m_pos = bottomLeft ? 0.0 : 1.0;
	m_pImpl->m_collapsed = true;
	DirtyLayout();
}

void Splitter::Restore()
{
	m_pImpl->m_pos = m_pImpl->m_oldPos;
	m_pImpl->m_collapsed = false;
	DirtyLayout();
}

void Splitter::OnLayout()
{
	tjm::animation::StoryBoard b;
	FLOAT left, top, right, bottom;
	FLOAT secondLeft, secondTop, secondRight, secondBottom;

	// First, set the position of the splitter
	SetBounds();
	FLOAT pos = SplitLength() * (FLOAT)m_pImpl->m_pos;
	m_pImpl->m_splitterPos = pos;

	// Read back bounded value
	pos = (FLOAT)m_pImpl->m_splitterPos.GetFinalValue();

	// Now position the left and right objects.
	if(GetOrientation() == Orientation::Horizontal)
	{
		top = GetLeftTop()->GetMarginTop();
		bottom = GetSize().height - GetLeftTop()->GetMarginBottom();
		switch(m_pImpl->m_leftTopLayoutType)
		{
		case SplitLayoutType::Variable:
			// Position at 0,0 up to splitter pos
			left = GetLeftTop()->GetMarginLeft();
			right = pos - (GetSplitterWidth() / 2) - GetLeftTop()->GetMarginRight();
			break;
		case SplitLayoutType::Fixed:
			// Position up against the splitter pos, full size
			right = pos - (GetSplitterWidth() / 2) - GetLeftTop()->GetMarginRight();
			left = right - GetSize().width;
			break;
		case SplitLayoutType::Overlay:
			// Easy case, just position at 0,0 full size
			left = GetLeftTop()->GetMarginLeft();
			right = GetSize().width - GetLeftTop()->GetMarginRight();

			// clip
			D2D1_RECT_F clipRect = D2D1::RectF(0, 0, pos - GetSplitterWidth() / 2 - GetLeftTop()->GetMarginLeft(), SplitHeight());
			GetLeftTop()->SetClippingRect(clipRect);
			break;
		}

		secondTop = GetRightBottom()->GetMarginTop();
		secondBottom = GetSize().height - GetRightBottom()->GetMarginBottom();
		switch(m_pImpl->m_rightBottomLayoutType)
		{
		case SplitLayoutType::Variable:
			// Position at splitter pos up to width
			secondRight = GetSize().width - GetRightBottom()->GetMarginRight();
			secondLeft = pos + (GetSplitterWidth() / 2) + GetRightBottom()->GetMarginLeft();
			break;
		case SplitLayoutType::Fixed:
			// Position up against the splitter pos, full size
			secondLeft = pos + (GetSplitterWidth() / 2) + GetRightBottom()->GetMarginLeft();
			secondRight = secondLeft + GetSize().width;
			break;
		case SplitLayoutType::Overlay:
			// Easy case, just position at 0,0 full size (plus margins)
			secondLeft = GetRightBottom()->GetMarginLeft();
			secondRight = GetSize().width - GetRightBottom()->GetMarginRight();

			// clip
			D2D1_RECT_F clipRect = D2D1::RectF(pos + GetSplitterWidth() / 2 - GetRightBottom()->GetMarginLeft(), 0, SplitLength(), SplitHeight());
			GetRightBottom()->SetClippingRect(clipRect);
			break;
		}
	}
	else
	{
		// Also add Masking to Object for Overlay case.
		// Also add explict Layout pass (rather than integrate with Render)
		left = GetLeftTop()->GetMarginLeft();
		right = GetSize().width - GetLeftTop()->GetMarginRight();
		switch(m_pImpl->m_leftTopLayoutType)
		{
		case SplitLayoutType::Variable:
			// Position at 0,0 up to splitter pos
			top = GetLeftTop()->GetMarginTop();
			bottom = pos - (GetSplitterWidth() / 2) - GetLeftTop()->GetMarginBottom();
			break;
		case SplitLayoutType::Fixed:
			// Position up against the splitter pos, full size
			bottom = pos - (GetSplitterWidth() / 2) - GetLeftTop()->GetMarginBottom();
			top = bottom - GetSize().height;
			break;
		case SplitLayoutType::Overlay:
			// Easy case, just position at 0,0 full size
			top = GetLeftTop()->GetMarginTop();
			bottom = GetSize().height - GetLeftTop()->GetMarginBottom();

			// clip
			D2D1_RECT_F clipRect = D2D1::RectF(0, 0, SplitHeight(), pos - (GetSplitterWidth() / 2) - GetLeftTop()->GetMarginTop());
			GetLeftTop()->SetClippingRect(clipRect);
			break;
		}

		secondLeft = GetRightBottom()->GetMarginTop();
		secondBottom = GetSize().height - GetRightBottom()->GetMarginBottom();
		switch(m_pImpl->m_leftTopLayoutType)
		{
		case SplitLayoutType::Variable:
			// Position at splitter pos up to width
			secondBottom = GetSize().height - GetRightBottom()->GetMarginBottom();
			secondTop = pos + (GetSplitterWidth() / 2) + GetRightBottom()->GetMarginTop();
			break;
		case SplitLayoutType::Fixed:
			// Position up against the splitter pos, full size
			secondTop = pos + (GetSplitterWidth() / 2) + GetRightBottom()->GetMarginTop();
			secondBottom = secondTop + GetSize().height;
			break;
		case SplitLayoutType::Overlay:
			// Easy case, just position at 0,0 full size (plus margins)
			secondTop = GetRightBottom()->GetMarginTop();
			secondBottom = GetSize().height - GetRightBottom()->GetMarginBottom();

			// clip
			D2D1_RECT_F clipRect = D2D1::RectF(0, pos + GetSplitterWidth() / 2 - GetRightBottom()->GetMarginTop(), SplitLength(), SplitHeight());
			GetRightBottom()->SetClippingRect(clipRect);
			break;
		}
	}
	GetLeftTop()->SetPosition(D2D1::Point2F(left, top));
	GetLeftTop()->SetSize(D2D1::SizeF(right - left, bottom - top));
	GetRightBottom()->SetPosition(D2D1::Point2F(secondLeft, secondTop));
	GetRightBottom()->SetSize(D2D1::SizeF(secondRight - secondLeft, secondBottom - secondTop));
}

D2D1_SIZE_F Splitter::GetPreferredSize(D2D1_SIZE_F& max)
{
	D2D1_SIZE_F first = GetLeftTop()->GetPreferredSize(max);
	D2D1_SIZE_F second = GetRightBottom()->GetPreferredSize(max);

	if(second.height > first.height)
		first.height = second.height;
	if(second.width > first.width)
		first.width = second.width;

	return first;
}

FLOAT Splitter::SplitLength() const
{
	if(GetOrientation() == Orientation::Horizontal)
		return GetSize().width;
	return GetSize().height;
}

FLOAT Splitter::SplitHeight() const
{
	if(GetOrientation() == Orientation::Horizontal)
		return GetSize().height;
	return GetSize().width;
}

void Splitter::SetBounds()
{
	DOUBLE min, max;

	if(IsCollapsed())
	{
		min = 0;
		max = SplitLength();
	}
	else
	{
		if(m_pImpl->m_minIsPercent)
		{
			min = SplitLength() * m_pImpl->m_min;
		}
		else
		{
			if(m_pImpl->m_min > 0)
				min = m_pImpl->m_min;
			else
				min = SplitLength() - m_pImpl->m_min;
		}

		if(m_pImpl->m_maxIsPercent)
		{
			max = SplitLength() * m_pImpl->m_max;
		}
		else
		{
			if(m_pImpl->m_max > 0)
				max = m_pImpl->m_max;
			else
				max = SplitLength() - m_pImpl->m_max;
		}
	}
	m_pImpl->m_splitterPos.SetMin(&min);
	m_pImpl->m_splitterPos.SetMax(&max);
}

void Splitter::OnRenderForeground(ID2D1RenderTarget* pTarget, const D2D1_RECT_F& /*box*/, DOUBLE effectiveOpacity)
{
	if(!m_pImpl->m_brush)
	{
		CORt(pTarget->CreateSolidColorBrush(GetColor(), &m_pImpl->m_brush));
	}

	switch(GetStyle())
	{
	case SplitterStyle::None:
		return;
	case SplitterStyle::Box:
		{
			D2D1_RECT_F splitterRect = GetSplitterRect();
			pTarget->DrawRectangle(splitterRect, m_pImpl->m_brush, 2.0f);
		}
		return;
	case SplitterStyle::Line:
		{
			D2D1_POINT_2F start;
			D2D1_POINT_2F end;
			if(GetOrientation() == Orientation::Horizontal)
			{
				start = D2D1::Point2F((FLOAT)m_pImpl->m_splitterPos, 0);
				end = D2D1::Point2F(start.x, SplitHeight());
			}
			else
			{
				start = D2D1::Point2F(0, (FLOAT)m_pImpl->m_splitterPos);
				end = D2D1::Point2F(SplitHeight(), start.y);
			}
			pTarget->DrawLine(start, end, m_pImpl->m_brush, 2.0f);
		}
	case SplitterStyle::Dots:
		{
			D2D1_ELLIPSE ellipse;
			ellipse.radiusX = ellipse.radiusY = GetSplitterWidth() / 6;
			FLOAT step = GetSplitterWidth();
			FLOAT circlePos = (SplitHeight()/2) - (3 * step);
			for(int i = 0; i < 7; ++i)
			{
				if(GetOrientation() == Orientation::Horizontal)
				{
					ellipse.point = D2D1::Point2F((FLOAT)m_pImpl->m_splitterPos, circlePos);
				}
				else
				{
					ellipse.point = D2D1::Point2F(circlePos, (FLOAT)m_pImpl->m_splitterPos);
				}
				pTarget->FillEllipse(ellipse, m_pImpl->m_brush);
				circlePos += step;
			}
		}
	}
}

D2D1_RECT_F Splitter::GetSplitterRect() const
{
	D2D1_RECT_F splitterRect;
	FLOAT splitpos = (FLOAT)m_pImpl->m_splitterPos;
	FLOAT step = GetSplitterWidth() / 2;

	if(GetOrientation() == Orientation::Horizontal)
	{
		splitterRect.bottom = SplitLength();
		splitterRect.top = 0;
		splitterRect.left = splitpos - step;
		splitterRect.right = splitpos + step;
	}
	else
	{
		splitterRect.bottom = splitpos + step;
		splitterRect.top = splitpos - step;;
		splitterRect.left = 0;
		splitterRect.right = SplitLength();
	}

	return splitterRect;
}

Object* Splitter::OnTouch(const D2D1_POINT_2F& pos)
{ 
	if(!GetMoveable())
		return nullptr;
	
	if(Intersects(GetSplitterRect(), pos))
	{
		return this;
	}
	return nullptr;
}

bool Splitter::OnTouchContinue(const TouchInfo& ti)
{ 
	tjm::animation::AllInstant ai(true);
	if(GetOrientation() == Orientation::Horizontal)
	{
		SetSplitterPos(ti.currentTouch.x);
	}
	else
	{
		SetSplitterPos(ti.currentTouch.y);
	}
	Layout();
	return true;
}

} // end namespace dash
} // end namespace tjm