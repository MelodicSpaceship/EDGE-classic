CHANGELOG for EDGE-Classic 1.37 (since EDGE-Classic 1.36)
====================================

New Features
------------
- Added support for Lua as the default scripting language
  - Includes a COAL compatibility layer to ease porting
  - Ported DOOM, Heretic, Blasphemer, and Harmony stock scripts to use Lua
  - Script debugging capabilities are included (https://github.com/slembcke/debugger.lua)
  - COAL, for the time being, will still be instantiated if a PWAD or pack file contains a COALHUDS file
- Added support for MPEG video playback
  - Files must use MPEG1 video and MP2 audio for proper decoding
  - DDFGAME: TITLE_MOVIE=<wad/packfile name> command added
    - Movies will always play before any defined TITLE_GRAPHIC entry in the same definition
  - DDFLEVL: PRE.MOVIE=<wad/packfile name> and POST.MOVIE=<wad/packfile name> commands added
    - Text will always be printed first, followed by movie playback, followed by any defined graphics
      for a DDFLEVL PRE/POST entry
  - RSCRIPT: PLAY_MOVIE <wad/packfile name> command added
- Added support for MUSINFO-based music changer things
  - Uses same numbering and methodology as detailed in https://doomwiki.org/wiki/MUSINFO
- Added simple IWAD selection dialog window if multiple valid IWADs are found on startup
  - Previous "best choice" scoring system removed

General Improvements/Changes
--------------------
- "GMGSx" (Default.sf2) and "microGM" soundfonts removed for licensing/copyright issues
  - "sf_GMbank" soundfont added as the new Default.sf2
  - "16-Bit Game Station" soundfont added as replacement for microGM
- "DMXOPL" instrument bank removed for licensing/copyright issues
  - Replaced with fully free variant from the Freedoom project
- YMFM OPL emulator replaced with Opal from Reality Adlib Tracker
  - RAD-format modules (version 2) are now supported in addition to regular OPL MUS/MIDI/IMF playback
- libVGM removed
  - gzip component of miniz also removed (was only used to decompress VGZ files)
- Alt+Enter will toggle between fullscreen and windowed modes
  - If toggling into windowed mode, the cursor will be released
- Z/R/D/MAPINFO parsing removed in order to focus on better validating UMAPINFO and adherence to its standards
- Updated default console and Options/Load/Save menu fonts
- Improved scaling and sampling quality of TrueType fonts at varying resolutions
- Improved kerning and sizing when drawing console text with TrueType fonts
- Option/Load Game/Save Game menu backgrounds will, by default, be a grayscaled version of the first valid title image
  - DDFSTYLE custom backgrounds for these menus are rendered afterwards to account for tinting or other desired effects
- Removed randomising from IMPACT_OBJECT and EFFECT_OBJECT spawning.
- Doom Builder 2 Configuration files added, courtesy of akaAgar (https://github.com/edge-classic/EDGE-classic/pull/579)
- Formatted EDGE Classic sources to have consistent style and included Clang formatting configuration
- Default Purple and Black splash objects added
- RSCRIPT:
  - New sector-based trigger conditions (alternative to using radii)
    - SECTOR_TRIGGER_TAG \<tag\>: Script will trigger once any sector with this tag is entered
    - SECTOR_INDEX_TAG \<index\>: Script will trigger once the sector with this index (as viewable via map editor) is entered
  - START_MAP directive will no longer clear existing scripts for the map in question
  - #CELAR_MAP \<mapname\> directive added for granular clearing of map scripts

Bugs fixed
----------
- Fixed UMAPINFO entries with no sky defined and past the range of stock levels causing a CTD
- Removed sector fog color check that was causing the HOM debugging texture to appear even with debug_hom set to 0
- Fixed A_CloseShotgun2 frame only playing the DBCLS sound and not calling A_Refire as well
- Fixed custom Dehacked weapons that use the chainsaw attack not playing the SAWFUL sfx when attacking (regardless of hitting a target)
- SKULLFLY attacks that cause zero damage no longer make victim enter painstate
- Fixed Nightmare/Fast Monster state duration reduction for monsters with a value for the 'FAST' parameter
  - Speed in fast mode is now governed by the 'FAST_SPEED' parameter in all cases, instead of using 'FAST' as a multiplier against the base speed
- Fixed calculation of scroller push forces when multiple scrollers affect the same mobj
- Fixed mirror and portal issues with GLES2 renderer 
- Fixed most compiler warnings under GCC/Clang
- Fixed custom PLAYPAL/palettes not being applied to the TEXTURE image class
- Fixed the "Reset to Defaults" menu option not properly resetting smoothed or upscaled images
- Fixed IWADs being dragged-and-dropped onto the program executable being treated as PWADs
- Fixed memory leak involving detail slope creation
- Fixed the "Blood Level" gameplay option defaulting to "Extra"