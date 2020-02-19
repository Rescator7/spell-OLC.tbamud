# spell-OLC.tbamud

# Main menu

<p> This is an example from the spell poison vnum 33. 
  
  - Type: SPELL or SKILL. mostly, spell needs to be called by cast 'spell name', while
  SKILL act more like a command: bash someone.
  - Status: Available / Unvailable. Turn on/off spells/skills on the fly.
  - Name: Name of the spell/skill could be changed at will.
  - Min position: Minimum position to cast a spell, or call a skill.
  - Target flags: (see below)
  - Magic FLAGS: (see below)
  
  - Damages: (Does damage up to max damage to the victim)
    - Damage (string formula)
    
    - Max damage (integer)
    
    Require Magic FLAGS: DAMAGE VIOLENT
      
    Learn more about string formula: 
    https://github.com/Rescator7/spell-OLC.tbamud/blob/master/src/formula.README
    
  - Pulse delay (string formula): Make a delay after a spell/skill is called.
  
  - Effectiveness (string formula): How effective, or dificult to call this spell/skill. 100% effectiveness doesn't
  imply 100% success rate. It's a mix of your spell/skill learned % * effectiveness / 100.
  for e.g: If you are learned at 66% in poison, and the effectiveness is 83%. The true effectiveness of
  that spell would be 66 * 83 / 100 = 54%. So, nearly 1 time out of 2 you'll receive the message: "You lost your concentration".
  
  #### Menu colors
  
  - Menu in white: Option not set.
  - Menu in blue: Option is set.
  - Menu in Yellow: Option should be set.
  - Menu in Red: Option is disabled. This spell or skill use special build-in code.

![screenshot](https://github.com/Rescator7/spell-OLC.tbamud/blob/master/screenshots/spell-OLC-scr1.jpg)

# Min position

<p>The minimum position to cast a spell, or perform a skill. Usually, Fighting is used for in combat spell, and Standing for non-combat spell.</p>
   
  - Dead
  - Mortally wounded
  - Incapacitated
  - Sleeping
  - Resting
  - Sitting
  - Fighting
  - Standing
  
![screenshot](https://github.com/Rescator7/spell-OLC.tbamud/blob/master/screenshots/spell-OLC-scr2.jpg)

# Magic flags

- DAMAGE: This flag activate "6) Damages" effect.
- AFFECTS: This flag activate "A) Menu -> Applies & Affects" effects.
- UNAFFECTS: This flag activate "D) Menu -> Dispells" effects.
- POINTS: This flag activate "9) Menu -> Points" effects.
- ALTER OBJS: To modify objects. Only used by those original spells:

              - SPELL_BLESS
              - SPELL_CURSE
              - SPELL_INVISIBLE
              - SPELL_POISON
              - SPELL_REMOVE_CURSE
              - SPELL_REMOVE_POISON
  This version of spell OLC doesn't support ALTER OBJS execpt original spells, but it's possible to manually add support in magic.c (mag_alter_objs)
- GROUPS: Spells groups. Only used by those original spells:

              - SPELL_GROUP_HEAL
              - SPELL_GOUP_ARMOR
              - SPELL_GOUP_RECALL
  This version of spell OLC doesn't support GROUPS except original spells, but it's possible to manually add support in magic.c (perform_mag_groups)
- MASSES: This is unused in TBA MUD 2020 yet.
- AREAS: Activate damages to be done in the room. Original SPELL_EARTHQUAKE require it.
- SUMMONS: This flag activate "X) Menu -> Summon mobile" effects.
- CREATIONS: This flag activate "O) Menu -> Create objects" effects.
- MANUAL: This flag is used by special spells/skills with function. 
- ROOMS: Differ from AREAS as it is used to modify a room. ONLY used by the original SPELL_DARKNESS.
 
 This version of spell OLC doesn't support ROOMS except the original SPELL, but it's possible to manually add support in magic.c (mag_rooms)
- VIOLENT: This flag is required by spells/skills with violent actions. It will stop your action in peaceful rooms, and it will start a fight even if your spell/skill failed. This flags is also required if DAMAGES or AREAS flags are set.
- ACCDUR: Used in conjonction with AFFECTS or PROTECTIONS, it accumulate the duration.
- ACCMOD: Used in conjonction with AFFECTS and APPLIES only, it accumulate modifier.
- PROTECTION: This flag is activate "P) Menu -> Protection from" effects.

<b>DON'T FORGET TO SET "Magic FLAGS" YOU NEED, OR YOU WILL SEARCH A LONG TIME FIGURING OUT WHY YOUR SPELL AS NO EFFECT.</b>
  
  Because, it's possible to create effects, and forget to activate them by using the proper "Magic FLAGS".
  
![screenshot](https://github.com/Rescator7/spell-OLC.tbamud/blob/master/screenshots/spell-OLC-scr3.jpg)

# Menu Points

<p> You can modify the player's points:</p>

  - Hit p. \<value>
  - Mana \<value>
  - Move \<value>
  - Gold \<value>
  
<p>Value is a string formula. The result of the formula will be <b>added</b> to the player's point.</p>

  - Hit p. can't go below 1, and not above MAX_HIT
  - Mana can't go below 0, and not above MAX_MANA
  - Move can't go below 0, and above MAX_MOVE
  - Gold can't go below 0, no upper limits!

<p>Affected by: <b>Target FLAGS</b></p>
<p>Require Magic Flags : <b>POINTS</b></p>

![screenshot](https://github.com/Rescator7/spell-OLC.tbamud/blob/master/screenshots/spell-OLC-scr4.jpg)
![screenshot](https://github.com/Rescator7/spell-OLC.tbamud/blob/master/screenshots/spell-OLC-scr5.jpg)
![screenshot](https://github.com/Rescator7/spell-OLC.tbamud/blob/master/screenshots/spell-OLC-scr6.jpg)
![screenshot](https://github.com/Rescator7/spell-OLC.tbamud/blob/master/screenshots/spell-OLC-scr7.jpg)
![screenshot](https://github.com/Rescator7/spell-OLC.tbamud/blob/master/screenshots/spell-OLC-scr8.jpg)
![screenshot](https://github.com/Rescator7/spell-OLC.tbamud/blob/master/screenshots/spell-OLC-scr9.jpg)
![screenshot](https://github.com/Rescator7/spell-OLC.tbamud/blob/master/screenshots/spell-OLC-scr10.jpg)
![screenshot](https://github.com/Rescator7/spell-OLC.tbamud/blob/master/screenshots/spell-OLC-scr11.jpg)
# Menu Script

<p>The menu script is more a list of commands that spell or skill will execute at the end of it execution.
The list support currenty those commands:</p>

<p>Communication:</p>
<pre>
SAY_TO_ROOM {"message"};
SAY_TO_SELF {"message"};
SAY_TO_CHAR {"message"};
SAY_TO_VICT {"message"};</pre>

<p>Those messages functions are complementary to the "M) Menu -> Messages".</p>

<p>Action:</p>
<pre>
TELEPORT {"where"};</pre>

<p>where = room vnum or mobile name.</p>

<p>Will teleport either the caster or the victiom to the room "where". 
Depend on MAG_FLAGS: SELF_ONLY, CHAR_ROOM.</p>
<pre>
LOAD_MOBILE {"mob, where"};</pre>

- mob = mobile vnum.
- where = room vnum or a second mobile name.

<p>Will load mobile "mob" to "where". The mobile will NOT be affected by AFF_CHARM.</p>
<p>For a charmed mobile use "X) Menu -> Summon mobile".</p>
<p>That's it for now. More script commands could easily be added in spells_script.c</p>

![screenshot](https://github.com/Rescator7/spell-OLC.tbamud/blob/master/screenshots/spell-OLC-scr13.jpg)
![screenshot](https://github.com/Rescator7/spell-OLC.tbamud/blob/master/screenshots/spell-OLC-scr14.jpg)

# Warnings

<p>Print a list of warnings about spell or skill bad setup.
  
  for e.g: If you create an apply or affects. Menu A, but you don't set Magic Flags: <b>AFFECTS</b>
  
  you'll receive that warning: 
  
  <i>Magic flags: MAG_AFFECTS is required. (Affects and applies are set).</i>
  </p>

![screenshot](https://github.com/Rescator7/spell-OLC.tbamud/blob/master/screenshots/spell-OLC-scr15.jpg)

# splist command

<p>List of spells and skills. The command accept 1 filter option.</p>

![screenshot](https://github.com/Rescator7/spell-OLC.tbamud/blob/master/screenshots/spell-OLC-scr12.jpg)

