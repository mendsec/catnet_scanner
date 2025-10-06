// GacUI front-end scaffold for Windows Network Scanner
// Requires GacUI include and lib paths set via environment variables:
//   GACUI_INCLUDE, GACUI_LIBS
// Build: powershell -ExecutionPolicy Bypass -File build.ps1 -Compiler MSVC -UseGacUI

// Import GacUI before Windows headers to avoid conflicts
#ifndef GACUI_AVAILABLE
#include <GacUI.h>
#define GACUI_AVAILABLE 1
#endif

// Register a default theme so controls with ThemeName can be created
#include <Skins/DarkSkin/DarkSkin.h>

#define WIN32_LEAN_AND_MEAN 1
#define _WINSOCKAPI_
#include <windows.h>
#include <stdio.h>
#include <string>
#include <cstring>
#include <fstream>

#include "app.h"
#include "scan.h"
#include "net.h"
#include "utils.h"
#include "export.h"

// Note: This is a minimal scaffold to host the existing scanning logic in a GacUI window.
// It creates a window with buttons (Local, Range, Ping, DNS, Export) and a list area.
// Hook implementations are straightforward and call existing C functions.

using namespace vl;
using namespace vl::presentation;
using namespace vl::presentation::controls;
using namespace vl::presentation::compositions;
using namespace vl::presentation::theme;
using namespace vl::collections;

class MainWindow : public GuiWindow
{
private:
    GuiButton* btnLocal = nullptr;
    GuiButton* btnFaixa = nullptr;
    GuiButton* btnPing = nullptr;
    GuiButton* btnDns = nullptr;
    GuiButton* btnExport = nullptr;
    GuiTextList* listView = nullptr;
    GuiLabel* lblStatus = nullptr;

    DeviceList results;
    ScanConfig cfg;

    static WString fromUtf8(const char* s)
    {
        if (!s) return WString();
        int len = (int)strlen(s);
        int wlen = MultiByteToWideChar(CP_UTF8, 0, s, len, nullptr, 0);
        if (wlen <= 0) return WString();
        std::wstring ws; ws.resize(wlen);
        MultiByteToWideChar(CP_UTF8, 0, s, len, ws.data(), wlen);
        return WString(ws.c_str());
    }

    static std::string toUtf8(const WString& ws)
    {
        const wchar_t* wbuf = ws.Buffer();
        int wlen = (int)ws.Length();
        int len = WideCharToMultiByte(CP_UTF8, 0, wbuf, wlen, nullptr, 0, nullptr, nullptr);
        std::string s; s.resize(len);
        if (len > 0)
        {
            WideCharToMultiByte(CP_UTF8, 0, wbuf, wlen, s.data(), len, nullptr, nullptr);
        }
        return s;
    }

    void RefreshList()
    {
        listView->GetItems().Clear();
        for (vint i = 0; i < (vint)results.count; ++i)
        {
            auto& di = results.items[i];
            WString ip = fromUtf8(di.ip);
            WString host = fromUtf8(di.hostname);
            WString mac = fromUtf8(di.mac);
            WString status = di.is_alive ? L"UP" : L"DOWN";
            Ptr<list::TextItem> item(new list::TextItem(ip + L"  " + host + L"  MAC: " + mac + L"  " + status));
            listView->GetItems().Add(item);
        }
    }

    void OnLocal(GuiGraphicsComposition*, GuiEventArgs&)
    {
        device_list_clear(&results);
        scan_subnet(&results, &cfg);
        RefreshList();
    }

    static WString AskString(GuiWindow* owner, const WString& title)
    {
        // Build a minimal modal dialog with a label, textbox, and OK/Cancel
        auto dialog = new GuiWindow(ThemeName::Window);
        dialog->SetText(title);
        dialog->GetBoundsComposition()->SetPreferredMinSize(Size(400, 120));

        auto table = new GuiTableComposition;
        table->SetRowsAndColumns(3, 2);
        table->SetRowOption(0, GuiCellOption::MinSizeOption());
        table->SetRowOption(1, GuiCellOption::PercentageOption(1.0));
        table->SetRowOption(2, GuiCellOption::MinSizeOption());
        table->SetColumnOption(0, GuiCellOption::PercentageOption(1.0));
        table->SetColumnOption(1, GuiCellOption::MinSizeOption());
        dialog->GetContainerComposition()->AddChild(table);

        auto lbl = new GuiLabel(ThemeName::Label);
        lbl->SetText(L"Value:");
        auto cellLbl = new GuiCellComposition; table->AddChild(cellLbl); cellLbl->SetSite(0,0,1,2);
        cellLbl->AddChild(lbl->GetBoundsComposition());

        auto tb = new GuiSinglelineTextBox(ThemeName::SinglelineTextBox);
        auto cellTb = new GuiCellComposition; table->AddChild(cellTb); cellTb->SetSite(1,0,1,2);
        cellTb->AddChild(tb->GetBoundsComposition());

        auto btnOk = new GuiButton(ThemeName::Button); btnOk->SetText(L"OK");
        auto btnCancel = new GuiButton(ThemeName::Button); btnCancel->SetText(L"Cancel");
        auto cellOk = new GuiCellComposition; table->AddChild(cellOk); cellOk->SetSite(2,0,1,1); cellOk->AddChild(btnOk->GetBoundsComposition());
        auto cellCancel = new GuiCellComposition; table->AddChild(cellCancel); cellCancel->SetSite(2,1,1,1); cellCancel->AddChild(btnCancel->GetBoundsComposition());

        WString result;
        bool accepted = false;
        btnOk->Clicked.AttachLambda([&](GuiGraphicsComposition*, GuiEventArgs&){ accepted = true; result = tb->GetText(); dialog->Hide(); });
        btnCancel->Clicked.AttachLambda([&](GuiGraphicsComposition*, GuiEventArgs&){ accepted = false; dialog->Hide(); });

        dialog->MoveToScreenCenter();
        dialog->ShowModalAndDelete(owner, [](){});
        return accepted ? result : WString();
    }

    void OnFaixa(GuiGraphicsComposition*, GuiEventArgs&)
    {
        WString start = AskString(this, L"Enter start IP:");
        WString end = AskString(this, L"Enter end IP:");
        if (start.Length() == 0 || end.Length() == 0) return;
        device_list_clear(&results);
        auto s = toUtf8(start);
        auto e = toUtf8(end);
        scan_range(&results, &cfg, s.c_str(), e.c_str());
        RefreshList();
    }

    void OnPing(GuiGraphicsComposition*, GuiEventArgs&)
    {
        WString wip = AskString(this, L"Enter IP for ping:");
        if (wip.Length() == 0) return;
        auto ip8 = toUtf8(wip);
        int ok = net_ping_ipv4(ip8.c_str());
        GetCurrentController()->DialogService()->ShowMessageBox(this->GetNativeWindow(), ok ? L"Ping OK" : L"Ping failed", L"Ping");
    }

    void OnDns(GuiGraphicsComposition*, GuiEventArgs&)
    {
        WString wip = AskString(this, L"Enter IP for reverse DNS:");
        if (wip.Length() == 0) return;
        auto ip8 = toUtf8(wip);
        char host[256] = {0};
        if (net_reverse_dns(ip8.c_str(), host, sizeof(host)))
        {
            GetCurrentController()->DialogService()->ShowMessageBox(this->GetNativeWindow(), fromUtf8(host), L"DNS");
        }
        else
        {
            GetCurrentController()->DialogService()->ShowMessageBox(this->GetNativeWindow(), L"(no name)", L"DNS");
        }
    }

    void OnExport(GuiGraphicsComposition*, GuiEventArgs&)
    {
        if (export_results_to_file("results.txt", &results))
        {
            GetCurrentController()->DialogService()->ShowMessageBox(this->GetNativeWindow(), L"Exported to results.txt", L"Export");
        }
        else
        {
            GetCurrentController()->DialogService()->ShowMessageBox(this->GetNativeWindow(), L"Failed to export.", L"Export");
        }
    }

public:
    MainWindow()
        : GuiWindow(ThemeName::Window)
    {
        // Init data
        device_list_init(&results);
        scan_config_init(&cfg);

        this->SetText(L"CatNet Scanner (GacUI)");
        this->GetBoundsComposition()->SetPreferredMinSize(Size(900, 600));

        // Layout
        auto table = new GuiTableComposition;
        table->SetRowsAndColumns(3, 1);
        table->SetRowOption(0, GuiCellOption::MinSizeOption());
        table->SetRowOption(1, GuiCellOption::PercentageOption(1.0));
        table->SetRowOption(2, GuiCellOption::MinSizeOption());
        table->SetColumnOption(0, GuiCellOption::PercentageOption(1.0));
        this->GetContainerComposition()->AddChild(table);
        table->SetAlignmentToParent(Margin(0, 0, 0, 0));
        table->SetCellPadding(4);

        // Header with buttons
        btnLocal = new GuiButton(ThemeName::Button); btnLocal->SetText(L"Local"); btnLocal->Clicked.AttachMethod(this, &MainWindow::OnLocal);
        btnFaixa = new GuiButton(ThemeName::Button); btnFaixa->SetText(L"Range"); btnFaixa->Clicked.AttachMethod(this, &MainWindow::OnFaixa);
        btnPing = new GuiButton(ThemeName::Button); btnPing->SetText(L"Ping"); btnPing->Clicked.AttachMethod(this, &MainWindow::OnPing);
        btnDns = new GuiButton(ThemeName::Button); btnDns->SetText(L"DNS"); btnDns->Clicked.AttachMethod(this, &MainWindow::OnDns);
        btnExport = new GuiButton(ThemeName::Button); btnExport->SetText(L"Export"); btnExport->Clicked.AttachMethod(this, &MainWindow::OnExport);

        auto header = new GuiStackComposition;
        header->SetDirection(GuiStackComposition::Horizontal);
        header->SetAlignmentToParent(Margin(4, 4, 4, 4));
        // Use stack items to ensure proper layout of controls
        {
            auto i = new GuiStackItemComposition; i->AddChild(btnLocal->GetBoundsComposition()); header->AddChild(i);
        }
        {
            auto i = new GuiStackItemComposition; i->AddChild(btnFaixa->GetBoundsComposition()); header->AddChild(i);
        }
        {
            auto i = new GuiStackItemComposition; i->AddChild(btnPing->GetBoundsComposition()); header->AddChild(i);
        }
        {
            auto i = new GuiStackItemComposition; i->AddChild(btnDns->GetBoundsComposition()); header->AddChild(i);
        }
        {
            auto i = new GuiStackItemComposition; i->AddChild(btnExport->GetBoundsComposition()); header->AddChild(i);
        }

        auto cellHeader = new GuiCellComposition;
        table->AddChild(cellHeader);
        cellHeader->SetSite(0, 0, 1, 1);
        cellHeader->AddChild(header);

        // List view
        listView = new GuiTextList(ThemeName::TextList);
        auto cellBody = new GuiCellComposition;
        table->AddChild(cellBody);
        cellBody->SetSite(1, 0, 1, 1);
        listView->GetBoundsComposition()->SetAlignmentToParent(Margin(4, 4, 4, 4));
        cellBody->AddChild(listView->GetBoundsComposition());

        // Initial message in the list
        listView->GetItems().Add(Ptr<list::TextItem>(new list::TextItem(L"Ready to scan")));

        // Status bar
        lblStatus = new GuiLabel(ThemeName::Label);
        lblStatus->SetText(L"Ready");
        auto cellStatus = new GuiCellComposition;
        table->AddChild(cellStatus);
        cellStatus->SetSite(2, 0, 1, 1);
        lblStatus->GetBoundsComposition()->SetAlignmentToParent(Margin(4, 4, 4, 4));
        cellStatus->AddChild(lblStatus->GetBoundsComposition());

        // Default close behavior: closing the main window ends the run loop
    }

    ~MainWindow()
    {
        device_list_clear(&results);
    }
};

// Some GacUI builds expect GuiMain inside vl::presentation namespace.
// Provide both global and namespaced entry points to ensure correct dispatch.
void GuiMain()
{
    // Initialize networking for scanning functions
    net_init();
    // Register a default theme to enable ThemeName-based controls
    vl::presentation::theme::RegisterTheme(vl::Ptr(new darkskin::Theme));
    {
        std::ofstream f("gacui_log.txt", std::ios::app);
        f << "GuiMain start" << std::endl;
    }
    auto window = new MainWindow;
    window->MoveToScreenCenter();
    window->Show();
    {
        std::ofstream f("gacui_log.txt", std::ios::app);
        f << "Before Run" << std::endl;
    }
    GetApplication()->Run(window);
    {
        std::ofstream f("gacui_log.txt", std::ios::app);
        f << "After Run" << std::endl;
    }
    delete window;
    net_cleanup();
}

namespace vl { namespace presentation {
    void GuiMain()
    {
        ::GuiMain();
    }
} }

// Entry point: initialize GacUI with GDI renderer and dispatch GuiMain via framework
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    // Use GDI renderer for broader compatibility and fewer D2D/DWrite deps
    SetupWindowsGDIRenderer();
    ::GuiMain();
    return 0;
}