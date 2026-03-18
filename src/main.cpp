/**
 * Theme Style Browser (tsb) - browse theme/style icon libraries.
 * Left: package tree. Right: file list with Name, Count, and one column per themestyle.
 *
 * Copyright (C) 2026 Lenik
 * SPDX-License-Identifier: AGPL-3.0-or-later (see LICENSE for Anti-AI restriction)
 */
#include "tsb/TSBFrame.hpp"
#include <bas/proc/MyStackWalker.hpp>
#include <bas/proc/stackdump.h>
#include <bas/wx/app.hpp>

#include <wx/app.h>
#include <wx/cmdline.h>
#include <wx/wx.h>

class TSBApp : public uiApp {
  public:
    void OnInitCmdLine(wxCmdLineParser& parser) override {
        wxApp::OnInitCmdLine(parser);
        parser.AddParam("path", wxCMD_LINE_VAL_STRING,
                        wxCMD_LINE_PARAM_OPTIONAL | wxCMD_LINE_PARAM_MULTIPLE);
    }
    bool OnUserInit() override {
        tsb::TSBFrame* frame = new tsb::TSBFrame("Theme Style Browser");
        frame->CenterOnScreen();
        frame->Show(true);
        if (argc >= 2)
            frame->openLibrary(wxString::FromUTF8(argv[1]));
        return true;
    }
};

int main(int argc, char** argv) {
    stackdump_install_crash_handler(&stackdump_color_schema_default);
    stackdump_set_interactive(1);

    TSBApp app;
    return app.main(argc, argv);
}