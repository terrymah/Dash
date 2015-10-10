#include "DGui.h"

#include <dwrite.h>
#include "utils.h"
#include "AnimatedVar.h"

#include <algorithm>
#include <vector>
#include <memory>
#include <atlbase.h>

namespace tjm {
namespace dash {

InputManager::InputManager() :
m_root(nullptr)
{
	m_info.owner = nullptr;
}

void InputManager::SetRoot(Object* root)
{
	m_root = root;
}

void InputManager::SetFocus(Object* focus)
{
    m_focus = focus;
}

void InputManager::OnKey(char key)
{
    bool handled = false;

    if (m_focus)
        handled = m_focus->Key(key);

    if (!handled)
        handled = m_root->Key(key);
}

bool InputManager::StartTouch(const D2D1_POINT_2F& point)
{
    if (m_focus)
        m_info.owner = m_focus->Touch(point);

    if (!m_info.owner)
	    m_info.owner = m_root->Touch(point);
	m_info.originalTouch = point;
	m_info.currentTouch = point;

	return m_info.owner != nullptr;
}

bool InputManager::ContinueTouch(const D2D1_POINT_2F& point)
{
	if(!m_info.owner)
		return false;

	m_info.previousTouch = m_info.currentTouch;
	m_info.currentTouch = point;
	
	TouchInfo ownerLocal = TranslateToObjLocal(m_info.owner);
	if(!m_info.owner->TouchContinue(ownerLocal))
	{
		Object* potentialOwner = m_info.owner->GetParent();
		while(potentialOwner)
		{
			TouchInfo poLocal = TranslateToObjLocal(potentialOwner);
			if(potentialOwner->TouchContinue(poLocal))
			{
				m_info.owner = potentialOwner;
				return true;
			}
		}
		m_info.owner = nullptr;
		return false;
	}
	return true;
}

bool InputManager::EndTouch(const D2D1_POINT_2F&)
{
	if(!m_info.owner)
		return false;

	TouchInfo ownerLocal = TranslateToObjLocal(m_info.owner);
	m_info.owner->TouchFinish(ownerLocal);
	m_info.owner = nullptr;
	return true;
}

TouchInfo InputManager::TranslateToObjLocal(Object* obj)
{
	TouchInfo ti;
	ti.owner = m_info.owner;
	ti.currentTouch = obj->WorldToLocal(m_info.currentTouch);
	ti.originalTouch = obj->WorldToLocal(m_info.originalTouch);
	ti.previousTouch = obj->WorldToLocal(m_info.previousTouch);

	return ti;
}

struct ObjectImpl
{
	Object* m_parent;

	tjm::animation::AnimatedVar m_width;
	tjm::animation::AnimatedVar m_height;

	D2D1_RECT_F m_clippingRect;
	bool m_hasClippingRect;
	FLOAT m_leftMargin;
	FLOAT m_topMargin;
	FLOAT m_rightMargin;
	FLOAT m_bottomMargin;
    FLOAT m_maxWidth;
    FLOAT m_maxHeight;
	tjm::animation::AnimatedVar m_x;
	tjm::animation::AnimatedVar m_y;
	tjm::animation::AnimatedVar m_xTrans;
	tjm::animation::AnimatedVar m_yTrans;
	tjm::animation::AnimatedVar m_opacity;
	int m_z;

	bool m_zTrusted;
	
	bool m_dirtyLayout;
	bool m_dirtyChild;
	std::vector<Object*> m_children;

	ObjectImpl();
	void TrustZ();
};

ObjectImpl::ObjectImpl() :
m_parent(nullptr),
m_leftMargin(0),
m_topMargin(0),
m_rightMargin(0),
m_bottomMargin(0),
m_z(0),
m_maxWidth(0),
m_maxHeight(0),
m_zTrusted(false),
m_dirtyLayout(true),
m_dirtyChild(false),
m_hasClippingRect(false)
{
}

void ObjectImpl::TrustZ()
{
	if(!m_zTrusted)
	{	
		std::sort(m_children.begin(), m_children.end(), [](const Object* left, const Object* right) {
			return left->GetZOrder() < right->GetZOrder();
		});
		m_zTrusted = true;
	}
}

Object::Object() :
m_pImpl(new ObjectImpl)
{
}

Object* Object::GetParent() const
{
	return m_pImpl->m_parent;
}

Object* Object::SetParent(Object* parent)
{
	Object* oldParent = GetParent();
	m_pImpl->m_parent = parent;
	return oldParent;
}

void Object::DirtyParentLayout()
{
	if(GetParent())
	{
		GetParent()->DirtyLayout();
	}
}

void Object::SetMargins(FLOAT left, FLOAT top, FLOAT right, FLOAT bottom)
{
	m_pImpl->m_leftMargin = left;
	m_pImpl->m_topMargin = top;
	m_pImpl->m_rightMargin = right;
	m_pImpl->m_bottomMargin = bottom;
	DirtyParentLayout();
}

void Object::SetMarginLeft(FLOAT margin)
{
	m_pImpl->m_leftMargin = margin;
	DirtyParentLayout();
}

void Object::SetMarginTop(FLOAT margin)
{
	m_pImpl->m_topMargin = margin;
	DirtyParentLayout();
}

void Object::SetMarginRight(FLOAT margin)
{
	m_pImpl->m_rightMargin = margin;
	DirtyParentLayout();
}

void Object::SetMarginBottom(FLOAT margin)
{
	m_pImpl->m_bottomMargin = margin;
	DirtyParentLayout();
}

void Object::DirtyParentZ()
{
	if(GetParent())
	{
		GetParent()->DirtyZ();
	}
}

void Object::GetMargins(FLOAT& left, FLOAT& top, FLOAT& right, FLOAT& bottom) const
{
	left = m_pImpl->m_leftMargin;
	top = m_pImpl->m_topMargin;	
	right = m_pImpl->m_rightMargin;
	bottom = m_pImpl->m_bottomMargin;
}

FLOAT Object::GetMarginLeft() const
{
	return m_pImpl->m_leftMargin;
}

FLOAT Object::GetMarginTop() const
{
	return m_pImpl->m_topMargin;
}

FLOAT Object::GetMarginRight() const
{
	return m_pImpl->m_rightMargin;
}

FLOAT Object::GetMarginBottom() const
{
	return m_pImpl->m_bottomMargin;
}

Object::~Object()
{
	delete m_pImpl;
}

void Object::DirtyLayout()
{
	m_pImpl->m_dirtyLayout = true;
	if(GetParent())
	{
		GetParent()->SetDirtyChildLayout();
	}
}

void Object::SetDirtyChildLayout()
{
	if(!m_pImpl->m_dirtyChild)
	{
		m_pImpl->m_dirtyChild = true;
		if(GetParent())
		{
			GetParent()->SetDirtyChildLayout();
		}
	}
}

void Object::DirtyZ()
{
	m_pImpl->m_zTrusted = false;
}

void Object::SetSize(D2D1_SIZE_F newSize)
{
	if (m_pImpl->m_height != newSize.height || m_pImpl->m_width != newSize.width)
	{
		tjm::animation::StoryBoard b;
		tjm::animation::InstantChange ic(m_pImpl->m_height, !GetVisible());
		tjm::animation::InstantChange ic2(m_pImpl->m_width, !GetVisible());
		m_pImpl->m_height = newSize.height;
		m_pImpl->m_width = newSize.width;
		DirtyLayout();
	}
}

bool Object::GetVisible() const
{
	return m_pImpl->m_opacity.GetFinalValue() > 0.0f;
}

DOUBLE Object::GetOpacity() const
{
	return m_pImpl->m_opacity;
}

void Object::SetVisible(bool visible)
{
	if(GetVisible() != visible)
	{
		m_pImpl->m_opacity = visible ? 1.0 : 0.0;
		OnVisibilityChange(visible);
	}
}

void Object::SetOpacity(DOUBLE opacity)
{
	bool oldVisibility = GetVisible();

	m_pImpl->m_opacity = opacity;

	if(GetVisible() != oldVisibility)
	{
		OnVisibilityChange(GetVisible());
	}
}

size_t Object::NumChildren() const
{
	return m_pImpl->m_children.size();
}

Object* Object::GetChild(size_t i)
{
	return m_pImpl->m_children[i];
}

const Object* Object::GetChild(size_t i) const
{
	return m_pImpl->m_children[i];
}

void Object::AddChild(Object* child)
{
	child->SetParent(this);
	m_pImpl->m_children.push_back(child);
	child->DirtyLayout();
	DirtyLayout();
	DirtyZ();
}

void Object::InsertChild(Object * child, size_t i)
{
    child->SetParent(this);
    m_pImpl->m_children.insert(m_pImpl->m_children.begin()+i, child);
    child->DirtyLayout();
    DirtyLayout();
    DirtyZ();
}

void Object::RemoveChild(Object* child)
{
	if(child)
	{
		child->SetParent(nullptr);
		std::vector<Object*>& v = m_pImpl->m_children;
		v.erase(std::remove(v.begin(), v.end(), child), v.end());
		DirtyLayout();
	}
}
	
D2D1_SIZE_F Object::GetSize() const 
{ 
	return D2D1::SizeF((FLOAT)m_pImpl->m_width, (FLOAT)m_pImpl->m_height); 
}

D2D1_SIZE_F Object::GetFinalSize() const 
{ 
	return D2D1::SizeF((FLOAT)m_pImpl->m_width.GetFinalValue(), (FLOAT)m_pImpl->m_height.GetFinalValue()); 
}

void Object::SetPosition(D2D1_POINT_2F newPos)
{
	tjm::animation::StoryBoard b;
	tjm::animation::InstantChange ic(m_pImpl->m_x, !GetVisible());
	tjm::animation::InstantChange ic2(m_pImpl->m_y, !GetVisible());
	m_pImpl->m_x = newPos.x;
	m_pImpl->m_y = newPos.y;
}

D2D1_POINT_2F Object::GetPosition() const 
{ 
	return D2D1::Point2F((FLOAT)m_pImpl->m_x, (FLOAT)m_pImpl->m_y); 
}

D2D1_POINT_2F Object::GetFinalPosition() const 
{ 
	return D2D1::Point2F((FLOAT)m_pImpl->m_x, (FLOAT)m_pImpl->m_y); 
}

void Object::SetZOrder(int z) 
{ 
	if(GetZOrder() != z)
	{
		m_pImpl->m_z = z; 
		DirtyParentZ();
	}
}

int Object::GetZOrder() const 
{ 
	return m_pImpl->m_z; 
}

void Object::SetTranslationX(double newX)
{
	m_pImpl->m_xTrans = newX;
}

void Object::SetTranslationY(double newY)
{
	m_pImpl->m_yTrans = newY;
}

void Object::SetTranslationXDelta(double xdelta)
{
	m_pImpl->m_xTrans = m_pImpl->m_xTrans.GetFinalValue() + xdelta;
}

void Object::SetTranslationYDelta(double ydelta)
{
	m_pImpl->m_yTrans = m_pImpl->m_yTrans.GetFinalValue() + ydelta;
}

D2D1_RECT_F Object::GetBoundingBox() const
{
	return D2D1::RectF(	(FLOAT)m_pImpl->m_x, 
						(FLOAT)m_pImpl->m_y, 
						(FLOAT)(m_pImpl->m_x + m_pImpl->m_width),
						(FLOAT)(m_pImpl->m_y + m_pImpl->m_height)
						);
}

void Object::Layout()
{
	if(m_pImpl->m_dirtyLayout)
	{
		OnLayout();
		m_pImpl->m_dirtyLayout = false;
	}

	if(m_pImpl->m_dirtyChild)
	{
		std::vector<Object*>& v = m_pImpl->m_children;
		for(auto& obj : v)
		{
			obj->Layout();
		}
		m_pImpl->m_dirtyChild = false;
	}
}

void Object::OnLayout()
{
    std::vector<Object*>& v = m_pImpl->m_children;
    for (auto& obj : v)
    {
        obj->SetPosition({ 0,0 });
        obj->SetSize(GetSize());
    }
}

void Object::Render(ID2D1RenderTarget* pTarget, const D2D1_RECT_F& box, DOUBLE baseOpacity)
{
	DOUBLE effectiveOpacity = GetOpacity() * baseOpacity;

	if(HasClippingRect())
		pTarget->PushAxisAlignedClip(GetClippingRect(),	D2D1_ANTIALIAS_MODE_ALIASED);

	// Opacity culling
	if(effectiveOpacity < 0.001)
		return;

	D2D1::Matrix3x2F preTrans;
	pTarget->GetTransform(&preTrans);

	// Set local translation
	pTarget->SetTransform(preTrans * D2D1::Matrix3x2F::Translation((FLOAT)m_pImpl->m_xTrans, (FLOAT)m_pImpl->m_yTrans));

	OnRenderBackground(pTarget, box, effectiveOpacity);
	
	m_pImpl->TrustZ();

	D2D1::Matrix3x2F postTrans;
	pTarget->GetTransform(&postTrans);

	for(auto& obj : m_pImpl->m_children)
	{
		if(Intersects(obj, box)) 
		{
			D2D1_RECT_F transBox(box);
			transBox.left -= obj->GetPosition().x;
			transBox.right -= obj->GetPosition().x;
			transBox.bottom -= obj->GetPosition().y;
			transBox.top -= obj->GetPosition().y;

			pTarget->SetTransform(postTrans * D2D1::Matrix3x2F::Translation(obj->GetPosition().x, obj->GetPosition().y));
			obj->Render(pTarget, transBox, effectiveOpacity);
		}
	}

	pTarget->SetTransform(postTrans);

	OnRenderForeground(pTarget, box, effectiveOpacity);

	pTarget->SetTransform(preTrans);

	if(HasClippingRect())
		pTarget->PopAxisAlignedClip();
}

D2D1_POINT_2F Object::WorldToLocal(const D2D1_POINT_2F& world) const
{
	if(!GetParent())
		return world;

	D2D1_POINT_2F pt = GetParent()->WorldToLocal(world);
	pt.x -= GetPosition().x;
	pt.y -= GetPosition().y;

	return pt;
}

Object* Object::Touch(const D2D1_POINT_2F& pos)
{
	m_pImpl->TrustZ();

	for(auto obj = m_pImpl->m_children.rbegin(); obj != m_pImpl->m_children.rend(); ++obj)
	{
		if(Intersects(*obj, pos))
		{
			D2D1_POINT_2F transPos(pos);
			transPos.x -= pos.x;
			transPos.y -= pos.y;
			Object* owner = (*obj)->Touch(transPos);
			if(owner)
				return owner;
		}
	}

	return OnTouch(pos);
}

bool Object::Key(char key)
{
    m_pImpl->TrustZ();

    if (OnKey(key))
        return true;

    for (auto obj = m_pImpl->m_children.rbegin(); obj != m_pImpl->m_children.rend(); ++obj)
    {
        if ((*obj)->Key(key))
            return true;
    }

    return false;
}

bool Object::TouchContinue(const TouchInfo& ti)
{
	return OnTouchContinue(ti);
}

void Object::TouchFinish(const TouchInfo& ti)
{
	return OnTouchFinish(ti);
}

void Object::SetClippingRect(const D2D1_RECT_F& rect)
{
	m_pImpl->m_clippingRect = rect;
	m_pImpl->m_hasClippingRect = true;
}

void Object::ClearClippingRect()
{
	m_pImpl->m_hasClippingRect = false;
}

bool Object::HasClippingRect()
{
	return m_pImpl->m_hasClippingRect;
}

D2D1_RECT_F Object::GetClippingRect() const
{
	return m_pImpl->m_clippingRect;
}

bool Intersects(const Object* obj, const D2D1_POINT_2F& point)
{
	return Intersects(obj->GetBoundingBox(), point);
}

bool Intersects(const Object* obj, const D2D1_RECT_F& rect)
{
	return Intersects(obj->GetBoundingBox(), rect);
}

bool Intersects(const D2D1_RECT_F& rect, const D2D1_POINT_2F& point)
{
	return point.x > rect.left && point.x < rect.right &&
		point.y > rect.top && point.y < rect.bottom;
}

bool Intersects(const D2D1_RECT_F& rect, const D2D1_RECT_F& other)
{
	return !(other.bottom < rect.top) &&
		!(other.top > rect.bottom) &&
		!(other.left > rect.right) &&
		!(other.right < rect.left);
}

void Clip(D2D1_RECT_F& in, const D2D1_RECT_F& clippingRect)
{
	if(in.right > clippingRect.right)
		in.right = clippingRect.right;
	if(in.bottom > clippingRect.bottom)
		in.bottom = clippingRect.bottom;
	if(in.left < clippingRect.left)
		in.left = clippingRect.left;
	if(in.top < clippingRect.top)
		in.top = clippingRect.top;
}

TestObject::TestObject(D2D1_COLOR_F color) :
m_color(color)
{
}

D2D1_SIZE_F TestObject::GetPreferredSize(D2D1_SIZE_F& max)
{
	D2D1_SIZE_F size;
	size.height = min(max.height, max.width);
	size.width = size.height;
	return size;
}

void TestObject::OnRenderBackground(ID2D1RenderTarget* pTarget, const D2D1_RECT_F&, DOUBLE effectiveOpacity)
{
	CComPtr<ID2D1SolidColorBrush> brush;
	D2D1_RECT_F render;
	render.left = render.top = 0;
	render.right = GetSize().width;
	render.bottom = GetSize().height;
	D2D1_BRUSH_PROPERTIES bp;
	bp.opacity = (FLOAT)effectiveOpacity;
	bp.transform = D2D1::Matrix3x2F();
	pTarget->CreateSolidColorBrush(m_color, bp, &brush);
	pTarget->FillRectangle(render, brush);
}

Object* PannableObject::OnTouch(const D2D1_POINT_2F&)
{
	return this;
}

bool PannableObject::OnTouchContinue(const TouchInfo& ti)
{
	tjm::animation::AllInstant ai(true);
	SetTranslationXDelta(ti.currentTouch.x - ti.previousTouch.x);
	SetTranslationYDelta(ti.currentTouch.y - ti.previousTouch.y);
	return true;
}

struct TextLabelImpl
{
    std::string m_text;
    std::string m_font;
    FLOAT m_size;
    D2D1_SIZE_F m_max;

    CComPtr<IDWriteFactory> m_factory;
    CComPtr<IDWriteTextFormat> m_format;
    CComPtr<IDWriteTextLayout> m_layout;

    void EnsureFormat();
    void EnsureLayout();

    TextLabelImpl();
    TextLabelImpl(const std::string & text, const std::string & font, FLOAT size);
};

void TextLabelImpl::EnsureFormat()
{
    if (!m_format) {
        CORt(m_factory->CreateTextFormat(towide(m_font).c_str(), nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, m_size, L"", &m_format));
    }
}

void TextLabelImpl::EnsureLayout()
{
    EnsureFormat();

    if (!m_layout) {
        CORt(m_factory->CreateTextLayout(towide(m_text).c_str(), m_text.length(), m_format, m_max.width, m_max.height, &m_layout));
    }
}

TextLabelImpl::TextLabelImpl() :
    m_font("Ariel"),
    m_size(17.0),
    m_max{ 10000,10000 }
{
    CORt(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&m_factory)));
}

TextLabelImpl::TextLabelImpl(const std::string& text, const std::string& font, FLOAT size) :
    m_text(text),
    m_font(font),
    m_size(size),
    m_max{ 10000,10000 }
{
    CORt(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&m_factory)));
}

TextLabel::TextLabel() :
    m_pImpl(new TextLabelImpl())
{
}

TextLabel::~TextLabel()
{
    delete m_pImpl;
}

TextLabel::TextLabel(const std::string& text, const std::string& font, FLOAT size) :
    m_pImpl(new TextLabelImpl(text, font, size))
{
}

void TextLabel::SetText(const std::string& text)
{
    m_pImpl->m_text = text;
    m_pImpl->m_layout.Release();
}

void TextLabel::SetFont(const std::string& font)
{
    m_pImpl->m_font = font;
    m_pImpl->m_format.Release();
}

void TextLabel::SetSize(FLOAT size)
{
    m_pImpl->m_size = size;
    m_pImpl->m_format.Release();
}

void TextLabel::OnRenderForeground(ID2D1RenderTarget * pTarget, const D2D1_RECT_F & /*rect*/, DOUBLE /* opacity */)
{
    if ((GetSize().height != m_pImpl->m_max.height) || (GetSize().width != m_pImpl->m_max.width))
    {
        m_pImpl->m_max = GetSize();
        m_pImpl->m_layout.Release();
        m_pImpl->EnsureLayout();
    }
    CComPtr<ID2D1SolidColorBrush> brush;
    pTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &brush);
    pTarget->DrawTextLayout({ 0,0 }, m_pImpl->m_layout, brush);
}

D2D1_SIZE_F TextLabel::GetPreferredSize(D2D1_SIZE_F & max)
{
    if ((max.height != m_pImpl->m_max.height) || (max.width != m_pImpl->m_max.width))
    {
        m_pImpl->m_max = max;
        m_pImpl->m_layout.Release();
        m_pImpl->EnsureLayout();
    }
    
    DWRITE_TEXT_METRICS metrics;
    CORt(m_pImpl->m_layout->GetMetrics(&metrics));
    D2D1_SIZE_F preferred;
    preferred.height = metrics.height;
    preferred.width = metrics.width;

    return preferred;
}

struct ListViewImpl
{
    Orientation m_orientation;
    Direction m_direction;
};

ListView::ListView() :
    m_pImpl(new ListViewImpl())
{
    m_pImpl->m_orientation = Orientation::Horizontal;
}

ListView::~ListView()
{
    delete m_pImpl;
}

void ListView::SetOrientation(Orientation o)
{
    if (m_pImpl->m_orientation != o)
    {
        m_pImpl->m_orientation = o;
        DirtyLayout();
    }
}

Orientation ListView::GetOrientation() const
{
    return m_pImpl->m_orientation;
}

void ListView::SetDirection(Direction d)
{
    if (m_pImpl->m_direction != d)
    {
        m_pImpl->m_direction = d;
        DirtyLayout();
    }
}

Direction ListView::GetDirection() const
{
    return m_pImpl->m_direction;
}

void ListView::OnLayout()
{
    D2D1_SIZE_F maxSize = GetFinalSize();

    FLOAT offset = 0;
    for (size_t i = 0; i < NumChildren(); ++i) {
        Object* child = GetChild(i);
        FLOAT left, right, top, bottom;
        child->GetMargins(left, top, right, bottom);

        D2D1_SIZE_F localMaxSize = maxSize;
        localMaxSize.height -= (top + bottom);
        localMaxSize.width -= (left + right);

        D2D1_SIZE_F preferredSize = child->GetPreferredSize(localMaxSize);
        
        if (GetOrientation() == Orientation::Vertical) {
            if (GetDirection() == Direction::TopDown) {
                offset += top;
                child->SetPosition(D2D1::Point2F(left, offset));
                child->SetSize(preferredSize);
                offset += preferredSize.height;
                offset += bottom;
            }
            else {
                offset += bottom;
                offset += preferredSize.height;
                child->SetPosition(D2D1::Point2F(left, maxSize.height-offset));
                child->SetSize(preferredSize);
                offset += top;
            }
        }
        else {
            if (GetDirection() == Direction::LeftRight) {
                offset += left;
                child->SetPosition(D2D1::Point2F(offset, top));
                child->SetSize(preferredSize);
                offset += preferredSize.width;
                offset += right;
            }
            else {
                offset += right;
                offset += preferredSize.width;
                child->SetPosition(D2D1::Point2F(offset, top));
                child->SetSize(preferredSize);
                offset += left;
            }
        }
    }

    for (size_t i = 0; i < NumChildren(); ++i) {
        GetChild(i)->SetVisible(true);
    }
}

} // end namespace dash
} // end namespace tjm