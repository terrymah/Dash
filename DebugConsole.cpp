#include "DGui.h"
#include "AnimatedVar.h"

namespace tjm {
namespace dash {

struct DebugConsoleImpl
{
    ListView m_console;
        ListView m_log;
        ListView m_inputList;
            TextLabel m_start;
            TextLabel m_input;
            SolidObject m_caret;

    DebugConsoleImpl();
};

DebugConsoleImpl::DebugConsoleImpl() :
    m_caret(D2D1::ColorF(D2D1::ColorF::White))
{
}

DebugConsole::DebugConsole() :
    m_pImpl(new DebugConsoleImpl)
{
    m_pImpl->m_console.SetOrientation(Orientation::Vertical);
    m_pImpl->m_console.SetDirection(Direction::TopDown);
    m_pImpl->m_console.SetZOrder(100);

    m_pImpl->m_log.SetOrientation(Orientation::Vertical);
    m_pImpl->m_log.SetDirection(Direction::BottomUp);

    m_pImpl->m_start.SetText(">");

    m_pImpl->m_console.AddChild(&m_pImpl->m_log);
    m_pImpl->m_console.AddChild(&m_pImpl->m_inputList);

    m_pImpl->m_inputList.AddChild(&m_pImpl->m_start);
    m_pImpl->m_inputList.AddChild(&m_pImpl->m_input);
    m_pImpl->m_inputList.AddChild(&m_pImpl->m_caret);

    AddChild(&m_pImpl->m_console);
}

DebugConsole::~DebugConsole()
{
    delete m_pImpl;
}

void DebugConsole::Log(const std::string text)
{

}

}