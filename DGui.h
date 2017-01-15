#ifndef DGUI_H
#define DGUI_H

#ifdef UI_EXPORTS
#define DUI_API __declspec(dllexport)
#else
#define DUI_API __declspec(dllimport)
#endif

#include <d2d1.h>
#include <string>
#include <functional>

namespace tjm {
namespace dash {

enum class Orientation
{
	Horizontal,
	Vertical
};

enum class Direction
{
    TopDown,
    LeftRight = TopDown,
    BottomUp,
    RightLeft = BottomUp
};

class Object;
struct TouchInfo
{
	Object* owner;
	D2D1_POINT_2F originalTouch;
	D2D1_POINT_2F previousTouch;
	D2D1_POINT_2F currentTouch;
};

class DUI_API InputManager
{
public:
	InputManager();
    void OnKey(char key);
	void SetRoot(Object* root);
    void SetFocus(Object* focus);
	bool StartTouch(const D2D1_POINT_2F& point);
	bool ContinueTouch(const D2D1_POINT_2F& point);
	bool EndTouch(const D2D1_POINT_2F& point);

private:
	TouchInfo TranslateToObjLocal(Object* obj);
	Object* m_root;
    Object* m_focus;
	TouchInfo m_info;
};

struct ObjectImpl;
class DUI_API Object
{
public:
	Object();
	virtual ~Object();

	// Family
	Object* GetParent() const;
	Object* SetParent(Object* parent);
	size_t NumChildren() const;
	Object* GetChild(size_t i);
	const Object* GetChild(size_t i) const;
	void AddChild(Object* child);
    void InsertChild(Object* child, size_t i);
	void RemoveChild(Object* child);

	void Render(ID2D1RenderTarget* pTarget, const D2D1_RECT_F& box, DOUBLE opacity=1.0);
	void Layout();

	void DirtyLayout();
	void DirtyParentLayout();
	void SetDirtyChildLayout();
	void DirtyZ();
	void DirtyParentZ();

	void SetSize(D2D1_SIZE_F newSize);
	D2D1_SIZE_F GetSize() const;
	D2D1_SIZE_F GetFinalSize() const;

	void SetMargins(FLOAT left, FLOAT top, FLOAT right, FLOAT bottom);
	void SetMarginLeft(FLOAT margin);
	void SetMarginTop(FLOAT margin);
	void SetMarginRight(FLOAT margin);
	void SetMarginBottom(FLOAT margin);

	void GetMargins(FLOAT& left, FLOAT& top, FLOAT& right, FLOAT& bottom) const;
	FLOAT GetMarginLeft() const;
	FLOAT GetMarginTop() const;
	FLOAT GetMarginRight() const;
	FLOAT GetMarginBottom() const;

	// Position changes animate
	void SetPosition(D2D1_POINT_2F newPos);
	D2D1_POINT_2F GetPosition() const;
	D2D1_POINT_2F GetFinalPosition() const;
	void SetZOrder(int z);
	int GetZOrder() const;

	// Internal translation
	void SetTranslationX(double newX);
	void SetTranslationY(double newY);
	void SetTranslationXDelta(double xdelta);
	void SetTranslationYDelta(double ydelta);

	void SetClippingRect(const D2D1_RECT_F& rect);
	void ClearClippingRect();
	bool HasClippingRect();
	D2D1_RECT_F GetClippingRect() const;

	// Visibility animates opacity
	bool GetVisible() const;
	DOUBLE GetOpacity() const;
	void SetVisible(bool visible);
	void SetOpacity(DOUBLE opacity);

	D2D1_RECT_F GetBoundingBox() const;

	// Input Handling
	D2D1_POINT_2F WorldToLocal(const D2D1_POINT_2F& world) const;
	Object* Touch(const D2D1_POINT_2F& pos);
    bool Key(char key);
	bool TouchContinue(const TouchInfo& ti);
	void TouchFinish(const TouchInfo& ti);

	// Optional overrides
	virtual D2D1_SIZE_F GetPreferredSize(D2D1_SIZE_F& max) { return max; }

protected:
	// Optional overrides
	virtual void OnRenderBackground(ID2D1RenderTarget*, const D2D1_RECT_F& /*box*/, DOUBLE /*effectiveOpacity*/) { }
	virtual void OnRenderForeground(ID2D1RenderTarget*, const D2D1_RECT_F& /*box*/, DOUBLE /*effectiveOpacity*/) { }
	virtual void OnVisibilityChange(bool /* visible */) { }
    virtual void OnLayout();
	virtual Object* OnTouch(const D2D1_POINT_2F& /*pos*/) { return nullptr; }
    virtual bool OnKey(char /*key*/) { return false; }
	virtual bool OnTouchContinue(const TouchInfo& /*ti*/) { return false; }
	virtual void OnTouchFinish(const TouchInfo& /*ti*/) { }

private:
	ObjectImpl* m_pImpl;
};

class DUI_API PannableObject : public Object
{
private:
	virtual Object* OnTouch(const D2D1_POINT_2F& pos);
	virtual bool OnTouchContinue(const TouchInfo& ti);	
};

DUI_API bool Intersects(const Object* obj, const D2D1_POINT_2F& point);
DUI_API bool Intersects(const Object* obj, const D2D1_RECT_F& rect);
DUI_API bool Intersects(const D2D1_RECT_F& rect, const D2D1_POINT_2F& point);
DUI_API bool Intersects(const D2D1_RECT_F& rect, const D2D1_RECT_F& other);
DUI_API void Clip(D2D1_RECT_F& in, const D2D1_RECT_F& clippingRect);

enum class SplitLayoutType
{
	Variable,
	Fixed,
	Overlay
};

enum class SplitterStyle
{
	None,
	Line,
	Box,
	Dots
};

struct SplitterImpl;
class DUI_API Splitter : public Object
{
public:
	Splitter();
	virtual ~Splitter();

	void SetOrientation(Orientation o);
	Orientation GetOrientation() const;

	void SetStyle(SplitterStyle s);
	SplitterStyle GetStyle() const;
	void SetColor(D2D1::ColorF color);
	D2D1::ColorF GetColor() const;

	void SetMoveable(bool moveable);
	bool GetMoveable() const;

	void SetLeftTop(Object* obj, SplitLayoutType layout=SplitLayoutType::Variable);
	Object* GetLeftTop() const;
	void SetLeftTopLayoutType(SplitLayoutType layout);
	SplitLayoutType GetLeftTopLayoutType() const;

	void SetRightBottom(Object* obj, SplitLayoutType layout=SplitLayoutType::Variable);
	Object* GetRightBottom() const;
	void SetRightBottomLayoutType(SplitLayoutType layout);
	SplitLayoutType GetRightBottomLayoutType() const;

	void SetSplitterWidth(FLOAT width);
	FLOAT GetSplitterWidth() const;
	void SetSplitterMin(DOUBLE min);
	void SetSplitterMinPercent(DOUBLE minPercent);
	void SetSplitterMax(DOUBLE max);
	void SetSplitterMaxPercent(DOUBLE maxPercent);

	void SetSplitterPos(DOUBLE pos);
	void SetSplitterPosPercent(DOUBLE posPercent);
	DOUBLE GetSplitterPos() const;
	DOUBLE GetSplitterPosFinal() const;

	bool IsCollapsed() const;
	void Collapse(bool bottomLeft);
	void Restore();
	
	D2D1_SIZE_F GetPreferredSize(D2D1_SIZE_F& max);

private:
	virtual void OnRenderForeground(ID2D1RenderTarget*, const D2D1_RECT_F& /*box*/, DOUBLE /*effectiveOpacity*/);

	FLOAT SplitLength() const;
	FLOAT SplitHeight() const;
	D2D1_RECT_F GetSplitterRect() const;
	void SetBounds();
	virtual void OnLayout();
	virtual Object* OnTouch(const D2D1_POINT_2F& pos);
	virtual bool OnTouchContinue(const TouchInfo& ti);

	SplitterImpl* m_pImpl;
};

struct TextLabelImpl;
class DUI_API TextLabel : public Object
{
public:
    TextLabel();
    TextLabel(const std::string& text, const std::string& font, FLOAT size);
    ~TextLabel();

    void SetText(const std::string& text);
    void SetFont(const std::string& font);
    void SetSize(FLOAT size);

private:
    virtual void OnRenderForeground(ID2D1RenderTarget*, const D2D1_RECT_F& /*box*/, DOUBLE /*effectiveOpacity*/);
    virtual D2D1_SIZE_F GetPreferredSize(D2D1_SIZE_F& max);

    TextLabelImpl* m_pImpl;
};

struct ListViewImpl;
class DUI_API ListView : public Object
{
public:
    ListView();
    ~ListView();

    void SetOrientation(Orientation o);
    Orientation GetOrientation() const;

    void SetDirection(Direction d);
    Direction GetDirection() const;

private:
    virtual void OnLayout();

    ListViewImpl* m_pImpl;
};

struct DebugConsoleImpl;
class DUI_API DebugConsole : public Object
{
public:
    DebugConsole();
    ~DebugConsole();

    void Log(const std::string text);

private:
    //void OnVisibilityChange(bool visible);

    DebugConsoleImpl* m_pImpl;
};

class DUI_API SolidObject : public Object
{
public:
    SolidObject(D2D1_COLOR_F color);

	virtual D2D1_SIZE_F GetPreferredSize(D2D1_SIZE_F& max);
private:
	virtual void OnRenderBackground(ID2D1RenderTarget*, const D2D1_RECT_F& /*box*/, DOUBLE /*effectiveOpacity*/);

	D2D1_COLOR_F m_color;
};

class DashApplication;
class ApplicationCore
{
public:
	virtual void InitializeApplication(DashApplication* app) = 0;
	virtual void DestroyApplication(DashApplication* app) = 0;

	virtual void PreRender(DashApplication* /*app*/) {}
	virtual void PostRender(DashApplication* /*app*/) {}
};

struct DashApplicationImpl;
class DUI_API DashApplication
{
public:
	DashApplication();

    // Run with a sample application core
    void Run();
    void Run(ApplicationCore* core);

    void Refresh();

	// ApplicationCore::InitializeApplication should call SetRoot
	void SetRoot(Object* root);
    Object* GetRoot() const;

    // Focus always gets first change at input processing, then root
    void SetFocus(Object* focus);
    Object* GetFocus() const;

    void OnMainThread(std::function<void()> func);
private:
	HRESULT CreateDeviceIndependentResources();
	HRESULT CreateDeviceResources();
    
	void OnRender();
	void OnResize(UINT width, UINT height);

	static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	DashApplicationImpl* m_pImpl;
};

} // end namespace dash
} // end namespace tjm

#endif