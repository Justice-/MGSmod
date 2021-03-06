#include "game.h"
#include "rng.h"
#include "input.h"
#include "keypress.h"
#include "output.h"
#include "skill.h"
#include "line.h"
#include "computer.h"
#include "weather_data.h"
#include "veh_interact.h"
#include "options.h"
#include "mapbuffer.h"
#include "bodypart.h"
#include "map.h"
#include "output.h"
#include <map>
#include <algorithm>
#include <string>
#include <fstream>
#include <sstream>
#include <math.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#if (defined _WIN32 || defined __WIN32__)
#include <windows.h>
#include <tchar.h>
#endif


//CAT-s:
int closeMonster= 99;

//CAT-mgs:
int JUMPING= 0;


void intro();
nc_color sev(int a);	// Right now, ONLY used for scent debugging....

// This is the main game set-up process.
game::game() :
 w_terrain(NULL),
 w_minimap(NULL),
 w_HP(NULL),
 w_moninfo(NULL),
 w_messages(NULL),
 w_location(NULL),
 w_status(NULL),
 om_hori(NULL),
 om_vert(NULL),
 om_diag(NULL),
 gamemode(NULL)
{
// dout() << "Game initialized.";
// Gee, it sure is init-y around here!
 init_itypes();	      // Set up item types                (SEE itypedef.cpp)
 init_mtypes();	      // Set up monster types             (SEE mtypedef.cpp)
 init_monitems();     // Set up the items monsters carry  (SEE monitemsdef.cpp)
 init_traps();	      // Set up the trap types            (SEE trapdef.cpp)
 init_mapitems();     // Set up which items appear where  (SEE mapitemsdef.cpp)
 init_recipes();      // Set up crafting reciptes         (SEE crafting.cpp)
 init_mongroups();    // Set up monster groupings         (SEE mongroupdef.cpp)
 init_missions();     // Set up mission templates         (SEE missiondef.cpp)
 init_construction(); // Set up constructables            (SEE construction.cpp)
 init_mutations();
 init_vehicles();     // Set up vehicles                  (SEE veh_typedef.cpp)
 init_autosave();     // Set up autosave
 load_keyboard_settings();


 gamemode = new special_game;	// Nothing, basically.
}

game::~game()
{
 delete gamemode;
 for (int i = 0; i < itypes.size(); i++)
  delete itypes[i];
 for (int i = 0; i < mtypes.size(); i++)
  delete mtypes[i];
 delwin(w_terrain);
 delwin(w_minimap);
 delwin(w_HP);
 delwin(w_moninfo);
 delwin(w_messages);
 delwin(w_location);
 delwin(w_status);
}

void game::init_ui(){

    clear();	// Clear the screen
    intro();	// Print an intro screen, make sure we're at least 80x25


    #if (defined _WIN32 || defined __WIN32__)

        TERMX = 55 + (OPTIONS[OPT_VIEWPORT_X] * 2 + 1);
        TERMY = OPTIONS[OPT_VIEWPORT_Y] * 2 + 1;
        VIEWX = (OPTIONS[OPT_VIEWPORT_X] > 17) ? 17 : OPTIONS[OPT_VIEWPORT_X];
        VIEWY = (OPTIONS[OPT_VIEWPORT_Y] > 17) ? 17 : OPTIONS[OPT_VIEWPORT_Y];
        VIEW_OFFSET_X = (OPTIONS[OPT_VIEWPORT_X] > 17) ? OPTIONS[OPT_VIEWPORT_X]-17 : 0;
        VIEW_OFFSET_Y = (OPTIONS[OPT_VIEWPORT_Y] > 17) ? OPTIONS[OPT_VIEWPORT_Y]-17 : 0;

        TERRAIN_WINDOW_WIDTH = (VIEWX * 2) + 1;
        TERRAIN_WINDOW_HEIGHT = (VIEWY * 2) + 1;
    #else
        getmaxyx(stdscr, TERMY, TERMX);


        //make sure TERRAIN_WINDOW_WIDTH and TERRAIN_WINDOW_HEIGHT are uneven
        if (TERMX%2 == 1) {
            TERMX--;
        }

        if (TERMY%2 == 0) {
            TERMY--;
        }

        TERRAIN_WINDOW_WIDTH = (TERMX - STATUS_WIDTH > 121) ? 121 : TERMX - STATUS_WIDTH;
        TERRAIN_WINDOW_HEIGHT = (TERMY > 121) ? 121 : TERMY;

        VIEW_OFFSET_X = (TERMX - STATUS_WIDTH > 121) ? (TERMX - STATUS_WIDTH - 121)/2 : 0;
        VIEW_OFFSET_Y = (TERMY > 121) ? (TERMY - 121)/2 : 0;

        VIEWX = (TERRAIN_WINDOW_WIDTH - 1) / 2;
        VIEWY = (TERRAIN_WINDOW_HEIGHT - 1) / 2;
    #endif

    if(VIEWX < 12)
	VIEWX = 12;

    if(VIEWY < 12)
	VIEWY = 12;


    // Set up the main UI windows.
    w_terrain = newwin(TERRAIN_WINDOW_HEIGHT, TERRAIN_WINDOW_WIDTH, VIEW_OFFSET_Y, VIEW_OFFSET_X);
    werase(w_terrain);

    w_minimap = newwin(MINIMAP_HEIGHT, MINIMAP_WIDTH, VIEW_OFFSET_Y, TERMX - MONINFO_WIDTH - MINIMAP_WIDTH - VIEW_OFFSET_X);
    werase(w_minimap);

    w_HP = newwin(HP_HEIGHT, HP_WIDTH, VIEW_OFFSET_Y + MINIMAP_HEIGHT, TERMX - MESSAGES_WIDTH - HP_WIDTH - VIEW_OFFSET_X);
    werase(w_HP);

    w_moninfo = newwin(MONINFO_HEIGHT, MONINFO_WIDTH, VIEW_OFFSET_Y, TERMX - MONINFO_WIDTH - VIEW_OFFSET_X);
    werase(w_moninfo);

    w_messages = newwin(MESSAGES_HEIGHT, MESSAGES_WIDTH, MONINFO_HEIGHT + VIEW_OFFSET_Y, TERMX - MESSAGES_WIDTH - VIEW_OFFSET_X);
    werase(w_messages);

    w_location = newwin(LOCATION_HEIGHT, LOCATION_WIDTH, MONINFO_HEIGHT+MESSAGES_HEIGHT + VIEW_OFFSET_Y, TERMX - LOCATION_WIDTH - VIEW_OFFSET_X);
    werase(w_location);

    w_status = newwin(STATUS_HEIGHT + (OPTIONS[OPT_VIEWPORT_Y]-12)*2, STATUS_WIDTH, MONINFO_HEIGHT+MESSAGES_HEIGHT+LOCATION_HEIGHT + VIEW_OFFSET_Y, TERMX - STATUS_WIDTH - VIEW_OFFSET_X);
    werase(w_status);

    w_void = newwin(TERMY-(MONINFO_HEIGHT+MESSAGES_HEIGHT+LOCATION_HEIGHT+STATUS_HEIGHT), STATUS_WIDTH, MONINFO_HEIGHT+MESSAGES_HEIGHT+LOCATION_HEIGHT+STATUS_HEIGHT + VIEW_OFFSET_Y, TERMX - STATUS_WIDTH - VIEW_OFFSET_X);
    werase(w_void);
}

void game::setup()
{
 u = player();
 m = map(&itypes, &mapitems, &traps); // Init the root map with our vectors

//CAT-mgs: check this
 z.reserve(1000); // Reserve some space
// active_npc.reserve(100);

// Even though we may already have 'd', nextinv will be incremented as needed
 nextinv = 'd';
 next_npc_id = 1;
 next_faction_id = 1;
 next_mission_id = 1;
// Clear monstair values
 monstairx = 0;
 monstairy = 0;
 monstairz = 0;
 last_target = -1;	// We haven't targeted any monsters yet

 curmes = 0;		// We haven't read any messages yet
 uquit = QUIT_NO;	// We haven't quit the game
 debugmon = false;	// We're not printing debug messages

// ... Unless data/no_npc.txt exists.
 std::ifstream ifile("data/no_npc.txt");
 if(ifile)
	no_npc = true;
 else
	no_npc = false;

//CAT-mgs:
  SNIPER= false;
  CARJUMPED= false;
  cat_lightning= false;

// Start with some nice weather... 
 weather = WEATHER_CLEAR; // WEATHER_LIGHTNING WEATHER_SNOWSTORM WEATHER_ACID_RAIN 
 nextweather = MINUTES(STARTING_MINUTES + 30); // Weather shift in 30

//CAT-s: 
 turnssincelastmon = 100; //Auto safe mode init
 autosafemode = OPTIONS[OPT_AUTOSAFEMODE];

 footsteps.clear();
 z.clear();
 coming_to_stairs.clear();
 active_npc.clear();
 factions.clear();
 active_missions.clear();
 items_dragged.clear();
 messages.clear();
 events.clear();

 turn.season = SUMMER;    // ... with winter conveniently a long ways off

 for (int i = 0; i < num_monsters; i++)	// Reset kill counts to 0
  kills[i] = 0;
// Set the scent map to 0
 for (int i = 0; i < SEEX * MAPSIZE; i++) {
  for (int j = 0; j < SEEX * MAPSIZE; j++)
   grscent[i][j] = 0;
 }

 if (opening_screen())
 {
	// Finally, draw the screen!
	refresh_all();
	draw();
 }
}

// Set up all default values for a new game
void game::start_game()
{

 popup_nowait("Please wait as we build your world");
// Init some factions.
 if (!load_master())	// Master data record contains factions.
  create_factions();
 cur_om = overmap(this, 0, 0);	// We start in the (0,0,0) overmap.
// Find a random house on the map, and set us there.
 cur_om.first_house(levx, levy);
 levx -= int(int(MAPSIZE / 2) / 2);
 levy -= int(int(MAPSIZE / 2) / 2);
 levz = 0;
// Start the overmap with out immediate neighborhood visible
 for (int i = -15; i <= 15; i++) {
  for (int j = -15; j <= 15; j++)
   cur_om.seen(levx + i, levy + j, 0) = true;
 }

// Convert the overmap coordinates to submap coordinates
 levx = levx * 2 - 1;
 levy = levy * 2 - 1;
 set_adjacent_overmaps(true);
// Init the starting map at this location.
 m.load(this, levx, levy, levz);
// Start us off somewhere in the shelter.
 u.posx = SEEX * int(MAPSIZE / 2) + 5;
 u.posy = SEEY * int(MAPSIZE / 2) + 6;

//CAT-mgs:
 ltar_x= u.posx;
 ltar_y= u.posy;

 u.str_cur = u.str_max;
 u.per_cur = u.per_max;
 u.int_cur = u.int_max;
 u.dex_cur = u.dex_max;

//CAT-mgs:
 turn = HOURS(OPTIONS[OPT_INITIAL_TIME]);
 run_mode = (OPTIONS[OPT_SAFEMODE] ? 1 : 0);
 mostseen = 0;	// ...and mostseen is 0, we haven't seen any monsters yet.

//CAT-mgs:
  if(OPTIONS[OPT_STATIC_SPAWN])
	nextspawn= (int)turn+5;
 else
	nextspawn= (int)turn+900;

 temperature = 65; // Springtime-appropriate?


//CAT-mgs: no NPCs
 create_starting_npcs();

 MAPBUFFER.set_dirty();

//CAT-g: some message to clear message windown
 add_msg("Zombies, huh?");
}

void game::create_factions()
{
 int num = dice(4, 3);
 faction tmp(0);
 tmp.make_army();
 factions.push_back(tmp);
 for (int i = 0; i < num; i++) {
  tmp = faction(assign_faction_id());
  tmp.randomize();
  tmp.likes_u = 100;
  tmp.respects_u = 100;
  tmp.known_by_u = true;
  factions.push_back(tmp);
 }
}

void game::create_starting_npcs()
{
 npc tmp;
 tmp.normalize(this);
 tmp.randomize(this, (one_in(2) ? NC_DOCTOR : NC_NONE));
 tmp.spawn_at(&cur_om, levx, levy);
 tmp.posx = SEEX * int(MAPSIZE / 2) + SEEX;
 tmp.posy = SEEY * int(MAPSIZE / 2) + 6;
 tmp.form_opinion(&u);
 tmp.attitude = NPCATT_NULL;
 tmp.mission = NPC_MISSION_SHELTER;
 tmp.chatbin.first_topic = TALK_SHELTER;
 tmp.chatbin.missions.push_back(
     reserve_random_mission(ORIGIN_OPENER_NPC, om_location(), tmp.id) );

 active_npc.push_back(tmp);
}

//CAT-mgs: *** whole lot ***
void game::cleanup_at_end()
{
 write_msg();
 refresh_all();

// Save the monsters before we die!
 despawn_monsters();
 if(uquit == QUIT_SUICIDE || uquit == QUIT_DIED)
 {
//CAT-s:
	stopLoop(-99);
	stopMusic(1); 

//CAT-s: need some pause (to have available channel)?, otherwise die sound (11) might not play
	timespec ts;
	ts.tv_sec= 1;
	ts.tv_nsec= 0;

	nanosleep(&ts, NULL);

	if(u.oxygen < 5 && u.underwater)
//CAT-s: drowned-arghh
	      playSound(7);
	else
//CAT-s: died-arghh
	      playSound(11);
	
	popup_top("GAME OVER");
//	popup_top("SNAAAAAAAAAAAAAAKE! - Game over.");

//CAT-s:
	playSound(1);	
	playSound(12);
	death_screen();
 }

 if(gamemode){
  delete gamemode;
  gamemode = new special_game;	// null gamemode or something..
 }
}


// MAIN GAME LOOP
// Returns true if game is over (death, saved, quit, etc)
bool game::do_turn()
{
 if (is_game_over()) {
  cleanup_at_end();
  return true;
 }
// Actual stuff
 gamemode->per_turn(this);
 turn.increment();
 process_events();
 process_missions();

 if(turn.hour == 0 && turn.minute == 0 && turn.second == 0) // Midnight!
  cur_om.process_mongroups();

// Check if we've overdosed... in any deadly way.
 if (u.stim > 250) {
  add_msg("You have a sudden heart attack!");
  u.hp_cur[hp_torso] = 0;
 } else if (u.stim < -200 || u.pkill > 240) {
  add_msg("Your breathing stops completely.");
  u.hp_cur[hp_torso] = 0;
 }

 if (turn % 50 == 0) {	// Hunger, thirst, & fatigue up every 5 minutes
  if ((!u.has_trait(PF_LIGHTEATER) || !one_in(3)) &&
      (!u.has_bionic(bio_recycler) || turn % 300 == 0))
   u.hunger++;
  if ((!u.has_bionic(bio_recycler) || turn % 100 == 0) &&
      (!u.has_trait(PF_PLANTSKIN) || !one_in(5)))
   u.thirst++;
  u.fatigue++;
  if (u.fatigue == 192 && !u.has_disease(DI_LYING_DOWN) &&
      !u.has_disease(DI_SLEEP)) {
   if (u.activity.type == ACT_NULL)
    add_msg("You're feeling tired.  Press '$' to lie down for sleep.");
   else
    cancel_activity_query("You're feeling tired.");
  }
  if (u.stim < 0)
   u.stim++;
  if (u.stim > 0)
   u.stim--;
  if (u.pkill > 0)
   u.pkill--;
  if (u.pkill < 0)
   u.pkill++;
  if (u.has_bionic(bio_solar) && is_in_sunlight(u.posx, u.posy))
   u.charge_power(1);
 }
 if (turn % 300 == 0) {	// Pain up/down every 30 minutes
  if (u.pain > 0)
   u.pain -= 1 + int(u.pain / 10);
  else if (u.pain < 0)
   u.pain++;
// Mutation healing effects
  if (u.has_trait(PF_FASTHEALER2) && one_in(5))
   u.healall(1);
  if (u.has_trait(PF_REGEN) && one_in(2))
   u.healall(1);
  if (u.has_trait(PF_ROT2) && one_in(5))
   u.hurtall(1);
  if (u.has_trait(PF_ROT3) && one_in(2))
   u.hurtall(1);

  if (u.radiation > 1 && one_in(3))
   u.radiation--;
  u.get_sick(this);
 }



// Update the weather, if it's time.
 if (turn >= nextweather)
  update_weather();

 if(levz >= 0)
 {
	weather_effect weffect;
	(weffect.*(weather_data[weather].effect))(this);
 }

//CAT-mgs: check this
// The following happens when we stay still; 10/40 minutes overdue for spawn
 if ((!u.has_trait(PF_INCONSPICUOUS) && turn > nextspawn +  100) ||
     ( u.has_trait(PF_INCONSPICUOUS) && turn > nextspawn +  400)   ) {

  spawn_mon(-1 + 2 * rng(0, 1), -1 + 2 * rng(0, 1));
  nextspawn = turn;
 }

 while(u.moves > 0)
 {
	  cleanup_dead();

	  if(!u.has_disease(DI_SLEEP) && u.activity.type == ACT_NULL)
		draw();

	  if(handle_action())
		  ++moves_since_last_save;

	  if(is_game_over())
	  {
		   cleanup_at_end();
		   return true;
	  }
 }

 u.reset(this);
 u.update_bodytemp(this);
 u.process_active_items(this);
 u.suffer(this);

 m.process_fields(this);
 m.process_active_items(this);
 m.step_in_field(u.posx, u.posy, this);
 
 if(u.has_disease(DI_SLEEP))
 {
	if(turn%20 == 0)
		draw();
	else
		return false;
 }
 else
 if(u.activity.type != ACT_NULL)
 {
	process_activity();

	if(turn%30 == 0)
		draw();
	else
		return false;
 }

//printf("\nTurn1: %d", int(turn));
 cleanup_dead();
 m.vehmove(this);

 monmove();
 update_stair_monsters();
//printf(" - Turn2");


// rustCheck();
 if(turn%10 == 0)
	u.update_morale();

 if(turn%2 == 0)
	 update_scent();

//CAT-MGS:
 wrefresh(0, true);
 return false;
}


void game::rustCheck() {
  if (OPTIONS[OPT_SKILL_RUST] == 2)
    return;

  for (std::vector<Skill*>::iterator aSkill = Skill::skills.begin()++;
       aSkill != Skill::skills.end(); ++aSkill) {
    int skillLevel = u.skillLevel(*aSkill);
    int forgetCap = skillLevel > 7 ? 7 : skillLevel;

    if (skillLevel > 0 && turn % (8192 / int(pow(2, double(forgetCap - 1)))) == 0) {
      if (rng(1,12) % (u.has_trait(PF_FORGETFUL) ? 3 : 4)) {
        if (u.has_bionic(bio_memory) && u.power_level > 0) {
          if (one_in(5))
            u.power_level--;
        } else {
          if (OPTIONS[OPT_SKILL_RUST] == 0 || u.skillLevel(*aSkill).exercise() > 0) {
            int newLevel;
            u.skillLevel(*aSkill).rust(newLevel);

            if (newLevel < skillLevel) {
              add_msg("Your skill in %s has reduced to %d!", (*aSkill)->name().c_str(), newLevel);
            }
          }
        }
      }
    }
  }
}

void game::process_events()
{
 for (int i = 0; i < events.size(); i++) {
  events[i].per_turn(this);
  if (events[i].turn <= int(turn)) {
   events[i].actualize(this);
   events.erase(events.begin() + i);
   i--;
  }
 }
}

void game::process_activity()
{
 it_book* reading;
 if (u.activity.type != ACT_NULL)
 {

  if (u.activity.type == ACT_WAIT) {	// Based on time, not speed
   u.activity.moves_left -= 100;
   u.pause(this);

  } else if (u.activity.type == ACT_REFILL_VEHICLE) {
   vehicle *veh = m.veh_at( u.activity.placement.x, u.activity.placement.y );
   if (!veh) {  // Vehicle must've moved or something!
    u.activity.moves_left = 0;
    return;
   }

   veh->refill (AT_GAS, 200);
   if(one_in(30)) {
     // Scan for the gas pump we're refuelling from and deactivate it.
    for(int i = -1; i <= 1; i++)
     for(int j = -1; j <= 1; j++)
      if(m.ter(u.posx + i, u.posy + j) == t_gas_pump) {
       add_msg("With a clang and a shudder, the gas pump goes silent.");
       m.ter(u.posx + i, u.posy + j) = t_gas_pump_empty;
       u.activity.moves_left = 0;
       // Found it, break out of the loop.
       i = 2;
       j = 2;
       break;
      }
   }
   u.pause(this);
   u.activity.moves_left -= 100;
  } else {
   u.activity.moves_left -= u.moves;
   u.moves = 0;
  }

  if (u.activity.moves_left <= 0) {	// We finished our activity!

   switch (u.activity.type) {

   case ACT_RELOAD:
    if (u.weapon.reload(u, u.activity.index))
     if (u.weapon.is_gun() && u.weapon.has_flag(IF_RELOAD_ONE)) {
      add_msg("You insert a cartridge into your %s.",
              u.weapon.tname(this).c_str());
      if (u.recoil < 8)
       u.recoil = 8;
      if (u.recoil > 8)
       u.recoil = (8 + u.recoil) / 2;
     } else {
      add_msg("You reload your %s.", u.weapon.tname(this).c_str());
      u.recoil = 6;
     }
    else
     add_msg("Can't reload your %s.", u.weapon.tname(this).c_str());
    break;

   case ACT_READ:
    if (u.activity.index == -2)
     reading = dynamic_cast<it_book*>(u.weapon.type);
    else
     reading = dynamic_cast<it_book*>(u.inv[u.activity.index].type);

    if (reading->fun != 0) {
     std::stringstream morale_text;
     u.add_morale(MORALE_BOOK, reading->fun * 5, reading->fun * 15, reading);
    }

    if (u.skillLevel(reading->type) < reading->level) {
     int min_ex = reading->time / 10 + u.int_cur / 4,
       max_ex = reading->time /  5 + u.int_cur / 2 - u.skillLevel(reading->type);
     if (min_ex < 1)
      min_ex = 1;
     if (max_ex < 2)
      max_ex = 2;
     if (max_ex > 10)
      max_ex = 10;

     int originalSkillLevel = u.skillLevel(reading->type);
     u.skillLevel(reading->type).readBook(min_ex, max_ex, reading->level);

     add_msg("You learn a little about %s! (%d%%%%)", reading->type->name().c_str(), u.skillLevel(reading->type).exercise());

     if (u.skillLevel(reading->type) > originalSkillLevel)
      add_msg("You increase %s to level %d.",
              reading->type->name().c_str(),
              (int)u.skillLevel(reading->type));

     if (u.skillLevel(reading->type) == reading->level)
      add_msg("You can no longer learn from %s.", reading->name.c_str());
    }
    break;

   case ACT_WAIT:
    add_msg("You finish waiting.");
    playSound(2);
    break;

   case ACT_CRAFT:
    complete_craft();
    break;

   case ACT_DISASSEMBLE:
    complete_disassemble();
    break;

   case ACT_BUTCHER:
    complete_butcher(u.activity.index);
    break;

   case ACT_FORAGE:
    forage();
    break;

   case ACT_BUILD:
    complete_construction();
    break;

   case ACT_TRAIN:
    if (u.activity.index < 0) {
     add_msg("You learn %s.", itypes[0 - u.activity.index]->name.c_str());
     u.styles.push_back( itype_id(0 - u.activity.index) );
    } else {
     u.sklevel[ u.activity.index ]++;

     int skillLevel = u.skillLevel(u.activity.index);
     u.skillLevel(u.activity.index).level(skillLevel + 1);
     add_msg("You finish training %s to level %d.",
             skill_name(u.activity.index).c_str(),
             (int)u.skillLevel(u.activity.index));
    }
    break;

   case ACT_VEHICLE:
    complete_vehicle (this);
    break;
   }

   bool act_veh = (u.activity.type == ACT_VEHICLE);
   u.activity.type = ACT_NULL;
   if (act_veh) {
    if (u.activity.values.size() < 7)
    {
/*
     dbg(D_ERROR) << "game:process_activity: invalid ACT_VEHICLE values: "
                  << u.activity.values.size();
     debugmsg ("process_activity invalid ACT_VEHICLE values:%d",
                u.activity.values.size());
*/
    }
    else {
     vehicle *veh = m.veh_at(u.activity.values[0], u.activity.values[1]);
     if (veh) {
      exam_vehicle(*veh, u.activity.values[0], u.activity.values[1],
                         u.activity.values[2], u.activity.values[3]);
      return;
     } else
     {
/*
      dbg(D_ERROR) << "game:process_activity: ACT_VEHICLE: vehicle not found";
      debugmsg ("process_activity ACT_VEHICLE: vehicle not found");
*/
     }
    }
   }
  }
 }
}

void game::cancel_activity()
{
 u.cancel_activity();
}

void game::cancel_activity_query(const char* message, ...)
{
 char buff[1024];
 va_list ap;
 va_start(ap, message);
 vsprintf(buff, message, ap);
 va_end(ap);
 std::string s(buff);

 bool doit = false;;

 switch (u.activity.type) {
  case ACT_NULL:
   doit = false;
   break;
  case ACT_READ:
   if (query_yn("%s Stop reading?", s.c_str()))
    doit = true;
   break;
  case ACT_RELOAD:
   if (query_yn("%s Stop reloading?", s.c_str()))
    doit = true;
   break;
  case ACT_CRAFT:
   if (query_yn("%s Stop crafting?", s.c_str()))
    doit = true;
   break;
  case ACT_DISASSEMBLE:
   if (query_yn("%s Stop disassembly?", s.c_str()))
    doit = true;
   break;
  case ACT_BUTCHER:
   if (query_yn("%s Stop butchering?", s.c_str()))
    doit = true;
   break;
  case ACT_FORAGE:
   if (query_yn("%s Stop foraging?", s.c_str()))
    doit = true;
   break;
  case ACT_BUILD:
  case ACT_VEHICLE:
   if (query_yn("%s Stop construction?", s.c_str()))
    doit = true;
   break;
  case ACT_REFILL_VEHICLE:
   if (query_yn("%s Stop pumping gas?", s.c_str()))
    doit = true;
   break;
  case ACT_TRAIN:
   if (query_yn("%s Stop training?", s.c_str()))
    doit = true;
   break;
  default:
   doit = true;
 }

 if (doit)
  u.cancel_activity();
}

void game::update_weather()
{
 season_type season = turn.season;
// Pick a new weather type (most likely the same one)
 int chances[NUM_WEATHER_TYPES];
 int total = 0;
 for (int i = 0; i < NUM_WEATHER_TYPES; i++) {
// Reduce the chance for freezing-temp-only weather to 0 if it's above freezing
// and vice versa.
  if ((weather_data[i].avg_temperature[season] < 32 && temperature > 32) ||
      (weather_data[i].avg_temperature[season] > 32 && temperature < 32)   )
   chances[i] = 0;
  else {
   chances[i] = weather_shift[season][weather][i];
   if (weather_data[i].dangerous && u.has_artifact_with(AEP_BAD_WEATHER))
    chances[i] = chances[i] * 4 + 10;
   total += chances[i];
  }
 }
 int choice = rng(0, total - 1);
 weather_type old_weather = weather;
 weather_type new_weather = WEATHER_CLEAR;

 if (total > 0) {
  while (choice >= chances[new_weather]) {
   choice -= chances[new_weather];
   new_weather = weather_type(int(new_weather) + 1);
  }
 } else {
  new_weather = weather_type(int(new_weather) + 1);
 }
// Advance the weather timer
 int minutes = rng(weather_data[new_weather].mintime,
                   weather_data[new_weather].maxtime);
 nextweather = turn + MINUTES(minutes);
 weather = new_weather;


 if (weather != old_weather && weather_data[weather].dangerous &&
     levz >= 0 && m.is_outside(u.posx, u.posy)) {
  std::stringstream weather_text;
  weather_text << "The weather changed to " << weather_data[weather].name <<
                  "!";
  cancel_activity_query(weather_text.str().c_str());
 }

// Now update temperature
 if (!one_in(4)) { // 3 in 4 chance of respecting avg temp for the weather
  int average = weather_data[weather].avg_temperature[season];
  if (temperature < average)
   temperature++;
  else if (temperature > average)
   temperature--;
 } else // 1 in 4 chance of random walk
  temperature += rng(-1, 1);

//CAT-mgs:
 if (turn.is_night())
  temperature += rng(-3, 1);
 else
  temperature += rng(-1, 3);
}

int game::assign_mission_id()
{
 int ret = next_mission_id;
 next_mission_id++;
 return ret;
}

void game::give_mission(mission_id type)
{
 mission tmp = mission_types[type].create(this);
 active_missions.push_back(tmp);
 u.active_missions.push_back(tmp.uid);
 u.active_mission = u.active_missions.size() - 1;
 mission_start m_s;
 mission *miss = find_mission(tmp.uid);
 (m_s.*miss->type->start)(this, miss);
}

void game::assign_mission(int id)
{
 u.active_missions.push_back(id);
 u.active_mission = u.active_missions.size() - 1;
 mission_start m_s;
 mission *miss = find_mission(id);
 (m_s.*miss->type->start)(this, miss);
}

int game::reserve_mission(mission_id type, int npc_id)
{
 mission tmp = mission_types[type].create(this, npc_id);
 active_missions.push_back(tmp);
 return tmp.uid;
}

int game::reserve_random_mission(mission_origin origin, point p, int npc_id)
{
 std::vector<int> valid;
 mission_place place;
 for (int i = 0; i < mission_types.size(); i++) {
  for (int j = 0; j < mission_types[i].origins.size(); j++) {
   if (mission_types[i].origins[j] == origin &&
       (place.*mission_types[i].place)(this, p.x, p.y)) {
    valid.push_back(i);
    j = mission_types[i].origins.size();
   }
  }
 }

 if (valid.empty())
  return -1;

 int index = valid[rng(0, valid.size() - 1)];

 return reserve_mission(mission_id(index), npc_id);
}

npc* game::find_npc(int id)
{
 for (int i = 0; i < active_npc.size(); i++) {
  if (active_npc[i].id == id)
   return &(active_npc[i]);
 }
 for (int i = 0; i < cur_om.npcs.size(); i++) {
  if (cur_om.npcs[i].id == id)
   return &(cur_om.npcs[i]);
 }
 return NULL;
}

mission* game::find_mission(int id)
{
 for (int i = 0; i < active_missions.size(); i++) {
  if (active_missions[i].uid == id)
   return &(active_missions[i]);
 }
 return NULL;
}

mission_type* game::find_mission_type(int id)
{
 for (int i = 0; i < active_missions.size(); i++) {
  if (active_missions[i].uid == id)
   return active_missions[i].type;
 }
 return NULL;
}

bool game::mission_complete(int id, int npc_id)
{
 mission* miss = find_mission(id);
 if (miss == NULL) {
/*
  dbg(D_ERROR) << "game:mission_complete: " << id << " - it's NULL!";
  debugmsg("game::mission_complete(%d) - it's NULL!", id);
*/
  return false;
 }
 mission_type* type = miss->type;
 switch (type->goal) {
  case MGOAL_GO_TO: {
   point cur_pos(levx + int(MAPSIZE / 2), levy + int(MAPSIZE / 2));
   if (rl_dist(cur_pos.x, cur_pos.y, miss->target.x, miss->target.y) <= 1)
    return true;
   return false;
  } break;

  case MGOAL_FIND_ITEM:
   if (!u.has_amount(type->item_id, 1))
    return false;
   if (miss->npc_id != -1 && miss->npc_id != npc_id)
    return false;
   return true;

  case MGOAL_FIND_ANY_ITEM:
   return (u.has_mission_item(miss->uid) &&
           (miss->npc_id == -1 || miss->npc_id == npc_id));

  case MGOAL_FIND_MONSTER:
   if (miss->npc_id != -1 && miss->npc_id != npc_id)
    return false;
   for (int i = 0; i < z.size(); i++) {
    if (z[i].mission_id == miss->uid)
     return true;
   }
   return false;

  case MGOAL_FIND_NPC:
   return (miss->npc_id == npc_id);

  case MGOAL_KILL_MONSTER:
   return (miss->step >= 1);

  default:
   return false;
 }
 return false;
}

bool game::mission_failed(int id)
{
 mission *miss = find_mission(id);
 return (miss->failed);
}

void game::wrap_up_mission(int id)
{
 mission *miss = find_mission(id);
 u.completed_missions.push_back( id );
 for (int i = 0; i < u.active_missions.size(); i++) {
  if (u.active_missions[i] == id) {
   u.active_missions.erase( u.active_missions.begin() + i );
   i--;
  }
 }
 switch (miss->type->goal) {
  case MGOAL_FIND_ITEM:
   u.use_amount(miss->type->item_id, 1);
   break;
  case MGOAL_FIND_ANY_ITEM:
   u.remove_mission_items(miss->uid);
   break;
 }
 mission_end endfunc;
 (endfunc.*miss->type->end)(this, miss);
}

void game::fail_mission(int id)
{
 mission *miss = find_mission(id);
 miss->failed = true;
 u.failed_missions.push_back( id );
 for (int i = 0; i < u.active_missions.size(); i++) {
  if (u.active_missions[i] == id) {
   u.active_missions.erase( u.active_missions.begin() + i );
   i--;
  }
 }
 mission_fail failfunc;
 (failfunc.*miss->type->fail)(this, miss);
}

void game::mission_step_complete(int id, int step)
{
 mission *miss = find_mission(id);
 miss->step = step;
 switch (miss->type->goal) {
  case MGOAL_FIND_ITEM:
  case MGOAL_FIND_MONSTER:
  case MGOAL_KILL_MONSTER: {
   bool npc_found = false;
   for (int i = 0; i < cur_om.npcs.size(); i++) {
    if (cur_om.npcs[i].id == miss->npc_id) {
     miss->target = point(cur_om.npcs[i].mapx, cur_om.npcs[i].mapy);
     npc_found = true;
    }
   }
   if (!npc_found)
    miss->target = point(-1, -1);
  } break;
 }
}

void game::process_missions()
{
 for (int i = 0; i < active_missions.size(); i++) {
  if (active_missions[i].deadline > 0 &&
      int(turn) > active_missions[i].deadline)
   fail_mission(active_missions[i].uid);
 }
}

bool game::handle_action()
{
  char ch = input();
  if (keymap.find(ch) == keymap.end()) {
	  if (ch != ' ' && ch != '\n')
		  add_msg("Unknown command: '%c'", ch);
	  return false;
  }

 action_id act = keymap[ch];

// This has no action unless we're in a special game mode.
 gamemode->pre_action(this, act);

 int veh_part;
 vehicle *veh = m.veh_at(u.posx, u.posy, veh_part);
 bool veh_ctrl = veh && veh->player_in_control (&u);

 switch (act) {

  case ACTION_PAUSE:
   if (run_mode > 1) // Monsters around and we don't wanna pause
   {
	if(run_mode==3)
		add_msg("Monster spotted you!");

//CAT-s:
	playSound(10);
   }
   else
   {
    if(u.in_vehicle)
	pldrive(0, 0);
    else
	u.pause(this);
   }
   break;

  case ACTION_MOVE_N:
   if (u.in_vehicle)
    pldrive(0, -1);
   else
    plmove(0, -1);
   break;

  case ACTION_MOVE_NE:
   if (u.in_vehicle)
    pldrive(1, -1);
   else
    plmove(1, -1);
   break;

  case ACTION_MOVE_E:
   if (u.in_vehicle)
    pldrive(1, 0);
   else
    plmove(1, 0);
   break;

  case ACTION_MOVE_SE:
   if (u.in_vehicle)
    pldrive(1, 1);
   else
    plmove(1, 1);
   break;

  case ACTION_MOVE_S:
   if (u.in_vehicle)
    pldrive(0, 1);
   else
   plmove(0, 1);
   break;

  case ACTION_MOVE_SW:
   if (u.in_vehicle)
    pldrive(-1, 1);
   else
    plmove(-1, 1);
   break;

  case ACTION_MOVE_W:
   if (u.in_vehicle)
    pldrive(-1, 0);
   else
    plmove(-1, 0);
   break;

  case ACTION_MOVE_NW:
   if (u.in_vehicle)
    pldrive(-1, -1);
   else
    plmove(-1, -1);
   break;

  case ACTION_MOVE_DOWN:
   if (!u.in_vehicle)
    vertical_move(-1, false);
   break;

  case ACTION_MOVE_UP:
   if (!u.in_vehicle)
    vertical_move( 1, false);
   break;

  case ACTION_CENTER:
   u.view_offset_x = 0;
   u.view_offset_y = 0;
   break;

  case ACTION_SHIFT_N:
   u.view_offset_y += -1;
   break;

  case ACTION_SHIFT_NE:
   u.view_offset_x += 1;
   u.view_offset_y += -1;
   break;

  case ACTION_SHIFT_E:
   u.view_offset_x += 1;
   break;

  case ACTION_SHIFT_SE:
   u.view_offset_x += 1;
   u.view_offset_y += 1;
   break;

  case ACTION_SHIFT_S:
   u.view_offset_y += 1;
   break;

  case ACTION_SHIFT_SW:
   u.view_offset_x += -1;
   u.view_offset_y += 1;
   break;

  case ACTION_SHIFT_W:
   u.view_offset_x += -1;
   break;

  case ACTION_SHIFT_NW:
   u.view_offset_x += -1;
   u.view_offset_y += -1;
   break;

  case ACTION_OPEN:
   open();
   break;

  case ACTION_CLOSE:
   close();
   break;

  case ACTION_SMASH:
   if (veh_ctrl)
    handbrake();
   else
    smash();
   break;

  case ACTION_EXAMINE:
   examine();
   break;

  case ACTION_PICKUP:
   pickup(u.posx, u.posy, 1);
   break;

  case ACTION_BUTCHER:
   butcher();
//CAT-s: butcher sound
   playSound(28);

//CAT-s: cut sound
//   playSound(54);
   break;

  case ACTION_CHAT:
//CAT-s: 
   playSound(1);
   chat();
   playSound(2);
   break;

  case ACTION_LOOK:
//CAT-s: 
   playSound(1);
   look_around();
   playSound(2);
   break;

  case ACTION_PEEK:
//CAT-s: 
   playSound(1);
   peek();
   break;

  case ACTION_LIST_ITEMS:
//CAT-s: 
   playSound(1);
   list_items();
   playSound(2);
   break;

  case ACTION_INVENTORY: 
  {

//CAT-s: 
   playSound(1);

   bool has = false;
   char cMenu = ' ';
   do {
    const std::string sSpaces = "                              ";
    char chItem = inv();
    cMenu = '+';
    has = u.has_item(chItem);

    if (has) {
     item oThisItem = u.i_at(chItem);
     std::vector<iteminfo> vThisItem, vDummy, vMenu;

     vMenu.push_back(iteminfo("MENU", "", "iOffsetX", 3));
     vMenu.push_back(iteminfo("MENU", "", "iOffsetY", 0));
     vMenu.push_back(iteminfo("MENU", "a", "ctivate"));
     vMenu.push_back(iteminfo("MENU", "R", "ead"));
     vMenu.push_back(iteminfo("MENU", "E", "at"));
     vMenu.push_back(iteminfo("MENU", "W", "ear"));
     vMenu.push_back(iteminfo("MENU", "w", "ield"));
     vMenu.push_back(iteminfo("MENU", "t", "hrow"));
     vMenu.push_back(iteminfo("MENU", "T", "ake off"));
     vMenu.push_back(iteminfo("MENU", "d", "rop"));
     vMenu.push_back(iteminfo("MENU", "U", "nload"));
     vMenu.push_back(iteminfo("MENU", "r", "eload"));

     oThisItem.info(true, &vThisItem);
     compare_split_screen_popup(0, 59, TERMY-VIEW_OFFSET_Y*2, oThisItem.tname(this), vThisItem, vDummy);

//CAT-mgs:
     cMenu = compare_split_screen_popup(59, 21, 25, "", vMenu, vDummy);

     switch(cMenu) {
      case 'a':
       use_item(chItem);
       break;
      case 'E':
       eat(chItem);
       break;
      case 'W':
       wear(chItem);
       break;
      case 'w':
       wield(chItem);
       break;
      case 't':
       plthrow(chItem);
       break;
      case 'T':
       takeoff(chItem);
       break;
      case 'd':
       drop(chItem);
       break;
      case 'U':
       unload(chItem);
       break;
      case 'r':
       reload(chItem);
       break;
      case 'R':
       u.read(this, chItem);
       break;
      default:
       break;
     }
    }
   } while (cMenu == ' ' || cMenu == '.' || cMenu == 'q' || cMenu == '\n' || cMenu == KEY_ESCAPE);

//CAT-s: 
   playSound(2);

  } break;

  case ACTION_COMPARE:
   compare();
//CAT-s: 
   playSound(2);
   break;

  case ACTION_ORGANIZE:
//CAT-s: 
   playSound(1);
   reassign_item();
   playSound(2);
   break;

  case ACTION_USE:
//CAT-s: 
   playSound(1);
   use_item();
//CAT-s: goes inside use_item()
//   playSound(6);
   break;

  case ACTION_USE_WIELDED:
//CAT-s: 
//   playSound(1);
   use_wielded_item();
   break;

  case ACTION_WEAR:
//CAT-s: 
   playSound(1);
   wear();
   playSound(6);
   break;

  case ACTION_TAKE_OFF:
//CAT-s: 
   playSound(1);
   takeoff();
   playSound(33);
   break;

  case ACTION_EAT:
//CAT-s: 
   playSound(1);
   eat();
//CAT-s: handle in eat(), below, or u.eat()? ...and what about the rest?
   playSound(6);
   break;

  case ACTION_READ:
//CAT-s: 
   playSound(1);
   read();
   playSound(6);
   break;

  case ACTION_WIELD:
//CAT-s: 
  playSound(1);
   wield();
   playSound(34);
   break;

  case ACTION_PICK_STYLE:
//CAT-s: 
   playSound(1);
   u.pick_style(this);
   if (u.weapon.type->id == 0 || u.weapon.is_style()) {
    u.weapon = item(itypes[u.style_selected], 0);
    u.weapon.invlet = ':';
   }
   playSound(6);
   break;

  case ACTION_RELOAD:
//CAT-s: 
   playSound(35);
   reload();
   break;

  case ACTION_UNLOAD:
//CAT-s: 
   playSound(33);
   unload();
   break;

  case ACTION_THROW:
//CAT-s: 
   playSound(1);
   plthrow();
   break;

  case ACTION_FIRE:
   plfire(false);
   u.view_offset_x= 0;
   u.view_offset_y= 0;
   SNIPER= false;
   break;

  case ACTION_KICK:
//CAT-mgs: it's now runJump or Kick
	playSound(1);
	runJump(true);

/*
   plfire(true);
   u.view_offset_x= 0;
   u.view_offset_y= 0;
   SNIPER= false;
*/
   break;

  case ACTION_SELECT_FIRE_MODE:
//CAT-s: 
   playSound(33);
   u.weapon.next_mode();
   break;

  case ACTION_DROP:
   drop();
   break;

  case ACTION_DIR_DROP:
   drop_in_direction();
   break;

  case ACTION_BIONICS:
//CAT-s:
   playSound(1);
   u.power_bionics(this);
   playSound(2);
   break;

  case ACTION_WAIT:
//CAT-s:
   playSound(1);
   wait();
   playSound(2);
   break;

  case ACTION_CRAFT:
//CAT-s:
   playSound(1);
   craft();
   playSound(2);
   break;

  case ACTION_RECRAFT:
   recraft();
   break;

  case ACTION_DISASSEMBLE:
   if (u.in_vehicle)
    add_msg("You can't disassemble items while in vehicle.");
   else
   {
//CAT-s:
	playSound(1);
	disassemble();
	playSound(2);
   }
   break;

  case ACTION_CONSTRUCT:
   if (u.in_vehicle)
    add_msg("You can't construct while in vehicle.");
   else
   {
//CAT-s:
	playSound(1);
	construction_menu();
	playSound(2);
   }
   break;

  case ACTION_SLEEP:
//CAT-s:
   playSound(1);
   if (veh_ctrl) {
    std::string message = veh->use_controls();
    if (!message.empty())
     add_msg(message.c_str());
   } else if (query_yn("Are you sure you want to sleep?")) {
    u.try_to_sleep(this);
    u.moves = 0;
   }
//CAT-s:
	playSound(2);
   break;

  case ACTION_TOGGLE_SAFEMODE:

/*
   if (run_mode == 0 ) {
    run_mode = 1;
    mostseen = 0;
    add_msg("Safe mode ON!");
   } else {
    turnssincelastmon = 0;
    run_mode = 0;
   }
*/
   break;

  case ACTION_TOGGLE_AUTOSAFE:
//CAT-mgs:
   autosafemode = true;
/*
   if (autosafemode) {
    add_msg("Auto safe mode OFF!");
    autosafemode = false;
   } else {
    add_msg("Auto safe mode ON");
    autosafemode = true;
   }
*/
   break;

  case ACTION_IGNORE_ENEMY:
/*
   if (run_mode == 2) {
    add_msg("Ignoring enemy!");
    run_mode = 1;
   }
*/
   break;

//CAT-s: ***
  case ACTION_SAVE:
	if (!u.in_vehicle)
	{
		if (query_yn("Save and quit?"))
		{
			save();
			u.moves = 0;
			uquit = QUIT_SAVED;
			MAPBUFFER.make_volatile();
		}
		break;
	}
	else 
  	   add_msg("Saving in vehicles is buggy, stop and get out of the vehicle first");
  break;

  case ACTION_QUIT:
   if (query_yn("Commit suicide?")) {
    u.moves = 0;
    std::vector<item> tmp = u.inv_dump();
    item your_body;
    your_body.make_corpse(itypes[itm_corpse], mtypes[mon_null], turn);
    your_body.name = u.name;
    m.add_item(u.posx, u.posy, your_body);
    for (int i = 0; i < tmp.size(); i++)
     m.add_item(u.posx, u.posy, tmp[i]);
    uquit = QUIT_SUICIDE;

//CAT-s: not here? 
//	stopMusic(0);
//	playSound(11);
   }
   break;

  case ACTION_PL_INFO:
//CAT-s:
   playSound(1);
   u.disp_info(this);
   playSound(2);
   break;

  case ACTION_MAP:
//CAT-s:
   playSound(1);
   draw_overmap();
   playSound(2);
   break;

  case ACTION_MISSIONS:
//CAT-s:
   playSound(1);
   list_missions();
   playSound(2);
   break;

  case ACTION_KILLS:
//CAT-s:
   playSound(1);
   disp_kills();
   playSound(2);
   break;

  case ACTION_FACTIONS:
//CAT-s:
   playSound(1);
   list_factions();
   playSound(2);
   break;

  case ACTION_MORALE:
//CAT-s:
   playSound(1);
   u.disp_morale(this);
   playSound(2);
   break;

  case ACTION_MESSAGES:
//CAT-s:
   playSound(1);
   msg_buffer();
   playSound(2);
   break;

  case ACTION_HELP:
//CAT-s:
   playSound(1);
   help();
   playSound(2);
   break;

  case ACTION_DEBUG:
//CAT-s:
   playSound(1);
   debug();
   playSound(2);
   break;

  case ACTION_DISPLAY_SCENT:
//CAT-s:
   playSound(1);
   display_scent();
   playSound(2);
   break;

  case ACTION_JUMP:
	playSound(1);
	runJump(false);
   break;
 }

 gamemode->post_action(this, act);

 return true;
}

#define SCENT_RADIUS 40

int& game::scent(int x, int y)
{
  if (x < (SEEX * MAPSIZE / 2) - SCENT_RADIUS || x >= (SEEX * MAPSIZE / 2) + SCENT_RADIUS ||
      y < (SEEY * MAPSIZE / 2) - SCENT_RADIUS || y >= (SEEY * MAPSIZE / 2) + SCENT_RADIUS) {
  nulscent = 0;
  return nulscent;	// Out-of-bounds - null scent
 }
 return grscent[x][y];
}

void game::update_scent()
{
 signed int newscent[SEEX * MAPSIZE][SEEY * MAPSIZE];
 if (!u.has_active_bionic(bio_scent_mask))
  grscent[u.posx][u.posy] = u.scent;
 else
  grscent[u.posx][u.posy] = 0;

 for (int x = u.posx - SCENT_RADIUS; x <= u.posx + SCENT_RADIUS; x++) {
  for (int y = u.posy - SCENT_RADIUS; y <= u.posy + SCENT_RADIUS; y++) {
   newscent[x][y] = 0;
   if (m.move_cost(x, y) != 0 || m.has_flag(bashable, x, y)) {
    int squares_used = 0;
    for (int i = -1; i <= 1; i++) {
     for (int j = -1; j <= 1; j++) {
      if (grscent[x][y] <= grscent[x+i][y+j]) {
       newscent[x][y] += grscent[x+i][y+j];
       squares_used++;
      }
     }
    }
    newscent[x][y] /= (squares_used + 1);
    if (m.field_at(x, y).type == fd_slime &&
        newscent[x][y] < 10 * m.field_at(x, y).density)
     newscent[x][y] = 10 * m.field_at(x, y).density;
    if (newscent[x][y] > 10000) {
/*
     dbg(D_ERROR) << "game:update_scent: Wacky scent at " << x << ","
                  << y << " (" << newscent[x][y] << ")";
     debugmsg("Wacky scent at %d, %d (%d)", x, y, newscent[x][y]);
*/
     newscent[x][y] = 0; // Scent should never be higher
    }
   }
  }
 }
 for (int x = u.posx - SCENT_RADIUS; x <= u.posx + SCENT_RADIUS; x++) {
  for (int y = u.posy - SCENT_RADIUS; y <= u.posy + SCENT_RADIUS; y++)
   if(m.move_cost(x, y) == 0)
    //Greatly reduce scent for bashable barriers, even more for ductaped barriers
    if (m.has_flag(reduce_scent, x, y))
     grscent[x][y] = newscent[x][y] / 12;
    else
     grscent[x][y] = newscent[x][y] / 4;
   else
    grscent[x][y] = newscent[x][y];
 }
 if (!u.has_active_bionic(bio_scent_mask))
  grscent[u.posx][u.posy] = u.scent;
 else
  grscent[u.posx][u.posy] = 0;
}

bool game::is_game_over()
{
 if (uquit != QUIT_NO)
  return true;
 for (int i = 0; i <= hp_torso; i++) {
  if (u.hp_cur[i] < 1) {
   std::vector<item> tmp = u.inv_dump();
   item your_body;
   your_body.make_corpse(itypes[itm_corpse], mtypes[mon_null], turn);
   your_body.name = u.name;
   m.add_item(u.posx, u.posy, your_body);
   for (int i = 0; i < tmp.size(); i++)
    m.add_item(u.posx, u.posy, tmp[i]);
   
//CAT-mgs: don't erase save file, first one
//   std::stringstream playerfile;
//   playerfile << "save/" << u.name << ".sav";
//   unlink(playerfile.str().c_str());

   uquit = QUIT_DIED;
   return true;
  }
 }
 return false;
}

void game::death_screen()
{
    gamemode->game_over(this);

//CAT-mgs: don't erase save file, second one
//   std::stringstream playerfile;
//   playerfile << "save/" << u.name << ".sav";
//   unlink(playerfile.str().c_str());

//CAT-mgs:
/*
    const std::string sText = "GAME OVER - Press Spacebar to Quit";

    WINDOW *w_death = newwin(5, 6+sText.size(), (TERMY-5)/2, (TERMX+6-sText.size())/2);

    wborder(w_death, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                     LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );

    mvwprintz(w_death, 2, 3, c_ltred, sText.c_str());
    wrefresh(w_death);
    refresh();
    InputEvent input;
    do
        input = get_input();
    while(input != Cancel && input != Close && input != Confirm);
    delwin(w_death);
*/

    disp_kills();
}


bool game::load_master()
{
 std::ifstream fin;
 std::string data;
 char junk;
 fin.open("save/master.gsav");
 if (!fin.is_open())
  return false;

// First, get the next ID numbers for each of these
 fin >> next_mission_id >> next_faction_id >> next_npc_id;
 int num_missions, num_npc, num_factions, num_items;

 fin >> num_missions;
 if (fin.peek() == '\n')
  fin.get(junk); // Chomp that pesky endline
 for (int i = 0; i < num_missions; i++) {
  mission tmpmiss;
  tmpmiss.load_info(this, fin);
  active_missions.push_back(tmpmiss);
 }

 fin >> num_factions;
 if (fin.peek() == '\n')
  fin.get(junk); // Chomp that pesky endline
 for (int i = 0; i < num_factions; i++) {
  getline(fin, data);
  faction tmp;
  tmp.load_info(data);
  factions.push_back(tmp);
 }
// NPCs come next
 fin >> num_npc;
 if (fin.peek() == '\n')
  fin.get(junk); // Chomp that pesky endline
 for (int i = 0; i < num_npc; i++) {
  getline(fin, data);
  npc tmp;
  tmp.load_info(this, data);
// We need to load up all their items too
  fin >> num_items;
  std::vector<item> tmpinv;
  for (int j = 0; j < num_items; j++) {
   std::string itemdata;
   char item_place;
   fin >> item_place;
   if (!fin.eof()) {
    getline(fin, itemdata);
    if (item_place == 'I')
     tmpinv.push_back(item(itemdata, this));
    else if (item_place == 'C' && !tmpinv.empty()) {
     tmpinv[tmpinv.size() - 1].contents.push_back(item(itemdata, this));
     j--;
    } else if (item_place == 'W')
     tmp.worn.push_back(item(itemdata, this));
    else if (item_place == 'w')
     tmp.weapon = item(itemdata, this);
    else if (item_place == 'c') {
     tmp.weapon.contents.push_back(item(itemdata, this));
     j--;
    }
   }
  }
  tmp.inv.add_stack(tmpinv);
  active_npc.push_back(tmp);
  if (fin.peek() == '\n')
   fin.get(junk); // Chomp that pesky endline
 }

 fin.close();
 return true;
}

void game::load(std::string name)
{
 std::ifstream fin;
 std::stringstream playerfile;
 playerfile << "save/" << name << ".sav";
 fin.open(playerfile.str().c_str());
// First, read in basic game state information.
 if (!fin.is_open()) {
/*
  dbg(D_ERROR) << "game:load: No save game exists!";
  debugmsg("No save game exists!");
*/
  return;
 }
 u = player();
 u.name = name;
 u.ret_null = item(itypes[0], 0);
 u.weapon = item(itypes[0], 0);
 int tmpturn, tmpspawn, tmpnextweather, tmprun, tmptar, tmpweather, tmptemp,
     comx, comy;
 fin >> tmpturn >> tmptar >> tmprun >> mostseen >> nextinv >> next_npc_id >>
        next_faction_id >> next_mission_id >> tmpspawn >> tmpnextweather >>
        tmpweather >> tmptemp >> levx >> levy >> levz >> comx >> comy;
 turn = tmpturn;
 nextspawn = tmpspawn;
 nextweather = tmpnextweather;

 cur_om = overmap(this, comx, comy);
 m.load(this, levx, levy, levz);

 run_mode = tmprun;
 if (OPTIONS[OPT_SAFEMODE] && run_mode == 0)
  run_mode = 1;
 autosafemode = OPTIONS[OPT_AUTOSAFEMODE];
 last_target = tmptar;
 weather = weather_type(tmpweather);
 temperature = tmptemp;
// Next, the scent map.
 for (int i = 0; i < SEEX * MAPSIZE; i++) {
  for (int j = 0; j < SEEY * MAPSIZE; j++)
   fin >> grscent[i][j];
 }
// Now the number of monsters...
 int nummon;
 fin >> nummon;
// ... and the data on each one.
 std::string data;
 z.clear();
 monster montmp;
 char junk;
 if (fin.peek() == '\n')
  fin.get(junk); // Chomp that pesky endline
 for (int i = 0; i < nummon; i++) {
  getline(fin, data);
  montmp.load_info(data, &mtypes);
  z.push_back(montmp);
 }
// And the kill counts;
 if (fin.peek() == '\n')
  fin.get(junk); // Chomp that pesky endline
 for (int i = 0; i < num_monsters; i++)
  fin >> kills[i];
// Finally, the data on the player.
 if (fin.peek() == '\n')
  fin.get(junk); // Chomp that pesky endline
 getline(fin, data);
 u.load_info(this, data);
// And the player's inventory...
 char item_place;
 std::string itemdata;
// We need a temporary vector of items.  Otherwise, when we encounter an item
// which is contained in another item, the auto-sort/stacking behavior of the
// player's inventory may cause the contained item to be misplaced.
 std::vector<item> tmpinv;
 while (!fin.eof()) {
  fin >> item_place;
  if (!fin.eof()) {
   getline(fin, itemdata);
   if (item_place == 'I')
    tmpinv.push_back(item(itemdata, this));
   else if (item_place == 'C')
    tmpinv[tmpinv.size() - 1].contents.push_back(item(itemdata, this));
   else if (item_place == 'W')
    u.worn.push_back(item(itemdata, this));
   else if (item_place == 'w')
    u.weapon = item(itemdata, this);
   else if (item_place == 'c')
    u.weapon.contents.push_back(item(itemdata, this));
  }
 }
// Now dump tmpinv into the player's inventory
 u.inv.add_stack(tmpinv);
 fin.close();
// Now load up the master game data; factions (and more?)
 load_master();
 set_adjacent_overmaps(true);
 MAPBUFFER.set_dirty();


//CAT-mgs:
 ltar_x= u.posx;
 ltar_y= u.posy;

//CAT-g: some message to clear message windown
 add_msg("I'm all out of bubblegum...");

// draw();
}

void game::save()
{
 std::stringstream playerfile, masterfile;
 std::ofstream fout;
 playerfile << "save/" << u.name << ".sav";
 masterfile << "save/master.gsav";
 fout.open(playerfile.str().c_str());
// First, write out basic game state information.
 fout << int(turn) << " " << int(last_target) << " " << int(run_mode) << " " <<
         mostseen << " " << nextinv << " " << next_npc_id << " " <<
         next_faction_id << " " << next_mission_id << " " << int(nextspawn) <<
         " " << int(nextweather) << " " << weather << " " << int(temperature) <<
         " " << levx << " " << levy << " " << levz << " " << cur_om.pos().x <<
         " " << cur_om.pos().y << " " << std::endl;
// Next, the scent map.
 for (int i = 0; i < SEEX * MAPSIZE; i++) {
  for (int j = 0; j < SEEY * MAPSIZE; j++)
   fout << grscent[i][j] << " ";
 }
// Now save all monsters.
 fout << std::endl << z.size() << std::endl;
 for (int i = 0; i < z.size(); i++)
  fout << z[i].save_info() << std::endl;
 for (int i = 0; i < num_monsters; i++)	// Save the kill counts, too.
  fout << kills[i] << " ";
// And finally the player.
 fout << u.save_info() << std::endl;
 fout << std::endl;
 fout.close();

// Now write things that aren't player-specific: factions and NPCs
 fout.open(masterfile.str().c_str());

 fout << next_mission_id << " " << next_faction_id << " " << next_npc_id <<
         " " << active_missions.size() << " ";
 for (int i = 0; i < active_missions.size(); i++)
  fout << active_missions[i].save_info() << " ";

 fout << factions.size() << std::endl;
 for (int i = 0; i < factions.size(); i++)
  fout << factions[i].save_info() << std::endl;

 fout << active_npc.size() << std::endl;
 for (int i = 0; i < active_npc.size(); i++) {
  active_npc[i].mapx = levx;
  active_npc[i].mapy = levy;
  fout << active_npc[i].save_info() << std::endl;
 }

 fout.close();

// Finally, save artifacts.
 if (itypes.size() > num_all_items) {
  fout.open("save/artifacts.gsav");
  for (int i = num_all_items; i < itypes.size(); i++)
   fout << itypes[i]->save_data() << "\n";
  fout.close();
 }
// aaaand the overmap, and the local map.
 cur_om.save();
 m.save(&cur_om, turn, levx, levy, levz);
 MAPBUFFER.save();
}

void game::delete_save()
{
#if (defined _WIN32 || defined __WIN32__)
      WIN32_FIND_DATA FindFileData;
      HANDLE hFind;
      TCHAR Buffer[MAX_PATH];

      GetCurrentDirectory(MAX_PATH, Buffer);
      SetCurrentDirectory("save");
      hFind = FindFirstFile("*", &FindFileData);
      if(INVALID_HANDLE_VALUE != hFind) {
       do {
        DeleteFile(FindFileData.cFileName);
       } while(FindNextFile(hFind, &FindFileData) != 0);
       FindClose(hFind);
      }
      SetCurrentDirectory(Buffer);
#else
     DIR *save_dir = opendir("save");
     struct dirent *save_dirent = NULL;
     if(save_dir != NULL && 0 == chdir("save"))
     {
      while ((save_dirent = readdir(save_dir)) != NULL)
       (void)unlink(save_dirent->d_name);
      (void)chdir("..");
      (void)closedir(save_dir);
     }
#endif
}

void game::advance_nextinv()
{
 if (nextinv == 'z')
  nextinv = 'A';
 else if (nextinv == 'Z')
  nextinv = 'a';
 else
  nextinv++;
}

void game::decrease_nextinv()
{
 if (nextinv == 'a')
  nextinv = 'Z';
 else if (nextinv == 'A')
  nextinv = 'z';
 else
  nextinv--;
}

void game::vadd_msg(const char* msg, va_list ap)
{
 char buff[1024];
 vsprintf(buff, msg, ap);
 std::string s(buff);
 if (s.length() == 0)
  return;
 if (!messages.empty() && int(messages.back().turn) + 3 >= int(turn) &&
     s == messages.back().message) {
  messages.back().count++;
  messages.back().turn = turn;
  return;
 }

 if (messages.size() == 256)
  messages.erase(messages.begin());
 messages.push_back( game_message(turn, s) );
}

void game::add_msg(const char* msg, ...)
{
 va_list ap;
 va_start(ap, msg);
 vadd_msg(msg, ap);
 va_end(ap);
}

void game::add_msg_if_player(player *p, const char* msg, ...)
{
 if (p && !p->is_npc())
 {
  va_list ap;
  va_start(ap, msg);
  vadd_msg(msg, ap);
  va_end(ap);
 }
}

void game::add_event(event_type type, int on_turn, int faction_id, int x, int y)
{
 event tmp(type, on_turn, faction_id, x, y);
 events.push_back(tmp);
}

bool game::event_queued(event_type type)
{
 for (int i = 0; i < events.size(); i++) {
  if (events[i].type == type)
   return true;
  }
  return false;
}

void game::debug()
{
 int action = menu("Debug Functions - Using these is CHEATING!",
                   "Wish for an item",       // 1
                   "Teleport - Short Range", // 2
                   "Teleport - Long Range",  // 3
                   "Reveal map",             // 4
                   "Spawn NPC",              // 5
                   "Spawn Monster",          // 6
                   "Check game state...",    // 7
                   "Kill NPCs",              // 8
                   "Mutate",                 // 9
                   "Spawn a vehicle",        // 10
                   "Increase all skills",    // 11
                   "Learn all melee styles", // 12
                   "Check NPC",              // 13
                   "Spawn Artifact",         // 14
                   "Cancel",                 // 15
                   NULL);
 int veh_num;
 std::vector<std::string> opts;
 switch (action) {
  case 1:
   wish();
   break;

  case 2:
   teleport();
   break;

  case 3: {
   point tmp = cur_om.choose_point(this, levz);
   if (tmp.x != -1) {
    z.clear();
    levx = tmp.x * 2 - int(MAPSIZE / 2);
    levy = tmp.y * 2 - int(MAPSIZE / 2);
    set_adjacent_overmaps(true);
    m.load(this, levx, levy, levz);
   }
  } break;

  case 4:
//   debugmsg("%d radio towers", cur_om.radios.size());

   for (int i = 0; i < OMAPX; i++) {
    for (int j = 0; j < OMAPY; j++)
     cur_om.seen(i, j, levz) = true;
   }

//CAT-s:
   playSound(1);
   draw_overmap();

   break;

  case 5: {
   npc temp;
   temp.normalize(this);
   temp.randomize(this);
   temp.attitude = NPCATT_TALK;
   temp.spawn_at(&cur_om, levx + (1 * rng(-2, 2)), levy + (1 * rng(-2, 2)));
   temp.posx = u.posx - 4;
   temp.posy = u.posy - 4;
   temp.form_opinion(&u);
   temp.attitude = NPCATT_TALK;
   temp.mission = NPC_MISSION_NULL;
   int mission_index = reserve_random_mission(ORIGIN_ANY_NPC,
                                              om_location(), temp.id);
   if (mission_index != -1)
   temp.chatbin.missions.push_back(mission_index);
   active_npc.push_back(temp);
  } break;

  case 6:
   monster_wish();
   break;

  case 7:
   popup_top("\
Location %d:%d in %d:%d, %s\n\
Current turn: %d; Next spawn %d.\n\
NPCs are %s spawn.\n\
%d monsters exist.\n\
%d events planned.", u.posx, u.posy, levx, levy,
oterlist[cur_om.ter(levx / 2, levy / 2, levz)].name.c_str(),
int(turn), int(nextspawn), (no_npc ? "NOT going to" : "going to"),
z.size(), events.size());

   if (!active_npc.empty())
    popup_top("%s: %d:%d (you: %d:%d)", active_npc[0].name.c_str(),
              active_npc[0].posx, active_npc[0].posy, u.posx, u.posy);
   break;

  case 8:
   for (int i = 0; i < active_npc.size(); i++) {
    add_msg("%s's head implodes!", active_npc[i].name.c_str());
    active_npc[i].hp_cur[bp_head] = 0;
   }
   break;

  case 9:
   mutation_wish();
   break;

  case 10:
   if (m.veh_at(u.posx, u.posy)) {
/*
    dbg(D_ERROR) << "game:load: There's already vehicle here";
    debugmsg ("There's already vehicle here");
*/
   }
   else {
    for (int i = 2; i < vtypes.size(); i++)
     opts.push_back (vtypes[i]->name);
    opts.push_back (std::string("Cancel"));
    veh_num = menu_vec ("Choose vehicle to spawn", opts) + 1;
    if (veh_num > 1 && veh_num < num_vehicles)
     m.add_vehicle (this, (vhtype_id)veh_num, u.posx, u.posy, -90);
   }
   break;

  case 11:
    for (std::vector<Skill*>::iterator aSkill = Skill::skills.begin()++; aSkill != Skill::skills.end(); ++aSkill)
      u.skillLevel(*aSkill).level(u.skillLevel(*aSkill) + 3);
   break;

  case 12:
   for (int i = itm_style_karate; i <= itm_style_zui_quan; i++)
    u.styles.push_back( itype_id(i) );
   break;

  case 13: {
   point p = look_around();
   int npcdex = npc_at(p.x, p.y);
   if (npcdex == -1)
    popup("No NPC there.");
   else {
    std::stringstream data;
    npc *p = &(active_npc[npcdex]);
    data << p->name << " " << (p->male ? "Male" : "Female") << std::endl;
    data << npc_class_name(p->myclass) << "; " <<
            npc_attitude_name(p->attitude) << std::endl;
    if (p->has_destination())
     data << "Destination: " << p->goalx << ":" << p->goaly << "(" <<
             oterlist[ cur_om.ter(p->goalx, p->goaly, p->goalz) ].name << ")" <<
             std::endl;
    else
     data << "No destination." << std::endl;
    data << "Trust: " << p->op_of_u.trust << " Fear: " << p->op_of_u.fear <<
            " Value: " << p->op_of_u.value << " Anger: " << p->op_of_u.anger <<
            " Owed: " << p->op_of_u.owed << std::endl;
    data << "Aggression: " << int(p->personality.aggression) << " Bravery: " <<
            int(p->personality.bravery) << " Collector: " <<
            int(p->personality.collector) << " Altruism: " <<
            int(p->personality.altruism) << std::endl;
    for (std::vector<Skill*>::iterator aSkill = Skill::skills.begin()++; aSkill != Skill::skills.end(); ++aSkill) {
      data << (*aSkill)->name() << ": " << p->skillLevel(*aSkill) << std::endl;
    }

    full_screen_popup(data.str().c_str());
   }
  } break;

  case 14:
   point center = look_around();
   artifact_natural_property prop =
    artifact_natural_property(rng(ARTPROP_NULL + 1, ARTPROP_MAX - 1));
   m.create_anomaly(center.x, center.y, prop);
   m.add_item(center.x, center.y, new_natural_artifact(prop), 0);
   break;
 }
//CAT-g:
// erase();
// refresh_all();
}

void game::mondebug()
{
 int tc;
 for (int i = 0; i < z.size(); i++) {
  z[i].debug(u);

/*
  if (z[i].has_flag(MF_SEES) &&
      m.sees(z[i].posx, z[i].posy, u.posx, u.posy, -1, tc))
   debugmsg("The %s can see you.", z[i].name().c_str());
  else
   debugmsg("The %s can't see you...", z[i].name().c_str());
*/

 }
}

void game::groupdebug()
{
 erase();
 mvprintw(0, 0, "OM %d : %d    M %d : %d", cur_om.pos().x, cur_om.pos().y, levx,
                                           levy);
 int dist, linenum = 1;
 for (int i = 0; i < cur_om.zg.size(); i++) {
 	if (cur_om.zg[i].posz != levz) { continue; }
  dist = trig_dist(levx, levy, cur_om.zg[i].posx, cur_om.zg[i].posy);
  if (dist <= cur_om.zg[i].radius) {
   mvprintw(linenum, 0, "Zgroup %d: Centered at %d:%d, radius %d, pop %d",
            i, cur_om.zg[i].posx, cur_om.zg[i].posy, cur_om.zg[i].radius,
            cur_om.zg[i].population);
   linenum++;
  }
 }
 getch();
}

void game::draw_overmap()
{
	cur_om.choose_point(this, levz);
}

void game::disp_kills()
{
 WINDOW *w = newwin(25, 80, (TERMY > 25) ? (TERMY-25)/2 : 0, (TERMX > 80) ? (TERMX-80)/2 : 0);

 wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );

 std::vector<mtype *> types;
 std::vector<int> count;
 for (int i = 0; i < num_monsters; i++) {
  if (kills[i] > 0) {
   types.push_back(mtypes[i]);
   count.push_back(kills[i]);
  }
 }

 mvwprintz(w, 0, 2, c_red, "KILL COUNT:");

 if (types.size() == 0) {
  mvwprintz(w, 2, 4, c_white, "You haven't killed any monsters.");
  wrefresh(w);
  getch();
  werase(w);
  wrefresh(w);
  delwin(w);
  refresh_all();
  return;
 }

//CAT-mgs: *** vvv
 for (int i = 0; i < types.size(); i++) {
  if (i < 22) 
  {
   mvwprintz(w, i + 2, 2, types[i]->color, "%c %s", types[i]->sym, types[i]->name.c_str());
   int hori = 25;
   if (count[i] >= 10)
    hori = 24;
   if (count[i] >= 100)
    hori = 23;
   if (count[i] >= 1000)
    hori = 22;
   mvwprintz(w, i + 2, hori, c_white, "%d", count[i]);
  }
  else 
  if (i < 44) 
  {
   mvwprintz(w, i -20, 29, types[i]->color, "%c %s", types[i]->sym, types[i]->name.c_str());
   int hori = 52;
   if (count[i] >= 10)
    hori = 51;
   if (count[i] >= 100)
    hori = 50;
   if (count[i] >= 1000)
    hori = 49;
   mvwprintz(w, i -20, hori, c_white, "%d", count[i]);
  }
  else 
  {
   mvwprintz(w, i -42, 56, types[i]->color, "%c %s", types[i]->sym, types[i]->name.c_str());
   int hori = 77;
   if (count[i] >= 10)
    hori = 76;
   if (count[i] >= 100)
    hori = 75;
   if (count[i] >= 1000)
    hori = 74;
   mvwprintz(w, i -42, hori, c_white, "%d", count[i]);
  }

 }

 wrefresh(w);
 getch();
 werase(w);
 wrefresh(w);
 delwin(w);
 refresh_all();
}

void game::disp_NPCs()
{
//CAT-mgs: not used

 WINDOW *w = newwin(25, 80, (TERMY > 25) ? (TERMY-25)/2 : 0, (TERMX > 80) ? (TERMX-80)/2 : 0);

 mvwprintz(w, 0, 0, c_white, "Your position: %d:%d", levx, levy);
 std::vector<npc*> closest;
 closest.push_back(&cur_om.npcs[0]);
 for (int i = 1; i < cur_om.npcs.size(); i++) {
  if (closest.size() < 20)
   closest.push_back(&cur_om.npcs[i]);
  else if (rl_dist(levx, levy, cur_om.npcs[i].mapx, cur_om.npcs[i].mapy) <
           rl_dist(levx, levy, closest[19]->mapx, closest[19]->mapy)) {
   for (int j = 0; j < 20; j++) {
    if (rl_dist(levx, levy, closest[j]->mapx, closest[j]->mapy) >
        rl_dist(levx, levy, cur_om.npcs[i].mapx, cur_om.npcs[i].mapy)) {
     closest.insert(closest.begin() + j, &cur_om.npcs[i]);
     closest.erase(closest.end() - 1);
     j = 20;
    }
   }
  }
 }
 for (int i = 0; i < 20; i++)
  mvwprintz(w, i + 2, 0, c_white, "%s: %d:%d", closest[i]->name.c_str(),
            closest[i]->mapx, closest[i]->mapy);

 wrefresh(w);
 getch();
 werase(w);
 wrefresh(w);
 delwin(w);
}

faction* game::list_factions(std::string title)
{
 std::vector<faction> valfac;	// Factions that we know of.
 for (int i = 0; i < factions.size(); i++) {
  if (factions[i].known_by_u)
   valfac.push_back(factions[i]);
 }
 if (valfac.size() == 0) {	// We don't know of any factions!
  popup("You don't know of any factions.  Press Spacebar...");
  return NULL;
 }

 WINDOW *w_list = newwin(25, 80, ((TERMY > 25) ? (TERMY-25)/2 : 0), (TERMX > 80) ? (TERMX-80)/2 : 0);
 WINDOW *w_info = newwin(23, 79 - MAX_FAC_NAME_SIZE, 1 + ((TERMY > 25) ? (TERMY-25)/2 : 0), MAX_FAC_NAME_SIZE + ((TERMX > 80) ? (TERMX-80)/2 : 0));

 wborder(w_list, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                 LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );

 int maxlength = 79 - MAX_FAC_NAME_SIZE;
 int sel = 0;

// Init w_list content
 mvwprintz(w_list, 1, 1, c_white, title.c_str());
 for (int i = 0; i < valfac.size(); i++) {
  nc_color col = (i == 0 ? h_white : c_white);
  mvwprintz(w_list, i + 2, 1, col, valfac[i].name.c_str());
 }
 wrefresh(w_list);
// Init w_info content
// fac_*_text() is in faction.cpp
 mvwprintz(w_info, 0, 0, c_white,
          "Ranking: %s", fac_ranking_text(valfac[0].likes_u).c_str());
 mvwprintz(w_info, 1, 0, c_white,
          "Respect: %s", fac_respect_text(valfac[0].respects_u).c_str());
 std::string desc = valfac[0].describe();
 int linenum = 3;
 while (desc.length() > maxlength) {
  size_t split = desc.find_last_of(' ', maxlength);
  std::string line = desc.substr(0, split);
  mvwprintz(w_info, linenum, 0, c_white, line.c_str());
  desc = desc.substr(split + 1);
  linenum++;
 }
 mvwprintz(w_info, linenum, 0, c_white, desc.c_str());
 wrefresh(w_info);
 InputEvent input;
 do {
  input = get_input();
  switch ( input ) {
  case DirectionS:	// Move selection down
   mvwprintz(w_list, sel + 2, 1, c_white, valfac[sel].name.c_str());
   if (sel == valfac.size() - 1)
    sel = 0;	// Wrap around
   else
    sel++;
   break;
  case DirectionN:	// Move selection up
   mvwprintz(w_list, sel + 2, 1, c_white, valfac[sel].name.c_str());
   if (sel == 0)
    sel = valfac.size() - 1;	// Wrap around
   else
    sel--;
   break;
  case Cancel:
  case Close:
   sel = -1;
   break;
  }
  if (input == DirectionS || input == DirectionN) {	// Changed our selection... update the windows
   mvwprintz(w_list, sel + 2, 1, h_white, valfac[sel].name.c_str());
   wrefresh(w_list);
   werase(w_info);
// fac_*_text() is in faction.cpp
   mvwprintz(w_info, 0, 0, c_white,
            "Ranking: %s", fac_ranking_text(valfac[sel].likes_u).c_str());
   mvwprintz(w_info, 1, 0, c_white,
            "Respect: %s", fac_respect_text(valfac[sel].respects_u).c_str());
   std::string desc = valfac[sel].describe();
   int linenum = 3;
   while (desc.length() > maxlength) {
    size_t split = desc.find_last_of(' ', maxlength);
    std::string line = desc.substr(0, split);
    mvwprintz(w_info, linenum, 0, c_white, line.c_str());
    desc = desc.substr(split + 1);
    linenum++;
   }
   mvwprintz(w_info, linenum, 0, c_white, desc.c_str());
   wrefresh(w_info);
  }
 } while (input != Cancel && input != Confirm && input != Close);
 werase(w_list);
 werase(w_info);
 delwin(w_list);
 delwin(w_info);
 refresh_all();
 if (sel == -1)
  return NULL;
 return &(factions[valfac[sel].id]);
}

void game::list_missions()
{
 WINDOW *w_missions = newwin(25, 80, (TERMY > 25) ? (TERMY-25)/2 : 0, (TERMX > 80) ? (TERMX-80)/2 : 0);

 int tab = 0, selection = 0;
 InputEvent input;
 do {
  werase(w_missions);
  //draw_tabs(w_missions, tab, "ACTIVE MISSIONS", "COMPLETED MISSIONS", "FAILED MISSIONS", NULL);
  std::vector<int> umissions;
  switch (tab) {
   case 0: umissions = u.active_missions;	break;
   case 1: umissions = u.completed_missions;	break;
   case 2: umissions = u.failed_missions;	break;
  }

  for (int i = 1; i < 79; i++) {
   mvwputch(w_missions, 2, i, c_ltgray, LINE_OXOX);
   mvwputch(w_missions, 24, i, c_ltgray, LINE_OXOX);

   if (i > 2 && i < 24) {
    mvwputch(w_missions, i, 0, c_ltgray, LINE_XOXO);
    mvwputch(w_missions, i, 30, c_ltgray, LINE_XOXO);
    mvwputch(w_missions, i, 79, c_ltgray, LINE_XOXO);
   }
  }

  draw_tab(w_missions, 7, "ACTIVE MISSIONS", (tab == 0) ? true : false);
  draw_tab(w_missions, 30, "COMPLETED MISSIONS", (tab == 1) ? true : false);
  draw_tab(w_missions, 56, "FAILED MISSIONS", (tab == 2) ? true : false);

  mvwputch(w_missions, 2,  0, c_white, LINE_OXXO); // |^
  mvwputch(w_missions, 2, 79, c_white, LINE_OOXX); // ^|

  mvwputch(w_missions, 24, 0, c_ltgray, LINE_XXOO); // |_
  mvwputch(w_missions, 24, 79, c_ltgray, LINE_XOOX); // _|

  mvwputch(w_missions, 2, 30, c_white, (tab == 1) ? LINE_XOXX : LINE_XXXX); // + || -|
  mvwputch(w_missions, 24, 30, c_white, LINE_XXOX); // _|_

  for (int i = 0; i < umissions.size(); i++) {
   mission *miss = find_mission(umissions[i]);
   nc_color col = c_white;
   if (i == u.active_mission && tab == 0)
    col = c_ltred;
   if (selection == i)
    mvwprintz(w_missions, 3 + i, 1, hilite(col), miss->name().c_str());
   else
    mvwprintz(w_missions, 3 + i, 1, col, miss->name().c_str());
  }

  if (selection >= 0 && selection < umissions.size()) {
   mission *miss = find_mission(umissions[selection]);
   mvwprintz(w_missions, 4, 31, c_white,
             miss->description.c_str());
   if (miss->deadline != 0)
    mvwprintz(w_missions, 5, 31, c_white, "Deadline: %d (%d)",
              miss->deadline, int(turn));
   mvwprintz(w_missions, 6, 31, c_white, "Target: (%d, %d)   You: (%d, %d)",
             miss->target.x, miss->target.y,
             (levx + int (MAPSIZE / 2)) / 2, (levy + int (MAPSIZE / 2)) / 2);
  } else {
   std::string nope;
   switch (tab) {
    case 0: nope = "You have no active missions!"; break;
    case 1: nope = "You haven't completed any missions!"; break;
    case 2: nope = "You haven't failed any missions!"; break;
   }
   mvwprintz(w_missions, 4, 31, c_ltred, nope.c_str());
  }

  wrefresh(w_missions);
  input = get_input();
  switch (input) {
  case DirectionE:
   tab++;
   if (tab == 3)
    tab = 0;
   break;
  case DirectionW:
   tab--;
   if (tab < 0)
    tab = 2;
   break;
  case DirectionS:
   selection++;
   if (selection >= umissions.size())
    selection = 0;
   break;
  case DirectionN:
   selection--;
   if (selection < 0)
    selection = umissions.size() - 1;
   break;
  case Confirm:
   u.active_mission = selection;
   break;
  }

 } while (input != Cancel && input != Close);


 werase(w_missions);
 delwin(w_missions);
 refresh_all();
}


void game::draw()
{
//CAT-g:
// werase(w_terrain);

//CAT-mgs: range 60-71 
// add_msg("CHAR x: %d, y: %d", u.posx, u.posy); 

 mon_info();
 draw_HP();
 draw_minimap();
 write_msg();

 werase(w_status);
 u.disp_status(w_status, this);
// TODO: Allow for a 24-hour option--already supported by calendar turn
 mvwprintz(w_status, 1, 41, c_white, turn.print_time().c_str());

 oter_id cur_ter = cur_om.ter((levx + int(MAPSIZE / 2)) / 2,
                              (levy + int(MAPSIZE / 2)) / 2, levz);
 std::string tername = oterlist[cur_ter].name;

 if (tername.length() > 17)
  tername = tername.substr(0, 17);
 werase(w_location);
 mvwprintz(w_location, 0,  0, oterlist[cur_ter].color, tername.c_str());


//CAT-mgs: level check
/*
	cur_ter = cur_om.ter((levx + int(MAPSIZE / 2)) / 2,
                              (levy + int(MAPSIZE / 2)) / 2, 0);
	add_msg("t_zero is: %d", cur_ter);

	cur_ter = cur_om.ter((levx + int(MAPSIZE / 2)) / 2,
                              (levy + int(MAPSIZE / 2)) / 2, levz);
	add_msg("this is: %d", cur_ter);
*/


//CAT: *** vvv
 if(levz < 0)
  mvwprintz(w_location, 0, 19, c_ltgray, "Underground: %d", levz);
 else
 if (levz > 0)
  mvwprintz(w_location, 0, 19, c_white, "Aboveground: +%d", levz);
 else
 {
	if(weather == WEATHER_SUNNY && turn.is_night())
		weather= WEATHER_CLEAR;

	mvwprintz(w_location, 0, 19, 
		weather_data[weather].color, weather_data[weather].name.c_str());

	nc_color col_temp = c_blue;
	if (temperature >= 90)
 	  col_temp = c_red;
	else if (temperature >= 75)
 	  col_temp = c_yellow;
	else if (temperature >= 60)
 	  col_temp = c_white;
	else if (temperature >= 50)
 	  col_temp = c_ltblue;
	else if (temperature >  32)
	  col_temp = c_cyan;

	if (OPTIONS[OPT_USE_CELSIUS])
 	  wprintz(w_location, col_temp, " %dC", int((temperature - 32) / 1.8));
	else
	   wprintz(w_location, col_temp, " %dF", temperature);


//CAT-mgs:
	if(turn.is_night())
	{
	
//add_msg("Turn is night.");

		switch(turn.moon())
		{
			case 0:
				wprintz(w_location, c_blue, "  New Moon");
				break;

			case 1:
				wprintz(w_location, c_ltblue, "  Half Moon");
				break;

			case 2:
				wprintz(w_location, c_cyan, "  Full Moon");
				break;
		}
	}
 }

 wrefresh(w_location);
//CAT: *** ^^^


 mvwprintz(w_status, 0, 41, c_white, "%s, day %d",
           season_name[turn.season].c_str(), turn.day + 1);

 if (run_mode != 0 || autosafemode != 0) {
//CAT-MGS:
//  int iPercent = ((turnssincelastmon*100)/OPTIONS[OPT_AUTOSAFEMODETURNS]);
  int iPercent = int((turnssincelastmon*100)/50);

  mvwprintz(w_status, 2, 51, (run_mode == 0) ? ((iPercent >= 25) ? c_green : c_red): c_green, "S");
  wprintz(w_status, (run_mode == 0) ? ((iPercent >= 50) ? c_green : c_red): c_green, "A");
  wprintz(w_status, (run_mode == 0) ? ((iPercent >= 75) ? c_green : c_red): c_green, "F");
  wprintz(w_status, (run_mode == 0) ? ((iPercent == 100) ? c_green : c_red): c_green, "E");


 wrefresh(w_status);
 draw_ter();
// draw_footsteps();


//CAT-mgs: ************************* music loops ***************************
//**************************************************************************
  if(run_mode == 2)
	run_mode = 3;
  else
  if(run_mode == 3)
	run_mode = 4;
  else
  if(run_mode == 4)
	run_mode = 0;


//CAT-s: BEGIN *** vvv
// per turn bkg_music and monster sounds

	static int time2growl= 0;
	static bool oldOutside= false;

	if(one_in(5) && (iPercent < 10) && (time2growl++ > closeMonster*2))
	{
	   playSound(rng(13,22));
	   time2growl= 0;
	}

//CAT-mgs: effect loops
	vehicle *veh = m.veh_at (u.posx, u.posy);
	if(u.in_vehicle && CARJUMPED)
	{
		playLoop(51);
		loopVolume((int)(abs(veh->velocity*2)/100)+10);
	}
	else
	if(m.is_outside(u.posx, u.posy) && levz >= 0)
	{
		if(weather == WEATHER_DRIZZLE || weather == WEATHER_ACID_DRIZZLE)
			playLoop(64);
		else
		if(weather == WEATHER_RAINY || weather == WEATHER_ACID_RAIN)
			playLoop(65);
		else
		if(weather == WEATHER_THUNDER || weather == WEATHER_LIGHTNING) 
			playLoop(66);
		else
		if(weather == WEATHER_SNOWSTORM) 
			playLoop(67);
		else
			stopLoop(-99);
	}
	else
		stopLoop(-99);


//CAT-mgs: music loops
	if(u.underwater)
	{
		playMusic(7);
		stopLoop(-99);
	}
	else
	if(u.has_active_item(itm_mp3_on))
	{
		if(!mp3Track())
			mp3Next();
	}
	else
	if(iPercent > 0 && iPercent < 110)
		fadeMusic(-2);
	else
	if(run_mode == 0 && iPercent < 25)
	{
		fadeMusic(90);
		playMusic(0);
	}
	else
	if(!m.is_outside(u.posx, u.posy) || levz < 0)
	{
		if(levz < 0)
			playMusic(2);
		else
			playMusic(1);

//CAT-s: stepping outdoors->indoors
		if(oldOutside)
		{
			playSound(55);
			oldOutside= false;
		}
	}
	else
	if(m.is_outside(u.posx, u.posy))
	{
		if(levz < 0)
			playMusic(2);
		else
			playMusic(3);

//CAT-s: stepping indoors->outdoors
		if(!oldOutside)
		{
			playSound(55);
			oldOutside= true;
		}

	}

//CAT-s: END *** ^^^
 }
}

bool game::isBetween(int test, int down, int up)
{
	if(test>down && test<up) return true;
	else return false;
}



void game::draw_ter(int posx, int posy)
{
//CAT-mgs: moved down, then removed
// mapRain.clear();

// posx/posy default to -999
 if (posx == -999)
  posx = u.posx + u.view_offset_x;
 if (posy == -999)
  posy = u.posy + u.view_offset_y;

 int t = 0;

//CAT-g: check this, performance
 lm.generate(this, posx, posy, natural_light_level(), u.active_light());
 m.draw(this, w_terrain, point(posx, posy));

//CAT-g:
 bool night_vision= false;
 if( (u.is_wearing(itm_goggles_nv) && u.has_active_item(itm_UPS_on)) 
	|| u.has_active_bionic(bio_night_vision) )
    night_vision= true;


 // Draw monsters
 int distx, disty;
 for (int i = 0; i < z.size(); i++) {
  disty = abs(z[i].posy - posy);
  distx = abs(z[i].posx - posx);
  if(distx <= VIEWX && disty <= VIEWY && u_see(&(z[i]), t)) 
	z[i].draw(w_terrain, posx, posy, false, night_vision);
  else
  if(z[i].has_flag(MF_WARM) && distx <= VIEWX && disty <= VIEWY &&
           (u.has_active_bionic(bio_infrared) || u.has_trait(PF_INFRARED)))
	mvwputch(w_terrain, VIEWY + z[i].posy - posy, 
			VIEWX + z[i].posx - posx, c_red, '?');
 }


//CAT-mgs: no NPCs - new draw
 for (int i = 0; i < active_npc.size(); i++) { 
  disty = abs(active_npc[i].posy - posy);
  distx = abs(active_npc[i].posx - posx);


//CAT-mgs: active_npc[i].posz == levz && 
  if( active_npc[i].posz == levz
		&& distx < VIEWX && disty < VIEWY 
		&& u_see(active_npc[i].posx, active_npc[i].posy, t) )
	active_npc[i].draw(w_terrain, posx, posy, false, night_vision);
 }


 if (u.has_active_bionic(bio_scent_vision)) {
  for (int realx = posx - VIEWX; realx <= posx + VIEWX; realx++) {
   for (int realy = posy - VIEWY; realy <= posy + VIEWY; realy++) {
    if (scent(realx, realy) != 0) {
     int tempx = posx - realx, tempy = posy - realy;
     if (!(isBetween(tempx, -2, 2) && isBetween(tempy, -2, 2))) {
      if (mon_at(realx, realy) != -1)
       mvwputch(w_terrain, realy + VIEWY - posy, realx + VIEWX - posx,
                c_white, '?');
      else
       mvwputch(w_terrain, realy + VIEWY - posy, realx + VIEWX - posx,
                c_magenta, '#');
     }
    }
   }
  }
 }

//CAT-mgs: weather drops
// ...this should go to 'weather.cpp' 
 if(levz >= 0)
 {
	  float fFactor = 0.01;
	  char cGlyph = ',';
	  nc_color colGlyph = c_ltblue;
	  bool bWeatherEffect = true;
	  switch(weather) 
	  {
      	  case WEATHER_ACID_DRIZZLE:
	            cGlyph = ',';
            	colGlyph = c_ltgreen;
      	      fFactor = 0.01;
	            break;
      	  case WEATHER_ACID_RAIN:
	            cGlyph = ',';
	            colGlyph = c_ltgreen;
	            fFactor = 0.03;
      	      break;
	        case WEATHER_DRIZZLE:
      	      cGlyph = ',';
	            colGlyph = c_ltblue;
      	      fFactor = 0.01;
	            break;
      	  case WEATHER_RAINY:
	            cGlyph = ',';
      	      colGlyph = c_ltblue;
	            fFactor = 0.04;
      	      break;
	        case WEATHER_THUNDER:
	            cGlyph = ',';
      	      colGlyph = c_ltblue;
	            fFactor = 0.05;
      	      break;
	        case WEATHER_LIGHTNING:
      	      cGlyph = ',';
	            colGlyph = c_ltblue;
      	      fFactor = 0.03;
	            break;
//CAT-mgs:
      	  case WEATHER_FLURRIES:
	            cGlyph = '.';
      	      colGlyph = c_white;
	            fFactor = 0.01;
      	      break;
      	  case WEATHER_SNOW:
	            cGlyph = '*';
      	      colGlyph = c_white;
	            fFactor = 0.03;
      	      break;
	        case WEATHER_SNOWSTORM:
      	      cGlyph = '*';
	            colGlyph = c_white;
            	fFactor = 0.05;
      	      break;
	        default:
            	bWeatherEffect = false;
      	      break;
	  }

	  if(bWeatherEffect)
	  {
		int iStartX = (TERRAIN_WINDOW_WIDTH > 121) ? (TERRAIN_WINDOW_WIDTH-121)/2 : 0;
		int iStartY = (TERRAIN_WINDOW_HEIGHT > 121) ? (TERRAIN_WINDOW_HEIGHT-121)/2: 0;
		int iEndX = (TERRAIN_WINDOW_WIDTH > 121) ? TERRAIN_WINDOW_WIDTH-(TERRAIN_WINDOW_WIDTH-121)/2: TERRAIN_WINDOW_WIDTH;
		int iEndY = (TERRAIN_WINDOW_HEIGHT > 121) ? TERRAIN_WINDOW_HEIGHT-(TERRAIN_WINDOW_HEIGHT-121)/2: TERRAIN_WINDOW_HEIGHT;

//CAT-g:
		if( (u.is_wearing(itm_goggles_nv) && u.has_active_item(itm_UPS_on)) 
				|| u.has_active_bionic(bio_night_vision) )
			colGlyph = c_ltgreen;

		int dropCount = iEndX * iEndY * fFactor;
		for(int i=0; i < dropCount; i++)
		{
			int iRandX = rng(iStartX, iEndX-1);
			int iRandY = rng(iStartY, iEndY-1);

			if( (iRandX!=VIEWX && iRandY!=VIEWY)
					&& m.is_outside(u.posx+(iRandX-VIEWX), u.posy+(iRandY-VIEWY)) )
				mvwputch(w_terrain, iRandY, iRandX, colGlyph, cGlyph);
		}
	  }
 } 
 
 if (u.has_disease(DI_VISUALS) || (u.has_disease(DI_HOT_HEAD) && u.disease_intensity(DI_HOT_HEAD) != 1))
   hallucinate(posx, posy);

 draw_footsteps();

//CAT-g: moved here
 wrefresh(w_terrain);

//CAT-mgs:
 wrefresh(0, true);

}


void game::refresh_all()
{
 m.reset_vehicle_cache();

 draw();

// wrefresh(w_moninfo);
// wrefresh(w_messages);
// wrefresh(w_terrain);

//CAT-g:
// werase(w_void);
// wrefresh(w_void);

//CAT-g: needed on Lin but not Win?
// refresh();

}

void game::draw_HP()
{
    werase(w_HP);
    int current_hp;
    nc_color color;
    std::string asterisks = "";
    for (int i = 0; i < num_hp_parts; i++) {
        current_hp = u.hp_cur[i];
        if (current_hp == u.hp_max[i]){
          color = c_green;
          asterisks = " **** ";
        } else if (current_hp > u.hp_max[i] * .8) {
          color = c_ltgreen;
          asterisks = " **** ";
        } else if (current_hp > u.hp_max[i] * .5) {
          color = c_yellow;
          asterisks = " ***  ";
        } else if (current_hp > u.hp_max[i] * .3) {
          color = c_ltred;
          asterisks = " **   ";
        } else {
          color = c_red;
          asterisks = " *    ";
        }
        if (u.has_trait(PF_SELFAWARE)) {
            if (current_hp >= 100){
                mvwprintz(w_HP, i * 2 + 1, 0, color, "%d     ", current_hp);
            } else if (current_hp >= 10) {
                mvwprintz(w_HP, i * 2 + 1, 0, color, " %d    ", current_hp);
            } else {
                mvwprintz(w_HP, i * 2 + 1, 0, color, "  %d    ", current_hp);
            }
        } else {
            mvwprintz(w_HP, i * 2 + 1, 0, color, asterisks.c_str());
        }
    }
    mvwprintz(w_HP,  0, 0, c_ltgray, "HEAD:  ");
    mvwprintz(w_HP,  2, 0, c_ltgray, "TORSO: ");
    mvwprintz(w_HP,  4, 0, c_ltgray, "L ARM: ");
    mvwprintz(w_HP,  6, 0, c_ltgray, "R ARM: ");
    mvwprintz(w_HP,  8, 0, c_ltgray, "L LEG: ");
    mvwprintz(w_HP, 10, 0, c_ltgray, "R LEG: ");
    mvwprintz(w_HP, 12, 0, c_ltgray, "POW:   ");
    if (u.max_power_level == 0){
        mvwprintz(w_HP, 13, 0, c_ltgray, " --   ");
    } else {
        if (u.power_level == u.max_power_level){
            color = c_blue;
        } else if (u.power_level >= u.max_power_level * .5){
            color = c_ltblue;
        } else if (u.power_level > 0){
            color = c_yellow;
        } else {
            color = c_red;
        }
        if (u.power_level >= 100){
            mvwprintz(w_HP, 13, 0, color, "%d     ", u.power_level);
        } else if (u.power_level >= 10){
            mvwprintz(w_HP, 13, 0, color, " %d    ", u.power_level);
        } else {
            mvwprintz(w_HP, 13, 0, color, "  %d    ", u.power_level);
        }
    }
    wrefresh(w_HP);
}


void game::draw_minimap()
{

//CAT-g: needed on Lin but not Win?
werase(w_minimap);

 mvwputch(w_minimap, 0, 0, c_white, LINE_OXXO);
 mvwputch(w_minimap, 0, 6, c_white, LINE_OOXX);
 mvwputch(w_minimap, 6, 0, c_white, LINE_XXOO);
 mvwputch(w_minimap, 6, 6, c_white, LINE_XOOX);
 for (int i = 1; i < 6; i++) {
  mvwputch(w_minimap, i, 0, c_white, LINE_XOXO);
  mvwputch(w_minimap, i, 6, c_white, LINE_XOXO);
  mvwputch(w_minimap, 0, i, c_white, LINE_OXOX);
  mvwputch(w_minimap, 6, i, c_white, LINE_OXOX);
 }

 int cursx = (levx + int(MAPSIZE / 2)) / 2;
 int cursy = (levy + int(MAPSIZE / 2)) / 2;

 bool drew_mission = false;
 point target(-1, -1);
 if (u.active_mission >= 0 && u.active_mission < u.active_missions.size())
  target = find_mission(u.active_missions[u.active_mission])->target;
 else
  drew_mission = true;

 if (target.x == -1)
  drew_mission = true;

 for (int i = -2; i <= 2; i++) {
  for (int j = -2; j <= 2; j++) {
   int omx = cursx + i;
   int omy = cursy + j;
   bool seen = false;
   oter_id cur_ter;
   if (omx >= 0 && omx < OMAPX && omy >= 0 && omy < OMAPY) {
    cur_ter = cur_om.ter(omx, omy, levz);
    seen    = cur_om.seen(omx, omy, levz);
   } else if ((omx < 0 || omx >= OMAPX) && (omy < 0 || omy >= OMAPY)) {
    if (omx < 0) omx += OMAPX;
    else         omx -= OMAPX;
    if (omy < 0) omy += OMAPY;
    else         omy -= OMAPY;
    cur_ter = om_diag->ter(omx, omy, levz);
    seen    = om_diag->seen(omx, omy, levz);
   } else if (omx < 0 || omx >= OMAPX) {
    if (omx < 0) omx += OMAPX;
    else         omx -= OMAPX;
    cur_ter = om_hori->ter(omx, omy, levz);
    seen    = om_hori->seen(omx, omy, levz);
   } else if (omy < 0 || omy >= OMAPY) {
    if (omy < 0) omy += OMAPY;
    else         omy -= OMAPY;
    cur_ter = om_vert->ter(omx, omy, levz);
    seen    = om_vert->seen(omx, omy, levz);
   } else {
/*
    dbg(D_ERROR) << "game:draw_minimap: No data loaded! omx: "
                 << omx << " omy: " << omy;
    debugmsg("No data loaded! omx: %d omy: %d", omx, omy);
*/
   }
   nc_color ter_color = oterlist[cur_ter].color;
   long ter_sym = oterlist[cur_ter].sym;
   if (seen) {
    if (!drew_mission && target.x == omx && target.y == omy) {
     drew_mission = true;
     if (i != 0 || j != 0)
      mvwputch   (w_minimap, 3 + j, 3 + i, red_background(ter_color), ter_sym);
     else
      mvwputch_hi(w_minimap, 3,     3,     ter_color, ter_sym);
    } else if (i == 0 && j == 0)
     mvwputch_hi(w_minimap, 3,     3,     ter_color, ter_sym);
    else
     mvwputch   (w_minimap, 3 + j, 3 + i, ter_color, ter_sym);
   }
  }
 }

// Print arrow to mission if we have one!
 if (!drew_mission) {
  double slope;
  if (cursx != target.x)
   slope = double(target.y - cursy) / double(target.x - cursx);
  if (cursx == target.x || abs(slope) > 3.5 ) { // Vertical slope
   if (target.y > cursy)
    mvwputch(w_minimap, 6, 3, c_red, '*');
   else
    mvwputch(w_minimap, 0, 3, c_red, '*');
  } else {
   int arrowx = 3, arrowy = 3;
   if (abs(slope) >= 1.) { // y diff is bigger!
    arrowy = (target.y > cursy ? 6 : 0);
    arrowx = 3 + 3 * (target.y > cursy ? slope : (0 - slope));
    if (arrowx < 0)
     arrowx = 0;
    if (arrowx > 6)
     arrowx = 6;
   } else {
    arrowx = (target.x > cursx ? 6 : 0);
    arrowy = 3 + 3 * (target.x > cursx ? slope : (0 - slope));
    if (arrowy < 0)
     arrowy = 0;
    if (arrowy > 6)
     arrowy = 6;
   }
   mvwputch(w_minimap, arrowy, arrowx, c_red, '*');
  }
 }

 wrefresh(w_minimap);
}

void game::hallucinate(const int x, const int y)
{
 for (int i = 0; i <= TERRAIN_WINDOW_WIDTH; i++) {
  for (int j = 0; j <= TERRAIN_WINDOW_HEIGHT; j++) {
   if (one_in(10)) {
    char ter_sym = terlist[m.ter(i + x - VIEWX + rng(-2, 2), j + y - VIEWY + rng(-2, 2))].sym;
    nc_color ter_col = terlist[m.ter(i + x - VIEWX + rng(-2, 2), j + y - VIEWY+ rng(-2, 2))].color;
    mvwputch(w_terrain, j, i, ter_col, ter_sym);
   }
  }
 }

//CAT-g: moved to draw_ter() this is called from
// wrefresh(w_terrain);
}


float game::natural_light_level()
{
   float ret= 1;

   if(levz >= 0)
   {
	ret = turn.sunlight();
	ret+= weather_data[weather].light_modifier;
   }

   return ret;
}

unsigned char game::light_level()
{
 if(turn == latest_lightlevel_turn)
	return latest_lightlevel;

 int ret= 1;
 if (levz >= 0)	
 {
	ret = turn.sunlight();
	ret -= weather_data[weather].sight_penalty;
 }

 for(int i = 0; i < events.size(); i++) {
  // The EVENT_DIM event slowly dims the sky, then relights it
  // EVENT_DIM has an occurance date of turn + 50, so the first 25 dim it
  if (events[i].type == EVENT_DIM) {
   int turns_left = events[i].turn - int(turn);
   i = events.size();
   if (turns_left > 25)
    ret = (ret * (turns_left - 25)) / 25;
   else
    ret = (ret * (25 - turns_left)) / 25;
  }
 }

//CAT-g: defines self-ilumination
// ...similar but different to items on the ground handled in lightmap

 int flashlight = u.active_item_charges(itm_flashlight_on);
 if(ret < 10 && flashlight > 0) {
// additive so that low battery flashlights
// still increase the light level rather than decrease it
  ret += flashlight;
  if (ret > 10)
   ret = 10;
 }
 else
 if (ret < 8 && u.has_active_bionic(bio_flashlight))
  ret = 8;
 else
 if (ret < 8 && event_queued(EVENT_ARTIFACT_LIGHT))
  ret = 8;
 else
 if(ret < 7 && u.has_amount(itm_torch_lit, 1)) 
  ret = 7;
 else
 if(ret < 6 && u.has_amount(itm_pda_flashlight, 1)) 
  ret = 6;
 else
 if(ret < 5 && u.has_amount(itm_candle_lit, 1))
  ret = 5;
 else
 if (ret < 4 && u.has_artifact_with(AEP_GLOW))
  ret = 4;
 else
 if (ret < 1)
  ret = 1;

 latest_lightlevel = ret;
 latest_lightlevel_turn = turn;

 return ret;
}


void game::reset_light_level()
{
 latest_lightlevel = 0;
 latest_lightlevel_turn = 0;
}

int game::assign_npc_id()
{
 int ret = next_npc_id;
 next_npc_id++;
 return ret;
}

int game::assign_faction_id()
{
 int ret = next_faction_id;
 next_faction_id++;
 return ret;
}

faction* game::faction_by_id(int id)
{
 for (int i = 0; i < factions.size(); i++) {
  if (factions[i].id == id)
   return &(factions[i]);
 }
 return NULL;
}

faction* game::random_good_faction()
{
 std::vector<int> valid;
 for (int i = 0; i < factions.size(); i++) {
  if (factions[i].good >= 5)
   valid.push_back(i);
 }
 if (valid.size() > 0) {
  int index = valid[rng(0, valid.size() - 1)];
  return &(factions[index]);
 }
// No good factions exist!  So create one!
 faction newfac(assign_faction_id());
 do
  newfac.randomize();
 while (newfac.good < 5);
 newfac.id = factions.size();
 factions.push_back(newfac);
 return &(factions[factions.size() - 1]);
}

faction* game::random_evil_faction()
{
 std::vector<int> valid;
 for (int i = 0; i < factions.size(); i++) {
  if (factions[i].good <= -5)
   valid.push_back(i);
 }
 if (valid.size() > 0) {
  int index = valid[rng(0, valid.size() - 1)];
  return &(factions[index]);
 }
// No good factions exist!  So create one!
 faction newfac(assign_faction_id());
 do
  newfac.randomize();
 while (newfac.good > -5);
 newfac.id = factions.size();
 factions.push_back(newfac);
 return &(factions[factions.size() - 1]);
}

//CAT-mgs: handle monster reaction here, or 'monster.cpp' as it is?
bool game::sees_u(int x, int y, int &t)
{
 // TODO: [lightmap] Apply default monster vison levels here
 //                  the light map should deal lighting from player or fires
 int range = light_level();

 // Set to max possible value if the player is lit brightly
 if (lm.at(0, 0) >= LL_LOW)
  range = DAYLIGHT_LEVEL;

 int mondex = mon_at(x,y);
 if (mondex != -1) {
  if(z[mondex].has_flag(MF_VIS10))
   range -= 50;
  else if(z[mondex].has_flag(MF_VIS20))
   range -= 40;
  else if(z[mondex].has_flag(MF_VIS30))
   range -= 30;
  else if(z[mondex].has_flag(MF_VIS40))
   range -= 20;
  else if(z[mondex].has_flag(MF_VIS50))
   range -= 10;
 }

 if( range <= 0)
  range = 1;
 
 return (!u.has_active_bionic(bio_cloak) &&
         !u.has_artifact_with(AEP_INVISIBLE) &&
         m.sees(x, y, u.posx, u.posy, range, t));
}

//CAT-mgs: fix when shift view in darkness *** vvv
bool game::u_see(int x, int y, int &t)
{
 int wanted_range = rl_dist(u.posx, u.posy, x, y);

 bool can_see = false;
 if (wanted_range < u.clairvoyance())
  can_see = true;
 else 
 if (wanted_range <= u.sight_range(light_level()) ||
          (wanted_range <= u.sight_range(DAYLIGHT_LEVEL) &&
            lm.at(x - u.posx-u.view_offset_x, y - u.posy-u.view_offset_y) >= LL_LOW))
  can_see = m.sees(u.posx, u.posy, x, y, wanted_range, t);

 return can_see;
}

bool game::u_see(monster *mon, int &t)
{
 int dist = rl_dist(u.posx, u.posy, mon->posx, mon->posy);
 if (u.has_trait(PF_ANTENNAE) && dist <= 3)
  return true;
 if (mon->has_flag(MF_DIGS) && !u.has_active_bionic(bio_ground_sonar) &&
     dist > 1)
  return false;	// Can't see digging monsters until we're right next to them

 return u_see(mon->posx, mon->posy, t);
}

bool game::pl_sees(player *p, monster *mon, int &t)
{
 // TODO: [lightmap] Allow npcs to use the lightmap
 if (mon->has_flag(MF_DIGS) && !p->has_active_bionic(bio_ground_sonar) &&
     rl_dist(p->posx, p->posy, mon->posx, mon->posy) > 1)
  return false;	// Can't see digging monsters until we're right next to them
 int range = p->sight_range(light_level());
 return m.sees(p->posx, p->posy, mon->posx, mon->posy, range, t);
}

point game::find_item(item *it)
{
 if (u.has_item(it))
  return point(u.posx, u.posy);

 point ret = m.find_item(it);
 if (ret.x != -1 && ret.y != -1)
  return ret;

//CAT-mgs: no NPCs
 for (int i = 0; i < active_npc.size(); i++) {
  for (int j = 0; j < active_npc[i].inv.size(); j++) {
   if (it == &(active_npc[i].inv[j]))
    return point(active_npc[i].posx, active_npc[i].posy);
  }
 }


 return point(-999, -999);
}

void game::remove_item(item *it)
{
 point ret;
 if (it == &u.weapon) {
  u.remove_weapon();
  return;
 }
 for (int i = 0; i < u.inv.size(); i++) {
  if (it == &u.inv[i]) {
   u.i_remn(i);
   return;
  }
 }
 for (int i = 0; i < u.worn.size(); i++) {
  if (it == &u.worn[i]) {
   u.worn.erase(u.worn.begin() + i);
   return;
  }
 }
 ret = m.find_item(it);
 if (ret.x != -1 && ret.y != -1) {
  for (int i = 0; i < m.i_at(ret.x, ret.y).size(); i++) {
   if (it == &m.i_at(ret.x, ret.y)[i]) {
    m.i_rem(ret.x, ret.y, i);
    return;
   }
  }
 }

//CAT-mgs: no NPCs
 for (int i = 0; i < active_npc.size(); i++) {
	  if (it == &active_npc[i].weapon) {
		   active_npc[i].remove_weapon();
		   return;
	  }

	  for (int j = 0; j < active_npc[i].inv.size(); j++) {
		   if (it == &active_npc[i].inv[j]) {
			    active_npc[i].i_remn(j);
			    return;
		   }
	  }

	  for (int j = 0; j < active_npc[i].worn.size(); j++) {
		   if (it == &active_npc[i].worn[j]) {
			    active_npc[i].worn.erase(active_npc[i].worn.begin() + j);
			    return;
		   }
	  }
 }

}

bool vector_has(std::vector<int> vec, int test)
{
 for (int i = 0; i < vec.size(); i++) {
  if (vec[i] == test)
   return true;
 }
 return false;
}

void game::mon_info()
{
 werase(w_moninfo);
 int buff;
 int newseen = 0;

//CAT-mgs: 30 <- 60
// const int iProxyDist = (OPTIONS[OPT_SAFEMODEPROXIMITY] <= 0) ? 30 : OPTIONS[OPT_SAFEMODEPROXIMITY];
 const int iProxyDist = 40;


// 7 0 1	unique_types uses these indices;
// 6 8 2	0-7 are provide by direction_from()
// 5 4 3	8 is used for local monsters (for when we explain them below)
 std::vector<int> unique_types[9];
// dangerous_types tracks whether we should print in red to warn the player
 bool dangerous[8];
 for (int i = 0; i < 8; i++)
  dangerous[i] = false;

//CAT-s:
 closeMonster= 99;

 direction dir_to_mon, dir_to_npc;
 for (int i = 0; i < z.size(); i++) {
  if (u_see(&(z[i]), buff)) {
   bool mon_dangerous = false;
   int j;
   if (z[i].attitude(&u) == MATT_ATTACK || z[i].attitude(&u) == MATT_FOLLOW) {
    if (sees_u(z[i].posx, z[i].posy, j))
     mon_dangerous = true;

//CAT-g: what did I do here?
//    if (rl_dist(u.posx, u.posy, z[i].posx, z[i].posy) <= iProxyDist)
//     newseen++;

//CAT-s:
	   int rlD= rl_dist(u.posx, u.posy, z[i].posx, z[i].posy);
	   if(rlD <= iProxyDist)
	   {  
		closeMonster= (rlD < closeMonster) ? rlD : closeMonster;
		newseen++;
	   }

   }

   dir_to_mon = direction_from(u.posx + u.view_offset_x, u.posy + u.view_offset_y,
                               z[i].posx, z[i].posy);
   int index = (abs(u.posx + u.view_offset_x - z[i].posx) <= VIEWX &&
                abs(u.posy + u.view_offset_y - z[i].posy) <= VIEWY) ?
                8 : dir_to_mon;
   if (mon_dangerous && index < 8)
    dangerous[index] = true;

   if (!vector_has(unique_types[dir_to_mon], z[i].type->id))
    unique_types[index].push_back(z[i].type->id);
  }
 }

//CAT-mgs: no NPCs
 for (int i = 0; i < active_npc.size(); i++) {
  if (u_see(active_npc[i].posx, active_npc[i].posy, buff)) { // TODO: NPC invis
   if (active_npc[i].attitude == NPCATT_KILL)
    if (rl_dist(u.posx, u.posy, active_npc[i].posx, active_npc[i].posy) <= iProxyDist)
     newseen++;

   point npcp(active_npc[i].posx, active_npc[i].posy);
   dir_to_npc = direction_from ( u.posx + u.view_offset_x, u.posy + u.view_offset_y,
                                 npcp.x, npcp.y );
   int index = (abs(u.posx + u.view_offset_x - npcp.x) <= VIEWX &&
                abs(u.posy + u.view_offset_y - npcp.y) <= VIEWY) ?
                8 : dir_to_npc;
   unique_types[index].push_back(-1 - i);
  }
 }


 if (newseen > mostseen) {
  if (u.activity.type == ACT_REFILL_VEHICLE)
   cancel_activity_query("Monster Spotted!");

  cancel_activity_query("Monster spotted!");
  turnssincelastmon = 0;
  if (run_mode == 1)
   run_mode = 2;	// Stop movement!
 } else if (autosafemode && newseen == 0) { // Auto-safemode
  turnssincelastmon++;
//CAT-mgs:
//  if(turnssincelastmon >= OPTIONS[OPT_AUTOSAFEMODETURNS] && run_mode == 0)
  if(turnssincelastmon >= 50 && run_mode == 0)
   run_mode = 1;
 }

 mostseen = newseen;
 nc_color tmpcol;
// Print the direction headings
// Reminder:
// 7 0 1	unique_types uses these indices;
// 6 8 2	0-7 are provide by direction_from()
// 5 4 3	8 is used for local monsters (for when we explain them below)
 mvwprintz(w_moninfo,  0,  0, (unique_types[7].empty() ?
           c_dkgray : (dangerous[7] ? c_ltred : c_ltgray)), "NW:");
 mvwprintz(w_moninfo,  0, 15, (unique_types[0].empty() ?
           c_dkgray : (dangerous[0] ? c_ltred : c_ltgray)), "North:");
 mvwprintz(w_moninfo,  0, 33, (unique_types[1].empty() ?
           c_dkgray : (dangerous[1] ? c_ltred : c_ltgray)), "NE:");
 mvwprintz(w_moninfo,  1,  0, (unique_types[6].empty() ?
           c_dkgray : (dangerous[6] ? c_ltred : c_ltgray)), "West:");
 mvwprintz(w_moninfo,  1, 31, (unique_types[2].empty() ?
           c_dkgray : (dangerous[2] ? c_ltred : c_ltgray)), "East:");
 mvwprintz(w_moninfo,  2,  0, (unique_types[5].empty() ?
           c_dkgray : (dangerous[5] ? c_ltred : c_ltgray)), "SW:");
 mvwprintz(w_moninfo,  2, 15, (unique_types[4].empty() ?
           c_dkgray : (dangerous[4] ? c_ltred : c_ltgray)), "South:");
 mvwprintz(w_moninfo,  2, 33, (unique_types[3].empty() ?
           c_dkgray : (dangerous[3] ? c_ltred : c_ltgray)), "SE:");

 for (int i = 0; i < 8; i++) {

  point pr;
  switch (i) {
   case 7: pr.y = 0; pr.x =  4; break;
   case 0: pr.y = 0; pr.x = 22; break;
   case 1: pr.y = 0; pr.x = 37; break;

   case 6: pr.y = 1; pr.x =  6; break;
   case 2: pr.y = 1; pr.x = 37; break;

   case 5: pr.y = 2; pr.x =  4; break;
   case 4: pr.y = 2; pr.x = 22; break;
   case 3: pr.y = 2; pr.x = 37; break;
  }

  for (int j = 0; j < unique_types[i].size() && j < 10; j++) {
   buff = unique_types[i][j];

//CAT-mgs: no NPCs
   if (buff < 0) { // It's an NPC!
    switch (active_npc[(buff + 1) * -1].attitude) {
     case NPCATT_KILL:   tmpcol = c_red;     break;
     case NPCATT_FOLLOW: tmpcol = c_ltgreen; break;
     case NPCATT_DEFEND: tmpcol = c_green;   break;
     default:            tmpcol = c_pink;    break;
    }
    mvwputch (w_moninfo, pr.y, pr.x, tmpcol, '@');
   } else // It's a monster!  easier.
    mvwputch (w_moninfo, pr.y, pr.x, mtypes[buff]->color, mtypes[buff]->sym);

   pr.x++;

  }

  if (unique_types[i].size() > 10) // Couldn't print them all!
   mvwputch (w_moninfo, pr.y, pr.x - 1, c_white, '+');
 } // for (int i = 0; i < 8; i++)

// Now we print their full names!

 bool listed_it[num_monsters]; // Don't list any twice!
 for (int i = 0; i < num_monsters; i++)
  listed_it[i] = false;

 point pr(0, 4);

// Start with nearby zombies--that's the most important
// We stop if pr.y hits 10--i.e. we're out of space
 for (int i = 0; i < unique_types[8].size() && pr.y < 12; i++) {
  buff = unique_types[8][i];
// buff < 0 means an NPC!  Don't list those.
  if (buff >= 0 && !listed_it[buff]) {
   listed_it[buff] = true;
   std::string name = mtypes[buff]->name;
// + 2 for the "Z "
   if (pr.x + 2 + name.length() >= 48) { // We're too long!
    pr.y++;
    pr.x = 0;
   }
   if (pr.y < 12) { // Don't print if we've overflowed
    mvwputch (w_moninfo, pr.y, pr.x, mtypes[buff]->color, mtypes[buff]->sym);
    nc_color danger = c_dkgray;
    if (mtypes[buff]->difficulty >= 30)
     danger = c_red;
    else if (mtypes[buff]->difficulty >= 16)
     danger = c_ltred;
    else if (mtypes[buff]->difficulty >= 8)
     danger = c_white;
    else if (mtypes[buff]->agro > 0)
     danger = c_ltgray;
    mvwprintz(w_moninfo, pr.y, pr.x + 2, danger, name.c_str());
   }
// +4 for the "Z " and two trailing spaces
   pr.x += 4 + name.length();
  }
 }
// Now, if there's space, the rest of the monsters!
 for (int j = 0; j < 8 && pr.y < 12; j++) {
  for (int i = 0; i < unique_types[j].size() && pr.y < 12; i++) {
   buff = unique_types[j][i];
// buff < 0 means an NPC!  Don't list those.
   if (buff >= 0 && !listed_it[buff]) {
    listed_it[buff] = true;
    std::string name = mtypes[buff]->name;
// + 2 for the "Z "
    if (pr.x + 2 + name.length() >= 48) { // We're too long!
     pr.y++;
     pr.x = 0;
    }
    if (pr.y < 12) { // Don't print if we've overflowed
     mvwputch (w_moninfo, pr.y, pr.x, mtypes[buff]->color, mtypes[buff]->sym);
     nc_color danger = c_dkgray;
     if (mtypes[buff]->difficulty >= 30)
      danger = c_red;
     else if (mtypes[buff]->difficulty >= 15)
      danger = c_ltred;
     else if (mtypes[buff]->difficulty >= 8)
      danger = c_white;
     else if (mtypes[buff]->agro > 0)
      danger = c_ltgray;
     mvwprintz(w_moninfo, pr.y, pr.x + 2, danger, name.c_str());
    }
// +3 for the "Z " and a trailing space
    pr.x += 3 + name.length();
   }
  }
 }

 wrefresh(w_moninfo);

//CAT-g:
// refresh();

}


void game::cleanup_dead()
{
//printf(" a1");

 for (int i = 0; i < z.size(); i++) {
  if(z[i].dead || z[i].hp <= 0) {
   
   if(!z[i].dead)
	kill_mon(i, false);

//   add_msg("Erase monster");

   z.erase(z.begin() + i);
   i--;
  }
 }

//printf(" a2");

//CAT-mgs: no NPCs
 for (int i = 0; i < active_npc.size(); i++) {
  if (active_npc[i].dead) {
   active_npc.erase( active_npc.begin() + i );
   i--;
  }
 }

}


//CAT-mgs: ***** vvv ******
void game::monmove()
{
//   cleanup_dead();

   for (int i = 0; i < z.size(); i++)
   {
/*
	while(!z[i].dead && !z[i].can_move_to(m, z[i].posx, z[i].posy))
	{
		bool okay = false;
		int xdir = rng(1, 2) * 2 - 3, ydir = rng(1, 2) * 2 - 3; // -1 or 1
		int startx = z[i].posx - 3 * xdir, endx = z[i].posx + 3 * xdir;
		int starty = z[i].posy - 3 * ydir, endy = z[i].posy + 3 * ydir;

		for(int x = startx; x != endx && !okay; x += xdir)
		{
		   for(int y = starty; y != endy && !okay; y += ydir)
		   {
			if(z[i].can_move_to(m, x, y))
			{
				z[i].posx = x;
				z[i].posy = y;
				okay = true;
			}
		   }
		}


		if(!okay)
			z[i].dead = true;
	}

*/


	if(!z[i].dead)
	{
		z[i].made_footstep = false;
		while(z[i].moves > 0 && !z[i].dead)
		{
			z[i].plan(this);
			z[i].move(this);
		}

		if(z[i].hurt(0))
			kill_mon(i, false);
		else
		{

			z[i].receive_moves();
			z[i].process_effects(this);
			z[i].process_triggers(this);
			m.mon_in_field(z[i].posx, z[i].posy, this, &(z[i]));

			if(u.has_active_bionic(bio_alarm) && u.power_level >= 1 
				&& rl_dist(u.posx, u.posy, z[i].posx, z[i].posy) <= 5)
			{
				u.power_level--;
				add_msg("Your motion alarm goes off!");
				cancel_activity_query("Your motion alarm goes off!");
				if (u.has_disease(DI_SLEEP) || u.has_disease(DI_LYING_DOWN))
				{
					u.rem_disease(DI_SLEEP);
					u.rem_disease(DI_LYING_DOWN);
				}
			}
		}

	}
   }


//CAT-mgs: no NPCs - new move
   for(int i = 0; i < active_npc.size(); i++)
   { 
	if(active_npc[i].hp_cur[hp_head] <= 0 
			|| active_npc[i].hp_cur[hp_torso] <= 0)
		active_npc[i].die(this);
	else
	if(active_npc[i].posz == levz)
	{
	   active_npc[i].reset(this);
	   active_npc[i].suffer(this);

	   while(!active_npc[i].dead && active_npc[i].moves > 0 )
		   active_npc[i].move(this);
	}
   }

}


//CAT-mgs: *** vvv
void game::sound(int x, int y, int vol, std::string description)
{

//CAT-mgs: 
   if(abs(x-u.posx) < 9 && abs(y-u.posy) < 9)
   {
	x= u.posx;
	y= u.posy;
   }
   else
      vol= (int)(vol/2); 

	// First, alert all monsters (that can hear) to the sound
	 for (int i = 0; i < z.size(); i++) {
	  if (z[i].can_hear()) {
	   int dist = rl_dist(x, y, z[i].posx, z[i].posy);
	//CAT-mgs:
	   int volume = vol - (z[i].has_flag(MF_GOODHEARING) ? (int)(dist/7) : (int)(dist/5));

	   if(volume < 0)
		volume= 0;

//		add_msg("VOLUME: %d", volume);

//CAT-mgs: it's getting accumulated? 
// ...sorted out in moster.cpp
	   z[i].wander_to(x, y, volume);
	   z[i].process_trigger(MTRIG_SOUND, volume*25);
	  }
	 }

	// Loud sounds make the next spawn sooner!
	 int spawn_range = int(MAPSIZE / 2) * SEEX;
	 if (vol >= spawn_range) {
	  int max = (vol - spawn_range);
	  int min = int(max / 6);
	  if (max > spawn_range * 4)
	   max = spawn_range * 4;
	  if (min > spawn_range * 4)
	   min = spawn_range * 4;
	  int change = rng(min, max);
	  if (nextspawn < change)
	   nextspawn = 0;
	  else
	   nextspawn -= change;
	 }

//CAT-mgs: *** ^^^


// Next, display the sound as the player hears it
 if (description == "")
  return;	// No description (e.g., footsteps)
 if (u.has_disease(DI_DEAF))
  return;	// We're deaf, can't hear it

 if (u.has_bionic(bio_ears))
  vol *= 3.5;
 if (u.has_trait(PF_BADHEARING))
  vol *= .5;
 if (u.has_trait(PF_CANINE_EARS))
  vol *= 1.5;
 int dist = rl_dist(x, y, u.posx, u.posy);
 if (dist > vol)
  return;	// Too far away, we didn't hear it!
 if (u.has_disease(DI_SLEEP) &&
     ((!u.has_trait(PF_HEAVYSLEEPER) && dice(2, 20) < vol - dist) ||
      ( u.has_trait(PF_HEAVYSLEEPER) && dice(3, 20) < vol - dist)   )) {
  u.rem_disease(DI_SLEEP);
  add_msg("You're woken up by a noise.");
  return;
 }
 if (!u.has_bionic(bio_ears) && rng( (vol - dist) / 2, (vol - dist) ) >= 150) {
  int duration = (vol - dist - 130) / 4;
  if (duration > 40)
   duration = 40;
  u.add_disease(DI_DEAF, duration, this);
 }
 if (x != u.posx || y != u.posy)
  cancel_activity_query("Heard %s!",
                        (description == "" ? "a noise" : description.c_str()));
// We need to figure out where it was coming from, relative to the player
 int dx = x - u.posx;
 int dy = y - u.posy;
// If it came from us, don't print a direction
 if (dx == 0 && dy == 0) {
  if (description[0] >= 'a' && description[0] <= 'z')
   description[0] += 'A' - 'a';	// Capitalize the sound
  add_msg("%s", description.c_str());
  return;
 }
 std::string direction = direction_name(direction_from(u.posx, u.posy, x, y));
 add_msg("From the %s you hear %s", direction.c_str(), description.c_str());
}

// add_footstep will create a list of locations to draw monster
// footsteps. these will be more or less accurate depending on the
// characters hearing and how close they are
void game::add_footstep(int x, int y, int volume, int distance)
{
 int t = 0;
 if (x == u.posx && y == u.posy)
  return;
 else if (u_see(x, y, t))
  return;
 int err_offset;
 if (volume / distance < 2)
  err_offset = 3;
 else if (volume / distance < 3)
  err_offset = 2;
 else
  err_offset = 1;
 if (u.has_bionic(bio_ears))
  err_offset--;
 if (u.has_trait(PF_BADHEARING))
  err_offset++;

 int tries = 0, origx = x, origy = y;
 if (err_offset > 0) {
  do {
   tries++;
   x = origx + rng(-err_offset, err_offset);
   y = origy + rng(-err_offset, err_offset);
  } while (tries < 10 && (u_see(x, y, t) || (x == u.posx && y == u.posy)));
 }
 if (tries < 10)
  footsteps.push_back(point(x, y));
 return;
}


// draws footsteps that have been created by monsters moving about
void game::draw_footsteps()
{
 for (int i = 0; i < footsteps.size(); i++) {

	if( footsteps[i].x < u.posx+SEEX && footsteps[i].x > u.posx-SEEX
			&& footsteps[i].y < u.posy+SEEY && footsteps[i].y > u.posy-SEEY )
	{
		mvwputch(w_terrain, VIEWY + footsteps[i].y - u.posy - u.view_offset_y,
                      VIEWX + footsteps[i].x - u.posx - u.view_offset_x, c_yellow, '?');

//CAT-mgs: more horror atmosphere
		if(one_in(30))
			playSound(rng(13,22));
	}
 }

 footsteps.clear();
 wrefresh(w_terrain);
 return;
}

void game::explosion(int x, int y, int power, int shrapnel, bool fire)
{
 timespec ts;	// Timespec for the animation of the explosion
 ts.tv_sec = 0;
 ts.tv_nsec = EXPLOSION_SPEED;
 int radius = sqrt(double(power / 4));
 int dam;
 std::string junk;
 int noise = power * fire ? 2 : 10;

//CAT-s:
 if (power >= 30)
 {

	playSound(49);
	sound(x, y, noise, "a huge explosion!");
 }
 else
 {
	playSound(48);
	sound(x, y, noise, "an explosion!");
 }

 for (int i = x - radius; i <= x + radius; i++) {
  for (int j = y - radius; j <= y + radius; j++) {
   if (i == x && j == y)
    dam = 5 * power;
   else
    dam = 5 * power / (rl_dist(x, y, i, j));

   if (m.has_flag(bashable, i, j))
    m.bash(i, j, dam, junk);
   if (m.has_flag(bashable, i, j))	// Double up for tough doors, etc.
    m.bash(i, j, dam, junk);
   if (m.is_destructable(i, j) && rng(25, 100) < dam)
    m.destroy(this, i, j, false);

   int mon_hit = mon_at(i, j), npc_hit = npc_at(i, j);
   if (mon_hit != -1 && !z[mon_hit].dead &&
       z[mon_hit].hurt(rng(dam / 2, dam * 1.5))) {
    if (z[mon_hit].hp < 0 - 1.5 * z[mon_hit].type->hp)
     explode_mon(mon_hit); // Explode them if it was big overkill
    else
     kill_mon(mon_hit, true); // TODO: player's fault?

    int vpart;
    vehicle *veh = m.veh_at(i, j, vpart);
    if (veh)
     veh->damage (vpart, dam, false);
   }

//CAT-mgs: no NPCs - hit
   if (npc_hit != -1) {
    active_npc[npc_hit].hit(this, bp_torso, 0, rng(dam / 2, dam * 1.5), 0);
    active_npc[npc_hit].hit(this, bp_head,  0, rng(dam / 3, dam),       0);
    active_npc[npc_hit].hit(this, bp_legs,  0, rng(dam / 3, dam),       0);
    active_npc[npc_hit].hit(this, bp_legs,  1, rng(dam / 3, dam),       0);
    active_npc[npc_hit].hit(this, bp_arms,  0, rng(dam / 3, dam),       0);
    active_npc[npc_hit].hit(this, bp_arms,  1, rng(dam / 3, dam),       0);
    if (active_npc[npc_hit].hp_cur[hp_head]  <= 0 ||
        active_npc[npc_hit].hp_cur[hp_torso] <= 0   ) {
     active_npc[npc_hit].die(this, true);
     //active_npc.erase(active_npc.begin() + npc_hit);
    }
   }


   if (u.posx == i && u.posy == j) {
    add_msg("You're caught in the explosion!");
    u.hit(this, bp_torso, 0, rng(dam / 2, dam * 1.5), 0);
    u.hit(this, bp_head,  0, rng(dam / 3, dam),       0);
    u.hit(this, bp_legs,  0, rng(dam / 3, dam),       0);
    u.hit(this, bp_legs,  1, rng(dam / 3, dam),       0);
    u.hit(this, bp_arms,  0, rng(dam / 3, dam),       0);
    u.hit(this, bp_arms,  1, rng(dam / 3, dam),       0);
   }

   if (fire) {
    if (m.field_at(i, j).type == fd_smoke)
     m.field_at(i, j) = field(fd_fire, 1, 0);
    m.add_field(this, i, j, fd_fire, dam / 10);
   }
  }
 }

// Draw the explosion
//CAT-mgs:
 nc_color cat_col= c_yellow;
 mvwputch(w_terrain, y + VIEWY - u.posy - u.view_offset_y,
		x + VIEWX - u.posx - u.view_offset_x, cat_col, '*');

//CAT-mgs: *** vvv
 for (int i = 1; i <= radius+1; i++) {

  if(i > radius)
	cat_col= c_ltred;
  else
  if(i > 2)
	cat_col= c_red;

  if(i != radius+1)
  {
	  mvwputch(w_terrain, y - i + VIEWY - u.posy - u.view_offset_y,
                      x - i + VIEWX - u.posx - u.view_offset_x, cat_col, '*');
	  mvwputch(w_terrain, y - i + VIEWY - u.posy - u.view_offset_y,
                      x + i + VIEWX - u.posx - u.view_offset_x, cat_col,'*');
	  mvwputch(w_terrain, y + i + VIEWY - u.posy - u.view_offset_y,
                      x - i + VIEWX - u.posx - u.view_offset_x, cat_col,'*');
	  mvwputch(w_terrain, y + i + VIEWY - u.posy - u.view_offset_y,
                      x + i + VIEWX - u.posx - u.view_offset_x, cat_col, '*');
  }

  for (int j = 1 - i; j < 0 + i; j++) {
   mvwputch(w_terrain, y - i + VIEWY - u.posy - u.view_offset_y,
                       x + j + VIEWX - u.posx - u.view_offset_x, cat_col,'*');
   mvwputch(w_terrain, y + i + VIEWY - u.posy - u.view_offset_y,
                       x + j + VIEWX - u.posx - u.view_offset_x, cat_col,'*');
   mvwputch(w_terrain, y + j + VIEWY - u.posy - u.view_offset_y,
                       x - i + VIEWX - u.posx - u.view_offset_x, cat_col,'*');
   mvwputch(w_terrain, y + j + VIEWY - u.posy - u.view_offset_y,
                       x + i + VIEWX - u.posx - u.view_offset_x, cat_col,'*');

  }

  wrefresh(w_terrain);
//CAT-mgs:
  wrefresh(0, true);
  nanosleep(&ts, NULL);
 }


// The rest of the function is shrapnel
 if (shrapnel <= 0)
  return;

 int sx, sy, t, ijunk, tx, ty;
 std::vector<point> traj;
 ts.tv_sec = 0;
 ts.tv_nsec = BULLET_SPEED/5;	
 for (int i = 0; i < shrapnel; i++) {
  sx = rng(x - 2 * radius, x + 2 * radius);
  sy = rng(y - 2 * radius, y + 2 * radius);
  if (m.sees(x, y, sx, sy, 50, t))
   traj = line_to(x, y, sx, sy, t);
  else
   traj = line_to(x, y, sx, sy, 0);

  dam = rng(40, 90);
  for (int j = 0; j < traj.size(); j++) {
   if (j > 0 && u_see(traj[j - 1].x, traj[j - 1].y, ijunk))
    m.drawsq(w_terrain, u, traj[j - 1].x, traj[j - 1].y, false, true);
   if (u_see(traj[j].x, traj[j].y, ijunk)) {
    mvwputch(w_terrain, traj[j].y + VIEWY - u.posy - u.view_offset_y,
                        traj[j].x + VIEWX - u.posx - u.view_offset_x, c_pink, '*');


    wrefresh(w_terrain);
//CAT-mgs:
    wrefresh(0, true);
    nanosleep(&ts, NULL);
   }
   tx = traj[j].x;
   ty = traj[j].y;
   if (mon_at(tx, ty) != -1) {
    dam -= z[mon_at(tx, ty)].armor_cut();
    if (z[mon_at(tx, ty)].hurt(dam))
     kill_mon(mon_at(tx, ty), true);
   } else if (npc_at(tx, ty) != -1) {
    body_part hit = random_body_part();
    if (hit == bp_eyes || hit == bp_mouth || hit == bp_head)
     dam = rng(2 * dam, 5 * dam);
    else if (hit == bp_torso)
     dam = rng(1.5 * dam, 3 * dam);

//CAT-mgs: no NPCs ***
    int npcdex = npc_at(tx, ty);
    active_npc[npcdex].hit(this, hit, rng(0, 1), 0, dam);
    if (active_npc[npcdex].hp_cur[hp_head] <= 0 ||
        active_npc[npcdex].hp_cur[hp_torso] <= 0) {
     active_npc[npcdex].die(this);
     //active_npc.erase(active_npc.begin() + npcdex);
    }
//CAT: end ***

   } else if (tx == u.posx && ty == u.posy) {
    body_part hit = random_body_part();
    int side = rng(0, 1);
    add_msg("Shrapnel hits your %s!", body_part_name(hit, side).c_str());
    u.hit(this, hit, rng(0, 1), 0, dam);

//CAT-s: pain sound, add heartBeat somewhere when low on health
	playSound(9); 

   } else
    m.shoot(this, tx, ty, dam, j == traj.size() - 1, 0);
  }
 }
}


void game::flashbang(int x, int y)
{
 int dist = rl_dist(u.posx, u.posy, x, y), t;
 if (dist <= 8) {
  if (!u.has_bionic(bio_ears))
   u.add_disease(DI_DEAF, 40 - dist * 4, this);
  if (m.sees(u.posx, u.posy, x, y, 8, t))
   u.infect(DI_BLIND, bp_eyes, (12 - dist) / 2, 10 - dist, this);
 }
 for (int i = 0; i < z.size(); i++) {
  dist = rl_dist(z[i].posx, z[i].posy, x, y);
  if (dist <= 4)
   z[i].add_effect(ME_STUNNED, 10 - dist);
  if (dist <= 8) {
   if (z[i].has_flag(MF_SEES) && m.sees(z[i].posx, z[i].posy, x, y, 8, t))
    z[i].add_effect(ME_BLIND, 18 - dist);
   if (z[i].has_flag(MF_HEARS))
    z[i].add_effect(ME_DEAF, 60 - dist * 4);
  }
 }
 sound(x, y, 30, "a huge boom!");

//CAT-s:
	playSound(48);

// TODO: Blind/deafen NPC
}

void game::use_computer(int x, int y)
{
 if (u.has_trait(PF_ILLITERATE)) {
  add_msg("You can not read a computer screen!");
  return;
 }
 computer* used = m.computer_at(x, y);

 if (used == NULL) {
/*
  dbg(D_ERROR) << "game:use_computer: Tried to use computer at (" << x
               << ", " << y << ") - none there";
  debugmsg("Tried to use computer at (%d, %d) - none there", x, y);
*/
  return;
 }

 used->use(this);

 refresh_all();
}

void game::resonance_cascade(int x, int y)
{
 int maxglow = 100 - 5 * trig_dist(x, y, u.posx, u.posy);
 int minglow =  60 - 5 * trig_dist(x, y, u.posx, u.posy);
 mon_id spawn;
 monster invader;
 if (minglow < 0)
  minglow = 0;
 if (maxglow > 0)
  u.add_disease(DI_TELEGLOW, rng(minglow, maxglow) * 100, this);
 int startx = (x < 8 ? 0 : x - 8), endx = (x+8 >= SEEX*3 ? SEEX*3 - 1 : x + 8);
 int starty = (y < 8 ? 0 : y - 8), endy = (y+8 >= SEEY*3 ? SEEY*3 - 1 : y + 8);
 for (int i = startx; i <= endx; i++) {
  for (int j = starty; j <= endy; j++) {
   switch (rng(1, 80)) {
   case 1:
   case 2:
    emp_blast(i, j);
    break;
   case 3:
   case 4:
   case 5:
    for (int k = i - 1; k <= i + 1; k++) {
     for (int l = j - 1; l <= j + 1; l++) {
      field_id type;
      switch (rng(1, 7)) {
       case 1: type = fd_blood;
       case 2: type = fd_bile;
       case 3:
       case 4: type = fd_slime;
       case 5: type = fd_fire;
       case 6:
       case 7: type = fd_nuke_gas;
      }
      if (m.field_at(k, l).type == fd_null || !one_in(3))
       m.field_at(k, l) = field(type, 3, 0);
     }
    }
    break;
   case  6:
   case  7:
   case  8:
   case  9:
   case 10:
    m.tr_at(i, j) = tr_portal;
    break;
   case 11:
   case 12:
    m.tr_at(i, j) = tr_goo;
    break;
   case 13:
   case 14:
   case 15:
    spawn = MonsterGroupManager::GetMonsterFromGroup("GROUP_NETHER", &mtypes);
    invader = monster(mtypes[spawn], i, j);
    z.push_back(invader);
    break;
   case 16:
   case 17:
   case 18:
    m.destroy(this, i, j, true);
    break;
   case 19:
    explosion(i, j, rng(1, 10), rng(0, 1) * rng(0, 6), one_in(4));
    break;
   }
  }
 }
}

void game::scrambler_blast(int x, int y)
{
 int mondex = mon_at(x, y);
 if (mondex != -1) {
  if (z[mondex].has_flag(MF_ELECTRONIC))
    z[mondex].make_friendly();
   add_msg("The %s sparks and begins searching for a target!", z[mondex].name().c_str());
 }
}
void game::emp_blast(int x, int y)
{
 int rn;
 if (m.has_flag(console, x, y)) {
  add_msg("The %s is rendered non-functional!", m.tername(x, y).c_str());
  m.ter(x, y) = t_console_broken;
  return;
 }
// TODO: More terrain effects.
 switch (m.ter(x, y)) {
 case t_card_science:
 case t_card_military:
  rn = rng(1, 100);
  if (rn > 92 || rn < 40) {
   add_msg("The card reader is rendered non-functional.");
   m.ter(x, y) = t_card_reader_broken;
  }
  if (rn > 80) {
   add_msg("The nearby doors slide open!");
   for (int i = -3; i <= 3; i++) {
    for (int j = -3; j <= 3; j++) {
     if (m.ter(x + i, y + j) == t_door_metal_locked)
      m.ter(x + i, y + j) = t_floor;
    }
   }
  }
  if (rn >= 40 && rn <= 80)
   add_msg("Nothing happens.");
  break;
 }
 int mondex = mon_at(x, y);
 if (mondex != -1) {
  if (z[mondex].has_flag(MF_ELECTRONIC)) {
   add_msg("The EMP blast fries the %s!", z[mondex].name().c_str());
   int dam = dice(10, 10);
   if (z[mondex].hurt(dam))
    kill_mon(mondex, true); // TODO: Player's fault?
   else if (one_in(6))
    z[mondex].make_friendly();
  } else
   add_msg("The %s is unaffected by the EMP blast.", z[mondex].name().c_str());
 }
 if (u.posx == x && u.posy == y) {
  if (u.power_level > 0) {
   add_msg("The EMP blast drains your power.");
   int max_drain = (u.power_level > 40 ? 40 : u.power_level);
   u.charge_power(0 - rng(1 + max_drain / 3, max_drain));
  }
// TODO: More effects?
 }
// Drain any items of their battery charge
 for (int i = 0; i < m.i_at(x, y).size(); i++) {
  if (m.i_at(x, y)[i].is_tool() &&
      (dynamic_cast<it_tool*>(m.i_at(x, y)[i].type))->ammo == AT_BATT)
   m.i_at(x, y)[i].charges = 0;
 }
// TODO: Drain NPC energy reserves
}

int game::npc_at(int x, int y)
{
 for(int i = 0; i < active_npc.size(); i++)
 {
	if(active_npc[i].posz == levz && !active_npc[i].dead
		&& active_npc[i].posx == x && active_npc[i].posy == y)
	   return i;
 }
 return -1;
}

int game::npc_by_id(int id)
{
 for (int i = 0; i < active_npc.size(); i++) {
  if (active_npc[i].id == id)
   return i;
 }
 return -1;
}

int game::mon_at(int x, int y)
{
 for (int i = 0; i < z.size(); i++) {
  if (z[i].posx == x && z[i].posy == y) {
   if (z[i].dead)
    return -1;
   else
    return i;
  }
 }
 return -1;
}

bool game::is_empty(int x, int y)
{
 return ((m.move_cost(x, y) > 0 || m.has_flag(liquid, x, y)) &&
         npc_at(x, y) == -1 && mon_at(x, y) == -1 &&
         (u.posx != x || u.posy != y));
}

bool game::is_in_sunlight(int x, int y)
{
 return (m.is_outside(x, y) && light_level() >= 40 &&
         (weather == WEATHER_CLEAR || weather == WEATHER_SUNNY));
}



int cat_lastTurn= 0;
int cat_killBonus= 0;
void game::kill_mon(int index, bool u_did_it)
{

  if(index < 0 || index >= z.size())
  {
	add_msg("Tried to kill monster %d! (%d in play)", index, z.size());
	return;
  }

//printf(" b1");
  if( u_did_it 
	|| z[index].has_effect(ME_ONFIRE)
	|| z[index].has_effect(ME_STUNNED) )
  {
//	add_msg("You killed...");
	if (z[index].has_flag(MF_GUILT))
	{
		mdeath tmpdeath;
		tmpdeath.guilt(this, &(z[index]));
		cat_killBonus= 0;
	}
	else
	{
		if(cat_lastTurn > 10)
		{
			int diff= int(turn) - cat_lastTurn;
			if(diff >= 0 && diff < 3 && cat_killBonus < 10)
				cat_killBonus++;
			else
				cat_killBonus= 1;

			if(cat_killBonus > 1)
			{
				playSound(6); //actionUse
				u.add_morale(MORALE_FEELING_GOOD, z[index].type->size*cat_killBonus, 200);
				add_msg("You're glad those monsters are dead (x%d).", cat_killBonus);  
			}
		}

		cat_lastTurn= int(turn);
	}

//printf(" b2");
	if(z[index].type->species != species_hallu)
		kills[z[index].type->id]++;	// Increment our kill counter
  }

//printf(" b3");

/*
//CAT-mgs: doesn't monster::die drop items already?
  for (int i = 0; i < z[index].inv.size(); i++)
	m.add_item(z[index].posx, z[index].posy, z[index].inv[i]);
*/

  z[index].die(this);
}


void game::explode_mon(int index)
{
 if (index < 0 || index >= z.size()) {
/*
  dbg(D_ERROR) << "game:explode_mon: Tried to explode monster " << index
               << "! (" << z.size() << " in play)";
  debugmsg("Tried to explode monster %d! (%d in play)", index, z.size());
*/
  return;
 }
 if (!z[index].dead) {
  z[index].dead = true;
  kills[z[index].type->id]++;	// Increment our kill counter
// Send body parts and blood all over!
  mtype* corpse = z[index].type;
  if (corpse->mat == FLESH || corpse->mat == VEGGY) { // No chunks otherwise
   int num_chunks;
   switch (corpse->size) {
    case MS_TINY:   num_chunks =  1; break;
    case MS_SMALL:  num_chunks =  2; break;
    case MS_MEDIUM: num_chunks =  4; break;
    case MS_LARGE:  num_chunks =  8; break;
    case MS_HUGE:   num_chunks = 16; break;
   }
   itype* meat;
   if (corpse->has_flag(MF_POISON)) {
    if (corpse->mat == FLESH)
     meat = itypes[itm_meat_tainted];
    else
     meat = itypes[itm_veggy_tainted];
   } else {
    if (corpse->mat == FLESH)
     meat = itypes[itm_meat];
    else
     meat = itypes[itm_veggy];
   }

   int posx = z[index].posx, posy = z[index].posy;
   for (int i = 0; i < num_chunks; i++) {
    int tarx = posx + rng(-3, 3), tary = posy + rng(-3, 3);
    std::vector<point> traj = line_to(posx, posy, tarx, tary, 0);

    bool done = false;
    for (int j = 0; j < traj.size() && !done; j++) {
     tarx = traj[j].x;
     tary = traj[j].y;
// Choose a blood type and place it
     field_id blood_type = fd_blood;
     if (corpse->dies == &mdeath::boomer)
      blood_type = fd_bile;
     else if (corpse->dies == &mdeath::acid)
      blood_type = fd_acid;
     if (m.field_at(tarx, tary).type == blood_type &&
         m.field_at(tarx, tary).density < 3)
      m.field_at(tarx, tary).density++;
     else
      m.add_field(this, tarx, tary, blood_type, 1);

     if (m.move_cost(tarx, tary) == 0) {
      std::string tmp = "";
      if (m.bash(tarx, tary, 3, tmp))
       sound(tarx, tary, 18, tmp);
      else {
       if (j > 0) {
        tarx = traj[j - 1].x;
        tary = traj[j - 1].y;
       }
       done = true;
      }
     }
    }
    m.add_item(tarx, tary, meat, turn);
   }
  }
 }

 z.erase(z.begin()+index);

/*
 if (last_target == index)
  last_target = -1;
 else if (last_target > index)
   last_target--;
*/

//CAT-s:
	playSound(29); //splat1 sound
}

void game::open()
{

//CAT-g:
 mvwprintw(w_terrain, 0, 0, "Open where?");
 wrefresh(w_terrain);
 playSound(1);

 u.moves -= 100;
 bool didit = false;

// DebugLog() << __FUNCTION__ << "calling get_input() \n";
 int openx, openy;
 InputEvent input = get_input();
 last_action += input;
 get_direction(openx, openy, input);
 if (openx != -2 && openy != -2)
 {
  int vpart;
  vehicle *veh = m.veh_at(u.posx + openx, u.posy + openy, vpart);
  if (veh && veh->part_flag(vpart, vpf_openable)) {
   if (veh->parts[vpart].open) {
    add_msg("That door is already open.");
    u.moves += 100;
   } else {
    veh->parts[vpart].open = 1;
    veh->insides_dirty = true;
   }
   return;
  }

  if (m.is_indoor(u.posx, u.posy))
   didit = m.open_door(u.posx + openx, u.posy + openy, true);
  else
   didit = m.open_door(u.posx + openx, u.posy + openy, false);
 }
 else
  add_msg("Invalid direction.");

//CAT-s: ***
   if(didit)
	playSound(56);
   else
   if(!didit)
   {
	switch(m.ter(u.posx + openx, u.posy + openy))
	{
	  case t_door_locked:
	  case t_door_locked_alarm:
	   add_msg("The door is locked!");
	   break;	// Trying to open a locked door uses the full turn's movement

	  case t_door_o:
	   add_msg("That door is already open.");
	   u.moves += 100;
	   break;
	  default:
	   add_msg("No door there.");
	   u.moves += 100;
	}
   }

}

void game::close()
{
//CAT-g:
 mvwprintw(w_terrain, 0, 0, "Close where?");
 wrefresh(w_terrain);
 playSound(1);

 bool didit = false;

// DebugLog() << __FUNCTION__ << "calling get_input() \n";
 int closex, closey;
 InputEvent input = get_input();
 last_action += input;
 get_direction(closex, closey, input);
 if (closex != -2 && closey != -2) {
  closex += u.posx;
  closey += u.posy;
  int vpart;
  vehicle *veh = m.veh_at(closex, closey, vpart);
  if (mon_at(closex, closey) != -1)
   add_msg("There's a %s in the way!",z[mon_at(closex, closey)].name().c_str());
  else if (veh && veh->part_flag(vpart, vpf_openable) &&
          veh->parts[vpart].open) {
   veh->parts[vpart].open = 0;
   veh->insides_dirty = true;
   didit = true;
  } else if (m.i_at(closex, closey).size() > 0)
   add_msg("There's %s in the way!", m.i_at(closex, closey).size() == 1 ?
           m.i_at(closex, closey)[0].tname(this).c_str() : "some stuff");
  else if (closex == u.posx && closey == u.posy)
   add_msg("There's some buffoon in the way!");
  else if (m.ter(closex, closey) == t_window_domestic && m.is_outside(u.posx, u.posy))  {
   add_msg("You phase through the glass, close the curtains, then phase back out");
   add_msg("Wait, no you don't. Never mind.");
 } else
   didit = m.close_door(closex, closey, true);
 } else
  add_msg("Invalid direction.");

//CAT-s: 
   if(didit)
   {
	playSound(57);
	u.moves -= 90;
   }
}

void game::smash()
{
//CAT-g:
 mvwprintw(w_terrain, 0, 0, "Smash where?");
 wrefresh(w_terrain);
 playSound(1);

 bool didit = false;
 std::string bashsound, extra;
 int smashskill = int(u.str_cur / 2.5 + u.weapon.type->melee_dam);

// DebugLog() << __FUNCTION__ << "calling get_input() \n";
 InputEvent input = get_input();
 last_action += input;
 if (input == Close) {
  add_msg("Never mind.");
  return;
 }


 int smashx, smashy;
 get_direction(smashx, smashy, input);
 if(smashx != -2 && smashy != -2)
 {
   if(m.has_flag(alarmed, u.posx + smashx, u.posy + smashy)
	&& !event_queued(EVENT_WANTED))
   {
//CAT-s:
	playSound(58); //alarm1 sound
  
	sound(u.posx, u.posy, 50, "An alarm sounds!");
	add_event(EVENT_WANTED, int(turn) + 300, 0, levx, levy);
   }

   didit = m.bash(u.posx + smashx, u.posy + smashy, smashskill, bashsound);
 }
 else
  add_msg("Invalid direction.");

 if (didit)
 {
   if (extra != "")
     add_msg(extra.c_str());

   sound(u.posx, u.posy, 14, bashsound);

   u.moves -= 80;
   if (u.skillLevel("melee") == 0)
    u.practice("melee", rng(0, 1) * rng(0, 1));

   if (u.weapon.made_of(GLASS) &&
       rng(0, u.weapon.volume() + 3) < u.weapon.volume())
   {
	    add_msg("Your %s shatters!", u.weapon.tname(this).c_str());
	    for (int i = 0; i < u.weapon.contents.size(); i++)
	      m.add_item(u.posx, u.posy, u.weapon.contents[i]);

	    sound(u.posx, u.posy, 12, "Crash!");
	    u.hit(this, bp_hands, 1, 0, rng(0, u.weapon.volume()));

	    if (u.weapon.volume() > 20)// Hurt left arm too, if it was big
      	u.hit(this, bp_hands, 0, 0, rng(0, u.weapon.volume() * .5));

	    u.remove_weapon();
   }
 }
 else
   add_msg("There's nothing there!");
}


void game::use_item(char chInput)
{
 char ch;
 if (chInput == '.')
  ch = inv("Use item:");
 else
  ch = chInput;

 if (ch == ' ') {
  add_msg("Never mind.");
  return;
 }
 last_action += ch;
 u.use(this, ch);
}

void game::use_wielded_item()
{
  u.use_wielded(this);
}

bool game::pl_choose_vehicle (int &x, int &y)
{
//CAT-g:
// refresh_all();
// mvprintz(0, 0, c_red, "Choose a vehicle at direction:");

 mvwprintw(w_terrain, 0, 0, "Refill where?");
 wrefresh(w_terrain);
 playSound(1);

// DebugLog() << __FUNCTION__ << "calling get_input() \n";
 InputEvent input = get_input();
 int dirx, diry;
 get_direction(dirx, diry, input);
 if (dirx == -2) {
  add_msg("Invalid direction!");
  return false;
 }
 x += dirx;
 y += diry;
 return true;
}

bool game::vehicle_near ()
{
 for (int dx = -1; dx <= 1; dx++) {
  for (int dy = -1; dy <= 1; dy++) {
   if (m.veh_at(u.posx + dx, u.posy + dy))
    return true;
  }
 }
 return false;
}

bool game::pl_refill_vehicle (vehicle &veh, int part, bool test)
{
    if (!veh.part_flag(part, vpf_fuel_tank))
        return false;
    int i_itm = -1;
    item *p_itm = 0;
    int min_charges = -1;
    bool i_cont = false;

    int ftype = veh.part_info(part).fuel_type;
    itype_id itid = default_ammo((ammotype)ftype);
    if (u.weapon.is_container() && u.weapon.contents.size() > 0 && u.weapon.contents[0].type->id == itid)
    {
        i_itm = -2;
        p_itm = &u.weapon.contents[0];
        min_charges = u.weapon.contents[0].charges;
        i_cont = true;
    }
    else
    if (u.weapon.type->id == itid)
    {
        i_itm = -2;
        p_itm = &u.weapon;
        min_charges = u.weapon.charges;
    }
    else
    for (int i = 0; i < u.inv.size(); i++)
    {
        item *itm = &u.inv[i];
        bool cont = false;
        if (itm->is_container() && itm->contents.size() > 0)
        {
            cont = true;
            itm = &(itm->contents[0]);
        }
        if (itm->type->id != itid)
            continue;
        if (i_itm < 0 || min_charges > itm->charges)
        {
            i_itm = i;
            p_itm = itm;
            i_cont = cont;
            min_charges = itm->charges;
        }
    }
    if (i_itm == -1)
        return false;
    else
    if (test)
        return true;

    int fuel_per_charge = 1;
    switch (ftype)
    {
    case AT_PLUT:
//CAT: 1000 -> 100
        fuel_per_charge = 100;
        break;
    case AT_PLASMA:
//CAT: 100 -> 10
        fuel_per_charge = 10;
        break;
    default:;
    }
    int max_fuel = veh.part_info(part).size;
    int dch = (max_fuel - veh.parts[part].amount) / fuel_per_charge;
    if (dch < 1)
        dch = 1;
    bool rem_itm = min_charges <= dch;
    int used_charges = rem_itm? min_charges : dch;
    veh.parts[part].amount += used_charges * fuel_per_charge;
    if (veh.parts[part].amount > max_fuel)
        veh.parts[part].amount = max_fuel;

    add_msg ("You %s %s's %s%s.", ftype == AT_BATT? "recharge" : "refill", veh.name.c_str(),
             ftype == AT_BATT? "battery" : (ftype == AT_PLUT? "reactor" : "fuel tank"),
             veh.parts[part].amount == max_fuel? " to its maximum" : "");

    p_itm->charges -= used_charges;
    if (rem_itm)
    {
        if (i_itm == -2)
        {
            if (i_cont)
                u.weapon.contents.erase (u.weapon.contents.begin());
            else
                u.remove_weapon ();
        }
        else
        {
            if (i_cont)
                u.inv[i_itm].contents.erase (u.inv[i_itm].contents.begin());
            else
                u.inv.remove_item (i_itm);
        }
    }
    return true;
}


void game::handbrake ()
{
 vehicle *veh = m.veh_at (u.posx, u.posy);
 if(!veh)
	return;

 add_msg ("You pull a handbrake.");
 veh->cruise_velocity = 0;


//CAT-mgs: && rng (15, 60) * 100 < abs(veh->velocity)
 if(veh->last_turn != 0)
 {
   veh->skidding = true;
   add_msg ("You lose control of %s.", veh->name.c_str());
   veh->turn (veh->last_turn > 0? 45 : -45);
 } 
 else
   veh->velocity-= (int)veh->velocity / 30;


 if(abs(veh->velocity) > 900)
	playSound(118);
 else
 if(abs(veh->velocity) < 500)
 {
	veh->stop();
	veh->velocity= 0;
 }

//CAT-mgs:
 u.moves = 0;
 pldrive(0, 0);
}


void game::exam_vehicle(vehicle &veh, int examx, int examy, int cx, int cy)
{
    veh_interact vehint;
    vehint.cx = cx;
    vehint.cy = cy;
    vehint.exec(this, &veh, examx, examy);
//    debugmsg ("exam_vehicle cmd=%c %d", vehint.sel_cmd, (int) vehint.sel_cmd);
    if (vehint.sel_cmd != ' ')
    {                                                        // TODO: different activity times
        u.activity = player_activity(ACT_VEHICLE,
                                     vehint.sel_cmd == 'f'? 200 : 20000,
                                     (int) vehint.sel_cmd);
        u.activity.values.push_back (veh.global_x());    // values[0]
        u.activity.values.push_back (veh.global_y());    // values[1]
        u.activity.values.push_back (vehint.cx);   // values[2]
        u.activity.values.push_back (vehint.cy);   // values[3]
        u.activity.values.push_back (-vehint.ddx - vehint.cy);   // values[4]
        u.activity.values.push_back (vehint.cx - vehint.ddy);   // values[5]
        u.activity.values.push_back (vehint.sel_part); // values[6]
        u.moves = 0;
    }
//CAT-g:
//    refresh_all();
}

// A gate handle is adjacent to a wall section, and next to that wall section on one side or
// another is the gate.  There may be a handle on the other side, but this is optional.
// The gate continues until it reaches a non-floor tile, so they can be arbitrary length.
//
//   |  !|!        !  !
//   +   +   --++++-  -++++++++++++
//   +   +   !     !
//   +   +
//   +   |!
//  !|
//
// The terrain type of the handle is passed in, and that is used to determine the type of
// the wall and gate.
static void open_gate( game *g, const int examx, const int examy, const enum ter_id handle_type ) {

 enum ter_id v_wall_type;
 enum ter_id h_wall_type;
 enum ter_id door_type;
 enum ter_id floor_type;
 const char *pull_message;
 const char *open_message;
 const char *close_message;

 switch(handle_type) {
 case t_gates_mech_control:
  v_wall_type = t_wall_v;
  h_wall_type = t_wall_h;
  door_type   = t_door_metal_locked;
  floor_type  = t_floor;
  pull_message = "You turn the handle...";
  open_message = "The gate is opened!";
  close_message = "The gate is closed!";
  break;

 case t_barndoor:
  v_wall_type = t_wall_wood;
  h_wall_type = t_wall_wood;
  door_type   = t_door_metal_locked;
  floor_type  = t_dirtfloor;
  pull_message = "You pull the rope...";
  open_message = "The barn doors opened!";
  close_message = "The barn doors closed!";
  break;

 case t_palisade_pulley:
  v_wall_type = t_palisade;
  h_wall_type = t_palisade;
  door_type   = t_palisade_gate;
  floor_type  = t_dirt;
  pull_message = "You pull the rope...";
  open_message = "The gate!";
  close_message = "The barn doors closed!";
  break;

  default: return; // No matching gate type
 }

 g->add_msg(pull_message);
 g->u.moves -= 900;
 if (((g->m.ter(examx-1, examy)==v_wall_type)&&
      ((g->m.ter(examx-1, examy+1)==floor_type)||
       (g->m.ter(examx-1, examy-1)==floor_type))&&
      (!((g->m.ter(examx-1, examy-1)==door_type)||
	 (g->m.ter(examx-1, examy+1)==door_type))))||
     ((g->m.ter(examx+1, examy)==v_wall_type)&&
      ((g->m.ter(examx+1, examy+1)==floor_type)||
       (g->m.ter(examx+1, examy-1)==floor_type))&&
      (!((g->m.ter(examx+1, examy-1)==door_type)||
	 (g->m.ter(examx+1, examy+1)==door_type))))) {
   //horizontal orientation of the gate
   if ((g->m.ter(examx, examy-1)==h_wall_type)||
       (g->m.ter(examx, examy+1)==h_wall_type)) {
     int x_incr=0; int y_offst=0;
     if (g->m.ter(examx, examy-1)==h_wall_type) y_offst = -1;
     if (g->m.ter(examx, examy+1)==h_wall_type) y_offst = 1;
     if (g->m.ter(examx+1, examy+y_offst) == floor_type) x_incr = 1;
     if (g->m.ter(examx-1, examy+y_offst) == floor_type) x_incr = -1;
     int cur_x = examx+x_incr;
     while (g->m.ter(cur_x, examy+y_offst)== floor_type) {
       g->m.ter(cur_x, examy+y_offst) = door_type;
       cur_x = cur_x+x_incr;
     }
     //vertical orientation of the gate
   } else if ((g->m.ter(examx-1, examy)==v_wall_type)||
	      (g->m.ter(examx+1, examy)==v_wall_type)) {
     int x_offst = 0; int y_incr = 0;
     if ((g->m.ter(examx-1, examy)==v_wall_type)) x_offst = -1;
     if ((g->m.ter(examx+1, examy)==v_wall_type)) x_offst = 1;
     if (g->m.ter(examx+x_offst, examy-1)== floor_type) y_incr = -1;
     if (g->m.ter(examx+x_offst, examy+1)== floor_type) y_incr = 1;
     int cur_y = examy+y_incr;
     while (g->m.ter(examx+x_offst, cur_y)==floor_type) {
       g->m.ter(examx+x_offst, cur_y) = door_type;
       cur_y = cur_y+y_incr;
     }
   }
   g->add_msg(close_message);
 } else if (((g->m.ter(examx, examy-1)==h_wall_type)&&
	   ((g->m.ter(examx+1, examy-1)==floor_type)||
	    (g->m.ter(examx-1, examy-1)==floor_type)))||
	  ((g->m.ter(examx, examy+1)==h_wall_type)&&
	   ((g->m.ter(examx+1, examy+1)==floor_type)||
	    (g->m.ter(examx-1, examy+1)==floor_type))))
 {
   //horizontal orientation of the gate
   if ((g->m.ter(examx, examy-1)==h_wall_type)||
       (g->m.ter(examx, examy+1)==h_wall_type)) {
     int x_incr=0; int y_offst=0;
     if (g->m.ter(examx, examy-1)==h_wall_type) y_offst = -1;
     if (g->m.ter(examx, examy+1)==h_wall_type) y_offst = 1;
     if (g->m.ter(examx+1, examy+y_offst) == floor_type) x_incr = 1;
     if (g->m.ter(examx-1, examy+y_offst) == floor_type) x_incr = -1;
     int cur_x = examx+x_incr;
     while (g->m.ter(cur_x, examy+y_offst)== floor_type) {
       g->m.ter(cur_x, examy+y_offst) = door_type;
       cur_x = cur_x+x_incr;
     }
 //vertical orientation of the gate
   } else if ((g->m.ter(examx-1, examy)==v_wall_type)||
	      (g->m.ter(examx+1, examy)==v_wall_type)) {
     int x_offst = 0; int y_incr = 0;
     if ((g->m.ter(examx-1, examy)==v_wall_type)) x_offst = -1;
     if ((g->m.ter(examx+1, examy)==v_wall_type)) x_offst = 1;
     if (g->m.ter(examx+x_offst, examy-1)== floor_type) y_incr = -1;
     if (g->m.ter(examx+x_offst, examy+1)== floor_type) y_incr = 1;
     int cur_y = examy+y_incr;
     while (g->m.ter(examx+x_offst, cur_y)==floor_type) {
       g->m.ter(examx+x_offst, cur_y) = door_type;
       cur_y = cur_y+y_incr;
     }
   }

   //closing the gate...
   g->add_msg(close_message);
 }
 else //opening the gate...
 {
   //horizontal orientation of the gate
   if ((g->m.ter(examx, examy-1)==h_wall_type)||
       (g->m.ter(examx, examy+1)==h_wall_type)) {
     int x_incr=0; int y_offst=0;
     if (g->m.ter(examx, examy-1)==h_wall_type) y_offst = -1;
     if (g->m.ter(examx, examy+1)==h_wall_type) y_offst = 1;
     if (g->m.ter(examx+1, examy+y_offst) == door_type) x_incr = 1;
     if (g->m.ter(examx-1, examy+y_offst) == door_type) x_incr = -1;
     int cur_x = examx+x_incr;
     while (g->m.ter(cur_x, examy+y_offst)==door_type) {
       g->m.ter(cur_x, examy+y_offst) = floor_type;
       cur_x = cur_x+x_incr;
     }
     //vertical orientation of the gate
   } else if ((g->m.ter(examx-1, examy)==v_wall_type)||
              (g->m.ter(examx+1, examy)==v_wall_type)) {
     int x_offst = 0; int y_incr = 0;
     if ((g->m.ter(examx-1, examy)==v_wall_type)) x_offst = -1;
     if ((g->m.ter(examx+1, examy)==v_wall_type)) x_offst = 1;
     if (g->m.ter(examx+x_offst, examy-1)== door_type) y_incr = -1;
     if (g->m.ter(examx+x_offst, examy+1)== door_type) y_incr = 1;
     int cur_y = examy+y_incr;
     while (g->m.ter(examx+x_offst, cur_y)==door_type) {
       g->m.ter(examx+x_offst, cur_y) = floor_type;
       cur_y = cur_y+y_incr;
     }
   }
   g->add_msg(open_message);
 }
}

void game::examine()
{
 if (u.in_vehicle) {
  int vpart;
  vehicle *veh = m.veh_at(u.posx, u.posy, vpart);
  bool qexv = (veh && (veh->velocity != 0 ?
                       query_yn("Really exit moving vehicle?") :
                       query_yn("Exit vehicle?")));
  if (qexv) {
   m.unboard_vehicle (this, u.posx, u.posy);
   u.moves -= 200;
   if (veh->velocity) {      // TODO: move player out of harms way
    int dsgn = veh->parts[vpart].mount_dx > 0? 1 : -1;
    fling_player_or_monster(&u, 0, veh->face.dir() + 90 * dsgn, 35);
   }
//CAT-g:
//   return;
  }
//CAT-g: needs to be here
   return;
 }

//CAT-g:
// mvwprintw(w_terrain, 0, 0, "Examine where? (Direction button) ");
// wrefresh(w_terrain);

 mvwprintw(w_terrain, 0, 0, "Examine where?");
 wrefresh(w_terrain);
 playSound(1);

// DebugLog() << __FUNCTION__ << "calling get_input() \n";
 int examx, examy;
 InputEvent input = get_input();
 last_action += input;
 if (input == Cancel || input == Close)
  return;
 get_direction(examx, examy, input);
 if (examx == -2 || examy == -2) {
  add_msg("Invalid direction.");
  return;
 }
 examx += u.posx;
 examy += u.posy;
 add_msg("That is a %s.", m.tername(examx, examy).c_str());

 int veh_part = 0;
 vehicle *veh = m.veh_at (examx, examy, veh_part);
 if (veh) {
  int vpcargo = veh->part_with_feature(veh_part, vpf_cargo, false);
  if (vpcargo >= 0 && veh->parts[vpcargo].items.size() > 0)
   pickup(examx, examy, 0);
  else if (u.in_vehicle)
   add_msg ("You can't do that while onboard.");
  else if (abs(veh->velocity) > 0)
   add_msg ("You can't do that on moving vehicle.");
  else
   exam_vehicle (*veh, examx, examy);
 } else if (m.has_flag(sealed, examx, examy)) {
  if (m.trans(examx, examy)) {
   std::string buff;
   if (m.i_at(examx, examy).size() <= 3 && m.i_at(examx, examy).size() != 0) {
    buff = "It contains ";
    for (int i = 0; i < m.i_at(examx, examy).size(); i++) {
     buff += m.i_at(examx, examy)[i].tname(this);
     if (i + 2 < m.i_at(examx, examy).size())
      buff += ", ";
     else if (i + 1 < m.i_at(examx, examy).size())
      buff += ", and ";
    }
    buff += ",";
   } else if (m.i_at(examx, examy).size() != 0)
    buff = "It contains many items,";
   buff += " but is firmly sealed.";
   add_msg(buff.c_str());
  } else {
   add_msg("There's something in there, but you can't see what it is, and the\
 %s is firmly sealed.", m.tername(examx, examy).c_str());
  }
 } else {
  if (m.i_at(examx, examy).size() == 0 && m.has_flag(container, examx, examy) &&
      !(m.has_flag(swimmable, examx, examy) || m.ter(examx, examy) == t_toilet))
   add_msg("It is empty.");
  else
   pickup(examx, examy, 0);
 }
 if (m.has_flag(console, examx, examy)) {
  use_computer(examx, examy);
  return;
 }
 if (m.ter(examx, examy) == t_card_science ||
     m.ter(examx, examy) == t_card_military  ) {
  itype_id card_type = (m.ter(examx, examy) == t_card_science ? itm_id_science :
                                                               itm_id_military);
  if (u.has_amount(card_type, 1) && query_yn("Swipe your ID card?")) {
   u.moves -= 100;
   for (int i = -3; i <= 3; i++) {
    for (int j = -3; j <= 3; j++) {
     if (m.ter(examx + i, examy + j) == t_door_metal_locked)
      m.ter(examx + i, examy + j) = t_floor;
    }
   }
   for (int i = 0; i < z.size(); i++) {
    if (z[i].type->id == mon_turret) {
     z.erase(z.begin() + i);
     i--;
    }
   }
   add_msg("You insert your ID card.");
   add_msg("The nearby doors slide into the floor.");
   u.use_amount(card_type, 1);
  } else {
   bool using_electrohack = (u.has_amount(itm_electrohack, 1) &&
                             query_yn("Use electrohack on the reader?"));
   bool using_fingerhack = (!using_electrohack && u.has_bionic(bio_fingerhack) &&
                            u.power_level > 0 &&
                            query_yn("Use fingerhack on the reader?"));
   if (using_electrohack || using_fingerhack) {
    u.moves -= 500;
    u.practice("computer", 20);
    int success = rng(u.skillLevel("computer") / 4 - 2, u.skillLevel("computer") * 2);
    success += rng(-3, 3);
    if (using_fingerhack)
     success++;
    if (u.int_cur < 8)
     success -= rng(0, int((8 - u.int_cur) / 2));
    else if (u.int_cur > 8)
     success += rng(0, int((u.int_cur - 8) / 2));
    if (success < 0) {
     add_msg("You cause a short circuit!");
     if (success <= -5) {
      if (using_electrohack) {
       add_msg("Your electrohack is ruined!");
       u.use_amount(itm_electrohack, 1);
      } else {
       add_msg("Your power is drained!");
       u.charge_power(0 - rng(0, u.power_level));
      }
     }
     m.ter(examx, examy) = t_card_reader_broken;
    } else if (success < 6)
     add_msg("Nothing happens.");
    else {
     add_msg("You activate the panel!");
     add_msg("The nearby doors slide into the floor.");
     m.ter(examx, examy) = t_card_reader_broken;
     for (int i = -3; i <= 3; i++) {
      for (int j = -3; j <= 3; j++) {
       if (m.ter(examx + i, examy + j) == t_door_metal_locked)
        m.ter(examx + i, examy + j) = t_floor;
      }
     }
    }
   }
  }
 } else if (m.ter(examx, examy) == t_elevator_control &&
            query_yn("Activate elevator?")) {
  int movez = (levz < 0 ? 2 : -2);
  levz += movez;
  m.load(this, levx, levy, levz);

//CAT-g: why in examine?, should be in do_turn()
  update_map(u.posx, u.posy);

  for (int x = 0; x < SEEX * MAPSIZE; x++) {
   for (int y = 0; y < SEEY * MAPSIZE; y++) {
    if (m.ter(x, y) == t_elevator) {
     u.posx = x;
     u.posy = y;
    }
   }
  }
  refresh_all();
}
 else if (m.ter(examx, examy) == t_gates_mech_control && query_yn("Use this winch?"))
 {
   open_gate( this, examx, examy, t_gates_mech_control );
 }
 else if (m.ter(examx, examy) == t_barndoor && query_yn("Pull the rope?"))
 {
   open_gate( this, examx, examy, t_barndoor );
 }
 else if (m.ter(examx, examy) == t_palisade_pulley && query_yn("Pull the rope?"))
 {
   open_gate( this, examx, examy, t_palisade_pulley );
 }

 else if (m.ter(examx, examy) == t_rubble && u.has_amount(itm_shovel, 1)) {
  if (query_yn("Clear up that rubble?")) {
  if (levz == -1) {
   u.moves -= 200;
   m.ter(examx, examy) = t_rock_floor;
   item rock(itypes[itm_rock], turn);
   m.add_item(u.posx, u.posy, rock);
   m.add_item(u.posx, u.posy, rock);
   add_msg("You clear the rubble up");
 } else {
   u.moves -= 200;
   m.ter(examx, examy) = t_dirt;
   item rock(itypes[itm_rock], turn);
   m.add_item(u.posx, u.posy, rock);
   m.add_item(u.posx, u.posy, rock);
   add_msg("You clear the rubble up");
 }} else {
   add_msg("You need a shovel to do that!");
  }
 } else if (m.ter(examx, examy) == t_ash && u.has_amount(itm_shovel, 1)) {
  if (query_yn("Clear up that rubble?")) {
  if (levz == -1) {
   u.moves -= 200;
   m.ter(examx, examy) = t_rock_floor;
   add_msg("You clear the ash up");
 } else {
   u.moves -= 200;
   m.ter(examx, examy) = t_dirt;
   add_msg("You clear the ash up");
 }} else {
   add_msg("You need a shovel to do that!");
  }
 } else if (m.ter(examx, examy) == t_chainfence_v && query_yn("Climb fence?") ||
            m.ter(examx, examy) == t_chainfence_h && query_yn("Climb fence?")) {
   u.moves -= 400;

  if (one_in(u.dex_cur))
   add_msg("You slip whilst climbing and fall down again");
  else
  {
   u.moves += u.dex_cur * 10;
   u.posx = examx;
   u.posy = examy;

//CAT-s: hoya!
	playSound(38);
  }

 } else if (m.ter(examx, examy) == t_groundsheet && query_yn("Take down tent?")) {
   u.moves -= 200;
   for (int i = -1; i <= 1; i++)
    for (int j = -1; j <= 1; j++)
    m.ter(examx + i, examy + j) = t_dirt;
  add_msg("You take down the tent");
  item tent(itypes[itm_tent_kit], turn);
  m.add_item(examx, examy, tent);

 } else if (m.ter(examx, examy) == t_skin_groundsheet && query_yn("Take down shelter?")) {
   u.moves -= 200;
   for (int i = -1; i <= 1; i++)
    for (int j = -1; j <= 1; j++)
    m.ter(examx + i, examy + j) = t_dirt;
  add_msg("You take down the shelter");
  item tent(itypes[itm_shelter_kit], turn);
  m.add_item(examx, examy, tent);

 } else if (m.ter(examx, examy) == t_wreckage && u.has_amount(itm_shovel, 1)) {
  if (query_yn("Clear up that wreckage?")) {
   u.moves -= 200;
   m.ter(examx, examy) = t_dirt;
   item chunk(itypes[itm_steel_chunk], turn);
   item scrap(itypes[itm_scrap], turn);
   item pipe(itypes[itm_pipe], turn);
   item wire(itypes[itm_wire], turn);
   m.add_item(examx, examy, chunk);
   m.add_item(examx, examy, scrap);
  if (one_in(5)) {
   m.add_item(examx, examy, pipe);
   m.add_item(examx, examy, wire); }
   add_msg("You clear the wreckage up");
 } else {
   add_msg("You need a shovel to do that!");
  }
 } else if (m.ter(examx, examy) == t_metal && u.has_amount(itm_shovel, 1)) {
  if (query_yn("Clear up that wreckage?")) {
   u.moves -= 200;
   m.ter(examx, examy) = t_floor;
   item chunk(itypes[itm_steel_chunk], turn);
   item scrap(itypes[itm_scrap], turn);
   item pipe(itypes[itm_pipe], turn);
   m.add_item(examx, examy, chunk);
   m.add_item(examx, examy, scrap);
  if (one_in(5)) {
   m.add_item(examx, examy, pipe);}
   add_msg("You clear the wreckage up");
 } else {
   add_msg("You need a shovel to do that!");
  }
 } else if (m.ter(examx, examy) == t_pit && u.has_amount(itm_2x4, 1)) {
  if (query_yn("Place a plank over the pit?")) {
   u.use_amount(itm_2x4, 1);
   m.ter(examx, examy) = t_pit_covered;
   add_msg("You place a plank of wood over the pit");
 } else {
   add_msg("You need a plank of wood to do that");
  }
 } else if (m.ter(examx, examy) == t_pit_spiked && u.has_amount(itm_2x4, 1)) {
  if (query_yn("Place a plank over the pit?")) {
   u.use_amount(itm_2x4, 1);
   m.ter(examx, examy) = t_pit_spiked_covered;
   add_msg("You place a plank of wood over the pit");
 } else {
   add_msg("You need a plank of wood to do that");
  }
 } else if (m.ter(examx, examy) == t_pit_covered && query_yn("Remove that plank?")) {
    item plank(itypes[itm_2x4], turn);
    add_msg("You remove the plank.");
     m.add_item(u.posx, u.posy, plank);
     m.ter(examx, examy) = t_pit;
 } else if (m.ter(examx, examy) == t_pit_spiked_covered && query_yn("Remove that plank?")) {
    item plank(itypes[itm_2x4], turn);
    add_msg("You remove the plank.");
     m.add_item(u.posx, u.posy, plank);
     m.ter(examx, examy) = t_pit_spiked;
 } else if (m.ter(examx, examy) == t_gas_pump && query_yn("Pump gas?")) {
  item gas(itypes[itm_gasoline], turn);
  if (one_in(10 + u.dex_cur)) {
   add_msg("You accidentally spill the gasoline.");
   m.add_item(u.posx, u.posy, gas);
  } else {
   u.moves -= 300;
   handle_liquid(gas, false, true);
  }
  if (one_in(20)) {
    add_msg("With a clang and a shudder, the gas pump goes silent.");
    m.ter(examx, examy) = t_gas_pump_empty;
  }
 } else if (m.ter(examx, examy) == t_fence_post && query_yn("Make Fence?")) {
  int ch = menu("Fence Construction:", "Rope Fence", "Wire Fence",
                "Barbed Wire Fence", "Cancel", NULL);
  switch (ch){
   case 1:{
  if (u.has_amount(itm_rope_6, 2)) {
   u.use_amount(itm_rope_6, 2);
   m.ter(examx, examy) = t_fence_rope;
   u.moves -= 200;
  } else
   add_msg("You need 2 six-foot lengths of rope to do that");
  } break;

   case 2:{
  if (u.has_amount(itm_wire, 2)) {
   u.use_amount(itm_wire, 2);
   m.ter(examx, examy) = t_fence_wire;
   u.moves -= 200;
  } else
   add_msg("You need 2 lengths of wire to do that!");
  } break;

   case 3:{
  if (u.has_amount(itm_wire_barbed, 2)) {
   u.use_amount(itm_wire_barbed, 2);
   m.ter(examx, examy) = t_fence_barbed;
   u.moves -= 200;
  } else
   add_msg("You need 2 lengths of barbed wire to do that!");
  } break;

   case 4:
   break;
  }
 } else if (m.ter(examx, examy) == t_fence_rope && query_yn("Remove fence material?")) {
  item rope(itypes[itm_rope_6], turn);
  m.add_item(u.posx, u.posy, rope);
  m.add_item(u.posx, u.posy, rope);
  m.ter(examx, examy) = t_fence_post;
  u.moves -= 200;

 } else if (m.ter(examx, examy) == t_fence_wire && query_yn("Remove fence material?")) {
  item rope(itypes[itm_wire], turn);
  m.add_item(u.posx, u.posy, rope);
  m.add_item(u.posx, u.posy, rope);
  m.ter(examx, examy) = t_fence_post;
  u.moves -= 200;
 } else if (m.ter(examx, examy) == t_fence_barbed && query_yn("Remove fence material?")) {
  item rope(itypes[itm_wire_barbed], turn);
  m.add_item(u.posx, u.posy, rope);
  m.add_item(u.posx, u.posy, rope);
  m.ter(examx, examy) = t_fence_post;
  u.moves -= 200;

 } else if (m.ter(examx, examy) == t_slot_machine) {
  if (u.cash < 10)
   add_msg("You need $10 to play.");
  else if (query_yn("Insert $10?")) {
   do {
    if (one_in(5))
     popup("Three cherries... you get your money back!");
    else if (one_in(20)) {
     popup("Three bells... you win $50!");
     u.cash += 40;	// Minus the $10 we wagered
    } else if (one_in(50)) {
     popup("Three stars... you win $200!");
     u.cash += 190;
    } else if (one_in(1000)) {
     popup("JACKPOT!  You win $5000!");
     u.cash += 4990;
    } else {
     popup("No win.");
     u.cash -= 10;
    }
   } while (u.cash >= 10 && query_yn("Play again?"));
  }
 } else if (m.ter(examx, examy) == t_bulletin) {
 	basecamp *camp = m.camp_at(examx, examy);
 	if (camp && camp->board_x() == examx && camp->board_y() == examy) {
 		std::vector<std::string> options;
 		options.push_back("Cancel");
 		int choice = menu_vec(camp->board_name().c_str(), options) - 1;
 	}
 	else {
 		bool create_camp = m.allow_camp(examx, examy);
 		std::vector<std::string> options;
 		if (create_camp)
 			options.push_back("Create camp");
 		options.push_back("Cancel");
 		// TODO: Other Bulletin Boards
 		int choice = menu_vec("Bulletin Board", options) - 1;
 		if (choice >= 0 && choice < options.size()) {
  		if (options[choice] == "Create camp") {
  			// TODO: Allow text entry for name
 	 		m.add_camp("Home", examx, examy);
 		 }
 		}
  }
 } else if (m.ter(examx, examy) == t_fault) {
  popup("\
This wall is perfectly vertical.  Odd, twisted holes are set in it, leading\n\
as far back into the solid rock as you can see.  The holes are humanoid in\n\
shape, but with long, twisted, distended limbs.");
 } else if (m.ter(examx, examy) == t_pedestal_wyrm &&
            m.i_at(examx, examy).empty()) {
  add_msg("The pedestal sinks into the ground...");
  m.ter(examx, examy) = t_rock_floor;
  add_event(EVENT_SPAWN_WYRMS, int(turn) + rng(5, 10));
 } else if (m.ter(examx, examy) == t_pedestal_temple) {
  if (m.i_at(examx, examy).size() == 1 &&
      m.i_at(examx, examy)[0].type->id == itm_petrified_eye) {
   add_msg("The pedestal sinks into the ground...");
   m.ter(examx, examy) = t_dirt;
   m.i_at(examx, examy).clear();
   add_event(EVENT_TEMPLE_OPEN, int(turn) + 4);
  } else if (u.has_amount(itm_petrified_eye, 1) &&
             query_yn("Place your petrified eye on the pedestal?")) {
   u.use_amount(itm_petrified_eye, 1);
   add_msg("The pedestal sinks into the ground...");
   m.ter(examx, examy) = t_dirt;
   add_event(EVENT_TEMPLE_OPEN, int(turn) + 4);
  } else
   add_msg("This pedestal is engraved in eye-shaped diagrams, and has a large\
 semi-spherical indentation at the top.");
 } else if (m.ter(examx, examy) >= t_switch_rg &&
            m.ter(examx, examy) <= t_switch_even &&
            query_yn("Flip the %s?", m.tername(examx, examy).c_str())) {
  u.moves -= 100;
  for (int y = examy; y <= examy + 5; y++) {
   for (int x = 0; x < SEEX * MAPSIZE; x++) {
    switch (m.ter(examx, examy)) {
     case t_switch_rg:
      if (m.ter(x, y) == t_rock_red)
       m.ter(x, y) = t_floor_red;
      else if (m.ter(x, y) == t_floor_red)
       m.ter(x, y) = t_rock_red;
      else if (m.ter(x, y) == t_rock_green)
       m.ter(x, y) = t_floor_green;
      else if (m.ter(x, y) == t_floor_green)
       m.ter(x, y) = t_rock_green;
      break;
     case t_switch_gb:
      if (m.ter(x, y) == t_rock_blue)
       m.ter(x, y) = t_floor_blue;
      else if (m.ter(x, y) == t_floor_blue)
       m.ter(x, y) = t_rock_blue;
      else if (m.ter(x, y) == t_rock_green)
       m.ter(x, y) = t_floor_green;
      else if (m.ter(x, y) == t_floor_green)
       m.ter(x, y) = t_rock_green;
      break;
     case t_switch_rb:
      if (m.ter(x, y) == t_rock_blue)
       m.ter(x, y) = t_floor_blue;
      else if (m.ter(x, y) == t_floor_blue)
       m.ter(x, y) = t_rock_blue;
      else if (m.ter(x, y) == t_rock_red)
       m.ter(x, y) = t_floor_red;
      else if (m.ter(x, y) == t_floor_red)
       m.ter(x, y) = t_rock_red;
      break;
     case t_switch_even:
      if ((y - examy) % 2 == 1) {
       if (m.ter(x, y) == t_rock_red)
        m.ter(x, y) = t_floor_red;
       else if (m.ter(x, y) == t_floor_red)
        m.ter(x, y) = t_rock_red;
       else if (m.ter(x, y) == t_rock_green)
        m.ter(x, y) = t_floor_green;
       else if (m.ter(x, y) == t_floor_green)
        m.ter(x, y) = t_rock_green;
       else if (m.ter(x, y) == t_rock_blue)
        m.ter(x, y) = t_floor_blue;
       else if (m.ter(x, y) == t_floor_blue)
        m.ter(x, y) = t_rock_blue;
      }
      break;
    }
   }
  }
  add_msg("You hear the rumble of rock shifting.");
  add_event(EVENT_TEMPLE_SPAWN, turn + 3);
 }
 //-----Jovan's-----
 //flowers
 else if ((m.ter(examx, examy)==t_mutpoppy)&&(query_yn("Pick the flower?"))) {
  add_msg("This flower has a heady aroma");
  if (!(u.is_wearing(itm_mask_filter)||u.is_wearing(itm_mask_gas) ||
      one_in(3)))  {
   add_msg("You fall asleep...");
   u.add_disease(DI_SLEEP, 1200, this);
   add_msg("Your legs are covered by flower's roots!");
   u.hurt(this,bp_legs, 0, 4);
   u.moves-=50;
  }
  m.ter(examx, examy) = t_dirt;
  m.add_item(examx, examy, this->itypes[itm_poppy_flower],0);
  m.add_item(examx, examy, this->itypes[itm_poppy_bud],0);
 }
// apple trees
 else if ((m.ter(examx, examy)==t_tree_apple) && (query_yn("Pick apples?")))
 {
  int num_apples = rng(1, u.skillLevel("survival"));
  if (num_apples >= 12)
    num_apples = 12;
  for (int i = 0; i < num_apples; i++)
   m.add_item(examx, examy, this->itypes[itm_apple],0);

  m.ter(examx, examy) = t_tree;
 }
// blueberry bushes
 else if ((m.ter(examx, examy)==t_shrub_blueberry) && (query_yn("Pick blueberries?")))
 {
  int num_blueberries = rng(1, u.skillLevel("survival"));

 if (num_blueberries >= 12)
    num_blueberries = 12;
  for (int i = 0; i < num_blueberries; i++)
   m.add_item(examx, examy, this->itypes[itm_blueberries],0);

  m.ter(examx, examy) = t_shrub;
 }

// harvesting wild veggies
 else if ((m.ter(examx, examy)==t_underbrush) && (query_yn("Forage for wild vegetables?")))
 {
  u.assign_activity(this, ACT_FORAGE, 500 / (u.skillLevel("survival") + 1), 0);
  u.activity.placement = point(examx, examy);
  u.moves = 0;
 }

 //-----Recycling machine-----
 else if ((m.ter(examx, examy)==t_recycler)&&(query_yn("Use the recycler?"))) {
  if (m.i_at(examx, examy).size() > 0)
  {
   sound(examx, examy, 80, "Ka-klunk!");
   int num_metal = 0;
   for (int i = 0; i < m.i_at(examx, examy).size(); i++)
   {
    item *it = &(m.i_at(examx, examy)[i]);
    if (it->made_of(STEEL))
     num_metal++;
    m.i_at(examx, examy).erase(m.i_at(examx, examy).begin() + i);
    i--;
   }
   if (num_metal > 0)
   {
    while (num_metal > 9)
    {
     m.add_item(u.posx, u.posy, this->itypes[itm_steel_lump], 0);
     num_metal -= 10;
    }
    do
    {
     m.add_item(u.posx, u.posy, this->itypes[itm_steel_chunk], 0);
     num_metal -= 3;
    } while (num_metal > 2);
   }
  }
  else add_msg("The recycler is empty.");
 }

 //-----------------
 if (m.tr_at(examx, examy) != tr_null &&
      traps[m.tr_at(examx, examy)]->difficulty < 99 &&
     u.per_cur-u.encumb(bp_eyes) >= traps[m.tr_at(examx, examy)]->visibility &&
     query_yn("There is a %s there.  Disarm?",
              traps[m.tr_at(examx, examy)]->name.c_str()))
  m.disarm_trap(this, examx, examy);
}



//CAT-mgs: *** vvv
void game::runJump(bool just_kick)
{

 if(u.in_vehicle && !just_kick) 
 {
//CAT-s: menuWrong sound
	playSound(3);	
	return;
 }

 if(just_kick)
 {

//CAT-mgs: KICK ********************************* vvv
   refresh_all();
   mvwprintw(w_terrain, 0, 0, "Kick/Push where?");
   wrefresh(w_terrain);
   playSound(1);

   int dirx, diry;
   get_direction(this, dirx, diry, input());
   if(dirx == -2)
   {
	add_msg("Invalid direction!");
	return;
   }


   int x = dirx + u.posx;
   int y = diry + u.posy;

//CAT-mgs: any furniture to kick?
   int required_str = 0;
   int mle= u.str_cur + rng(7+int(u.skillLevel("melee")/2), 9+u.skillLevel("melee"));
   if(mle > 40)
	mle= 40;
  
   switch(m.ter(x, y))
   {
	  case t_fridge:
	  case t_glass_fridge:
	   required_str = 10;
	   break;
	  case t_bookcase:
	  case t_locker:
	   required_str = 9;
	   break;
	  case t_dresser:
	  case t_rack:
	  case t_chair:
	  case t_armchair:
	  case t_bench:
	  case t_table:
	  case t_desk:
	   required_str = 8;
	   break;
   }

	int angle= 360;
	if(dirx == 0 && diry == -1)
		angle= 0;
	else 
	if(dirx == 1 && diry == -1)
		angle= 45;
	else 
	if(dirx == 1 && diry == 0)
		angle= 90;
	else 
	if(dirx == 1 && diry == 1)
		angle= 135;
	else 
	if(dirx == 0 && diry == 1)
		angle= 180;
	else 
	if(dirx == -1 && diry == 1)
		angle= 225;
	else 
	if(dirx == -1 && diry == 0)
		angle= 270;
	else 
	if(dirx == -1 && diry == -1)
		angle= 315;

   int xd=x+dirx;
   int yd=y+diry;
   int mi = mon_at(x, y);

   if(mi >= 0)
   {
	mle= mle*2 - z[mi].type->size*10;
	if(mle < 5)
		mle= 5;

	add_msg("You kick it with %d points of power!", mle);
	fling_player_or_monster(0, &z[mi], angle-90, mle);
	if(mle > 25 && !one_in(1+int(u.skillLevel("melee")/2)))
		z[mi].add_effect(ME_STUNNED, int(1+int(u.skillLevel("melee")/2)));


//CAT-s: hoya & noiseWhack
	playSound(38);
	playSound(94);
   }
   else	
   if(required_str > 0 && u.str_cur > required_str && m.ter(xd, yd) == t_floor)
   {

//CAT-s: hoya & noiseWhack
	playSound(94);
	mi = mon_at(xd, yd);
	if(mi >= 0)
		fling_player_or_monster(0, &z[mi], angle-90, 30);

//	if(is_empty(xd, yd))
	{
		m.ter(xd, yd) = m.ter(x, y);
		m.ter(x, y) = t_floor;
	}
   }
   else
   {
//CAt-s: punchMiss
	playSound(47);

	std::string junk;
	m.bash(x, y, int(mle*1.5), junk);
//	add_msg("You kick emty air.");
   }

   u.moves-= 140;
return;

 }

//CAT-mgs: JUMP ********************************* vvv
 u.view_offset_x= 0;
 u.view_offset_y= 0;
 int lx = u.posx;
 int ly = u.posy;

 refresh_all();
 mvwprintw(w_terrain, 0, 0, "Jump where?");
 mvwputch(w_terrain, VIEWY, VIEWX, c_red, 'X');
 wrefresh(w_terrain);

 int mx, my, junk;
 InputEvent input;

 int landX, dx;
 int landY, dy;

 do
 {
	input = get_input();
	get_direction(mx, my, input);

	if(mx != -2 && my != -2)
	{
	// Directional key pressed
	   lx += mx;
	   ly += my;
	}


//	g->draw();
//	g->draw_ter(lx, ly);

	draw_ter();


	dx= lx - u.posx;
	dy= ly - u.posy;

	if(dx < -8)
		lx++;
	else
	if(dx > 8)
		lx--;

	if(dy < -8)
		ly++;
	else
	if(dy > 8)
		ly--;


	mvwputch(w_terrain, ly - u.posy + VIEWY, 
			lx - u.posx + VIEWX, c_yellow, 'x');


//	float dist= 1.5 + sqrt(dx*dx + dy*dy);
	float dist= 1.2 + sqrt(dx*dx + dy*dy);


	landX= lx+ (int)(dx/ (dist/5));
	landY= ly+ (int)(dy/ (dist/5));


	mvwputch(w_terrain, landY - u.posy + VIEWY, 
			landX - u.posx + VIEWX, c_ltred, 'X');

	wrefresh(w_terrain);

 } while (input != Close && input != Cancel && input != Confirm);

 if(input == Confirm)
 {
	add_msg("You jump.");
	refresh_all();


	timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = 90000000;

	std::vector<point> ret;

	int blah;

	ret= line_to(u.posx, u.posy, lx, ly, blah); 

	dx= ret[0].x - u.posx;
	dy= ret[0].y - u.posy;

	plmove(dx, dy);		
	draw_ter();

	JUMPING= 1;
	for(int i= 1; i < ret.size(); i++) 
	{
		dx= ret[i].x - ret[i-1].x;
		dy= ret[i].y - ret[i-1].y;


		plmove(dx, dy);		
		draw_ter();
	}

	JUMPING= 2;
	playSound(38);

	ret= line_to(lx, ly, landX, landY, blah); 

	dx= ret[0].x - lx;
	dy= ret[0].y - ly;
	plmove(dx, dy);
	draw_ter();

//CAT-mgs:
  wrefresh(0, true);

	nanosleep(&ts, NULL);	
	for(int i= 1; i < ret.size(); i++) 
	{
		dx= ret[i].x - ret[i-1].x;
		dy= ret[i].y - ret[i-1].y;
	
		plmove(dx, dy);		
		draw_ter();

//CAT: shadow or motion blur, not looking good enough?
//		mvwputch(w_terrain, VIEWY -dy, VIEWX -dx , c_dkgray, '@');

		wrefresh(w_terrain);
//CAT-mgs:
  wrefresh(0, true);

		nanosleep(&ts, NULL);	
	}

	JUMPING= 3; //landing
	playSound(55);
	plmove(0, 0);

	JUMPING= 0;

//	g->u.view_offset_x= lx - g->u.posx;
//	g->u.view_offset_y= ly - g->u.posy;
 }
 else
 {
	u.view_offset_x= 0;
	u.view_offset_y= 0;
 }


// add_msg("1234 1234 1234 1234 1234 1234 1234 1234 1234 1234 1234 1234 1234 1234 1234 1234 1234 1234");

//   add_msg("The blah glows brightly!");
//   add_event(EVENT_ARTIFACT_LIGHT, int(turn) + 30);

//   add_msg("The sky starts to dim.");
//   add_event(EVENT_DIM, int(turn) + 50);
}



//Shift player by one tile, look_around(), then restore previous position.
//represents carfully peeking around a corner, hence the large move cost.
void game::peek()
{

//CAT-g:
 mvwprintw(w_terrain, 0, 0, "Peek from where?");
 wrefresh(w_terrain);
 playSound(1);

 int mx, my;
 InputEvent input;
 input = get_input();
 get_direction (mx, my, input);
 if (mx != -2 && my != -2 &&
     m.move_cost(u.posx + mx, u.posy + my) > 0) {
  u.moves -= 200;
  u.posx += mx;
  u.posy += my;
  look_around();
  u.posx -= mx;
  u.posy -= my;
 }
}

point game::look_around()
{
//CAT-mgs: 
 bool night_vision= false;
 if( (u.is_wearing(itm_goggles_nv) && u.has_active_item(itm_UPS_on)) 
	|| u.has_active_bionic(bio_night_vision) )
    night_vision= true;

//CAT-mgs: comment out?
// draw_ter();

//CAT-mgs: 
 u.view_offset_x= 0;
 u.view_offset_y= 0;
 int lx = u.posx + u.view_offset_x;
 int ly = u.posy + u.view_offset_y;


 int mx, my, junk;
 InputEvent input;
 WINDOW* w_look = newwin(13, 48, 12+VIEW_OFFSET_Y, VIEWX * 2 + 8+VIEW_OFFSET_X);
 wborder(w_look, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                 LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 mvwprintz(w_look, 1, 1, c_white, "Looking Around");
 mvwprintz(w_look, 2, 1, c_white, "Use directional keys.");
 mvwprintz(w_look, 3, 1, c_white, "Press ENTER to lock-on, ESC to re-center.");

//CAT-mgs:
 draw_ter(lx, ly);
 wrefresh(w_look);
 wrefresh(w_terrain);

 do {
// DebugLog() << __FUNCTION__ << "calling get_input() \n";
  input = get_input();
  if (!u_see(lx, ly, junk))
   mvwputch(w_terrain, ly - u.posy + VIEWY, lx - u.posx + VIEWX, c_black, ' ');
  get_direction(mx, my, input);
  if (mx != -2 && my != -2) {	// Directional key pressed
   lx += mx;
   ly += my;
  }


  u.view_offset_x= lx- u.posx;
  u.view_offset_y= ly - u.posy;

//CAT-g: 
//  werase(w_terrain);
  draw_ter(lx, ly);

//CAT-g: do we need this?
  for (int i = 1; i < 12; i++) {
   for (int j = 1; j < 47; j++)
    mvwputch(w_look, i, j, c_white, ' ');
  }

//CAT-g: enable this, add range
  mvwprintw(w_look, 6, 1, "Items: %d", m.i_at(lx, ly).size() );
  mvwprintw(w_look, 7, 1, "Range: %d", rl_dist(u.posx, u.posy, lx, ly));

  int veh_part = 0;
  vehicle *veh = m.veh_at(lx, ly, veh_part);
  if (u_see(lx, ly, junk)) {
   if (m.move_cost(lx, ly) == 0)
    mvwprintw(w_look, 1, 1, "%s; Impassable", m.tername(lx, ly).c_str());
   else
    mvwprintw(w_look, 1, 1, "%s; Movement cost %d", m.tername(lx, ly).c_str(),
                                                    m.move_cost(lx, ly) * 50);
   mvwprintw(w_look, 2, 1, "%s", m.features(lx, ly).c_str());
   field tmpfield = m.field_at(lx, ly);
   if (tmpfield.type != fd_null)
    mvwprintz(w_look, 4, 1, fieldlist[tmpfield.type].color[tmpfield.density-1],
              "%s", fieldlist[tmpfield.type].name[tmpfield.density-1].c_str());
   if (m.tr_at(lx, ly) != tr_null &&
       u.per_cur - u.encumb(bp_eyes) >= traps[m.tr_at(lx, ly)]->visibility)
    mvwprintz(w_look, 5, 1, traps[m.tr_at(lx, ly)]->color, "%s",
              traps[m.tr_at(lx, ly)]->name.c_str());

   int dex = mon_at(lx, ly);
   if (dex != -1 && u_see(&(z[dex]), junk)) {
    z[mon_at(lx, ly)].draw(w_terrain, lx, ly, true, night_vision);
    z[mon_at(lx, ly)].print_info(this, w_look);

//CAT-mgs:
    if(z[mon_at(lx, ly)].friendly == -1) 
		mvwprintw(w_look, 4,1, "PET LV. %d",
			int(z[mon_at(lx, ly)].speed/40), z[mon_at(lx, ly)].speed);

//	mvwprintw(w_look, 4,2, "TSP:%d MLE:%d HP2:%d",
//		z[mon_at(lx, ly)].type->speed, z[mon_at(lx, ly)].type->melee_skill, z[mon_at(lx, ly)].type->hp);


    if (!m.has_flag(container, lx, ly))
     if (m.i_at(lx, ly).size() > 1)
      mvwprintw(w_look, 3, 1, "There are several items there.");
     else if (m.i_at(lx, ly).size() == 1)
      mvwprintw(w_look, 3, 1, "There is an item there.");
   } else if (npc_at(lx, ly) != -1) {
    active_npc[npc_at(lx, ly)].draw(w_terrain, lx, ly, true, night_vision);
    active_npc[npc_at(lx, ly)].print_info(w_look);
    if (!m.has_flag(container, lx, ly))
     if (m.i_at(lx, ly).size() > 1)
      mvwprintw(w_look, 3, 1, "There are several items there.");
     else if (m.i_at(lx, ly).size() == 1)
      mvwprintw(w_look, 3, 1, "There is an item there.");
   } else if (veh) {
     mvwprintw(w_look, 3, 1, "There is a %s there. Parts:", veh->name.c_str());
     veh->print_part_desc(w_look, 4, 48, veh_part);
     m.drawsq(w_terrain, u, lx, ly, true, true, lx, ly);
   } else if (!m.has_flag(container, lx, ly) && m.i_at(lx, ly).size() > 0) {
    mvwprintw(w_look, 3, 1, "There is a %s there.",
              m.i_at(lx, ly)[0].tname(this).c_str());
    if (m.i_at(lx, ly).size() > 1)
     mvwprintw(w_look, 4, 1, "There are other items there as well.");
    m.drawsq(w_terrain, u, lx, ly, true, true, lx, ly);
   } else
    m.drawsq(w_terrain, u, lx, ly, true, true, lx, ly);

  } else if (lx == u.posx && ly == u.posy) {
   mvwputch_inv(w_terrain, VIEWX, VIEWY, u.color(), '@');
   mvwprintw(w_look, 1, 1, "You (%s)", u.name.c_str());
   if (veh) {
    mvwprintw(w_look, 3, 1, "There is a %s there. Parts:", veh->name.c_str());
    veh->print_part_desc(w_look, 4, 48, veh_part);
    m.drawsq(w_terrain, u, lx, ly, true, true, lx, ly);
   }
  } else if (u.sight_impaired() &&
              lm.at(lx - u.posx, ly - u.posy) == LL_BRIGHT &&
              rl_dist(u.posx, u.posy, lx, ly) < u.unimpaired_range() &&
              m.sees(u.posx, u.posy, lx, ly, u.unimpaired_range(), junk)) {
   if (u.has_disease(DI_BOOMERED))
    mvwputch_inv(w_terrain, ly - u.posy + VIEWY, lx - u.posx + VIEWX, c_pink, '#');
   else
    mvwputch_inv(w_terrain, ly - u.posy + VIEWY, lx - u.posx + VIEWX, c_ltgray, '#');
   mvwprintw(w_look, 1, 1, "Bright light.");
  } else {
   mvwputch(w_terrain, VIEWY, VIEWX, c_white, 'x');
   mvwprintw(w_look, 1, 1, "Unseen.");
  }

  if (m.graffiti_at(lx, ly).contents)
  {
   mvwprintw(w_look, 6, 1, "Graffiti: %s", m.graffiti_at(lx, ly).contents->c_str());
   wborder(w_look, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
  			LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
  }

  wrefresh(w_look);
  wrefresh(w_terrain);
 } while (input != Close && input != Cancel && input != Confirm);


//CAT-mgs:
 if(input == Confirm)
	return point(lx, ly);
 else
 {
	u.view_offset_x= 0;
	u.view_offset_y= 0;
 }

//CAT-s: close menu
 playSound(2);

 return point(-1, -1);
}

bool game::list_items_match(std::string sText, std::string sPattern)
{
 unsigned long iPos;

 do {
  iPos = sPattern.find(",");

  if (sText.find((iPos == std::string::npos) ? sPattern : sPattern.substr(0, iPos)) != std::string::npos)
   return true;

  if (iPos != std::string::npos)
   sPattern = sPattern.substr(iPos+1, sPattern.size());

 } while(iPos != std::string::npos);

 return false;
}

void game::list_items()
{
 int iInfoHeight = 12;
 WINDOW* w_items = newwin(TERMY-iInfoHeight-VIEW_OFFSET_Y*2, 55, VIEW_OFFSET_Y, TERRAIN_WINDOW_WIDTH + VIEW_OFFSET_X);
 WINDOW* w_item_info = newwin(iInfoHeight-1, 53, TERMY-iInfoHeight-VIEW_OFFSET_Y, TERRAIN_WINDOW_WIDTH+1+VIEW_OFFSET_X);
 WINDOW* w_item_info_border = newwin(iInfoHeight, 55, TERMY-iInfoHeight-VIEW_OFFSET_Y, TERRAIN_WINDOW_WIDTH+VIEW_OFFSET_X);

 std::vector <item> here;
 std::map<int, std::map<int, std::map<std::string, int> > > grounditems;
 std::map<std::string, item> iteminfo;

 //Area to search +- of players position. TODO: Use Perception
 const int iSearchX = 12 + ((VIEWX > 12) ? ((VIEWX-12)/2) : 0);
 const int iSearchY = 12 + ((VIEWY > 12) ? ((VIEWY-12)/2) : 0);
 int iItemNum = 0;

 int iTile;
 for (int iRow = (iSearchY * -1); iRow <= iSearchY; iRow++) {
  for (int iCol = (iSearchX * -1); iCol <= iSearchX; iCol++) {
    if (!m.has_flag(container, u.posx + iCol, u.posy + iRow) &&
       u_see(u.posx + iCol, u.posy + iRow, iTile)) {
    here.clear();
    here = m.i_at(u.posx + iCol, u.posy + iRow);

    for (int i = 0; i < here.size(); i++) {
     grounditems[iCol][iRow][here[i].tname(this)]++;
     if (grounditems[iCol][iRow][here[i].tname(this)] == 1) {
      iteminfo[here[i].tname(this)] = here[i];
      iItemNum++;
     }
    }
   }
  }
 }

 const int iStoreViewOffsetX = u.view_offset_x;
 const int iStoreViewOffsetY = u.view_offset_y;

 int iActive = 0;
 const int iMaxRows = TERMY-iInfoHeight-2-VIEW_OFFSET_Y*2;
 int iStartPos = 0;
 int iActiveX = 0;
 int iActiveY = 0;
 int iLastActiveX = -1;
 int iLastActiveY = -1;
 std::vector<point> vPoint;
 InputEvent input = Undefined;
 long ch = '.';
 int iFilter = 0;
 bool bStopDrawing = false;

 do {
  if (iItemNum > 0) {
   u.view_offset_x = 0;
   u.view_offset_y = 0;

   if (ch == 'I' || ch == 'c' || ch == 'C') {
    compare(iActiveX, iActiveY);
    ch = '.';
    refresh_all();

   } else if (ch == 'f' || ch == 'F') {
    for (int i = 0; i < iInfoHeight-1; i++)
     mvwprintz(w_item_info, i, 1, c_black, "%s", "                                                     ");

    mvwprintz(w_item_info, 2, 2, c_white, "%s", "How to use the filter:");
    mvwprintz(w_item_info, 3, 2, c_white, "%s", "Example: pi  will match any itemname with pi in it.");
    mvwprintz(w_item_info, 5, 2, c_white, "%s", "Seperate multiple items with ,");
    mvwprintz(w_item_info, 6, 2, c_white, "%s", "Example: back,flash,aid, ,band");
    mvwprintz(w_item_info, 8, 2, c_white, "%s", "To exclude certain items, place a - in front");
    mvwprintz(w_item_info, 9, 2, c_white, "%s", "Example: -pipe,chunk,steel");
    wrefresh(w_item_info);

    sFilter = string_input_popup("Filter:", 55, sFilter);
    iActive = 0;
    iLastActiveX = -1;
    iLastActiveY = -1;
    ch = '.';

   } else if (ch == 'r' || ch == 'R') {
    sFilter = "";
    iLastActiveX = -1;
    iLastActiveY = -1;
    ch = '.';
   }

   if (ch == '.') {
    for (int i = 1; i < TERMX; i++) {
     if (i < 55) {
      mvwputch(w_items, 0, i, c_ltgray, LINE_OXOX); // -
      mvwputch(w_items, TERMY-iInfoHeight-1-VIEW_OFFSET_Y*2, i, c_ltgray, LINE_OXOX); // -
     }

     if (i < TERMY-iInfoHeight-VIEW_OFFSET_Y*2) {
      mvwputch(w_items, i, 0, c_ltgray, LINE_XOXO); // |
      mvwputch(w_items, i, 54, c_ltgray, LINE_XOXO); // |
     }
    }

    mvwputch(w_items, 0,  0, c_ltgray, LINE_OXXO); // |^
    mvwputch(w_items, 0, 54, c_ltgray, LINE_OOXX); // ^|

    mvwputch(w_items, TERMY-iInfoHeight-1-VIEW_OFFSET_Y*2,  0, c_ltgray, LINE_XXXO); // |-
    mvwputch(w_items, TERMY-iInfoHeight-1-VIEW_OFFSET_Y*2, 54, c_ltgray, LINE_XOXX); // -|

    int iTempStart = 19;
    if (sFilter != "") {
     iTempStart = 15;
     mvwprintz(w_items, TERMY-iInfoHeight-1-VIEW_OFFSET_Y*2, iTempStart + 19, c_ltgreen, " %s", "R");
     wprintz(w_items, c_white, "%s", "eset ");
    }

    mvwprintz(w_items, TERMY-iInfoHeight-1-VIEW_OFFSET_Y*2, iTempStart, c_ltgreen, " %s", "C");
    wprintz(w_items, c_white, "%s", "ompare ");

    mvwprintz(w_items, TERMY-iInfoHeight-1-VIEW_OFFSET_Y*2, iTempStart + 10, c_ltgreen, " %s", "F");
    wprintz(w_items, c_white, "%s", "ilter ");

    refresh_all();
   }

   bStopDrawing = false;

   switch(input) {
    case DirectionN:
     iActive--;
     if (iActive < 0) {
      iActive = 0;
      bStopDrawing = true;
     }
     break;
    case DirectionS:
     iActive++;
     if (iActive >= iItemNum - iFilter) {
      iActive = iItemNum - iFilter-1;
      bStopDrawing = true;
     }
     break;
   }

   if (!bStopDrawing) {
    if (iItemNum - iFilter > iMaxRows) {
     iStartPos = iActive - (iMaxRows - 1) / 2;

     if (iStartPos < 0)
      iStartPos = 0;
     else if (iStartPos + iMaxRows > iItemNum - iFilter)
      iStartPos = iItemNum - iFilter - iMaxRows;
    }

    for (int i = 0; i < iMaxRows; i++)
     mvwprintz(w_items, 1 + i, 1, c_black, "%s", "                                                     ");

    //TODO: Speed this up, first attemp to do so failed
    int iNum = 0;
    iFilter = 0;
    iActiveX = 0;
    iActiveY = 0;
    std::string sActiveItemName;
    std::stringstream sText;
    std::string sFilterPre = "";
    std::string sFilterTemp = sFilter;
    if (sFilterTemp != "" && sFilter.substr(0, 1) == "-") {
     sFilterPre = "-";
     sFilterTemp = sFilterTemp.substr(1, sFilterTemp.size()-1);
    }

    for (int iRow = (iSearchY * -1); iRow <= iSearchY; iRow++) {
     for (int iCol = (iSearchX * -1); iCol <= iSearchX; iCol++) {
      for (std::map< std::string, int>::iterator iter=grounditems[iCol][iRow].begin(); iter!=grounditems[iCol][iRow].end(); ++iter) {
       if (sFilterTemp == "" || (sFilterTemp != "" && ((sFilterPre != "-" && list_items_match(iter->first, sFilterTemp)) ||
                                                       (sFilterPre == "-" && !list_items_match(iter->first, sFilterTemp))))) {
        if (iNum >= iStartPos && iNum < iStartPos + ((iMaxRows > iItemNum) ? iItemNum : iMaxRows) ) {
         if (iNum == iActive) {
          iActiveX = iCol;
          iActiveY = iRow;
          sActiveItemName = iter->first;
         }
         sText.str("");
         sText << iter->first;
         if (iter->second > 1)
          sText << " " << "[" << iter->second << "]";
         mvwprintz(w_items, 1 + iNum - iStartPos, 2, ((iNum == iActive) ? c_ltgreen : c_white), "%s", (sText.str()).c_str());
         mvwprintz(w_items, 1 + iNum - iStartPos, 48, ((iNum == iActive) ? c_ltgreen : c_ltgray), "%*d %s",
                   ((iItemNum > 9) ? 2 : 1),
                   trig_dist(0, 0, iCol, iRow),
                   direction_name_short(direction_from(0, 0, iCol, iRow)).c_str()
                  );
        }

        iNum++;
       } else {
        iFilter++;
       }
      }
     }
    }

    mvwprintz(w_items, 0, 23 + ((iItemNum - iFilter > 9) ? 0 : 1), c_ltgreen, " %*d", ((iItemNum - iFilter > 9) ? 2 : 1), iActive+1);
    wprintz(w_items, c_white, " / %*d ", ((iItemNum - iFilter > 9) ? 2 : 1), iItemNum - iFilter);

    werase(w_item_info);
    mvwprintz(w_item_info, 0, 0, c_white, "%s", iteminfo[sActiveItemName].info().c_str());

    for (int j=0; j < iInfoHeight-1; j++)
     mvwputch(w_item_info_border, j, 0, c_ltgray, LINE_XOXO);

    for (int j=0; j < iInfoHeight-1; j++)
     mvwputch(w_item_info_border, j, 54, c_ltgray, LINE_XOXO);

    for (int j=0; j < 54; j++)
     mvwputch(w_item_info_border, iInfoHeight-1, j, c_ltgray, LINE_OXOX);

    mvwputch(w_item_info_border, iInfoHeight-1, 0, c_ltgray, LINE_XXOO);
    mvwputch(w_item_info_border, iInfoHeight-1, 54, c_ltgray, LINE_XOOX);

    //Only redraw trail/terrain if x/y position changed
    if (iActiveX != iLastActiveX || iActiveY != iLastActiveY) {
     iLastActiveX = iActiveX;
     iLastActiveY = iActiveY;

     //Remove previous trail
     for (int i = 0; i < vPoint.size(); i++) {
      m.drawsq(w_terrain, u, vPoint[i].x, vPoint[i].y, false, true);
     }

     //Draw new trail
     vPoint = line_to(u.posx, u.posy, u.posx + iActiveX, u.posy + iActiveY, 0);
     for (int i = 1; i < vPoint.size(); i++) {
       m.drawsq(w_terrain, u, vPoint[i-1].x, vPoint[i-1].y, true, true);
     }

     mvwputch(w_terrain, vPoint[vPoint.size()-1].y + VIEWY - u.posy - u.view_offset_y,
                         vPoint[vPoint.size()-1].x + VIEWX - u.posx - u.view_offset_x, c_white, 'X');

     wrefresh(w_terrain);
    }

    wrefresh(w_items);
    wrefresh(w_item_info_border);
    wrefresh(w_item_info);
   }

   refresh();
   ch = getch();
   input = get_input(ch);
  } else {
   add_msg("You dont see any items around you!");
   ch = ' ';
   input = Close;
  }
 } while (input != Close && input != Cancel);

 u.view_offset_x = iStoreViewOffsetX;
 u.view_offset_y = iStoreViewOffsetY;

 erase();
 refresh_all();
}

// Pick up items at (posx, posy).
void game::pickup(int posx, int posy, int min)
{
 item_exchanges_since_save += 1; // Keeping this simple.
 write_msg();
 if (u.weapon.type->id == itm_bio_claws) {
  add_msg("You cannot pick up items with your claws out!");
  return;
 }
 bool weight_is_okay = (u.weight_carried() <= u.weight_capacity() * .25);
 bool volume_is_okay = (u.volume_carried() <= u.volume_capacity() -  2);
 bool from_veh = false;
 int veh_part = 0;
 vehicle *veh = m.veh_at (posx, posy, veh_part);
 if (veh) {
  veh_part = veh->part_with_feature(veh_part, vpf_cargo, false);
  from_veh = veh && veh_part >= 0 &&
             veh->parts[veh_part].items.size() > 0 &&
             query_yn("Get items from %s?", veh->part_info(veh_part).name);
 }
// Picking up water?
 if ((!from_veh) && m.i_at(posx, posy).size() == 0) {
  if (m.has_flag(swimmable, posx, posy) || m.ter(posx, posy) == t_toilet || m.ter(posx, posy) == t_water_sh) {
   item water = m.water_from(posx, posy);
    // Try to handle first (bottling) drink after.
    // changed boolean, large sources should be infinite
   if (handle_liquid(water, true, true)) {
    u.moves -= 100;
   } else if (query_yn("Drink from your hands?")) {
    u.inv.push_back(water);
    u.eat(this, u.inv.size() - 1);
    u.moves -= 350;
   }
  }
  return;
// Few item here, just get it
 } else if ((from_veh ? veh->parts[veh_part].items.size() :
                        m.i_at(posx, posy).size()          ) <= min) {
  int iter = 0;
  item newit = from_veh ? veh->parts[veh_part].items[0] : m.i_at(posx, posy)[0];
  if (newit.made_of(LIQUID)) {
   add_msg("You can't pick up a liquid!");
   return;
  }
  if (newit.invlet == 0) {
   newit.invlet = nextinv;
   advance_nextinv();
  }
  while (iter < 52 && u.has_item(newit.invlet) &&
         !u.i_at(newit.invlet).stacks_with(newit)) {
   newit.invlet = nextinv;
   iter++;
   advance_nextinv();
  }
  if (iter == 52) {
   add_msg("You're carrying too many items!");
   return;
  } else if (u.weight_carried() + newit.weight() > u.weight_capacity()) {
   add_msg("The %s is too heavy!", newit.tname(this).c_str());
   decrease_nextinv();
  } else if (u.volume_carried() + newit.volume() > u.volume_capacity()) {
   if (u.is_armed()) {
    if (!u.weapon.has_flag(IF_NO_UNWIELD)) {
     if (newit.is_armor() && // Armor can be instantly worn
         query_yn("Put on the %s?", newit.tname(this).c_str())) {
      if(u.wear_item(this, &newit)){
       if (from_veh)
        veh->remove_item (veh_part, 0);
       else
        m.i_clear(posx, posy);
      }
     } else if (query_yn("Drop your %s and pick up %s?",
                u.weapon.tname(this).c_str(), newit.tname(this).c_str())) {
      if (from_veh)
       veh->remove_item (veh_part, 0);
      else
       m.i_clear(posx, posy);
      m.add_item(posx, posy, u.remove_weapon());
      u.i_add(newit, this);
      u.wield(this, u.inv.size() - 1);
      u.moves -= 100;
      add_msg("Wielding %c - %s", newit.invlet, newit.tname(this).c_str());
     } else
      decrease_nextinv();
    } else {
     add_msg("There's no room in your inventory for the %s, and you can't\
 unwield your %s.", newit.tname(this).c_str(), u.weapon.tname(this).c_str());
     decrease_nextinv();
    }
   } else {
    u.i_add(newit, this);
    u.wield(this, u.inv.size() - 1);
    if (from_veh)
     veh->remove_item (veh_part, 0);
    else
     m.i_clear(posx, posy);
    u.moves -= 100;
    add_msg("Wielding %c - %s", newit.invlet, newit.tname(this).c_str());
   }
  } else if (!u.is_armed() &&
             (u.volume_carried() + newit.volume() > u.volume_capacity() - 2 ||
              newit.is_weap() || newit.is_gun())) {
   u.weapon = newit;
   if (from_veh)
    veh->remove_item (veh_part, 0);
   else
    m.i_clear(posx, posy);
   u.moves -= 100;
   add_msg("Wielding %c - %s", newit.invlet, newit.tname(this).c_str());
  } else {
   u.i_add(newit, this);
   if (from_veh)
    veh->remove_item (veh_part, 0);
   else
    m.i_clear(posx, posy);
   u.moves -= 100;
   add_msg("%c - %s", newit.invlet, newit.tname(this).c_str());
  }
  if (weight_is_okay && u.weight_carried() >= u.weight_capacity() * .25)
   add_msg("You're overburdened!");
  if (volume_is_okay && u.volume_carried() > u.volume_capacity() - 2) {
   add_msg("You struggle to carry such a large volume!");
  }

//CAT-s:
  playSound(4);

  return;
 }
// Otherwise, we have 2 or more items and should list them, etc.
 WINDOW* w_pickup = newwin(12, 48, VIEW_OFFSET_Y, VIEWX * 2 + 8 + VIEW_OFFSET_X);
 WINDOW* w_item_info = newwin(12, 48, 12 + VIEW_OFFSET_Y, VIEWX * 2 + 8 + VIEW_OFFSET_X);
 int maxitems = 9;	 // Number of items to show at one time.
 std::vector <item> here = from_veh? veh->parts[veh_part].items : m.i_at(posx, posy);
 bool getitem[here.size()];
 for (int i = 0; i < here.size(); i++)
  getitem[i] = false;
 char ch = ' ';
 int start = 0, cur_it, iter;
 int new_weight = u.weight_carried(), new_volume = u.volume_carried();
 bool update = true;
 mvwprintw(w_pickup, 0,  0, "PICK UP");
// Now print the two lists; those on the ground and about to be added to inv
// Continue until we hit return or space
 do {
  for (int i = 1; i < 12; i++) {
   for (int j = 0; j < 48; j++)
    mvwaddch(w_pickup, i, j, ' ');
  }
  if (ch == '<' && start > 0) {
   start -= maxitems;
   mvwprintw(w_pickup, maxitems + 2, 0, "         ");
  }
  if (ch == '>' && start + maxitems < here.size()) {
   start += maxitems;
   mvwprintw(w_pickup, maxitems + 2, 12, "            ");
  }
  if (ch >= 'a' && ch <= 'a' + here.size() - 1) {
   ch -= 'a';
   getitem[ch] = !getitem[ch];
   wclear(w_item_info);
   if (getitem[ch]) {
    mvwprintw(w_item_info, 1, 0, here[ch].info().c_str());
    wborder(w_item_info, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                         LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
    wrefresh(w_item_info);
    new_weight += here[ch].weight();
    new_volume += here[ch].volume();
    update = true;
   } else {
    wborder(w_item_info, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                         LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
    wrefresh(w_item_info);
    new_weight -= here[ch].weight();
    new_volume -= here[ch].volume();
    update = true;
   }
  }
  if (ch == ',') {
   int count = 0;
   for (int i = 0; i < here.size(); i++) {
    if (getitem[i])
     count++;
    else {
     new_weight += here[i].weight();
     new_volume += here[i].volume();
    }
    getitem[i] = true;
   }
   if (count == here.size()) {
    for (int i = 0; i < here.size(); i++)
     getitem[i] = false;
    new_weight = u.weight_carried();
    new_volume = u.volume_carried();
   }
   update = true;
  }
  for (cur_it = start; cur_it < start + maxitems; cur_it++) {
   mvwprintw(w_pickup, 1 + (cur_it % maxitems), 0,
             "                                        ");
   if (cur_it < here.size()) {
    mvwputch(w_pickup, 1 + (cur_it % maxitems), 0, here[cur_it].color(&u),
             char(cur_it + 'a'));
    if (getitem[cur_it])
     wprintw(w_pickup, " + ");
    else
     wprintw(w_pickup, " - ");
    wprintz(w_pickup, here[cur_it].color(&u), here[cur_it].tname(this).c_str());
    if (here[cur_it].charges > 0)
     wprintz(w_pickup, here[cur_it].color(&u), " (%d)", here[cur_it].charges);
   }
  }
  if (start > 0)
   mvwprintw(w_pickup, maxitems + 2, 0, "< Go Back");
  if (cur_it < here.size())
   mvwprintw(w_pickup, maxitems + 2, 12, "> More items");
  if (update) {		// Update weight & volume information
   update = false;
   mvwprintw(w_pickup, 0,  7, "                           ");
   mvwprintz(w_pickup, 0,  9,
             (new_weight >= u.weight_capacity() * .25 ? c_red : c_white),
             "Wgt %d", new_weight);
   wprintz(w_pickup, c_white, "/%d", int(u.weight_capacity() * .25));
   mvwprintz(w_pickup, 0, 22,
             (new_volume > u.volume_capacity() - 2 ? c_red : c_white),
             "Vol %d", new_volume);
   wprintz(w_pickup, c_white, "/%d", u.volume_capacity() - 2);
  }
  wrefresh(w_pickup);
  ch = getch();
 } while (ch != ' ' && ch != '\n' && ch != KEY_ESCAPE);
 if (ch != '\n') {
  werase(w_pickup);
  wrefresh(w_pickup);
  delwin(w_pickup);
  delwin(w_item_info);
  add_msg("Never mind.");
  return;
 }
// At this point we've selected our items, now we add them to our inventory
 int curmit = 0;
 bool got_water = false;	// Did we try to pick up water?
 for (int i = 0; i < here.size(); i++) {
  iter = 0;
// This while loop guarantees the inventory letter won't be a repeat. If it
// tries all 52 letters, it fails and we don't pick it up.
  if (getitem[i] && here[i].made_of(LIQUID))
   got_water = true;
  else if (getitem[i]) {
   iter = 0;
   while (iter < 52 && (here[i].invlet == 0 ||
                        (u.has_item(here[i].invlet) &&
                         !u.i_at(here[i].invlet).stacks_with(here[i]))) ) {
    here[i].invlet = nextinv;
    iter++;
    advance_nextinv();
   }
   if (iter == 52) {
    add_msg("You're carrying too many items!");
    werase(w_pickup);
    wrefresh(w_pickup);
    delwin(w_pickup);
    return;
   } else if (u.weight_carried() + here[i].weight() > u.weight_capacity()) {
    add_msg("The %s is too heavy!", here[i].tname(this).c_str());
    decrease_nextinv();
   } else if (u.volume_carried() + here[i].volume() > u.volume_capacity()) {
    if (u.is_armed()) {
     if (!u.weapon.has_flag(IF_NO_UNWIELD)) {
      if (here[i].is_armor() && // Armor can be instantly worn
          query_yn("Put on the %s?", here[i].tname(this).c_str())) {
       if(u.wear_item(this, &(here[i])))
       {
        if (from_veh)
         veh->remove_item (veh_part, curmit);
        else
         m.i_rem(posx, posy, curmit);
        curmit--;
       }
      } else if (query_yn("Drop your %s and pick up %s?",
                u.weapon.tname(this).c_str(), here[i].tname(this).c_str())) {
       if (from_veh)
        veh->remove_item (veh_part, curmit);
       else
        m.i_rem(posx, posy, curmit);
       m.add_item(posx, posy, u.remove_weapon());
       u.i_add(here[i], this);
       u.wield(this, u.inv.size() - 1);
       curmit--;
       u.moves -= 100;
       add_msg("Wielding %c - %s", u.weapon.invlet, u.weapon.tname(this).c_str());
      } else
       decrease_nextinv();
     } else {
      add_msg("There's no room in your inventory for the %s, and you can't\
  unwield your %s.", here[i].tname(this).c_str(), u.weapon.tname(this).c_str());
      decrease_nextinv();
     }
    } else {
     u.i_add(here[i], this);
     u.wield(this, u.inv.size() - 1);
     if (from_veh)
      veh->remove_item (veh_part, curmit);
     else
      m.i_rem(posx, posy, curmit);
     curmit--;
     u.moves -= 100;
    }
   } else if (!u.is_armed() &&
            (u.volume_carried() + here[i].volume() > u.volume_capacity() - 2 ||
              here[i].is_weap() || here[i].is_gun())) {
    u.weapon = here[i];
    if (from_veh)
     veh->remove_item (veh_part, curmit);
    else
     m.i_rem(posx, posy, curmit);
    u.moves -= 100;
    curmit--;
   } else {
    u.i_add(here[i], this);
    if (from_veh)
     veh->remove_item (veh_part, curmit);
    else
     m.i_rem(posx, posy, curmit);
    u.moves -= 100;
    curmit--;
   }

//CAT-s:
  playSound(4);

  }
  curmit++;
 }
 if (got_water)
  add_msg("You can't pick up a liquid!");
 if (weight_is_okay && u.weight_carried() >= u.weight_capacity() * .25)
  add_msg("You're overburdened!");
 if (volume_is_okay && u.volume_carried() > u.volume_capacity() - 2) {
  add_msg("You struggle to carry such a large volume!");

//CAT-s:
//  playSound(4);

 }
 werase(w_pickup);
 wrefresh(w_pickup);
 delwin(w_pickup);
 delwin(w_item_info);
}

// Handle_liquid returns false if we didn't handle all the liquid.
bool game::handle_liquid(item &liquid, bool from_ground, bool infinite)
{
 if (!liquid.made_of(LIQUID)) {
/*
  dbg(D_ERROR) << "game:handle_liquid: Tried to handle_liquid a non-liquid!";
  debugmsg("Tried to handle_liquid a non-liquid!");
*/
  return false;
 }
 if (liquid.type->id == itm_gasoline && vehicle_near() && query_yn("Refill vehicle?")) {
  int vx = u.posx, vy = u.posy;
  if (pl_choose_vehicle(vx, vy)) {
   vehicle *veh = m.veh_at (vx, vy);
   if (veh) {
    int ftype = AT_GAS;
    int fuel_cap = veh->fuel_capacity(ftype);
    int fuel_amnt = veh->fuel_left(ftype);
    if (fuel_cap < 1)
     add_msg ("This vehicle doesn't use %s.", veh->fuel_name(ftype).c_str());
    else if (fuel_amnt == fuel_cap)
     add_msg ("Already full.");
    else if (infinite && query_yn("Pump until full?")) {
     u.assign_activity(this, ACT_REFILL_VEHICLE, 2 * (fuel_cap - fuel_amnt));
     u.activity.placement = point(vx, vy);
    } else { // Not infinite
     veh->refill (AT_GAS, liquid.charges);
     add_msg ("You refill %s with %s%s.", veh->name.c_str(),
              veh->fuel_name(ftype).c_str(),
              veh->fuel_left(ftype) >= fuel_cap? " to its maximum" : "");
     u.moves -= 100;
     return true;
    }
   } else // if (veh)
    add_msg ("There isn't any vehicle there.");
   return false;
  } // if (pl_choose_vehicle(vx, vy))

 } else { // Not filling vehicle

   // Ask to pour rotten liquid (milk!) from the get-go
  if (!from_ground && liquid.rotten(this) &&
      query_yn("Pour %s on the ground?", liquid.tname(this).c_str())) {
   m.add_item(u.posx, u.posy, liquid);
   return true;
  }

  std::stringstream text;
  text << "Container for " << liquid.tname(this);
  char ch = inv_type(text.str().c_str(), IC_CONTAINER);
  if (!u.has_item(ch)) {
    // No container selected (escaped, ...), ask to pour
    // we asked to pour rotten already
   if (!from_ground && !liquid.rotten(this) &&
       query_yn("Pour %s on the ground?", liquid.tname(this).c_str())) {
    m.add_item(u.posx, u.posy, liquid);

//CAT-s: splash sound?
  playSound(27);

    return true;
   }
   return false;
  }

  item *cont = &(u.i_at(ch));
  if (cont == NULL || cont->is_null()) {
    // Container is null, ask to pour.
    // we asked to pour rotten already
   if (!from_ground && !liquid.rotten(this) &&
       query_yn("Pour %s on the ground?", liquid.tname(this).c_str())) {
    m.add_item(u.posx, u.posy, liquid);
    return true;
   }
   add_msg("Never mind.");
   return false;

  } else if (liquid.is_ammo() && (cont->is_tool() || cont->is_gun())) {
// for filling up chainsaws, jackhammers and flamethrowers
   ammotype ammo = AT_NULL;
   int max = 0;

   if (cont->is_tool()) {
    it_tool* tool = dynamic_cast<it_tool*>(cont->type);
    ammo = tool->ammo;
    max = tool->max_charges;
   } else {
    it_gun* gun = dynamic_cast<it_gun*>(cont->type);
    ammo = gun->ammo;
    max = gun->clip;
   }

   ammotype liquid_type = liquid.ammo_type();

   if (ammo != liquid_type) {
    add_msg("Your %s won't hold %s.", cont->tname(this).c_str(),
                                      liquid.tname(this).c_str());
    return false;
   }

   if (max <= 0 || cont->charges >= max) {
    add_msg("Your %s can't hold any more %s.", cont->tname(this).c_str(),
                                               liquid.tname(this).c_str());
    return false;
   }

   if (cont->charges > 0 && cont->curammo->id != liquid.type->id) {
    add_msg("You can't mix loads in your %s.", cont->tname(this).c_str());
    return false;
   }

   add_msg("You pour %s into your %s.", liquid.tname(this).c_str(),
                                        cont->tname(this).c_str());
   cont->curammo = dynamic_cast<it_ammo*>(liquid.type);
   if (infinite)
    cont->charges = max;
   else {
    cont->charges += liquid.charges;
    if (cont->charges > max) {
     int extra = cont->charges - max;
     cont->charges = max;
     liquid.charges = extra;
     add_msg("There's some left over!");
     return false;
    }
   }
   return true;

  } else if (!cont->is_container()) {
   add_msg("That %s won't hold %s.", cont->tname(this).c_str(),
                                     liquid.tname(this).c_str());
   return false;
  } else        // filling up normal containers
    {
      // first, check if liquid types are compatible
      if (!cont->contents.empty())
      {
        if  (cont->contents[0].type->id != liquid.type->id)
        {
          add_msg("You can't mix loads in your %s.", cont->tname(this).c_str());
          return false;
        }
      }

      // ok, liquids are compatible.  Now check what the type of liquid is
      // this will determine how much the holding container can hold

      it_container* container = dynamic_cast<it_container*>(cont->type);
      int holding_container_charges;

      if (liquid.type->is_food())
      {
        it_comest* tmp_comest = dynamic_cast<it_comest*>(liquid.type);
        holding_container_charges = container->contains * tmp_comest->charges;
      }
      else if (liquid.type->is_ammo())
      {
        it_ammo* tmp_ammo = dynamic_cast<it_ammo*>(liquid.type);
        holding_container_charges = container->contains * tmp_ammo->count;
      }
      else
        holding_container_charges = container->contains;

      // if the holding container is NOT empty
      if (!cont->contents.empty())
      {
        // case 1: container is completely full
        if (cont->contents[0].charges == holding_container_charges)
        {
          add_msg("Your %s can't hold any more %s.", cont->tname(this).c_str(),
                                                   liquid.tname(this).c_str());
          return false;
        }

        // case 2: container is half full

        if (infinite)
        {
          cont->contents[0].charges = holding_container_charges;
          add_msg("You pour %s into your %s.", liquid.tname(this).c_str(),
                                        cont->tname(this).c_str());
        }
        else // Container is finite, not empty and not full, add liquid to it
        {
          add_msg("You pour %s into your %s.", liquid.tname(this).c_str(),
                    cont->tname(this).c_str());
          cont->contents[0].charges += liquid.charges;
          if (cont->contents[0].charges > holding_container_charges)
          {
            int extra = cont->contents[0].charges - holding_container_charges;
            cont->contents[0].charges = holding_container_charges;
            liquid.charges = extra;
            add_msg("There's some left over!");
            // Why not try to find another container here?
            return false;
          }
          return true;
        }
      }
      else  // pouring into an empty container
      {
        if (!(container->flags & mfb(con_wtight)))  // invalid container types
        {
          add_msg("That %s isn't water-tight.", cont->tname(this).c_str());
          return false;
        }
        else if (!(container->flags & mfb(con_seals)))
        {
          add_msg("You can't seal that %s!", cont->tname(this).c_str());
          return false;
        }
        // pouring into a valid empty container
        int default_charges = 1;

        if (liquid.is_food())
        {
          it_comest* comest = dynamic_cast<it_comest*>(liquid.type);
          default_charges = comest->charges;
        }
        else if (liquid.is_ammo())
        {
          it_ammo* ammo = dynamic_cast<it_ammo*>(liquid.type);
          default_charges = ammo->count;
        }

        if (infinite) // if filling from infinite source, top it to max
          liquid.charges = container->contains * default_charges;
        else if (liquid.charges > container->contains * default_charges)
        {
          add_msg("You fill your %s with some of the %s.", cont->tname(this).c_str(),
                                                    liquid.tname(this).c_str());
          u.inv_sorted = false;
          int oldcharges = liquid.charges - container->contains * default_charges;
          liquid.charges = container->contains * default_charges;
          cont->put_in(liquid);
          liquid.charges = oldcharges;
          return false;
        }
        cont->put_in(liquid);
        return true;
      }
    }
    return false;
  }

//CAT-s:
  playSound(27);

 return true;
}

void game::drop(char chInput)
{
 std::vector<item> dropped;

 if (chInput == '.')
  dropped = multidrop();
 else {
  int index = u.inv.index_by_letter(chInput);

  if (index == -1) {
   dropped.push_back(u.i_rem(chInput));
  } else {
   dropped.push_back(u.inv.remove_item(index));
  }
 }

 if (dropped.size() == 0) {
  add_msg("Never mind.");
  return;
 }

 item_exchanges_since_save += dropped.size();

 itype_id first = itype_id(dropped[0].type->id);
 bool same = true;
 for (int i = 1; i < dropped.size() && same; i++) {
  if (dropped[i].type->id != first)
   same = false;
 }

 int veh_part = 0;
 bool to_veh = false;
 vehicle *veh = m.veh_at(u.posx, u.posy, veh_part);
 if (veh) {
  veh_part = veh->part_with_feature (veh_part, vpf_cargo);
  to_veh = veh_part >= 0;
 }
 if (dropped.size() == 1 || same) {
  if (to_veh)
   add_msg("You put your %s%s in the %s's %s.", dropped[0].tname(this).c_str(),
          (dropped.size() == 1 ? "" : "s"), veh->name.c_str(),
          veh->part_info(veh_part).name);
  else
   add_msg("You drop your %s%s.", dropped[0].tname(this).c_str(),
          (dropped.size() == 1 ? "" : "s"));
 } else {
  if (to_veh)
   add_msg("You put several items in the %s's %s.", veh->name.c_str(),
           veh->part_info(veh_part).name);
  else
   add_msg("You drop several items.");
 }

//CAT-s:
  playSound(5);

 bool vh_overflow = false;
 int i = 0;
 if (to_veh) {
  for (i = 0; i < dropped.size(); i++)
   if (!veh->add_item (veh_part, dropped[i])) {
    vh_overflow = true;

	//CAT-s: actionDrop sound
	  playSound(5);

    break;
   }
  if (vh_overflow)
  {
	add_msg ("The trunk is full, so some items fall on the ground.");

	//CAT-s: actionDrop sound
	  playSound(5);
  }
 }
 if (!to_veh || vh_overflow)
  for (i = 0; i < dropped.size(); i++) {
    m.add_item(u.posx, u.posy, dropped[i]);
 }
}

void game::drop_in_direction()
{
//CAT-g:
// refresh_all();
// mvprintz(0, 0, c_red, "Choose a direction:");

//CAT-g: for different than default color use this
// mvwprintz(w_terrain, 0, 0, c_red, "Drop where?");

//CAT-g:
 mvwprintw(w_terrain, 0, 0, "Drop where?");
 wrefresh(w_terrain);
 playSound(1);


// DebugLog() << __FUNCTION__ << "calling get_input() \n";
 int dirx, diry;
 InputEvent input = get_input();
 get_direction(dirx, diry, input);
 if (dirx == -2) {
  add_msg("Invalid direction!");

//CAT-s: 
  playSound(3);

  return;
 }
 dirx += u.posx;
 diry += u.posy;
 int veh_part = 0;
 bool to_veh = false;
 vehicle *veh = m.veh_at(dirx, diry, veh_part);
 if (veh) {
  veh_part = veh->part_with_feature (veh_part, vpf_cargo);
  to_veh = veh->type != veh_null && veh_part >= 0;
 }

 if (m.has_flag(noitem, dirx, diry) || m.has_flag(sealed, dirx, diry)) {
  add_msg("You can't place items there!");
  return;
 }

 std::string verb = (m.move_cost(dirx, diry) == 0 ? "put" : "drop");
 std::string prep = (m.move_cost(dirx, diry) == 0 ? "in"  : "on"  );

 std::vector<item> dropped = multidrop();

 if (dropped.size() == 0) {
  add_msg("Never mind.");
  return;
 }

//CAT-s: 
  playSound(5);

 item_exchanges_since_save += dropped.size();

 itype_id first = itype_id(dropped[0].type->id);
 bool same = true;
 for (int i = 1; i < dropped.size() && same; i++) {
  if (dropped[i].type->id != first)
   same = false;

//CAT-s: 
	playSound(5);
 }

 if (dropped.size() == 1 || same)
 {
  if (to_veh)
   add_msg("You put your %s%s in the %s's %s.", dropped[0].tname(this).c_str(),
          (dropped.size() == 1 ? "" : "s"), veh->name.c_str(),
          veh->part_info(veh_part).name);
  else
   add_msg("You %s your %s%s %s the %s.", verb.c_str(),
           dropped[0].tname(this).c_str(),
           (dropped.size() == 1 ? "" : "s"), prep.c_str(),
           m.tername(dirx, diry).c_str());
 } else {
  if (to_veh)
   add_msg("You put several items in the %s's %s.", veh->name.c_str(),
           veh->part_info(veh_part).name);
  else
   add_msg("You %s several items %s the %s.", verb.c_str(), prep.c_str(),
           m.tername(dirx, diry).c_str());
 }
 if (to_veh) {
  bool vh_overflow = false;
  for (int i = 0; i < dropped.size(); i++) {
   vh_overflow = vh_overflow || !veh->add_item (veh_part, dropped[i]);
   if (vh_overflow)
    m.add_item(dirx, diry, dropped[i]);
  }
  if (vh_overflow)
   add_msg ("Trunk is full, so some items fall on the ground.");
 } else {
  for (int i = 0; i < dropped.size(); i++)
   m.add_item(dirx, diry, dropped[i]);
 }
}

void game::reassign_item()
{
 char ch = inv("Reassign item:");
 if (ch == ' ') {
  add_msg("Never mind.");
  return;
 }
 if (!u.has_item(ch)) {
  add_msg("You do not have that item.");
  return;
 }
 char newch = popup_getkey("%c - %s; enter new letter.", ch,
                           u.i_at(ch).tname().c_str());
 if ((newch < 'A' || (newch > 'Z' && newch < 'a') || newch > 'z')) {
  add_msg("%c is not a valid inventory letter.", newch);
  return;
 }
 item* change_from = &(u.i_at(ch));
 if (u.has_item(newch)) {
  item* change_to = &(u.i_at(newch));
  change_to->invlet = ch;
  add_msg("%c - %s", ch, change_to->tname().c_str());
 }
 change_from->invlet = newch;
 add_msg("%c - %s", newch, change_from->tname().c_str());
}

void game::plthrow(char chInput)
{
 char ch;

 if (chInput != '.') {
  ch = chInput;
 } else {
  ch = inv("Throw item:");
 }

 int range = u.throw_range(u.lookup_item(ch));
 if (range < 0) {
  add_msg("You don't have that item.");
  return;
 } else if (range == 0) {
  add_msg("That is too heavy to throw.");
  return;
 }


 item thrown = u.i_at(ch);
 if (thrown.type->id > num_items && thrown.type->id < num_all_items) {
  add_msg("That's part of your body, you can't throw that!");
  return;
 }


//CAT-mgs: 
/*
 // TODO: [lightmap] This appears to redraw the screen for throwing,
 //                  check were lightmap needs to be shown
 int sight_range = u.sight_range(light_level());
 if (range < sight_range)
  range = sight_range;
*/

 int x = u.posx, y = u.posy;
 int x0 = x - range;
 int y0 = y - range;
 int x1 = x + range;
 int y1 = y + range;
 int junk;

 for (int j = u.posx - VIEWX; j <= u.posx + VIEWX; j++) {
  for (int k = u.posy - VIEWY; k <= u.posy + VIEWY; k++) {
   if (u_see(j, k, junk)) {
    if (k >= y0 && k <= y1 && j >= x0 && j <= x1)
     m.drawsq(w_terrain, u, j, k, false, true);
    else
     mvwputch(w_terrain, k + VIEWY - u.posy - u.view_offset_y,
                         j + VIEWX - u.posx - u.view_offset_x, c_dkgray, '#');
   }
  }
 }

//CAT-g:
 bool night_vision= false;
 if( (u.is_wearing(itm_goggles_nv) && u.has_active_item(itm_UPS_on)) 
	|| u.has_active_bionic(bio_night_vision) )
    night_vision= true;

 int passtarget = -1;

/*
 std::vector <monster> mon_targets;
 std::vector <int> targetindices;
 for (int i = 0; i < z.size(); i++) {
  if (u_see(&(z[i]), junk) && z[i].posx >= x0 && z[i].posx <= x1 &&
                              z[i].posy >= y0 && z[i].posy <= y1) {
   mon_targets.push_back(z[i]);
   targetindices.push_back(i);
   if (i == last_target)
    passtarget = mon_targets.size() - 1;

   z[i].draw(w_terrain, u.posx, u.posy, true, night_vision);
  }
 }
*/

 // target() sets x and y, or returns false if we canceled (by pressing Esc)
 std::vector <point> trajectory = target(x, y, x0, y0, x1, y1, &thrown);
 if(trajectory.size() < 1)
  return;


 u.i_rem(ch);
 u.moves -= 125;
 u.practice("throw", 10);

 throw_item(u, x, y, thrown, trajectory);
}


//CAT-mgs: *** vvv
void game::plfire(bool burst)
{
 ltar_x= u.posx;
 ltar_y= u.posy;

 if(u.recoil < 30)
	u.recoil= 30;

//CAT-s:
 playSound(0);

 do
 {
	 SNIPER= false;
	 int reload_index = -1;
	 if (!u.weapon.is_gun())
	  return;

	 vehicle *veh = m.veh_at(u.posx, u.posy);
	 if (veh && veh->player_in_control(&u) && u.weapon.is_two_handed(&u)) {
	  add_msg ("You need a free arm to drive!");

//CAT-s: menuWrong sound
		playSound(3);

	  return;
	 }


//CAT-mgs: NX-17 charge rifle
//... charge only with UPS turned ON
	if (u.weapon.has_flag(IF_CHARGE) && !u.weapon.active)
	{
		 if (u.has_charges(itm_UPS_on, 1)) // || u.has_charges(itm_UPS_off, 1)
		 {
			   add_msg("Your %s starts charging.", u.weapon.tname().c_str());
			   u.weapon.charges = 0;
			   u.weapon.curammo = dynamic_cast<it_ammo*>(itypes[itm_charge_shot]);
			   u.weapon.active = true;
			   return;
		 }
		 else
		 {
			   add_msg("You need an active charged UPS.");

//CAT-s: menuWrong sound
				playSound(3);

			   return;
		 }
	}

	 if (u.weapon.has_flag(IF_RELOAD_AND_SHOOT)) {
	  reload_index = u.weapon.pick_reload_ammo(u, true);
	  if (reload_index == -1) {
	   add_msg("Out of ammo!");

//CAT-s: menuWrong sound
		playSound(3);

	   return;
	  }

	  u.weapon.reload(u, reload_index);
	  u.moves -= u.weapon.reload_time(u);

//CAT-g: 
//	  refresh_all(); 
	 }

	 if (u.weapon.num_charges() == 0 && !u.weapon.has_flag(IF_RELOAD_AND_SHOOT)) {
	  add_msg("You need to reload!");

//CAT-s: menuWrong sound
		playSound(3);

	  return;
	 }
	 if (u.weapon.has_flag(IF_FIRE_100) && u.weapon.num_charges() < 100) {
	  add_msg("Your %s needs 100 charges to fire!", u.weapon.tname().c_str());

//CAT-s: menuWrong sound
		playSound(3);

	  return;
	 }

	 if (u.weapon.has_flag(IF_USE_UPS) && !u.has_charges(itm_UPS_off, 5) &&
	     !u.has_charges(itm_UPS_on, 5)) {
	  add_msg("You need a UPS with at least 5 charges to fire that!");

//CAT-s: menuWrong sound
		playSound(3);

	  return;
	 }

	 if ((u.weapon.has_flag(IF_STR8_DRAW)  && u.str_cur <  4) ||
	     (u.weapon.has_flag(IF_STR10_DRAW) && u.str_cur <  5)   ) {
	  add_msg("You're not strong enough to draw the bow!");

//CAT-s: menuWrong sound
		playSound(3);

	  return;
	 }


	 int junk;
	 int range = u.weapon.range(&u);

//CAT-mgs: 
/*
 // TODO: [lightmap] This appears to redraw the screen for fireing,
 //                  check were lightmap needs to be shown
 int sight_range = u.sight_range(light_level());
 if (range > sight_range)
  range = sight_range;
*/

	 int x = u.posx, y = u.posy;
	 int x0 = x - range;
	 int y0 = y - range;
	 int x1 = x + range;
	 int y1 = y + range;

/*
	 for (int j = x - VIEWX; j <= x + VIEWX; j++) {
	  for (int k = y - VIEWY; k <= y + VIEWY; k++) {
	   if (u_see(j, k, junk)) {
	    if (k >= y0 && k <= y1 && j >= x0 && j <= x1)
	     m.drawsq(w_terrain, u, j, k, false, true);
	    else
	     mvwputch(w_terrain, k + VIEWY - y, j + VIEWX - x, c_dkgray, '#');
	   }
	  }
	 }
*/

//CAT-g:
	 bool night_vision= false;
	 if( (u.is_wearing(itm_goggles_nv) && u.has_active_item(itm_UPS_on)) 
		|| u.has_active_bionic(bio_night_vision) )
	    night_vision= true;

	 int passtarget = -1;

/*
	// Populate a list of targets with the zombies in range and visible
	 std::vector <monster> mon_targets;
	 std::vector <int> targetindices;

	 for (int i = 0; i < z.size(); i++) {
	  if (z[i].posx >= x0 && z[i].posx <= x1 &&
	      z[i].posy >= y0 && z[i].posy <= y1 &&
      	z[i].friendly == 0 && u_see(&(z[i]), junk)) {
	   mon_targets.push_back(z[i]);
	   targetindices.push_back(i);
	   if (i == last_target)
	    passtarget = mon_targets.size() - 1;
//	   z[i].draw(w_terrain, u.posx, u.posy, true, night_vision);
	  }
	 }
 */

//CAT-mgs: sniping
	 if( u.weapon.typeId() == itm_remington_700 
			|| u.weapon.typeId() == itm_browning_blr
			|| u.weapon.has_gunmod(itm_conversion_sniper) >= 0 )
		SNIPER= true;


	 // target() sets x and y, and returns an empty vector if we canceled (Esc)
	 std::vector <point> trajectory = target(x, y, 
				x0, y0, x1, y1, &u.weapon);



//	 draw_ter(); 
	 wrefresh(w_terrain);

	 if(trajectory.size() == 0) {
	  if(u.weapon.has_flag(IF_RELOAD_AND_SHOOT))
	   unload();
	  return;
	 }

/*
	 if (passtarget != -1) { // We picked a real live target
	  last_target = targetindices[passtarget]; // Make it our default for next time
	  z[targetindices[passtarget]].add_effect(ME_HIT_BY_PLAYER, 100);
	 }
*/

	 if (u.weapon.has_flag(IF_USE_UPS)) {
	  if (u.has_charges(itm_UPS_off, 5))
	   u.use_charges(itm_UPS_off, 5);
	  else if (u.has_charges(itm_UPS_on, 5))
	   u.use_charges(itm_UPS_on, 5);
	 }

	 if(u.weapon.mode == IF_MODE_BURST)
	  burst = true;

	// Train up our skill
	 it_gun* firing = dynamic_cast<it_gun*>(u.weapon.type);
	 int num_shots = 1;
	 if (burst)
	  num_shots = u.weapon.burst_size();
	 if (num_shots > u.weapon.num_charges())
	   num_shots = u.weapon.num_charges();
	 if (u.skillLevel(firing->skill_used) == 0 ||
	     (firing->ammo != AT_BB && firing->ammo != AT_NAIL))
	  u.practice(firing->skill_used, 4 + (num_shots / 2));
	 if (u.skillLevel("gun") == 0 ||
	     (firing->ammo != AT_BB && firing->ammo != AT_NAIL))
	   u.practice("gun", 5);

	 fire(u, x, y, trajectory, burst);


	monmove();
	update_stair_monsters();

 }while(true);
}


void game::butcher()
{
 std::vector<int> corpses;
 for (int i = 0; i < m.i_at(u.posx, u.posy).size(); i++) {
  if (m.i_at(u.posx, u.posy)[i].type->id == itm_corpse)
   corpses.push_back(i);
 }
 if (corpses.size() == 0) {
  add_msg("There are no corpses here to butcher.");
  return;
 }
 int factor = u.butcher_factor();
 if (factor == 999) {
  add_msg("You don't have a sharp item to butcher with.");
  return;
 }
// We do it backwards to prevent the deletion of a corpse from corrupting our
// vector of indices.
 for (int i = corpses.size() - 1; i >= 0; i--) {
  mtype *corpse = m.i_at(u.posx, u.posy)[corpses[i]].corpse;
  if (query_yn("Butcher the %s corpse?", corpse->name.c_str())) {
   int time_to_cut;
   switch (corpse->size) {	// Time in turns to cut up te corpse
    case MS_TINY:   time_to_cut =  2; break;
    case MS_SMALL:  time_to_cut =  5; break;
    case MS_MEDIUM: time_to_cut = 10; break;
    case MS_LARGE:  time_to_cut = 18; break;
    case MS_HUGE:   time_to_cut = 40; break;
   }
   time_to_cut *= 100;	// Convert to movement points
   time_to_cut += factor * 5;	// Penalty for poor tool
   if (time_to_cut < 250)
    time_to_cut = 250;
   u.assign_activity(this, ACT_BUTCHER, time_to_cut, corpses[i]);
   u.moves = 0;
   return;
  }
 }
}

void game::complete_butcher(int index)
{
 mtype* corpse = m.i_at(u.posx, u.posy)[index].corpse;
 int age = m.i_at(u.posx, u.posy)[index].bday;
 m.i_rem(u.posx, u.posy, index);
 int factor = u.butcher_factor();
 int pieces, pelts, bones, sinews;
 double skill_shift = 0.;

 int sSkillLevel = u.skillLevel("survival");

 switch (corpse->size) {
  case MS_TINY:   pieces =  1; pelts =  1; bones = 1; sinews = 1; break;
  case MS_SMALL:  pieces =  2; pelts =  3; bones = 4; sinews = 4; break;
  case MS_MEDIUM: pieces =  4; pelts =  6; bones = 9; sinews = 9; break;
  case MS_LARGE:  pieces =  8; pelts = 10; bones = 14;sinews = 14;break;
  case MS_HUGE:   pieces = 16; pelts = 18; bones = 21;sinews = 21;break;
 }
 if (sSkillLevel < 3)
  skill_shift -= rng(0, 8 - sSkillLevel);
 else
  skill_shift += rng(0, sSkillLevel);
 if (u.dex_cur < 8)
  skill_shift -= rng(0, 8 - u.dex_cur) / 4;
 else
  skill_shift += rng(0, u.dex_cur - 8) / 4;
 if (u.str_cur < 4)
  skill_shift -= rng(0, 5 * (4 - u.str_cur)) / 4;
 if (factor > 0)
  skill_shift -= rng(0, factor / 5);

 int practice = 4 + pieces;
 if (practice > 20)
  practice = 20;
 u.practice("survival", practice);

 pieces += int(skill_shift);
 if (skill_shift < 5)  {	// Lose some pelts and bones
  pelts += (skill_shift - 5);
  bones += (skill_shift - 2);
  sinews += (skill_shift - 8);
 }

 if (bones > 0) {
  if (corpse->has_flag(MF_BONES)) {
    m.add_item(u.posx, u.posy, itypes[itm_bone], age, bones);
   add_msg("You harvest some usable bones!");
  } else if (corpse->mat == VEGGY) {
    m.add_item(u.posx, u.posy, itypes[itm_plant_sac], age, bones);
   add_msg("You harvest some fluid bladders!");
  }
 }

 if (sinews > 0) {
  if (corpse->has_flag(MF_BONES)) {
    m.add_item(u.posx, u.posy, itypes[itm_sinew], age, sinews);
   add_msg("You harvest some usable sinews!");
  } else if (corpse->mat == VEGGY) {
    m.add_item(u.posx, u.posy, itypes[itm_plant_fibre], age, sinews);
   add_msg("You harvest some plant fibres!");
  }
 }

 if ((corpse->has_flag(MF_FUR) || corpse->has_flag(MF_LEATHER)) &&
     pelts > 0) {
  add_msg("You manage to skin the %s!", corpse->name.c_str());
  for (int i = 0; i < pelts; i++) {
   itype* pelt;
   if (corpse->has_flag(MF_FUR) && corpse->has_flag(MF_LEATHER)) {
    if (one_in(2))
     pelt = itypes[itm_fur];
    else
     pelt = itypes[itm_leather];
   } else if (corpse->has_flag(MF_FUR))
    pelt = itypes[itm_fur];
   else
    pelt = itypes[itm_leather];
   m.add_item(u.posx, u.posy, pelt, age);
  }
 }

 //Add a chance of CBM recovery. For shocker and cyborg corpses.
 if (corpse->has_flag(MF_CBM)) {
  //As long as the factor is above -4 (the sinew cutoff), you will be able to extract cbms
  if(skill_shift >= 0){
   add_msg("You discover a CBM in the %s!", corpse->name.c_str());
   //To see if it spawns a battery
   if(rng(0,1) == 1){ //The battery works
    m.add_item(u.posx, u.posy, itypes[itm_bionics_batteries], age);
   }else{//There is a burnt out CBM
    m.add_item(u.posx, u.posy, itypes[itm_burnt_out_bionic], age);
   }
  }
  if(skill_shift >= 0){
   //To see if it spawns a random additional CBM
   if(rng(0,1) == 1){ //The CBM works
    int index = rng(0, mapitems[mi_bionics].size()-1);
    m.add_item(u.posx, u.posy, itypes[ mapitems[mi_bionics][index] ], age);
   }else{//There is a burnt out CBM
    m.add_item(u.posx, u.posy, itypes[itm_burnt_out_bionic], age);
   }
  }
 }

 if (pieces <= 0)
  add_msg("Your clumsy butchering destroys the meat!");
 else {
  itype* meat;
  if (corpse->has_flag(MF_POISON)) {
    if (corpse->mat == FLESH)
     meat = itypes[itm_meat_tainted];
    else
     meat = itypes[itm_veggy_tainted];
  } else {
   if (corpse->mat == FLESH)
    if(corpse->has_flag(MF_HUMAN))
     meat = itypes[itm_human_flesh];
    else
     meat = itypes[itm_meat];
   else
    meat = itypes[itm_veggy];
  }
  for (int i = 0; i < pieces; i++)
   m.add_item(u.posx, u.posy, meat, age);
  add_msg("You butcher the corpse.");
 }
}

void game::forage()
{
  int veggy_chance = rng(1, 20);

  if (veggy_chance < u.skillLevel("survival"))
  {
    add_msg("You found some wild veggies!");
    u.practice("survival", 10);
    m.add_item(u.activity.placement.x, u.activity.placement.y, this->itypes[itm_veggy_wild],0);
    m.ter(u.activity.placement.x, u.activity.placement.y) = t_dirt;
  }
  else
  {
    add_msg("You didn't find anything.");
    if (!one_in(u.skillLevel("survival")))
    m.ter(u.activity.placement.x, u.activity.placement.y) = t_dirt;
  }
}

void game::eat(char chInput)
{
 char ch;
 if (u.has_trait(PF_RUMINANT) && m.ter(u.posx, u.posy) == t_underbrush &&
     query_yn("Eat underbrush?")) {
  u.moves -= 400;
  u.hunger -= 10;
  m.ter(u.posx, u.posy) = t_grass;
  add_msg("You eat the underbrush.");
  return;
 }
 if (chInput == '.')
  ch = inv_type("Consume item:", IC_COMESTIBLE);
 else
  ch = chInput;

 if (ch == ' ') {
  add_msg("Never mind.");
  return;
 }

 if (!u.has_item(ch)) {
  add_msg("You don't have item '%c'!", ch);
  return;
 }
 u.eat(this, u.lookup_item(ch));
}

void game::wear(char chInput)
{
 char ch;
 if (chInput == '.')
  ch = inv_type("Wear item:", IC_ARMOR);
 else
  ch = chInput;

 if (ch == ' ') {
  add_msg("Never mind.");
  return;
 }
 u.wear(this, ch);
}

void game::takeoff(char chInput)
{
 char ch;
 if (chInput == '.')
  ch = inv_type("Take off item:", IC_NULL);
 else
  ch = chInput;

 if (u.takeoff(this, ch))
  u.moves -= 250; // TODO: Make this variable
 else
  add_msg("Invalid selection.");
}

void game::reload(char chInput)
{
 //Quick and dirty hack
 //Save old weapon in temp variable
 //Wield item that should be unloaded
 //Reload weapon
 //Put unloaded item back into inventory
 //Wield old weapon
 bool bSwitch = false;
 item oTempWeapon;
 int iItemIndex = u.inv.index_by_letter(chInput);

 if (u.weapon.invlet != chInput && iItemIndex != -1) {
  oTempWeapon = u.weapon;
  u.weapon = u.inv[iItemIndex];
  u.inv.remove_item(iItemIndex);
  bSwitch = true;
 }

 if (bSwitch || u.weapon.invlet == chInput) {
  reload();
  u.activity.moves_left = 0;
  monmove();
  process_activity();
 }

 if (bSwitch) {
  u.inv.push_back(u.weapon);
  u.weapon = oTempWeapon;
 }
}

void game::reload()
{
 if (u.weapon.is_gun()) {
  if (u.weapon.has_flag(IF_RELOAD_AND_SHOOT)) {
   add_msg("Your %s does not need to be reloaded; it reloads and fires in a \
single action.", u.weapon.tname().c_str());
   return;
  }
  if (u.weapon.ammo_type() == AT_NULL) {
   add_msg("Your %s does not reload normally.", u.weapon.tname().c_str());
   return;
  }
  if (u.weapon.charges == u.weapon.clip_size()) {
   int alternate_magazine = -1;
   for (int i = 0; i < u.weapon.contents.size(); i++) {
     if (u.weapon.contents[i].is_gunmod() &&
         (u.weapon.contents[i].typeId() == itm_spare_mag &&
          u.weapon.contents[i].charges < (dynamic_cast<it_gun*>(u.weapon.type))->clip) ||
         (u.weapon.contents[i].has_flag(IF_MODE_AUX) &&
          u.weapon.contents[i].charges < u.weapon.contents[i].clip_size()))
      alternate_magazine = i;
   }
   if(alternate_magazine == -1) {
    add_msg("Your %s is fully loaded!", u.weapon.tname(this).c_str());
    return;
   }
  }
  int index = u.weapon.pick_reload_ammo(u, true);
  if (index == -1) {
   add_msg("Out of ammo!");
   return;
  }
  u.assign_activity(this, ACT_RELOAD, u.weapon.reload_time(u), index);
  u.moves = 0;
 } else if (u.weapon.is_tool()) {
  it_tool* tool = dynamic_cast<it_tool*>(u.weapon.type);
  if (tool->ammo == AT_NULL) {
   add_msg("You can't reload a %s!", u.weapon.tname(this).c_str());
   return;
  }
  int index = u.weapon.pick_reload_ammo(u, true);
  if (index == -1) {
// Reload failed
   add_msg("Out of %s!", ammo_name(tool->ammo).c_str());
   return;
  }
  u.assign_activity(this, ACT_RELOAD, u.weapon.reload_time(u), index);
  u.moves = 0;
 } else if (!u.is_armed())
  add_msg("You're not wielding anything.");
 else
  add_msg("You can't reload a %s!", u.weapon.tname(this).c_str());

//CAT-g: 
// refresh_all();
}

// Unload a containter, gun, or tool
// If it's a gun, some gunmods can also be loaded
void game::unload(char chInput)
{
 //Quick and dirty hack
 //Save old weapon in temp variable
 //Wield item that should be unloaded
 //Unload weapon
 //Put unloaded item back into inventory
 //Wield old weapon
 bool bSwitch = false;
 item oTempWeapon;
 int iItemIndex = u.inv.index_by_letter(chInput);

 if (u.weapon.invlet != chInput && iItemIndex != -1) {
  oTempWeapon = u.weapon;
  u.weapon = u.inv[iItemIndex];
  u.inv.remove_item(iItemIndex);
  bSwitch = true;
 }

 if (bSwitch || u.weapon.invlet == chInput) {
  unload();
 }

 if (bSwitch) {
  u.inv.push_back(u.weapon);
  u.weapon = oTempWeapon;
 }
}

void game::unload()
{
//CAT-mgs: from DDA.5 
 if ( u.weapon.has_flag(IF_NO_UNLOAD)
		|| (!u.weapon.is_gun() && u.weapon.contents.size() == 0 
		&& (!u.weapon.is_tool() || u.weapon.ammo_type() == AT_NULL)) )
 {
	add_msg("You can't unload a %s!", u.weapon.tname(this).c_str());
	return;
 }


/*
 if( !u.weapon.is_gun() && u.weapon.contents.size() == 0 
	&& (!u.weapon.is_tool() || u.weapon.ammo_type() == AT_NULL 
	|| u.weapon.has_flag(IF_NO_UNLOAD)) )
 {

	add_msg("You can't unload a %s!", u.weapon.tname(this).c_str());
	return;
 }
*/


 int spare_mag = -1;
 int has_m203 = -1;
 int has_shotgun = -1;
 if (u.weapon.is_gun()) {
  spare_mag = u.weapon.has_gunmod (itm_spare_mag);
  has_m203 = u.weapon.has_gunmod (itm_m203);
  has_shotgun = u.weapon.has_gunmod (itm_u_shotgun);
 }
 if (u.weapon.is_container() || u.weapon.charges == 0 &&
     (spare_mag == -1 || u.weapon.contents[spare_mag].charges <= 0) &&
     (has_m203 == -1 || u.weapon.contents[has_m203].charges <= 0) &&
     (has_shotgun == -1 || u.weapon.contents[has_shotgun].charges <= 0)) {
  if (u.weapon.contents.size() == 0) {
   if (u.weapon.is_gun())
    add_msg("Your %s isn't loaded, and is not modified.",
            u.weapon.tname(this).c_str());
   else
    add_msg("Your %s isn't charged." , u.weapon.tname(this).c_str());
   return;
  }
// Unloading a container!
  u.moves -= 40 * u.weapon.contents.size();
  std::vector<item> new_contents;	// In case we put stuff back
  while (u.weapon.contents.size() > 0) {
   item content = u.weapon.contents[0];
   int iter = 0;
// Pick an inventory item for the contents
   while ((content.invlet == 0 || u.has_item(content.invlet)) && iter < 52) {
    content.invlet = nextinv;
    advance_nextinv();
    iter++;
   }
   if (content.made_of(LIQUID)) {
    if (!handle_liquid(content, false, false))
     new_contents.push_back(content);// Put it back in (we canceled)
   } else {
    if (u.volume_carried() + content.volume() <= u.volume_capacity() &&
        u.weight_carried() + content.weight() <= u.weight_capacity() &&
        iter < 52) {
     add_msg("You put the %s in your inventory.", content.tname(this).c_str());
     u.i_add(content, this);
    } else {
     add_msg("You drop the %s on the ground.", content.tname(this).c_str());
     m.add_item(u.posx, u.posy, content);
    }
   }
   u.weapon.contents.erase(u.weapon.contents.begin());
  }
  u.weapon.contents = new_contents;
  return;
 }
// Unloading a gun or tool!
 u.moves -= int(u.weapon.reload_time(u) / 2);
 // Default to unloading the gun, but then try other alternatives.
 item* weapon = &u.weapon;
 it_ammo* tmpammo;
 if (weapon->is_gun()) {	// Gun ammo is combined with existing items
  // If there's an active gunmod, unload it first.
  item* active_gunmod = weapon->active_gunmod();
  if (active_gunmod != NULL && active_gunmod->charges > 0)
   weapon = active_gunmod;
  // Then try and unload a spare magazine if there is one.
  else if (spare_mag != -1 && weapon->contents[spare_mag].charges > 0)
   weapon = &weapon->contents[spare_mag];
  // Then try the grenade launcher
  else if (has_m203 != -1 && weapon->contents[has_m203].charges > 0)
   weapon = &weapon->contents[has_m203];
  // Then try an underslung shotgun
  else if (has_shotgun != -1 && weapon->contents[has_shotgun].charges > 0)
   weapon = &weapon->contents[has_shotgun];
  for (int i = 0; i < u.inv.size() && weapon->charges > 0; i++) {
   if (u.inv[i].is_ammo()) {
    tmpammo = dynamic_cast<it_ammo*>(u.inv[i].type);
    if (tmpammo->id == weapon->curammo->id &&
        u.inv[i].charges < tmpammo->count) {
     weapon->charges -= (tmpammo->count - u.inv[i].charges);
     u.inv[i].charges = tmpammo->count;
     if (weapon->charges < 0) {
      u.inv[i].charges += weapon->charges;
      weapon->charges = 0;
     }
    }
   }
  }
 }
 item newam;

 if ((weapon->is_gun() || weapon->is_gunmod()) && weapon->curammo != NULL)
  newam = item(weapon->curammo, turn);
 else
  newam = item(itypes[default_ammo(weapon->ammo_type())], turn);
 while (weapon->charges > 0) {
  int iter = 0;
  while ((newam.invlet == 0 || u.has_item(newam.invlet)) && iter < 52) {
   newam.invlet = nextinv;
   advance_nextinv();
   iter++;
  }
  if (newam.made_of(LIQUID))
   newam.charges = weapon->charges;
  weapon->charges -= newam.charges;
  if (weapon->charges < 0) {
   newam.charges += weapon->charges;
   weapon->charges = 0;
  }
  if (u.weight_carried() + newam.weight() < u.weight_capacity() &&
      u.volume_carried() + newam.volume() < u.volume_capacity() && iter < 52) {
   if (newam.made_of(LIQUID)) {
    if (!handle_liquid(newam, false, false))
     weapon->charges += newam.charges;	// Put it back in
   } else
    u.i_add(newam, this);
  } else
   m.add_item(u.posx, u.posy, newam);
 }
 weapon->curammo = NULL;
}

void game::wield(char chInput)
{
 if (u.weapon.has_flag(IF_NO_UNWIELD)) {
// Bionics can't be unwielded
  add_msg("You cannot unwield your %s.", u.weapon.tname(this).c_str());
  return;
 }
 char ch;
 if (chInput == '.') {
  if (u.styles.empty())
   ch = inv("Wield item:");
  else
   ch = inv("Wield item: Press - to choose a style");
 } else
  ch = chInput;

 bool success = false;
 if (ch == '-')
  success = u.wield(this, -3);
 else
  success = u.wield(this, u.lookup_item(ch));

 if (success)
  u.recoil = 5;
}

void game::read()
{
 char ch = inv_type("Read:", IC_BOOK);
 u.read(this, ch);
}

void game::chat()
{
 if (active_npc.size() == 0) {
  add_msg("You talk to yourself for a moment.");
  return;
 }
 std::vector<npc*> available;
 int junk;
 for (int i = 0; i < active_npc.size(); i++) {
  if (u_see(active_npc[i].posx, active_npc[i].posy, junk) &&
      rl_dist(u.posx, u.posy, active_npc[i].posx, active_npc[i].posy) <= 24)
   available.push_back(&active_npc[i]);
 }
 if (available.size() == 0) {
  add_msg("There's no-one close enough to talk to.");
  return;
 } else if (available.size() == 1)
  available[0]->talk_to_u(this);
 else {
  WINDOW *w = newwin(available.size() + 3, 40, (TERMY-available.size() + 3)/2, (TERMX-40)/2);
  wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
             LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
  for (int i = 0; i < available.size(); i++)
   mvwprintz(w, i + 1, 1, c_white, "%d: %s", i + 1, available[i]->name.c_str());
  mvwprintz(w, available.size() + 1, 1, c_white, "%d: Cancel",
            available.size() + 1);
  wrefresh(w);
  char ch;
  do {
   ch = getch();
  } while (ch < '1' || ch > '1' + available.size());
  ch -= '1';
  if (ch == available.size())
   return;
  delwin(w);
  available[ch]->talk_to_u(this);
 }
 u.moves -= 100;
}


void game::pldrive(int x, int y) {

 if (run_mode > 1)
 {
	// Monsters around and we don't wanna run
	if(run_mode==3)
		add_msg("Monster spotted you!");
//CAT-s:
	playSound(10);
	return;
 }

 int part = -1;
 vehicle *veh = m.veh_at (u.posx, u.posy, part);
 if (!veh) {
/*
  dbg(D_ERROR) << "game:pldrive: can't find vehicle! Drive mode is now off.";
  debugmsg ("game::pldrive error: can't find vehicle! Drive mode is now off.");
*/
  u.in_vehicle = false;
  CARJUMPED= false;
  return;
 }

 int pctr = veh->part_with_feature (part, vpf_controls);
 if(pctr < 0)
 {
	  add_msg("You can't drive from here, you need controls.");
//CAT-s:
	playSound(3);
	return;
 }


// add_msg("POW: %d", veh->total_power());

//CAT-mgs: *** vvv
//TODO: elctric motor needs "jumpstart" & different sound effect
 if(!CARJUMPED && veh->type != veh_bicycle && veh->total_power() > 0)
 {
	int mskill= u.skillLevel("mechanics");
	int eskill= u.skillLevel("electronics");
	if(mskill < 3 || eskill < 3)
	{
		playSound(3);
		add_msg("You need at least skill 3 in mechanics and electronics to hot-wire the vehicle.");
	}
	else
//	if(query_yn("Attempt to hot-wire?"))
	{
	   playSound(52);		
	   add_msg("You attempt hot-wiring...");
	   if( one_in(2) && !one_in( int((mskill+eskill)/3) ) )
	   {
		m.board_vehicle (this, x, y, &u);
		u.moves -= 200;
//CAT-s:
		playSound(53);
		CARJUMPED= true;
		add_msg("You start the engine.");
	   }
	   else
		add_msg("The engine fails to start.");
	}

	u.moves = 0;
	return;
 }
//CAT: *** ^^^


 int thr_amount = 10 * 100;
 if(veh->cruise_on)
	veh->cruise_thrust(-y * thr_amount);


 veh->turn (15 * x);
 if (veh->skidding && veh->valid_wheel_config()) {
  if (rng (0, 100) < u.dex_cur + u.skillLevel("driving") * 2) {
   add_msg ("You regain control of the %s.", veh->name.c_str());
   veh->velocity = veh->forward_velocity();
   veh->skidding = false;
   veh->move.init (veh->turn_dir);
  }
 }


 if(y == -1 
		&& (abs(veh->cruise_velocity) != 0 || abs(veh->velocity) != 0)
		&& veh->total_power() > 0)
	playSound(126);
 else
 if(y == 1 
		&& (abs(veh->cruise_velocity) != 0 || abs(veh->velocity) != 0)	
		&& veh->total_power() > 0)
	playSound(127);



 u.moves = 0;
 if (x != 0 && veh->velocity != 0 && one_in(4))
   u.practice("driving", 1);


//CAT-g: auto shift view
	static int old_vel= 0;
	static int old_cvel= 0;
	static int old_turn= 0;

	if( abs(old_vel - veh->velocity) > 900 
		|| old_turn != veh->turn_dir || old_cvel != veh->cruise_velocity )
	{
		float cat_vel= veh->velocity*2;

		if(cat_vel > 10000) cat_vel= 10000;
		u.view_offset_y= (int)(sin(veh->turn_dir * M_PI/180) *cat_vel)/1000;
		u.view_offset_x= (int)(cos(veh->turn_dir * M_PI/180) *cat_vel)/1000;

		old_turn= veh->turn_dir;

		old_vel= veh->velocity;
		old_cvel= veh->cruise_velocity;
	}
}


void game::plmove(int x, int y)
{
 if (run_mode > 1)
 {
	// Monsters around and we don't wanna run
	if(run_mode==3)
		add_msg("Monster spotted you!");
//CAT-s:
	playSound(10);
	return;
 }

 if (u.has_disease(DI_STUNNED)) {
  x = rng(u.posx - 1, u.posx + 1);
  y = rng(u.posy - 1, u.posy + 1);
 } else {
  x += u.posx;
  y += u.posy;
 }


// Check if our movement is actually an attack on a monster
 int mondex = mon_at(x, y);
 bool displace = false;	// Are we displacing a monster?
 if (mondex != -1) {
  if (z[mondex].friendly == 0) {
   int udam = u.hit_mon(this, &z[mondex]);
   char sMonSym = '%';
   nc_color cMonColor = z[mondex].type->color;
   if (z[mondex].hurt(udam))
    kill_mon(mondex, true);
   else
    sMonSym = z[mondex].symbol();
   hit_animation(w_terrain, x - u.posx + VIEWX - u.view_offset_x,
                 y - u.posy + VIEWY - u.view_offset_y,
                 red_background(cMonColor), sMonSym);
   return;
  } else
   displace = true;
 }

//CAT-mgs: no NPCs
// If not a monster, maybe there's an NPC there
 int npcdex = npc_at(x, y);
 if (npcdex != -1) {
  if (!active_npc[npcdex].is_enemy() &&
      !query_yn("Really attack %s?", active_npc[npcdex].name.c_str())) {
   if (active_npc[npcdex].is_friend()) {
    add_msg("%s moves out of the way.", active_npc[npcdex].name.c_str());
    active_npc[npcdex].move_away_from(this, u.posx, u.posy);
   }

   return;	// Cancel the attack
  }
  u.hit_player(this, active_npc[npcdex]);
  active_npc[npcdex].make_angry();
  if (active_npc[npcdex].hp_cur[hp_head]  <= 0 ||
      active_npc[npcdex].hp_cur[hp_torso] <= 0   ) {
   active_npc[npcdex].die(this, true);
   //active_npc.erase(active_npc.begin() + npcdex);
  }
  return;
 }

//CAT-s:
 if(u.underwater && m.ter(x,y) != t_water_dp)
 {
	vertical_move(1, true);
	u.underwater = false;
 }

// Otherwise, actual movement, zomg
 if (u.has_disease(DI_AMIGARA)) {
  int curdist = 999, newdist = 999;
  for (int cx = 0; cx < SEEX * MAPSIZE; cx++) {
   for (int cy = 0; cy < SEEY * MAPSIZE; cy++) {
    if (m.ter(cx, cy) == t_fault) {
     int dist = rl_dist(cx, cy, u.posx, u.posy);
     if (dist < curdist)
      curdist = dist;
     dist = rl_dist(cx, cy, x, y);
     if (dist < newdist)
      newdist = dist;
    }
   }
  }
  if (newdist > curdist) {
   add_msg("You cannot pull yourself away from the faultline...");
   return;
  }
 }

 if (u.has_disease(DI_IN_PIT)) {
  if (rng(0, 40) > u.str_cur + int(u.dex_cur / 2)) {
   add_msg("You try to escape the pit, but slip back in.");
   u.moves -= 100;
   return;
  } else {
   add_msg("You escape the pit!");
   u.rem_disease(DI_IN_PIT);
  }
 }
 if (u.has_disease(DI_DOWNED)) {
  if (rng(0, 40) > u.dex_cur + int(u.str_cur / 2)) {
   add_msg("You struggle to stand.");
   u.moves -= 100;
   return;
  } else {
   add_msg("You stand up.");
   u.rem_disease(DI_DOWNED);
   u.moves -= 100;
   return;
  }
 }

 int vpart = -1, dpart = -1;
 vehicle *veh = m.veh_at(x, y, vpart);
 bool veh_closed_door = false;
 if (veh) {
  dpart = veh->part_with_feature (vpart, vpf_openable);
  veh_closed_door = dpart >= 0 && !veh->parts[dpart].open;
 }

 // move_cost() of 0 = impassible (e.g. a wall)
 if (m.move_cost(x, y) > 0)
 { 
  if (u.underwater)
   u.underwater = false;

  dpart = veh ? veh->part_with_feature (vpart, vpf_seat) : -1;
  bool can_board = dpart >= 0 && veh->parts[dpart].items.size() == 0
	 && !veh->parts[dpart].has_flag(vehicle_part::passenger_flag);

  if(can_board && query_yn("Board vehicle?"))
  {
	m.board_vehicle (this, x, y, &u);
	u.moves -= 200;
 	return;
  }


//CAT-mgs: *** vvv
  if(JUMPING != 2 && m.field_at(x, y).is_dangerous() )
  {
	if(JUMPING == 0 && !m.field_at(u.posx, u.posy).is_dangerous())
	{
	     	if( !query_yn("Really step into that %s?", m.field_at(x, y).name().c_str()) )
			return;
	}
  }

  if(JUMPING != 2 && m.tr_at(x, y) != tr_null &&
      u.per_cur - u.encumb(bp_eyes) >= traps[m.tr_at(x, y)]->visibility)
  {
	if(!traps[m.tr_at(x, y)]->is_benign())
	{
		bool goon= true;
		
		if(JUMPING == 3)
		{
			bool canHang= false;
	      	for(int i= -1; i <= 1; i++)
			{
				for(int j= -1; j <= 1; j++)
				{
					//|| !m.is_outside(x+i,y+j)
					if(m.ter(x+i,y+j) == t_sidewalk)
						canHang= true;
				}

			}

			if(canHang)
				goon= query_yn("You hang on, drop down?");
		}
		else
		if(JUMPING == 0)		
	      	goon= query_yn("Really step onto that %s?",traps[m.tr_at(x, y)]->name.c_str());

		if(!goon)
	           	return;
	}
  }


//CAT-mgs: abovelevel drop
/*
  if(JUMPING != 2 && levz > 0 && m.ter(x, y) == t_air)
  {
//CAT: fall at once, or... ? 
	
	if(JUMPING == 0 && !m.ter(u.posx, u.posy) != t_air)
	{
  		if( !query_yn("Really jump down?") )
			return;
	}

	add_msg("You fall down a level!");
	vertical_move(-1, true); 
  }

*/


// no need to query if stepping into 'benign' traps
/*
  if (m.tr_at(x, y) != tr_null &&
      u.per_cur - u.encumb(bp_eyes) >= traps[m.tr_at(x, y)]->visibility &&
      !query_yn("Really step onto that %s?",traps[m.tr_at(x, y)]->name.c_str()))
   return;
*/



// Calculate cost of moving
  u.moves -= u.run_cost(m.move_cost(x, y) * 50);

// Adjust recoil down
  if (u.recoil > 0) {
    if (int(u.str_cur / 2) + u.skillLevel("gun") >= u.recoil)
    u.recoil = 0;
   else {
     u.recoil -= int(u.str_cur / 2) + u.skillLevel("gun");
    u.recoil = int(u.recoil / 2);
   }
  }


//CAT-mgs:
  static ter_id last_ter;

  if ((!u.has_trait(PF_PARKOUR) && m.move_cost(x, y) > 2) ||
      ( u.has_trait(PF_PARKOUR) && m.move_cost(x, y) > 4    ))
  {

   if (veh)
   {
	add_msg("Moving past this %s is slow!", veh->part_info(vpart).name);

//CAT-s: say "Hoya!"
//	playSound(38);
	playSound(55); //step
   }
   else
   {

//CAT-s: say "Hoya!" or Splash!
	if(m.ter(x,y) == t_water_sh || m.ter(x,y) == t_sewage)
	{
		if(one_in(5))
			playSound(27); //splash1.wav
	}
	else
	{
		if(last_ter != m.ter(x,y) && JUMPING == 0 )
		{
		   if( !query_yn("Really step up on this %s?", m.tername(x, y).c_str()) )
			return;
		}

		playSound(38); //Hoya!
	}

	add_msg("Moving past this %s is slow!", m.tername(x, y).c_str());
   }
  }

  last_ter= m.ter(x,y);


  if (m.has_flag(rough, x, y) && (!u.in_vehicle)) {
   if (one_in(5) && u.armor_bash(bp_feet) < rng(2, 5)) {
    add_msg("You hurt your feet on the %s!", m.tername(x, y).c_str());
    u.hit(this, bp_feet, 0, 0, 1);
    u.hit(this, bp_feet, 1, 0, 1);
   }
  }
  if (m.has_flag(sharp, x, y) && !one_in(3) && !one_in(40 - int(u.dex_cur/2))
      && (!u.in_vehicle)) {
   if (!u.has_trait(PF_PARKOUR) || one_in(4)) {
    body_part bp = random_body_part();
    int side = rng(0, 1);
    add_msg("You cut your %s on the %s!", body_part_name(bp, side).c_str(), m.tername(x, y).c_str());
    u.hit(this, bp, side, 0, rng(1, 4));
   }
  }

  if (!u.has_artifact_with(AEP_STEALTH) && !u.has_trait(PF_LEG_TENTACLES)) {

   if (u.has_trait(PF_LIGHTSTEP))
    sound(x, y, 4, "");	// Sound of footsteps may awaken nearby monsters
   else
    sound(x, y, 8, "");	// Sound of footsteps may awaken nearby monsters
  }
  if (one_in(20) && u.has_artifact_with(AEP_MOVEMENT_NOISE))
   sound(x, y, 40, "You emit a rattling sound.");
// If we moved out of the nonant, we need update our map data
  if (m.has_flag(swimmable, x, y) && u.has_disease(DI_ONFIRE)) {
   add_msg("The water puts out the flames!");
   u.rem_disease(DI_ONFIRE);
  }
// displace is set at the top of this function.
  if (displace) { // We displaced a friendly monster!
// Immobile monsters can't be displaced.
   if (z[mondex].has_flag(MF_IMMOBILE)) {
// ...except that turrets can be picked up.
// TODO: Make there a flag, instead of hard-coded to mon_turret
    if (z[mondex].type->id == mon_turret) {
     if (query_yn("Deactivate the turret?")) {
      z.erase(z.begin() + mondex);
      u.moves -= 100;
      m.add_item(z[mondex].posx, z[mondex].posy, itypes[itm_bot_turret], turn);
     }
     return;
    } else {
     add_msg("You can't displace your %s.", z[mondex].name().c_str());
     return;
    }
   }
   z[mondex].move_to(this, u.posx, u.posy);
   add_msg("You displace the %s.", z[mondex].name().c_str());
  }
//CAT-g: why in plmove, should be in do_turn()?
  if (x < SEEX * int(MAPSIZE / 2) || y < SEEY * int(MAPSIZE / 2) ||
      x >= SEEX * (1 + int(MAPSIZE / 2)) || y >= SEEY * (1 + int(MAPSIZE / 2)))
   update_map(x, y);

  u.posx = x;
  u.posy = y;

//CAT-mgs:
 ltar_x= u.posx;
 ltar_y= u.posy;

  if(JUMPING != 2 && m.tr_at(x, y) != tr_null) { // We stepped on a trap!
   trap* tr = traps[m.tr_at(x, y)];
   if (!u.avoid_trap(tr)) {
    trapfunc f;
    (f.*(tr->act))(this, x, y);
   }
  }
//*** ^^^

// Some martial art styles have special effects that trigger when we move
  switch (u.weapon.type->id) {

   case itm_style_capoeira:
    if (u.disease_level(DI_ATTACK_BOOST) < 2)
     u.add_disease(DI_ATTACK_BOOST, 2, this, 2, 2);
    if (u.disease_level(DI_DODGE_BOOST) < 2)
     u.add_disease(DI_DODGE_BOOST, 2, this, 2, 2);
    break;

   case itm_style_ninjutsu:
    u.add_disease(DI_ATTACK_BOOST, 2, this, 1, 3);
    break;

   case itm_style_crane:
    if (!u.has_disease(DI_DODGE_BOOST))
     u.add_disease(DI_DODGE_BOOST, 1, this, 3, 3);
    break;

   case itm_style_leopard:
    u.add_disease(DI_ATTACK_BOOST, 2, this, 1, 4);
    break;

   case itm_style_dragon:
    if (!u.has_disease(DI_DAMAGE_BOOST))
     u.add_disease(DI_DAMAGE_BOOST, 2, this, 3, 3);
    break;

   case itm_style_lizard: {
    bool wall = false;
    for (int wallx = x - 1; wallx <= x + 1 && !wall; wallx++) {
     for (int wally = y - 1; wally <= y + 1 && !wall; wally++) {
      if (m.has_flag(supports_roof, wallx, wally))
       wall = true;
     }
    }
    if (wall)
     u.add_disease(DI_ATTACK_BOOST, 2, this, 2, 8);
    else
     u.rem_disease(DI_ATTACK_BOOST);
   } break;
  }



// List items here
  if (!u.has_disease(DI_BLIND) && m.i_at(x, y).size() <= 3 &&
                                  m.i_at(x, y).size() != 0) {
   std::string buff = "You see here ";
   for (int i = 0; i < m.i_at(x, y).size(); i++) {
    buff += m.i_at(x, y)[i].tname(this);
    if (i + 2 < m.i_at(x, y).size())
     buff += ", ";
    else if (i + 1 < m.i_at(x, y).size())
     buff += ", and ";
   }
   buff += ".";
   add_msg(buff.c_str());
  } else if (m.i_at(x, y).size() != 0)
   add_msg("There are many items here.");

 } else if (veh_closed_door) { // move_cost <= 0
  veh->parts[dpart].open = 1;
  veh->insides_dirty = true;
  u.moves -= 100;
  add_msg ("You open the %s's %s.", veh->name.c_str(),
                                    veh->part_info(dpart).name);

//CAT-s:
	playSound(56);

 } else if (m.has_flag(swimmable, x, y)) { // Dive into water!
// Requires confirmation if we were on dry land previously
  if ((m.has_flag(swimmable, u.posx, u.posy) &&
      m.move_cost(u.posx, u.posy) == 0) || query_yn("Dive into the water?")) {
   if (m.move_cost(u.posx, u.posy) > 0 && u.swim_speed() < 500)
   {
	add_msg("You start swimming.  Press '>' to dive underwater.");
//CAT-s: waterDive
	playSound(39);
   }

//CAT-s: waterSwim
   if(one_in(7) && !u.underwater)
	playSound(40);

   plswim(x, y);
  }

 } 
 else
 {
// Invalid move

//CAT-mgs:
	if(JUMPING == 2)
	{
		std::string junk;
		m.bash(x, y, 20+u.str_cur, junk);

//		add_msg("STR_CUR: %d", u.str_cur);
	}

	if (u.has_disease(DI_BLIND) || u.has_disease(DI_STUNNED))
	{
	// Only lose movement if we're blind
	   add_msg("You bump into a %s!", m.tername(x, y).c_str());
	   u.moves -= 100;
	}
	else
	  if(m.open_door(x, y, m.is_indoor(u.posx, u.posy)))
	  {
//CAT-s: 
		playSound(56);
		u.moves -= 100;
	  }
	  else
	  if (m.ter(x, y) == t_door_locked || m.ter(x, y) == t_door_locked_alarm)
	  {
		u.moves -= 100;
		add_msg("That door is locked!");
	  }
 }
}


void game::plswim(int x, int y)
{
//CAT-g: it's already handled above in plmove(), or not?
 if (x < SEEX * int(MAPSIZE / 2) || y < SEEY * int(MAPSIZE / 2) ||
     x >= SEEX * (1 + int(MAPSIZE / 2)) || y >= SEEY * (1 + int(MAPSIZE / 2)))
  update_map(x, y);

 u.posx = x;
 u.posy = y;

 if (u.has_disease(DI_ONFIRE)) {
  add_msg("The water puts out the flames!");
  u.rem_disease(DI_ONFIRE);
 }
 int movecost = u.swim_speed();
 u.practice("swimming", 1);
 if (movecost >= 500) {
  if (!u.underwater) {
   add_msg("You sink%s!", (movecost >= 400 ? " like a rock" : ""));
   u.underwater = true;
   u.oxygen = 30 + 2 * u.str_cur;
  }
 }
 if (u.oxygen <= 5 && u.underwater) {
  if (movecost < 500)
   popup("You need to breathe! (Press '<' to surface.)");
  else
   popup("You need to breathe but you can't swim!  Get to dry land, quick!");
 }
 u.moves -= (movecost > 200 ? 200 : movecost);
 for (int i = 0; i < u.inv.size(); i++) {
  if (u.inv[i].type->m1 == IRON && u.inv[i].damage < 5 && one_in(8))
   u.inv[i].damage++;
 }
}

void game::fling_player_or_monster(player *p, monster *zz, int dir, int flvel)
{
    int steps = 0;
    bool is_u = p && (p == &u);
    int dam1, dam2;

    bool is_player;
    if (p)
        is_player = true;
    else
    if (zz)
        is_player = false;
    else
	return;


    tileray tdir(dir);
    std::string sname, snd;
    if (p)
    {
        if (is_u)
            sname = std::string ("You are");
        else
            sname = p->name + " is";
    }
    else
        sname = zz->name() + " is";

    int range = flvel / 10;
    int x = (is_player? p->posx : zz->posx);
    int y = (is_player? p->posy : zz->posy);
    while (range > 0)
    {
        tdir.advance();
        x = (is_player? p->posx : zz->posx) + tdir.dx();
        y = (is_player? p->posy : zz->posy) + tdir.dy();
        std::string dname;
        bool thru = true;
        bool slam = false;
        int mondex = mon_at(x, y);
        dam1 = flvel / 3 + rng (0, flvel * 1 / 3);
        if (mondex >= 0)
        {
            slam = true;
            dname = z[mondex].name();
            dam2 = flvel / 3 + rng (0, flvel * 1 / 3);
            if (z[mondex].hurt(dam2))
		{
			kill_mon(mondex, !is_player);
//			add_msg("kill_mon 1");

		}
            else
             thru = false;

            if (is_player)
			p->hitall (this, dam1, 40);
            else
		if(zz->hurt(dam1))
		{
			kill_mon(mondex, !is_player);
//			add_msg("kill_mon2");
		}


        } else if (m.move_cost(x, y) == 0 && !m.has_flag(swimmable, x, y)) {
            slam = true;
            int vpart;
            vehicle *veh = m.veh_at(x, y, vpart);
            dname = veh ? veh->part_info(vpart).name : m.tername(x, y).c_str();
            if (m.has_flag(bashable, x, y))
                thru = m.bash(x, y, flvel, snd);
            else
                thru = false;
            if (snd.length() > 0)
                add_msg ("You hear a %s", snd.c_str());
            if (is_player)
                p->hitall (this, dam1, 40);
            else
		if(zz->hurt(dam1))
		{
			kill_mon(mondex, !is_player);
//			add_msg("kill_mon3");
		}

            flvel = flvel / 2;
        }

        if (slam)
            add_msg ("%s slammed against the %s for %d damage!", sname.c_str(), dname.c_str(), dam1);

        if (thru)
        {
            if (is_player)
            {
                p->posx = x;
                p->posy = y;
            }
            else
            {
                zz->posx = x;
                zz->posy = y;
            }
        }
        else
            break;

        range--;
        steps++;


//CAT-mgs:
/*
        timespec ts;   // Timespec for the animation
        ts.tv_sec = 0;
        ts.tv_nsec = BILLION / 40;
        nanosleep (&ts, 0);
*/
    }

    if(!m.has_flag(swimmable, x, y))
    {
        // fall on ground
        dam1 = rng (flvel / 3, flvel * 2 / 3) / 2;
        if (is_player)
        {
            int dex_reduce = p->dex_cur < 4? 4 : p->dex_cur;
            dam1 = dam1 * 8 / dex_reduce;
            if (p->has_trait(PF_PARKOUR))
                dam1 /= 2;
            if (dam1 > 0)
                p->hitall (this, dam1, 40);
        }
        else
	  if(zz->hurt(dam1))
	  {
			int mondex = mon_at(zz->posx, zz->posy);
			kill_mon(mondex, !is_player);
//			add_msg("kill_mon4");
	  }

        if (is_u)
            if (dam1 > 0)
                add_msg ("You fall on the ground for %d damage.", dam1);
            else
                add_msg ("You fall on the ground.");
    }
    else
    if (is_u)
        add_msg ("You fall into water.");
}


void game::vertical_move(int movez, bool force)
{
// > and < are used for diving underwater.
 if (m.has_flag(swimmable, u.posx, u.posy)){
  if (movez < 0) {
   if (u.underwater) {
    add_msg("You are already underwater!");
    return;
   }

   u.underwater = true;
   u.oxygen = 30 + 2 * u.str_cur;
   add_msg("You dive underwater!");

//CAT-s:
	playSound(39);
	stopMusic(1);
	stopLoop(-99);

  } else {


//CAT-s: *** vvv
   if(u.swim_speed() > 500 && !force)
	add_msg("You can't surface!");
   else
   {
	u.underwater = false;
	add_msg("You surface.");
	
	playSound(30);
	stopMusic(1);
   }

  }
  return;
 }
// Force means we're going down, even if there's no staircase, etc.
// This happens with sinkholes and the like.
 if (!force && ((movez == -1 && !m.has_flag(goes_down, u.posx, u.posy)) ||
                (movez ==  1 && !m.has_flag(goes_up,   u.posx, u.posy))   )) {
  add_msg("You can't go %s here!", (movez == -1 ? "down" : "up"));
  return;
 }

 map tmpmap(&itypes, &mapitems, &traps);
 tmpmap.load(this, levx, levy, levz + movez, false);

// Find the corresponding staircase
 int stairx = -1, stairy = -1;
 bool rope_ladder = false;
 if (force) {
  stairx = u.posx;
  stairy = u.posy;
 } else { // We need to find the stairs.
  int best = 999;
   for (int i = u.posx - SEEX *2; i <= u.posx + SEEX *2; i++) {
    for (int j = u.posy - SEEY *2; j <= u.posy + SEEY *2; j++) {

    if(rl_dist(u.posx, u.posy, i, j) <= best &&
        ((movez == -1 && tmpmap.has_flag(goes_up, i, j)) ||
         (movez ==  1 && (tmpmap.has_flag(goes_down, i, j) ||
                          tmpmap.ter(i, j) == t_manhole_cover))))
    {

//CAT: hmmm?
//		add_msg("FOUND STAIRS x: %d, y: %d", i, j);

		stairx = i;
		stairy = j;
		best = rl_dist(u.posx, u.posy, i, j);
    }
   }
  }


  if(stairx == -1 || stairy == -1) { // No stairs found!

//CAT-mgs:
    stairx = u.posx;
    stairy = u.posy;

   if (movez < 0) {

    if (tmpmap.move_cost(u.posx, u.posy) == 0) {
//CAT-s:
	playSound(3);	
      popup("Halfway down, the way down becomes blocked off.");
      return;
    } else if (u.has_amount(itm_rope_30, 1)) {
     if (query_yn("There is a sheer drop halfway down. Climb your rope down?")){
      rope_ladder = true;
      u.use_amount(itm_rope_30, 1);

     } else
      return;
    } else if (!query_yn("There is a sheer drop halfway down. Jump?"))
     return;

   }
   else
   {
//CAT-mgs: just in case
//CAT-s:
	playSound(3);	

//	add_msg("So it's blocked off?");  

     popup("This way up is blocked off.");
     return;

   }
  }

 }


//CAT-mgs:
  bool replace_monsters = false;
// Replace the stair monsters if we just came back
 if(monstairz == movez 
		&& abs(monstairx - levx) <= 1 && abs(monstairy - levy) <= 1)
	replace_monsters = true;

 if(!force) {
  monstairx = levx;
  monstairy = levy;
  monstairz = movez;
  despawn_monsters(true);
 }

 z.clear();

// Figure out where we know there are up/down connectors
 std::vector<point> discover;
 for (int x = 0; x < OMAPX; x++) {
  for (int y = 0; y < OMAPY; y++) {
   if (cur_om.seen(x, y, levz) &&
       ((movez ==  1 && oterlist[ cur_om.ter(x, y, levz) ].known_up) ||
        (movez == -1 && oterlist[ cur_om.ter(x, y, levz) ].known_down) ))
    discover.push_back( point(x, y) );
  }
 }

 int z = levz + movez;
 // Fill in all the tiles we know about (e.g. subway stations)
 for (int i = 0; i < discover.size(); i++) {
  int x = discover[i].x, y = discover[i].y;
  cur_om.seen(x, y, z) = true;
  if (movez ==  1 && !oterlist[ cur_om.ter(x, y, z) ].known_down &&
      !cur_om.has_note(x, y, z))
   cur_om.add_note(x, y, z, "AUTO: goes down");
  if (movez == -1 && !oterlist[ cur_om.ter(x, y, z) ].known_up &&
      !cur_om.has_note(x, y, z))
   cur_om.add_note(x, y, z, "AUTO: goes up");
 }
//CAT-mgs: ^^^


 levz += movez;
 u.moves -= 100;
 m.clear_vehicle_cache();
 m.vehicle_list.clear();

 m.load(this, levx, levy, levz);

//CAT: crashing bug monitor
// add_msg("STAIRS x: %d, y: %d", stairx, stairy);


 u.posx = stairx;
 u.posy = stairy;


 if(rope_ladder)
  m.ter(u.posx, u.posy) = t_rope_up;

 if (m.ter(stairx, stairy) == t_manhole_cover) {
  m.add_item(stairx + rng(-1, 1), stairy + rng(-1, 1),
             itypes[itm_manhole_cover], 0);
  m.ter(stairx, stairy) = t_manhole;
 }


 if(replace_monsters)
  replace_stair_monsters();

//CAT-mgs: check this
  m.spawn_monsters(this);


//CAT-mgs: move NPCs close to stairs
 for(int i = 0; i < active_npc.size(); i++)
 {
   if(active_npc[i].attitude == NPCATT_FOLLOW)
   {	
	int px, py;
      int tries = 0;
	do{
       px = u.posx + rng(-1, 1);
       py = u.posy + rng(-1, 1);
       tries++;
	}while(!is_empty(px, py) && tries < 10);

	active_npc[i].posx= px;
	active_npc[i].posy= py;
	active_npc[i].posz= levz;
   }
 }


 if (force) {	// Basically, we fell.
  if (u.has_trait(PF_WINGS_BIRD))
   add_msg("You flap your wings and flutter down gracefully.");
  else {
   int dam = int((u.str_max / 4) + rng(5, 10)) * rng(1, 3);//The bigger they are
   dam -= rng(u.dodge(this), u.dodge(this) * 3);
   if (dam <= 0)
    add_msg("You fall expertly and take no damage.");
   else {
    add_msg("You fall heavily, taking %d damage.", dam);
    u.hurtall(dam);
   }
  }
 }

 if (m.tr_at(u.posx, u.posy) != tr_null) { // We stepped on a trap!
  trap* tr = traps[m.tr_at(u.posx, u.posy)];
  if (force || !u.avoid_trap(tr)) {
   trapfunc f;
   (f.*(tr->act))(this, u.posx, u.posy);
  }
 }


//CAT-s:
  if(levz < 0)
  {
	if(!mp3Track() && levz == -1)
		stopMusic(7);

	playSound(31);
  }
  else
  if(!mp3Track())
  {
//	stopMusic(1);
	playSound(30);
  }


 set_adjacent_overmaps(true);
 refresh_all();
}


void game::update_map(int &x, int &y)
{
 int group = 0;
 int olevx = 0, olevy = 0;

 int shiftx = 0, shifty = 0;


 while (x < SEEX * int(MAPSIZE / 2)) {
  x += SEEX;
  shiftx--;
 }
 while (x >= SEEX * (1 + int(MAPSIZE / 2))) {
  x -= SEEX;
  shiftx++;
 }
 while (y < SEEY * int(MAPSIZE / 2)) {
  y += SEEY;
  shifty--;
 }
 while (y >= SEEY * (1 + int(MAPSIZE / 2))) {
  y -= SEEY;
  shifty++;
 }


//CAT-mgs:
 if(!shiftx && !shifty)
	return;

 m.shift(this, levx, levy, levz, shiftx, shifty);

 levx += shiftx;
 levy += shifty;

//add_msg("levx: %d, levy: %d", levx, levy);


 if (levx < 0) {
  levx += OMAPX * 2;
  olevx = -1;
 } else if (levx > (OMAPX*2 - 1)) {
  levx -= OMAPX * 2;
  olevx = 1;
 }

 if (levy < 0) {
  levy += OMAPY * 2;
  olevy = -1;
 } else if (levy > (OMAPY*2 - 1)) {
  levy -= OMAPY * 2;
  olevy = 1;
 }


 if(olevx != 0 || olevy != 0)
 {
//CAT-mgs: moved here from below
//	update_overmap_seen();
	add_msg("CROSSING REGION BORDER...");
	write_msg();

	cur_om.save();
	cur_om = overmap(this, cur_om.pos().x + olevx, cur_om.pos().y + olevy);
 }

  despawn_monsters(false, shiftx, shifty);
  set_adjacent_overmaps();

//CAT-mgs: *** vvv
//CAT-mgs: no NPCs
 for(int i = 0; i < active_npc.size(); i++)
 {
	active_npc[i].shift(shiftx, shifty);

/*
	if(active_npc[i].posx < 0 - SEEX * 2 || active_npc[i].posy < 0 - SEEX * 2
		|| active_npc[i].posx > SEEX * (MAPSIZE + 2) || active_npc[i].posy >SEEY * (MAPSIZE + 2) )
	{
	   active_npc[i].mapx = levx + (active_npc[i].posx / SEEX);
	   active_npc[i].mapy = levy + (active_npc[i].posy / SEEY);
	   active_npc[i].posx %= SEEX;
	   active_npc[i].posy %= SEEY;

//CAT-mgs:
	   cur_om.npcs.push_back(active_npc[i]);
	   active_npc[i].dead = true;
	   i--;
	}
*/

 }


//CAT-mgs: *** vvv
/*
//CAT-mgs: no NPCs
// Spawn static NPCs?
 for (int i = 0; i < cur_om.npcs.size(); i++) {

  if (rl_dist(levx + int(MAPSIZE / 2), levy + int(MAPSIZE / 2),
              cur_om.npcs[i].mapx, cur_om.npcs[i].mapy) <=
              int(MAPSIZE / 2) + 1) {

   int dx = cur_om.npcs[i].mapx - levx, dy = cur_om.npcs[i].mapy - levy;

   npc & temp = cur_om.npcs[i];

   if (temp.posx == -1 || temp.posy == -1) {

    temp.posx = SEEX * 2 * (temp.mapx - levx) + rng(0 - SEEX, SEEX);
    temp.posy = SEEY * 2 * (temp.mapy - levy) + rng(0 - SEEY, SEEY);

   } else {

    temp.posx += dx * SEEX;
    temp.posy += dy * SEEY;
   }

   if(temp.marked_for_death)
    temp.die(this, false);
   else
    active_npc.push_back(temp);

   // Remove current and step back one to properly get next.
   cur_om.npcs.erase(cur_om.npcs.begin() + i);
   i--;

  }
 }
*/
//CAT-mgs: ***


 // Static monsters
 m.spawn_monsters(this);

 // Dynamic spawn
 if(turn >= nextspawn)
	spawn_mon(shiftx, shifty);


// Shift scent
 unsigned int newscent[SEEX * MAPSIZE][SEEY * MAPSIZE];
 for (int i = 0; i < SEEX * MAPSIZE; i++) {
  for (int j = 0; j < SEEY * MAPSIZE; j++)
   newscent[i][j] = scent(i + (shiftx * SEEX), j + (shifty * SEEY));
 }
 for (int i = 0; i < SEEX * MAPSIZE; i++) {
  for (int j = 0; j < SEEY * MAPSIZE; j++)
   scent(i, j) = newscent[i][j];
 }

//CAT-mgs: moved above
 update_overmap_seen();

//CAT-g:
// draw_minimap();
}


//CAT-mgs: *** vvv
void game::set_adjacent_overmaps(bool from_scratch)
{

 bool do_h = false, do_v = false, do_d = false;
 int hori_disp = (levx > OMAPX) ? 1 : (levx < 0) ? -1 : 0;
 int vert_disp = (levy > OMAPY) ? 1 : (levy < 0) ? -1 : 0;

 if(hori_disp == 0 && vert_disp == 0)
	return;


 int diag_posx = cur_om.pos().x + hori_disp;
 int diag_posy = cur_om.pos().y + vert_disp;

 if(!om_hori || om_hori->pos().x != diag_posx || om_hori->pos().y != cur_om.pos().y || from_scratch)
 {
  delete om_hori;
  om_hori = new overmap(this, diag_posx, cur_om.pos().y);
 }

 if(!om_vert || om_vert->pos().x != cur_om.pos().x || om_vert->pos().y != diag_posy || from_scratch)
 {
  delete om_vert;
  om_vert = new overmap(this, cur_om.pos().x, diag_posy);
 }

 if(!om_diag || om_diag->pos().x != diag_posx || om_diag->pos().y != diag_posy || from_scratch)
 {
  delete om_diag;
  om_diag = new overmap(this, diag_posx, diag_posy);
 }

}


void game::update_overmap_seen()
{
 int omx = (levx + int(MAPSIZE / 2)) / 2, omy = (levy + int(MAPSIZE / 2)) / 2;
 int dist = u.overmap_sight_range(light_level());
 cur_om.seen(omx, omy, levz) = true; // We can always see where we're standing
 if (dist == 0)
  return; // No need to run the rest!

 bool altered_om_vert = false, altered_om_diag = false, altered_om_hori = false;
 for (int x = omx - dist; x <= omx + dist; x++) {
  for (int y = omy - dist; y <= omy + dist; y++) {

   std::vector<point> line = line_to(omx, omy, x, y, 0);
   int sight_points = dist;
   int cost = 0;
   for (int i = 0; i < line.size() && sight_points >= 0; i++) {
    int lx = line[i].x, ly = line[i].y;
    if (lx >= 0 && lx < OMAPX && ly >= 0 && ly < OMAPY)
     cost = oterlist[cur_om.ter(lx, ly, levz)].see_cost;
    else if ((lx < 0 || lx >= OMAPX) && (ly < 0 || ly >= OMAPY)) {
     if (lx < 0) lx += OMAPX;
     else        lx -= OMAPX;
     if (ly < 0) ly += OMAPY;
     else        ly -= OMAPY;
     cost = oterlist[om_diag->ter(lx, ly, levz)].see_cost;
    } else if (lx < 0 || lx >= OMAPX) {
     if (lx < 0) lx += OMAPX;
     else        lx -= OMAPX;
     cost = oterlist[om_hori->ter(lx, ly, levz)].see_cost;
    } else if (ly < 0 || ly >= OMAPY) {
     if (ly < 0) ly += OMAPY;
     else        ly -= OMAPY;
     cost = oterlist[om_vert->ter(lx, ly, levz)].see_cost;
    }
    sight_points -= cost;
   }


   if (sight_points >= 0)
   {
	    int tmpx = x, tmpy = y;
	    if (tmpx >= 0 && tmpx < OMAPX && tmpy >= 0 && tmpy < OMAPY)
	     cur_om.seen(tmpx, tmpy, levz) = true;
	    else
	    if ((tmpx < 0 || tmpx >= OMAPX) && (tmpy < 0 || tmpy >= OMAPY)) {
	     if (tmpx < 0) tmpx += OMAPX;
	     else          tmpx -= OMAPX;
	     if (tmpy < 0) tmpy += OMAPY;
	     else          tmpy -= OMAPY;
	     om_diag->seen(tmpx, tmpy, levz) = true;
	     altered_om_diag = true;
	    } else if (tmpx < 0 || tmpx >= OMAPX) {
	     if (tmpx < 0) tmpx += OMAPX;
	     else          tmpx -= OMAPX;
	     om_hori->seen(tmpx, tmpy, levz) = true;
	     altered_om_hori = true;
	    } else if (tmpy < 0 || tmpy >= OMAPY) {
	     if (tmpy < 0) tmpy += OMAPY;
	     else          tmpy -= OMAPY;
	     om_vert->seen(tmpx, tmpy, levz) = true;
	     altered_om_vert = true;
	    }
   }

  }
 }


//CAT-mgs:
 if((omx < 3 || omx > OMAPX+3) && (omy < 3 || omy > OMAPY+3)) 
 {
	om_vert->save();
//	add_msg("SAVE_VERT");
 }

 if(omx < 3 || omx >= OMAPX+3) 
 {
	om_hori->save();
//	add_msg("SAVE_HORI");
 }
 
 if(omy < 3 || omy >= OMAPY+3) 
 {
	om_diag->save();
//	add_msg("SAVE_DIAG");
 }

// add_msg("omx: %d, omy: %d", omx,omy);
}



point game::om_location()
{
 point ret;
 ret.x = int( (levx + int(MAPSIZE / 2)) / 2);
 ret.y = int( (levy + int(MAPSIZE / 2)) / 2);
 return ret;
}

void game::replace_stair_monsters()
{
 for (int i = 0; i < coming_to_stairs.size(); i++)
  z.push_back(coming_to_stairs[i].mon);
 coming_to_stairs.clear();
}



//CAT-mgs: *** vvv
void game::update_stair_monsters()
{
 if (abs(levx - monstairx) > 1 || abs(levy - monstairy) > 1)
  return;


 for (int i = 0; i < coming_to_stairs.size(); i++)
 {
  coming_to_stairs[i].count--;

  if (coming_to_stairs[i].count <= 0)
  {

   int close= 999;
   int sx, sy, mposx, mposy;
   bool found_stairs = false;

   int startx = rng(0, SEEX * MAPSIZE - 1);
   int starty = rng(0, SEEY * MAPSIZE - 1);

   for(int x = 0; x < SEEX * MAPSIZE; x++)
   {
    for(int y = 0; y < SEEY * MAPSIZE; y++)
    {
	sx = (startx + x) % (SEEX * MAPSIZE),
	sy = (starty + y) % (SEEY * MAPSIZE);

	if(rl_dist(u.posx, u.posy, sx, sy) < close
		&& ( (monstairz < 0 && m.has_flag(goes_up, sx, sy))
		 || (monstairz > 0 && m.has_flag(goes_down, sx, sy)) ) )
	{
	      mposx = sx;
		mposy = sy;

	      found_stairs = true;
		close= rl_dist(u.posx, u.posy, sx, sy);
	}

    }//for x
   }//for y

   if(found_stairs)
   {
	sx= mposx;
	sy= mposy;

      int tries = 0;
      while(!is_empty(mposx, mposy) && tries < 10)
	{
       mposx = sx + rng(-1, 1);
       mposy = sy + rng(-1, 1);
       tries++;
	}

      if(tries < 9)
	{
       coming_to_stairs[i].mon.posx = sx;
       coming_to_stairs[i].mon.posy = sy;
       z.push_back( coming_to_stairs[i].mon );
       int t;
       if (u_see(sx, sy, t))
        add_msg("A %s comes %s the %s!", coming_to_stairs[i].mon.name().c_str(),
                (m.has_flag(goes_up, sx, sy) ? "down" : "up"),
                m.tername(sx, sy).c_str());
      }
   }

   coming_to_stairs.erase(coming_to_stairs.begin() + i);
   i--;
  }
 }


 if(coming_to_stairs.empty()) {
  monstairx = 0;
  monstairy = 0;
  monstairz = 0;
 }
}

void game::despawn_monsters(const bool stairs, const int shiftx, const int shifty)
{
 for (unsigned int i = 0; i < z.size(); i++) {
  // If either shift argument is non-zero, we're shifting.
  if(shiftx != 0 || shifty != 0) {
   z[i].shift(shiftx, shifty);
   if (z[i].posx >= 0 - SEEX             && z[i].posy >= 0 - SEEX &&
       z[i].posx <= SEEX * (MAPSIZE + 1) && z[i].posy <= SEEY * (MAPSIZE + 1))
     // We're inbounds, so don't despawn after all.
     continue;
  }

  if (stairs && z[i].will_reach(this, u.posx, u.posy)) {
   int turns = z[i].turns_to_reach(this, u.posx, u.posy);
   if (turns < 999)
    coming_to_stairs.push_back( monster_and_count(z[i], 1 + turns) );
  } else if (z[i].spawnmapx != -1) {
   // Static spawn, create a new spawn here.
   z[i].spawnmapx = levx + z[i].posx / SEEX;
   z[i].spawnmapy = levy + z[i].posy / SEEY;
   tinymap tmp(&itypes, &mapitems, &traps);
   tmp.load(this, z[i].spawnmapx, z[i].spawnmapy, levz, false);
   tmp.add_spawn(&(z[i]));
   tmp.save(&cur_om, turn, z[i].spawnmapx, z[i].spawnmapy, levz);
  } else if ((stairs || shiftx != 0 || shifty != 0) && z[i].friendly < 0) {
   // Friendly, make it into a static spawn.
   tinymap tmp(&itypes, &mapitems, &traps);
   tmp.load(this, levx, levy, levz, false);
   tmp.add_spawn(&(z[i]));
   tmp.save(&cur_om, turn, levx, levy, levz);
  } else {
   	// No spawn site, so absorb them back into a group.
   int group = valid_group((mon_id)(z[i].type->id), levx + shiftx, levy + shifty, levz);
   if (group != -1) {
    cur_om.zg[group].population++;
    if (cur_om.zg[group].population / pow(cur_om.zg[group].radius, 2.0) > 5 &&
        !cur_om.zg[group].diffuse)
     cur_om.zg[group].radius++;
   }
  }
  // Shifting needs some cleanup for despawned monsters since they won't be cleared afterwards.
  if(shiftx != 0 || shifty != 0) {
    z.erase(z.begin()+i);
    i--;
  }
 }
}

void game::spawn_mon(int shiftx, int shifty)
{
 int nlevx = levx + shiftx;
 int nlevy = levy + shifty;
 int group;
 int monx, mony;
 int dist;
 int pop, rad;
 int iter;
 int t;

//CAT-mgs: no NPCs
 // Create a new NPC?
 if (!no_npc && one_in(100 + 15 * cur_om.npcs.size())) {
  npc tmp;
  tmp.normalize(this);
  tmp.randomize(this);
  //tmp.stock_missions(this);
  tmp.spawn_at(&cur_om, levx + (1 * rng(-5, 5)), levy + (1 * rng(-5, 5)));
  tmp.posx = SEEX * 2 * (tmp.mapx - levx) + rng(0 - SEEX, SEEX);
  tmp.posy = SEEY * 2 * (tmp.mapy - levy) + rng(0 - SEEY, SEEY);
  tmp.form_opinion(&u);
  tmp.attitude = NPCATT_TALK;
  tmp.mission = NPC_MISSION_NULL;
  int mission_index = reserve_random_mission(ORIGIN_ANY_NPC,
                                             om_location(), tmp.id);
  if (mission_index != -1)
  tmp.chatbin.missions.push_back(mission_index);
  active_npc.push_back(tmp);
 }


// Now, spawn monsters (perhaps)
 monster zom;
 for (int i = 0; i < cur_om.zg.size(); i++) { // For each valid group...
 	if (cur_om.zg[i].posz != levz) { continue; } // skip other levels - hack
  group = 0;
  if(cur_om.zg[i].diffuse)
   dist = rl_dist(nlevx, nlevy, cur_om.zg[i].posx, cur_om.zg[i].posy);
  else
   dist = trig_dist(nlevx, nlevy, cur_om.zg[i].posx, cur_om.zg[i].posy);
  pop = cur_om.zg[i].population;
  rad = cur_om.zg[i].radius;
  if (dist <= rad) {
// (The area of the group's territory) in (population/square at this range)
// chance of adding one monster; cap at the population OR 16
   while ( (cur_om.zg[i].diffuse ?
            long( pop) :
            long((1.0 - double(dist / rad)) * pop) )
	  > rng(0, pow(rad, 2.0)) &&
          rng(0, MAPSIZE * 4) > group && group < pop && group < MAPSIZE * 3)
    group++;

   cur_om.zg[i].population -= group;
   // Reduce group radius proportionally to remaining
   // population to maintain a minimal population density.
   if (cur_om.zg[i].population / pow(cur_om.zg[i].radius, 2.0) < 1.0 &&
       !cur_om.zg[i].diffuse)
     cur_om.zg[i].radius--;

   if (group > 0) // If we spawned some zombies, advance the timer
    nextspawn += rng(group * 4 + z.size() * 4, group * 10 + z.size() * 10);

   for (int j = 0; j < group; j++) {	// For each monster in the group...
     mon_id type = MonsterGroupManager::GetMonsterFromGroup(cur_om.zg[i].type, &mtypes, (int)turn);
     zom = monster(mtypes[type]);
     iter = 0;
     do {
      monx = rng(0, SEEX * MAPSIZE - 1);
      mony = rng(0, SEEY * MAPSIZE - 1);
      if (shiftx == 0 && shifty == 0) {
       if (one_in(2))
        shiftx = 1 - 2 * rng(0, 1);
       else
        shifty = 1 - 2 * rng(0, 1);
      }
      if (shiftx == -1)
       monx = (SEEX * MAPSIZE) / 6;
      else if (shiftx == 1)
       monx = (SEEX * MAPSIZE * 5) / 6;
      if (shifty == -1)
       mony = (SEEY * MAPSIZE) / 6;
      if (shifty == 1)
       mony = (SEEY * MAPSIZE * 5) / 6;
      monx += rng(-5, 5);
      mony += rng(-5, 5);
      iter++;

     } while ((!zom.can_move_to(m, monx, mony) || !is_empty(monx, mony) ||
                m.sees(u.posx, u.posy, monx, mony, SEEX, t) ||
                rl_dist(u.posx, u.posy, monx, mony) < 8) && iter < 50);
     if (iter < 50) {
      zom.spawn(monx, mony);
      z.push_back(zom);
     }
   }	// Placing monsters of this group is done!
   if (cur_om.zg[i].population <= 0) { // Last monster in the group spawned...
    cur_om.zg.erase(cur_om.zg.begin() + i); // ...so remove that group
    i--;	// And don't increment i.
   }
  }
 }
}

int game::valid_group(mon_id type, int x, int y, int z)
{
 std::vector <int> valid_groups;
 std::vector <int> semi_valid;	// Groups that're ALMOST big enough
 int dist;
 for (int i = 0; i < cur_om.zg.size(); i++) {
 	if (cur_om.zg[i].posz != z) { continue; }
  dist = trig_dist(x, y, cur_om.zg[i].posx, cur_om.zg[i].posy);
  if (dist < cur_om.zg[i].radius) {
   if(MonsterGroupManager::IsMonsterInGroup(cur_om.zg[i].type, type)) {
     valid_groups.push_back(i);
   }
  } else if (dist < cur_om.zg[i].radius + 3) {
   if(MonsterGroupManager::IsMonsterInGroup(cur_om.zg[i].type, type)) {
     semi_valid.push_back(i);
   }
  }
 }
 if (valid_groups.size() == 0) {
  if (semi_valid.size() == 0)
   return -1;
  else {
// If there's a group that's ALMOST big enough, expand that group's radius
// by one and absorb into that group.
   int semi = rng(0, semi_valid.size() - 1);
   if (!cur_om.zg[semi_valid[semi]].diffuse)
    cur_om.zg[semi_valid[semi]].radius++;
   return semi_valid[semi];
  }
 }
 return valid_groups[rng(0, valid_groups.size() - 1)];
}

void game::wait()
{
 char ch = menu("Wait for how long?", "5 Minutes", "30 Minutes", "1 hour",
                "2 hours", "3 hours", "6 hours", "Exit", NULL);
 int time;
 if (ch == 7)
  return;
 switch (ch) {
  case 1: time =   5000; break;
  case 2: time =  30000; break;
  case 3: time =  60000; break;
  case 4: time = 120000; break;
  case 5: time = 180000; break;
  case 6: time = 360000; break;
 }
 u.assign_activity(this, ACT_WAIT, time, 0);
 u.moves = 0;
}


bool game::game_quit()
{
 if (uquit == QUIT_MENU)
  return true;
 return false;
}

//CAT-g: *** whole bunch below ***
void game::write_msg()
{
 werase(w_messages);
 int maxlength = 80 - (SEEX * 2 + 10);	// Matches size of w_messages

//CAT-g: separate messages from status text
 int line = 6;

 for (int i = messages.size() - 1; i >= 0 && line < 8; i--) {
  std::string mes = messages[i].message;
  if (messages[i].count > 1) {
   std::stringstream mesSS;
   mesSS << mes << " x " << messages[i].count;
   mes = mesSS.str();
  }

// Split the message into many if we must!
  size_t split;
  while (mes.length() > maxlength && line >= 0) {
   split = mes.find_last_of(' ', maxlength);
   if (split > maxlength)
    split = maxlength;
   nc_color col = c_ltgray;
   if (int(messages[i].turn) >= curmes)
    col = c_yellow;
   else if (int(messages[i].turn) + 5 > curmes)
    col = c_white;
   //mvwprintz(w_messages, line, 0, col, mes.substr(0, split).c_str());
   mvwprintz(w_messages, line, 0, col, mes.substr(split + 1).c_str());
   mes = mes.substr(0, split);
   line--;
   //mes = mes.substr(split + 1);
  }
  if (line >= 0) {
   nc_color col = c_ltgray;
   if (int(messages[i].turn) >= curmes)
    col = c_yellow;
   else if (int(messages[i].turn) + 5 > curmes)
    col = c_white;
   mvwprintz(w_messages, line, 0, col, mes.c_str());
   line--;
  }
 }
 curmes = int(turn);
 wrefresh(w_messages);
}

void game::msg_buffer()
{
 WINDOW *w = newwin(25, 80, (TERMY > 25) ? (TERMY-25)/2 : 0, (TERMX > 80) ? (TERMX-80)/2 : 0);

 int offset = 0;
 InputEvent input;
 do {
  werase(w);
  wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
             LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
  mvwprintz(w, 24, 32, c_red, "Press q to return");

  int line = 1;
  int lasttime = -1;
  int i;
  for (i = 1; i <= 20 && line <= 23 && offset + i <= messages.size(); i++) {
   game_message *mtmp = &(messages[ messages.size() - (offset + i) ]);
   calendar timepassed = turn - mtmp->turn;

   int tp = int(timepassed);
   nc_color col = (tp <=  2 ? c_ltred : (tp <=  7 ? c_white :
                   (tp <= 12 ? c_ltgray : c_dkgray)));

   if (int(timepassed) > lasttime) {
    mvwprintz(w, line, 3, c_ltblue, "%s ago:",
              timepassed.textify_period().c_str());
    line++;
    lasttime = int(timepassed);
   }

   if (line <= 23) { // Print the actual message... we may have to split it
    std::string mes = mtmp->message;
    if (mtmp->count > 1) {
     std::stringstream mesSS;
     mesSS << mes << " x " << mtmp->count;
     mes = mesSS.str();
    }
// Split the message into many if we must!
    size_t split;
    while (mes.length() > 78 && line <= 23) {
     split = mes.find_last_of(' ', 78);
     if (split > 78)
      split = 78;
     mvwprintz(w, line, 1, c_ltgray, mes.substr(0, split).c_str());
     line++;
     mes = mes.substr(split);
    }
    if (line <= 23) {
     mvwprintz(w, line, 1, col, mes.c_str());
     line++;
    }
   } // if (line <= 23)
  } //for (i = 1; i <= 10 && line <= 23 && offset + i <= messages.size(); i++)
  if (offset > 0)
   mvwprintz(w, 24, 27, c_magenta, "^^^");
  if (offset + i < messages.size())
   mvwprintz(w, 24, 51, c_magenta, "vvv");
  wrefresh(w);

//  DebugLog() << __FUNCTION__ << "calling get_input() \n";
  input = get_input();
  int dirx = 0, diry = 0;

  get_direction(dirx, diry, input);
  if (diry == -1 && offset > 0)
   offset--;
  if (diry == 1 && offset < messages.size())
   offset++;

 } while (input != Close && input != Cancel && input != Confirm);

 werase(w);
 delwin(w);
 refresh_all();
}

void game::teleport(player *p)
{
 if (p == NULL)
  p = &u;
 int newx, newy, t, tries = 0;
 bool is_u = (p == &u);

 p->add_disease(DI_TELEGLOW, 300, this);
 do {
  newx = p->posx + rng(0, SEEX * 2) - SEEX;
  newy = p->posy + rng(0, SEEY * 2) - SEEY;
  tries++;
 } while (tries < 15 && !is_empty(newx, newy));
 bool can_see = (is_u || u_see(newx, newy, t));
 std::string You = (is_u ? "You" : p->name);
 if (p->in_vehicle)
   m.unboard_vehicle (this, p->posx, p->posy);
 p->posx = newx;
 p->posy = newy;
 if (tries == 15) {
  if (m.move_cost(newx, newy) == 0) {	// TODO: If we land in water, swim
   if (can_see)
    add_msg("%s teleport%s into the middle of a %s!", You.c_str(),
            (is_u ? "" : "s"), m.tername(newx, newy).c_str());
   p->hurt(this, bp_torso, 0, 500);
  } else if (mon_at(newx, newy) != -1) {
   int i = mon_at(newx, newy);
   if (can_see)
    add_msg("%s teleport%s into the middle of a %s!", You.c_str(),
            (is_u ? "" : "s"), z[i].name().c_str());
   explode_mon(i);
  }
 }

if(is_u)
   update_map(u.posx, u.posy);
}

void game::nuke(int x, int y)
{
	// TODO: nukes hit above surface, not z = 0
 if (x < 0 || y < 0 || x >= OMAPX || y >= OMAPY)
  return;
 int mapx = x * 2, mapy = y * 2;
 map tmpmap(&itypes, &mapitems, &traps);
 tmpmap.load(this, mapx, mapy, 0, false);
 for (int i = 0; i < SEEX * 2; i++) {
  for (int j = 0; j < SEEY * 2; j++) {
   if (!one_in(10))
    tmpmap.ter(i, j) = t_rubble;
   if (one_in(3))
    tmpmap.add_field(NULL, i, j, fd_nuke_gas, 3);
   tmpmap.radiation(i, j) += rng(20, 80);
  }
 }
 tmpmap.save(&cur_om, turn, mapx, mapy, 0);
 cur_om.ter(x, y, 0) = ot_crater;
}

std::vector<faction *> game::factions_at(int x, int y)
{
 std::vector<faction *> ret;
 for (int i = 0; i < factions.size(); i++) {
  if (factions[i].omx == cur_om.pos().x && factions[i].omy == cur_om.pos().y &&
      trig_dist(x, y, factions[i].mapx, factions[i].mapy) <= factions[i].size)
   ret.push_back(&(factions[i]));
 }
 return ret;
}

nc_color sev(int a)
{
 switch (a) {
  case 0: return c_cyan;
  case 1: return c_blue;
  case 2: return c_green;
  case 3: return c_yellow;
  case 4: return c_ltred;
  case 5: return c_red;
  case 6: return c_magenta;
 }
 return c_dkgray;
}

void game::display_scent()
{
//CAT-mgs:
// int div = 1 + query_int("Sensitivity");

 draw_ter();

 int div= 5;
 for (int x = u.posx - getmaxx(w_terrain)/2; x <= u.posx + getmaxx(w_terrain)/2; x++) {
  for (int y = u.posy - getmaxy(w_terrain)/2; y <= u.posy + getmaxy(w_terrain)/2; y++) {
   int sn = scent(x, y) / (div * 2);
   mvwprintz(w_terrain, getmaxy(w_terrain)/2 + y - u.posy, getmaxx(w_terrain)/2 + x - u.posx, sev(sn), "%d",
             sn % 10);
  }
 }
 wrefresh(w_terrain);
 getch();
}

void game::init_autosave()
{
 moves_since_last_save = 0;
 item_exchanges_since_save = 0;
}

int game::autosave_timeout()
{
 if (!OPTIONS[OPT_AUTOSAVE])
  return -1; // -1 means block instead of timeout

 const double upper_limit = 60 * 1000;
 const double lower_limit = 5 * 1000;
 const double range = upper_limit - lower_limit;

 // Items exchanged
 const double max_changes = 20.0;
 const double max_moves = 500.0;

 double move_multiplier = 0.0;
 double changes_multiplier = 0.0;

 if( moves_since_last_save < max_moves )
  move_multiplier = 1 - (moves_since_last_save / max_moves);

 if( item_exchanges_since_save < max_changes )
  changes_multiplier = 1 - (item_exchanges_since_save / max_changes);

 double ret = lower_limit + (range * move_multiplier * changes_multiplier);
 return ret;
}

void game::autosave()
{
  if (u.in_vehicle || !moves_since_last_save && !item_exchanges_since_save)
    return;
  add_msg("Saving game, this may take a while");
  save();

  moves_since_last_save = 0;
  item_exchanges_since_save = 0;
}

void intro()
{
 int maxx, maxy;
 getmaxyx(stdscr, maxy, maxx);
 WINDOW* tmp = newwin(25, 80, 0, 0);
 while (maxy < 25 || maxx < 80) {
  werase(tmp);
  wprintw(tmp, "\
Whoa. Whoa. Hey. This game requires a minimum terminal size of 80x25. I'm\n\
sorry if your graphical terminal emulator went with the woefully-diminuitive\n\
80x24 as its default size, but that just won't work here.  Now stretch the\n\
bottom of your window downward so you get an extra line.\n");
  wrefresh(tmp);
  refresh();
  wrefresh(tmp);
  getch();
  getmaxyx(stdscr, maxy, maxx);
 }
 werase(tmp);
 wrefresh(tmp);
 delwin(tmp);
 erase();
}
