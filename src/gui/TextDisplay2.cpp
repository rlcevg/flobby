// This file is part of flobby (GPL v2 or later), see the LICENSE file

#include "TextDisplay2.h"
#include "PopupMenu.h"
#include "LogFile.h"
#include "log/Log.h"
#include "TextFunctions.h"

#include <FL/Fl.H>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <sstream>
#include <cstring>


Fl_Text_Display::Style_Table_Entry TextDisplay2::textStyles_[STYLE_COUNT];


void TextDisplay2::initTextStyles()
{
    textStyles_[STYLE_TIME] =   {  FL_FOREGROUND_COLOR, FL_COURIER, FL_NORMAL_SIZE };
    textStyles_[STYLE_LOW] =    {  FL_FOREGROUND_COLOR, FL_HELVETICA, FL_NORMAL_SIZE };
    textStyles_[STYLE_NORMAL] = {  FL_FOREGROUND_COLOR, FL_HELVETICA, FL_NORMAL_SIZE };
    textStyles_[STYLE_HIGH] =   {  FL_FOREGROUND_COLOR, FL_HELVETICA, FL_NORMAL_SIZE };
    textStyles_[STYLE_MYTEXT] = {  FL_FOREGROUND_COLOR, FL_HELVETICA, FL_NORMAL_SIZE };
}


TextDisplay2::TextDisplay2(int x, int y, int w, int h, LogFile* logFile, char const * label)
    : Fl_Text_Display(x, y, w, h, label)
    , logFile_(logFile)
{
    textsize(12);

    align(FL_ALIGN_TOP_LEFT);
    box(FL_THIN_DOWN_BOX);
    text_ = new Fl_Text_Buffer();
    buffer(text_);
    text_->tab_distance(4);

    style_ = new Fl_Text_Buffer();
    int style_size = sizeof(textStyles_)/sizeof(textStyles_[0]);
    highlight_data(style_, textStyles_, style_size, 'A', 0, 0);

    wrap_mode(WRAP_AT_BOUNDS, 0);

}

TextDisplay2::~TextDisplay2()
{
}

void TextDisplay2::append(std::string const & text, int interest)
{
    // prepends with time stamp and adds newline at end
    // interest: -2=my, -1=low, 0=normal, 1=high

    // scroll to bottom if last line is visible
    bool const scrollToBottom = !(mLastChar < text_->length());

    // if string is empty we just add one empty line
    if (text.empty())
    {
        text_->append("\n");
        style_->append("\n");
    }
    else
    {
        // time stamp
        std::string const timeNow = getHourMinuteNow();

        std::ostringstream oss;
        oss << timeNow << " " << text << '\n';
        std::string const line = oss.str();

        text_->append(line.c_str());

        // style for time (including trailing space)
        std::string const styleTime(timeNow.size()+1, 'A');
        style_->append(styleTime.c_str());

        // style for rest (text + newline)
        char style;
        switch (interest)
        {
        case -2: style = 'E'; break;
        case -1: style = 'B'; break;
        case  0: style = 'C'; break;
        case  1: style = 'D'; break;

        default:
            style = 'D';
            LOG(WARNING)<< "unknown interest level "<< interest;
            break;
        }
        std::string const styleText(text.size(), style);
        style_->append(styleText.c_str());
        style_->append("\n");
    }

    // limit text buffer size, raise limit if we are scrolled up
    int const maxLength = 20000*(scrollToBottom ? 1 : 10);
    while (text_->length() > maxLength)
    {
        int const posNewline = text_->line_end(0);
        text_->remove(0, posNewline+1);
        style_->remove(0, posNewline+1);
    }

    if (scrollToBottom)
    {
        scroll(text_->length(), 0);
    }
}

int TextDisplay2::handle(int event)
{
    // make mouse wheel scroll in bigger steps if shift is down
    if (event == FL_MOUSEWHEEL && Fl::event_shift())
    {
        Fl::e_dy *= 3;
    }

    switch (event)
    {
    case FL_PUSH: // handle double click on text with web link
        if (Fl::event_clicks() && Fl::event_button() == FL_LEFT_MOUSE)
        {
            int const pos = xy_to_position(Fl::event_x(), Fl::event_y());
            if (pos < text_->length())
            {
                int posStart;
                if (text_->search_backward(pos, "http", &posStart) && posStart >= text_->line_start(pos))
                {
                    int posSpace;
                    int posNewline;
                    int found = 0;
                    found += text_->findchar_forward(posStart, ' ', &posSpace);
                    found += text_->findchar_forward(posStart, '\n', &posNewline);
                    if (found > 0)
                    {
                        int const posEnd = std::min(posSpace, posNewline);
                        text_->select(posStart, posEnd);
                        char * sel = text_->selection_text();
                        std::string const link = sel;
                        ::free(sel);
                        flOpenUri(link);
                        return 1;
                    }
                }
            }
        }
        else if (Fl::event_button() == FL_RIGHT_MOUSE && Fl::event_clicks() == 0)
        {
            PopupMenu menu;
            menu.add("Clear", 1);

            if (logFile_ && LogFile::enabled() && boost::filesystem::exists(logFile_->path()))
            {
                menu.add("Open log", 2);
            }

            int const id = menu.show();
            switch (id)
            {
            case 1:
                text_->remove(0, text_->length());
                style_->remove(0, style_->length());
                return 1;
            case 2:
                LogFile::openLogFile(logFile_->path());
                return 1;
            }
        }
        break;
    }

    return Fl_Text_Display::handle(event);
}
