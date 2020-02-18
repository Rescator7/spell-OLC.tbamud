# spell-OLC.tbamud

![screenshot](https://github.com/Rescator7/spell-OLC.tbamud/blob/master/screenshots/spell-OLC-scr1.jpg)
![screenshot](https://github.com/Rescator7/spell-OLC.tbamud/blob/master/screenshots/spell-OLC-scr2.jpg)
![screenshot](https://github.com/Rescator7/spell-OLC.tbamud/blob/master/screenshots/spell-OLC-scr3.jpg)
![screenshot](https://github.com/Rescator7/spell-OLC.tbamud/blob/master/screenshots/spell-OLC-scr4.jpg)
![screenshot](https://github.com/Rescator7/spell-OLC.tbamud/blob/master/screenshots/spell-OLC-scr5.jpg)
![screenshot](https://github.com/Rescator7/spell-OLC.tbamud/blob/master/screenshots/spell-OLC-scr6.jpg)
![screenshot](https://github.com/Rescator7/spell-OLC.tbamud/blob/master/screenshots/spell-OLC-scr7.jpg)
![screenshot](https://github.com/Rescator7/spell-OLC.tbamud/blob/master/screenshots/spell-OLC-scr8.jpg)
![screenshot](https://github.com/Rescator7/spell-OLC.tbamud/blob/master/screenshots/spell-OLC-scr9.jpg)
![screenshot](https://github.com/Rescator7/spell-OLC.tbamud/blob/master/screenshots/spell-OLC-scr10.jpg)
![screenshot](https://github.com/Rescator7/spell-OLC.tbamud/blob/master/screenshots/spell-OLC-scr11.jpg)
![screenshot](https://github.com/Rescator7/spell-OLC.tbamud/blob/master/screenshots/spell-OLC-scr12.jpg)

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

<p>Will load mobile <mob> to <where>. The mobile will NOT be affected by AFF_CHARM.</p>
<p>That's it for now. More script commands could easily be added in spells_script.c</p>

![screenshot](https://github.com/Rescator7/spell-OLC.tbamud/blob/master/screenshots/spell-OLC-scr13.jpg)
![screenshot](https://github.com/Rescator7/spell-OLC.tbamud/blob/master/screenshots/spell-OLC-scr14.jpg)
![screenshot](https://github.com/Rescator7/spell-OLC.tbamud/blob/master/screenshots/spell-OLC-scr15.jpg)
