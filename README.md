# spell-OLC.tbamud

<p>Spell and skill online creation for Tba mud 2020.</p>
https://github.com/tbamud/tbamud

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

# Menu Protection from

<p>This menu makes protection spells. Protection are NOT against an affect, but an entire spell ! The system accept upto 6 protections.</p>

1) Name     : poison (33)
   Duration : 4
   Resist % : 20

- Name (integer)
- Duration (string formula)
- Resist % (string formula)

<p>Require Magic Flags: <b>PROTECTION</b>
  
![screenshot](https://github.com/Rescator7/spell-OLC.tbamud/blob/master/screenshots/spell-OLC-scr5.jpg)

# Menu Applies & Affects

<p>This menu add applies and affects to a spell/skill. This is a 6 slot menu. </p>

- APPLIES (1 - 25) 
  - NONE(1): should not be used. it does nothing.
  - CLASS(8): should not be used. it does nothing.
  - LEVEL(9): should not be used. it does nothing.
  - GOLD(16): should not be used. it does nothing.
  - EXP(17): should not be used. it does nothing.

- AFFECTS (26 - 48)
  - UNUSED(46): should not be used.
  - PROT_SPELL(48): should not be used. it is reserved for Protection Menu system.
<p> APPLIES require a modifier and a duration. Both are string formula. AFFECTS only require the duration.</p>

<p>Require Magic Flags: <b>AFFECTS</b>
  
![screenshot](https://github.com/Rescator7/spell-OLC.tbamud/blob/master/screenshots/spell-OLC-scr6.jpg)

# Menu Dispel

<p>This menu is used to create a spell or skill that will remove all affects and applies from a specific spell.</p>
This is a 3 slots menu.</p> 

  - spell(string formula)
  
<p>Require Magic Flags: <b>UNAFFECTS</b></p>

![screenshot](https://github.com/Rescator7/spell-OLC.tbamud/blob/master/screenshots/spell-OLC-scr7.jpg)

# Menu Create objects

<p>This menu is used to create an object. Up to 3 objects. The object is a string formula. Examples: </p>

  1) Object : 1200 + self.class
  2) Object : 1300 + (self.good ? 1 : (self.evil ? 2 : 3))
  
<p>Example #1 will create:</p>

 - object vnum 1200 if caster is Magical User 
 - object vnum 1201 if caster is Cleric
 - object vnum 1202 if caster is a Thief
 - object vnum 1203 if caster is a Warrior

<p>Example #2 will create: </p>

  - object 1301 if the caster is good
  - object 1302 if the caster is evil
  - object 1303 if the caster is neutral
  
<p>Require Magic Flags: <b>CREATIONS</b></p>
<p><b>NOTE:</b> Object vnum <= 0 is not considered valid.</p>

![screenshot](https://github.com/Rescator7/spell-OLC.tbamud/blob/master/screenshots/spell-OLC-scr8.jpg)

# Menu Summon mobile

<p>This menu is used to summon a mobile. It may or not require an object item to succeed</p>

   - Spell Clone(9) 
   1) Mobile        : 10
   2) Required item : 161

<p>This exemple is from spell clone(9). It load mobile(10) "the clone". If the object(161) "sacrificial entrails" is in the inventory of the caster, the spell will succeed and the object will be deleted. The mobile will be affected by CHARM.</p>

<p>Info: Both "Mobile" and "Required item" are string formula.</p>

<p>Require Magic Flags: <b>SUMMONS</b></p>
<p><b>NOTE:</b> Mobile vnum <= 0 is not considered valid.</p>
  
![screenshot](https://github.com/Rescator7/spell-OLC.tbamud/blob/master/screenshots/spell-OLC-scr9.jpg)

# Menu Classes

<p>You can assign a class to your spell or skill in this menu. You can assign all the 4 originals classes if needed.</p>

1) Class       : Magic User (  3) 
    - Prac gain % : \<empty>
    - Mana        : (25 - (1 * self.level)) > 10 ? (25 - (1 * self.level)) : 10
2) Class       : Cleric (  7) 
    - Prac gain % : \<empty>
    - Mana        : (25 - (1 * self.level)) > 10 ? (25 - (1 * self.level)) : 10
3) Class       : \<empty> (  0) 

 - Class(integer) = Class name (# i) it is assigned to.
 - Prac gain %(string formula): Which % gain you receive toward learning a spell/skill by practice (at the guild). <b>IF NOT SET</b> the TBA mud build-in learning system would be used. <b>NOTE:</b> Minimum gain is 5%, to avoid having formula that return negative or NULL value.
 - Mana(string formula): Spell mana cost. <b>This option is disabled for SKILL</b>
 
 <b>PLEASE NOTES: </b>
 1) From the main menu switching from Type: SPELL to SKILL will unset all Menu -> Classes: Mana settings!
 2) There is a mininmum mana cost per spell of 5 mana. Even, if Mana is set to 0.

![screenshot](https://github.com/Rescator7/spell-OLC.tbamud/blob/master/screenshots/spell-OLC-scr10.jpg)

# Menu Messages

<p>In this menu you can set the basic spell and skill messages. The messages support act codes.</p>

   1) Wear off  : You feel less righteous.
   2) To self   : $b glows briefly.
   3) To victim : You feel righteous.
   4) To room   : \<empty>

- Wear off message: This message is sent when an affect or apply wear off. If Applies & Affects isn't set it does nothing.
- To self: Send this message to the caster. Used in particuliar spell or skill, when there is no victim, or the caster isn't the victim.
- To victim: Send this message to the victim. The caster is sometime the victim. For e.g: in SELF_ONLY spell.
- To room: Send this message to the room.

<p>read more about act code: https://github.com/tbamud/tbamud/blob/master/doc/act.txt</p>

<p><b>NOTES:</b></p>

  1) Spell OLC created a new act code $b this code act like $p when the target of the spell is an object, and act like $N when the target is a MOB/PC. In this exemple spell bless(23) will send the message $b glows briedfly to the caster. $b = will be either the name of the object or victim's name.

  2) It's still possible to send more messages using "Menu -> Script".
  
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

