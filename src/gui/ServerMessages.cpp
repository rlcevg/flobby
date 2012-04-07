#include "ServerMessages.h"
#include "TextDisplay.h"
#include "StringTable.h"
#include "Prefs.h"
#include "IChat.h"
#include "PopupMenu.h"

#include "model/Model.h"

#include <FL/fl_ask.H>
#include <boost/bind.hpp>

static char const * PrefServerMessagesSplitH = "ServerMessagesSplitH";

ServerMessages::ServerMessages(int x, int y, int w, int h, Model & model, IChat & chat):
    Fl_Tile(x,y,w,h, "Server"),
    model_(model),
    chat_(chat)
{
    text_ = new TextDisplay(x, y, w/2, h);
    userList_ = new StringTable(x+w/2, y, w/2, h, "UserList",
            { "name", "flags" });
    end();

    // model signals
    model_.connectConnected( boost::bind(&ServerMessages::connected, this, _1) );
    model_.connectLoginResult( boost::bind(&ServerMessages::loginResult, this, _1, _2) );
    model_.connectServerMsg( boost::bind(&ServerMessages::message, this, _1) );
    model_.connectUserJoined( boost::bind(&ServerMessages::userJoined, this, _1) );
    model_.connectUserChanged( boost::bind(&ServerMessages::userChanged, this, _1) );
    model_.connectUserLeft( boost::bind(&ServerMessages::userLeft, this, _1) );
    model_.connectRing( boost::bind(&ServerMessages::ring, this, _1) );

    userList_->connectRowClicked( boost::bind(&ServerMessages::userClicked, this, _1, _2) );
    userList_->connectRowDoubleClicked( boost::bind(&ServerMessages::userDoubleClicked, this, _1, _2) );

}

ServerMessages::~ServerMessages()
{
    prefs.set(PrefServerMessagesSplitH, userList_->x());
}

void ServerMessages::initTiles()
{
    int x;
    prefs.get(PrefServerMessagesSplitH, x, 0);
    if (x != 0)
    {
        position(userList_->x(), 0, x, 0);
    }
}

void ServerMessages::loginResult(bool success, std::string const & info)
{
    if (success)
    {
        std::vector<User const *> users = model_.getUsers();

        for (auto u : users)
        {
            assert(u);
            userList_->addRow(makeRow(*u));
        }
    }
    else
    {
        text_->append("Login failed: " + info);
    }
}

void ServerMessages::connected(bool connected)
{
    if (!connected)
    {
        userList_->clear();
        text_->append("Disconnected from server");
    }
}

void ServerMessages::message(std::string const & msg)
{
    text_->append(msg);
}

void ServerMessages::userJoined(User const & user)
{
    userList_->addRow(makeRow(user));
}

void ServerMessages::userChanged(User const & user)
{
    userList_->updateRow(makeRow(user));
}

void ServerMessages::userLeft(User const & user)
{
    userList_->removeRow(user.name());
}

StringTableRow ServerMessages::makeRow(User const & user)
{
    return StringTableRow( user.name(),
        {
            user.name(),
            flagsString(user)
        } );
}

std::string ServerMessages::flagsString(User const & user)
{
    std::ostringstream oss;
    oss << (user.status().bot() ? "B" : "");
    oss << (user.joinedBattle() != 0 ? "J" : "");
    oss << (user.status().inGame() ? "G" : "");
    return oss.str();
}

void ServerMessages::userClicked(int rowIndex, int button)
{
    if (button == FL_RIGHT_MOUSE)
    {
        StringTableRow const & row = userList_->getRow(static_cast<std::size_t>(rowIndex));

        User const & user = model_.getUser(row.id_);
        PopupMenu menu;
        menu.add("Open chat", 1);
        Battle const * battle = user.joinedBattle();
        if (battle != 0)
        {
            menu.add("Join same battle", 2);
        }

        if (menu.size() > 0)
        {
            int const id = menu.show();
            switch (id)
            {
            case 1:
                chat_.openPrivateChat(user.name());
                break;
            case 2:
                if (battle->passworded())
                {
                    char const * password = fl_input("Enter battle password");
                    if (password != NULL)
                    {
                        model_.joinBattle(battle->id(), password);
                    }
                }
                else
                {
                    model_.joinBattle(battle->id());
                }
                break;
            }
        }
    }
}

void ServerMessages::userDoubleClicked(int rowIndex, int button)
{
    StringTableRow const & row = userList_->getRow(static_cast<std::size_t>(rowIndex));
    chat_.openPrivateChat(row.id_);
}

void ServerMessages::ring(std::string const & userName)
{
    text_->append("ring from " + userName);
    fl_beep();
}