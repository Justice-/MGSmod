#include <vector>
#include <string>
#include "game.h"
#include "keypress.h"
#include "output.h"
#include "line.h"
#include "skill.h"
#include "rng.h"
#include "item.h"
#include "options.h"


int time_to_fire(player &p, it_gun* firing);
int recoil_add(player &p);
void make_gun_sound_effect(game *g, player &p, bool burst, item* weapon);
int calculate_range(player &p, int tarx, int tary);
double calculate_missed_by(player &p, int trange, item* weapon);
void shoot_monster(game *g, player &p, monster &mon, int &dam, double goodhit, item* weapon);
void shoot_player(game *g, player &p, player *h, int &dam, double goodhit);

void splatter(game *g, std::vector<point> trajectory, int dam,
              monster* mon = NULL);

void ammo_effects(game *g, int x, int y, long flags);

//CAT-g: BEGIN ***** whole lot ***
void game::fire(player &p, int tarx, int tary, std::vector<point> &trajectory,
                bool burst)
{
 item ammotmp;
 item* gunmod = p.weapon.active_gunmod();
 it_ammo *curammo = NULL;
 item *weapon = NULL;


 if (p.weapon.has_flag(IF_CHARGE)) { // It's a charger gun, so make up a type
// Charges maxes out at 8.
  int charges = p.weapon.num_charges();
  it_ammo *tmpammo = dynamic_cast<it_ammo*>(itypes[itm_charge_shot]);

  tmpammo->damage = charges * charges;
  tmpammo->pierce = (charges >= 4 ? (charges - 3) * 2.5 : 0);
  tmpammo->range = 5 + charges * 5;
  if (charges <= 4)
   tmpammo->accuracy = 14 - charges * 2;
  else // 5, 12, 21, 32
   tmpammo->accuracy = charges * (charges - 4);
  tmpammo->recoil = tmpammo->accuracy * .8;
  tmpammo->ammo_effects = 0;
  if (charges >= 8)
   tmpammo->ammo_effects |= mfb(AMMO_EXPLOSIVE_BIG);
  else if (charges >= 6)
   tmpammo->ammo_effects |= mfb(AMMO_EXPLOSIVE);
  if (charges >= 5)
   tmpammo->ammo_effects |= mfb(AMMO_FLAME);
  else if (charges >= 4)
   tmpammo->ammo_effects |= mfb(AMMO_INCENDIARY);

  if (gunmod != NULL) {
   weapon = gunmod;
  } else {
   weapon = &p.weapon;
  }
  curammo = tmpammo;
  weapon->curammo = tmpammo;
  weapon->active = false;
  weapon->charges = 0;
 } else if (gunmod != NULL) {
  weapon = gunmod;
  curammo = weapon->curammo;
 } else {// Just a normal gun. If we're here, we know curammo is valid.
  curammo = p.weapon.curammo;
  weapon = &p.weapon;
 }

 ammotmp = item(curammo, 0);
 ammotmp.charges = 1;

 if (!weapon->is_gun() && !weapon->is_gunmod()) 
	return;

 bool is_bolt = false;
 unsigned int effects = curammo->ammo_effects;
// Bolts and arrows are silent
 if (curammo->type == AT_BOLT || curammo->type == AT_ARROW)
  is_bolt = true;

 int x = p.posx, y = p.posy;
 // Have to use the gun, gunmods don't have a type
 it_gun* firing = dynamic_cast<it_gun*>(p.weapon.type);
 if (p.has_trait(PF_TRIGGERHAPPY) && one_in(30))
  burst = true;
 if (burst && weapon->burst_size() < 2)
  burst = false; // Can't burst fire a semi-auto

 int junk = 0;
 bool u_see_shooter = u_see(p.posx, p.posy, junk);
// Use different amounts of time depending on the type of gun and our skill
 p.moves -= time_to_fire(p, firing);
// Decide how many shots to fire
 int num_shots = 1;
 if (burst)
  num_shots = weapon->burst_size();
 if (num_shots > weapon->num_charges() && !weapon->has_flag(IF_CHARGE))
  num_shots = weapon->num_charges();


// Set up a timespec for use in the nanosleep function below
 timespec ts;
 ts.tv_sec = 0;


 // Use up some ammunition
 int trange = trig_dist(p.posx, p.posy, tarx, tary);
 if (trange < int(firing->volume / 3) && firing->ammo != AT_SHOT)
  trange = int(firing->volume / 3);
 else if (p.has_bionic(bio_targeting)) {
  if (trange > LONG_RANGE)
   trange = int(trange * .65);
  else
   trange = int(trange * .8);
 }
 if (firing->skill_used == Skill::skill("rifle") && trange > LONG_RANGE)
  trange = LONG_RANGE + .6 * (trange - LONG_RANGE);
 std::string message = "";



 int tart;
 bool missed = false;

//CAT-mgs: *** vvv
 for(int curshot = 0; curshot < num_shots; curshot++)
 {

  // Drop a shell casing if appropriate.
  itype_id casing_type = itm_null;

  switch(curammo->type) {
  case AT_SHOT: casing_type = itm_shot_hull; break;
  case AT_9MM: casing_type = itm_9mm_casing; break;
  case AT_38: casing_type = itm_38_casing; break;
  case AT_40: casing_type = itm_40_casing; break;
  case AT_44: casing_type = itm_44_casing; break;
  case AT_45: casing_type = itm_45_casing; break;
  case AT_57: casing_type = itm_57mm_casing; break;
  case AT_46: casing_type = itm_46mm_casing; break;
  case AT_762: casing_type = itm_762_casing; break;
  case AT_223: casing_type = itm_223_casing; break;
  case AT_3006: casing_type = itm_3006_casing; break;
  case AT_308: casing_type = itm_308_casing; break;
  case AT_40MM: casing_type = itm_40mm_casing; break;
  default: 
	break;
  }

  if(casing_type != itm_null) 
  {

//CAT-s: shellDrop sound
	if(one_in(2))
		playSound(41);

   int x = p.posx - 1 + rng(0, 2);
   int y = p.posy - 1 + rng(0, 2);
   std::vector<item>& items = m.i_at(x, y);
   int i;
   for (i = 0; i < items.size(); i++)
    if (items[i].typeId() == casing_type &&
        items[i].charges < (dynamic_cast<it_ammo*>(items[i].type))->count) {
     items[i].charges++;
     break;
    }

   if (i == items.size()) {
    item casing;
    casing.make(itypes[casing_type]);
    // Casing needs a charges of 1 to stack properly with other casings.
    casing.charges = 1;
    m.add_item(x, y, casing);

   }
  }


  // Use up a round (or 100)
  if (weapon->has_flag(IF_FIRE_100))
   weapon->charges -= 100;
  else
   weapon->charges--;

  // Current guns have a durability between 5 and 9.
  // Misfire chance is between 1/64 and 1/1024.
  if (one_in(2 << firing->durability)) {
   add_msg("Your weapon misfired!");
   return;
  }

//CAT:
  make_gun_sound_effect(this, p, burst, weapon);


  int trange = calculate_range(p, tarx, tary);
  double missed_by = calculate_missed_by(p, trange, weapon);


// Calculate a penalty based on the monster's speed
  double monster_speed_penalty = 1.0;
  int target_index = mon_at(tarx, tary);
  if (target_index != -1) {
   monster_speed_penalty = double(z[target_index].speed) / 80.0;
   if (monster_speed_penalty < 1.)
    monster_speed_penalty = 1.0;
  }


//CAT-mgs: *** vvv
	if( curshot > 0 ) 
	{
		if(rl_dist(u.posx, u.posy, tarx, tary) > 1)
		{
			do
			{
				tarx+= rng(-1,1);
				tary+= rng(-1,1);

			}while(tarx == p.posx && tary == p.posy);
		}

		if (m.sees(p.posx, p.posy, tarx, tary, 0, tart))
			trajectory = line_to(p.posx, p.posy, tarx, tary, tart);
		else
			trajectory = line_to(p.posx, p.posy, tarx, tary, 0);


		if(recoil_add(p) % 2 == 1)
			p.recoil++;

		p.recoil += recoil_add(p) / 2;
	}
	else
		p.recoil += recoil_add(p);



//CAT-mgs: *** vvv
  if (missed_by >= 1.0) {
// We missed D:

   missed = true;
   if (!burst) {
    if (&p == &u)
     add_msg("You miss!");
    else if (u_see_shooter)
     add_msg("%s misses!", p.name.c_str());
   }
  } else if (missed_by >= 0.7 / monster_speed_penalty) {
// Hit the space, but not necessarily the monster there
   missed = true;
   if (!burst) {
    if (&p == &u)
     add_msg("You barely miss!");
    else if (u_see_shooter)
     add_msg("%s barely misses!", p.name.c_str());
   }
  }


//CAT-mgs: trajectory *** vvv
  int dam= weapon->gun_damage();
  for( int i= 0; i < trajectory.size() &&
       ( dam > 0 || (effects & (AMMO_FLAME | AMMO_TRAIL)) ); i++ )
  {


	// Drawing the bullet uses player u, and not player p, because it's drawn
	// relative to YOUR position, which may not be the gunman's position.
	if(u_see(trajectory[i].x, trajectory[i].y, junk))
	{
//CAT:
		char bullet = '.';
//		if(i%2 == 0)
//			bullet = '*';

		nc_color cat_col= c_ltred;
		if(curammo->type == AT_FUSION)
		{
			bullet = '*';
			cat_col= c_yellow;	
		}
		else
		if(curammo->type == AT_PLASMA)
		{
			bullet = 'o';
			cat_col= c_magenta;	
		}
		else
		if(effects & mfb(AMMO_TRAIL))
		{
			bullet = '8';	
			cat_col= c_white;	
		}
		else
		if(effects & mfb(AMMO_FLAME))
		{
			bullet = '#';

			if(i > 2)
				m.add_field(this, trajectory[i].x, 
					trajectory[i].y, fd_fire, rng(1,7));
		}


		if(i >= trajectory.size()-2)
		{
			bullet = '*';
			cat_col= c_red;
		}

//CAT:	
		mvwputch(w_terrain, trajectory[i].y + VIEWY-u.view_offset_y - u.posy,
                        trajectory[i].x + VIEWX-u.view_offset_x - u.posx, cat_col, bullet);

		if(effects & mfb(AMMO_TRAIL))
			mvwputch(w_terrain, trajectory[i].y + VIEWY-u.view_offset_y - u.posy +rng(-1,1),
                        trajectory[i].x + VIEWX-u.view_offset_x - u.posx +rng(-1,1), c_ltgray, '8');

	}


	if (dam <= 0)
	{
	// Ran out of momentum.
	    ammo_effects(this, trajectory[i].x, trajectory[i].y, effects);

	    if (is_bolt &&
      	  ((curammo->m1 == WOOD && !one_in(4)) ||
	         (curammo->m1 != WOOD && !one_in(15))))
	     m.add_item(trajectory[i].x, trajectory[i].y, ammotmp);

	    if (weapon->num_charges() == 0)
		weapon->curammo = NULL;

	    return;
	}



	int tx = trajectory[i].x, ty = trajectory[i].y;

	// If there's a monster in the path of our bullet, and either our aim was true,
	//  OR it's not the monster we were aiming at and we were lucky enough to hit it
	int mondex = mon_at(tx, ty);

	if(mondex != -1)
	{
	    double goodhit = missed_by;


//CAT-mgs: more precision
	    if(z[mondex].type->size > 0 && i < trajectory.size() - 1) // Unintentional hit
	    {
	        goodhit = 0.5 + (int)(missed_by / z[mondex].type->size);

//add_msg("casulty: %f, i: %d, tr_size: %d", goodhit, i, trajectory.size() - 1 );
	    }

	    std::vector<point> blood_traj = trajectory;
	    blood_traj.insert(blood_traj.begin(), point(p.posx, p.posy));
	    splatter(this, blood_traj, dam, &z[mondex]);
	    shoot_monster(this, p, z[mondex], dam, goodhit, weapon);

	}
	else
//CAT-mgs: what the?
//	if( (!missed || one_in(3)) &&
	if( !missed 
		&& (npc_at(tx, ty) != -1 || (u.posx == tx && u.posy == ty)) )
	{
	    double goodhit = missed_by;
	    if(i < trajectory.size() - 1) // Unintentional hit
		goodhit = double(rand() / (RAND_MAX + 1.0)); //CAT: / 2;


	    player *h;
	    bool weTarget= false;
	    if (u.posx == tx && u.posy == ty)
	    {
		h = &u;
		weTarget= true;
	    }
	    else
		h = &(active_npc[npc_at(tx, ty)]);

//CAT-mgs: don't let our turrets (moves= -99) shoot us
	    if(!(weTarget && p.moves < -98)) 
	    {
		   std::vector<point> blood_traj = trajectory;
		   blood_traj.insert(blood_traj.begin(), point(p.posx, p.posy));
		   splatter(this, blood_traj, dam);
		   shoot_player(this, p, h, dam, goodhit);
	    }
	}
	else
	    m.shoot(this, tx, ty, dam, i == trajectory.size() - 1, effects);

//CAT: 
	if(&p == &u)
		ts.tv_nsec = (BULLET_SPEED*100)/(trajectory.size()*9);
	else  
		ts.tv_nsec = BULLET_SPEED/(trajectory.size()*9); //BULLET_SPEED / 100;

	wrefresh(w_terrain);
	nanosleep(&ts, NULL);

  } // Done with the trajectory!



  int lastx = trajectory[trajectory.size() - 1].x;
  int lasty = trajectory[trajectory.size() - 1].y;
  ammo_effects(this, lastx, lasty, effects);

  if (m.move_cost(lastx, lasty) == 0) {
   lastx = trajectory[trajectory.size() - 2].x;
   lasty = trajectory[trajectory.size() - 2].y;
  }
  if (is_bolt &&
      ((curammo->m1 == WOOD && !one_in(5)) ||
       (curammo->m1 != WOOD && !one_in(15))  ))
    m.add_item(lastx, lasty, ammotmp);

 }

 if (weapon->num_charges() == 0)
  weapon->curammo = NULL;
}
//CAT-g: END *****




void game::throw_item(player &p, int tarx, int tary, item &thrown,
                      std::vector<point> &trajectory)
{

//CAT:
	playSound(73);

 int deviation = 0;
 int trange = 1.5 * rl_dist(p.posx, p.posy, tarx, tary);

// Throwing attempts below "Basic Competency" level are extra-bad
 int skillLevel = p.skillLevel("throw");

 if (skillLevel < 3)
  deviation += rng(0, 8 - skillLevel);

 if (skillLevel < 8)
  deviation += rng(0, 8 - skillLevel);
 else
  deviation -= skillLevel - 6;

 deviation += p.throw_dex_mod();

 if (p.per_cur < 6)
  deviation += rng(0, 8 - p.per_cur);
 else if (p.per_cur > 8)
  deviation -= p.per_cur - 8;

 deviation += rng(0, p.encumb(bp_hands) * 2 + p.encumb(bp_eyes) + 1);
 if (thrown.volume() > 5)
  deviation += rng(0, 1 + (thrown.volume() - 5) / 4);
 if (thrown.volume() == 0)
  deviation += rng(0, 3);

 deviation += rng(0, 1 + abs(p.str_cur - thrown.weight()));

 double missed_by = .01 * deviation * trange;
 bool missed = false;
 int tart;

 if (missed_by >= 1) {
// We missed D:
// Shoot a random nearby space?
  if (missed_by > 9)
   missed_by = 9;
  tarx += rng(0 - int(sqrt(double(missed_by))), int(sqrt(double(missed_by))));
  tary += rng(0 - int(sqrt(double(missed_by))), int(sqrt(double(missed_by))));
  if (m.sees(p.posx, p.posy, tarx, tary, -1, tart))
   trajectory = line_to(p.posx, p.posy, tarx, tary, tart);
  else
   trajectory = line_to(p.posx, p.posy, tarx, tary, 0);
  missed = true;
  if (!p.is_npc())
   add_msg("You miss!");
 } else if (missed_by >= .6) {
// Hit the space, but not necessarily the monster there
  missed = true;
  if (!p.is_npc())
   add_msg("You barely miss!");
 }

 std::string message;
 int dam = (thrown.weight() / 4 + thrown.type->melee_dam / 2 + p.str_cur / 2) /
            double(2 + double(thrown.volume() / 4));
 if (dam > thrown.weight() * 3)
  dam = thrown.weight() * 3;

 int i = 0, tx = 0, ty = 0;
 for (i = 0; i < trajectory.size() && dam > -10; i++) {
  message = "";
  double goodhit = missed_by;
  tx = trajectory[i].x;
  ty = trajectory[i].y;
// If there's a monster in the path of our item, and either our aim was true,
//  OR it's not the monster we were aiming at and we were lucky enough to hit it
  if (mon_at(tx, ty) != -1 &&
      (!missed || one_in(7 - int(z[mon_at(tx, ty)].type->size)))) {
   if (rng(0, 100) < 20 + skillLevel * 12 &&
       thrown.type->melee_cut > 0) {
    if (!p.is_npc()) {
     message += " You cut the ";
     message += z[mon_at(tx, ty)].name();
     message += "!";

//CAT-s:
	playSound(92);	//noiseSwoosh sound
    }
    if (thrown.type->melee_cut > z[mon_at(tx, ty)].armor_cut())
     dam += (thrown.type->melee_cut - z[mon_at(tx, ty)].armor_cut());
   }


   if (thrown.made_of(GLASS) && !thrown.active && // active = molotov, etc.
       rng(0, thrown.volume() + 8) - rng(0, p.str_cur) < thrown.volume()) {
    if (u_see(tx, ty, tart))
     add_msg("The %s shatters!", thrown.tname().c_str());
    for (int i = 0; i < thrown.contents.size(); i++)
     m.add_item(tx, ty, thrown.contents[i]);
    sound(tx, ty, 17, "glass breaking!");
    int glassdam = rng(0, thrown.volume() * 2);
    if (glassdam > z[mon_at(tx, ty)].armor_cut())
     dam += (glassdam - z[mon_at(tx, ty)].armor_cut());
   }else
    m.add_item(tx, ty, thrown);

   if (i < trajectory.size() - 1)
    goodhit = double(double(rand() / RAND_MAX) / 2);
   if (goodhit < .1 && !z[mon_at(tx, ty)].has_flag(MF_NOHEAD)) {
    message = "Headshot!";
    dam = rng(dam, dam * 3);
    p.practice("throw", 5);
   } else if (goodhit < .2) {
    message = "Critical!";
    dam = rng(dam, dam * 2);
    p.practice("throw", 2);
   } else if (goodhit < .4)
    dam = rng(int(dam / 2), int(dam * 1.5));
   else if (goodhit < .5) {
    message = "Grazing hit.";
    dam = rng(0, dam);
   }
   if (!p.is_npc())
    add_msg("%s You hit the %s for %d damage.",
            message.c_str(), z[mon_at(tx, ty)].name().c_str(), dam);
   else if (u_see(tx, ty, tart))
    add_msg("%s hits the %s for %d damage.", message.c_str(),
            z[mon_at(tx, ty)].name().c_str(), dam);
   if (z[mon_at(tx, ty)].hurt(dam))
    kill_mon(mon_at(tx, ty), !p.is_npc());
   return;
  } else // No monster hit, but the terrain might be.
   m.shoot(this, tx, ty, dam, false, 0);
  if (m.move_cost(tx, ty) == 0) {
   if (i > 0) {
    tx = trajectory[i - 1].x;
    ty = trajectory[i - 1].y;
   } else {
    tx = u.posx;
    ty = u.posy;
   }
   i = trajectory.size();
  }
 }

 if (m.move_cost(tx, ty) == 0) {
  if (i > 1) {
   tx = trajectory[i - 2].x;
   ty = trajectory[i - 2].y;
  } else {
   tx = u.posx;
   ty = u.posy;
  }
 }


//CAT-mgs: 
 if(m.ter(tx,ty) == t_air)
	return;


 if (thrown.made_of(GLASS) && !thrown.active && // active means molotov, etc
     rng(0, thrown.volume() + 8) - rng(0, p.str_cur) < thrown.volume()) {
  if (u_see(tx, ty, tart))
   add_msg("The %s shatters!", thrown.tname().c_str());
  for (int i = 0; i < thrown.contents.size(); i++)
   m.add_item(tx, ty, thrown.contents[i]);
  sound(tx, ty, 17, "glass breaking!");

//CAT-s:
	playSound(25);

 } else {
  sound(tx, ty, 8, "thud.");
  m.add_item(tx, ty, thrown);

//CAT-s:
	playSound(91);
 }
}




//CAT-mgs: BEGIN ***** whole lot ***
std::vector<point> game::target(int &x, int &y, int lowx, int lowy, int hix,
                                int hiy, std::vector <monster> t, int &target,
                                item *relevent)
{

 std::vector<point> ret;
 int tarx, tary, tart, junk;

 // TODO: [lightmap] Enable auto targeting based on lightmap
 int sight_dist = u.sight_range(light_level());

//CAT-mgs:
 target = -1;
 x= ltar_x;
 y= ltar_y;

 WINDOW* w_target = newwin(12, 48, 0, TERRAIN_WINDOW_WIDTH + 7 + VIEW_OFFSET_X);

 if(!relevent) // currently targetting vehicle to refill with fuel
	mvwprintz(w_target, 1, 1, c_red, "Select a vehicle");
 else
 if(relevent == &u.weapon && relevent->is_gun())
	mvwprintz(w_target, 1, 1, c_red, "Firing %s (%d)", u.weapon.tname().c_str(), u.weapon.charges);
 else
	mvwprintz(w_target, 1, 1, c_red, "Throwing %s", relevent->tname().c_str());

 if(relevent)
 {
	mvwprintz(w_target, 2, 1, c_white, "ENTER or 'f' to fire,'z' to kick, ESC to exit.");
	mvwprintz(w_target, 3, 1, c_white, "SPACE or '.' to pause and take a kneeling shot.");
 }
 else
	 mvwprintz(w_target, 2, 1, c_white, "Move the aim with directional keys.");



 bool night_vision= false;
 if( u.has_active_bionic(bio_night_vision)
	|| ( u.is_wearing(itm_goggles_nv) 
	&& u.has_active_item(itm_UPS_on) ) )
    night_vision= true;


 int ch;
 point center;

 int cat_x= x-u.posx; 
 int cat_y= y-u.posy; 

 int dist= (int)sqrt(cat_x*cat_x + cat_y*cat_y);

 if(dist > 9)
 {
	cat_x= 0;
	cat_y= 0;
 }

 do
 {
	// Clear the target window.
	for(int i = 5; i < 12; i++)
	{
	   for (int j = 1; j < 46; j++)
		mvwputch(w_target, i, j, c_white, ' ');
	}

	center = point(x-cat_x, y-cat_y);

	u.view_offset_x= x-u.posx-cat_x;
	u.view_offset_y= y-u.posy-cat_y;

	cleanup_dead();

	m.vehmove(this);
	m.process_fields(this);
	m.process_active_items(this);
	m.step_in_field(u.posx, u.posy, this);

	u.reset(this);
	u.suffer(this);

	draw_HP();
	write_msg();
	wborder(w_target, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                 LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );	
	u.disp_status(w_status, this);

	lm.generate(this, center.x, center.y, natural_light_level(), u.active_light());
	m.draw(this, w_terrain, center);


//CAT: sniper scope view
	if(SNIPER)
	{
		nc_color cat_c= c_ltgray;
		if(u.weapon.typeId() == itm_remington_700)
		{
		   cat_c= c_ltgreen;
		   mvwputch(w_terrain, VIEWY+2+cat_y, VIEWX+cat_x, cat_c, '|');
		   mvwputch(w_terrain, VIEWY+3+cat_y, VIEWX+cat_x, cat_c, '|');
//		   mvwputch(w_terrain, VIEWY-2+cat_y, VIEWX+cat_x, cat_c, '|');
//		   mvwputch(w_terrain, VIEWY-3+cat_y, VIEWX+cat_x, cat_c, '|');

		   mvwputch(w_terrain, VIEWY+cat_y, VIEWX+2+cat_x, cat_c, '-');
		   mvwputch(w_terrain, VIEWY+cat_y, VIEWX+3+cat_x, cat_c, '-');
		   mvwputch(w_terrain, VIEWY+cat_y, VIEWX-2+cat_x, cat_c, '-');
		   mvwputch(w_terrain, VIEWY+cat_y, VIEWX-3+cat_x, cat_c, '-');
		}
		else
		if(u.weapon.typeId() == itm_browning_blr)
		{
		   cat_c= c_cyan;
		   mvwputch(w_terrain, VIEWY+2+cat_y, VIEWX+cat_x, cat_c, '|');
		   mvwputch(w_terrain, VIEWY+3+cat_y, VIEWX+cat_x, cat_c, '|');
		   mvwputch(w_terrain, VIEWY-2+cat_y, VIEWX+cat_x, cat_c, '|');
		   mvwputch(w_terrain, VIEWY-3+cat_y, VIEWX+cat_x, cat_c, '|');

		   mvwputch(w_terrain, VIEWY+cat_y, VIEWX+2+cat_x, cat_c, '-');
		   mvwputch(w_terrain, VIEWY+cat_y, VIEWX+3+cat_x, cat_c, '-');
		   mvwputch(w_terrain, VIEWY+cat_y, VIEWX-2+cat_x, cat_c, '-');
		   mvwputch(w_terrain, VIEWY+cat_y, VIEWX-3+cat_x, cat_c, '-');
		}
		else
		{
		   mvwputch(w_terrain, VIEWY+4+cat_y, VIEWX+cat_x, cat_c, '|');
		   mvwputch(w_terrain, VIEWY+3+cat_y, VIEWX+cat_x, cat_c, '|');
		   mvwputch(w_terrain, VIEWY-4+cat_y, VIEWX+cat_x, cat_c, '|');
		   mvwputch(w_terrain, VIEWY-3+cat_y, VIEWX+cat_x, cat_c, '|');

		   mvwputch(w_terrain, VIEWY+cat_y, VIEWX+4+cat_x, cat_c, '-');
		   mvwputch(w_terrain, VIEWY+cat_y, VIEWX+3+cat_x, cat_c, '-');
		   mvwputch(w_terrain, VIEWY+cat_y, VIEWX-4+cat_x, cat_c, '-');
		   mvwputch(w_terrain, VIEWY+cat_y, VIEWX-3+cat_x, cat_c, '-');
		}
	}



	int snip_x= u.posx - x;
	int snip_y= u.posy - y;
	dist= (int)sqrt(snip_x*snip_x + snip_y*snip_y);

	// Draw the Monsters
	for(int i = 0; i < z.size(); i++)
	{
		int s_x= z[i].posx - x;
		int s_y= z[i].posy - y;
		int s_d= (int)sqrt(s_x*s_x + s_y*s_y);

		if(!SNIPER || (SNIPER && s_d < 4))
		{
			if(u_see(&(z[i]), tart))
			    z[i].draw(w_terrain, center.x, center.y, false, night_vision);
		}
	}

	// Draw the NPCs
	for(int i = 0; i < active_npc.size(); i++)
	{
		int s_x= active_npc[i].posx - x;
		int s_y= active_npc[i].posy - y;
		int s_d= (int)sqrt(s_x*s_x + s_y*s_y);

		if(!SNIPER || (SNIPER && s_d < 4))
		{
			if(active_npc[i].posz == levz 
				&& u_see(active_npc[i].posx, active_npc[i].posy, tart))
			    active_npc[i].draw(w_terrain, center.x, center.y, false, night_vision);
		}
	}


	if(m.sees(u.posx, u.posy, x, y, -1, tart))
	{
	   ret= line_to(u.posx, u.posy, x, y, tart); 


	   // Draw the trajectory
	   for(int i= 0; i < ret.size(); i++) 
	   {

		if(SNIPER)
			i= ret.size()-1;

		int mondex = mon_at(ret[i].x, ret[i].y);
		int npcdex = npc_at(ret[i].x, ret[i].y);

		// NPCs and monsters get drawn with inverted colors
		if(mondex >=0 && u_see(&(z[mondex]), tart))
		   z[mondex].draw(w_terrain, 
			center.x, center.y, true, night_vision);
		else
		if(npcdex >= 0)
		   active_npc[npcdex].draw(w_terrain, 
			center.x, center.y, true, night_vision);
		else
		if(!SNIPER)
		{
			int atx = VIEWX + ret[i].x - center.x;
			int aty = VIEWY + ret[i].y - center.y;

			mvwputch(w_terrain, aty, atx, c_ltred, 'x');
		}
	   }
	}


	if(!relevent)
	{ 
		// currently targetting vehicle to refill with fuel
		vehicle *veh = m.veh_at(x, y);

		if (veh)
		    mvwprintw(w_target, 5, 1, "There is a %s", veh->name.c_str());
	}
	else
	   mvwprintw(w_target, 4, 1, "Range: %d (Recoil: %d)         ", dist, u.recoil);


//CAT: 
	if(!SNIPER && mon_at(x, y) < 0)
		mvwputch(w_terrain, VIEWY+cat_y, VIEWX+cat_x, c_red, 'X');
	else
	if(mon_at(x, y) >= 0 && u_see(&(z[mon_at(x, y)]), tart))
		z[mon_at(x, y)].print_info(this, w_target);



	wrefresh(w_status);
	wrefresh(w_target);
	wrefresh(w_terrain);

	if(is_game_over())
	{
		ret.clear();
		return ret;
	}

	ch = input();
	get_direction(this, tarx, tary, ch);
	if (tarx != -2 && tary != -2 && ch != '.' && ch != ' ' && ch != '5') 
	{
		// Direction character pressed
		x += tarx;
		y += tary;

		if(x < lowx)
			x = lowx;
		else 
		if(x > hix)
			x = hix;
		if(y < lowy)
			y = lowy;
		else
		if(y > hiy)
			y = hiy;

//CAT:
//		int cat_r= 1+rl_dist(u.posx, u.posy, x, y)/SEEX;

		int cat_r= 1;
		if(dist > 9)
			cat_r= 0;

		cat_x= (int)(x-u.posx) * cat_r;
		cat_y= (int)(y-u.posy) * cat_r;

		ltar_x= x;
		ltar_y= y;

	}
	else
	if(ch == KEY_ESCAPE || ch == 'q')
	{
		ltar_x= u.posx;
		ltar_y= u.posy;
		SNIPER= false;

//CAT-s:
		playSound(0);

		ret.clear();
		return ret;
	}
	else
	if(ch == 'f' || ch == 'F' || ch == '\n')
	{

		if(x != u.posx || y != u.posy)
		{

			lm.generate(this, center.x, center.y, natural_light_level(), u.active_light());
			m.draw(this, w_terrain, center);

	// Draw the Monsters
	for(int i = 0; i < z.size(); i++)
	{
		int s_x= z[i].posx - x;
		int s_y= z[i].posy - y;
		int s_d= (int)sqrt(s_x*s_x + s_y*s_y);

		if(!SNIPER || (SNIPER && s_d < 4))
		{
			if(u_see(&(z[i]), tart))
			    z[i].draw(w_terrain, center.x, center.y, false, night_vision);
		}
	}

	// Draw the NPCs
	for(int i = 0; i < active_npc.size(); i++)
	{
		int s_x= active_npc[i].posx - x;
		int s_y= active_npc[i].posy - y;
		int s_d= (int)sqrt(s_x*s_x + s_y*s_y);

		if(!SNIPER || (SNIPER && s_d < 4))
		{
			if(active_npc[i].posz == levz 
				&& u_see(active_npc[i].posx, active_npc[i].posy, tart))
			    active_npc[i].draw(w_terrain, center.x, center.y, false, night_vision);
		}
	}


			u.moves= 0;
			u.process_active_items(this);
			turn.increment();

			return ret;
		}
		else
		{
//CAT-s: menuWrong
			playSound(3);

			add_msg("Too easy...");			

			ret.clear();
			return ret;
		}
	}
	else 
	if(ch == '.' || ch == '5' || ch == ' ')
	{
//CAT-s:
		playSound(0);
		add_msg("You pause to concentrate... ");

		if(u.recoil > 0)
		{
			u.recoil= 0;
			playSound(6);
		}
	}
	else 
	if(ch == 'z')
	{
		runJump(true);
		u.recoil= 45;
	}

	if(u.recoil > 3)
		u.recoil= int(u.recoil/2);

	if(u.recoil > 0 && u.recoil < 3)
		u.recoil= 3;

//	u.moves= 0;

	monmove();
	update_stair_monsters();

	turn.increment();

 } while (true);

}
//CAT-mgs: END *****



void game::hit_monster_with_flags(monster &z, unsigned int effects)
{
 if (effects & mfb(AMMO_FLAME)) {

  if (z.made_of(VEGGY) || z.made_of(COTTON) || z.made_of(WOOL) ||
      z.made_of(PAPER) || z.made_of(WOOD))
   z.add_effect(ME_ONFIRE, rng(8, 20));
  else if (z.made_of(FLESH))
   z.add_effect(ME_ONFIRE, rng(5, 10));
 } else if (effects & mfb(AMMO_INCENDIARY)) {

  if (z.made_of(VEGGY) || z.made_of(COTTON) || z.made_of(WOOL) ||
      z.made_of(PAPER) || z.made_of(WOOD))
   z.add_effect(ME_ONFIRE, rng(2, 6));
  else if (z.made_of(FLESH) && one_in(4))
   z.add_effect(ME_ONFIRE, rng(1, 4));

 }
} 

int time_to_fire(player &p, it_gun* firing)
{
 int time = 0;
 if (firing->skill_used == Skill::skill("pistol")) {
   if (p.skillLevel("pistol") > 6)
     time = 10;
   else
     time = (80 - 10 * p.skillLevel("pistol"));
 } else if (firing->skill_used == Skill::skill("shotgun")) {
   if (p.skillLevel("shotgun") > 3)
     time = 70;
   else
     time = (150 - 25 * p.skillLevel("shotgun"));
 } else if (firing->skill_used == Skill::skill("smg")) {
   if (p.skillLevel("smg") > 5)
     time = 20;
   else
     time = (80 - 10 * p.skillLevel("smg"));
 } else if (firing->skill_used == Skill::skill("rifle")) {
   if (p.skillLevel("rifle") > 8)
     time = 30;
   else
     time = (150 - 15 * p.skillLevel("rifle"));
 } else if (firing->skill_used == Skill::skill("archery")) {
   if (p.skillLevel("archery") > 8)
     time = 20;
   else
     time = (220 - 25 * p.skillLevel("archery"));
 } else if (firing->skill_used == Skill::skill("launcher")) {
   if (p.skillLevel("launcher") > 8)
     time = 30;
   else
     time = (200 - 20 * p.skillLevel("launcher"));
 } else
   time =  0;

 return time;
}



//CAT-s: *** whole lot ***
void make_gun_sound_effect(game *g, player &p, bool burst, item* weapon)
{
 std::string gunsound;
 // noise() doesn't suport gunmods, but it does return the right value
 int noise = p.weapon.noise();

// g->add_msg("GUN NOISE= %d", noise);

 if(weapon->curammo->type == AT_PLASMA)
 {
	g->sound(p.posx, p.posy, 8, "Zwrrm!");
//CAT-s: plasma
	playSound(99);
 }
 else 
 if(weapon->curammo->type == AT_FUSION
	|| weapon->curammo->type == AT_BATT
	|| weapon->curammo->type == AT_PLUT
	|| weapon->curammo->type == AT_PLASMA)
 {
	g->sound(p.posx, p.posy, 7, "Fzzt!");
//CAT-s: laser
	playSound(37);
 }
 else 
 if (weapon->curammo->type == AT_40MM)
 {
	g->sound(p.posx, p.posy, 8, "Thunk!");

//CAT-s: noiseThump
	playSound(93);
 }
 else
 if (weapon->curammo->type == AT_GAS)
 {
	g->sound(p.posx, p.posy, 4, "Fwoosh!");
//CAT-s: spray
	playSound(62);
 }
 else
 if(weapon->curammo->type == AT_66MM)
 {
	g->sound(p.posx, p.posy, 4, "Froomsh!");
//CAT-s: rocket
	playSound(61);
 }
 else 
 if(weapon->curammo->type != AT_BOLT 
	&& weapon->curammo->type != AT_ARROW)
 {

//CAT-mgs:
	 if( weapon->has_gunmod(itm_silencer) >= 0 )
	 {
//CAT-s: silencer
		playSound(36);
		gunsound = "Ptseeww!";
	 }
	 else
	 if (noise < 5) {
	  if (burst)
	   gunsound = "Brrrip!"; 
	  else
	   gunsound = "plink!"; 
   
//CAT:
	   playSound(68);
	 }
	 else 
	 if (noise < 25) {
	  if (burst)
	   gunsound = "Brrrap!"; 
	  else
	   gunsound = "bang!"; 

//CAT:
	   playSound(69);
	 }
	 else
	 if (noise < 60) {
	  if (burst)
	   gunsound = "P-p-p-pow!"; 
	  else
	   gunsound = "blam!"; 

//CAT: uh, too fast to be heard, or too fast for SDL to process?
//	   Sleep(1);
	   playSound(70);
	 }
	 else
	 {
	  if (burst)
	   gunsound = "Kaboom!!"; 
	  else
	   gunsound = "kerblam!"; 

//CAT:
	   playSound(71);
	 }

	g->sound(p.posx, p.posy, noise, gunsound);

//CAT: some ricochet sound for testing
	if(one_in(3))
		playSound(rng(42,43));

 }
//CAT:
 else
 if(weapon->curammo->type == AT_BOLT 
	|| weapon->curammo->type == AT_ARROW)
 {
	g->sound(p.posx, p.posy, noise, "ploomp!");
	playSound(72);
 }

}


int calculate_range(player &p, int tarx, int tary)
{
 int trange = rl_dist(p.posx, p.posy, tarx, tary);
 it_gun* firing = dynamic_cast<it_gun*>(p.weapon.type);
 if (trange < int(firing->volume / 3) && firing->ammo != AT_SHOT)
  trange = int(firing->volume / 3);
 else if (p.has_bionic(bio_targeting)) {
  if (trange > LONG_RANGE)
   trange = int(trange * .65);
  else
   trange = int(trange * .8);
 }

 if (firing->skill_used == Skill::skill("rifle") && trange > LONG_RANGE)
  trange = LONG_RANGE + .6 * (trange - LONG_RANGE);

 return trange;
}

//CAT-mgs: *** vvv
double calculate_missed_by(player &p, int trange, item* weapon)
{
  it_gun* firing = dynamic_cast<it_gun*>(p.weapon.type);
  double deviation = 0.0; 

  if (p.skillLevel(firing->skill_used) < 9)
    deviation += rng( 0, 3 * (9 - p.skillLevel(firing->skill_used)) );

  if (p.skillLevel("gun") < 9)
    deviation += rng( 0, (9 - p.skillLevel("gun"))/2 );

  deviation += p.ranged_dex_mod();
  deviation += p.ranged_per_mod();

  deviation += rng(0, 2 * p.encumb(bp_arms)) + rng(0, 4 * p.encumb(bp_eyes));

  deviation += rng(0, weapon->curammo->accuracy);

  // item::accuracy() doesn't support gunmods.
  deviation += rng(0, p.weapon.accuracy());
  int adj_recoil = p.recoil + p.driving_recoil;

//CAT-mgs: bonus precision for kneeling shoot (recoil == 0)
  deviation += rng(int(adj_recoil / 4), adj_recoil);
  double ret= 0.00325 * deviation * trange;
  if(p.recoil <= 0)
	ret/= 2;

// .013 * trange is a computationally cheap version of finding the tangent.
// (note that .00325 * 4 = .013; .00325 is used because deviation is a number
//  of quarter-degrees)
// It's also generous; missed_by will be rather short.
  return ret;
}

int recoil_add(player &p)
{
 // Gunmods don't have atype,so use guns.
 it_gun* firing = dynamic_cast<it_gun*>(p.weapon.type);
 // item::recoil() doesn't suport gunmods, so call it on player gun.
 int ret = p.weapon.recoil();
 ret -= rng(p.str_cur / 2, p.str_cur);
 ret -= rng(0, p.skillLevel(firing->skill_used) / 2);

 if (ret > 0)
  return ret;
 return 0;
}

void shoot_monster(game *g, player &p, monster &mon, int &dam, double goodhit, item* weapon)
{
 // Gunmods don't have a type, so use the player weapon type.
 it_gun* firing = dynamic_cast<it_gun*>(p.weapon.type);
 std::string message;
 int junk;
 bool u_see_mon = g->u_see(&(mon), junk);
 if (mon.has_flag(MF_HARDTOSHOOT) && !one_in(4) &&
     !(weapon->curammo->m1 == LIQUID) &&
     weapon->curammo->accuracy >= 4) { // Buckshot hits anyway
  if (u_see_mon)
   g->add_msg("The shot passes through the %s without hitting.",
           mon.name().c_str());
  goodhit = 1;
 } else { // Not HARDTOSHOOT
// Armor blocks BEFORE any critical effects.
  int zarm = mon.armor_cut();
  zarm -= weapon->curammo->pierce;
  if (weapon->curammo->m1 == LIQUID)
   zarm = 0;
  else if (weapon->curammo->accuracy < 4) // Shot doesn't penetrate armor well
   zarm *= rng(2, 4);
  if (zarm > 0)
   dam -= zarm;
  if (dam <= 0) {
   if (u_see_mon)
    g->add_msg("The shot reflects off the %s!",
            mon.name_with_armor().c_str());
   dam = 0;
   goodhit = 1;
  }

  if (goodhit < .1 && !mon.has_flag(MF_NOHEAD)) {
   message = "Headshot!";
   dam = rng(5 * dam, 8 * dam);
   p.practice(firing->skill_used, 5);
  } else if (goodhit < .2) {
   message = "Critical!";
   dam = rng(dam * 2, dam * 3);
   p.practice(firing->skill_used, 2);
  } else if (goodhit < .4) {
   dam = rng(int(dam * .9), int(dam * 1.5));
   p.practice(firing->skill_used, rng(0, 2));
  } else if (goodhit <= .7) {
   message = "Grazing hit.";
   dam = rng(0, dam);
  } else
   dam = 0;
// Find the zombie at (x, y) and hurt them, MAYBE kill them!
  if (dam > 0) {
   mon.moves -= dam * 5;
   if (&p == &(g->u) && u_see_mon)
    g->add_msg("%s You hit the %s for %d damage.", message.c_str(), mon.name().c_str(), dam);
   else if (u_see_mon)
    g->add_msg("%s %s shoots the %s.", message.c_str(), p.name.c_str(), mon.name().c_str());

   bool bMonDead = mon.hurt(dam);
//CAT-g:
   hit_animation(g->w_terrain, mon.posx - g->u.posx + VIEWX - g->u.view_offset_x,
                 mon.posy - g->u.posy + VIEWY - g->u.view_offset_y,
                 red_background(mon.type->color), (bMonDead) ? '%' : mon.symbol());

   if (bMonDead)
    g->kill_mon(g->mon_at(mon.posx, mon.posy), (&p == &(g->u)));
   else if (weapon->curammo->ammo_effects != 0)
    g->hit_monster_with_flags(mon, weapon->curammo->ammo_effects);

   dam = 0;
  }
 }
}

void shoot_player(game *g, player &p, player *h, int &dam, double goodhit)
{
 int npcdex = g->npc_at(h->posx, h->posy);
 // Gunmods don't have a type, so use the player gun type.
 it_gun* firing = dynamic_cast<it_gun*>(p.weapon.type);
 body_part hit;
 int side = rng(0, 1), junk;
 if (goodhit < .003) {
  hit = bp_eyes;
  dam = rng(3 * dam, 5 * dam);
  p.practice(firing->skill_used, 5);
 } else if (goodhit < .066) {
  if (one_in(25))
   hit = bp_eyes;
  else if (one_in(15))
   hit = bp_mouth;
  else
   hit = bp_head;
  dam = rng(2 * dam, 5 * dam);
  p.practice(firing->skill_used, 5);
 } else if (goodhit < .2) {
  hit = bp_torso;
  dam = rng(dam, 2 * dam);
  p.practice(firing->skill_used, 2);
 } else if (goodhit < .4) {
  if (one_in(3))
   hit = bp_torso;
  else if (one_in(2))
   hit = bp_arms;
  else
   hit = bp_legs;
  dam = rng(int(dam * .9), int(dam * 1.5));
  p.practice(firing->skill_used, rng(0, 1));
 } else if (goodhit < .5) {
  if (one_in(2))
   hit = bp_arms;
  else
   hit = bp_legs;
  dam = rng(dam / 2, dam);
 } else {
  dam = 0;
 }
 if (dam > 0) {
  h->moves -= rng(0, dam);
  if (h == &(g->u))
   g->add_msg("%s shoots your %s for %d damage!", p.name.c_str(),
              body_part_name(hit, side).c_str(), dam);
  else {
   if (&p == &(g->u)) {
    g->add_msg("You shoot %s's %s.", h->name.c_str(),
               body_part_name(hit, side).c_str());
                g->active_npc[npcdex].make_angry();
 } else if (g->u_see(h->posx, h->posy, junk))
    g->add_msg("%s shoots %s's %s.",
               (g->u_see(p.posx, p.posy, junk) ? p.name.c_str() : "Someone"),
               h->name.c_str(), body_part_name(hit, side).c_str());
  }
  h->hit(g, hit, side, 0, dam);
/*
  if (h != &(g->u)) {
   int npcdex = g->npc_at(h->posx, h->posy);
   if (g->active_npc[npcdex].hp_cur[hp_head]  <= 0 ||
       g->active_npc[npcdex].hp_cur[hp_torso] <= 0   ) {
    g->active_npc[npcdex].die(g, !p.is_npc());
    g->active_npc.erase(g->active_npc.begin() + npcdex);
   }
  }
*/
 }
}

void splatter(game *g, std::vector<point> trajectory, int dam, monster* mon)
{

//CAT-s:
  playSound(27); 

 field_id blood = fd_blood;
 if (mon != NULL) {
  if (!mon->made_of(FLESH))
   return;
  if (mon->type->dies == &mdeath::boomer)
   blood = fd_bile;
  else if (mon->type->dies == &mdeath::acid)
   blood = fd_acid;
 }

 int distance = 1;
 if (dam > 50)
  distance = 3;
 else if (dam > 20)
  distance = 2;

 std::vector<point> spurt = continue_line(trajectory, distance);

 for (int i = 0; i < spurt.size(); i++) {
  int tarx = spurt[i].x, tary = spurt[i].y;
  if (g->m.field_at(tarx, tary).type == blood &&
      g->m.field_at(tarx, tary).density < 3)
   g->m.field_at(tarx, tary).density++;
  else
   g->m.add_field(g, tarx, tary, blood, 1);
 }
}

//CAT-mgs: **** vvv
void ammo_effects(game *g, int x, int y, long effects)
{
//CAT: C4 not accounted for, it's TOOL, no AMMO
 if (effects & mfb(AMMO_EXPLOSIVE))
 {
	g->explosion(x, y, 15, 0, false);

	for(int i=0; i < 3;i++)
	{
		int px= x + rng(-1, 1);
		int py= y + rng(-1, 1);

		ter_id &pter = g->m.ter(px, py);
		if(pter == t_dirt || pter == t_grass || pter == t_sand)
            	pter = t_dirtmound;
	}
 }

 if (effects & mfb(AMMO_EXPLOSIVE_BIG))
 {
	g->explosion(x, y, 40, 0, false);

	for(int i=0; i < 5;i++)
	{
		int px= x + rng(-2, 2);
		int py= y + rng(-2, 2);

		ter_id &pter = g->m.ter(px, py);
		if(pter == t_dirt || pter == t_grass || pter == t_sand)
            	pter = t_dirtmound;
		else
		if(pter == t_pavement || pter == t_pavement_y || pter == t_sidewalk)
            	pter = t_rubble;
	}
 }

 if (effects & mfb(AMMO_FRAG))
 {
	g->explosion(x, y, 12, 9, false);

	for(int i=0; i < 5;i++)
	{
		int px= x + rng(-3, 3);
		int py= y + rng(-3, 3);

		ter_id &pter = g->m.ter(px, py);
		if(pter == t_dirt || pter == t_grass || pter == t_sand)
            	pter = t_dirtmound;
		else
		if(pter == t_pavement || pter == t_pavement_y || pter == t_sidewalk)
            	pter = t_rubble;
	}
 }

 if (effects & mfb(AMMO_NAPALM))
  g->explosion(x, y, 18, 0, true);

 if (effects & mfb(AMMO_TEARGAS)) {
  for (int i = -2; i <= 2; i++) {
   for (int j = -2; j <= 2; j++)
    g->m.add_field(g, x + i, y + j, fd_tear_gas, 3);
  }
 }

 if (effects & mfb(AMMO_SMOKE)) {
  for (int i = -1; i <= 1; i++) {
   for (int j = -1; j <= 1; j++)
    g->m.add_field(g, x + i, y + j, fd_smoke, 3);
  }
 }

 if (effects & mfb(AMMO_FLASHBANG))
  g->flashbang(x, y);

//CAT-mgs:
 if(effects & mfb(AMMO_FLAME))
 {
	if(g->u.weapon.has_flag(IF_FIRE_100))
		g->explosion(x +rng(-3,3), y +rng(-3,3), 10, 0, true);
	else
		g->explosion(x+rng(-2,2), y+rng(-2,2), 4, 0, true);

	g->explosion(x, y, 7, 0, true);
 }

}
