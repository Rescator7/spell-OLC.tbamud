# spell-OLC.tbamud

![screenshot](https://github.com/Rescator7/spell-OLC.tbamud/blob/master/screenshots/spell-OLC-scr1.jpg)
![screenshot](https://github.com/Rescator7/spell-OLC.tbamud/blob/master/screenshots/spell-OLC-scr2.jpg)
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
  
  for e.g: If you create an apply or affects. Menu A, but you don't set Magic Flags: <b>POINTS</b>
  
  you'll receive that warning: 
  
  <i>Magic flags: MAG_AFFECTS is required. (Affects and applies are set).</i>
  </p>

![screenshot](https://github.com/Rescator7/spell-OLC.tbamud/blob/master/screenshots/spell-OLC-scr15.jpg)

# splist command

<p>List of spells and skills. The command accept 1 filter option.</p>

![screenshot](https://github.com/Rescator7/spell-OLC.tbamud/blob/master/screenshots/spell-OLC-scr12.jpg)

