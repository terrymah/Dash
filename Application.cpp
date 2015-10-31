#include "DGui.h"
#include "AnimatedVar.h"
#include "utils.h"
#include "windows.h"
#include "Windowsx.h"
#include <exception>

#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
#endif

namespace tjm {
namespace dash {

struct DashApplicationImpl;
class ChangeHandler : public tjm::animation::AnimationClient
{
public:
	ChangeHandler(DashApplicationImpl* appImpl) : m_appImpl(appImpl) {}
	virtual void OnChange();

private:
	DashApplicationImpl* m_appImpl;
};

struct DashApplicationImpl
{
	ApplicationCore* m_core;
	DashApplication* m_app;
	
	ChangeHandler m_ch;
	tjm::animation::AnimationLibrary m_animLibrary;
	tjm::dash::InputManager m_inputManager;

	Object* m_root;
    Object* m_focus;
	HWND m_hwnd;
	ID2D1Factory* m_pDirect2dFactory;
	ID2D1HwndRenderTarget* m_pRenderTarget;

	DashApplicationImpl(DashApplication* app);
};

void ChangeHandler::OnChange()
{
	::InvalidateRect(m_appImpl->m_hwnd, nullptr, FALSE);
}

class SampleApplicationCore : public ApplicationCore
{
public:
	SampleApplicationCore();
	virtual void InitializeApplication(DashApplication* app);
	virtual void DestroyApplication(DashApplication* app);

private:
    SolidObject m_left;
    ListView m_list;
    SolidObject m_right;
    TextLabel m_labelLeft;
    TextLabel m_labelLeft2;
    TextLabel m_labelLeft3;
    TextLabel m_labelRight;
	Splitter m_split;
};


DashApplicationImpl::DashApplicationImpl(DashApplication* app) :
m_app(app),
m_core(nullptr),
m_root(nullptr),
m_ch(this),
m_animLibrary(&m_ch),
m_pRenderTarget(nullptr)
{
}

SampleApplicationCore::SampleApplicationCore() :
m_left(D2D1::ColorF(D2D1::ColorF::Aqua)),
m_right(D2D1::ColorF(D2D1::ColorF::OrangeRed))
{
}

void SampleApplicationCore::InitializeApplication(DashApplication * app)
{
    m_labelLeft.SetText("Left");
    m_labelLeft2.SetText("Left2");
    m_labelLeft3.SetText("Left3");
    m_list.AddChild(&m_labelLeft);
    m_list.AddChild(&m_labelLeft2);
    m_list.AddChild(&m_labelLeft3);
    m_list.SetOrientation(Orientation::Vertical);

    m_labelRight.SetText("Right");

	m_split.SetLeftTop(&m_left);
	m_split.SetRightBottom(&m_right);

    m_list.SetVisible(true);
	m_split.SetVisible(true);
	m_left.SetVisible(true);
    m_left.AddChild(&m_list);
    m_labelLeft.SetVisible(true);
	m_right.SetVisible(true);
    m_right.AddChild(&m_labelRight);
    m_labelRight.SetVisible(true);

	app->SetRoot(&m_split);
}

void SampleApplicationCore::DestroyApplication(DashApplication * /*app*/)
{
}

DashApplication::DashApplication()
{
	CoInitialize(nullptr);
	m_pImpl = new DashApplicationImpl(this);
}

HRESULT DashApplication::CreateDeviceResources()
{
	HRESULT hr = S_OK;

	if (!m_pImpl->m_pRenderTarget)
	{
		RECT rc;
		GetClientRect(m_pImpl->m_hwnd, &rc);

		D2D1_SIZE_U size = D2D1::SizeU(
			rc.right - rc.left,
			rc.bottom - rc.top
			);

		// Create a Direct2D render target.
		hr = m_pImpl->m_pDirect2dFactory->CreateHwndRenderTarget(
			D2D1::RenderTargetProperties(),
			D2D1::HwndRenderTargetProperties(m_pImpl->m_hwnd, size),
			&m_pImpl->m_pRenderTarget
			);
	}

	return hr;
}

void DashApplication::OnRender()
{
	m_pImpl->m_core->PreRender(this);

	CORt(CreateDeviceResources());

	m_pImpl->m_pRenderTarget->BeginDraw();
	m_pImpl->m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
	m_pImpl->m_pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

	D2D1_SIZE_F rtSize = m_pImpl->m_pRenderTarget->GetSize();

	bool forceResize = m_pImpl->m_root->GetSize().height != rtSize.height || m_pImpl->m_root->GetSize().width != rtSize.width;
	tjm::animation::AllInstant ai(forceResize);
	m_pImpl->m_root->SetSize(rtSize);
	m_pImpl->m_root->Layout();

	m_pImpl->m_root->Render(m_pImpl->m_pRenderTarget, m_pImpl->m_root->GetBoundingBox());

	CORt(m_pImpl->m_pRenderTarget->EndDraw());

	m_pImpl->m_core->PostRender(this);
}

void DashApplication::OnResize(UINT width, UINT height)
{
	CORt(CreateDeviceResources());

	// Note: This method can fail, but it's okay to ignore the
	// error here, because the error will be returned again
	// the next time EndDraw is called.
	m_pImpl->m_pRenderTarget->Resize(D2D1::SizeU(width, height));
	tjm::animation::AllInstant ai(true);
	m_pImpl->m_root->SetSize(D2D1::SizeF((FLOAT)width, (FLOAT)height));
	m_pImpl->m_root->Layout();
}

// The windows procedure.
LRESULT CALLBACK DashApplication::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
	DWORD xPos, yPos;
	if (message == WM_CREATE)
	{
		LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
		DashApplication *pDemoApp = (DashApplication *)pcs->lpCreateParams;
		::SetWindowLongPtrW(hwnd, GWLP_USERDATA, PtrToUlong(pDemoApp));
		result = 1;
	}
	else
	{
		DashApplication *pDemoApp = reinterpret_cast<DashApplication *>(static_cast<LONG_PTR>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA)));
		bool wasHandled = false;

		if (pDemoApp)
		{
			switch (message)
			{
            case WM_CHAR:
                pDemoApp->m_pImpl->m_inputManager.OnKey((char)wParam);
                result = 0;
                wasHandled = true;
                break;

			case WM_SIZE:
			{
				UINT width = LOWORD(lParam);
				UINT height = HIWORD(lParam);
				pDemoApp->OnResize(width, height);
			}
			result = 0;
			wasHandled = true;
			break;

			case WM_DISPLAYCHANGE:
			{
				InvalidateRect(hwnd, NULL, FALSE);
			}
			result = 0;
			wasHandled = true;
			break;

			case WM_PAINT:
			{
				pDemoApp->OnRender();
				ValidateRect(hwnd, NULL);
			}
			result = 0;
			wasHandled = true;
			break;

			case WM_DESTROY:
			{
				PostQuitMessage(0);
			}
			result = 1;
			wasHandled = true;
			break;

			case WM_LBUTTONDOWN:
				xPos = GET_X_LPARAM(lParam);
				yPos = GET_Y_LPARAM(lParam);
				result = 0;
				wasHandled = pDemoApp->m_pImpl->m_inputManager.StartTouch(D2D1::Point2F((FLOAT)xPos, (FLOAT)yPos));
				break;

			case WM_LBUTTONUP:
				xPos = GET_X_LPARAM(lParam);
				yPos = GET_Y_LPARAM(lParam);
				result = 0;
				wasHandled = pDemoApp->m_pImpl->m_inputManager.EndTouch(D2D1::Point2F((FLOAT)xPos, (FLOAT)yPos));
				break;

			case WM_MOUSEMOVE:
				if (wParam & MK_LBUTTON)
				{
					xPos = GET_X_LPARAM(lParam);
					yPos = GET_Y_LPARAM(lParam);
					result = 0;
					wasHandled = pDemoApp->m_pImpl->m_inputManager.ContinueTouch(D2D1::Point2F((FLOAT)xPos, (FLOAT)yPos));
				}
				break;
			}
		}

		if (!wasHandled)
		{
			result = DefWindowProc(hwnd, message, wParam, lParam);
		}
	}

	return result;
}

// Creates resources that are not bound to a particular device.
// Their lifetime effectively extends for the duration of the
// application.
HRESULT DashApplication::CreateDeviceIndependentResources()
{
	HRESULT hr = S_OK;

	// Create a Direct2D factory.
	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pImpl->m_pDirect2dFactory);

	return hr;
}

void DashApplication::Refresh()
{
    m_pImpl->m_ch.OnChange();
}

void DashApplication::Run()
{
    SampleApplicationCore core;
    Run(&core);
}

void DashApplication::Run(ApplicationCore* core)
{
    m_pImpl->m_core = core;
	m_pImpl->m_core->InitializeApplication(this);

	CORt(CreateDeviceIndependentResources());

	// Register the window class
	WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = DashApplication::WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = sizeof(LONG_PTR);
	wcex.hInstance = HINST_THISCOMPONENT;
	wcex.hbrBackground = NULL;
	wcex.lpszMenuName = NULL;
	wcex.hCursor = LoadCursor(NULL, IDI_APPLICATION);
	wcex.lpszClassName = L"DashApplication";

	RegisterClassEx(&wcex);

	// Because the CreateWindow function takes its size in pixels,
	// obtain the system DPI and use it to scale the window size.
	FLOAT dpiX, dpiY;

	// The factory returns the current system DPI. This is also the value it will use
	// to create its own windows.
	m_pImpl->m_pDirect2dFactory->GetDesktopDpi(&dpiX, &dpiY);

	// Create the window.
	m_pImpl->m_hwnd = CreateWindow(
		L"DashApplication",
		L"DashApplication",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		static_cast<UINT>(ceil(640.f * dpiX / 96.f)),
		static_cast<UINT>(ceil(480.f * dpiY / 96.f)),
		NULL,
		NULL,
		HINST_THISCOMPONENT,
		this
		);
	CORt(m_pImpl->m_hwnd ? S_OK : E_FAIL);

	m_pImpl->m_root->SetVisible(true);
	ShowWindow(m_pImpl->m_hwnd, SW_SHOWNORMAL);
	UpdateWindow(m_pImpl->m_hwnd);

	MSG msg;

	while (GetMessage(&msg, NULL, 0, 0))
	{
		m_pImpl->m_animLibrary.Update();
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		m_pImpl->m_animLibrary.Kick();
	}
    m_pImpl->m_core = nullptr;
}

void DashApplication::SetRoot(Object* root)
{
	m_pImpl->m_root = root;
	m_pImpl->m_inputManager.SetRoot(root);
}

Object* DashApplication::GetRoot() const
{
    return m_pImpl->m_root;
}

}
}