// Converted from .qc on 05/02/2016

#include "g_local.h"
#include "fb_globals.h"

static char* EnemyTeamName (int botNumber);
static char* FriendTeamName (int botNumber);

#define FB_CVAR_DODGEFACTOR "k_fbskill_movement_dodgefactor"
#define FB_CVAR_LOOKANYWHERE "k_fbskill_aim_lookanywhere"
#define FB_CVAR_LOOKAHEADTIME "k_fbskill_goallookaheadtime"
#define FB_CVAR_PREDICTIONERROR "k_fbskill_goalpredictionerror"
#define FB_CVAR_VISIBILITY "k_fbskill_visibility"
#define FB_CVAR_LGPREF "k_fbskill_aim_lgpref"
#define FB_CVAR_ACCURACY "k_fbskill_aim_accuracy"
#define FB_CVAR_YAW_MIN_ERROR "k_fbskill_aim_yaw_min"
#define FB_CVAR_YAW_MAX_ERROR "k_fbskill_aim_yaw_max"
#define FB_CVAR_YAW_MULTIPLIER "k_fbskill_aim_yaw_multiplier"
#define FB_CVAR_YAW_SCALE "k_fbskill_aim_yaw_scale"
#define FB_CVAR_PITCH_MIN_ERROR "k_fbskill_aim_pitch_min"
#define FB_CVAR_PITCH_MAX_ERROR "k_fbskill_aim_pitch_max"
#define FB_CVAR_PITCH_MULTIPLIER "k_fbskill_aim_pitch_multiplier"
#define FB_CVAR_PITCH_SCALE "k_fbskill_aim_pitch_scale"
#define FB_CVAR_ATTACK_RESPAWNS "k_fbskill_aim_attack_respawns"
#define FB_CVAR_REACTION_TIME "k_fbskill_reactiontime"

#define FB_CVAR_MIN_VOLATILITY "k_fbskill_vol_min"
#define FB_CVAR_MAX_VOLATILITY "k_fbskill_vol_max"
#define FB_CVAR_INITIAL_VOLATILITY "k_fbskill_vol_init"
#define FB_CVAR_REDUCE_VOLATILITY "k_fbskill_vol_reduce"
#define FB_CVAR_OWNSPEED_VOLATILITY_THRESHOLD "k_fbskill_vol_ownvel"
#define FB_CVAR_OWNSPEED_VOLATILITY_INCREASE "k_fbskill_vol_ownvel_incr"
#define FB_CVAR_ENEMYSPEED_VOLATILITY_THRESHOLD "k_fbskill_vol_oppvel"
#define FB_CVAR_ENEMYSPEED_VOLATILITY_INCREASE "k_fbskill_vol_oppvel_incr"
#define FB_CVAR_ENEMYDIRECTION_VOLATILITY_INCREASE "k_fbskill_vol_oppdir_incr"

static float RangeOverSkill (int skill_level, float minimum, float maximum)
{
	float skill = skill_level * 1.0f / (MAX_FROGBOT_SKILL - MIN_FROGBOT_SKILL);

	return minimum + skill * (maximum - minimum);
}

void RunRandomTrials (float min, float max, float mult)
{
	int i = 0;
	int trials = 1000;
	float frac = (max - min) / 6;
	int hits[] = { 0, 0, 0, 0, 0, 0 };
	for (i = 0; i < trials; ++i) {
		float result = dist_random (min, max, mult);

		if (result < min + frac)
			++hits[0];
		else if (result < min + frac * 2)
			++hits[1];
		else if (result < min + frac * 3)
			++hits[2];
		else if (result < min + frac * 4)
			++hits[3];
		else if (result < min + frac * 5)
			++hits[4];
		else
			++hits[5];
	}

	G_bprint (2, "Randomisation trials: %f to %f, %f\n", min, max, mult);
	for (i = 0; i < 6; ++i)
		G_bprint (2, " < %2.3f: %4d %3.2f%%\n", min + frac * (i+1), hits[i], hits[i] * 100.0f / trials);
}

void RegisterSkillVariables (void)
{
	extern qbool RegisterCvar (const char* var);

	RegisterCvar (FB_CVAR_DODGEFACTOR);
	RegisterCvar (FB_CVAR_LOOKANYWHERE);
	RegisterCvar (FB_CVAR_LOOKAHEADTIME);
	RegisterCvar (FB_CVAR_PREDICTIONERROR);
	RegisterCvar (FB_CVAR_VISIBILITY);
	RegisterCvar (FB_CVAR_LGPREF);
	RegisterCvar (FB_CVAR_ACCURACY);
	RegisterCvar (FB_CVAR_YAW_MIN_ERROR);
	RegisterCvar (FB_CVAR_YAW_MAX_ERROR);
	RegisterCvar (FB_CVAR_YAW_MULTIPLIER);
	RegisterCvar (FB_CVAR_YAW_SCALE);
	RegisterCvar (FB_CVAR_PITCH_MIN_ERROR);
	RegisterCvar (FB_CVAR_PITCH_MAX_ERROR);
	RegisterCvar (FB_CVAR_PITCH_MULTIPLIER);
	RegisterCvar (FB_CVAR_PITCH_SCALE);
	RegisterCvar (FB_CVAR_ATTACK_RESPAWNS);
	RegisterCvar (FB_CVAR_REACTION_TIME);

	RegisterCvar (FB_CVAR_MIN_VOLATILITY);
	RegisterCvar (FB_CVAR_MAX_VOLATILITY);
	RegisterCvar (FB_CVAR_INITIAL_VOLATILITY);
	RegisterCvar (FB_CVAR_REDUCE_VOLATILITY);
	RegisterCvar (FB_CVAR_OWNSPEED_VOLATILITY_THRESHOLD);
	RegisterCvar (FB_CVAR_OWNSPEED_VOLATILITY_INCREASE);
	RegisterCvar (FB_CVAR_ENEMYSPEED_VOLATILITY_THRESHOLD);
	RegisterCvar (FB_CVAR_ENEMYSPEED_VOLATILITY_INCREASE);
	RegisterCvar (FB_CVAR_ENEMYDIRECTION_VOLATILITY_INCREASE);
}

void SetAttributesBasedOnSkill (int skill)
{
	cvar_fset (FB_CVAR_ACCURACY, 45 - min (skill, 10) * 2.25);

	cvar_fset (FB_CVAR_DODGEFACTOR, 1);
	cvar_fset (FB_CVAR_LOOKANYWHERE, 1);
	cvar_fset (FB_CVAR_LOOKAHEADTIME, 1);
	cvar_fset (FB_CVAR_PREDICTIONERROR, 0);

	cvar_fset (FB_CVAR_LGPREF, RangeOverSkill (skill, 0.0f, 1.0f));
	cvar_fset (FB_CVAR_VISIBILITY, 0.7071067f - (0.02f * min (skill, 10)));   // equivalent of 90 => 120

	cvar_fset (FB_CVAR_YAW_MIN_ERROR, RangeOverSkill(skill, 1, 1));
	cvar_fset (FB_CVAR_YAW_MAX_ERROR, RangeOverSkill (skill, 5, 4));
	cvar_fset (FB_CVAR_YAW_MULTIPLIER, RangeOverSkill(skill, 3, 2));
	cvar_fset (FB_CVAR_YAW_SCALE, RangeOverSkill (skill, 5, 1));

	cvar_fset (FB_CVAR_PITCH_MIN_ERROR, RangeOverSkill(skill, 2, 1));
	cvar_fset (FB_CVAR_PITCH_MAX_ERROR, RangeOverSkill(skill, 5, 2));
	cvar_fset (FB_CVAR_PITCH_MULTIPLIER, RangeOverSkill(skill, 3, 2));
	cvar_fset (FB_CVAR_PITCH_SCALE, RangeOverSkill (skill, 5, 1));

	cvar_fset (FB_CVAR_ATTACK_RESPAWNS, skill >= 15 ? 1 : 0);
	cvar_fset (FB_CVAR_REACTION_TIME, RangeOverSkill (skill, 0.4f, 0.2f));

	// Volatility
	cvar_fset (FB_CVAR_MIN_VOLATILITY, 1.0f);
	cvar_fset (FB_CVAR_MAX_VOLATILITY, RangeOverSkill (skill, 3.0f, 1.5f));
	cvar_fset (FB_CVAR_INITIAL_VOLATILITY, RangeOverSkill (skill, 2.0f, 1.4f));
	cvar_fset (FB_CVAR_REDUCE_VOLATILITY, RangeOverSkill (skill, 0.99f, 0.95f));
	cvar_fset (FB_CVAR_OWNSPEED_VOLATILITY_THRESHOLD, RangeOverSkill (skill, 320, 400));
	cvar_fset (FB_CVAR_OWNSPEED_VOLATILITY_INCREASE, RangeOverSkill (skill, 0.3f, 0.1f));
	cvar_fset (FB_CVAR_ENEMYSPEED_VOLATILITY_THRESHOLD, RangeOverSkill (skill, 280, 400));
	cvar_fset (FB_CVAR_OWNSPEED_VOLATILITY_INCREASE, RangeOverSkill (skill, 0.6f, 0.2f));
	cvar_fset (FB_CVAR_ENEMYDIRECTION_VOLATILITY_INCREASE, RangeOverSkill (skill, 0.8f, 0.4f));
}

// TODO: Exchange standard attributes for different bot characters/profiles
void SetAttribs(gedict_t* self)
{
	self->fb.skill.accuracy = bound (0, cvar( FB_CVAR_ACCURACY ), 45);

	self->fb.skill.dodge_amount = bound (0, cvar ( FB_CVAR_DODGEFACTOR ), 1);
	self->fb.skill.look_anywhere = bound (0, cvar ( FB_CVAR_LOOKANYWHERE ), 1);
	self->fb.skill.lookahead_time = bound (0, cvar ( FB_CVAR_LOOKAHEADTIME ), 45);
	self->fb.skill.prediction_error = bound (0, cvar ( FB_CVAR_PREDICTIONERROR ), 1);

	self->fb.skill.lg_preference = bound (0, cvar ( FB_CVAR_LGPREF ), 1);
	self->fb.skill.visibility = bound (0.5, cvar ( FB_CVAR_VISIBILITY ), 0.7071067f);   // fov 90 (0.707) => fov 120 (0.5)

	self->fb.skill.aim_params[YAW].minimum = bound (0, cvar (FB_CVAR_YAW_MIN_ERROR), 1);
	self->fb.skill.aim_params[YAW].maximum = bound (0, cvar (FB_CVAR_YAW_MAX_ERROR), 10);
	self->fb.skill.aim_params[YAW].multiplier = bound (0, cvar (FB_CVAR_YAW_MULTIPLIER), 10);
	self->fb.skill.aim_params[YAW].scale = bound (0, cvar (FB_CVAR_YAW_SCALE), 5);

	self->fb.skill.aim_params[PITCH].minimum = bound (0, cvar (FB_CVAR_PITCH_MIN_ERROR), 10);
	self->fb.skill.aim_params[PITCH].maximum = bound (0, cvar (FB_CVAR_PITCH_MAX_ERROR), 10);
	self->fb.skill.aim_params[PITCH].multiplier = bound (0, cvar (FB_CVAR_PITCH_MULTIPLIER), 10);
	self->fb.skill.aim_params[PITCH].scale = bound (0, cvar (FB_CVAR_PITCH_SCALE), 5);

	self->fb.skill.attack_respawns = cvar (FB_CVAR_ATTACK_RESPAWNS) > 0;

	// Volatility
	self->fb.skill.min_volatility = bound (0, cvar (FB_CVAR_MIN_VOLATILITY), 5.0f);
	self->fb.skill.max_volatility = bound (0, cvar (FB_CVAR_MAX_VOLATILITY), 5.0f);
	self->fb.skill.initial_volatility = bound (0, cvar (FB_CVAR_INITIAL_VOLATILITY), 5.0f);
	self->fb.skill.reduce_volatility = bound (0, cvar (FB_CVAR_REDUCE_VOLATILITY), 1.0f);
	self->fb.skill.ownspeed_volatility_threshold = bound (0, cvar (FB_CVAR_OWNSPEED_VOLATILITY_THRESHOLD), 1000);
	self->fb.skill.ownspeed_volatility = bound (0, cvar (FB_CVAR_OWNSPEED_VOLATILITY_INCREASE), 5.0f);
	self->fb.skill.enemyspeed_volatility_threshold = bound (0, cvar (FB_CVAR_ENEMYSPEED_VOLATILITY_THRESHOLD), 1000);
	self->fb.skill.enemyspeed_volatility = bound (0, cvar (FB_CVAR_ENEMYSPEED_VOLATILITY_INCREASE), 5.0f);
	self->fb.skill.enemydirection_volatility = bound (0, cvar (FB_CVAR_ENEMYDIRECTION_VOLATILITY_INCREASE), 5.0f);
	self->fb.skill.awareness_delay = bound (0, cvar (FB_CVAR_REACTION_TIME), 1.0f);
}

char* SetTeamNetName(int botNumber, const char* teamName) {
	float playersOnThisTeam = 0,
	      playersOnOtherTeams = 0,
	      frogbotsOnThisTeam = 0;
	char* attemptedName = NULL;
	gedict_t* search_entity = NULL;

	playersOnThisTeam = 0;
	frogbotsOnThisTeam = 0;
	playersOnOtherTeams = 0;

	for (search_entity = world; search_entity = find_plr (search_entity); ) {
		if (!search_entity->isBot) {
			if ( streq( getteam(search_entity), teamName)) {
				playersOnThisTeam = playersOnThisTeam + 1;
			}
			else {
				playersOnOtherTeams = playersOnOtherTeams + 1;
			}
		}
		else if ( streq( getteam(search_entity), teamName ) ) {
			frogbotsOnThisTeam = frogbotsOnThisTeam + 1;
		}
	}

	if (playersOnOtherTeams > 0 && playersOnThisTeam == 0) {
		attemptedName = EnemyTeamName(botNumber);
	}
	else if (playersOnThisTeam > 0 && playersOnOtherTeams == 0) {
		attemptedName = FriendTeamName(botNumber);
	}
	else {
		attemptedName = SetNetName(botNumber);
	}

	if (attemptedName) {
		return attemptedName;
	}
	return SetNetName(botNumber);
}

static char* EnemyTeamName(int botNumber) {
	char* names[] = {
		": Timber",
		": Sujoy",
		": Rix",
		": Batch",

		": Nikodemus",
		": Paralyzer",
		": Kane",
		": Gollum",

		": sCary",
		": Xenon",
		": Thresh",
		": Frick",

		": B2",
		": Reptile",
		": Unholy",
		": Spice"
	};
	char* custom_name = cvar_string (va ("k_fb_name_enemy_%d", botNumber));

	if (strnull (custom_name)) {
		return names[(int)bound(0, botNumber, sizeof(names) / sizeof(names[0]) - 1)];
	}

	return custom_name;
}

static char* FriendTeamName(int botNumber) {
	char* names[] = {
		"> MrJustice",
		"> Parrais",
		"> Jon",
		"> Gaz",

		"> Jakey",
		"> Tele",
		"> Thurg",
		"> Kool",

		"> Zaphod",
		"> Dreamer",
		"> Mandrixx",
		"> Skill5",

		"> Gunner",
		"> DanJ",
		"> Vid",
		"> Soul99"
	};
	char* custom_name = cvar_string (va ("k_fb_name_team_%d", botNumber));

	if (strnull (custom_name)) {
		return names[(int)bound(0, botNumber, sizeof(names) / sizeof(names[0]) - 1)];
	}

	return custom_name;
}

// FIXME
char* SetNetName(int botNumber) {
	char* names[] = {
		"/ bro",
		"/ goldenboy",
		"/ tincan",
		"/ grue",

		"/ dizzy",
		"/ daisy",
		"/ denzil",
		"/ dora",

		"/ shortie",
		"/ machina",
		"/ gudgie",
		"/ scoosh",

		"/ frazzle",
		"/ pop",
		"/ junk",
		"/ overflow"
	};
	char* custom_name = cvar_string (va ("k_fb_name_%d", botNumber));

	if (strnull (custom_name)) {
		return names[(int)bound(0, botNumber, sizeof(names) / sizeof(names[0]) - 1)];
	}

	return custom_name;
}
