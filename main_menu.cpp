#include "game.h"
#include "input.h"
#include "keypress.h"
#include "debug.h"
#include "mapbuffer.h"
#include "cursesdef.h"

#include <sys/stat.h>
#include <dirent.h>

#define dbg(x) dout((DebugLevel)(x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "

void game::print_menu(WINDOW* w_open, int iSel, const int iMenuOffsetX, int iMenuOffsetY, bool bShowDDA)
{
    werase(w_open);

//    for (int i = 1; i < 79; i++)
//        mvwputch(w_open, 23, i, c_white, LINE_OXOX);

    mvwprintz(w_open, 23, 5, c_ltblue, "Dark Days Ahead v0.3:MGSmod rev.11 *** Forum: www.cataclysmdda.com/smf");

    int iLine = 0;
    const int iOffsetX1 = 3;
    const int iOffsetX2 = 14;
    const int iOffsetX3 = 15;

    const nc_color cColor1 = c_green;
    const nc_color cColor2 = c_red;
    const nc_color cColor3 = c_red;

    mvwprintz(w_open, iLine++, iOffsetX1, cColor1, "_________            __                   .__                            ");
    mvwprintz(w_open, iLine++, iOffsetX1, cColor1, "\\_   ___ \\ _____   _/  |_ _____     ____  |  |   ___.__   ______  _____  ");
    mvwprintz(w_open, iLine++, iOffsetX1, cColor1, "/    \\  \\/ \\__  \\  \\   __\\\\__  \\  _/ ___\\ |  |  <   |  | /  ___/ /     \\ ");
    mvwprintz(w_open, iLine++, iOffsetX1, cColor1, "\\     \\____ / __ \\_ |  |   / __ \\_\\  \\___ |  |__ \\___  | \\___ \\ |  Y Y  \\");
    mvwprintz(w_open, iLine++, iOffsetX1, cColor1, " \\______  /(____  / |__|  (____  / \\___  >|____/ / ____|/____  >|__|_|  /");
    mvwprintz(w_open, iLine++, iOffsetX1, cColor1, "        \\/      \\/             \\/      \\/        \\/          \\/       \\/ ");


	 mvwprintz(w_open, iMenuOffsetY++, iMenuOffsetX, (iSel == 0 ? h_white : c_white), "Briefing");
	 mvwprintz(w_open, iMenuOffsetY++, iMenuOffsetX, (iSel == 1 ? h_white : c_white), "New Game");
	 mvwprintz(w_open, iMenuOffsetY++, iMenuOffsetX, (iSel == 2 ? h_white : c_white), "Load Game");
	 mvwprintz(w_open, iMenuOffsetY++, iMenuOffsetX, (iSel == 3 ? h_white : c_white), "New World");
	 mvwprintz(w_open, iMenuOffsetY++, iMenuOffsetX, (iSel == 4 ? h_white : c_white), "Special");
	 mvwprintz(w_open, iMenuOffsetY++, iMenuOffsetX, (iSel == 5 ? h_white : c_white), "Options");
	 mvwprintz(w_open, iMenuOffsetY++, iMenuOffsetX, (iSel == 6 ? h_white : c_white), "Help");
	 mvwprintz(w_open, iMenuOffsetY++, iMenuOffsetX, (iSel == 7 ? h_white : c_white), "Credits");
	 mvwprintz(w_open, iMenuOffsetY++, iMenuOffsetX, (iSel == 8 ? h_white : c_white), "Quit");

    if (bShowDDA) {
        iLine++;
        mvwprintz(w_open, iLine++, iOffsetX2, cColor2, "    __  __________________    __       _______________    ____ ");
        mvwprintz(w_open, iLine++, iOffsetX2, cColor2, "   /  |/  / ____/_  __/   |  / /      / ____/ ____/   |  / __ \\");
        mvwprintz(w_open, iLine++, iOffsetX2, cColor2, "  / /|_/ / __/   / / / /| | / /      / / __/ __/ / /| | / /_/ /");
        mvwprintz(w_open, iLine++, iOffsetX2, cColor2, " / /  / / /___  / / / ___ |/ /___   / /_/ / /___/ ___ |/ _, _/ ");
        mvwprintz(w_open, iLine++, iOffsetX2, cColor2, "/_/  /_/_____/ /_/ /_/  |_/_____/   \\____/_____/_/  |_/_/ |_|  ");

        iLine++;
        mvwprintz(w_open, iLine++, iOffsetX3, cColor3, "         _____ ____  __    ________");
        mvwprintz(w_open, iLine++, iOffsetX3, cColor3, "        / ___// __ \\/ /   /  _/ __ \\");
        mvwprintz(w_open, iLine++, iOffsetX3, cColor3, "        \\__ \\/ / / / /    / // / / /    tactical survival");
        mvwprintz(w_open, iLine++, iOffsetX3, cColor3, "       ___/ / /_/ / /____/ // /_/ /        espionage action");
        mvwprintz(w_open, iLine++, iOffsetX3, cColor3, "      /____/\\____/_____/___/_____/");
//        mvwprintz(w_open, iLine++, iOffsetX3, cColor3, "");
    }


 wrefresh(w_open);
// refresh();
}

void game::print_menu_items(WINDOW* w_in, std::vector<std::string> vItems, int iSel, int iOffsetY, int iOffsetX)
{
    mvwprintz(w_in, iOffsetY, iOffsetX, c_black, "");

    for (int i=0; i < vItems.size(); i++) {
        wprintz(w_in, c_ltgray, "[");
        if (iSel == i) {
            wprintz(w_in, h_white, vItems[i].c_str());
        } else {
            wprintz(w_in, c_white, (vItems[i].substr(0, 1)).c_str());
            wprintz(w_in, c_dkgray, (vItems[i].substr(1)).c_str());
        }
        wprintz(w_in, c_ltgray, "] ");
    }
}

bool game::opening_screen()
{
 WINDOW* w_open = newwin((VIEWY*2)+1, 65+(VIEWX*2), 0, 0);
 int iMenuOffsetX = 3;
 int iMenuOffsetY = 12;

//CAT-s:
    playMusic(0); 
    playSound(17); 

// print_menu(w_open, 0, iMenuOffsetX, iMenuOffsetY);
 std::vector<std::string> savegames, templates;
 std::string tmp;
 dirent *dp;
 DIR *dir = opendir("save");
 if (!dir) {
#if (defined _WIN32 || defined __WIN32__)
  mkdir("save");
#else
  mkdir("save", 0777);
#endif
  dir = opendir("save");
 }
 if (!dir) {
  dbg(D_ERROR) << "game:opening_screen: Unable to make save directory.";
  debugmsg("Could not make './save' directory");
  endwin();
  exit(1);
 }
 while ((dp = readdir(dir))) {
  tmp = dp->d_name;
  if (tmp.find(".sav") != std::string::npos)
   savegames.push_back(tmp.substr(0, tmp.find(".sav")));
 }
 closedir(dir);
 dir = opendir("data");
 while ((dp = readdir(dir))) {
  tmp = dp->d_name;
  if (tmp.find(".template") != std::string::npos)
   templates.push_back(tmp.substr(0, tmp.find(".template")));
 }
 int sel1 = 1, sel2 = 1, layer = 1;
 InputEvent input;
 bool start = false;

 // Load MOTD and store it in a string
 std::vector<std::string> motd;
 std::ifstream motd_file;
 motd_file.open("data/motd");
 if (!motd_file.is_open())
  motd.push_back("No message today.");
 else {
  while (!motd_file.eof()) {
   std::string tmp;
   getline(motd_file, tmp);
   if (tmp[0] != '#')
    motd.push_back(tmp);
  }
 }

 // Load Credits and store it in a string
 std::vector<std::string> credits;
 std::ifstream credits_file;
 credits_file.open("data/credits");
 if (!credits_file.is_open())
  credits.push_back("No message today.");
 else {
  while (!credits_file.eof()) {
   std::string tmp;
   getline(credits_file, tmp);
   if (tmp[0] != '#')
    credits.push_back(tmp);
  }
 }

 while(!start) {

//CAT-s:
    playSound(0);


  if (layer == 1) {
//CAT-mgs:
   print_menu(w_open, sel1, iMenuOffsetX, iMenuOffsetY, (sel1!=0 && sel1!=7));

   if (sel1 == 0) {	// Print the MOTD.
    for (int i = 0; i < motd.size() && i < 16; i++)
     mvwprintz(w_open, i + 7, 12 + iMenuOffsetX, c_ltred, motd[i].c_str());

    wrefresh(w_open);
//    refresh();
   } else if (sel1 == 7) {	// Print the Credits.
    for (int i = 0; i < credits.size() && i < 16; i++)
     mvwprintz(w_open, i + 7, 12 + iMenuOffsetX, c_ltred, credits[i].c_str());

    wrefresh(w_open);
//    refresh();
   }

   input = get_input();
   if (input == DirectionN) {
    if (sel1 > 0)
     sel1--;
    else
     sel1 = 8;
   } else if (input == DirectionS) {
    if (sel1 < 8)
     sel1++;
    else
     sel1 = 0;
   } else if ((input == DirectionE || input == Confirm) && sel1 > 0 && sel1 != 7) {
    if (sel1 == 5) {
     show_options();
    } else if (sel1 == 6) {
     help();
    } else if (sel1 == 8) {
     uquit = QUIT_MENU;
     return false;
    } else {
     sel2 = 1;
     layer = 2;
    }
   }
  } else if (layer == 2) {
   if (sel1 == 1) {	// New Character
    mvwprintz(w_open, 1 + iMenuOffsetY, 12 + iMenuOffsetX, (sel2 == 1 ? h_white : c_white),
              "Custom Character");
    mvwprintz(w_open, 2 + iMenuOffsetY, 12 + iMenuOffsetX, (sel2 == 2 ? h_white : c_white),
              "Preset Character");
    mvwprintz(w_open, 3 + iMenuOffsetY, 12 + iMenuOffsetX, (sel2 == 3 ? h_white : c_white),
              "Random Character");
    wrefresh(w_open);
//    refresh();

//CAT-mgs:
sel2= 2;
layer= 3;
 //   input = get_input();

    if (input == DirectionN) {
     if (sel2 > 1)
      sel2--;
     else
      sel2 = 3;
    } if (input == DirectionS) {
     if (sel2 < 3)
      sel2++;
     else
      sel2 = 1;
    } else if (input == DirectionW) {
     mvwprintz(w_open, 1 + iMenuOffsetY, 12 + iMenuOffsetX, c_black, "                ");
     mvwprintz(w_open, 2 + iMenuOffsetY, 12 + iMenuOffsetX, c_black, "                ");
     mvwprintz(w_open, 3 + iMenuOffsetY, 12 + iMenuOffsetX, c_black, "                ");
     layer = 1;
     sel1 = 1;
    }
    if (input == DirectionE || input == Confirm) {
     if (sel2 == 1) {
      if (!u.create(this, PLTYPE_CUSTOM)) {
       u = player();
       delwin(w_open);
       return (opening_screen());
      }
      start_game();
      start = true;
     }
     if (sel2 == 2) {
      layer = 3;
      sel1 = 0;
      mvwprintz(w_open, 1 + iMenuOffsetY, 12 + iMenuOffsetX, c_dkgray, "Custom Character");
      mvwprintz(w_open, 2 + iMenuOffsetY, 12 + iMenuOffsetX, c_white,  "Preset Character");
      mvwprintz(w_open, 3 + iMenuOffsetY, 12 + iMenuOffsetX, c_dkgray, "Random Character");
     }
     if (sel2 == 3) {
      if (!u.create(this, PLTYPE_RANDOM)) {
       u = player();
       delwin(w_open);
       return (opening_screen());
      }
      start_game();
      start = true;
     }
    }
   } else if (sel1 == 2) {	// Load Character
    if (savegames.size() == 0)
     mvwprintz(w_open, 2 + iMenuOffsetY, 12 + iMenuOffsetX, c_yellow, "No save games found!");
    else {
     int savestart = (sel2 < 7 ?  0 : sel2 - 7),
         saveend   = (sel2 < 7 ? 14 : sel2 + 7);
     for (int i = savestart; i < saveend; i++) {
      int line = 2 + iMenuOffsetY + i - savestart;
      if (i < savegames.size())
       mvwprintz(w_open, line, 12 + iMenuOffsetX, (sel2 - 1 == i ? h_white : c_white),
                 savegames[i].c_str());
     }
    }

    wrefresh(w_open);
//    refresh();
    input = get_input();
    if (input == DirectionN) {
     if (sel2 > 1)
      sel2--;
     else
      sel2 = savegames.size();
    } else if (input == DirectionS) {
     if (sel2 < savegames.size())
      sel2++;
     else
      sel2 = 1;
    } else if (input == DirectionW) {
     layer = 1;
    }

    if (input == DirectionE || input == Confirm) {
     if (sel2 > 0 && savegames.size() > 0) {
      load(savegames[sel2 - 1]);
      start = true;
     }
    }
   } else if (sel1 == 3) {  // Delete world
    if (query_yn("Delete the world and all saves?")) {
     delete_save();
     savegames.clear();
     MAPBUFFER.reset();
     MAPBUFFER.make_volatile();
    }

    layer = 1;
   } else if (sel1 == 4) {	// Special game
    for (int i = 1; i < NUM_SPECIAL_GAMES; i++) {
     mvwprintz(w_open, 3 + i + iMenuOffsetY, 12 + iMenuOffsetX, (sel2 == i ? h_white : c_white),
               special_game_name( special_game_id(i) ).c_str());
    }
    wrefresh(w_open);
//    refresh();
    input = get_input();
    if (input == DirectionN) {
     if (sel2 > 1)
      sel2--;
     else
      sel2 = NUM_SPECIAL_GAMES - 1;
    } else if (input == DirectionS) {
     if (sel2 < NUM_SPECIAL_GAMES - 1)
      sel2++;
     else
      sel2 = 1;
    } else if (input == DirectionW) {
     layer = 1;
    }
    if (input == DirectionE || input == Confirm) {
     if (sel2 >= 1 && sel2 < NUM_SPECIAL_GAMES) {
      delete gamemode;
      gamemode = get_special_game( special_game_id(sel2) );
      if (!gamemode->init(this)) {
       delete gamemode;
       gamemode = new special_game;
       u = player();
       delwin(w_open);
       return (opening_screen());
      }
      start = true;
     }
    }
   }
  } else if (layer == 3) {	// Character Templates
   if (templates.size() == 0)
    mvwprintz(w_open, 2 + iMenuOffsetY, 29 + iMenuOffsetX, c_yellow, "No templates found!");
   else {
    int tempstart = (sel1 < 6 ?  0 : sel1 - 6),
        tempend   = (sel1 < 6 ? 14 : sel1 + 8);
    for (int i = tempstart; i < tempend; i++) {
     int line = 2 + iMenuOffsetY + i - tempstart;
     if (i < templates.size())
      mvwprintz(w_open, line, 29 + iMenuOffsetX, (sel1 == i ? h_white : c_white),
                templates[i].c_str());
    }
   }

   wrefresh(w_open);
//   refresh();
   input = get_input();
   if (input == DirectionN) {
    if (sel1 > 0)
     sel1--;
    else
     sel1 = templates.size() - 1;
   } else if (input == DirectionS) {
    if (sel1 < templates.size() - 1)
     sel1++;
    else
     sel1 = 0;
   } else if (input == DirectionW || templates.size() == 0) {
    sel1 = 1;
    layer = 2;
//    print_menu(w_open, sel1, iMenuOffsetX, iMenuOffsetY, false);
   }
   else if (input == DirectionE || input == Confirm) {
    if (!u.create(this, PLTYPE_TEMPLATE, templates[sel1])) {
     u = player();
     delwin(w_open);
     return (opening_screen());
    }

    start_game();
    start = true;
   }
  }
 }

 delwin(w_open);
 if (start == false)
  uquit = QUIT_MENU;
 return start;
}



