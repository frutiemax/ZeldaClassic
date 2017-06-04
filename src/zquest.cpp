//--------------------------------------------------------
//  Zelda Classic
//  by Jeremy Craner, 1999-2000
//
//  zquest.cc
//
//  Main code for the quest editor.
//
//--------------------------------------------------------

/*
  #define  INTERNAL_VERSION  0xA721
  */

#include "precompiled.h" //always first

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <ctype.h>
#include <assert.h>
#include <time.h>
#include <vector>

#include "parser/Compiler.h"
#include "zc_alleg.h"
#include "mem_debug.h"
void setZScriptVersion(int) { } //bleh...

#include <png.h>
#include <pngconf.h>

#include <loadpng.h>
#include <jpgalleg.h>

#include "gui.h"
#include "load_gif.h"
#include "save_gif.h"
#include "editbox.h"
#include "zq_misc.h"
#include "zq_tiles.h"                                       // tile and combo code

#include "zquest.h"
#include "zquestdat.h"
#include "ffasm.h"

// the following are used by both zelda.cc and zquest.cc
#include "zdefs.h"
#include "tiles.h"
#include "colors.h"
#include "qst.h"
#include "zsys.h"
#include "zcmusic.h"

#include "midi.h"
#include "sprite.h"
#include "items.h"
#include "fontsdat.h"
#include "jwinfsel.h"
#include "zq_class.h"
#include "subscr.h"
#include "zq_subscr.h"
#include "ffscript.h"
#include "EditboxNew.h"
#include "sfx.h"

#include "zq_custom.h" // custom items and guys
#include "zq_strings.h"

#include "questReport.h"
#include "backend/AllBackends.h"

#ifdef ALLEGRO_DOS
static const char *data_path_name   = "dos_data_path";
static const char *midi_path_name   = "dos_midi_path";
static const char *image_path_name  = "dos_image_path";
static const char *tmusic_path_name = "dos_tmusic_path";
static const char *last_quest_name  = "dos_last_quest";
static const char *qtname_name      = "dos_qtname%d";
static const char *qtpath_name      = "dos_qtpath%d";
#elif defined(ALLEGRO_WINDOWS)
static const char *data_path_name   = "win_data_path";
static const char *midi_path_name   = "win_midi_path";
static const char *image_path_name  = "win_image_path";
static const char *tmusic_path_name = "win_tmusic_path";
static const char *last_quest_name  = "win_last_quest";
static const char *qtname_name      = "win_qtname%d";
static const char *qtpath_name      = "win_qtpath%d";
#elif defined(ALLEGRO_UNIX)
static const char *data_path_name   = "linux_data_path";
static const char *midi_path_name   = "linux_midi_path";
static const char *image_path_name  = "linux_image_path";
static const char *tmusic_path_name = "linux_tmusic_path";
static const char *last_quest_name  = "linux_last_quest";
static const char *qtname_name      = "linux_qtname%d";
static const char *qtpath_name      = "linux_qtpath%d";
#elif defined(ALLEGRO_MACOSX)
static const char *data_path_name   = "macosx_data_path";
static const char *midi_path_name   = "macosx_midi_path";
static const char *image_path_name  = "macosx_image_path";
static const char *tmusic_path_name = "macosx_tmusic_path";
static const char *last_quest_name  = "macosx_last_quest";
static const char *qtname_name      = "macosx_qtname%d";
static const char *qtpath_name      = "macosx_qtpath%d";
#endif

#include "win32.h" //win32 fixes

#include "zq_init.h"
#include "zq_doors.h"
#include "zq_rules.h"
#include "zq_cset.h"

#ifdef _MSC_VER
#include <crtdbg.h>
#define stricmp _stricmp
#define unlink _unlink
#define snprintf _snprintf

#endif

// MSVC fix
#if _MSC_VER >= 1900
FILE _iob[] = { *stdin, *stdout, *stderr };
extern "C" FILE * __cdecl __iob_func(void) { return _iob; }
#endif



#define zc_max(a,b)  ((a)>(b)?(a):(b))
#define zc_min(a,b)  ((a)<(b)?(a):(b))

void do_previewtext();

int startdmapxy[6] = {-1000, -1000, -1000, -1000, -1000, -1000};
bool cancelgetnum=false;

int tooltip_timer=0, tooltip_maxtimer=30, tooltip_current_combo=0, tooltip_current_ffc=0;
int mousecomboposition;

int original_playing_field_offset=0;
int playing_field_offset=original_playing_field_offset;
int passive_subscreen_height=56;
int passive_subscreen_offset=0;

bool disable_saving=false, OverwriteProtection;
bool halt=false;
bool show_sprites=true;
bool show_hitboxes = false;

// Used to find FFC script names
extern std::map<int, pair<string,string> > ffcmap;
std::vector<string> asffcscripts;
extern std::map<int, pair<string,string> > globalmap;
std::vector<string> asglobalscripts;
extern std::map<int, pair<string, string> > itemmap;
std::vector<string> asitemscripts;

extern GameScripts scripts;

int CSET_SIZE = 16;
int CSET_SHFT = 4;
//editbox_data temp_eb_data;
/*
  #define CSET(x)         ((x)<<CSET_SHFT)
  #define csBOSS          14
  */

/*
  enum { m_block, m_coords, m_flags, m_guy, m_warp, m_misc, m_layers,
  m_menucount };
  */
void update_combo_cycling();
void update_freeform_combos();

/*
  #define MAXMICE 14
  #define MAXARROWS 8
  #define SHADOW_DEPTH 2
  */
int coord_timer=0, coord_frame=0;

bool is_large()
{
	return Backend::graphics->getVirtualMode() == 0;
}

int virtualScreenScale()
{
	return Backend::graphics->getVirtualMode() == 0 ? 3 : 1;
}

int blackout_color()
{
	return is_large() ? 8 : 0;
}

int draw_mode=0;

byte BMM()
{
	return is_large() ? 3 : 1; // Big Minimap
}

int mapscreen_x()
{
	return is_large() ? 0 : 0;
}

int mapscreen_y()
{
	return is_large() ? 16 : 16;
}

int mapscreensize()
{
	return is_large() ? 2 : 1;
}

bool showedges()
{
	return is_large();
}

size_and_pos map_page_bar(int idx)
{
	size_and_pos result;
	if(is_large())
	{
		result.x = mapscreen_x() + (idx * 16 * 2 * mapscreensize());
		result.y = mapscreen_y() + ((showedges() ? 13 : 11) * 16 * mapscreensize());
		result.w = 64;
		result.h = 20;
	}
	else
	{
	}
	return result;
}

size_and_pos combolist(int idx)
{
	size_and_pos result;
	if (is_large())
	{
		switch (idx)
		{
		case 0:
			result.x = 576 + 8;
			result.y = 64;
			result.w = 4;
			result.h = 24;
			break;
		case 1:
			result.x = 576 + 8 + 72;
			result.y = 64;
			result.w = 4;
			result.h = 24;
			break;
		case 2:
			result.x = 576 + 8 + 72 + 72;
			result.y = 64;
			result.w = 4;
			result.h = 24;
			break;
		}
	}
	else
	{
		switch (idx)
		{
		case 0:
			result.x = 256;
			result.y = 16;
			result.w = 4;
			result.h = 14;
			break;
		default:
			result.x = -1;
			result.y = -1;
			result.w = -1;
			result.h = -1;
		}
	}
	return result;
}

size_and_pos comboaliaslist(int idx)
{
	size_and_pos result;
	if (is_large())
	{
		result = combolist(idx);
		result.h -= 5;
	}
	else
	{
		switch (idx)
		{
		case 0:
			result.x = 256;
			result.y = 16;
			result.w = 4;
			result.h = 10;
			break;
		default:
			result.x = -1;
			result.y = -1;
			result.w = -1;
			result.h = -1;			
		}
	}
	return result;
}

size_and_pos comboalias_preview(int idx)
{
	size_and_pos result;
	if (is_large())
	{
		result = combolist(idx);
		result.y += (result.h << 4) + 16;
		result.w = result.w << 4;
		result.h = 64;		
	}
	else
	{
		switch (idx)
		{
		case 0:
			result.x = 256;
			result.y = 176;
			result.w = 64;
			result.h = 64;
			break;
		default:
			result.x = -1;
			result.y = -1;
			result.w = -1;
			result.h = -1;
		}
	}
	return result;
}

size_and_pos combo_preview()
{
	size_and_pos result;
	if (is_large())
	{
		result.x = 576 + 96 - 24;
		result.y = 6;
		result.w = 32;
		result.h = 32;
	}
	else
	{
		result.x = 304;
		result.y = 0;
		result.w = 16;
		result.h = 16;
	}
	return result;
}

size_and_pos combolist_window()
{
	size_and_pos result;
	if (is_large())
	{
		result.x = 576;
		result.y = 0;
		result.w = 224;
		result.h = 464;
	}
	else
	{
		result.x = -1;
		result.y = -1;
		result.w = -1;
		result.h = -1;

	}
	return result;
}

size_and_pos layer_panel()
{
	size_and_pos result;
	if (is_large())
	{
		size_and_pos mpb0 = map_page_bar(0);
		size_and_pos mpb8 = map_page_bar(8);
		result.x = mpb0.x;
		result.y = mpb0.y + mpb0.h;
		result.w = mpb8.x + mpb8.w;
		result.h = 40;
	}
	else
	{
		result.x = -1;
		result.y = -1;
		result.w = -1;
		result.h = -1;

	}
	return result;
}

size_and_pos panel(int idx)
{
	size_and_pos result;
	if(is_large())
	{
		size_and_pos lp = layer_panel();
		result.x = 10 + 48 * BMM();
		result.y = lp.y + lp.h;
		result.w = (map_page_bar(6).x) - (10 + 48 * BMM());
		result.h = 76 + 32;
	}
	else
	{
		result.x = 58;
		result.y = 192;
		result.w = 198;
		result.h = 48;		
	}
	return result;
}

size_and_pos combolistscrollers(int idx)
{
	size_and_pos result;
	if (is_large())
	{
		size_and_pos cl = combolist(idx);
		result.w = 11;
		result.h = 11;
		result.x = cl.x + 21;
		result.y = cl.y - 22;
	}
	else
	{
		switch (idx)
		{
		case 0:
			result.w = 11;
			result.h = 11;
			result.x = panel(0).x + panel(0).w - 15;
			result.y = panel(0).y + 9;
			break;
		default:
			result.w = -1;
			result.h = -1;
			result.x = -1;
			result.y = -1;
		}
	}
	return result;
}

size_and_pos minimap()
{
	size_and_pos result;
	if (is_large())
	{
		result.w = 7 + 48 * BMM();
		result.h = 16 + 27 * BMM();
		result.x = 3;
		result.y = panel(0).y + 4;
	}
	else
	{
		result.x = 3;
		result.y = 195;
		result.w = 55;
		result.h = 43;
	}
	return result;
}

size_and_pos favorites_window()
{
	size_and_pos result;
	if (is_large())
	{
		result.x = 576;
		result.y = 464;
		result.w = 224;
		result.h = 136;
	}
	else
	{
		result.x = -1;
		result.y = -1;
		result.w = -1;
		result.h = -1;
	}
	return result;
}

size_and_pos favorites_list()
{
	size_and_pos result;
	if (is_large())
	{
		size_and_pos fw = favorites_window();
		result.x = fw.x + 8;
		result.y = fw.y + 16;
		result.w = (fw.w - 16) >> 4;
		result.h = (fw.h - 24) >> 4;
	}
	else
	{
		result.x = -1;
		result.y = -1;
		result.w = -1;
		result.h = -1;
	}
	return result;
}

size_and_pos commands_window()
{
	size_and_pos result;
	if (is_large())
	{
		result.w = 576 - (panel(0).x + panel(0).w);
		result.h = Backend::graphics->virtualScreenH() - panel(0).y;
		result.x = favorites_window().x - result.w;
		result.y = panel(0).y;
	}
	else
	{
		result.x = -1;
		result.y = -1;
		result.w = -1;
		result.h = -1;
	}
	return result;
}

size_and_pos commands_list()
{
	size_and_pos result;
	if (is_large())
	{
		result.x = commands_window().x + 8;
		result.y = commands_window().y + 20;
		result.w = 2;
		result.h = 4;
	}
	else
	{
		result.x = -1;
		result.y = -1;
		result.w = -1;
		result.h = -1;
	}
	return result;
}


size_and_pos tooltip_box;
size_and_pos tooltip_trigger;

int command_buttonwidth = 88;
int command_buttonheight = 19;

int layerpanel_buttonwidth = 58;
int layerpanel_buttonheight = 16;

int favorite_combos[MAXFAVORITECOMBOS];
int favorite_comboaliases[MAXFAVORITECOMBOALIASES];
int favorite_commands[MAXFAVORITECOMMANDS];

int mouse_scroll_h()
{
	return is_large() ? 10 : 16;
	;
}

int readsize, writesize;
bool fake_pack_writing=false;

int showxypos_x;
int showxypos_y;
int showxypos_w;
int showxypos_h;
int showxypos_color;
int showxypos_ffc=-1000;
bool showxypos_icon=false;

int showxypos_cursor_x;
int showxypos_cursor_y;
bool showxypos_cursor_icon=false;

bool close_button_quit=false;
bool canfill=true;                                          //to prevent double-filling (which stops undos)
bool resize_mouse_pos=false;                                //for eyeball combos
int lens_hint_item[MAXITEMS][2];                            //aclk, aframe
int lens_hint_weapon[MAXWPNS][5];                           //aclk, aframe, dir, x, y

RGB_MAP zq_rgb_table;
COLOR_MAP trans_table, trans_table2;
char *datafile_str;
DATAFILE *zcdata=NULL, *fontsdata=NULL, *sfxdata=NULL;
MIDI *song=NULL;
FONT       *nfont, *zfont, *z3font, *z3smallfont, *deffont, *lfont, *lfont_l, *pfont, *mfont, *ztfont, *sfont, *sfont2, *sfont3, *spfont, *ssfont1, *ssfont2, *ssfont3, *ssfont4, *gblafont,
           *goronfont, *zoranfont, *hylian1font, *hylian2font, *hylian3font, *hylian4font, *gboraclefont, *gboraclepfont, *dsphantomfont, *dsphantompfont;
BITMAP *menu1, *menu3, *mapscreenbmp, *tmp_scr, *screen2, *mouse_bmp[MOUSE_BMP_MAX][4], *icon_bmp[ICON_BMP_MAX][4], *select_bmp[2], *dmapbmp_small, *dmapbmp_large;
BITMAP *arrow_bmp[MAXARROWS],*brushbmp, *brushscreen, *tooltipbmp;//*brushshadowbmp;
byte *colordata=NULL, *trashbuf=NULL;
itemdata *itemsbuf;
wpndata  *wpnsbuf;
comboclass *combo_class_buf;
guydata  *guysbuf;
item_drop_object    item_drop_sets[MAXITEMDROPSETS];
newcombo curr_combo;
PALETTE RAMpal;
midi_info Midi_Info;
bool zq_showpal=false;
bool combo_cols=true;

// Dummy - needed to compile, but unused
refInfo ffcScriptData[32];

extern std::string zScript;
char zScriptBytes[512];

extern void deallocate_biic_list();

zinitdata zinit;

int onImport_ComboAlias();
int onExport_ComboAlias();


typedef struct map_and_screen
{
    int map;
    int screen;
} map_and_screen;

typedef int (*intF)();
typedef struct command_pair
{
    char name[80];
    int flags;
    intF command;
} command_pair;

extern command_pair commands[cmdMAX];

map_and_screen map_page[9]= {{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}};
int alignment_arrow_timer=0;
int  Flip=0,Combo=0,CSet=2,First[3]= {0,0,0},current_combolist=0,current_comboalist=0,current_mappage=0;
int  Flags=0,Flag=1,menutype=(m_block);
int MouseScroll, SavePaths, CycleOn, ShowGrid, GridColor, TileProtection, InvalidStatic, RulesetDialog, EnableTooltips, ShowFFScripts, ShowSquares, ShowInfo;
int FlashWarpSquare = -1, FlashWarpClk = 0; // flash the destination warp return when ShowSquares is active
bool ShowFPS;
int ComboBrush;                                             //show the brush instead of the normal mouse
int ComboBrushPause;                                        //temporarily disable the combo brush
int BrushPosition;                                          //top left, middle, bottom right, etc.
int FloatBrush;                                             //makes the combo brush float a few pixels up and left
//complete with shadow
int OpenLastQuest;                                          //makes the program reopen the quest that was
//open at the time you quit
int ShowMisalignments;                                      //makes the program display arrows over combos that are
//not aligned with the next screen.
int AnimationOn;                                            //animate the combos in zquest?
int AutoBackupRetention;                                    //use auto-backup feature?  if so, how many backups (1-10) to keep
int AutoSaveInterval;                                       //how often a timed autosave is made (not overwriting the current file)
int UncompressedAutoSaves;                                  //should timed saves be uncompressed/encrypted?
int KeyboardRepeatDelay;                                    //the time in milliseconds after holding down a key that the key starts to repeat
int KeyboardRepeatRate;                                     //the time in milliseconds between each repetition of a repeated key

time_t auto_save_time_start, auto_save_time_current;
double auto_save_time_diff;
int AutoSaveRetention;                                      //how many autosaves of a quest to keep
int ImportMapBias;                                          //tells what has precedence on map importing
int BrushWidth=1, BrushHeight=1;
bool quit=false,saved=true;
bool __debug=false;
//bool usetiles=true;
byte LayerMask[2];                                          //determines which layers are on or off.  0-15
int LayerMaskInt[7];
int CurrentLayer=0;
int DuplicateAction[4];
int OnlyCheckNewTilesForDuplicates;
/*
  , HorizontalDuplicateAction;
  int VerticalDuplicateAction, BothDuplicateAction;
  */
word msg_count, qt_count;
int LeechUpdate;
int LeechUpdateTiles;
int SnapshotFormat;

int memrequested;
byte Color;
int jwin_pal[jcMAX];
int gui_colorset=0;

combo_alias combo_aliases[MAXCOMBOALIASES];
static int combo_apos=0; //currently selected combo
static int combo_alistpos[3]= {0,0,0}; //first displayed combo alias
int alias_origin=0;
int alias_cset_mod=0;

bool trip=false;

int fill_type=1;

bool first_save=false;
char *filepath,*temppath,*midipath,*datapath,*imagepath,*tmusicpath,*last_timed_save;
char *helpbuf;
string helpstr;

ZCMUSIC *zcmusic = NULL;
int midi_volume = 255;
extern int prv_mode;
int prv_warp = 0;
int prv_twon = 0;
int ff_combo = 0;

int zqUseWin32Proc = 1, ForceExit = 0;
int joystick_index=0;

char *getBetaControlString();


void loadlvlpal(int level);
bool get_debug()
{
    return __debug;
    //return true;
}

void set_debug(bool d)
{
    __debug=d;
    return;
}

// quest data
zquestheader header;
byte                quest_rules[QUESTRULES_SIZE];
byte                extra_rules[EXTRARULES_SIZE];
byte                midi_flags[MIDIFLAGS_SIZE];
byte                music_flags[MUSICFLAGS_SIZE];
word                map_count;
miscQdata           misc;
std::vector<mapscr> TheMaps;
zcmap               *ZCMaps;
byte                *quest_file;
dmap                *DMaps;
MsgStr              *MsgStrings;
int					msg_strings_size;
//DoorComboSet      *DoorComboSets;
zctune              *customtunes;
//emusic            *enhancedMusic;
ZCHEATS             zcheats;
byte                use_cheats;
byte                use_tiles;
extern zinitdata    zinit;
char                palnames[MAXLEVELS][17];
quest_template      QuestTemplates[MAXQTS];
char                fontsdat_sig[52];
char                zquestdat_sig[52];
char                qstdat_sig[52];
char                sfxdat_sig[52];

int gme_track=0;

int dlevel; // just here until gamedata is properly done

bool gotoless_not_equal;  // Used by BuildVisitors.cpp

bool bad_version(int ver)
{
    if(ver < 0x170)
        return true;
        
    return false;
}

fix LinkModifiedX()
{
    if(resize_mouse_pos)
    {
        return (fix)((Backend::mouse->getVirtualScreenX()/mapscreensize())-((8*mapscreensize())-1)+(showedges()?8:0));
    }
    else
    {
        return (fix)(Backend::mouse->getVirtualScreenX()-7);
    }
}

fix LinkModifiedY()
{
    if(resize_mouse_pos)
    {
        return (fix)((Backend::mouse->getVirtualScreenY()/mapscreensize())-((8*mapscreensize())-1)-16+(showedges()?16:0));
    }
    else
    {
        return (fix)(Backend::mouse->getVirtualScreenY() -7);
    }
}

static MENU import_menu[] =
{
    { (char *)"&Map",                       onImport_Map,              NULL,                     0,            NULL   },
    { (char *)"&DMaps",                     onImport_DMaps,            NULL,                     0,            NULL   },
    { (char *)"&Tiles",                     onImport_Tiles,            NULL,                     0,            NULL   },
    { (char *)"&Enemies",                   onImport_Guys,             NULL,                     0,            NULL   },
    { (char *)"Su&bscreen",                 onImport_Subscreen,        NULL,                     0,            NULL   },
    { (char *)"&Palettes",                  onImport_Pals,             NULL,                     0,            NULL   },
    { (char *)"&String Table",              onImport_Msgs,             NULL,                     0,            NULL   },
    { (char *)"&Combo Table",               onImport_Combos,           NULL,                     0,            NULL   },
    { (char *)"&Combo Alias",               onImport_ComboAlias,       NULL,                     0,            NULL   },
    { (char *)"&Graphics Pack",             onImport_ZGP,              NULL,                     0,            NULL   },
    { (char *)"&Quest Template",            onImport_ZQT,              NULL,                     0,            NULL   },
    { (char *)"&Unencoded Quest",           onImport_UnencodedQuest,   NULL,                     0,            NULL   },
    {  NULL,                                NULL,                      NULL,                     0,            NULL   }
};

static MENU export_menu[] =
{
    { (char *)"&Map",                       onExport_Map,              NULL,                     0,            NULL   },
    { (char *)"&DMaps",                     onExport_DMaps,            NULL,                     0,            NULL   },
    { (char *)"&Tiles",                     onExport_Tiles,            NULL,                     0,            NULL   },
    { (char *)"&Enemies",                   onExport_Guys,             NULL,                     0,            NULL   },
    { (char *)"Su&bscreen",                 onExport_Subscreen,        NULL,                     0,            NULL   },
    { (char *)"&Palettes",                  onExport_Pals,             NULL,                     0,            NULL   },
    { (char *)"&String Table",              onExport_Msgs,             NULL,                     0,            NULL   },
    { (char *)"Text Dump",                  onExport_MsgsText,         NULL,                     0,            NULL   },
    { (char *)"&Combo Table",               onExport_Combos,           NULL,                     0,            NULL   },
    { (char *)"&Combo Alias",               onExport_ComboAlias,       NULL,                     0,            NULL   },
    { (char *)"&Graphics Pack",             onExport_ZGP,              NULL,                     0,            NULL   },
    { (char *)"&Quest Template",            onExport_ZQT,              NULL,                     0,            NULL   },
    { (char *)"&Unencoded Quest",           onExport_UnencodedQuest,   NULL,                     0,            NULL   },
    {  NULL,                                NULL,                      NULL,                     0,            NULL   }
};

static MENU file_menu[] =
{
    { (char *)"&New",                       onNew,                     NULL,                     0,            NULL   },
    { (char *)"&Open\tF3",                  onOpen,                    NULL,                     0,            NULL   },
    { (char *)"&Save\tF2",                  onSave,                    NULL,                     0,            NULL   },
    { (char *)"Save &as...",                onSaveAs,                  NULL,                     0,            NULL   },
    { (char *)"&Revert",                    onRevert,                  NULL,                     0,            NULL   },
    { (char *)"Quest &Templates...",        onQuestTemplates,          NULL,                     0,            NULL   },
    { (char *)"",                           NULL,                      NULL,                     0,            NULL   },
    { (char *)"&Import\t ",                 NULL,                      import_menu,              0,            NULL   },
    { (char *)"&Export\t ",                 NULL,                      export_menu,              0,            NULL   },
    { (char *)"",                           NULL,                      NULL,                     0,            NULL   },
    { (char *)"E&xit\tESC",                 onExit,                    NULL,                     0,            NULL   },
    {  NULL,                                NULL,                      NULL,                     0,            NULL   }
};

static MENU maps_menu[] =
{
    { (char *)"&Goto Map...",               onGotoMap,                 NULL,                     0,            NULL   },
    { (char *)"Next Map\t.",                onIncMap,                  NULL,                     0,            NULL   },
    { (char *)"Previous Map\t,",            onDecMap,                  NULL,                     0,            NULL   },
    { (char *)"",                           NULL,                      NULL,                     0,            NULL   },
    { (char *)"D&elete Map",                onDeleteMap,               NULL,                     0,            NULL   },
    {  NULL,                                NULL,                      NULL,                     0,            NULL   }
};

static MENU misc_menu[] =
{
    { (char *)"S&ubscreens",                onEditSubscreens,          NULL,                     0,            NULL   },
    { (char *)"&Master Subscreen Type",     onSubscreen,               NULL,                     0,            NULL   },
    { (char *)"&Shop Types",                onShopTypes,               NULL,                     0,            NULL   },
    { (char *)"&Info Types",                onInfoTypes,               NULL,                     0,            NULL   },
    { (char *)"&Warp Rings",                onWarpRings,               NULL,                     0,            NULL   },
    { (char *)"&Triforce Pieces",           onTriPieces,               NULL,                     0,            NULL   },
    { (char *)"&End String",                onEndString,               NULL,                     0,            NULL   },
    { (char *)"Item &Drop Sets",            onItemDropSets,            NULL,                     0,            NULL   },
//{ (char *)"Screen &Opening/Closing",    onScreenOpeningClosing,    NULL,                     0,            NULL   },
    {  NULL,                                NULL,                      NULL,                     0,            NULL   }
};

static MENU spr_menu[] =
{
    { (char *)"&Weapons/Misc",              onCustomWpns,              NULL,                     0,            NULL   },
    { (char *)"&Link",                      onCustomLink,              NULL,                     0,            NULL   },
    {  NULL,                                NULL,                      NULL,                     0,            NULL   }
};

MENU colors_menu[] =
{
    { (char *)"&Main   ",                   onColors_Main,             NULL,                     0,            NULL   },
    { (char *)"&Levels   ",                 onColors_Levels,           NULL,                     0,            NULL   },
    { (char *)"&Sprites   ",                onColors_Sprites,          NULL,                     0,            NULL   },
    {  NULL,                                NULL,                      NULL,                     0,            NULL   }
};

static MENU defs_menu[] =
{
    { (char *)"&Palettes",                  onDefault_Pals,            NULL,                     0,            NULL   },
    { (char *)"&Tiles",                     onDefault_Tiles,           NULL,                     0,            NULL   },
    { (char *)"&Combos",                    onDefault_Combos,          NULL,                     0,            NULL   },
    { (char *)"&Items",                     onDefault_Items,           NULL,                     0,            NULL   },
    { (char *)"&Enemies",                   onDefault_Guys,            NULL,                     0,            NULL   },
    { (char *)"&Weapon Sprites",            onDefault_Weapons,         NULL,                     0,            NULL   },
    { (char *)"&Map Styles",                onDefault_MapStyles,       NULL,                     0,            NULL   },
    { (char *)"SF&X Data",                  onDefault_SFX,             NULL,                     0,            NULL   },
    {  NULL,                                NULL,                      NULL,                     0,            NULL   }
};

int onEditComboAlias();

static MENU graphics_menu[] =
{
    { (char *)"&Palettes\t ",               NULL,                      colors_menu,              0,            NULL   },
    { (char *)"&Sprites\t ",                NULL,                      spr_menu,                 0,            NULL   },
    { (char *)"&Combos",                    onCombos,                  NULL,                     0,            NULL   },
    { (char *)"&Tiles",                     onTiles,                   NULL,                     0,            NULL   },
    { (char *)"&Game icons",                onIcons,                   NULL,                     0,            NULL   },
    { (char *)"Misc co&lors",               onMiscColors,              NULL,                     0,            NULL   },
    { (char *)"&Map styles",                onMapStyles,               NULL,                     0,            NULL   },
    { (char *)"&Door Combo Sets",           onDoorCombos,              NULL,                     0,            NULL   },
    { (char *)"Combo &Aliases",             onEditComboAlias,          NULL,                     0,            NULL   },
    {  NULL,                                NULL,                      NULL,                     0,            NULL   }
};

static MENU audio_menu[] =
{
    { (char *)"SF&X Data",                  onSelectSFX,               NULL,                     0,            NULL   },
    { (char *)"&MIDIs",                     onMidis,                   NULL,                     0,            NULL   },
//{ (char *)"&Enhanced Music",            onEnhancedMusic,           NULL,                     D_DISABLED,   NULL   },
    {  NULL,                                NULL,                      NULL,                     0,            NULL   }
};

static MENU script_menu[] =
{
    { (char *)"Import ASM &FFC Script",     onImportFFScript,          NULL,                     0,            NULL   },
    { (char *)"Import ASM &Item Script",    onImportItemScript,        NULL,                     0,            NULL   },
    { (char *)"Import ASM &Global Script",  onImportGScript,           NULL,                     0,            NULL   },
    { (char *)"Compile &ZScript...",        onCompileScript,           NULL,                     0,            NULL   },
    {  NULL,                                NULL,                      NULL,                     0,            NULL   }
};

static MENU rules_menu[] =
{
    { (char *)"&Animation",                 onAnimationRules,          NULL,                     0,            NULL   },
    { (char *)"&Combos",                    onComboRules,              NULL,                     0,            NULL   },
    { (char *)"&Items",                     onItemRules,               NULL,                     0,            NULL   },
    { (char *)"&Enemies",                   onEnemyRules,              NULL,                     0,            NULL   },
    { (char *)"&NES Fixes ",                onFixesRules,              NULL,                     0,            NULL   },
    { (char *)"&Other",                     onMiscRules,               NULL,                     0,            NULL   },
    { (char *)"&Backward compatibility",    onCompatRules,             NULL,                     0,            NULL   },
    {  NULL,                                NULL,                      NULL,                     0,            NULL   }
};

static MENU quest_menu[] =
{
    { (char *)"&Header",                    onHeader,                  NULL,                     0,            NULL   },
    { (char *)"&Rules\t ",                  NULL,                      rules_menu,               0,            NULL   },
    { (char *)"Ma&p Count",                 onMapCount,                NULL,                     0,            NULL   },
    { (char *)"Ch&eats",                    onCheats,                  NULL,                     0,            NULL   },
    { (char *)"&Items",                     onCustomItems,             NULL,                     0,            NULL   },
    { (char *)"Ene&mies",                   onCustomEnemies,           NULL,                     0,            NULL   },
    { (char *)"&Strings",                   onStrings,                 NULL,                     0,            NULL   },
    { (char *)"&DMaps",                     onDmaps,                   NULL,                     0,            NULL   },
    { (char *)"I&nit Data",                 onInit,                    NULL,                     0,            NULL   },
    { (char *)"Misc D&ata\t ",              NULL,                      misc_menu,                0,            NULL   },
    { (char *)"",                           NULL,                      NULL,                     0,            NULL   },
    { (char *)"&Graphics\t ",               NULL,                      graphics_menu,            0,            NULL   },
    { (char *)"A&udio\t ",                  NULL,                      audio_menu,               0,            NULL   },
    { (char *)"S&cripts\t ",                NULL,                      script_menu,              0,            NULL   },
    { (char *)"",                           NULL,                      NULL,                     0,            NULL   },
    { (char *)"&Template",                  onTemplates,               NULL,                     0,            NULL   },
    { (char *)"De&faults\t ",               NULL,                      defs_menu,                0,            NULL   },
    {  NULL,                                NULL,                      NULL,                     0,            NULL   }
};

static MENU paste_menu[] =
{
    { (char *)"Paste &To All",              onPasteToAll,              NULL,                     0,            NULL   },
    { (char *)"Paste &All To All",          onPasteAllToAll,           NULL,                     0,            NULL   },
    {  NULL,                                NULL,                      NULL,                     0,            NULL   }
};

static MENU paste_item_menu[] =
{
    { (char *)"&Undercombo",                onPasteUnderCombo,         NULL,                     0,            NULL   },
    { (char *)"&Secret Combos",             onPasteSecretCombos,       NULL,                     0,            NULL   },
    { (char *)"&Freeform Combos",           onPasteFFCombos,           NULL,                     0,            NULL   },
    { (char *)"Screen &Data",               onPasteScreenData,         NULL,                     0,            NULL   },
    { (char *)"&Warps",                     onPasteWarps,              NULL,                     0,            NULL   },
    { (char *)"Warp &Return",               onPasteWarpLocations,      NULL,                     0,            NULL   },
    { (char *)"&Enemies",                   onPasteEnemies,            NULL,                     0,            NULL   },
    { (char *)"Room &Type Data",            onPasteRoom,               NULL,                     0,            NULL   },
    { (char *)"&Guy/String",                onPasteGuy,                NULL,                     0,            NULL   },
    { (char *)"Doo&rs",                     onPasteDoors,              NULL,                     0,            NULL   },
    { (char *)"&Layers",                    onPasteLayers,             NULL,                     0,            NULL   },
    { (char *)"&Palette",                   onPastePalette,            NULL,                     0,            NULL   },
    {  NULL,                                NULL,                      NULL,                     0,            NULL   }
};

static MENU edit_menu[] =
{
    { (char *)"&Undo\tU",                   onUndo,                    NULL,                     0,            NULL   },
    { (char *)"&Copy\tC",                   onCopy,                    NULL,                     0,            NULL   },
    { (char *)"&Paste\tV",                  onPaste,                   NULL,                     0,            NULL   },
    { (char *)"Paste A&ll",                 onPasteAll,                NULL,                     0,            NULL   },
    { (char *)"&Adv. Paste\t ",             NULL,                      paste_menu,               0,            NULL   },
    { (char *)"Paste &Spec.\t ",            NULL,                      paste_item_menu,          0,            NULL   },
    { (char *)"&Delete\tDel",               onDelete,                  NULL,                     0,            NULL   },
    { (char *)"",                           NULL,                      NULL,                     0,            NULL   },
    { (char *)"&Maps\t ",                   NULL,                      maps_menu,                0,            NULL   },
    {  NULL,                                NULL,                      NULL,                     0,            NULL   }
};

static MENU drawing_mode_menu[] =
{
    { (char *)"&Normal",                    onDrawingModeNormal,       NULL,                     0,            NULL   },
    { (char *)"&Relational",                onDrawingModeRelational,   NULL,                     0,            NULL   },
    { (char *)"&Dungeon Carving",           onDrawingModeDungeon,      NULL,                     0,            NULL   },
    { (char *)"&Combo Alias",               onDrawingModeAlias,        NULL,                     0,            NULL   },
    {  NULL,                                NULL,                      NULL,                     0,            NULL   }
};

static MENU integrity_check_menu[] =
{
    { (char *)"&All ",                      onIntegrityCheckAll,       NULL,                     0,            NULL   },
    { (char *)"&Warps ",                    onIntegrityCheckWarps,     NULL,                     0,            NULL   },
    { (char *)"&Screens ",                  onIntegrityCheckRooms,     NULL,                     0,            NULL   },
    {  NULL,                                NULL,                      NULL,                     0,            NULL   }
};

static MENU quest_reports_menu[] =
{
    { (char *)"&Combo Locations",           onComboLocationReport,     NULL,                     0,            NULL   },
    { (char *)"&Combo Type Locations",      onComboTypeLocationReport, NULL,                     0,            NULL   },
    { (char *)"&Enemy Locations",           onEnemyLocationReport,     NULL,                     0,            NULL   },
    { (char *)"&Item Locations",            onItemLocationReport,      NULL,                     0,            NULL   },
    { (char *)"&Script Locations",          onScriptLocationReport,    NULL,                     0,            NULL   },
    { (char *)"&What Links Here",           onWhatWarpsReport,         NULL,                     0,            NULL   },
    { (char *)"In&tegrity Check\t ",        NULL,                      integrity_check_menu,     0,            NULL   },
    {  NULL,                                NULL,                      NULL,                     0,            NULL   }
};

static MENU tool_menu[] =
{
    { (char *)"Combo &Flags\tF8",           onFlags,                   NULL,                     0,            NULL   },
    { (char *)"&Color Set Fix",             onCSetFix,                 NULL,                     0,            NULL   },
    { (char *)"&NES Dungeon Template",      onTemplate,                NULL,                     0,            NULL   },
    { (char *)"&Apply Template to All",     onReTemplate,              NULL,                     0,            NULL   },
    { (char *)"",                           NULL,                      NULL,                     0,            NULL   },
    { (char *)"&Preview Mode\tX",           onPreviewMode,             NULL,                     0,            NULL   },
    { (char *)"Drawing &Mode\t ",           NULL,                      drawing_mode_menu,        0,            NULL   },
    { (char *)"",                           NULL,                      NULL,                     0,            NULL   },
    { (char *)"&List Combos Used\t'",       onUsedCombos,              NULL,                     0,            NULL   },
    { (char *)"&Quest Reports\t ",          NULL,                      quest_reports_menu,       0,            NULL   },
    {  NULL,                                NULL,                      NULL,                     0,            NULL   }
};

static MENU view_menu[] =
{
    { (char *)"View &Map...",               onViewMap,                 NULL,                     0,            NULL   },
    { (char *)"View &Palette",              onShowPal,                 NULL,                     0,            NULL   },
    { (char *)"",                           NULL,                      NULL,                     0,            NULL   },
    { (char *)"Show &Walkability\tW",       onShowWalkability,         NULL,                     0,            NULL   },
    { (char *)"Show &Flags\tF",             onShowFlags,               NULL,                     0,            NULL   },
    { (char *)"Show &CSets",                onShowCSet,                NULL,                     0,            NULL   },
    { (char *)"Show &Types",                onShowCType,               NULL,                     0,            NULL   },
    { (char *)"",                           NULL,                      NULL,                     0,            NULL   },
    { (char *)"Show Screen &Info\tN",       onToggleShowInfo,          NULL,                     0,            NULL   },
    { (char *)"Show &Squares",              onToggleShowSquares,       NULL,                     0,            NULL   },
    { (char *)"Show Script &Names",         onToggleShowScripts,       NULL,                     0,            NULL   },
    { (char *)"Show &Grid\t~",              onToggleGrid,              NULL,                     0,            NULL   },
    {  NULL,                                NULL,                      NULL,                     0,            NULL   }
};

int onSelectFFCombo();

static MENU data_menu[] =
{
    { (char *)"&Screen Data\tF9",           onScrData,                 NULL,                     0,            NULL   },
    { (char *)"&Freeform Combos\tF7",       onSelectFFCombo,           NULL,                     0,            NULL   },
    { (char *)"La&yers\tF12",               onLayers,                  NULL,                     0,            NULL   },
    { (char *)"&Tile Warp\tF10",            onTileWarp,                NULL,                     0,            NULL   },
    { (char *)"Side &Warp\tF11",            onSideWarp,                NULL,                     0,            NULL   },
    { (char *)"Secret &Combos\tF5",         onSecretCombo,             NULL,                     0,            NULL   },
    { (char *)"&Under Combo",               onUnderCombo,              NULL,                     0,            NULL   },
    { (char *)"&Doors\tF6",                 onDoors,                   NULL,                     0,            NULL   },
    { (char *)"Ma&ze Path",                 onPath,                    NULL,                     0,            NULL   },
    { (char *)"",                           NULL,                      NULL,                     0,            NULL   },
    { (char *)"&Guy\tG",                    onGuy,                     NULL,                     0,            NULL   },
    { (char *)"&Message String\tS",         onString,                  NULL,                     0,            NULL   },
    { (char *)"&Room Type\tR",              onRType,                   NULL,                     0,            NULL   },
    { (char *)"Catch &All\tA",              onCatchall,                NULL,                     D_DISABLED,   NULL   },
    { (char *)"",                           NULL,                      NULL,                     0,            NULL   },
    { (char *)"&Item\tI",                   onItem,                    NULL,                     0,            NULL   },
    { (char *)"&Enemies\tE",                onEnemies,                 NULL,                     0,            NULL   },
    { (char *)"&Palette\tF4",               onScreenPalette,           NULL,                     0,            NULL   },
    {  NULL,                                NULL,                      NULL,                     0,            NULL   }
};

static MENU tunes_menu[] =
{
    { (char *)"Wind Fish",				  playTune1,                 NULL,                     0,            NULL   },
    { (char *)"Overworld",				  playTune2,                 NULL,                     0,            NULL   },
    { (char *)"Hyrule Castle",			  playTune3,                 NULL,                     0,            NULL   },
    { (char *)"Lost Woods",			      playTune4,                 NULL,                     0,            NULL   },
    { (char *)"Great Sea",				  playTune5,                 NULL,                     0,            NULL   },
    { (char *)"East Hyrule",				  playTune6,                 NULL,                     0,            NULL   },
    { (char *)"Dancing Dragon",			  playTune7,                 NULL,                     0,            NULL   },
    { (char *)"Stone Tower",				  playTune8,                 NULL,                     0,            NULL   },
    { (char *)"Villages",				      playTune9,                 NULL,                     0,            NULL   },
    { (char *)"Swamp + Desert",		      playTune10,                NULL,                     0,            NULL   },
    { (char *)"Outset Island",			  playTune11,                NULL,                     0,            NULL   },
    { (char *)"Kakariko Village",			  playTune12,                NULL,                     0,            NULL   },
    { (char *)"Clock Town",				  playTune13,                NULL,                     0,            NULL   },
    { (char *)"Temple",				      playTune14,                NULL,                     0,            NULL   },
    { (char *)"Dark World",				  playTune15,                NULL,                     0,            NULL   },
    { (char *)"Dragon Roost",				  playTune16,                NULL,                     0,            NULL   },
    { (char *)"Horse Race",				  playTune17,                NULL,                     0,            NULL   },
    { (char *)"Credits",				      playTune18,                NULL,                     0,            NULL   },
    { (char *)"Zelda's Lullaby",			  playTune19,                NULL,                     0,            NULL   },
    {  NULL,                                NULL,                      NULL,                     0,            NULL   }
};

int onFullscreenMenu();
int onWindowed1Menu();
int onWindowed2Menu();
int onWindowed4Menu();
int onNativeDepthMenu();

static MENU video_mode_menu[] =
{
	{ (char *)"&Fullscreen",				onFullscreenMenu,		 NULL,                      0, NULL },
	{ (char *)"Windowed 320x240",		    onWindowed1Menu,		 NULL,                      0, NULL },
	{ (char *)"&Windowed 800x600",		    onWindowed2Menu,		 NULL,                      0, NULL },
	{ (char *)"&Windowed 1600x1200",		onWindowed4Menu,		 NULL,                      0, NULL },
	{ (char *)"Native Color &Depth",        onNativeDepthMenu,       NULL,                      0, NULL },
	{ NULL,                                 NULL,                    NULL,                      0, NULL }
};

static MENU etc_menu[] =
{
    { (char *)"&Help",                      onHelp,                    NULL,                     0,            NULL   },
    { (char *)"&About",                     onAbout,                   NULL,                     0,            NULL   },
    { (char *)"About Video Mode...",        onZQVidMode,               NULL,                     0,            NULL   },
    { (char *)"&Options...",                onOptions,                 NULL,                     0,            NULL   },
    { (char *)"Set Video &Mode",            NULL,			           video_mode_menu,          0,            NULL   },
    { (char *)"",                           NULL,                      NULL,                     0,            NULL   },
    { (char *)"&View Pic...",               onViewPic,                 NULL,                     0,            NULL   },
    { (char *)"",                           NULL,                      NULL,                     0,            NULL   },
    { (char *)"The Travels of Link",        NULL,                      tunes_menu,               0,            NULL   },
    { (char *)"&Play music",                playMusic,                 NULL,                     0,            NULL   },
    { (char *)"&Change track",              changeTrack,               NULL,                     0,            NULL   },
    { (char *)"&Stop tunes",                stopMusic,                 NULL,                     0,            NULL   },
    { (char *)"",                           NULL,                      NULL,                     0,            NULL   },
    { (char *)"&Take Snapshot\tZ",          onSnapshot,                NULL,                     0,            NULL   },
    {  NULL,                                NULL,                      NULL,                     0,            NULL   }
};

static MENU zquest_menu[] =
{
    { (char *)"&File",                      NULL, (MENU *) file_menu,       0,            NULL   },
    { (char *)"&Quest",                     NULL, (MENU *) quest_menu,      0,            NULL   },
    { (char *)"&Edit",                      NULL, (MENU *) edit_menu,       0,            NULL   },
    { (char *)"&View",                      NULL, (MENU *) view_menu,       0,            NULL   },
    { (char *)"&Tools",                     NULL, (MENU *) tool_menu,       0,            NULL   },
    { (char *)"&Screen",                    NULL, (MENU *) data_menu,       0,            NULL   },
    { (char *)"Et&c",                       NULL, (MENU *) etc_menu,        0,            NULL   },
    {  NULL,                                NULL,                      NULL,                     0,            NULL   }
};

void rebuild_trans_table();
int launchPicViewer(BITMAP **pictoview, PALETTE pal,
                    int *px2, int *py2, double *scale, bool isviewingmap);

int onResetTransparency()
{
    restore_mouse();
    rebuild_trans_table();
    jwin_alert("Notice","Translucency Table Rebuilt",NULL,NULL,"OK",NULL,13,27,lfont);
    
    refresh(rALL);
    return D_O_K;
}

bool checkResolutionFits(int width, int height)
{
	if (Backend::graphics->desktopW() < width || Backend::graphics->desktopH() < height)
	{
		return (jwin_alert("Resolution Warning", "The chosen resolution is too", "big to fit the screen.", "Use it anyway?", "&Yes", "&No", 'y', 'n', is_large() ? lfont_l : font) == 1);
	}
	return true;
}
    
void setVideoModeMenuFlags()
{
	video_mode_menu[0].flags = (Backend::graphics->isFullscreen() ? D_SELECTED : 0);
	video_mode_menu[1].flags = (!Backend::graphics->isFullscreen() && Backend::graphics->screenW() == 320 && Backend::graphics->screenH() == 240) ? D_SELECTED : 0;
	video_mode_menu[2].flags = (!Backend::graphics->isFullscreen() && Backend::graphics->screenW() == 800 && Backend::graphics->screenH() == 600) ? D_SELECTED : 0;	
	video_mode_menu[3].flags = (!Backend::graphics->isFullscreen() && Backend::graphics->screenW() == 1600 && Backend::graphics->screenH() == 1200) ? D_SELECTED : 0;
	video_mode_menu[4].flags = (Backend::graphics->isNativeColorDepth() ? D_SELECTED : 0);
}

int onNativeDepthMenu()
{
	Backend::graphics->setUseNativeColorDepth(!Backend::graphics->isNativeColorDepth());
	setVideoModeMenuFlags();
	gui_bg_color = jwin_pal[jcBOX];
	gui_fg_color = jwin_pal[jcBOXFG];
	gui_mg_color = jwin_pal[jcMEDDARK];
	Backend::mouse->setCursorSprite(mouse_bmp[MOUSE_BMP_NORMAL][0]);
	Backend::palette->setPalette(RAMpal);
	int x = Backend::graphics->screenW() / 2;
	int y = Backend::graphics->screenH() / 2;
	Backend::mouse->setVirtualScreenPos(x, y);
	Backend::mouse->setCursorVisibility(true);
	return D_REDRAW;
}


int onFullscreenMenu()
{
	Backend::graphics->setFullscreen(true);
	setVideoModeMenuFlags();
	gui_bg_color = jwin_pal[jcBOX];
	gui_fg_color = jwin_pal[jcBOXFG];
	gui_mg_color = jwin_pal[jcMEDDARK];
	Backend::mouse->setCursorSprite(mouse_bmp[MOUSE_BMP_NORMAL][0]);
	Backend::palette->setPalette(RAMpal);
	int x = Backend::graphics->screenW() / 2;
	int y = Backend::graphics->screenH() / 2;
	Backend::mouse->setVirtualScreenPos(x, y);
	Backend::mouse->setCursorVisibility(true);
	return D_REDRAW;
}

int onWindowed1Menu()
{
	Backend::graphics->setScreenResolution(320, 240);
	Backend::graphics->setFullscreen(false);
	setVideoModeMenuFlags();
	gui_bg_color = jwin_pal[jcBOX];
	gui_fg_color = jwin_pal[jcBOXFG];
	gui_mg_color = jwin_pal[jcMEDDARK];
	Backend::mouse->setCursorSprite(mouse_bmp[MOUSE_BMP_NORMAL][0]);
	Backend::palette->setPalette(RAMpal);
	int x = Backend::graphics->screenW() / 2;
	int y = Backend::graphics->screenH() / 2;
	Backend::mouse->setVirtualScreenPos(x, y);
	Backend::mouse->setCursorVisibility(true);
	return D_REDRAW;
}

int onWindowed2Menu()
{
	if (checkResolutionFits(800, 600))
	{
		Backend::graphics->setScreenResolution(800, 600);
		Backend::graphics->setFullscreen(false);
		setVideoModeMenuFlags();
		gui_bg_color = jwin_pal[jcBOX];
		gui_fg_color = jwin_pal[jcBOXFG];
		gui_mg_color = jwin_pal[jcMEDDARK];
		Backend::mouse->setCursorSprite(mouse_bmp[MOUSE_BMP_NORMAL][0]);
		Backend::palette->setPalette(RAMpal);
		int x = Backend::graphics->screenW() / 2;
		int y = Backend::graphics->screenH() / 2;
		Backend::mouse->setVirtualScreenPos(x, y);
		Backend::mouse->setCursorVisibility(true);
	}
	return D_REDRAW;
}

int onWindowed4Menu()
{
	if (checkResolutionFits(1600, 1200))
	{
		Backend::graphics->setScreenResolution(1600, 1200);
		Backend::graphics->setFullscreen(false);
		setVideoModeMenuFlags();
		gui_bg_color = jwin_pal[jcBOX];
		gui_fg_color = jwin_pal[jcBOXFG];
		gui_mg_color = jwin_pal[jcMEDDARK];
		Backend::mouse->setCursorSprite(mouse_bmp[MOUSE_BMP_NORMAL][0]);
		Backend::palette->setPalette(RAMpal);
		int x = Backend::graphics->screenW() / 2;
		int y = Backend::graphics->screenH() / 2;
		Backend::mouse->setVirtualScreenPos(x, y);
		Backend::mouse->setCursorVisibility(true);
	}
	return D_REDRAW;
}

int onEnter()
{
    return D_O_K;
}

//PROC, x, y, w, h, fg, bg, key, flags, d1, d2, *dp, *dp2, *dp3

//*text, (*proc), *child, flags, *dp

int d_nbmenu_proc(int msg,DIALOG *d,int c);


/*int onY()
{
  return D_O_K;
}*/

int onToggleGrid()
{
    if(key[KEY_ZC_LCONTROL] || key[KEY_ZC_RCONTROL])
    {
        GridColor=(GridColor+8)%16;
    }
    else
    {
        ShowGrid=!ShowGrid;
    }
    
    return D_O_K;
}

int onToggleShowScripts()
{
    ShowFFScripts=!ShowFFScripts;
    return D_O_K;
}

int onToggleShowSquares()
{
    ShowSquares=!ShowSquares;
    return D_O_K;
}

int onToggleShowInfo()
{
    ShowInfo=!ShowInfo;
    return D_O_K;
}

static DIALOG dialogs[] =
{
    // still unused:  jm
    /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key)    (flags)  (d1)         (d2)     (dp) */
    { d_nbmenu_proc,     0,    0,    0,    13,    0,    0,    0,       D_USER,  0,             0, (void *) zquest_menu, NULL, NULL },
    
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    '=',     0,       0,              0, (void *) onIncreaseCSet, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    '-',     0,       0,              0, (void *) onDecreaseCSet, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    '*',     0,       0,              0, (void *) onIncreaseFlag, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_CLOSEBRACE, 0, (void *) onIncreaseFlag, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    '/',     0,       0,              0, (void *) onDecreaseFlag, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_OPENBRACE,  0, (void *) onDecreaseFlag, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_UP,         0, (void *) onUp, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_DOWN,       0, (void *) onDown, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_LEFT,       0, (void *) onLeft, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_RIGHT,      0, (void *) onRight, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_PGUP,       0, (void *) onPgUp, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_PGDN,       0, (void *) onPgDn, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_PLUS_PAD,   0, (void *) onIncreaseCSet, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_MINUS_PAD,  0, (void *) onDecreaseCSet, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_ASTERISK,   0, (void *) onIncreaseFlag, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_SLASH_PAD,  0, (void *) onDecreaseFlag, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_F1,         0, (void *) onHelp, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_F2,         0, (void *) onSave, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_F3,         0, (void *) onOpen, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_F4,         0, (void *) onScreenPalette, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_F5,         0, (void *) onSecretCombo, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_F6,         0, (void *) onDoors, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_F7,         0, (void *) onSelectFFCombo, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_F8,         0, (void *) onFlags, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_F9,         0, (void *) onScrData, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_F10,        0, (void *) onTileWarp, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_F11,        0, (void *) onSideWarp, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_F12,        0, (void *) onLayers, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_ESC,        0, (void *) onExit, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_A,          0, (void *) onCatchall, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_B,          0, (void *) onResetTransparency, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_C,          0, (void *) onCopy, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_D,          0, (void *) onToggleDarkness, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_E,          0, (void *) onEnemies, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_F,          0, (void *) onShowFlags, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_G,          0, (void *) onGuy, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_H,          0, (void *) onH, NULL, NULL },      //Flip
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_I,          0, (void *) onItem, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_J,          0, (void *) onJ, NULL, NULL },      //This does nothing
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_K,          0, (void *) onCombos, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_L,          0, (void *) onShowDarkness, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_M,          0, (void *) onM, NULL, NULL },      // This does nothing
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_N,          0, (void *) onToggleShowInfo, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_O,          0, (void *) onDrawingMode, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_P,          0, (void *) onGotoPage, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_Q,          0, (void *) onShowComboInfoCSet, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_R,          0, (void *) onRType, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_S,          0, (void *) onString, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_T,          0, (void *) onTiles, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_U,          0, (void *) onUndo, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_V,          0, (void *) onPaste, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_W,          0, (void *) onShowWalkability, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_X,          0, (void *) onPreviewMode, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_Y,          0, (void *) onCompileScript, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_Z,          0, (void *) onSnapshot, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    '0',     0,       0,              0, (void *) on0, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    '1',     0,       0,              0, (void *) on1, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    '2',     0,       0,              0, (void *) on2, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    '3',     0,       0,              0, (void *) on3, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    '4',     0,       0,              0, (void *) on4, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    '5',     0,       0,              0, (void *) on5, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    '6',     0,       0,              0, (void *) on6, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    '7',     0,       0,              0, (void *) on7, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    '8',     0,       0,              0, (void *) on8, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    '9',     0,       0,              0, (void *) on9, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    ')',     0,       0,              0, (void *) on10, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    '!',     0,       0,              0, (void *) on11, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    '@',     0,       0,              0, (void *) on12, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    '#',     0,       0,              0, (void *) on13, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    '$',     0,       0,              0, (void *) on14, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    ',',     0,       0,              0, (void *) onDecMap, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    '.',     0,       0,              0, (void *) onIncMap, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    '<',     0,       0,              0, (void *) onDecScrPal, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    '>',     0,       0,              0, (void *) onIncScrPal, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_TILDE,      0, (void *) onToggleGrid, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    39,      0,       0,              0, (void *) onUsedCombos, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_SPACE,      0, (void *) onSpacebar, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_DEL,        0, (void *) onDelete, NULL, NULL },      //
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_ENTER,      0, (void *) onEnter, NULL, NULL },
    { NULL,              0,    0,    0,    0,    0,    0,    0,       0,       0,              0,       NULL, NULL, NULL }
};

static DIALOG getnum_dlg[] =
{
    // (dialog proc)       (x)   (y)    (w)     (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp)
    { jwin_win_proc,        80,   80,     160,    72,   vc(0),              vc(11),           0,       D_EXIT,     0,             0,       NULL, NULL, NULL },
    { jwin_rtext_proc,      114,  104+4,  48,     8,    jwin_pal[jcBOXFG],  jwin_pal[jcBOX],  0,       0,          0,             0, (void *) "Value:", NULL, NULL },
    { jwin_edit_proc,       168,  104,    48,     16,    0,                 0,                0,       0,          6,             0,       NULL, NULL, NULL },
    { jwin_button_proc,     90,   126,    61,     21,   vc(0),              vc(11),           13,      D_EXIT,     0,             0, (void *) "OK", NULL, NULL },
    { jwin_button_proc,     170,  126,    61,     21,   vc(0),              vc(11),           27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

int getnumber(const char *prompt,int initialval)
{
    cancelgetnum=true;
    char buf[20];
    sprintf(buf,"%d",initialval);
    getnum_dlg[0].dp=(void *)prompt;
    getnum_dlg[0].dp2=lfont;
    getnum_dlg[2].dp=(void *)buf;

	DIALOG *getnum_cpy = resizeDialog(getnum_dlg, 1.5);
    
    int ret=zc_popup_dialog(getnum_cpy,2);
	delete[] getnum_cpy;
    
    if(ret!=0&&ret!=4)
    {
        cancelgetnum=false;
    }
    
    if(ret==3)
        return atoi(buf);
        
    return initialval;
}

int gettilepagenumber(const char *prompt, int initialval)
{
    char buf[20];
    sprintf(buf,"%d",initialval);
    getnum_dlg[0].dp=(void *)prompt;
    getnum_dlg[0].dp2=lfont;
    getnum_dlg[2].dp=buf;

	DIALOG *getnum_cpy = resizeDialog(getnum_dlg, 1.5);
    
    int ret = zc_popup_dialog(getnum_cpy,2);

	delete[] getnum_cpy;
    
    if(ret==3)
        return atoi(buf);
        
    return -1;
}

int gethexnumber(const char *prompt,int initialval)
{
    cancelgetnum=true;
    char buf[20];
    sprintf(buf,"%X",initialval);
    getnum_dlg[0].dp=(void *)prompt;
    getnum_dlg[0].dp2=lfont;
    getnum_dlg[2].dp=(void *)buf;

	DIALOG *getnum_cpy = resizeDialog(getnum_dlg, 1.5);
    
    int ret=zc_popup_dialog(getnum_cpy,2);

	delete[] getnum_cpy;
    
    if(ret!=0&&ret!=4)
    {
        cancelgetnum=false;
    }
    
    if(ret==3)
        return xtoi(buf);
        
    return initialval;
}

void update_combo_cycling()
{
    Map.update_combo_cycling();
}

void update_freeform_combos()
{
    Map.update_freeform_combos();
}

bool layers_valid(mapscr *tempscr)
{
    for(int i=0; i<6; i++)
    {
        if(tempscr->layermap[i]>map_count)
        {
            return false;
        }
    }
    
    return true;
}

void fix_layers(mapscr *tempscr, bool showwarning)
{
    char buf[80]="layers have been changed: ";
    
    for(int i=0; i<6; i++)
    {
        if(tempscr->layermap[i]>map_count)
        {
            strcat(buf, "%d ");
            sprintf(buf, buf, i+1);
            tempscr->layermap[i]=0;
        }
    }
    
    if(showwarning)
    {
        jwin_alert("Invalid layers detected",
                   "One or more layers on this screen used",
                   "maps that do not exist. The settings of these",
                   buf, "O&K", NULL, 'o', 0, lfont);
    }
}

/***********************/
/*** dialog handlers ***/
/***********************/

extern const char *colorlist(int index, int *list_size);

static char autobackup_str_buf[32];
const char *autobackuplist(int index, int *list_size)
{
    if(index>=0)
    {
        bound(index,0,10);
        
        if(index==0)
        {
            sprintf(autobackup_str_buf,"Disabled");
        }
        else
        {
            sprintf(autobackup_str_buf,"%2d",index);
        }
        
        return autobackup_str_buf;
    }
    
    *list_size=11;
    return NULL;
}

static char autosave_str_buf[32];
const char *autosavelist(int index, int *list_size)
{
    if(index>=0)
    {
        if(index==0)
            sprintf(autosave_str_buf, "Disabled");
        else if(index<=10)
            sprintf(autosave_str_buf, "%d Minute%c", index, index>1 ? 's' : 0);
        else if(index==11)
            sprintf(autosave_str_buf, "15 Minutes");
        else if(index==12)
            sprintf(autosave_str_buf, "30 Minutes");
        else if(index==13)
            sprintf(autosave_str_buf, "45 Minutes");
        else if(index==14)
            sprintf(autosave_str_buf, "1 Hour");
        else if(index==15)
            sprintf(autosave_str_buf, "90 Minutes");
        else if(index==16)
            sprintf(autosave_str_buf, "2 Hours");
        
        return autosave_str_buf;
    }
    
    *list_size=17;
    return NULL;
}

int autosaveListToMinutes(int index)
{
    switch(index)
    {
        case 11: return 15;
        case 12: return 30;
        case 13: return 45;
        case 14: return 60;
        case 15: return 90;
        case 16: return 120;
        default: return index;
    }
}

int autosaveMinutesToList(int time)
{
    if(time<=0)
        return 0;
    if(time<10)
        return time;
    
    // Use <= so, say, 12 minutes doesn't become 2 hours
    if(time<=15)
        return 11;
    if(time<=30)
        return 12;
    if(time<=45)
        return 13;
    if(time<=60)
        return 14;
    if(time<=90)
        return 15;
    return 16;
}

const char *autosavelist2(int index, int *list_size)
{
    if(index>=0)
    {
        bound(index,0,9);
        sprintf(autosave_str_buf,"%2d",index+1);
        return autosave_str_buf;
    }
    
    *list_size=10;
    return NULL;
}


static int options_1_list[] =
{
    // dialog control number
    4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, -1
};

static int options_2_list[] =
{
    // dialog control number
    31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, -1
};

static TABPANEL options_tabs[] =
{
    // (text)
    { (char *)" 1 ",       D_SELECTED,   options_1_list,  0, NULL },
    { (char *)" 2 ",       0,            options_2_list,  0, NULL },
    { NULL,                0,            NULL, 0, NULL }
};

static ListData autobackup_list(autobackuplist, &font);
static ListData autosave_list(autosavelist, &font);
static ListData autosave_list2(autosavelist2, &font);
static ListData color_list(colorlist, &font);
static ListData snapshotformat_list(snapshotformatlist, &font);

static DIALOG options_dlg[] =
{
    /* (dialog proc)           (x)     (y)     (w)     (h)    (fg)        (bg)      (key)    (flags)    (d1)  (d2) (dp) */
    { jwin_win_proc,            0,      0,    260,    238,    vc(14),     vc(1),       0,    D_EXIT,     0,    0, (void *) "ZQuest Options",                                              NULL,   NULL                },
    { jwin_tab_proc,            4,     23,    252,    182,    vc(0),      vc(15),      0,    0,          0,    0, (void *) options_tabs,                                                  NULL, (void *)options_dlg },
    { jwin_button_proc,        60,    212,     61,     21,    vc(14),     vc(1),      13,    D_EXIT,     0,    0, (void *) "OK",                                                          NULL,   NULL                },
    { jwin_button_proc,       140,    212,     61,     21,    vc(14),     vc(1),      27,    D_EXIT,     0,    0, (void *) "Cancel",                                                      NULL,   NULL                },
    // 4
    { jwin_check_proc,         12,     44,    129,      9,    vc(14),     vc(1),       0,    0,          1,    0, (void *) "Mouse scroll",                                                NULL,   NULL                },
    { jwin_check_proc,         12,     54,    129,      9,    vc(14),     vc(1),       0,    0,          1,    0, (void *) "Save paths",                                                  NULL,   NULL                },
    { jwin_check_proc,         12,     64,    129,      9,    vc(14),     vc(1),       0,    0,          1,    0, (void *) "Palette cycle",                                               NULL,   NULL                },
    { jwin_check_proc,         12,     74,    129,      9,    vc(14),     vc(1),       0,    0,          1,    0, (void *) "Show Frames Per Second",                                      NULL,   NULL                },
    { jwin_check_proc,         12,     84,    129,      9,    vc(14),     vc(1),       0,    0,          1,    0, (void *) "Combo Brush",			                                      NULL,   NULL                },
    { jwin_check_proc,         12,     94,    129,      9,    vc(14),     vc(1),       0,    0,          1,    0, (void *) "Floating Brush",                                              NULL,   NULL                },
    { jwin_check_proc,         12,    104,    129,      9,    vc(14),     vc(1),       0,    0,          1,    0, (void *) "Reload Last Quest",                                           NULL,   NULL                },
    { jwin_check_proc,         12,    114,    129,      9,    vc(14),     vc(1),       0,    0,          1,    0, (void *) "Show Misaligns",                                              NULL,   NULL                },
    { jwin_check_proc,         12,    124,    129,      9,    vc(14),     vc(1),       0,    0,          1,    0, (void *) "Animate Combos",                                              NULL,   NULL                },
    { jwin_check_proc,         12,    134,    129,      9,    vc(14),     vc(1),       0,    0,          1,    0, (void *) "Overwrite Protection",                                        NULL,   NULL                },
    { jwin_check_proc,         12,    144,    129,      9,    vc(14),     vc(1),       0,    0,          1,    0, (void *) "Tile Protection",                                             NULL,   NULL                },
    { jwin_check_proc,         12,    154,    129,      9,    vc(14),     vc(1),       0,    0,          1,    0, (void *) "Use Static for Invalid Data",                                 NULL,   NULL                },
    { jwin_check_proc,         12,    164,    129,      9,    vc(14),     vc(1),       0,    0,          1,    0, (void *) "Show Ruleset Dialog When Creating New Quests",                NULL,   NULL                },
    { jwin_check_proc,         12,    174,    129,      9,    vc(14),     vc(1),       0,    0,          1,    0, (void *) "Show Ruleset Dialog When Creating New Quests",				  NULL,   NULL                },
    { jwin_check_proc,         12,    184,    129,      9,    vc(14),     vc(1),       0,    0,          1,    0, (void *) "Enable Tooltips",								              NULL,   NULL                },
    { d_dummy_proc,            12,    194,    129,      9,    vc(14),     vc(1),       0,    0,          1,    0, NULL,						                                              NULL,   NULL                },
    
    // 20
    { d_dummy_proc,             0,      0,      0,      0,    vc(14),     vc(1),       0,    0,          0,    0,  NULL,                                                                   NULL,   NULL                },
    { d_dummy_proc,             0,      0,      0,      0,    vc(14),     vc(1),       0,    0,          0,    0,  NULL,                                                                   NULL,   NULL                },
    { d_dummy_proc,             0,      0,      0,      0,    vc(14),     vc(1),       0,    0,          0,    0,  NULL,                                                                   NULL,   NULL                },
    { d_dummy_proc,             0,      0,      0,      0,    vc(14),     vc(1),       0,    0,          0,    0,  NULL,                                                                   NULL,   NULL                },
    { d_dummy_proc,             0,      0,      0,      0,    vc(14),     vc(1),       0,    0,          0,    0,  NULL,                                                                   NULL,   NULL                },
    { d_dummy_proc,             0,      0,      0,      0,    vc(14),     vc(1),       0,    0,          0,    0,  NULL,                                                                   NULL,   NULL                },
    { d_dummy_proc,             0,      0,      0,      0,    vc(14),     vc(1),       0,    0,          0,    0,  NULL,                                                                   NULL,   NULL                },
    { d_dummy_proc,             0,      0,      0,      0,    vc(14),     vc(1),       0,    0,          0,    0,  NULL,                                                                   NULL,   NULL                },
    { d_dummy_proc,             0,      0,      0,      0,    vc(14),     vc(1),       0,    0,          0,    0,  NULL,                                                                   NULL,   NULL                },
    { d_dummy_proc,             0,      0,      0,      0,    vc(14),     vc(1),       0,    0,          0,    0,  NULL,                                                                   NULL,   NULL                },
    { d_dummy_proc,             0,      0,      0,      0,    vc(14),     vc(1),       0,    0,          0,    0,  NULL,                                                                   NULL,   NULL                },
    
    // 31
    { jwin_text_proc,          12,     48,    129,      9,    0,          0,           0,    0,          0,    0, (void *) "Auto-backup Retention:",                                      NULL,   NULL                },
    { jwin_droplist_proc,     120,     44,     73,     16,    0,          0,           0,    0,          0,    0, (void *) &autobackup_list,                                              NULL,   NULL                },
    { jwin_text_proc,          12,     66,    129,      9,    0,          0,           0,    0,          0,    0, (void *) "Auto-save Interval:",                                         NULL,   NULL                },
    { jwin_droplist_proc,     105,     62,     86,     16,    0,          0,           0,    0,          0,    0, (void *) &autosave_list,                                                NULL,   NULL                },
    { jwin_text_proc,          12,     84,    129,      9,    0,          0,           0,    0,          0,    0, (void *) "Auto-save Retention:",                                        NULL,   NULL                },
    { jwin_droplist_proc,     111,     80,     49,     16,    0,          0,           0,    0,          0,    0, (void *) &autosave_list2,                                               NULL,   NULL                },
    { jwin_check_proc,         12,     98,    129,      9,    vc(14),     vc(1),       0,    0,          1,    0, (void *) "Uncompressed Auto-saves",                                     NULL,   NULL                },
    { jwin_text_proc,          12,    112,    129,      9,    0,          0,           0,    0,          0,    0, (void *) "Grid Color:",                                                 NULL,   NULL                },
    { jwin_droplist_proc,      64,    108,    100,     16,    0,          0,           0,    0,          0,    0, (void *) &color_list,                                                   NULL,   NULL                },
    { jwin_text_proc,          12,    130,    129,      9,    0,          0,           0,    0,          0,    0, (void *) "Snapshot Format:",                                            NULL,   NULL                },
    { jwin_droplist_proc,      93,    126,     55,     16,    0,          0,           0,    0,          0,    0, (void *) &snapshotformat_list,                                          NULL,   NULL                },
    
    // 42
    { jwin_text_proc,          12,    148,    129,      9,    0,          0,           0,    0,          0,    0, (void *) "Keyboard Repeat Delay:",                                      NULL,   NULL                },
    { jwin_edit_proc,         121,    144,     36,     16,    0,          0,           0,    0,          5,    0,  NULL,                                                                   NULL,   NULL                },
    { jwin_text_proc,          12,    166,    129,      9,    0,          0,           0,    0,          0,    0, (void *) "Keyboard Repeat Rate:",                                       NULL,   NULL                },
    { jwin_edit_proc,         121,    162,     36,     16,    0,          0,           0,    0,          5,    0,  NULL,                                                                   NULL,   NULL                },
    { d_dummy_proc,             0,      0,      0,      0,    vc(14),     vc(1),       0,    0,          0,    0,  NULL,                                                                   NULL,   NULL                },
    { d_dummy_proc,             0,      0,      0,      0,    vc(14),     vc(1),       0,    0,          0,    0,  NULL,                                                                   NULL,   NULL                },
    { d_dummy_proc,             0,      0,      0,      0,    vc(14),     vc(1),       0,    0,          0,    0,  NULL,                                                                   NULL,   NULL                },
    { d_dummy_proc,             0,      0,      0,      0,    vc(14),     vc(1),       0,    0,          0,    0,  NULL,                                                                   NULL,   NULL                },
    { d_dummy_proc,             0,      0,      0,      0,    vc(14),     vc(1),       0,    0,          0,    0,  NULL,                                                                   NULL,   NULL                },
    { d_dummy_proc,             0,      0,      0,      0,    vc(14),     vc(1),       0,    0,          0,    0,  NULL,                                                                   NULL,   NULL                },
    { d_dummy_proc,             0,      0,      0,      0,    vc(14),     vc(1),       0,    0,          0,    0,  NULL,                                                                   NULL,   NULL                },
    { d_dummy_proc,             0,      0,      0,      0,    vc(14),     vc(1),       0,    0,          0,    0,  NULL,                                                                   NULL,   NULL                },
    { d_dummy_proc,             0,      0,      0,      0,    vc(14),     vc(1),       0,    0,          0,    0,  NULL,                                                                   NULL,   NULL                },
    { d_dummy_proc,             0,      0,      0,      0,    vc(14),     vc(1),       0,    0,          0,    0,  NULL,                                                                   NULL,   NULL                },
    
    { d_timer_proc,             0,      0,      0,      0,    0,          0,           0,    0,          0,    0,  NULL,                                                                   NULL,   NULL                },
    { NULL,                     0,      0,      0,      0,    0,          0,           0,    0,          0,    0,  NULL,                                                                   NULL,   NULL                }
};

int onOptions()
{
    int OldAutoSaveInterval=AutoSaveInterval;
    char kbdelay[80], kbrate[80];
    sprintf(kbdelay, "%d", KeyboardRepeatDelay);
    sprintf(kbrate, "%d", KeyboardRepeatRate);
    options_dlg[0].dp2=lfont;
    reset_combo_animations();
    reset_combo_animations2();
    go();
    options_dlg[4].flags = MouseScroll ? D_SELECTED : 0;
    options_dlg[5].flags = SavePaths ? D_SELECTED : 0;
    options_dlg[6].flags = CycleOn ? D_SELECTED : 0;
    options_dlg[7].flags = ShowFPS ? D_SELECTED : 0;
    options_dlg[8].flags = ComboBrush ? D_SELECTED : 0;
    options_dlg[9].flags = FloatBrush ? D_SELECTED : 0;
    options_dlg[10].flags = OpenLastQuest ? D_SELECTED : 0;
    options_dlg[11].flags = ShowMisalignments ? D_SELECTED : 0;
    options_dlg[12].flags = AnimationOn ? D_SELECTED : 0;
    options_dlg[13].flags = OverwriteProtection ? D_SELECTED : 0;
    options_dlg[14].flags = TileProtection ? D_SELECTED : 0;
    options_dlg[15].flags = InvalidStatic ? D_SELECTED : 0;
    options_dlg[16].flags = RulesetDialog ? D_SELECTED : 0;
    options_dlg[17].flags = EnableTooltips ? D_SELECTED : 0;
    options_dlg[32].d1 = AutoBackupRetention;
    options_dlg[34].d1 = autosaveMinutesToList(AutoSaveInterval);
    options_dlg[36].d1 = AutoSaveRetention;
    options_dlg[37].flags = UncompressedAutoSaves ? D_SELECTED : 0;
    options_dlg[39].d1 = GridColor;
    options_dlg[41].d1 = SnapshotFormat;
    options_dlg[43].dp = kbdelay;
    options_dlg[45].dp = kbrate;

	DIALOG *options_cpy = resizeDialog(options_dlg, 1.5);
    
    if(zc_popup_dialog(options_cpy,-1) == 2)
    {
        MouseScroll                = options_cpy[4].flags & D_SELECTED ? 1 : 0;
        SavePaths                  = options_cpy[5].flags & D_SELECTED ? 1 : 0;
        CycleOn                    = options_cpy[6].flags & D_SELECTED ? 1 : 0;
        ShowFPS                    = options_cpy[7].flags & D_SELECTED ? 1 : 0;
        ComboBrush                 = options_cpy[8].flags & D_SELECTED ? 1 : 0;
        FloatBrush                 = options_cpy[9].flags & D_SELECTED ? 1 : 0;
        OpenLastQuest              = options_cpy[10].flags & D_SELECTED ? 1 : 0;
        ShowMisalignments          = options_cpy[11].flags & D_SELECTED ? 1 : 0;
        AnimationOn                = options_cpy[12].flags & D_SELECTED ? 1 : 0;
        OverwriteProtection        = options_cpy[13].flags & D_SELECTED ? 1 : 0;
        TileProtection             = options_cpy[14].flags & D_SELECTED ? 1 : 0;
        InvalidStatic              = options_cpy[15].flags & D_SELECTED ? 1 : 0;
        RulesetDialog              = options_cpy[16].flags & D_SELECTED ? 1 : 0;
        EnableTooltips             = options_cpy[17].flags & D_SELECTED ? 1 : 0;
        AutoBackupRetention        = options_cpy[32].d1;
        AutoSaveInterval           = autosaveListToMinutes(options_cpy[34].d1);
        AutoSaveRetention          = options_cpy[36].d1;
        UncompressedAutoSaves      = options_cpy[37].flags & D_SELECTED ? 1 : 0;
        GridColor                  = options_cpy[39].d1;
        SnapshotFormat             = options_cpy[41].d1;
        KeyboardRepeatDelay        = atoi(kbdelay);
        KeyboardRepeatRate         = atoi(kbrate);
        
        set_keyboard_rate(KeyboardRepeatDelay,KeyboardRepeatRate);
    }

	delete[] options_cpy;
    
    if(AutoSaveInterval!=OldAutoSaveInterval)
    {
        time(&auto_save_time_start);
    }
    
    save_config_file();
    setup_combo_animations();
    setup_combo_animations2();
    refresh(rALL);
    comeback();
    return D_O_K;
}

enum {dm_normal, dm_relational, dm_dungeon, dm_alias, dm_max};
const char *dm_names[dm_max]=
{
    "Normal",
    "Relational",
    "Dungeon",
    "Alias"
};

byte relational_tile_grid[11+(rtgyo*2)][16+(rtgxo*2)];

void fix_drawing_mode_menu()
{
    for(int i=0; i<dm_max; ++i)
    {
        drawing_mode_menu[i].flags=0;
    }
    
    drawing_mode_menu[draw_mode].flags=D_SELECTED;
}

int onDrawingMode()
{
    draw_mode=(draw_mode+1)%dm_max;
    memset(relational_tile_grid,(draw_mode==dm_relational?1:0),(11+(rtgyo*2))*(16+(rtgxo*2)));
    fix_drawing_mode_menu();
    restore_mouse();
    return D_O_K;
}

int onDrawingModeNormal()
{
    draw_mode=dm_normal;
    memset(relational_tile_grid,(draw_mode==dm_relational?1:0),(11+(rtgyo*2))*(16+(rtgxo*2)));
    fix_drawing_mode_menu();
    restore_mouse();
    return D_O_K;
}

int onDrawingModeRelational()
{
    if(draw_mode==dm_relational)
    {
        return onDrawingModeNormal();
    }
    
    draw_mode=dm_relational;
    memset(relational_tile_grid,(draw_mode==dm_relational?1:0),(11+(rtgyo*2))*(16+(rtgxo*2)));
    fix_drawing_mode_menu();
    restore_mouse();
    return D_O_K;
}

int onDrawingModeDungeon()
{
    if(draw_mode==dm_dungeon)
    {
        return onDrawingModeNormal();
    }
    
    draw_mode=dm_dungeon;
    memset(relational_tile_grid,(draw_mode==dm_relational?1:0),(11+(rtgyo*2))*(16+(rtgxo*2)));
    fix_drawing_mode_menu();
    restore_mouse();
    return D_O_K;
}

int onDrawingModeAlias()
{
    if(draw_mode==dm_alias)
    {
        return onDrawingModeNormal();
    }
    
    draw_mode=dm_alias;
    alias_cset_mod=0;
    memset(relational_tile_grid,(draw_mode==dm_relational?1:0),(11+(rtgyo*2))*(16+(rtgxo*2)));
    fix_drawing_mode_menu();
    restore_mouse();
    return D_O_K;
}

int onReTemplate()
{
    if(jwin_alert("Confirm Overwrite","Apply NES Dungeon template to","all screens on this map?",NULL,"&Yes","&No",'y','n',lfont)==1)
    {
        Map.TemplateAll();
        refresh(rALL);
    }
    
    return D_O_K;
}

int onUndo()
{
    Map.Uhuilai();
    refresh(rALL);
    return D_O_K;
}

extern short ffposx[32];
extern short ffposy[32];
extern long ffprvx[32];
extern long ffprvy[32];

int onCopy()
{
    if(prv_mode)
    {
        Map.set_prvcmb(Map.get_prvcmb()==0?1:0);
        
        for(int i=0; i<32; i++)
        {
            ffposx[i]=-1000;
            ffposy[i]=-1000;
            ffprvx[i]=-10000000;
            ffprvy[i]=-10000000;
        }
        
        return D_O_K;
    }
    
    Map.Copy();
    return D_O_K;
}

int onH()
{
    return D_O_K;
}

int onPaste()
{
    {
        Map.Paste();
        refresh(rALL);
    }
    return D_O_K;
}

int onPasteAll()
{
    Map.PasteAll();
    refresh(rALL);
    return D_O_K;
}

int onPasteToAll()
{
    Map.PasteToAll();
    refresh(rALL);
    return D_O_K;
}

int onPasteAllToAll()
{
    Map.PasteAllToAll();
    refresh(rALL);
    return D_O_K;
}

int onPasteUnderCombo()
{
    Map.PasteUnderCombo();
    refresh(rALL);
    return D_O_K;
}

int onPasteSecretCombos()
{
    Map.PasteSecretCombos();
    refresh(rALL);
    return D_O_K;
}

int onPasteFFCombos()
{
    Map.PasteFFCombos();
    refresh(rALL);
    return D_O_K;
}

int onPasteWarps()
{
    Map.PasteWarps();
    refresh(rALL);
    return D_O_K;
}

int onPasteScreenData()
{
    Map.PasteScreenData();
    refresh(rALL);
    return D_O_K;
}

int onPasteWarpLocations()
{
    Map.PasteWarpLocations();
    refresh(rALL);
    return D_O_K;
}

int onPasteDoors()
{
    Map.PasteDoors();
    refresh(rALL);
    return D_O_K;
}

int onPasteLayers()
{
    Map.PasteLayers();
    refresh(rALL);
    return D_O_K;
}

int onPastePalette()
{
    Map.PastePalette();
    refresh(rALL);
    return D_O_K;
}

int onPasteRoom()
{
    Map.PasteRoom();
    refresh(rALL);
    return D_O_K;
}

int onPasteGuy()
{
    Map.PasteGuy();
    refresh(rALL);
    return D_O_K;
}

int onPasteEnemies()
{
    Map.PasteEnemies();
    refresh(rALL);
    return D_O_K;
}

int onDelete()
{
    restore_mouse();
    
    if(Map.CurrScr()->valid&mVALID)
    {
        if(jwin_alert("Confirm Delete","Delete this screen?", NULL, NULL, "Yes", "Cancel", 'y', 27,lfont) == 1)
        {
            Map.Ugo();
            Map.clearscr(Map.getCurrScr());
            refresh(rALL);
        }
    }
    
    memset(relational_tile_grid,(draw_mode==dm_relational?1:0),(11+(rtgyo*2))*(16+(rtgxo*2)));
    saved=false;
    return D_O_K;
    
}

int onDeleteMap()
{
    if(jwin_alert("Confirm Delete","Clear this entire map?", NULL, NULL, "Yes", "Cancel", 'y', 27,lfont) == 1)
    {
        Map.clearmap(false);
        refresh(rALL);
        saved=false;
    }
    
    return D_O_K;
}

int onToggleDarkness()
{
    Map.CurrScr()->flags^=4;
    refresh(rMAP+rMENU);
    saved=false;
    return D_O_K;
}

int onIncMap()
{
    int m=Map.getCurrMap();
    int oldcolor=Map.getcolor();
    Map.setCurrMap(m+1>=map_count?0:m+1);
    
    if(m!=Map.getCurrMap())
    {
        memset(relational_tile_grid,(draw_mode==dm_relational?1:0),(11+(rtgyo*2))*(16+(rtgxo*2)));
    }
    
    int newcolor=Map.getcolor();
    
    if(newcolor!=oldcolor)
    {
        rebuild_trans_table();
    }
    
    refresh(rALL);
    return D_O_K;
}

int onDecMap()
{
    int m=Map.getCurrMap();
    int oldcolor=Map.getcolor();
    Map.setCurrMap((m-1<0)?map_count-1:zc_min(m-1,map_count-1));
    
    if(m!=Map.getCurrMap())
    {
        memset(relational_tile_grid,(draw_mode==dm_relational?1:0),(11+(rtgyo*2))*(16+(rtgxo*2)));
    }
    
    int newcolor=Map.getcolor();
    
    if(newcolor!=oldcolor)
    {
        rebuild_trans_table();
    }
    
    refresh(rALL);
    return D_O_K;
}


int onDefault_Pals()
{
    if(jwin_alert("Confirm Reset","Reset all palette data?", NULL, NULL, "Yes", "Cancel", 'y', 27,lfont) == 1)
    {
        saved=false;
        
        if(!init_colordata(true, &header, &misc))
        {
            jwin_alert("Error","Palette reset failed.",NULL,NULL,"O&K",NULL,'k',0,lfont);
        }
        
        refresh_pal();
    }
    
    return D_O_K;
}

int onDefault_Combos()
{
    if(jwin_alert("Confirm Reset","Reset combo data?", NULL, NULL, "Yes", "Cancel", 'y', 27,lfont) == 1)
    {
        saved=false;
        
        if(!init_combos(true, &header))
        {
            jwin_alert("Error","Combo reset failed.",NULL,NULL,"O&K",NULL,'k',0,lfont);
        }
        
        refresh(rALL);
    }
    
    return D_O_K;
}

int onDefault_Items()
{
    if(jwin_alert("Confirm Reset","Reset all items?", NULL, NULL, "Yes", "Cancel", 'y', 27,lfont) == 1)
    {
        saved=false;
        reset_items(true, &header);
    }
    
    return D_O_K;
}

int onDefault_Weapons()
{
    if(jwin_alert("Confirm Reset","Reset weapon/misc. sprite data?", NULL, NULL, "Yes", "Cancel", 'y', 27,lfont) == 1)
    {
        saved=false;
        reset_wpns(true, &header);
    }
    
    return D_O_K;
}

int onDefault_Guys()
{
    if(jwin_alert("Confirm Reset","Reset all enemy/NPC data?", NULL, NULL, "Yes", "Cancel", 'y', 27,lfont) == 1)
    {
        saved=false;
        reset_guys();
    }
    
    return D_O_K;
}


int onDefault_Tiles()
{
    if(jwin_alert("Confirm Reset","Reset all tiles?", NULL, NULL, "Yes", "Cancel", 'y', 27,lfont) == 1)
    {
        saved=false;
        
        if(!init_tiles(true, &header))
        {
            jwin_alert("Error","Tile reset failed.",NULL,NULL,"O&K",NULL,'k',0,lfont);
        }
        
        refresh(rALL);
    }
    
    return D_O_K;
}

void change_sfx(SAMPLE *sfx1, SAMPLE *sfx2);

int onDefault_SFX()
{
    if(jwin_alert("Confirm Reset","Reset all sound effects?", NULL, NULL, "Yes", "Cancel", 'y', 27,lfont) == 1)
    {
        saved=false;
        Backend::sfx->loadDefaultSamples(Z35, sfxdata, old_sfx_string);
    }
    
    return D_O_K;
}


int onDefault_MapStyles()
{
    if(jwin_alert("Confirm Reset","Reset all map styles?", NULL, NULL, "Yes", "Cancel", 'y', 27,lfont) == 1)
    {
        saved=false;
        reset_mapstyles(true, &misc);
    }
    
    return D_O_K;
}


int on0()
{
    saved=false;
    Map.setcolor(0);
    refresh(rSCRMAP);
    return D_O_K;
}
int on1()
{
    saved=false;
    Map.setcolor(1);
    refresh(rSCRMAP);
    return D_O_K;
}
int on2()
{
    saved=false;
    Map.setcolor(2);
    refresh(rSCRMAP);
    return D_O_K;
}
int on3()
{
    saved=false;
    Map.setcolor(3);
    refresh(rSCRMAP);
    return D_O_K;
}
int on4()
{
    saved=false;
    Map.setcolor(4);
    refresh(rSCRMAP);
    return D_O_K;
}
int on5()
{
    saved=false;
    Map.setcolor(5);
    refresh(rSCRMAP);
    return D_O_K;
}

int on6()
{
    saved=false;
    Map.setcolor(6);
    refresh(rSCRMAP);
    return D_O_K;
}
int on7()
{
    saved=false;
    Map.setcolor(7);
    refresh(rSCRMAP);
    return D_O_K;
}
int on8()
{
    saved=false;
    Map.setcolor(8);
    refresh(rSCRMAP);
    return D_O_K;
}
int on9()
{
    saved=false;
    Map.setcolor(9);
    refresh(rSCRMAP);
    return D_O_K;
}
int on10()
{
    saved=false;
    Map.setcolor(10);
    refresh(rSCRMAP);
    return D_O_K;
}
int on11()
{
    saved=false;
    Map.setcolor(11);
    refresh(rSCRMAP);
    return D_O_K;
}
int on12()
{
    saved=false;
    Map.setcolor(12);
    refresh(rSCRMAP);
    return D_O_K;
}
int on13()
{
    saved=false;
    Map.setcolor(13);
    refresh(rSCRMAP);
    return D_O_K;
}
int on14()
{
    saved=false;
    Map.setcolor(14);
    refresh(rSCRMAP);
    return D_O_K;
}

int onLeft()
{
    int tempcurrscr=Map.getCurrScr();
    
    if(!key[KEY_LSHIFT] && !key[KEY_RSHIFT])
    {
        Map.scroll(2);
        
        if(tempcurrscr!=Map.getCurrScr())
        {
            memset(relational_tile_grid,(draw_mode==dm_relational?1:0),(11+(rtgyo*2))*(16+(rtgxo*2)));
        }
        
        refresh(rALL);
    }
    else if((First[current_combolist]>0)&&(draw_mode!=dm_alias))
    {
        First[current_combolist]-=1;
        clear_tooltip();
        refresh(rCOMBOS);
    }
    else if((combo_alistpos[current_comboalist]>0)&&(draw_mode==dm_alias))
    {
        combo_alistpos[current_comboalist]-=1;
        clear_tooltip();
        refresh(rCOMBOS);
    }
    
    clear_keybuf();
    return D_O_K;
}

int onRight()
{
    int tempcurrscr=Map.getCurrScr();
    
    if(!key[KEY_LSHIFT] && !key[KEY_RSHIFT])
    {
        Map.scroll(3);
        
        if(tempcurrscr!=Map.getCurrScr())
        {
            memset(relational_tile_grid,(draw_mode==dm_relational?1:0),(11+(rtgyo*2))*(16+(rtgxo*2)));
        }
        
        refresh(rALL);
    }
    else if((First[current_combolist]<(MAXCOMBOS-(combolist(0).w*combolist(0).h)))&&(draw_mode!=dm_alias))
    {
        First[current_combolist]+=1;
        clear_tooltip();
        refresh(rCOMBOS);
    }
    else if((combo_alistpos[current_comboalist]<(MAXCOMBOALIASES-(combolist(0).w*combolist(0).h)))&&(draw_mode==dm_alias))
    {
        combo_alistpos[current_comboalist]+=1;
        clear_tooltip();
        refresh(rCOMBOS);
    }
    
    clear_keybuf();
    return D_O_K;
}

int onUp()
{
    int tempcurrscr=Map.getCurrScr();
    
    if(!key[KEY_LSHIFT] && !key[KEY_RSHIFT])
    {
        Map.scroll(0);
        
        if(tempcurrscr!=Map.getCurrScr())
        {
            memset(relational_tile_grid,(draw_mode==dm_relational?1:0),(11+(rtgyo*2))*(16+(rtgxo*2)));
        }
        
        refresh(rALL);
    }
    else if((First[current_combolist]>0)&&(draw_mode!=dm_alias))
    {
        First[current_combolist]-=zc_min(First[current_combolist],combolist(0).w);
        clear_tooltip();
        
        refresh(rCOMBOS);
    }
    else if((combo_alistpos[current_comboalist]>0)&&(draw_mode==dm_alias))
    {
        combo_alistpos[current_comboalist]-=zc_min(combo_alistpos[current_comboalist],combolist(0).w);
        clear_tooltip();
        refresh(rCOMBOS);
    }
    
    clear_keybuf();
    return D_O_K;
}

int onDown()
{
    int tempcurrscr=Map.getCurrScr();
    
    if(!key[KEY_LSHIFT] && !key[KEY_RSHIFT])
    {
        Map.scroll(1);
        
        if(tempcurrscr!=Map.getCurrScr())
        {
            memset(relational_tile_grid,(draw_mode==dm_relational?1:0),(11+(rtgyo*2))*(16+(rtgxo*2)));
        }
        
        refresh(rALL);
    }
    else if((First[current_combolist]<(MAXCOMBOS-(combolist(0).w*combolist(0).h)))&&(draw_mode!=dm_alias))
    {
        First[current_combolist]+=zc_min((MAXCOMBOS-combolist(0).w)-First[current_combolist],combolist(0).w);
        clear_tooltip();
        refresh(rCOMBOS);
    }
    else if((combo_alistpos[current_comboalist]<(MAXCOMBOALIASES-(comboaliaslist(0).w*comboaliaslist(0).h)))&&(draw_mode==dm_alias))
    {
        combo_alistpos[current_comboalist]+=zc_min((MAXCOMBOALIASES-combolist(0).w)-combo_alistpos[current_comboalist],combolist(0).w);
        clear_tooltip();
        refresh(rCOMBOS);
    }
    
    clear_keybuf();
    return D_O_K;
}

int onPgUp()
{
    if(!key[KEY_LSHIFT] && !key[KEY_RSHIFT] &&
            !key[KEY_ZC_LCONTROL] && !key[KEY_ZC_RCONTROL] && !is_large())
    {
        menutype=wrap(menutype-1,0,m_menucount-1);
        refresh(rMENU);
    }
    else if((First[current_combolist]>0)&&(draw_mode!=dm_alias))
    {
        if(key[KEY_ZC_LCONTROL] || key[KEY_ZC_RCONTROL])
        {
            First[current_combolist]-=zc_min(First[current_combolist],256);
            clear_tooltip();
        }
        else
        {
            First[current_combolist]-=zc_min(First[current_combolist],(combolist(0).w*combolist(0).h));
            clear_tooltip();
        }
        
        refresh(rCOMBOS);
    }
    else if((combo_alistpos[current_comboalist]>0)&&(draw_mode==dm_alias))
    {
        if(key[KEY_ZC_LCONTROL] || key[KEY_ZC_RCONTROL])
        {
            combo_alistpos[current_comboalist]=0;
            clear_tooltip();
        }
        else
        {
            combo_alistpos[current_comboalist]-=zc_min(combo_alistpos[current_comboalist],(comboaliaslist(0).w*comboaliaslist(0).h));
            clear_tooltip();
        }
        
        refresh(rCOMBOS);
    }
    
    return D_O_K;
}

int onPgDn()
{
    if(!key[KEY_LSHIFT] && !key[KEY_RSHIFT] &&
            !key[KEY_ZC_LCONTROL] && !key[KEY_ZC_RCONTROL] && !is_large())
    {
        menutype=wrap(menutype+1,0,m_menucount-1);
        refresh(rMENU);
    }
    else if((First[current_combolist]<(MAXCOMBOS-(combolist(0).w*combolist(0).h)))&&(draw_mode!=dm_alias))
    {
        if(key[KEY_ZC_LCONTROL] || key[KEY_ZC_RCONTROL])
        {
            First[current_combolist]=zc_min((MAXCOMBOS-combolist(0).w*combolist(0).h),First[current_combolist]+256);
            clear_tooltip();
        }
        else
        {
            First[current_combolist]=zc_min((MAXCOMBOS-(combolist(0).w*combolist(0).h)),First[current_combolist]+(combolist(0).w*combolist(0).h));
            clear_tooltip();
        }
        
        refresh(rCOMBOS);
    }
    else if((combo_alistpos[current_comboalist]<(MAXCOMBOALIASES-(comboaliaslist(0).w*comboaliaslist(0).h)))&&(draw_mode==dm_alias))
    {
        if(key[KEY_ZC_LCONTROL] || key[KEY_ZC_RCONTROL])
        {
            combo_alistpos[current_comboalist]=MAXCOMBOALIASES-(comboaliaslist(0).w*comboaliaslist(0).h);
            clear_tooltip();
        }
        else
        {
            combo_alistpos[current_comboalist]=zc_min((MAXCOMBOALIASES-(comboaliaslist(0).w*comboaliaslist(0).h)),combo_alistpos[current_comboalist]+(comboaliaslist(0).w*comboaliaslist(0).h));
            clear_tooltip();
        }
        
        refresh(rCOMBOS);
    }
    
    return D_O_K;
}

int onIncreaseCSet()
{
    if(!key[KEY_LSHIFT] && !key[KEY_RSHIFT] &&
            !key[KEY_ZC_LCONTROL] && !key[KEY_ZC_RCONTROL] &&
            !key[KEY_ALT] && !key[KEY_ALTGR])
    {
        if(draw_mode!=dm_alias)
        {
            CSet=wrap(CSet+1,0,11);
            refresh(rCOMBOS+rMENU+rCOMBO);
        }
        else
        {
            alias_cset_mod=wrap(alias_cset_mod+1,0,11);
        }
    }
    else if(key[KEY_LSHIFT] || key[KEY_RSHIFT])
    {
        int drawmap, drawscr;
        
        if(CurrentLayer==0)
        {
            drawmap=Map.getCurrMap();
            drawscr=Map.getCurrScr();
        }
        else
        {
            drawmap=Map.CurrScr()->layermap[CurrentLayer-1]-1;
            drawscr=Map.CurrScr()->layerscreen[CurrentLayer-1];
            
            if(drawmap<0)
            {
                return D_O_K;
            }
        }
        
        if(!(Map.AbsoluteScr(drawmap, drawscr)->valid&mVALID))
        {
            return D_O_K;
        }
        
        saved=false;
        Map.Ugo();
        int changeby=1;
        
        if(key[KEY_ZC_LCONTROL] || key[KEY_ZC_RCONTROL])
        {
            changeby*=16;
        }
        
        if(key[KEY_ALT] || key[KEY_ALTGR])
        {
            changeby*=256;
        }
        
        for(int i=0; i<176; i++)
        {
            int temp=Map.AbsoluteScr(drawmap, drawscr)->data[i];
            
            temp+=changeby;
            
            if(temp>=MAXCOMBOS)
            {
                temp=temp-MAXCOMBOS;
            }
            
            Map.AbsoluteScr(drawmap, drawscr)->data[i]=temp;
        }
        
        refresh(rMAP+rSCRMAP);
    }
    
    return D_O_K;
}

int onDecreaseCSet()
{
    if(!key[KEY_LSHIFT] && !key[KEY_RSHIFT] &&
            !key[KEY_ZC_LCONTROL] && !key[KEY_ZC_RCONTROL] &&
            !key[KEY_ALT] && !key[KEY_ALTGR])
    {
        if(draw_mode!=dm_alias)
        {
            CSet=wrap(CSet-1,0,11);
            refresh(rCOMBOS+rMENU+rCOMBO);
        }
        else
        {
            alias_cset_mod=wrap(alias_cset_mod-1,0,11);
        }
    }
    else if(key[KEY_LSHIFT] || key[KEY_RSHIFT])
    {
        int drawmap, drawscr;
        
        if(CurrentLayer==0)
        {
            drawmap=Map.getCurrMap();
            drawscr=Map.getCurrScr();
        }
        else
        {
            drawmap=Map.CurrScr()->layermap[CurrentLayer-1]-1;
            drawscr=Map.CurrScr()->layerscreen[CurrentLayer-1];
            
            if(drawmap<0)
            {
                return D_O_K;
            }
        }
        
        if(!(Map.AbsoluteScr(drawmap, drawscr)->valid&mVALID))
        {
            return D_O_K;
        }
        
        saved=false;
        Map.Ugo();
        int changeby=1;
        
        if(key[KEY_ZC_LCONTROL] || key[KEY_ZC_RCONTROL])
        {
            changeby*=16;
        }
        
        if(key[KEY_ALT] || key[KEY_ALTGR])
        {
            changeby*=256;
        }
        
        for(int i=0; i<176; i++)
        {
            int temp=Map.AbsoluteScr(drawmap, drawscr)->data[i];
            temp-=changeby;
            
            if(temp<0)
            {
                temp=MAXCOMBOS+temp;
            }
            
            Map.AbsoluteScr(drawmap, drawscr)->data[i]=temp;
        }
        
        refresh(rMAP+rSCRMAP);
    }
    
    return D_O_K;
}

int onGotoPage()
{
    int choosepage=getnumber("Scroll to Combo Page", 0);
    
    if(!cancelgetnum)
    {
        int page=(zc_min(choosepage,COMBO_PAGES-1));
        First[current_combolist]=page<<8;
    }
    
    return D_O_K;
}

bool getname(const char *prompt,const char *ext,EXT_LIST *list,const char *def,bool usefilename)
{
    go();
    int ret=0;
    ret = getname_nogo(prompt,ext,list,def,usefilename);
    comeback();
    return ret != 0;
}


bool getname_nogo(const char *prompt,const char *ext,EXT_LIST *list,const char *def,bool usefilename)
{
    if(def!=temppath)
        strcpy(temppath,def);
        
    if(!usefilename)
    {
        int i=(int)strlen(temppath);
        
        while(i>=0 && temppath[i]!='\\' && temppath[i]!='/')
            temppath[i--]=0;
    }
    
    //  int ret = file_select_ex(prompt,temppath,ext,255,-1,-1);
    int ret=0;
    int sel=0;
    
    if(list==NULL)
    {
        ret = jwin_file_select_ex(prompt,temppath,ext,2048,-1,-1,lfont);
    }
    else
    {
        ret = jwin_file_browse_ex(prompt, temppath, list, &sel, 2048, -1, -1, lfont);
    }
    
    return ret!=0;
}


static char track_number_str_buf[32];
const char *tracknumlist(int index, int *list_size)
{
    if(index>=0)
    {
        bound(index,0,255);
        sprintf(track_number_str_buf,"%02d",index+1);
        return track_number_str_buf;
    }
    
    *list_size=zcmusic_get_tracks(zcmusic);
    return NULL;
}

static ListData tracknum_list(tracknumlist, &font);

static DIALOG change_track_dlg[] =
{
    // (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp)
    { jwin_win_proc,      60-12,   40,   200-16,  72,  vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "Select Track", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { jwin_droplist_proc, 72-12,   60+4,   161,  16,   jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,       0,     0,             0, (void *) &tracknum_list, NULL, NULL },
    { jwin_button_proc,   70,   87,  61,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "OK", NULL, NULL },
    { jwin_button_proc,   150,  87,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};
//  return list_dlg[2].d1;

int changeTrack()
{
    restore_mouse();
    change_track_dlg[0].dp2=lfont;
    change_track_dlg[2].d1=gme_track;

	DIALOG *change_track_cpy = resizeDialog(change_track_dlg, 1.5);
    
    if(zc_popup_dialog(change_track_cpy,2)==3)
    {
        gme_track= change_track_cpy[2].d1;
        zcmusic_change_track(zcmusic, gme_track);
    }

	delete[] change_track_cpy;
    
    return D_O_K;
}


int playMusic()
{
    char *ext;
    bool ismidi=false;
    char allmusic_types[256];
    sprintf(allmusic_types, "%s;mid", zcmusic_types);
    
    if(getname("Load Music",(char*)allmusic_types,NULL,midipath,false))
    {
        strcpy(midipath,temppath);
        
        ext=get_extension(midipath);
        
        if(
            (stricmp(ext,"ogg")==0)||
            (stricmp(ext,"mp3")==0)||
            (stricmp(ext,"it")==0)||
            (stricmp(ext,"xm")==0)||
            (stricmp(ext,"s3m")==0)||
            (stricmp(ext,"mod")==0)||
            (stricmp(ext,"spc")==0)||
            (stricmp(ext,"gym")==0)||
            (stricmp(ext,"nsf")==0)||
            (stricmp(ext,"gbs")==0)||
            (stricmp(ext,"vgm")==0)
        )
        {
            ismidi=false;
        }
        else if((stricmp(ext,"mid")==0))
        {
            ismidi=true;
        }
        else
        {
            return D_O_K;
        }
        
        stop_midi();
        
        if(zcmusic != NULL)
        {
            zcmusic_stop(zcmusic);
            zcmusic_unload_file(zcmusic);
            zcmusic = NULL;
        }
        
        if(ismidi)
        {
            if((song=load_midi(midipath))!=NULL)
            {
                if(play_midi(song,true)==0)
                {
                    etc_menu[8].flags =
                        commands[cmdPlayTune].flags = 0;
                        
                    etc_menu[9].flags = D_SELECTED;
                    commands[cmdPlayMusic].flags = 0;
                    
                    etc_menu[10].flags =
                        commands[cmdChangeTrack].flags = D_DISABLED;
                }
            }
        }
        else
        {
            gme_track=0;
            zcmusic = (ZCMUSIC*)zcmusic_load_file(midipath);
            
            if(zcmusic!=NULL)
            {
                etc_menu[8].flags =
                    commands[cmdPlayTune].flags = 0;
                    
                etc_menu[9].flags=D_SELECTED;
                commands[cmdPlayMusic].flags = 0;
                
                etc_menu[10].flags =
                    commands[cmdChangeTrack].flags = (zcmusic_get_tracks(zcmusic)<2)?D_DISABLED:0;
                    
                zcmusic_play(zcmusic, midi_volume);
            }
        }
    }
    
    return D_O_K;
}

// It took awhile to get these values right, so no meddlin'!
int playTune1()
{
    return playTune(0);
}
int playTune2()
{
    return playTune(81);
}
int playTune3()
{
    return playTune(233);
}
int playTune4()
{
    return playTune(553);
}
int playTune5()
{
    return playTune(814);
}
int playTune6()
{
    return playTune(985);
}
int playTune7()
{
    return playTune(1153);
}
int playTune8()
{
    return playTune(1333);
}
int playTune9()
{
    return playTune(1556);
}
int playTune10()
{
    return playTune(1801);
}
int playTune11()
{
    return playTune(2069);
}
int playTune12()
{
    return playTune(2189);
}
int playTune13()
{
    return playTune(2569);
}
int playTune14()
{
    return playTune(2753);
}
int playTune15()
{
    return playTune(2856);
}
int playTune16()
{
    return playTune(3042);
}
int playTune17()
{
    return playTune(3125);
}
int playTune18()
{
    return playTune(3217);
}
int playTune19()
{
    return playTune(3296);
}

int playTune(int pos)
{
    stop_midi();
    
    if(zcmusic != NULL)
    {
        zcmusic_stop(zcmusic);
        zcmusic_unload_file(zcmusic);
        zcmusic = NULL;
    }
    
    if(play_midi((MIDI*)zcdata[THETRAVELSOFLINK_MID].dat,true)==0)
    {
        midi_seek(pos);
        
        etc_menu[8].flags = D_SELECTED;
        commands[cmdPlayTune].flags = 0;
        
        etc_menu[9].flags =
            commands[cmdPlayMusic].flags = 0;
            
        etc_menu[10].flags =
            commands[cmdChangeTrack].flags = D_DISABLED;
    }
    
    return D_O_K;
}

int stopMusic()
{
    stop_midi();
    
    if(zcmusic != NULL)
    {
        zcmusic_stop(zcmusic);
        zcmusic_unload_file(zcmusic);
        zcmusic = NULL;
    }
    
    etc_menu[8].flags =
        etc_menu[9].flags =
            commands[cmdPlayTune].flags =
                commands[cmdPlayMusic].flags = 0;
                
    etc_menu[10].flags =
        commands[cmdChangeTrack].flags = D_DISABLED;
    return D_O_K;
}

#include "zq_files.h"

int onTemplates()
{
    edit_qt();
    return D_O_K;
}

//  +----------+
//  |          |
//  | View Pic |
//  |          |
//  |          |
//  |          |
//  +----------+

BITMAP *pic=NULL;
BITMAP *bmap=NULL;
PALETTE picpal;
PALETTE mappal;
int  picx=0,picy=0,mapx=0,mapy=0,pblack,pwhite;

double picscale=1.0,mapscale=1.0;
bool vp_showpal=true, vp_showsize=true, vp_center=true;

//INLINE int pal_sum(RGB p) { return p.r + p.g + p.b; }

void get_bw(RGB *pal,int &black,int &white)
{
    black=white=1;
    
    for(int i=1; i<256; i++)
    {
        if(pal_sum(pal[i])<pal_sum(pal[black]))
            black=i;
            
        if(pal_sum(pal[i])>pal_sum(pal[white]))
            white=i;
    }
}

void draw_bw_mouse(int white, int old_mouse, int new_mouse)
{
    blit(mouse_bmp[old_mouse][0],mouse_bmp[new_mouse][0],0,0,0,0,16,16);
    
    for(int y=0; y<16; y++)
    {
        for(int x=0; x<16; x++)
        {
            if(getpixel(mouse_bmp[new_mouse][0],x,y)!=0)
            {
                putpixel(mouse_bmp[new_mouse][0],x,y,white);
            }
        }
    }
}

int load_the_pic(BITMAP **dst, PALETTE dstpal)
{
    PALETTE temppal;
    
    for(int i=0; i<256; i++)
    {
        temppal[i]=dstpal[i];
        dstpal[i]=RAMpal[i];
    }
    
    // set up the new palette
    for(int i=0; i<64; i++)
    {
        dstpal[i].r = i;
        dstpal[i].g = i;
        dstpal[i].b = i;
    }
    
    Backend::palette->setPalette(dstpal);
    
    BITMAP *graypic = create_bitmap_ex(8,SCREEN_W,SCREEN_H);
    int _w = screen->w-1;
    int _h = screen->h-1;
    
    // gray scale the current frame
    for(int y=0; y<_h; y++)
    {
        for(int x=0; x<_w; x++)
        {
            int c = screen->line[y][x];
            int gray = zc_min((temppal[c].r*42 + temppal[c].g*75 + temppal[c].b*14) >> 7, 63);
            graypic->line[y][x] = gray;
        }
    }
    
    blit(graypic,screen,0,0,0,0,SCREEN_W,SCREEN_H);
    destroy_bitmap(graypic);
    char extbuf[2][80];
    memset(extbuf[0],0,80);
    memset(extbuf[1],0,80);
    sprintf(extbuf[0], "View Image (%s", snapshotformat_str[0][1]);
    strcpy(extbuf[1], snapshotformat_str[0][1]);
    
    for(int i=1; i<ssfmtMAX; ++i)
    {
        sprintf(extbuf[0], "%s, %s", extbuf[0], snapshotformat_str[i][1]);
        sprintf(extbuf[1], "%s;%s", extbuf[1], snapshotformat_str[i][1]);
    }
    
    sprintf(extbuf[0], "%s)", extbuf[0]);
    
    int gotit = getname(extbuf[0],extbuf[1],NULL,imagepath,true);
    
    if(!gotit)
    {
        Backend::palette->setPalette(temppal);
        Backend::palette->getPalette(dstpal);
        return 1;
    }
    
    strcpy(imagepath,temppath);
    
    if(*dst)
    {
        destroy_bitmap(*dst);
    }
    
    for(int i=0; i<256; i++)
    {
        dstpal[i].r = 0;
        dstpal[i].g = 0;
        dstpal[i].b = 0;
    }
    
    *dst = load_bitmap(imagepath,picpal);
    
    if(!*dst)
    {
        jwin_alert("Error","Error loading image:",imagepath,NULL,"OK",NULL,13,27,lfont);
        return 2;
    }
    
    //  get_bw(picpal,pblack,pwhite);
    //  draw_bw_mouse(pwhite);
    //  gui_bg_color = pblack;
    //  gui_fg_color = pwhite;
    
    if(vp_center)
    {
        picx=picy=0;
    }
    else
    {
        picx=(*dst)->w- Backend::graphics->virtualScreenW();
        picy=(*dst)->h- Backend::graphics->virtualScreenH();
    }
    
    return 0;
}

int mapMaker(BITMAP * _map, PALETTE _mappal)
{
    char buf[50];
    int num=0;
    
    do
    {
#ifdef ALLEGRO_MACOSX
        snprintf(buf, 50, "../../../zelda%03d.%s", ++num, snapshotformat_str[SnapshotFormat][1]);
#else
        snprintf(buf, 50, "zelda%03d.%s", ++num, snapshotformat_str[SnapshotFormat][1]);
#endif
        buf[49]='\0';
    }
    while(num<999 && exists(buf));
    
    save_bitmap(buf,_map,_mappal);
    
    return D_O_K;
}

int onViewPic()
{
    return launchPicViewer(&pic,picpal,&picx,&picy,&picscale,false);
}

int launchPicViewer(BITMAP **pictoview, PALETTE pal, int *px2, int *py2, double *scale2, bool isviewingmap)
{
    restore_mouse();
    BITMAP *buf;
    bool done=false, redraw=true;
    
    go();
    clear_bitmap(screen);
    
    // Always call load_the_map() when viewing the map.
    if((!*pictoview || isviewingmap) && (isviewingmap ? load_the_map() : load_the_pic(pictoview,pal)))
    {
        Backend::palette->setPalette(RAMpal);
        comeback();
        return D_O_K;
    }
    
    get_bw(pal,pblack,pwhite);
    
    int oldfgcolor = gui_fg_color;
    int oldbgcolor = gui_bg_color;
    
    buf = create_bitmap_ex(8, Backend::graphics->virtualScreenW(), Backend::graphics->virtualScreenH());
    
    if(!buf)
    {
        jwin_alert("Error","Error creating temp bitmap",NULL,NULL,"OK",NULL,13,27,lfont);
        return D_O_K;
    }
    
    //  go();
    //  clear_bitmap(screen);
    Backend::palette->setPalette(pal);
    
    do
    {
        if(redraw)
        {
            clear_to_color(buf,pblack);
            stretch_blit(*pictoview,buf,0,0,(*pictoview)->w,(*pictoview)->h,
                         int(Backend::graphics->virtualScreenW() +(*px2-(*pictoview)->w)* *scale2)/2,int(Backend::graphics->virtualScreenH() +(*py2-(*pictoview)->h)* *scale2)/2,
                         int((*pictoview)->w* *scale2),int((*pictoview)->h* *scale2));
                         
            if(vp_showpal)
                for(int i=0; i<256; i++)
                    rectfill(buf,((i&15)<<2)+ Backend::graphics->virtualScreenW() -64,((i>>4)<<2)+ Backend::graphics->virtualScreenH() -64,((i&15)<<2)+ Backend::graphics->virtualScreenW() -64+3,((i>>4)<<2)+ Backend::graphics->virtualScreenH() -64+3,i);
                    
            if(vp_showsize)
            {
                //        text_mode(pblack);
                textprintf_ex(buf,font,0, Backend::graphics->virtualScreenH() -8,pwhite,pblack,"%dx%d %.2f%%",(*pictoview)->w,(*pictoview)->h,*scale2*100.0);
            }
            
            blit(buf,screen,0,0,0,0, Backend::graphics->virtualScreenW(), Backend::graphics->virtualScreenH());            
            redraw=false;
        }

		Backend::graphics->waitTick();
		Backend::graphics->showBackBuffer();
        
        int step = 4;
        
        if(*scale2 < 1.0)
            step = int(4.0/ *scale2);
            
        if(key[KEY_LSHIFT] || key[KEY_RSHIFT])
            step <<= 2;
            
        if(key[KEY_ZC_LCONTROL] || key[KEY_ZC_RCONTROL])
            step = 1;
            
        if(key[KEY_UP])
        {
            *py2+=step;
            redraw=true;
        }
        
        if(key[KEY_DOWN])
        {
            *py2-=step;
            redraw=true;
        }
        
        if(key[KEY_LEFT])
        {
            *px2+=step;
            redraw=true;
        }
        
        if(key[KEY_RIGHT])
        {
            *px2-=step;
            redraw=true;
        }
        
        if(keypressed() && !redraw)
            switch(readkey()>>8)
            {
            case KEY_PGUP:
                *scale2*=0.95;
                
                if(*scale2<0.1) *scale2=0.1;
                
                redraw=true;
                break;
                
            case KEY_PGDN:
                *scale2/=0.95;
                
                if(*scale2>5.0) *scale2=5.0;
                
                redraw=true;
                break;
                
            case KEY_HOME:
                *scale2/=2.0;
                
                if(*scale2<0.1) *scale2=0.1;
                
                redraw=true;
                break;
                
            case KEY_END:
                *scale2*=2.0;
                
                if(*scale2>5.0) *scale2=5.0;
                
                redraw=true;
                break;
                
            case KEY_TILDE:
                *scale2=0.5;
                redraw=true;
                break;
                
            case KEY_Z:
                *px2=(*pictoview)->w-Backend::graphics->virtualScreenW();
                *py2=(*pictoview)->h-Backend::graphics->virtualScreenH();
                vp_center=false;
                redraw=true;
                break;
                
            case KEY_1:
                *scale2=1.0;
                redraw=true;
                break;
                
            case KEY_2:
                *scale2=2.0;
                redraw=true;
                break;
                
            case KEY_3:
                *scale2=3.0;
                redraw=true;
                break;
                
            case KEY_4:
                *scale2=4.0;
                redraw=true;
                break;
                
            case KEY_5:
                *scale2=5.0;
                redraw=true;
                break;
                
            case KEY_C:
                *px2=*py2=0;
                redraw=vp_center=true;
                break;
                
            case KEY_S:
                vp_showsize = !vp_showsize;
                redraw=true;
                break;
                
            case KEY_D:
                vp_showpal = !vp_showpal;
                redraw=true;
                break;
                
            case KEY_P:
                if(isviewingmap) break;
                
            case KEY_ESC:
                done=true;
                break;
                
            case KEY_SPACE:
                if(isviewingmap ? load_the_map() : load_the_pic(pictoview,pal)==2)
                {
                    done=true;
                }
                else
                {
                    redraw=true;
                    gui_bg_color = pblack;
                    gui_fg_color = pwhite;
                    *scale2=1.0;
                    Backend::palette->setPalette(pal);
                }
                
                get_bw(pal,pblack,pwhite);
                break;
            }
    }
    while(!done);
    
    destroy_bitmap(buf);
    Backend::palette->setPalette(RAMpal);
    gui_fg_color = oldfgcolor;
    gui_bg_color = oldbgcolor;
    
    comeback();
    Backend::mouse->setWheelPosition(0);
    return D_O_K;
}

static DIALOG loadmap_dlg[] =
{
    // (dialog proc)         (x)    (y)     (w)     (h)     (fg)        (bg)    (key)     (flags)  (d1)  (d2)   (dp)                                 (dp2)   (dp3)
    {  jwin_win_proc,          0,     0,    225,    113,    vc(14),     vc(1),      0,    D_EXIT,     0,    0, (void *) "View Map",                 NULL,   NULL  },
    {  d_timer_proc,           0,     0,      0,      0,    0,          0,          0,    0,          0,    0,  NULL,                                NULL,   NULL  },
    {  jwin_text_proc,        32,    26,     96,      8,    vc(11),     vc(1),      0,    0,          0,    0, (void *) "Resolution",               NULL,   NULL  },
    // 3
    {  jwin_radio_proc,       16,    36,     97,      9,    vc(14),     vc(1),      0,    0,          0,    0, (void *) "1/4 - 1024x352",		   NULL,   NULL  },
    {  jwin_radio_proc,       16,    46,     97,      9,    vc(14),     vc(1),      0,    0,          0,    0, (void *) "1/2 - 2048x704",		   NULL,   NULL  },
    {  jwin_radio_proc,       16,    56,     97,      9,    vc(14),     vc(1),      0,    0,          0,    0, (void *) "Full - 4096x1408",		   NULL,   NULL  },
    {  jwin_text_proc,       144,    26,     97,      9,    vc(11),     vc(1),      0,    0,          0,    0, (void *) "Options",                  NULL,   NULL  },
    // 7
    {  jwin_check_proc,      144,    36,     97,      9,    vc(14),     vc(1),      0,    0,          1,    0, (void *) "Walk",                     NULL,   NULL  },
    {  jwin_check_proc,      144,    46,     97,      9,    vc(14),     vc(1),      0,    0,          1,    0, (void *) "Flags",                    NULL,   NULL  },
    {  jwin_check_proc,      144,    56,     97,      9,    vc(14),     vc(1),      0,    0,          1,    0, (void *) "Dark",                     NULL,   NULL  },
    {  jwin_check_proc,      144,    66,     97,      9,    vc(14),     vc(1),      0,    0,          1,    0, (void *) "Items",                    NULL,   NULL  },
    // 11
    {  jwin_button_proc,      42,    80,     61,     21,    vc(14),     vc(1),     13,    D_EXIT,     0,    0, (void *) "OK",                       NULL,   NULL  },
    {  jwin_button_proc,     122,    80,     61,     21,    vc(14),     vc(1),     27,    D_EXIT,     0,    0, (void *) "Cancel",                   NULL,   NULL  },
    {  jwin_check_proc,       16,    68,     97,      9,    vc(14),     vc(1),      0,    0,          1,    0, (void *) "Save to File (Mapmaker)",  NULL,   NULL  },
    {  NULL,                   0,     0,      0,      0,    0,          0,          0,    0,          0,    0,  NULL,                                NULL,   NULL  }
};

int load_the_map()
{
    static int res = 1;
    static int flags = cDEBUG;
    
    loadmap_dlg[0].dp2    = lfont;
    loadmap_dlg[3].flags  = (res==2) ? D_SELECTED : 0;
    loadmap_dlg[4].flags  = (res==1) ? D_SELECTED : 0;
    loadmap_dlg[5].flags  = (res==0) ? D_SELECTED : 0;
    loadmap_dlg[7].flags  = (flags&cWALK)   ? D_SELECTED : 0;
    loadmap_dlg[8].flags  = (flags&cFLAGS)  ? D_SELECTED : 0;
    loadmap_dlg[9].flags  = (flags&cNODARK) ? 0 : D_SELECTED;
    loadmap_dlg[10].flags = (flags&cNOITEM) ? 0 : D_SELECTED;
    loadmap_dlg[13].flags = 0;

	DIALOG *loadmap_cpy = resizeDialog(loadmap_dlg, 1.5);
    
    if(zc_popup_dialog(loadmap_cpy,11) != 11)
    {
        return 1;
    }
    
    flags = cDEBUG;
    
    if(loadmap_cpy[3].flags&D_SELECTED)  res=2;
    
    if(loadmap_cpy[4].flags&D_SELECTED)  res=1;
    
    if(loadmap_cpy[5].flags&D_SELECTED)  res=0;
    
    if(loadmap_cpy[7].flags&D_SELECTED)  flags|=cWALK;
    
    if(loadmap_cpy[8].flags&D_SELECTED)  flags|=cFLAGS;
    
    if(!(loadmap_cpy[9].flags&D_SELECTED))  flags|=cNODARK;
    
    if(!(loadmap_cpy[10].flags&D_SELECTED)) flags|=cNOITEM;
    
    if(bmap)
    {
        destroy_bitmap(bmap);
    }
    
    
    bmap = create_bitmap_ex(8,(256*16)>>res,(176*8)>>res);
    
    if(!bmap)
    {
        jwin_alert("Error","Error creating bitmap.",NULL,NULL,"OK",NULL,13,27,lfont);
        return 2;
    }
    
    for(int y=0; y<8; y++)
    {
        for(int x=0; x<16; x++)
        {
            Map.draw(screen2, 0, 0, flags, -1, y*16+x);
            stretch_blit(screen2, bmap, 0, 0, 256, 176, x<<(8-res), (y*176)>>res, 256>>res,176>>res);
        }
    }
    
    memcpy(mappal,RAMpal,sizeof(RAMpal));
    vp_showpal = false;
    get_bw(picpal,pblack,pwhite);
    mapx = mapy = 0;
    mapscale = 1;
    imagepath[0] = 0;
    
    if(loadmap_cpy[13].flags & D_SELECTED) mapMaker(bmap, mappal);

	delete[] loadmap_cpy;
    
    return 0;
}

int onViewMap()
{
    int temp_aligns=ShowMisalignments;
    ShowMisalignments=0;
    /*if(load_the_map()==0)
    {*/
    launchPicViewer(&bmap,mappal,&mapx, &mapy, &mapscale,true);
    //}
    ShowMisalignments=temp_aligns;
    return D_O_K;
}

static const char *dirstr[4] = {"North","South","West","East"};
char _pathstr[40]="North,North,North,North";

char *pathstr(byte path[])
{
    sprintf(_pathstr,"%s,%s,%s,%s",dirstr[path[0]],dirstr[path[1]],
            dirstr[path[2]],dirstr[path[3]]);
    return _pathstr;
}

char _ticksstr[32]="99.99 seconds";

char *ticksstr(int tics)
{
    int mins=tics/(60*60);
    tics=tics-(mins*60*60);
    int secs=tics/60;
    tics=tics-(secs*60);
    tics=tics*100/60;
    
    if(mins>0)
    {
        sprintf(_ticksstr,"%d:%02d.%02d",mins, secs, tics);
    }
    else
    {
        sprintf(_ticksstr,"%d.%02d seconds",secs, tics);
    }
    
    return _ticksstr;
}
void textprintf_disabled(BITMAP *bmp, AL_CONST FONT *f, int x, int y, int color_hl, int color_sh, AL_CONST char *format, ...)
{
    char buf[512];
    va_list ap;
    ASSERT(bmp);
    ASSERT(f);
    ASSERT(format);
    
    va_start(ap, format);
    uvszprintf(buf, sizeof(buf), format, ap);
    va_end(ap);
    
    
    textout_ex(bmp, f, buf, x+1, y+1, color_hl, -1);
    
    textout_ex(bmp, f, buf, x, y, color_sh, -1);
}

void textprintf_centre_disabled(BITMAP *bmp, AL_CONST FONT *f, int x, int y, int color_hl, int color_sh, AL_CONST char *format, ...)
{
    char buf[512];
    va_list ap;
    ASSERT(bmp);
    ASSERT(f);
    ASSERT(format);
    
    va_start(ap, format);
    uvszprintf(buf, sizeof(buf), format, ap);
    va_end(ap);
    
    textout_centre_ex(bmp, f, buf, x+1, y+1, color_hl, -1);
    textout_centre_ex(bmp, f, buf, x, y, color_sh, -1);
}

void drawpanel(int pnl)
{
    mapscr *scr=Map.CurrScr();
    int NextCombo=combobuf[Combo].nextcombo;
    int NextCSet=combobuf[Combo].nextcset;
    
    if(prv_mode)
    {
        jwin_draw_frame(menu1,0,panel(8).y,panel(8).x+panel(0).w, panel(0).h, FR_WIN);
        rectfill(menu1,panel(8).x,panel(8).y+2,panel(8).x+panel(0).w-3,panel(8).y+panel(0).h-3,jwin_pal[jcBOX]);
    }
    else
    {
        jwin_draw_frame(menu1,0,panel(0).y,panel(0).x+panel(0).w,panel(0).h, FR_WIN);
        rectfill(menu1,panel(0).x,panel(0).y+2,panel(0).x+panel(0).w-3,panel(0).y+panel(0).h-3,jwin_pal[jcBOX]);
        
        if(!is_large())
        {
            jwin_draw_frame(menu1,combolistscrollers(0).x,combolistscrollers(0).y,combolistscrollers(0).w,combolistscrollers(0).h,FR_ETCHED);
            
            for(int i=0; i<3; i++)
            {
                _allegro_hline(menu1,combolistscrollers(0).x+5-i,combolistscrollers(0).y+4+i, combolistscrollers(0).x+5+i, vc(0));
            }
            
            jwin_draw_frame(menu1,combolistscrollers(0).x,combolistscrollers(0).y+combolistscrollers(0).h-2,combolistscrollers(0).w,combolistscrollers(0).h,FR_ETCHED);
            
            for(int i=0; i<3; i++)
            {
                _allegro_hline(menu1,combolistscrollers(0).x+5-i,combolistscrollers(0).y+combolistscrollers(0).h+4-i, combolistscrollers(0).x+5+i, vc(0));
            }
        }
        
        textprintf_disabled(menu1,spfont,panel(0).x+panel(0).w-7,panel(0).y+3,jwin_pal[jcLIGHT],jwin_pal[jcMEDDARK],"%d",menutype+1);
        
        switch(pnl)
        {
            // New Large Mode single panel
        case -1:
        {
            // Coords1
            set_clip_rect(menu1,panel(8).x,panel(8).y,panel(8).x+panel(8).w-5,panel(8).y+panel(8).h);
            
            for(int i=0; i<4; i++)
            {
                jwin_draw_frame(menu1,panel(8).x+14+(32*i),panel(8).y+12,20,20,FR_DEEP);
                
                if(i==0 && scr->hasitem && scr->item > 0)
                {
                    rectfill(menu1,panel(8).x+16+(32*i),panel(8).y+14,panel(8).x+31+(32*i),panel(8).y+29,0);
                    overtile16(menu1, itemsbuf[scr->item].tile,panel(8).x+16+(32*i),panel(8).y+14,itemsbuf[scr->item].csets&15,0);
                }
                else
                    blit(icon_bmp[i][coord_frame], menu1, 0, 0, panel(8).x+16+(32*i),panel(8).y+14, 16, 16);
            }
            
            textprintf_centre_ex(menu1,font,panel(8).x+24+0*32,panel(8).y+34,jwin_pal[jcBOXFG],-1,"%d",scr->itemx);
            textprintf_centre_ex(menu1,font,panel(8).x+24+1*32,panel(8).y+34,jwin_pal[jcBOXFG],-1,"%d",scr->stairx);
            textprintf_centre_ex(menu1,font,panel(8).x+24+2*32,panel(8).y+34,jwin_pal[jcBOXFG],-1,"%d",scr->warparrivalx);
            textprintf_centre_ex(menu1,font,panel(8).x+24+3*32,panel(8).y+34,jwin_pal[jcBOXFG],-1,"%d",Flag);
            
            textprintf_centre_ex(menu1,font,panel(8).x+24+0*32,panel(8).y+42,jwin_pal[jcBOXFG],-1,"%d",scr->itemy);
            textprintf_centre_ex(menu1,font,panel(8).x+24+1*32,panel(8).y+42,jwin_pal[jcBOXFG],-1,"%d",scr->stairy);
            textprintf_centre_ex(menu1,font,panel(8).x+24+2*32,panel(8).y+42,jwin_pal[jcBOXFG],-1,"%d",scr->warparrivaly);
            
            // Coords2
            for(int i=0; i<4; i++)
            {
                jwin_draw_frame(menu1,panel(8).x+14+(32*i),panel(8).y+54,20,20,FR_DEEP);
                blit(icon_bmp[ICON_BMP_RETURN_A+i][coord_frame], menu1, 0, 0, panel(8).x+16+(32*i),panel(8).y+56, 16, 16);
            }
            
            textprintf_centre_ex(menu1,font,panel(8).x+24+0*32,panel(8).y+76,jwin_pal[jcBOXFG],-1,"%d",scr->warpreturnx[0]);
            textprintf_centre_ex(menu1,font,panel(8).x+24+1*32,panel(8).y+76,jwin_pal[jcBOXFG],-1,"%d",scr->warpreturnx[1]);
            textprintf_centre_ex(menu1,font,panel(8).x+24+2*32,panel(8).y+76,jwin_pal[jcBOXFG],-1,"%d",scr->warpreturnx[2]);
            textprintf_centre_ex(menu1,font,panel(8).x+24+3*32,panel(8).y+76,jwin_pal[jcBOXFG],-1,"%d",scr->warpreturnx[3]);
            
            textprintf_centre_ex(menu1,font,panel(8).x+24+0*32,panel(8).y+84,jwin_pal[jcBOXFG],-1,"%d",scr->warpreturny[0]);
            textprintf_centre_ex(menu1,font,panel(8).x+24+1*32,panel(8).y+84,jwin_pal[jcBOXFG],-1,"%d",scr->warpreturny[1]);
            textprintf_centre_ex(menu1,font,panel(8).x+24+2*32,panel(8).y+84,jwin_pal[jcBOXFG],-1,"%d",scr->warpreturny[2]);
            textprintf_centre_ex(menu1,font,panel(8).x+24+3*32,panel(8).y+84,jwin_pal[jcBOXFG],-1,"%d",scr->warpreturny[3]);
            
            // Enemies
            int epx = 2+panel(8).x+14+4*32;
            int epy = 2+panel(8).y+12;
            jwin_draw_frame(menu1, epx-2,epy-2, 16*4+4,16*3+4,FR_DEEP);
            rectfill(menu1, epx, epy, -1+epx+16*4,-1+epy+16*3,vc(0));
            
            for(int i=0; i< 10 && Map.CurrScr()->enemy[i]!=0; i++)
            {
                int id = Map.CurrScr()->enemy[i];
                int tile = get_bit(quest_rules, qr_NEWENEMYTILES) ? guysbuf[id].e_tile : guysbuf[id].tile;
                int cset = guysbuf[id].cset;
                
                if(tile)
                    overtile16(menu1, tile+efrontfacingtile(id),epx+(i%4)*16,epy+((i/4)*16),cset,0);
            }
        }
        break;
        
        case m_block:
        {
            char name[256], shortname[256];
            strncpy(name,get_filename(filepath),255);
            
            if(name[0]==0)
            {
                sprintf(name, "[Untitled]");
            }
            
            strip_extra_spaces(name);
            shorten_string(shortname, name, pfont, 255, (panel(0).x+panel(0).w-86)-(panel(0).x+1)-4);
            set_clip_rect(menu1,panel(0).x,panel(0).y,panel(0).x+panel(0).w-5,panel(0).y+46);
            extract_name(filepath,name,FILENAME8__);
            textprintf_disabled(menu1,pfont,panel(0).x+1,panel(0).y+3,jwin_pal[jcLIGHT],jwin_pal[jcMEDDARK],"File:");
            textprintf_ex(menu1,pfont,panel(0).x+1,panel(0).y+13,vc(0),-1,"%s",shortname);
            textprintf_disabled(menu1,pfont,panel(0).x+1,panel(0).y+24,jwin_pal[jcLIGHT],jwin_pal[jcMEDDARK],"Combo:");
            textprintf_ex(menu1,pfont,panel(0).x+1+text_length(pfont, "Combo: "),panel(0).y+24,vc(0),-1,"%d",Combo);
            textprintf_disabled(menu1,pfont,panel(0).x+1,panel(0).y+34,jwin_pal[jcLIGHT],jwin_pal[jcMEDDARK],"Type:");
//        textprintf_ex(menu1,pfont,panel(0).x+1+text_length(pfont, "Type: "),panel(0).y+34,vc(0),-1,"%s",combotype_string[combobuf[Combo].type]);
            textprintf_ex(menu1,pfont,panel(0).x+1+text_length(pfont, "Type: "),panel(0).y+34,vc(0),-1,"%s",combo_class_buf[combobuf[Combo].type].name);
            textprintf_centre_disabled(menu1,spfont,panel(0).x+panel(0).w-76,panel(0).y+3,jwin_pal[jcLIGHT],jwin_pal[jcMEDDARK],"Combo");
            jwin_draw_frame(menu1,panel(0).x+panel(0).w-86,panel(0).y+9,20, 20, FR_DEEP);
            put_combo(menu1,panel(0).x+panel(0).w-84,panel(0).y+11,Combo,CSet,0,0);
            
            textprintf_centre_disabled(menu1,spfont,panel(0).x+panel(0).w-52,panel(0).y+3,jwin_pal[jcLIGHT],jwin_pal[jcMEDDARK],"Walk");
            jwin_draw_frame(menu1,panel(0).x+panel(0).w-62,panel(0).y+9,20, 20, FR_DEEP);
            put_combo(menu1,panel(0).x+panel(0).w-60,panel(0).y+11,Combo,CSet,0,0);
            put_walkflags(menu1,panel(0).x+panel(0).w-60,panel(0).y+11,Combo,0);
            
            textprintf_centre_disabled(menu1,spfont,panel(0).x+panel(0).w-28,panel(0).y+3,jwin_pal[jcLIGHT],jwin_pal[jcMEDDARK],"Cycle");
            jwin_draw_frame(menu1,panel(0).x+panel(0).w-38,panel(0).y+9,20, 20, FR_DEEP);
            
            if(NextCombo>0)
            {
                put_combo(menu1,panel(0).x+panel(0).w-36,panel(0).y+11,NextCombo,NextCSet,0,0);
            }
            else
            {
                if(InvalidStatic)
                {
                    for(int dy=0; dy<16; dy++)
                    {
                        for(int dx=0; dx<16; dx++)
                        {
                            menu1->line[dy+panel(0).y+11][dx+panel(0).x+panel(0).w-36]=vc((((rand()%100)/50)?0:8)+(((rand()%100)/50)?0:7));
                        }
                    }
                }
                else
                {
                    rectfill(menu1, panel(0).x+panel(0).w-36,panel(0).y+11, panel(0).x+panel(0).w-36+15,panel(0).y+11+15,vc(0));
                    rect(menu1, panel(0).x+panel(0).w-36,panel(0).y+11, panel(0).x+panel(0).w-36+15,panel(0).y+11+15,vc(15));
                    line(menu1, panel(0).x+panel(0).w-36,panel(0).y+11, panel(0).x+panel(0).w-36+15,panel(0).y+11+15,vc(15));
                    line(menu1, panel(0).x+panel(0).w-36,panel(0).y+11+15, panel(0).x+panel(0).w-36+15,panel(0).y+11,vc(15));
                }
            }
            
            textprintf_disabled(menu1,spfont,panel(0).x+panel(0).w-28,panel(0).y+32,jwin_pal[jcLIGHT],jwin_pal[jcMEDDARK],"CSet");
            textprintf_ex(menu1,spfont,panel(0).x+panel(0).w-10,panel(0).y+32,jwin_pal[jcBOXFG],-1,"%d", CSet);
            
            textprintf_disabled(menu1,spfont,panel(0).x+panel(0).w-32,panel(0).y+39,jwin_pal[jcLIGHT],jwin_pal[jcMEDDARK],"Layer");
            textprintf_ex(menu1,spfont,panel(0).x+panel(0).w-10,panel(0).y+39,jwin_pal[jcBOXFG],-1,"%d", CurrentLayer);
        }
        break;
        
        case m_coords:
            set_clip_rect(menu1,panel(1).x,panel(1).y,panel(1).x+panel(1).w-5,panel(1).y+46);
            
            for(int i=0; i<4; i++)
            {
                jwin_draw_frame(menu1,panel(1).x+14+(32*i),panel(1).y+4,20,20,FR_DEEP);
                
                if(i==0 && scr->hasitem && scr->item > 0)
                {
                    rectfill(menu1,panel(8).x+16+(32*i),panel(1).y+6,panel(1).x+31+(32*i),panel(1).y+21,0);
                    overtile16(menu1, itemsbuf[scr->item].tile,panel(1).x+16+(32*i),panel(1).y+6,itemsbuf[scr->item].csets&15,0);
                }
                else
                    blit(icon_bmp[i][coord_frame], menu1, 0, 0, panel(1).x+16+(32*i),panel(1).y+6, 16, 16);
            }
            
            textprintf_centre_ex(menu1,font,panel(1).x+24+0*32,panel(1).y+26,jwin_pal[jcBOXFG],-1,"%d",scr->itemx);
            textprintf_centre_ex(menu1,font,panel(1).x+24+1*32,panel(1).y+26,jwin_pal[jcBOXFG],-1,"%d",scr->stairx);
            //textprintf_centre_ex(menu1,font,panel(1).x+24+2*32,panel(1).y+26,jwin_pal[jcBOXFG],-1,"%d",scr->warpreturnx);
            textprintf_centre_ex(menu1,font,panel(1).x+24+2*32,panel(1).y+26,jwin_pal[jcBOXFG],-1,"%d",scr->warparrivalx);
            textprintf_centre_ex(menu1,font,panel(1).x+24+3*32,panel(1).y+26,jwin_pal[jcBOXFG],-1,"%d",Flag);
            
            textprintf_centre_ex(menu1,font,panel(1).x+24+0*32,panel(1).y+34,jwin_pal[jcBOXFG],-1,"%d",scr->itemy);
            textprintf_centre_ex(menu1,font,panel(1).x+24+1*32,panel(1).y+34,jwin_pal[jcBOXFG],-1,"%d",scr->stairy);
            //textprintf_centre_ex(menu1,font,panel(1).x+24+2*32,panel(1).y+34,jwin_pal[jcBOXFG],-1,"%d",scr->warpreturny);
            textprintf_centre_ex(menu1,font,panel(1).x+24+2*32,panel(1).y+34,jwin_pal[jcBOXFG],-1,"%d",scr->warparrivaly);
            
            break;
            
        case m_coords2:
            set_clip_rect(menu1,panel(7).x,panel(7).y,panel(7).x+panel(7).w-5,panel(7).y+46);
            
            for(int i=0; i<4; i++)
            {
                jwin_draw_frame(menu1,panel(7).x+14+(32*i),panel(7).y+4,20,20,FR_DEEP);
                blit(icon_bmp[ICON_BMP_RETURN_A+i][coord_frame], menu1, 0, 0, panel(7).x+16+(32*i),panel(7).y+6, 16, 16);
            }
            
            textprintf_centre_ex(menu1,font,panel(7).x+24+0*32,panel(7).y+26,jwin_pal[jcBOXFG],-1,"%d",scr->warpreturnx[0]);
            textprintf_centre_ex(menu1,font,panel(7).x+24+1*32,panel(7).y+26,jwin_pal[jcBOXFG],-1,"%d",scr->warpreturnx[1]);
            textprintf_centre_ex(menu1,font,panel(7).x+24+2*32,panel(7).y+26,jwin_pal[jcBOXFG],-1,"%d",scr->warpreturnx[2]);
            textprintf_centre_ex(menu1,font,panel(7).x+24+3*32,panel(7).y+26,jwin_pal[jcBOXFG],-1,"%d",scr->warpreturnx[3]);
            //textprintf_centre_ex(menu1,font,panel[7].x+24+4*32,panel[7].y+26,jwin_pal[jcBOXFG],-1,"%d",Flag);
            
            textprintf_centre_ex(menu1,font,panel(7).x+24+0*32,panel(7).y+34,jwin_pal[jcBOXFG],-1,"%d",scr->warpreturny[0]);
            textprintf_centre_ex(menu1,font,panel(7).x+24+1*32,panel(7).y+34,jwin_pal[jcBOXFG],-1,"%d",scr->warpreturny[1]);
            textprintf_centre_ex(menu1,font,panel(7).x+24+2*32,panel(7).y+34,jwin_pal[jcBOXFG],-1,"%d",scr->warpreturny[2]);
            textprintf_centre_ex(menu1,font,panel(7).x+24+3*32,panel(7).y+34,jwin_pal[jcBOXFG],-1,"%d",scr->warpreturny[3]);
            
            break;
            
        case m_flags:
        {
            set_clip_rect(menu1,panel(2).x,panel(2).y,panel(2).x+panel(2).w-5,panel(2).y+46);
            
            byte f=scr->flags;
            byte wf=scr->flags2;
            byte f3=scr->flags3;
            char *flagheader=(char *)"E_WSLE_HET_S_MLW_DIB";
            char flagdata[30];
            
            for(byte i=0; i<strlen(flagheader); ++i)
            {
                textprintf_centre_disabled(menu1,font,panel(2).x+37+(i*6),panel(2).y+6,jwin_pal[jcLIGHT],jwin_pal[jcMEDDARK],"%c",flagheader[i]);
            }
            
            textprintf_disabled(menu1,font,panel(2).x+5,panel(2).y+14,jwin_pal[jcLIGHT],jwin_pal[jcMEDDARK],"Flags:");
            
            sprintf(flagdata,"%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d",bit(f3,7),bit(f3,6),bit(f3,5),bit(f3,4),bit(f3,3),bit(f3,2),bit(f3,1),bit(f3,0),bit(wf,7),bit(wf,6),bit(wf,5),bit(wf,4),bit(f,7),bit(f,6),bit(f,5),bit(f,4),bit(f,3),bit(f,2),bit(f,1),bit(f,0));
            
            for(byte i=0; i<strlen(flagheader); ++i)
            {
                textprintf_centre_ex(menu1,font,panel(2).x+37+(i*6),panel(2).y+14,jwin_pal[jcBOXFG],-1,"%c",flagdata[i]);
            }
            
            f=scr->enemyflags;
            char *enemyflagheader=(char *)"BILFR24Z";
            char enemyflagdata[30];
            
            for(byte i=0; i<strlen(enemyflagheader); ++i)
            {
                textprintf_centre_disabled(menu1,font,panel(2).x+43+(i*6),panel(2).y+26,jwin_pal[jcLIGHT],jwin_pal[jcMEDDARK],"%c",enemyflagheader[i]);
            }
            
            textprintf_disabled(menu1,font,panel(2).x+5,panel(2).y+34,jwin_pal[jcLIGHT],jwin_pal[jcMEDDARK],"Enemy:");
            sprintf(enemyflagdata,"%d%d%d%d%d%d%d%d",bit(f,7),bit(f,6),bit(f,5),bit(f,4),bit(f,3),bit(f,2),bit(f,1),bit(f,0));
            
            for(byte i=0; i<strlen(enemyflagheader); ++i)
            {
                textprintf_centre_ex(menu1,font,panel(2).x+43+(i*6),panel(2).y+34,jwin_pal[jcBOXFG],-1,"%c",enemyflagdata[i]);
            }
            
            textprintf_disabled(menu1,font,panel(2).x+101,panel(2).y+26,jwin_pal[jcLIGHT],jwin_pal[jcMEDDARK],"Pattern:");
            textprintf_ex(menu1,font,panel(2).x+101,panel(2).y+34,jwin_pal[jcBOXFG],-1,"%s",short_pattern_string[(Map.CurrScr()->pattern)]);
        }
        break;
        
        case m_guy:
        {
            set_clip_rect(menu1,panel(3).x,panel(3).y,panel(3).x+panel(3).w-5,panel(3).y+46);
            char buf[MSGSIZE+1], shortbuf[MSGSIZE+1];
            strncpy(buf,MsgString(scr->str, true, false),72);
            buf[MSGSIZE] = '\0';
            
            if((scr->str)==0)
            {
                sprintf(buf, "(None)");
            }
            
            strip_extra_spaces(buf);
            shorten_string(shortbuf, buf, pfont, 72, 140);
            textprintf_disabled(menu1,pfont,panel(3).x+6,panel(0).y+8,jwin_pal[jcLIGHT],jwin_pal[jcMEDDARK],"Guy:");
            textprintf_disabled(menu1,pfont,panel(3).x+6,panel(0).y+16,jwin_pal[jcLIGHT],jwin_pal[jcMEDDARK],"String:");
            textprintf_disabled(menu1,pfont,panel(3).x+6,panel(0).y+24,jwin_pal[jcLIGHT],jwin_pal[jcMEDDARK],"Room:");
            textprintf_ex(menu1,pfont,panel(3).x+40-16,panel(3).y+8,jwin_pal[jcBOXFG],-1,"%s",guy_string[scr->guy]);
            textprintf_ex(menu1,pfont,panel(3).x+40-6,panel(3).y+16,jwin_pal[jcBOXFG],-1,"%s",shortbuf);
            textprintf_ex(menu1,pfont,panel(3).x+40-10,panel(3).y+24,jwin_pal[jcBOXFG],-1,"%s",roomtype_string[scr->room]);
            int rtype=scr->room;
            
            if(strcmp(catchall_string[rtype]," "))
            {
                textprintf_disabled(menu1,pfont,panel(3).x+6,panel(0).y+32,jwin_pal[jcLIGHT],jwin_pal[jcMEDDARK],"%s:",catchall_string[rtype]);
                int xofs=text_length(pfont,catchall_string[rtype])+5;
                
                switch(rtype)
                {
                case rSP_ITEM:
                    textprintf_ex(menu1,pfont,panel(3).x+7+xofs,panel(3).y+32,jwin_pal[jcBOXFG],-1,"%s",item_string[scr->catchall]);
                    break;
                    
                case rINFO:
                    textprintf_ex(menu1,pfont,panel(3).x+7+xofs,panel(3).y+32,jwin_pal[jcBOXFG],-1,"(%d) %s",scr->catchall,misc.info[scr->catchall].name);
                    break;
                    
                case rP_SHOP:
                case rSHOP:
                    textprintf_ex(menu1,pfont,panel(3).x+7+xofs,panel(3).y+32,jwin_pal[jcBOXFG],-1,"(%d) %s",scr->catchall,misc.shop[scr->catchall].name);
                    break;
                    
                default:
                    textprintf_ex(menu1,pfont,panel(3).x+7+xofs,panel(3).y+32,jwin_pal[jcBOXFG],-1,"%d",scr->catchall);
                    break;
                }
            }
        }
        break;
        
        case m_warp:
            set_clip_rect(menu1,panel(4).x,panel(4).y,panel(4).x+panel(4).w-5,panel(4).y+46);
            
            textprintf_disabled(menu1,font,panel(4).x+7,panel(4).y+6,jwin_pal[jcLIGHT],jwin_pal[jcMEDDARK],"Tile Warp:");
            textprintf_disabled(menu1,font,panel(4).x+7,panel(4).y+14,jwin_pal[jcLIGHT],jwin_pal[jcMEDDARK],"Type:");
            textprintf_disabled(menu1,font,panel(4).x+7,panel(4).y+26,jwin_pal[jcLIGHT],jwin_pal[jcMEDDARK],"Side Warp:");
            textprintf_disabled(menu1,font,panel(4).x+7,panel(4).y+34,jwin_pal[jcLIGHT],jwin_pal[jcMEDDARK],"Type:");
            textprintf_ex(menu1,font,panel(4).x+59,panel(4).y+6,jwin_pal[jcBOXFG],-1,"%d:%02X",Map.CurrScr()->tilewarpdmap[0],scr->tilewarpscr[0]);
            textprintf_ex(menu1,font,panel(4).x+59,panel(4).y+14,jwin_pal[jcBOXFG],-1,"%s",warptype_string[scr->tilewarptype[0]]);
            
            textprintf_ex(menu1,font,panel(4).x+59,panel(4).y+26,jwin_pal[jcBOXFG],-1,"%d:%02X",Map.CurrScr()->sidewarpdmap[0],scr->sidewarpscr[0]);
            textprintf_ex(menu1,font,panel(4).x+59,panel(4).y+34,jwin_pal[jcBOXFG],-1,"%s",warptype_string[scr->sidewarptype[0]]);
            break;
            
        case m_misc:
        {
            set_clip_rect(menu1,panel(5).x,panel(5).y,panel(5).x+panel(5).w-5,panel(5).y+46);
            
            textprintf_disabled(menu1,font,panel(5).x+7,panel(5).y+14,jwin_pal[jcLIGHT],jwin_pal[jcMEDDARK],"Triggers:");
            byte wf=scr->flags2;
            char *triggerheader=(char *)"TBLR";
            char triggerdata[30];
            
            for(byte i=0; i<strlen(triggerheader); ++i)
            {
                textprintf_centre_disabled(menu1,font,panel(5).x+57+(i*6),panel(5).y+6,jwin_pal[jcLIGHT],jwin_pal[jcMEDDARK],"%c",triggerheader[i]);
            }
            
            sprintf(triggerdata,"%d%d%d%d",bit(wf,0),bit(wf,1),bit(wf,2),bit(wf,3));
            
            for(byte i=0; i<strlen(triggerheader); ++i)
            {
                textprintf_centre_ex(menu1,font,panel(5).x+57+(i*6),panel(5).y+14,jwin_pal[jcBOXFG],-1,"%c",triggerdata[i]);
            }
            
            textprintf_disabled(menu1,font,panel(5).x+7,panel(5).y+26,jwin_pal[jcLIGHT],jwin_pal[jcMEDDARK],"Path:");
            textprintf_disabled(menu1,font,panel(5).x+7,panel(5).y+34,jwin_pal[jcLIGHT],jwin_pal[jcMEDDARK],"Exit dir:");
            textprintf_ex(menu1,font,panel(5).x+54,panel(5).y+26,jwin_pal[jcBOXFG],-1,"%s",scr->flags&64?pathstr(scr->path):"(None)");
            textprintf_ex(menu1,font,panel(5).x+54,panel(5).y+34,jwin_pal[jcBOXFG],-1,"%s",scr->flags&64?dirstr[scr->exitdir]:"(None)");
        }
        break;
        
        case m_layers:
            if(!is_large())
            {
                set_clip_rect(menu1,panel(6).x,panel(6).y,panel(6).x+panel(6).w-5,panel(6).y+46);
                
                textprintf_centre_disabled(menu1,font,panel(6).x+88,panel(6).y+2,jwin_pal[jcLIGHT],jwin_pal[jcMEDDARK],"Layers");
                textprintf_centre_disabled(menu1,font,panel(6).x+13,panel(6).y+11,jwin_pal[jcLIGHT],jwin_pal[jcMEDDARK],"0");
                draw_checkbox(menu1,panel(6).x+9,panel(6).y+20,vc(1),vc(14), LayerMaskInt[0]!=0);
                textprintf_centre_disabled(menu1,font,panel(6).x+38,panel(6).y+11,jwin_pal[jcLIGHT],jwin_pal[jcMEDDARK],"1");
                draw_checkbox(menu1,panel(6).x+34,panel(6).y+20,vc(1),vc(14), LayerMaskInt[1]!=0);
                textprintf_centre_disabled(menu1,font,panel(6).x+63,panel(6).y+11,jwin_pal[jcLIGHT],jwin_pal[jcMEDDARK],"2");
                draw_checkbox(menu1,panel(6).x+59,panel(6).y+20,vc(1),vc(14), LayerMaskInt[2]!=0);
                textprintf_centre_disabled(menu1,font,panel(6).x+88,panel(6).y+11,jwin_pal[jcLIGHT],jwin_pal[jcMEDDARK],"3");
                draw_checkbox(menu1,panel(6).x+84,panel(6).y+20,vc(1),vc(14), LayerMaskInt[3]!=0);
                textprintf_centre_disabled(menu1,font,panel(6).x+113,panel(6).y+11,jwin_pal[jcLIGHT],jwin_pal[jcMEDDARK],"4");
                draw_checkbox(menu1,panel(6).x+109,panel(6).y+20,vc(1),vc(14), LayerMaskInt[4]!=0);
                textprintf_centre_disabled(menu1,font,panel(6).x+138,panel(6).y+11,jwin_pal[jcLIGHT],jwin_pal[jcMEDDARK],"5");
                draw_checkbox(menu1,panel(6).x+134,panel(6).y+20,vc(1),vc(14), LayerMaskInt[5]!=0);
                textprintf_centre_disabled(menu1,font,panel(6).x+163,panel(6).y+11,jwin_pal[jcLIGHT],jwin_pal[jcMEDDARK],"6");
                draw_checkbox(menu1,panel(6).x+159,panel(6).y+20,vc(1),vc(14), LayerMaskInt[6]!=0);
                draw_layerradio(menu1,panel(6).x+9,panel(6).y+30,vc(1),vc(14), CurrentLayer);
                
                textprintf_disabled(menu1,spfont,panel(6).x+panel(6).w-28,panel(6).y+36,jwin_pal[jcLIGHT],jwin_pal[jcMEDDARK],"CSet");
                textprintf_ex(menu1,spfont,panel(6).x+panel(6).w-10,panel(6).y+36,jwin_pal[jcBOXFG],-1,"%d", CSet);
            }
            
            break;
        }                                                         //switch(menutype)
    }
}

void show_screen_error(const char *str, int i, int c)
{
    rectfill(menu1, 575-text_length(lfont_l,str),388-(i*16),575,388-((i-1)*16)-4,vc(0));
    textout_shadowed_ex(menu1,lfont_l, str,575-text_length(lfont_l,str),388-(i*16),c,vc(0),-1);
}

void tile_warp_notification(int which, char *buf)
{
    char letter = 'A'+which;
    
    switch(Map.CurrScr()->tilewarptype[which])
    {
    case wtCAVE:
        sprintf(buf,"Tile Warp %c: Cave/Item Cellar",letter);
        break;
        
    default:
    {
        char buf2[25];
        
        if(strlen(DMaps[Map.CurrScr()->tilewarpdmap[which]].name)==0)
        {
            sprintf(buf2,"%d",Map.CurrScr()->tilewarpdmap[which]);
        }
        else
            sprintf(buf2,"%d-%s",Map.CurrScr()->tilewarpdmap[which],DMaps[Map.CurrScr()->tilewarpdmap[which]].name);
            
        sprintf(buf,"Tile Warp %c: %s, %02X", letter, buf2, Map.CurrScr()->tilewarpscr[which]);
        break;
    }
    
    case wtNOWARP:
        sprintf(buf,"Tile Warp %c: Cancel Warp", letter);
        break;
    }
}

void side_warp_notification(int which, int dir, char *buf)
{
    char letter = 'A'+which;
    char buf3[16];
    
    if(dir==0 && Map.CurrScr()->timedwarptics)
        sprintf(buf3,"%s, Timed",dirstr[dir]);
    else if(dir==4)
        sprintf(buf3,"Timed");
    else
        strcpy(buf3, dirstr[dir]);
        
    switch(Map.CurrScr()->sidewarptype[which])
    {
    case wtCAVE:
        sprintf(buf,"Side Warp %c (%s): Cave/Item Cellar",letter, buf3);
        break;
        
    default:
    {
        // Destination DMap name
        if(strlen(DMaps[Map.CurrScr()->sidewarpdmap[which]].name)==0)
        {
            sprintf(buf,"Side Warp %c (%s): %d, %02X", letter, buf3, Map.CurrScr()->sidewarpdmap[which], Map.CurrScr()->sidewarpscr[which]);
        }
        else
            sprintf(buf,"Side Warp %c (%s): %d-%s, %02X", letter, buf3, Map.CurrScr()->sidewarpdmap[which],DMaps[Map.CurrScr()->sidewarpdmap[which]].name, Map.CurrScr()->sidewarpscr[which]);
            
        break;
    }
    
    case wtNOWARP:
        sprintf(buf,"Side Warp %c (%s): Cancel Warp", letter, buf3);
        break;
    }
}

static bool arrowcursor = true; // Used by combo aliases and Combo Brush cursors. -L

void refresh(int flags)
{
    // CPage = Map.CurrScr()->cpage;
    int curscr;
    
    if(flags&rCLEAR)
        clear_to_color(menu1,vc(0));
        
    if(flags&rMAP)
    {
        if(!layers_valid(Map.CurrScr()))
            fix_layers(Map.CurrScr(), true);
            
        curscr=Map.getCurrScr();
        Map.setCurrScr(curscr);                                 // to update palette
        clear_to_color(mapscreenbmp,vc(0));
        Map.draw(mapscreenbmp, showedges()?16:0, showedges()?16:0, Flags, -1, -1);
        
        if(showedges())
        {
            if(Map.getCurrScr()<128)
            {
                //not the first row of screens
                if(Map.getCurrScr()>15)
                {
                    Map.drawrow(mapscreenbmp, 16, 0, Flags, 160, -1, Map.getCurrScr()-16);
                }
                else
                {
                    Map.drawstaticrow(mapscreenbmp, 16, 0);
                }
                
                //not the last row of screens
                if(Map.getCurrScr()<112)
                {
                    Map.drawrow(mapscreenbmp, 16, 192, Flags, 0, -1, Map.getCurrScr()+16);
                }
                else
                {
                    Map.drawstaticrow(mapscreenbmp, 16, 192);
                }
                
                //not the first column of screens
                if(Map.getCurrScr()&0x0F)
                {
                    Map.drawcolumn(mapscreenbmp, 0, 16, Flags, 15, -1, Map.getCurrScr()-1);
                }
                else
                {
                    Map.drawstaticcolumn(mapscreenbmp, 0, 16);
                }
                
                //not the last column of screens
                if((Map.getCurrScr()&0x0F)<15)
                {
                    Map.drawcolumn(mapscreenbmp, 272, 16, Flags, 0, -1, Map.getCurrScr()+1);
                }
                else
                {
                    Map.drawstaticcolumn(mapscreenbmp, 272, 16);
                }
                
                //not the first row or first column of screens
                if((Map.getCurrScr()>15)&&(Map.getCurrScr()&0x0F))
                {
                    Map.drawblock(mapscreenbmp, 0, 0, Flags, 175, -1, Map.getCurrScr()-17);
                }
                else
                {
                    Map.drawstaticblock(mapscreenbmp, 0, 0);
                }
                
                //not the first row or last column of screens
                if((Map.getCurrScr()>15)&&((Map.getCurrScr()&0x0F)<15))
                {
                    Map.drawblock(mapscreenbmp, 272, 0, Flags, 160, -1, Map.getCurrScr()-15);
                }
                else
                {
                    Map.drawstaticblock(mapscreenbmp, 272, 0);
                }
                
                //not the last row or first column of screens
                if((Map.getCurrScr()<112)&&(Map.getCurrScr()&0x0F))
                {
                    Map.drawblock(mapscreenbmp, 0, 192, Flags, 15, -1, Map.getCurrScr()+15);
                }
                else
                {
                    Map.drawstaticblock(mapscreenbmp, 0, 192);
                }
                
                //not the last row or last column of screens
                if((Map.getCurrScr()<112)&&((Map.getCurrScr()&0x0F)<15))
                {
                    Map.drawblock(mapscreenbmp, 272, 192, Flags, 0, -1, Map.getCurrScr()+17);
                }
                else
                {
                    Map.drawstaticblock(mapscreenbmp, 272, 192);
                }
            }
        }
        
        if(showxypos_icon)
        {
            if(showxypos_color==vc(15))
                safe_rect(mapscreenbmp,showxypos_x+(showedges()?16:0),showxypos_y+(showedges()?16:0),showxypos_x+(showedges()?16:0)+showxypos_w-1,showxypos_y+(showedges()?16:0)+showxypos_h-1,showxypos_color);
            else
                rectfill(mapscreenbmp,showxypos_x+(showedges()?16:0),showxypos_y+(showedges()?16:0),showxypos_x+(showedges()?16:0)+showxypos_w-1,showxypos_y+(showedges()?16:0)+showxypos_h-1,showxypos_color);
        }
        
        if(showxypos_cursor_icon)
        {
            safe_rect(mapscreenbmp,showxypos_cursor_x+(showedges()?16:0),showxypos_cursor_y+(showedges()?16:0),showxypos_cursor_x+(showedges()?16:0)+showxypos_w-1,showxypos_cursor_y+(showedges()?16:0)+showxypos_h-1,vc(15));
        }
        
        if(ShowSquares)
        {
            if(Map.CurrScr()->stairx || Map.CurrScr()->stairy)
            {
                int x1 = Map.CurrScr()->stairx+(showedges()?16:0);
                int y1 = Map.CurrScr()->stairy+(showedges()?16:0);
                safe_rect(mapscreenbmp,x1,y1,x1+15,y1+15,vc(14));
            }
            
            if(Map.CurrScr()->warparrivalx || Map.CurrScr()->warparrivaly)
            {
                int x1 = Map.CurrScr()->warparrivalx +(showedges()?16:0);
                int y1 = Map.CurrScr()->warparrivaly +(showedges()?16:0);
                safe_rect(mapscreenbmp,x1,y1,x1+15,y1+15,vc(10));
            }
            
            for(int i=0; i<4; i++) if(Map.CurrScr()->warpreturnx[i] || Map.CurrScr()->warpreturny[i])
                {
                    int x1 = Map.CurrScr()->warpreturnx[i]+(showedges()?16:0);
                    int y1 = Map.CurrScr()->warpreturny[i]+(showedges()?16:0);
                    int clr = vc(9);
                    
                    if(FlashWarpSquare==i)
                    {
                        if(!FlashWarpClk)
                            FlashWarpSquare=-1;
                        else if(!(--FlashWarpClk%3))
                            clr = vc(15);
                    }
                    
                    safe_rect(mapscreenbmp,x1,y1,x1+15,y1+15,clr);
                }
                
            /*
                  for (int i=0; i<4; i++) for (int j=0; j<9; i++)
                  {
                    int x1 = stx[i][j]+(showedges?16:0);
                    int y1 = sty[i][j]+(showedges?16:0);
                    rect(mapscreenbmp,x1,y1,x1+15,y1+15,vc(15));
                  }
            */
            
        }
        
        if(mapscreensize()==1)
        {
            blit(mapscreenbmp,menu1,0,0,mapscreen_x(),mapscreen_y(), 16 * (showedges() ? 18 : 16), 16 * (showedges() ? 13 : 11));
        }
        else
        {
            stretch_blit(mapscreenbmp,menu1,0,0, 16 * (showedges() ? 18 : 16), 16 * (showedges() ? 13 : 11),mapscreen_x(),mapscreen_y(),int(mapscreensize()* 16 * (showedges() ? 18 : 16)),int(mapscreensize()* 16 * (showedges() ? 13 : 11)));
        }
        
        if(showedges())
        {
            //top preview
            for(int j=0; j<int(16*mapscreensize()); j++)
            {
                for(int i=0; i<288*mapscreensize(); i++)
                {
                    if(((i^j)&1)==0)
                    {
                        putpixel(menu1,mapscreen_x()+i,mapscreen_y()+j,vc(0));
                    }
                }
            }
            
            //bottom preview
            for(int j=int(192*mapscreensize()); j<int(208*mapscreensize()); j++)
            {
                for(int i=0; i<288*mapscreensize(); i++)
                {
                    if(((i^j)&1)==0)
                    {
                        putpixel(menu1,mapscreen_x()+i,mapscreen_y()+j,vc(0));
                    }
                }
            }
            
            //left preview
            for(int j=int(16*mapscreensize()); j<int(192*mapscreensize()); j++)
            {
                for(int i=0; i<16*mapscreensize(); i++)
                {
                    if(((i^j)&1)==0)
                    {
                        putpixel(menu1,mapscreen_x()+i,mapscreen_y()+j,vc(0));
                    }
                }
                
            }
            
            //right preview
            for(int j=int(16*mapscreensize()); j<int(192*mapscreensize()); j++)
            {
                for(int i=int(272*mapscreensize()); i<int(288*mapscreensize()); i++)
                {
                    if(((i^j)&1)==0)
                    {
                        putpixel(menu1,mapscreen_x()+i,mapscreen_y()+j,vc(0));
                    }
                }
            }
        }
        
        if(!(Flags&cDEBUG))
        {
            for(int j=int(168*mapscreensize()); j<int(176*mapscreensize()); j++)
            {
                for(int i=0; i<int(256*mapscreensize()); i++)
                {
                
                    if(((i^j)&1)==0)
                    {
                        putpixel(menu1,int(mapscreen_x()+(showedges()?(16*mapscreensize()):0)+i),
                                 int(mapscreen_y()+(showedges()?(16*mapscreensize()):0)+j),vc(blackout_color()));
                    }
                }
            }
        }
        
        if((Map.isDark()) && !(Flags&cNODARK))
        {
            for(int j=0; j<80*mapscreensize(); j++)
            {
                for(int i=0; i<(80*mapscreensize())-j; i++)
                {
                    if(((i^j)&1)==0)
                    {
                        putpixel(menu1,int(mapscreen_x()+(showedges()?(16*mapscreensize()):0))+i,
                                 int(mapscreen_y()+(showedges()?(16*mapscreensize()):0)+j),vc(blackout_color()));
                    }
                }
            }
        }
        
        double startx=mapscreen_x()+(showedges()?(16*mapscreensize()):0);
        double starty=mapscreen_y()+(showedges()?(16*mapscreensize()):0);
        int startxint=mapscreen_x()+(showedges()?int(16*mapscreensize()):0);
        int startyint=mapscreen_y()+(showedges()?int(16*mapscreensize()):0);
        bool inrect = isinRect(Backend::mouse->getVirtualScreenX(), Backend::mouse->getVirtualScreenY(),startxint,startyint,int(startx+(256*mapscreensize())-1),int(starty+(176*mapscreensize())-1));
        
        if(!(flags&rNOCURSOR) && ((ComboBrush && !ComboBrushPause)||draw_mode==dm_alias) && inrect)
        {
            arrowcursor = false;
            int mgridscale=16*mapscreensize();
            Backend::mouse->setCursorSprite(mouse_bmp[MOUSE_BMP_BLANK][0]);
            int mx=(Backend::mouse->getVirtualScreenX()-(showedges()?mgridscale:0))/mgridscale*mgridscale;
            int my=(Backend::mouse->getVirtualScreenY()-16-(showedges()?mgridscale:0))/mgridscale*mgridscale;
            clear_bitmap(brushscreen);
            int tempbw=BrushWidth;
            int tempbh=BrushHeight;
            
            if(draw_mode==dm_alias)
            {
                BrushWidth = combo_aliases[combo_apos].width+1;
                BrushHeight = combo_aliases[combo_apos].height+1;
            }
            
            if((FloatBrush)&&(draw_mode!=dm_alias))
            {
                if(is_large())
                {
                    stretch_blit(brushbmp, brushscreen, 0, 0, BrushWidth*16, BrushHeight*16, mx+(showedges()?mgridscale:0)-(SHADOW_DEPTH*mapscreensize()), my+(showedges()?mgridscale:0)-(SHADOW_DEPTH*mapscreensize()), BrushWidth*mgridscale, BrushHeight*mgridscale);
                }
                else
                {
                    blit(brushbmp, brushscreen, 0, 0, mx+(showedges()?mgridscale:0)-SHADOW_DEPTH, my+(showedges()?mgridscale:0)-SHADOW_DEPTH, BrushWidth*mgridscale, BrushHeight*mgridscale);
                }
                
                //shadow
                for(int i=0; i<SHADOW_DEPTH*mapscreensize(); i++)
                {
                    for(int j=0; j<BrushHeight*mgridscale; j++)
                    {
                        if((((i^j)&1)==1) && (my+j)<12*mgridscale)
                        {
                            putpixel(brushscreen,mx+(showedges()?mgridscale:0)+i+(BrushWidth*mgridscale)-(SHADOW_DEPTH*mapscreensize()),my+(showedges()?mgridscale:0)+j,vc(0));
                        }
                    }
                }
                
                for(int i=0; i<BrushWidth*mgridscale; i++)
                {
                    for(int j=0; j<SHADOW_DEPTH*mapscreensize(); j++)
                    {
                        if((((i^j)&1)==1) && (mx+i)<16*mgridscale)
                        {
                            putpixel(brushscreen,mx+(showedges()?mgridscale:0)+i,my+(showedges()?mgridscale:0)+j+(BrushHeight*mgridscale)-(SHADOW_DEPTH*mapscreensize()),vc(0));
                        }
                    }
                }
            }
            else
            {
                if(draw_mode!=dm_alias)
                {
                    if(is_large())
                    {
                        stretch_blit(brushbmp, brushscreen, 0, 0, BrushWidth*16, BrushHeight*16, mx+(showedges()?mgridscale:0), my+(showedges()?mgridscale:0), BrushWidth*mgridscale, BrushHeight*mgridscale);
                    }
                    else
                    {
                        blit(brushbmp, brushscreen, 0, 0, mx+(showedges()?mgridscale:0), my+(showedges()?mgridscale:0), BrushWidth*mgridscale, BrushHeight*mgridscale);
                    }
                }
                else
                {
                    combo_alias *combo = &combo_aliases[combo_apos];
                    
                    if(is_large())
                    {
                        switch(alias_origin)
                        {
                        case 0:
                            stretch_blit(brushbmp, brushscreen, 0,                                                                   0,                                                                     BrushWidth*16, BrushHeight*16, mx+(showedges()?mgridscale:0),                                       my+(showedges()?mgridscale:0),                                        BrushWidth*mgridscale, BrushHeight*mgridscale);
                            break;
                            
                        case 1:
                            stretch_blit(brushbmp, brushscreen, (mx<combo->width*mgridscale)?((combo->width)*16)-mx/mapscreensize():0, 0,                                                                     BrushWidth*16, BrushHeight*16, zc_max((mx-(combo->width)*mgridscale),0)+(showedges()?mgridscale:0), my+(showedges()?mgridscale:0),                                        BrushWidth*mgridscale, BrushHeight*mgridscale);
                            break;
                            
                        case 2:
                            stretch_blit(brushbmp, brushscreen, 0, (my<combo->height*mgridscale)?((combo->height)*16)-my/mapscreensize():0, BrushWidth*16, BrushHeight*16, mx+(showedges()?mgridscale:0),                                       zc_max((my-(combo->height)*mgridscale),0)+(showedges()?mgridscale:0), BrushWidth*mgridscale, BrushHeight*mgridscale);
                            break;
                            
                        case 3:
                            stretch_blit(brushbmp, brushscreen, (mx<combo->width*mgridscale)?((combo->width)*16)-mx/mapscreensize():0, (my<combo->height*mgridscale)?((combo->height)*16)-my/mapscreensize():0, BrushWidth*16, BrushHeight*16, zc_max((mx-(combo->width)*mgridscale),0)+(showedges()?mgridscale:0), zc_max((my-(combo->height)*mgridscale),0)+(showedges()?mgridscale:0), BrushWidth*mgridscale, BrushHeight*mgridscale);
                            break;
                        }
                    }
                    else
                    {
                        switch(alias_origin)
                        {
                        case 0:
                            blit(brushbmp, brushscreen, 0,                                             0,                                               mx+(showedges()?mgridscale:0),                               my+(showedges()?mgridscale:0),                                BrushWidth*mgridscale, BrushHeight*mgridscale);
                            break;
                            
                        case 1:
                            blit(brushbmp, brushscreen, (mx<combo->width*16)?((combo->width)*16)-mx:0, 0,                                               zc_max((mx-(combo->width)*16),0)+(showedges()?mgridscale:0), my+(showedges()?mgridscale:0),                                BrushWidth*mgridscale, BrushHeight*mgridscale);
                            break;
                            
                        case 2:
                            blit(brushbmp, brushscreen, 0, (my<combo->height*16)?((combo->height)*16)-my:0, mx+(showedges()?mgridscale:0),                               zc_max((my-(combo->height)*16),0)+(showedges()?mgridscale:0), BrushWidth*mgridscale, BrushHeight*mgridscale);
                            break;
                            
                        case 3:
                            blit(brushbmp, brushscreen, (mx<combo->width*16)?((combo->width)*16)-mx:0, (my<combo->height*16)?((combo->height)*16)-my:0, zc_max((mx-(combo->width)*16),0)+(showedges()?mgridscale:0), zc_max((my-(combo->height)*16),0)+(showedges()?mgridscale:0), BrushWidth*mgridscale, BrushHeight*mgridscale);
                            break;
                        }
                    }
                }
            }
            
            masked_blit(brushscreen, menu1, 0, 0, 0, 16, (16+(showedges()?2:0))*mgridscale, (11+(showedges()?2:0))*mgridscale);
            BrushWidth=tempbw;
            BrushHeight=tempbh;
        }
        else
        {
            if(!arrowcursor)
            {
				Backend::mouse->setCursorSprite(mouse_bmp[MOUSE_BMP_NORMAL][0]);
                arrowcursor = true;
            }
        }
        
        if(ShowGrid)
        {
            int w=16;
            int h=11;
            
            if(showedges())
            {
                w=18;
                h=13;
            }
            
            for(int x=16; x<w*16; x+=16)
            {
                _allegro_vline(menu1, (x*mapscreensize())+mapscreen_x(), mapscreen_y(), mapscreen_y()+(h*16*mapscreensize())-1, vc(GridColor));
            }
            
            for(int y=16; y<h*16; y+=16)
            {
                _allegro_hline(menu1, mapscreen_x(), (y*mapscreensize())+mapscreen_y(), mapscreen_x()+(w*16*mapscreensize())-1, vc(GridColor));
            }
        }
        
        // Map tabs
        if(is_large())
        {
            map_page[current_mappage].map=Map.getCurrMap();
            map_page[current_mappage].screen=Map.getCurrScr();
            
            for(int btn=0; btn<(showedges()?9:8); ++btn)
            {
                char tbuf[10];
                sprintf(tbuf, "%d:%02X", map_page[btn].map+1, map_page[btn].screen);
                draw_text_button(menu1,map_page_bar(btn).x, map_page_bar(btn).y, map_page_bar(btn).w, map_page_bar(btn).h,tbuf,vc(1),vc(14),(btn==current_mappage?2:0),true);
            }
            
            draw_text_button(menu1,combolist_window().x-64,0,64,16,dm_names[draw_mode],vc(1),vc(14),0,true);
        }
    }
    
    if(flags&rSCRMAP)
    {
        //  text_mode(vc(0));
        rectfill(menu1, minimap().x-1, minimap().y-2,minimap().x+minimap().w-1,minimap().y+minimap().h+(is_large()?4:-1),jwin_pal[jcBOX]);
        // The frame.
        jwin_draw_frame(menu1,minimap().x,minimap().y+9,minimap().w-1, minimap().h-10, FR_DEEP);
        // The black bar covering screens 88-8F.
        rectfill(menu1, minimap().x+2+24* BMM(), minimap().y+12+24* BMM(),minimap().x+(27+22)*BMM(),minimap().y+11+27* BMM(),vc(0));

        safe_rect(menu1, minimap().x+2,minimap().y+11,minimap().x+3+48* BMM(),minimap().y+12+27* BMM(),vc(0));
        
        if(Map.getCurrMap()<Map.getMapCount())
        {
            for(int i=0; i<MAPSCRS; i++)
            {
                if(Map.Scr(i)->valid&mVALID)
                {
                    //vc(0)
                    rectfill(menu1,(i&15)*3* BMM() +minimap().x+3,(i/16)*3* BMM() +minimap().y+12,
                             (i&15)*3* BMM() +(is_large()?8:2)+minimap().x+3,(i/16)*3* BMM() +minimap().y+12+(is_large()?8:2), lc1((Map.Scr(i)->color)&15));
                             
                    if(((Map.Scr(i)->color)&15)>0)
                    {
                        if(!is_large())
                            putpixel(menu1,(i&15)*3* BMM() +1+minimap().x+3,(i/16)*3* BMM() +minimap().y+12+1,lc2((Map.Scr(i)->color)&15));
                        else
                            rectfill(menu1,(i&15)*3* BMM() +2+minimap().x+4,(i/16)*3* BMM() +minimap().y+11+4,(i&15)*3* BMM() +2+minimap().x+6,(i/16)*3* BMM() +minimap().y+11+6, lc2((Map.Scr(i)->color)&15));
                    }
                }
                else
                {
                    if(InvalidStatic)
                    {
                        for(int dy=0; dy<3*BMM(); dy++)
                        {
                            for(int dx=0; dx<3*BMM(); dx++)
                            {
                                menu1->line[dy+(i/16)*3*BMM()+minimap().y+12][dx+(i&15)*3* BMM() +minimap().x+3]=vc((((rand()%100)/50)?0:8)+(((rand()%100)/50)?0:7));
                            }
                        }
                    }
                    else
                    {
                        rectfill(menu1, (i&15)*3* BMM() +minimap().x+3, (i/16)*3* BMM() +minimap().y+12,
                                 (i&15)*3* BMM() +minimap().x+3+(1+ BMM()*BMM()), (i/16)*3* BMM() +minimap().y+12+(1+ BMM()*BMM()), vc(0));
                    }
                }
            }
            
            int s=Map.getCurrScr();
            // The white marker rect
            safe_rect(menu1,(s&15)*3*BMM()+minimap().x+3,(s/16)*3*BMM()+minimap().y+12,(s&15)*3*BMM()+(is_large()?8:2)+minimap().x+3,(s/16)*3*BMM()+minimap().y+12+(is_large()?8:2),vc(15));
            
            textprintf_disabled(menu1,font,minimap().x,minimap().y,jwin_pal[jcLIGHT],jwin_pal[jcMEDDARK],"M");
            textprintf_ex(menu1,font,minimap().x+8,minimap().y,jwin_pal[jcBOXFG],jwin_pal[jcBOX],"%-3d",Map.getCurrMap()+1);
            
            textprintf_disabled(menu1,font,minimap().x+36,minimap().y,jwin_pal[jcLIGHT],jwin_pal[jcMEDDARK],"S");
            textprintf_ex(menu1,font,minimap().x+36+8,minimap().y,jwin_pal[jcBOXFG],jwin_pal[jcBOX],"%02X",s);
        }
    }
    
    if(flags&rCOMBOS)
    {
        if(is_large())
        {
            jwin_draw_frame(menu1,combolist_window().x,combolist_window().y,combolist_window().w,combolist_window().h, FR_WIN);
            rectfill(menu1,combolist_window().x+2,combolist_window().y+2,combolist_window().x+combolist_window().w-3,combolist_window().y+combolist_window().h-3,jwin_pal[jcBOX]);
            jwin_draw_frame(menu1,combolistscrollers(0).x,combolistscrollers(0).y,combolistscrollers(0).w,combolistscrollers(0).h,FR_ETCHED);
            
            for(int i=0; i<3; i++)
            {
                _allegro_hline(menu1,combolistscrollers(0).x+5-i,combolistscrollers(0).y+4+i, combolistscrollers(0).x+5+i, vc(0));
            }
            
            jwin_draw_frame(menu1,combolistscrollers(0).x+combolistscrollers(0).w,combolistscrollers(0).y,combolistscrollers(0).w,combolistscrollers(0).h,FR_ETCHED);
            
            for(int i=0; i<3; i++)
            {
                _allegro_hline(menu1,combolistscrollers(0).x+combolistscrollers(0).w+5-i,combolistscrollers(0).y+6-i, combolistscrollers(0).x+combolistscrollers(0).w+5+i, vc(0));
            }
            
            jwin_draw_frame(menu1,combolistscrollers(1).x,combolistscrollers(1).y,combolistscrollers(1).w,combolistscrollers(1).h,FR_ETCHED);
            
            for(int i=0; i<3; i++)
            {
                _allegro_hline(menu1,combolistscrollers(1).x+5-i,combolistscrollers(1).y+4+i, combolistscrollers(1).x+5+i, vc(0));
            }
            
            jwin_draw_frame(menu1,combolistscrollers(1).x+combolistscrollers(1).w,combolistscrollers(1).y,combolistscrollers(1).w,combolistscrollers(1).h,FR_ETCHED);
            
            for(int i=0; i<3; i++)
            {
                _allegro_hline(menu1,combolistscrollers(1).x+combolistscrollers(1).w+5-i,combolistscrollers(1).y+6-i, combolistscrollers(1).x+combolistscrollers(1).w+5+i, vc(0));
            }
            
            jwin_draw_frame(menu1,combolistscrollers(2).x,combolistscrollers(2).y,combolistscrollers(2).w,combolistscrollers(2).h,FR_ETCHED);
            
            for(int i=0; i<3; i++)
            {
                _allegro_hline(menu1,combolistscrollers(2).x+5-i,combolistscrollers(2).y+4+i, combolistscrollers(2).x+5+i, vc(0));
            }
            
            jwin_draw_frame(menu1,combolistscrollers(2).x+combolistscrollers(2).w,combolistscrollers(2).y,combolistscrollers(2).w,combolistscrollers(2).h,FR_ETCHED);
            
            for(int i=0; i<3; i++)
            {
                _allegro_hline(menu1,combolistscrollers(2).x+combolistscrollers(2).w+5-i,combolistscrollers(2).y+6-i, combolistscrollers(2).x+combolistscrollers(2).w+5+i, vc(0));
            }
        }
        
        if(draw_mode!=dm_alias)
        {
            if(is_large())
            {
                jwin_draw_frame(menu1,combolist(0).x-2,combolist(0).y-2,(combolist(0).w<<4)+4,(combolist(0).h<<4)+4,FR_DEEP);
                jwin_draw_frame(menu1,combolist(1).x-2,combolist(1).y-2,(combolist(1).w<<4)+4,(combolist(1).h<<4)+4,FR_DEEP);
                jwin_draw_frame(menu1,combolist(2).x-2,combolist(2).y-2,(combolist(2).w<<4)+4,(combolist(2).h<<4)+4,FR_DEEP);
                
                if(MouseScroll)
                {
                    jwin_draw_frame(menu1,combolist(0).x-2,combolist(0).y-10,(combolist(0).w<<4)+4,6,FR_DEEP);
                    jwin_draw_frame(menu1,combolist(1).x-2,combolist(1).y-10,(combolist(1).w<<4)+4,6,FR_DEEP);
                    jwin_draw_frame(menu1,combolist(2).x-2,combolist(2).y-10,(combolist(2).w<<4)+4,6,FR_DEEP);
                    
                    rectfill(menu1,combolist(0).x,combolist(0).y-8,combolist(0).x+(combolist(0).w<<4)-1,combolist(0).y-7,jwin_pal[jcBOXFG]);
                    rectfill(menu1,combolist(1).x,combolist(1).y-8,combolist(1).x+(combolist(1).w<<4)-1,combolist(1).y-7,jwin_pal[jcBOXFG]);
                    rectfill(menu1,combolist(2).x,combolist(2).y-8,combolist(2).x+(combolist(2).w<<4)-1,combolist(2).y-7,jwin_pal[jcBOXFG]);
                    
                    jwin_draw_frame(menu1,combolist(0).x-2,combolist(0).y+(combolist(0).h<<4)+4,(combolist(0).w<<4)+4,6,FR_DEEP);
                    jwin_draw_frame(menu1,combolist(1).x-2,combolist(1).y+(combolist(1).h<<4)+4,(combolist(1).w<<4)+4,6,FR_DEEP);
                    jwin_draw_frame(menu1,combolist(2).x-2,combolist(2).y+(combolist(2).h<<4)+4,(combolist(2).w<<4)+4,6,FR_DEEP);
                    
                    rectfill(menu1,combolist(0).x,combolist(0).y+(combolist(0).h<<4)+6,combolist(0).x+(combolist(0).w<<4)-1,combolist(0).y+(combolist(0).h<<4)+7,jwin_pal[jcBOXFG]);
                    rectfill(menu1,combolist(1).x,combolist(1).y+(combolist(1).h<<4)+6,combolist(1).x+(combolist(1).w<<4)-1,combolist(1).y+(combolist(1).h<<4)+7,jwin_pal[jcBOXFG]);
                    rectfill(menu1,combolist(2).x,combolist(2).y+(combolist(2).h<<4)+6,combolist(2).x+(combolist(2).w<<4)-1,combolist(2).y+(combolist(2).h<<4)+7,jwin_pal[jcBOXFG]);
                }
            }
            
            int drawmap, drawscr;
            drawmap=Map.CurrScr()->layermap[CurrentLayer-1]-1;
            drawscr=Map.CurrScr()->layerscreen[CurrentLayer-1];
            
            for(int j=0; j<3; ++j)
            {
                if(j==0||is_large())
                {
                    for(int i=0; i<(combolist(j).w*combolist(j).h); i++)
                    {
                        put_combo(menu1,(i%combolist(j).w)*16+combolist(j).x,(i/combolist(j).w)*16+combolist(j).y,i+First[j],CSet,Flags&(cFLAGS|cWALK),0);
                    }
                }
            }
            
            int rect_pos=Combo-First[current_combolist];
            
            if((rect_pos>=0)&&(rect_pos<(First[current_combolist]+(combolist(current_combolist).w*combolist(current_combolist).h))))
                safe_rect(menu1, (rect_pos&(combolist(current_combolist).w-1))*16+combolist(current_combolist).x, (rect_pos/combolist(current_combolist).w)*16+combolist(current_combolist).y, ((rect_pos&(combolist(current_combolist).w-1))*16+combolist(current_combolist).x)+15, ((rect_pos/combolist(current_combolist).w)*16+combolist(current_combolist).y)+15, 255);
        }
        else
        {
            if(is_large())
            {
                jwin_draw_frame(menu1,comboaliaslist(0).x-2,comboaliaslist(0).y-2,(comboaliaslist(0).w<<4)+4,(comboaliaslist(0).h<<4)+4,FR_DEEP);
                jwin_draw_frame(menu1,comboaliaslist(1).x-2,comboaliaslist(1).y-2,(comboaliaslist(1).w<<4)+4,(comboaliaslist(1).h<<4)+4,FR_DEEP);
                jwin_draw_frame(menu1,comboaliaslist(2).x-2,comboaliaslist(2).y-2,(comboaliaslist(2).w<<4)+4,(comboaliaslist(2).h<<4)+4,FR_DEEP);
                
                jwin_draw_frame(menu1,comboalias_preview(0).x-2,comboalias_preview(0).y-2,comboalias_preview(0).w+4,comboalias_preview(0).h+4,FR_DEEP);
                jwin_draw_frame(menu1,comboalias_preview(1).x-2,comboalias_preview(1).y-2,comboalias_preview(1).w+4,comboalias_preview(1).h+4,FR_DEEP);
                jwin_draw_frame(menu1,comboalias_preview(2).x-2,comboalias_preview(2).y-2,comboalias_preview(2).w+4,comboalias_preview(2).h+4,FR_DEEP);
                
                if(MouseScroll)
                {
                    jwin_draw_frame(menu1,comboaliaslist(0).x-2,comboaliaslist(0).y-10,(comboaliaslist(0).w<<4)+4,6,FR_DEEP);
                    jwin_draw_frame(menu1,comboaliaslist(1).x-2,comboaliaslist(1).y-10,(comboaliaslist(1).w<<4)+4,6,FR_DEEP);
                    jwin_draw_frame(menu1,comboaliaslist(2).x-2,comboaliaslist(2).y-10,(comboaliaslist(2).w<<4)+4,6,FR_DEEP);
                    
                    rectfill(menu1,comboaliaslist(0).x,comboaliaslist(0).y-8,comboaliaslist(0).x+(comboaliaslist(0).w<<4)-1,comboaliaslist(0).y-7,jwin_pal[jcBOXFG]);
                    rectfill(menu1,comboaliaslist(1).x,comboaliaslist(1).y-8,comboaliaslist(1).x+(comboaliaslist(1).w<<4)-1,comboaliaslist(1).y-7,jwin_pal[jcBOXFG]);
                    rectfill(menu1,comboaliaslist(2).x,comboaliaslist(2).y-8,comboaliaslist(2).x+(comboaliaslist(2).w<<4)-1,comboaliaslist(2).y-7,jwin_pal[jcBOXFG]);
                    
                    jwin_draw_frame(menu1,comboalias_preview(0).x-2,comboalias_preview(0).y+comboalias_preview(0).h+4,comboalias_preview(0).w+4,6,FR_DEEP);
                    jwin_draw_frame(menu1,comboalias_preview(1).x-2,comboalias_preview(1).y+comboalias_preview(1).h+4,comboalias_preview(1).w+4,6,FR_DEEP);
                    jwin_draw_frame(menu1,comboalias_preview(2).x-2,comboalias_preview(2).y+comboalias_preview(2).h+4,comboalias_preview(2).w+4,6,FR_DEEP);
                    
                    rectfill(menu1,comboalias_preview(0).x,comboalias_preview(0).y+comboalias_preview(0).h+6,comboalias_preview(0).x+comboalias_preview(0).w-1,comboalias_preview(0).y+comboalias_preview(0).h+7,jwin_pal[jcBOXFG]);
                    rectfill(menu1,comboalias_preview(1).x,comboalias_preview(1).y+comboalias_preview(1).h+6,comboalias_preview(1).x+comboalias_preview(1).w-1,comboalias_preview(1).y+comboalias_preview(1).h+7,jwin_pal[jcBOXFG]);
                    rectfill(menu1,comboalias_preview(2).x,comboalias_preview(2).y+comboalias_preview(2).h+6,comboalias_preview(2).x+comboalias_preview(2).w-1,comboalias_preview(2).y+comboalias_preview(2).h+7,jwin_pal[jcBOXFG]);
                }
            }
            
            BITMAP *prv = create_bitmap_ex(8,64,64);
            clear_bitmap(prv);
            int scalefactor = 1;
            
            for(int j=0; j<3; ++j)
            {
                if(j==0||is_large())
                {
                    for(int i=0; i<(comboaliaslist(j).w*comboaliaslist(j).h); i++)
                    {
                        draw_combo_alias_thumbnail(menu1, &combo_aliases[combo_alistpos[j]+i], (i%comboaliaslist(j).w)*16+comboaliaslist(j).x,(i/comboaliaslist(j).w)*16+comboaliaslist(j).y,1);
                    }
                    
                    if((combo_aliases[combo_apos].width>7)||(combo_aliases[combo_apos].height>7))
                    {
                        scalefactor=4;
                    }
                    else if((combo_aliases[combo_apos].width>3)||(combo_aliases[combo_apos].height>3))
                    {
                        scalefactor=2;
                    }
                    
                    stretch_blit(brushbmp, prv, 0,0,scalefactor*64,zc_min(scalefactor*64,176),0,0,64,scalefactor==4?44:64);
                    blit(prv,menu1,0,0,comboalias_preview(j).x,comboalias_preview(j).y,comboalias_preview(j).w,comboalias_preview(j).h);
                }
                
                int rect_pos=combo_apos-combo_alistpos[current_comboalist];
                
                if((rect_pos>=0)&&(rect_pos<(combo_alistpos[current_comboalist]+(comboaliaslist(current_comboalist).w*comboaliaslist(current_comboalist).h))))
                    safe_rect(menu1,(rect_pos&(combolist(current_comboalist).w-1))*16+combolist(current_comboalist).x,(rect_pos/combolist(current_comboalist).w)*16+combolist(current_comboalist).y,((rect_pos&(combolist(current_comboalist).w-1))*16+combolist(current_comboalist).x)+15,((rect_pos/combolist(current_comboalist).w)*16+combolist(current_comboalist).y)+15,255);
            }
            
            destroy_bitmap(prv);
        }
    }
    
    if(flags&rCOMBO)
    {
        int drawmap, drawscr;
        drawmap=Map.CurrScr()->layermap[CurrentLayer-1]-1;
        drawscr=Map.CurrScr()->layerscreen[CurrentLayer-1];
        
        // Combo preview
        if(is_large())
        {
            static BITMAP *combo_preview_bmp=create_bitmap_ex(8,32,32);
            static BITMAP *cycle_preview_bmp=create_bitmap_ex(8,32,32);
            // Combo
			size_and_pos cprect = combo_preview();
            put_combo(combo_preview_bmp,0,0,(draw_mode==dm_alias)?combo_aliases[combo_apos].combos[0]:Combo,(draw_mode==dm_alias)?wrap(combo_aliases[combo_apos].csets[0]+alias_cset_mod, 0, 11):CSet,Flags&(cFLAGS|cWALK),0);
            jwin_draw_frame(menu1, cprect.x-2, cprect.y-2, cprect.w+4, cprect.h+4, FR_DEEP);
            stretch_blit(combo_preview_bmp, menu1, 0, 0, 16, 16, cprect.x, cprect.y, cprect.w, cprect.h);
            
            if(draw_mode!=dm_alias)
            {
                char buf[17];
                sprintf(buf,"Combo: %d",Combo);
                textprintf_ex(menu1,pfont, cprect.x-text_length(pfont,buf)-8, cprect.y+2,jwin_pal[jcBOXFG],jwin_pal[jcBOX],"%s",buf);
                sprintf(buf,"CSet: %d",CSet);
                int offs = 8;
                textprintf_ex(menu1,pfont, cprect.x-text_length(pfont,buf)-8, cprect.y+11,jwin_pal[jcBOXFG],jwin_pal[jcBOX],"%s",buf);
                strncpy(buf,combo_class_buf[combobuf[Combo].type].name,16);
                
                if(strlen(combo_class_buf[combobuf[Combo].type].name) > 16)
                {
                    buf[15]='.';
                    buf[14]='.';
                    offs = 5;
                }
                
                buf[16]='\0';
                //if (combobuf[Combo].type != 0)
                textprintf_ex(menu1,pfont, cprect.x-text_length(pfont,buf)-offs, cprect.y+20,jwin_pal[jcBOXFG],jwin_pal[jcBOX],"%s",buf);
            }
            
            // Cycle
            int NextCombo=combobuf[Combo].nextcombo;
            int NextCSet=combobuf[Combo].nextcset;
            jwin_draw_frame(menu1, cprect.x+int(cprect.w*1.5)-2, cprect.y-2, cprect.w+4, cprect.h+4, FR_DEEP);
            
            if(NextCombo>0 && draw_mode != dm_alias)
            {
                put_combo(cycle_preview_bmp,0,0,NextCombo,NextCSet,Flags&(cFLAGS|cWALK),0);
                
                if(Flags&cWALK) put_walkflags(cycle_preview_bmp,0,0,NextCombo,0);
                
                if(Flags&cFLAGS) put_flags(cycle_preview_bmp,0,0,NextCombo,0,cFLAGS,0);
                
                stretch_blit(cycle_preview_bmp, menu1, 0, 0, 16, 16, cprect.x+int(cprect.w*1.5), cprect.y, cprect.w, cprect.h);
            }
            else
            {
                if(InvalidStatic)
                {
                    for(int dy=0; dy<32; dy++)
                    {
                        for(int dx=0; dx<32; dx++)
                        {
                            menu1->line[dy+ cprect.y][dx+ cprect.x+int(cprect.w*1.5)]=vc((((rand()%100)/50)?0:8)+(((rand()%100)/50)?0:7));
                        }
                    }
                }
                else
                {
                    rectfill(menu1, cprect.x+int(cprect.w*1.5), cprect.y, cprect.x+int(cprect.w*2.5), cprect.y+ cprect.h,vc(0));
                    safe_rect(menu1, cprect.x+int(cprect.w*1.5), cprect.y, cprect.x+int(cprect.w*2.5), cprect.y+ cprect.h,vc(15));
                    line(menu1, cprect.x+int(cprect.w*1.5), cprect.y, cprect.x+int(cprect.w*2.5), cprect.y+ cprect.h,vc(15));
                    line(menu1, cprect.x+int(cprect.w*1.5), cprect.y+ cprect.h, cprect.x+int(cprect.w*2.5), cprect.y,vc(15));
                }
            }
            
            if(draw_mode!=dm_alias)
            {
            
                textprintf_ex(menu1,pfont, cprect.x+int(cprect.w*2.5)+6, cprect.y+2,jwin_pal[jcBOXFG],jwin_pal[jcBOX],"Cycle: %d",NextCombo);
                textprintf_ex(menu1,pfont, cprect.x+int(cprect.w*2.5)+6, cprect.y+11,jwin_pal[jcBOXFG],jwin_pal[jcBOX],"CSet: %d",NextCSet);
                char buf[17];
                int offs = 8;
                strncpy(buf,combo_class_buf[combobuf[NextCombo].type].name,16);
                
                if(strlen(combo_class_buf[combobuf[NextCombo].type].name) > 15)
                {
                    buf[15]='.';
                    buf[14]='.';
                    offs = 5;
                }
                
                buf[16]='\0';
                textprintf_ex(menu1,pfont, cprect.x+int(cprect.w*2.5)+6, cprect.y+20,jwin_pal[jcBOXFG],jwin_pal[jcBOX],"%s",buf);
            }
            
        }
        else
        {
            put_combo(menu1, combo_preview().x, combo_preview().y,(draw_mode==dm_alias)?combo_aliases[combo_apos].combos[0]:Combo,(draw_mode==dm_alias)?wrap(combo_aliases[combo_apos].csets[0]+alias_cset_mod, 0, 11):CSet,Flags&(cFLAGS|cWALK),0);
        }
    }
    
    if(flags&rMENU)
    {
        drawpanel(is_large()?-1:menutype);
        set_clip_rect(menu1,0,0,Backend::graphics->virtualScreenW()-1,Backend::graphics->virtualScreenH()-1);
    }
    
    if(flags&rFAVORITES)
    {
        if(is_large())
        {
            jwin_draw_frame(menu1,favorites_window().x,favorites_window().y,favorites_window().w,favorites_window().h, FR_WIN);
            rectfill(menu1,favorites_window().x+2,favorites_window().y+2,favorites_window().x+favorites_window().w-3,favorites_window().y+favorites_window().h-3,jwin_pal[jcBOX]);
            jwin_draw_frame(menu1,favorites_list().x-2,favorites_list().y-2,(favorites_list().w<<4)+4,(favorites_list().h<<4)+4, FR_DEEP);
            rectfill(menu1,favorites_list().x,favorites_list().y,favorites_list().x+(favorites_list().w<<4)-1,favorites_list().y+(favorites_list().h<<4)-1,jwin_pal[jcBOXFG]);
            textprintf_ex(menu1,font,favorites_list().x-2,favorites_list().y-11,jwin_pal[jcBOXFG],-1,"Favorite Combos");
            
            if(draw_mode!=dm_alias)
            {
                for(int i=0; i<(favorites_list().w*favorites_list().h); i++)
                {
                    if(favorite_combos[i]==-1)
                    {
                        if(InvalidStatic)
                        {
                            for(int dy=0; dy<16; dy++)
                            {
                                for(int dx=0; dx<16; dx++)
                                {
                                    menu1->line[(i/favorites_list().w)*16+favorites_list().y+dy][(i%favorites_list().w)*16+favorites_list().x+dx]=vc((((rand()%100)/50)?0:8)+(((rand()%100)/50)?0:7));
                                }
                            }
                        }
                        else
                        {
                            rectfill(menu1, (i%favorites_list().w)*16+favorites_list().x, (i/favorites_list().w)*16+favorites_list().y, (i%favorites_list().w)*16+favorites_list().x+15, (i/favorites_list().w)*16+favorites_list().y+15, vc(0));
                            safe_rect(menu1, (i%favorites_list().w)*16+favorites_list().x, (i/favorites_list().w)*16+favorites_list().y, (i%favorites_list().w)*16+favorites_list().x+15, (i/favorites_list().w)*16+favorites_list().y+15, vc(15));
                            line(menu1, (i%favorites_list().w)*16+favorites_list().x, (i/favorites_list().w)*16+favorites_list().y, (i%favorites_list().w)*16+favorites_list().x+15, (i/favorites_list().w)*16+favorites_list().y+15, vc(15));
                            line(menu1, (i%favorites_list().w)*16+favorites_list().x, (i/favorites_list().w)*16+favorites_list().y+15, (i%favorites_list().w)*16+favorites_list().x+15, (i/favorites_list().w)*16+favorites_list().y, vc(15));
                        }
                    }
                    else
                    {
                        put_combo(menu1,(i%favorites_list().w)*16+favorites_list().x,(i/favorites_list().w)*16+favorites_list().y,favorite_combos[i],CSet,Flags&(cFLAGS|cWALK),0);
                    }
                }
            }
            else
            {
                for(int i=0; i<(favorites_list().w*favorites_list().h); i++)
                {
                    if(favorite_comboaliases[i]==-1)
                    {
                        if(InvalidStatic)
                        {
                            for(int dy=0; dy<16; dy++)
                            {
                                for(int dx=0; dx<16; dx++)
                                {
                                    menu1->line[(i/favorites_list().w)*16+favorites_list().y+dy][(i%favorites_list().w)*16+favorites_list().x+dx]=vc((((rand()%100)/50)?0:8)+(((rand()%100)/50)?0:7));
                                }
                            }
                        }
                        else
                        {
                            rectfill(menu1, (i%favorites_list().w)*16+favorites_list().x, (i/favorites_list().w)*16+favorites_list().y, (i%favorites_list().w)*16+favorites_list().x+15, (i/favorites_list().w)*16+favorites_list().y+15, vc(0));
                            safe_rect(menu1, (i%favorites_list().w)*16+favorites_list().x, (i/favorites_list().w)*16+favorites_list().y, (i%favorites_list().w)*16+favorites_list().x+15, (i/favorites_list().w)*16+favorites_list().y+15, vc(15));
                            line(menu1, (i%favorites_list().w)*16+favorites_list().x, (i/favorites_list().w)*16+favorites_list().y, (i%favorites_list().w)*16+favorites_list().x+15, (i/favorites_list().w)*16+favorites_list().y+15, vc(15));
                            line(menu1, (i%favorites_list().w)*16+favorites_list().x, (i/favorites_list().w)*16+favorites_list().y+15, (i%favorites_list().w)*16+favorites_list().x+15, (i/favorites_list().w)*16+favorites_list().y, vc(15));
                        }
                    }
                    else
                    {
                        draw_combo_alias_thumbnail(menu1, &combo_aliases[favorite_comboaliases[i]], (i%favorites_list().w)*16+favorites_list().x,(i/favorites_list().w)*16+favorites_list().y,1);
                    }
                }
            }
        }
    }
    
    if(flags&rCOMMANDS)
    {
        if(is_large())
        {
            jwin_draw_frame(menu1,commands_window().x,commands_window().y,commands_window().w,commands_window().h, FR_WIN);
            rectfill(menu1,commands_window().x+2,commands_window().y+2,commands_window().x+commands_window().w-3,commands_window().y+commands_window().h-3,jwin_pal[jcBOX]);
            jwin_draw_frame(menu1,commands_list().x-2,commands_list().y-2,(commands_list().w*command_buttonwidth)+4,(commands_list().h*command_buttonheight)+4, FR_DEEP);
            rectfill(menu1,commands_list().x,commands_list().y,commands_list().x+(commands_list().w*command_buttonwidth)-1,commands_list().y+(commands_list().h*command_buttonheight)-1,jwin_pal[jcBOXFG]);
            textprintf_ex(menu1,font,commands_list().x-2,commands_list().y-14,jwin_pal[jcBOXFG],-1,"Favorite Commands");
            FONT *tfont=font;
            font=pfont;
            
            for(int cmd=0; cmd<(commands_list().w*commands_list().h); ++cmd)
            {
                draw_text_button(menu1,
                                 (cmd%commands_list().w)*command_buttonwidth+commands_list().x,
                                 (cmd/commands_list().w)*command_buttonheight+commands_list().y,
                                 command_buttonwidth,
                                 command_buttonheight,
                                 (favorite_commands[cmd]==cmdCatchall&&strcmp(catchall_string[Map.CurrScr()->room]," "))?catchall_string[Map.CurrScr()->room]:commands[favorite_commands[cmd]].name,
                                 vc(1),
                                 vc(14),
                                 commands[favorite_commands[cmd]].flags,
                                 true);
            }
            
            font=tfont;
        }
    }
    
    if(is_large()) // Layer panels
    {
        jwin_draw_frame(menu1,layer_panel().x-2,layer_panel().y,layer_panel().w+2,layer_panel().h,FR_DEEP);
        rectfill(menu1,layer_panel().x+2,layer_panel().y+2,layer_panel().x+layer_panel().w-3,layer_panel().y+layer_panel().h-3,jwin_pal[jcBOX]);
        
        bool groundlayers = false;
        bool overheadlayers = false;
        bool flyinglayers = false;
        
        for(int i=0; i<=6; ++i)
        {
            char tbuf[15];
            
            if(i>0 && Map.CurrScr()->layermap[i-1])
            {
                if(i<3) groundlayers = true;
                else if(i<5) overheadlayers = true;
                else if(i<7) flyinglayers = true;
                
                sprintf(tbuf, "%s%d (%d:%02X)", (i==2 && Map.CurrScr()->flags7&fLAYER2BG) || (i==3 && Map.CurrScr()->flags7&fLAYER3BG) ? "-":"", i, Map.CurrScr()->layermap[i-1], Map.CurrScr()->layerscreen[i-1]);
            }
            else
            {
                sprintf(tbuf, "%s%d", (i==2 && Map.CurrScr()->flags7&fLAYER2BG) || (i==3 && Map.CurrScr()->flags7&fLAYER3BG) ? "-":"", i);
            }
            
            int rx = (i * (layerpanel_buttonwidth+23)) + layer_panel().x+6;
            int ry = layer_panel().y+16;
            draw_text_button(menu1, rx,ry, layerpanel_buttonwidth, layerpanel_buttonheight, tbuf,vc(1),vc(14), CurrentLayer==i? D_SELECTED : (!Map.CurrScr()->layermap[i-1] && i>0) ? D_DISABLED : 0,true);
            draw_checkbox(menu1,rx+layerpanel_buttonwidth+4,ry+2,vc(1),vc(14), LayerMaskInt[i]!=0);
            
            // Draw the group divider
            if(i==3 || i==5)
            {
                _allegro_vline(menu1, rx-4, layer_panel().y+3, layer_panel().y+36, jwin_pal[jcLIGHT]);
                _allegro_vline(menu1, rx-5, layer_panel().y+3, layer_panel().y+36, jwin_pal[jcMEDDARK]);
            }
        }
        
        if(groundlayers)
            textprintf_ex(menu1,font,layer_panel().x+60,layer_panel().y+4,jwin_pal[jcBOXFG],-1,"Ground (Walkable) Layers");
        else
            textprintf_disabled(menu1,font,layer_panel().x+60,layer_panel().y+4,jwin_pal[jcLIGHT],jwin_pal[jcMEDDARK],"Ground (Walkable) Layers");
            
        if(overheadlayers)
            textprintf_ex(menu1,font,layer_panel().x+268,layer_panel().y+4,jwin_pal[jcBOXFG],-1,"Overhead Layers (Ground)");
        else
            textprintf_disabled(menu1,font,layer_panel().x+268,layer_panel().y+4,jwin_pal[jcLIGHT],jwin_pal[jcMEDDARK],"Overhead Layers (Ground)");
            
        if(flyinglayers)
            textprintf_ex(menu1,font,layer_panel().x+434,layer_panel().y+4,jwin_pal[jcBOXFG],-1,"Overhead Layers (Flying)");
        else
            textprintf_disabled(menu1,font,layer_panel().x+434,layer_panel().y+4,jwin_pal[jcLIGHT],jwin_pal[jcMEDDARK],"Overhead Layers (Flying)");
            
        //font=tfont;
    }
    
    //} //if(true)
    if(zq_showpal)
    {
        for(int i=0; i<256; i++)
        {
            rectfill(menu1,((i&15)<<2)+256,((i>>4)<<2)+176,((i&15)<<2)+259,((i>>4)<<2)+179,i);
        }
    }
    
    if(ShowFPS)
    {
        textprintf_shadowed_ex(menu1,is_large()?lfont:sfont,0,prv_mode?32:16,vc(15),vc(0),-1,"FPS:%-3d",Backend::graphics->getLastFPS());
    }
    
    if(prv_mode)
    {
        textout_shadowed_ex(menu1,sfont,"Preview Mode",0,16,vc(15),vc(0),-1);
        
        if(prv_twon)
        {
            textprintf_shadowed_ex(menu1,sfont,0,24,vc(15),vc(0),-1,"T Warp=%d tics", Map.get_prvtime());
        }
        
        do_previewtext();
        
    }
    
    if(ShowFFScripts && !prv_mode)
    {
        int ypos = ShowFPS ? 28 : 18;
        
        for(int i=0; i< MAXFFCS; i++)
            if(Map.CurrScr()->ffscript[i] && Map.CurrScr()->ffdata[i])
            {
                textout_shadowed_ex(menu1,is_large() ? lfont_l : font, ffcmap[Map.CurrScr()->ffscript[i]-1].second.c_str(),2,ypos,vc(showxypos_ffc==i ? 14 : 15),vc(0),-1);
                ypos+=16;
            }
    }
    
    // Show Errors & Details
    //This includes the presence of: Screen State Carryover, Timed Warp, Maze Path, the 'Sideview Gravity', 'Invisible Link',
    //'Save Screen', 'Continue Here' and 'Treat As..' Screen Flags,
    // the String, every Room Type and Catch All, and all four Tile and Side Warps.
    if(is_large() && !prv_mode && ShowInfo)
    {
        int i=0;
        char buf[2048];
        
        // Start with general information
        if(Map.CurrScr()->flags3&fINVISLINK)
        {
            sprintf(buf,"Invisible Link");
            show_screen_error(buf,i++,vc(15));
        }
        
        if(Map.getLayerTargetMap() > 0)
        {
            int m = Map.getLayerTargetMultiple();
            sprintf(buf,"Used as a layer by screen %d:%02X",Map.getLayerTargetMap(),Map.getLayerTargetScr());
            char buf2[16];
            
            if(m>0)
            {
                sprintf(buf2," and %d other%s",m,m>1?"s":"");
                strcat(buf,buf2);
            }
            
            show_screen_error(buf,i++,vc(15));
        }
        
        if(Map.CurrScr()->nextmap)
        {
            sprintf(buf,"Screen State carries over to %d:%02X",Map.CurrScr()->nextmap,Map.CurrScr()->nextscr);
            show_screen_error(buf,i++,vc(15));
        }
        
        if(Map.CurrScr()->timedwarptics)
        {
            sprintf(buf,"%s%sTimed Warp: %s",(Map.CurrScr()->flags4&fTIMEDDIRECT)?"Direct ":"",(Map.CurrScr()->flags5&fRANDOMTIMEDWARP)?"Random ":"",ticksstr(Map.CurrScr()->timedwarptics));
            show_screen_error(buf,i++,vc(15));
        }
        
        if(Map.CurrScr()->flags&fMAZE)
        {
            sprintf(buf,"Maze Path: %s (Exit %s)",pathstr(Map.CurrScr()->path),dirstr[Map.CurrScr()->exitdir]);
            show_screen_error(buf,i++,vc(15));
        }
        
        bool continuescreen = false, savecombo = false;
        
        if(Map.CurrScr()->flags4&fAUTOSAVE)
        {
            sprintf(buf,"Automatic Save%s Screen", (Map.CurrScr()->flags6&fCONTINUEHERE) ? "-Continue":"");
            show_screen_error(buf,i++,vc(15));
            continuescreen = ((Map.CurrScr()->flags6&fCONTINUEHERE)!=0);
            savecombo = true;
        }
        else if(Map.CurrScr()->flags6&fCONTINUEHERE)
        {
            sprintf(buf,"Continue Screen");
            show_screen_error(buf,i++,vc(15));
            continuescreen = true;
        }
        
        if(Map.CurrScr()->flags7&fSIDEVIEW)
        {
            sprintf(buf,"Sideview Gravity");
            show_screen_error(buf,i++,vc(15));
        }
        
        if(Map.CurrScr()->flags6 & (fCAVEROOM|fDUNGEONROOM))
        {
            sprintf(buf,"Treat As %s%s Screen", (Map.CurrScr()->flags6&fCAVEROOM) ? "Interior":"NES Dungeon",
                    (Map.CurrScr()->flags6 & (fCAVEROOM|fDUNGEONROOM)) == (fCAVEROOM|fDUNGEONROOM) ? " or NES Dungeon":"");
            show_screen_error(buf,i++,vc(15));
        }
        
        if(Map.CurrScr()->oceansfx != 0)
        {
            const SFXSample *oceansample = Backend::sfx->getSample(Map.CurrScr()->oceansfx);
            const char *sndname = oceansample == NULL ? "(None)" : oceansample->name.c_str();
            sprintf(buf,"Ambient Sound: %s", sndname);
            show_screen_error(buf,i++,vc(15));
        }
        
        if(Map.CurrScr()->bosssfx != 0)
        {
            const SFXSample *bosssample = Backend::sfx->getSample(Map.CurrScr()->bosssfx);
            const char *sndname = bosssample == NULL ? "(None)" : bosssample->name.c_str();
            sprintf(buf,"Boss Roar Sound: %s", sndname);
            show_screen_error(buf,i++,vc(15));
        }
        
        if(Map.CurrScr()->str)
        {
            strncpy(buf,MsgString(Map.CurrScr()->str, true, false),72);
            buf[72] = '\0';
            char shortbuf[72];
            strip_extra_spaces(buf);
            shorten_string(shortbuf, buf, lfont_l, 72, 280);
            sprintf(buf,"String %s",shortbuf);
            show_screen_error(buf,i++,vc(15));
        }
        
        if((Map.CurrScr()->flags&fWHISTLE) || (Map.CurrScr()->flags7&fWHISTLEWATER))
        {
            sprintf(buf,"Whistle ->%s%s%s",(Map.CurrScr()->flags&fWHISTLE)?" Stairs":"",
                    (Map.CurrScr()->flags&fWHISTLE && Map.CurrScr()->flags7&fWHISTLEWATER)?", ":"",
                    (Map.CurrScr()->flags7&fWHISTLEWATER)?"Dry Lake":"");
            show_screen_error(buf,i++,vc(15));
        }
        
        switch(Map.CurrScr()->room)
        {
        case rSP_ITEM:
            sprintf(buf,"Special Item is %s",item_string[Map.CurrScr()->catchall]);
            show_screen_error(buf,i++, vc(15));
            break;
            
        case rINFO:
        {
            int shop = Map.CurrScr()->catchall;
            sprintf(buf,"Pay For Info: -%d, -%d, -%d",
                    misc.info[shop].price[0],misc.info[shop].price[1],misc.info[shop].price[2]);
            show_screen_error(buf,i++, vc(15));
        }
        break;
        
        case rMONEY:
            sprintf(buf,"Secret Money: %d Rupees",Map.CurrScr()->catchall);
            show_screen_error(buf,i++, vc(15));
            break;
            
        case rGAMBLE:
            show_screen_error("Gamble Room",i++, vc(15));
            break;
            
        case rREPAIR:
            sprintf(buf,"Door Repair: -%d Rupees",Map.CurrScr()->catchall);
            show_screen_error(buf,i++, vc(15));
            break;
            
        case rRP_HC:
            sprintf(buf,"Take %s or %s", item_string[iRPotion], item_string[iHeartC]);
            show_screen_error(buf,i++, vc(15));
            break;
            
        case rGRUMBLE:
            show_screen_error("Feed the Goriya",i++, vc(15));
            break;
            
        case rTRIFORCE:
            show_screen_error("Level 9 Entrance",i++, vc(15));
            break;
            
        case rP_SHOP:
        case rSHOP:
        {
            int shop = Map.CurrScr()->catchall;
            sprintf(buf,"%sShop: ",
                    Map.CurrScr()->room==rP_SHOP ? "Potion ":"");
                    
            for(int j=0; j<3; j++) if(misc.shop[shop].item[j]>0)  // Print the 3 items and prices
                {
                    strcat(buf,item_string[misc.shop[shop].item[j]]);
                    strcat(buf,":");
                    char pricebuf[4];
                    sprintf(pricebuf,"%d",misc.shop[shop].price[j]);
                    strcat(buf,pricebuf);
                    
                    if(j<2 && misc.shop[shop].item[j+1]>0) strcat(buf,", ");
                }
                
            show_screen_error(buf,i++, vc(15));
        }
        break;
        
        case rTAKEONE:
        {
            int shop = Map.CurrScr()->catchall;
            sprintf(buf,"Take Only One: %s%s%s%s%s",
                    misc.shop[shop].item[0]<1?"":item_string[misc.shop[shop].item[0]],misc.shop[shop].item[0]>0?", ":"",
                    misc.shop[shop].item[1]<1?"":item_string[misc.shop[shop].item[1]],(misc.shop[shop].item[1]>0&&misc.shop[shop].item[2]>0)?", ":"",
                    misc.shop[shop].item[2]<1?"":item_string[misc.shop[shop].item[2]]);
            show_screen_error(buf,i++, vc(15));
        }
        break;
        
        case rBOMBS:
            sprintf(buf,"More Bombs: -%d Rupees",Map.CurrScr()->catchall);
            show_screen_error(buf,i++, vc(15));
            break;
            
        case rARROWS:
            sprintf(buf,"More Arrows: -%d Rupees",Map.CurrScr()->catchall);
            show_screen_error(buf,i++, vc(15));
            break;
            
        case rSWINDLE:
            sprintf(buf,"Leave Life or %d Rupees",Map.CurrScr()->catchall);
            show_screen_error(buf,i++, vc(15));
            break;
            
        case r10RUPIES:
            show_screen_error("10 Rupees",i++, vc(15));
            break;
            
        case rGANON:
            show_screen_error("Ganon Room",i++, vc(15));
            break;
            
        case rZELDA:
            show_screen_error("Zelda Room",i++, vc(15));
            break;
            
        case rMUPGRADE:
            show_screen_error("1/2 Magic Upgrade",i++, vc(15));
            break;
            
        case rLEARNSLASH:
            show_screen_error("Learn Slash",i++, vc(15));
            break;
            
        case rWARP:
            sprintf(buf,"3-Stair Warp: Warp Ring %d",Map.CurrScr()->catchall);
            show_screen_error(buf,i++, vc(15));
            break;
        }
        
        bool undercombo = false, warpa = false, warpb = false, warpc = false, warpd = false, warpr = false;
        
        for(int c=0; c<176+128+1+MAXFFCS; ++c)
        {
            // Checks both combos, secret combos, undercombos and FFCs
//Fixme:
            int ctype =
                combobuf[vbound(
                             (c>=305 ? Map.CurrScr()->ffdata[c-305] :
                              c>=304 ? Map.CurrScr()->undercombo :
                              c>=176 ? Map.CurrScr()->secretcombo[c-176] :
                              Map.CurrScr()->data.empty() ? 0 : // Sanity check: does room combo data exist?
                              Map.CurrScr()->data[c]
                             ), 0, MAXCOMBOS-1)].type;
                             
            if(!undercombo && integrityBoolUnderCombo(Map.CurrScr(),ctype))
            {
                undercombo = true;
                show_screen_error("Under Combo is combo 0",i++, vc(7));
            }
            
            // Tile Warp types
            switch(ctype)
            {
            case cSAVE:
            case cSAVE2:
                if(!savecombo)
                {
                    savecombo = true;
                    
                    if(integrityBoolSaveCombo(Map.CurrScr(),ctype))
                        show_screen_error("Save Screen",i++, vc(15));
                    else
                        show_screen_error("Save-Continue Screen",i++, vc(15));
                }
                
                break;
                
            case cSTAIRR:
            case cPITR:
            case cSWARPR:
                if(!warpr && (Map.CurrScr()->tilewarptype[0]==wtCAVE || Map.CurrScr()->tilewarptype[1]==wtCAVE ||
                              Map.CurrScr()->tilewarptype[2]==wtCAVE || Map.CurrScr()->tilewarptype[3]==wtCAVE))
                {
                    warpr = true;
                    show_screen_error("Random Tile Warp contains Cave/Item Cellar",i++, vc(7));
                }
                
                break;
                
            case cCAVED:
            case cPITD:
            case cSTAIRD:
            case cCAVE2D:
            case cSWIMWARPD:
            case cDIVEWARPD:
            case cSWARPD:
                if(!warpd)
                {
                    warpd = true;
                    tile_warp_notification(3,buf);
                    show_screen_error(buf,i++, vc(15));
                }
                
                break;
                
            case cCAVEC:
            case cPITC:
            case cSTAIRC:
            case cCAVE2C:
            case cSWIMWARPC:
            case cDIVEWARPC:
            case cSWARPC:
                if(!warpc)
                {
                    warpc = true;
                    tile_warp_notification(2,buf);
                    show_screen_error(buf,i++, vc(15));
                }
                
                break;
                
            case cCAVEB:
            case cPITB:
            case cSTAIRB:
            case cCAVE2B:
            case cSWIMWARPB:
            case cDIVEWARPB:
            case cSWARPB:
                if(!warpb)
                {
                    warpb = true;
                    tile_warp_notification(1,buf);
                    show_screen_error(buf,i++, vc(15));
                }
                
                break;
                
            case cCAVE:
            case cPIT:
            case cSTAIR:
            case cCAVE2:
            case cSWIMWARP:
            case cDIVEWARP:
            case cSWARPA:
                if(!warpa)
                {
                    warpa = true;
                    tile_warp_notification(0,buf);
                    show_screen_error(buf,i++, vc(15));
                }
                
                break;
            }
        }
        
        int sidewarpnotify = 0;
        
        if(Map.CurrScr()->flags2&wfUP)
        {
            side_warp_notification(Map.CurrScr()->sidewarpindex&3,0,buf);
            show_screen_error(buf,i++, vc(15));
            sidewarpnotify|=(1<<(Map.CurrScr()->sidewarpindex&3));
        }
        
        if(Map.CurrScr()->flags2&wfDOWN)
        {
            side_warp_notification((Map.CurrScr()->sidewarpindex>>2)&3,1,buf);
            show_screen_error(buf,i++, vc(15));
            sidewarpnotify|=(1<<((Map.CurrScr()->sidewarpindex>>2)&3));
        }
        
        if(Map.CurrScr()->flags2&wfLEFT)
        {
            side_warp_notification((Map.CurrScr()->sidewarpindex>>4)&3,2,buf);
            show_screen_error(buf,i++, vc(15));
            sidewarpnotify|=(1<<((Map.CurrScr()->sidewarpindex>>4)&3));
        }
        
        if(Map.CurrScr()->flags2&wfRIGHT)
        {
            side_warp_notification((Map.CurrScr()->sidewarpindex>>6)&3,3,buf);
            show_screen_error(buf,i++, vc(15));
            sidewarpnotify|=(1<<((Map.CurrScr()->sidewarpindex>>6)&3));
        }
        
        if(!(sidewarpnotify&1) && Map.CurrScr()->timedwarptics)
        {
            side_warp_notification(0,4,buf); // Timed Warp
            show_screen_error(buf,i++, vc(15));
        }
        
        // Now for errors
        if((Map.CurrScr()->flags4&fSAVEROOM) && !savecombo) show_screen_error("Save Point->Continue Here, but no Save Point combo?",i++, vc(14));
        
        if(integrityBoolEnemiesItem(Map.CurrScr())) show_screen_error("Enemies->Item, but no enemies",i++, vc(7));
        
        if(integrityBoolEnemiesSecret(Map.CurrScr())) show_screen_error("Enemies->Secret, but no enemies",i++, vc(7));
        
        if(integrityBoolStringNoGuy(Map.CurrScr())) show_screen_error("String, but Guy is (none)",i++, vc(14));
        
        if(integrityBoolGuyNoString(Map.CurrScr())) show_screen_error("Non-Fairy Guy, but String is (none)",i++, vc(14));
        
        if(integrityBoolRoomNoGuy(Map.CurrScr())) show_screen_error("Guy is (none)",i++, vc(14));
        
        if(integrityBoolRoomNoString(Map.CurrScr())) show_screen_error("String is (none)",i++, vc(14));
        
        if(integrityBoolRoomNoGuyNoString(Map.CurrScr())) show_screen_error("Guy and String are (none)",i++, vc(14));
    }
    
    if(!is_large())
    {
        if(draw_mode!=dm_normal)
        {
            textout_shadowed_right_ex(menu1,sfont,dm_names[draw_mode],mapscreen_x()+((16+(showedges()?1:0))*16*mapscreensize())-1,mapscreen_y()+((showedges()?1:0)*16*mapscreensize()),vc(15),vc(0),-1);
        }
    }
    
    if((tooltip_timer>=tooltip_maxtimer)&&(tooltip_box.x>=0&&tooltip_box.y>=0))
    {
        masked_blit(tooltipbmp, menu1, 0, 0, tooltip_box.x, tooltip_box.y, tooltip_box.w, tooltip_box.h);
    }
    
//  textprintf_ex(menu1,font,16, 200,vc(15),-1,"%d %d %d %d %d",tooltip_timer,tooltip_box.x,tooltip_box.y,tooltip_box.w,tooltip_box.h);

   
    if(flags&rCLEAR)
    {
        blit(menu1,screen,0,0,0,0,Backend::graphics->virtualScreenW(),Backend::graphics->virtualScreenH());
    }
    else
    {
        blit(menu1,screen,0,16,0,16,Backend::graphics->virtualScreenW(),Backend::graphics->virtualScreenH()-16);
        blit(menu1,screen,combolist_window().x-64,0,combolist_window().x-64,0,combolist_window().w+64,16);
        
        if(flags&rCOMBO)
        {
            blit(menu1,screen,combo_preview().x,combo_preview().y,combo_preview().x,combo_preview().y,combo_preview().w,combo_preview().h);
        }
    }
    
    ComboBrushPause=0;
    
    SCRFIX();
	Backend::graphics->waitTick();
	Backend::graphics->showBackBuffer();
}

void select_scr()
{
    if(Map.getCurrMap()>=Map.getMapCount())
        return;
        
    int tempcb=ComboBrush;
    ComboBrush=0;
    
    //scooby
    while(Backend::mouse->anyButtonClicked())
    {   
        int x= Backend::mouse->getVirtualScreenX();
        int y= Backend::mouse->getVirtualScreenY();
        int s=(vbound(((y-(minimap().y+9+3))/(3*BMM())),0,8)  <<4)+ vbound(((x-(minimap().x+3))/(3*BMM())),0,15);
        
        if(s>=MAPSCRS)
            s-=16;
            
        if(s!=Map.getCurrScr())
        {
            Map.setCurrScr(s);
            //      vsync();
            //      refresh(rALL);
        }
        
        do_animations();
        refresh(rALL);
    }
    
    ComboBrush=tempcb;
}

bool select_favorite()
{
    int tempcb=ComboBrush;
    ComboBrush=0;
    bool valid=false;
    
    while(Backend::mouse->anyButtonClicked())
    {
        valid=false;
        int x= Backend::mouse->getVirtualScreenX();
        
        if(x<favorites_list().x) x=favorites_list().x;
        
        if(x>favorites_list().x+(favorites_list().w*16)-1) x=favorites_list().x+(favorites_list().w*16)-1;
        
        int y= Backend::mouse->getVirtualScreenY();
        
        if(y<favorites_list().y) y=favorites_list().y;
        
        if(y>favorites_list().y+(favorites_list().h*16)-1) y=favorites_list().y+(favorites_list().h*16)-1;
        
        int tempc=(((y-favorites_list().y)>>4)*favorites_list().w)+((x-favorites_list().x)>>4);
        
        if(draw_mode!=dm_alias)
        {
            if(favorite_combos[tempc]!=-1)
            {
                Combo=favorite_combos[tempc];
                valid=true;
            }
        }
        else
        {
            if(favorite_comboaliases[tempc]!=-1)
            {
                combo_apos=favorite_comboaliases[tempc];
                valid=true;
            }
        }
        
        do_animations();
        refresh(rALL);
    }
    
    ComboBrush=tempcb;
    return valid;
}

void select_combo(int clist)
{
    current_combolist=clist;
    int tempcb=ComboBrush;
    ComboBrush=0;
    
    while(Backend::mouse->anyButtonClicked())
    {
		int x = Backend::mouse->getVirtualScreenX();
        
        if(x<combolist(current_combolist).x) x=combolist(current_combolist).x;
        
        if(x>combolist(current_combolist).x+(combolist(current_combolist).w*16)-1) x=combolist(current_combolist).x+(combolist(current_combolist).w*16)-1;
        
        int y= Backend::mouse->getVirtualScreenY();
        
        if(y<combolist(current_combolist).y) y=combolist(current_combolist).y;
        
        if(y>combolist(current_combolist).y+(combolist(current_combolist).h*16)-1) y=combolist(current_combolist).y+(combolist(current_combolist).h*16)-1;
        
        Combo=(((y-combolist(current_combolist).y)>>4)*combolist(current_combolist).w)+((x-combolist(current_combolist).x)>>4)+First[current_combolist];
        do_animations();
        refresh(rALL);
    }
    
    ComboBrush=tempcb;
}

void select_comboa(int clist)
{
    current_comboalist=clist;
    int tempcb=ComboBrush;
    ComboBrush=0;
    alias_cset_mod=0;
    
    while(Backend::mouse->anyButtonClicked())
    {
        int x= Backend::mouse->getVirtualScreenX();
        
        if(x<comboaliaslist(current_comboalist).x) x=comboaliaslist(current_comboalist).x;
        
        if(x>comboaliaslist(current_comboalist).x+(comboaliaslist(current_comboalist).w*16)-1) x=comboaliaslist(current_comboalist).x+(comboaliaslist(current_comboalist).w*16)-1;
        
        int y= Backend::mouse->getVirtualScreenY();
        
        if(y<comboaliaslist(current_comboalist).y) y=comboaliaslist(current_comboalist).y;
        
        if(y>comboaliaslist(current_comboalist).y+(comboaliaslist(current_comboalist).h*16)-1) y=comboaliaslist(current_comboalist).y+(comboaliaslist(current_comboalist).h*16)-1;
        
        combo_apos=(((y-comboaliaslist(current_comboalist).y)>>4)*comboaliaslist(current_comboalist).w)+((x-comboaliaslist(current_comboalist).x)>>4)+combo_alistpos[current_comboalist];
        do_animations();
        refresh(rALL);
    }
    
    ComboBrush=tempcb;
}

void update_combobrush()
{
    clear_bitmap(brushbmp);
    
    if(draw_mode==dm_alias)
    {
        //int count=(combo_aliases[combo_apos].width+1)*(combo_aliases[combo_apos].height+1)*(comboa_lmasktotal(combo_aliases[combo_apos].layermask));
        for(int z=0; z<=comboa_lmasktotal(combo_aliases[combo_apos].layermask); z++)
        {
            for(int y=0; y<=combo_aliases[combo_apos].height; y++)
            {
                for(int x=0; x<=combo_aliases[combo_apos].width; x++)
                {
                    int position = ((y*(combo_aliases[combo_apos].width+1))+x)+((combo_aliases[combo_apos].width+1)*(combo_aliases[combo_apos].height+1)*z);
                    
                    if(combo_aliases[combo_apos].combos[position])
                    {
                        if(z==0)
                        {
                            putcombo(brushbmp,x<<4,y<<4,combo_aliases[combo_apos].combos[position],wrap(combo_aliases[combo_apos].csets[position]+alias_cset_mod, 0, 11));
                        }
                        else
                        {
                            overcombo(brushbmp,x<<4,y<<4,combo_aliases[combo_apos].combos[position],wrap(combo_aliases[combo_apos].csets[position]+alias_cset_mod, 0, 11));
                        }
                    }
                }
            }
        }
        
        switch(alias_origin)
        {
        case 0:
            //if(!(combo_aliases[combo_apos].combos[0]))
            textprintf_shadowed_ex(brushbmp, sfont, 6, 6, vc(15), vc(0), -1, "x");
            break;
            
        case 1:
            //if(!(combo_aliases[combo_apos].combos[combo_aliases[combo_apos].width]))
            textprintf_shadowed_ex(brushbmp, sfont, 6+(combo_aliases[combo_apos].width*16), 6, vc(15), vc(0), -1, "x");
            break;
            
        case 2:
            //if(!(combo_aliases[combo_apos].combos[(combo_aliases[combo_apos].width+1)*combo_aliases[combo_apos].height]))
            textprintf_shadowed_ex(brushbmp, sfont, 6, 6+(combo_aliases[combo_apos].height*16), vc(15), vc(0), -1, "x");
            break;
            
        case 3:
            //if(!(combo_aliases[combo_apos].combos[(combo_aliases[combo_apos].width+1)*(combo_aliases[combo_apos].height)-1]))
            textprintf_shadowed_ex(brushbmp, sfont, 6+(combo_aliases[combo_apos].width*16), 6+(combo_aliases[combo_apos].height*16), vc(15), vc(0), -1, "x");
            break;
        }
    }
    else
    {
        if(combo_cols==false)
        {
            for(int i=0; i<256; i++)
            {
                if(((i%COMBOS_PER_ROW)<BrushWidth)&&((i/COMBOS_PER_ROW)<BrushHeight))
                {
                    put_combo(brushbmp,(i%COMBOS_PER_ROW)<<4,(i/COMBOS_PER_ROW)<<4,Combo+i,CSet,Flags&(cFLAGS|cWALK),0);
                }
            }
        }
        else
        {
            int c = 0;
            
            for(int i=0; i<256; i++)
            {
                if(((i%COMBOS_PER_ROW)<BrushWidth)&&((i/COMBOS_PER_ROW)<BrushHeight))
                {
                    put_combo(brushbmp,(i%COMBOS_PER_ROW)<<4,(i/COMBOS_PER_ROW)<<4,Combo+c,CSet,Flags&(cFLAGS|cWALK),0);
                }
                
                ++c;
                
                if((i&3)==3)
                    c+=48;
                    
                if((i%COMBOS_PER_ROW)==(COMBOS_PER_ROW-1))
                    c-=256;
            }
        }
    }
}

byte relational_source_grid[256]=
{
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
    16, 16, 17, 17, 18, 18, 19, 19, 16, 16, 17, 17, 18, 18, 19, 19,
    20, 20, 20, 20, 21, 21, 21, 21, 22, 22, 22, 22, 23, 23, 23, 23,
    24, 24, 24, 24, 25, 25, 25, 25, 24, 24, 24, 24, 25, 25, 25, 25,
    26, 27, 26, 27, 26, 27, 26, 27, 28, 29, 28, 29, 28, 29, 28, 29,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    31, 31, 31, 31, 31, 31, 31, 31, 32, 32, 32, 32, 32, 32, 32, 32,
    33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
    34, 35, 36, 37, 34, 35, 36, 37, 34, 35, 36, 37, 34, 35, 36, 37,
    38, 38, 39, 39, 38, 38, 39, 39, 38, 38, 39, 39, 38, 38, 39, 39,
    40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40,
    41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41, 41,
    42, 43, 42, 43, 42, 43, 42, 43, 42, 43, 42, 43, 42, 43, 42, 43,
    44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 44,
    45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45,
    46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46
};


void draw(bool justcset)
{
    Map.Ugo();
    saved=false;
    int drawmap, drawscr;
    
    if(CurrentLayer==0)
    {
        drawmap=Map.getCurrMap();
        drawscr=Map.getCurrScr();
    }
    else
    {
        drawmap=Map.CurrScr()->layermap[CurrentLayer-1]-1;
        drawscr=Map.CurrScr()->layerscreen[CurrentLayer-1];
        
        if(drawmap<0)
        {
            return;
        }
    }
    
    if(!(Map.AbsoluteScr(drawmap, drawscr)->valid&mVALID))
    {
        Map.CurrScr()->valid|=mVALID;
        Map.AbsoluteScr(drawmap, drawscr)->valid|=mVALID;
        Map.setcolor(Color);
    }
    
    refresh(rMAP+rSCRMAP);
    
    while(Backend::mouse->anyButtonClicked())
    {
        int x= Backend::mouse->getVirtualScreenX();
        int y= Backend::mouse->getVirtualScreenY();
        double startx=mapscreen_x()+(showedges()?(16*mapscreensize()):0);
        double starty=mapscreen_y()+(showedges()?(16*mapscreensize()):0);
        int startxint=mapscreen_x()+(showedges()?int(16*mapscreensize()):0);
        int startyint=mapscreen_y()+(showedges()?int(16*mapscreensize()):0);
        
        if(isinRect(x,y,startxint,startyint,int(startx+(256*mapscreensize())-1),int(starty+(176*mapscreensize())-1)))
        {
            int cxstart=(x-startxint)/int(16*mapscreensize());
            int cystart=(y-startyint)/int(16*mapscreensize());
            int cstart=(cystart*16)+cxstart;
            combo_alias *combo = &combo_aliases[combo_apos];
            
            switch(draw_mode)
            {
            case dm_normal:
            {
                int cc=Combo;
                
                if(!combo_cols)
                {
                    for(int cy=0; cy+cystart<11&&cy<BrushHeight; cy++)
                    {
                        for(int cx=0; cx+cxstart<16&&cx<BrushWidth; cx++)
                        {
                            int c=cstart+(cy*16)+cx;
                            
                            if(!(key[KEY_LSHIFT]||key[KEY_RSHIFT]))
                            {
                                if(!justcset) Map.AbsoluteScr(drawmap, drawscr)->data[c]=cc+cx;
                            }
                            
                            Map.AbsoluteScr(drawmap, drawscr)->cset[c]=CSet;
                        }
                        
                        cc+=20;
                    }
                }
                else
                {
                    int p=Combo/256;
                    int pc=Combo%256;
                    
                    for(int cy=0; cy+cystart<11&&cy<BrushHeight; cy++)
                    {
                        for(int cx=0; cx+cxstart<16&&cx<BrushWidth; cx++)
                        {
                            int c=cstart+(cy*16)+cx;
                            cc=((cx/4)*52)+(cy*4)+(cx%4)+pc;
                            
                            if(cc>=0&&cc<256)
                            {
                                cc+=(p*256);
                                
                                if(!justcset) Map.AbsoluteScr(drawmap, drawscr)->data[c]=cc;
                                
                                Map.AbsoluteScr(drawmap, drawscr)->cset[c]=CSet;
                            }
                        }
                    }
                }
                
                update_combobrush();
            }
            break;
            
            case dm_relational:
            {
                int c2,c3;
                int cx, cy, cx2, cy2;
                cy=cstart>>4;
                cx=cstart&15;
                
                if(key[KEY_LSHIFT]||key[KEY_RSHIFT])
                {
                    relational_tile_grid[(cy+rtgyo)][cx+rtgxo]=1;
                    Map.AbsoluteScr(drawmap, drawscr)->data[cstart]=Combo+47;
                    Map.AbsoluteScr(drawmap, drawscr)->cset[cstart]=CSet;
                }
                else
                {
                    relational_tile_grid[(cy+rtgyo)][cx+rtgxo]=0;
                }
                
                for(int y2=-1; y2<2; ++y2)
                {
                    cy2=cy+y2;
                    
                    if((cy2>11)||(cy2<0))
                    {
                        continue;
                    }
                    
                    for(int x2=-1; x2<2; ++x2)
                    {
                        cx2=cx+x2;
                        
                        if((cx2>15)||(cx2<0))
                        {
                            continue;
                        }
                        
                        c2=cstart+(y2*16)+x2;
                        c3=((relational_tile_grid[((cy2-1)+rtgyo)][(cx2+1)+rtgxo]?1:0)<<0)+
                           ((relational_tile_grid[((cy2-1)+rtgyo)][(cx2-1)+rtgxo]?1:0)<<1)+
                           ((relational_tile_grid[((cy2+1)+rtgyo)][(cx2-1)+rtgxo]?1:0)<<2)+
                           ((relational_tile_grid[((cy2+1)+rtgyo)][(cx2+1)+rtgxo]?1:0)<<3)+
                           ((relational_tile_grid[((cy2)+rtgyo)][(cx2+1)+rtgxo]?1:0)<<4)+
                           ((relational_tile_grid[((cy2-1)+rtgyo)][(cx2)+rtgxo]?1:0)<<5)+
                           ((relational_tile_grid[((cy2)+rtgyo)][(cx2-1)+rtgxo]?1:0)<<6)+
                           ((relational_tile_grid[((cy2+1)+rtgyo)][(cx2)+rtgxo]?1:0)<<7);
                           
                        if(relational_tile_grid[((c2>>4)+rtgyo)][(c2&15)+rtgxo]==0)
                        {
                            Map.AbsoluteScr(drawmap, drawscr)->data[c2]=Combo+relational_source_grid[c3];
                            Map.AbsoluteScr(drawmap, drawscr)->cset[c2]=CSet;
                        }
                    }
                }
            }
            break;
            
            case dm_dungeon:
            {
                int c2,c3,c4;
                int cx, cy, cx2, cy2;
                cy=cstart>>4;
                cx=cstart&15;
                
                if(key[KEY_LSHIFT]||key[KEY_RSHIFT])
                {
                    relational_tile_grid[(cy+rtgyo)][cx+rtgxo]=0;
                    
                    for(int y2=-1; y2<2; ++y2)
                    {
                        cy2=cy+y2;
                        
                        if((cy2>11)||(cy2<0))
                        {
                            continue;
                        }
                        
                        for(int x2=-1; x2<2; ++x2)
                        {
                            cx2=cx+x2;
                            
                            if((cx2>15)||(cx2<0))
                            {
                                continue;
                            }
                            
                            if(relational_tile_grid[(cy2+rtgyo)][cx2+rtgxo]!=0)
                            {
                                relational_tile_grid[(cy2+rtgyo)][cx2+rtgxo]=1;
                            };
                        }
                    }
                    
                    Map.AbsoluteScr(drawmap, drawscr)->data[cstart]=Combo;
                    Map.AbsoluteScr(drawmap, drawscr)->cset[cstart]=CSet;
                }
                else
                {
                    relational_tile_grid[(cy+rtgyo)][cx+rtgxo]=2;
                    
                    for(int y2=-1; y2<2; ++y2)
                    {
                        cy2=cy+y2;
                        
                        if((cy2>11)||(cy2<0))
                        {
                            continue;
                        }
                        
                        for(int x2=-1; x2<2; ++x2)
                        {
                            cx2=cx+x2;
                            
                            if((cx2>15)||(cx2<0))
                            {
                                continue;
                            }
                            
                            if(relational_tile_grid[(cy2+rtgyo)][cx2+rtgxo]==0)
                            {
                                relational_tile_grid[(cy2+rtgyo)][cx2+rtgxo]=1;
                            };
                        }
                    }
                    
                    Map.AbsoluteScr(drawmap, drawscr)->data[cstart]=Combo+48+47;
                    Map.AbsoluteScr(drawmap, drawscr)->cset[cstart]=CSet;
                }
                
                for(int y2=0; y2<11; ++y2)
                {
                    for(int x2=0; x2<16; ++x2)
                    {
                        c2=(y2*16)+x2;
                        c4=relational_tile_grid[((y2)+rtgyo)][(x2)+rtgxo];
                        c3=(((relational_tile_grid[((y2-1)+rtgyo)][(x2+1)+rtgxo]>c4)?1:0)<<0)+
                           (((relational_tile_grid[((y2-1)+rtgyo)][(x2-1)+rtgxo]>c4)?1:0)<<1)+
                           (((relational_tile_grid[((y2+1)+rtgyo)][(x2-1)+rtgxo]>c4)?1:0)<<2)+
                           (((relational_tile_grid[((y2+1)+rtgyo)][(x2+1)+rtgxo]>c4)?1:0)<<3)+
                           (((relational_tile_grid[((y2)+rtgyo)][(x2+1)+rtgxo]>c4)?1:0)<<4)+
                           (((relational_tile_grid[((y2-1)+rtgyo)][(x2)+rtgxo]>c4)?1:0)<<5)+
                           (((relational_tile_grid[((y2)+rtgyo)][(x2-1)+rtgxo]>c4)?1:0)<<6)+
                           (((relational_tile_grid[((y2+1)+rtgyo)][(x2)+rtgxo]>c4)?1:0)<<7);
                           
                        if(relational_tile_grid[(y2+rtgyo)][x2+rtgxo]<2)
                        {
                            Map.AbsoluteScr(drawmap, drawscr)->data[c2]=Combo+relational_source_grid[c3]+(48*c4);
                            Map.AbsoluteScr(drawmap, drawscr)->cset[c2]=CSet;
                        }
                    }
                }
            }
            break;
            
            case dm_alias:
                if(!combo->layermask)
                {
                    int ox=0, oy=0;
                    
                    switch(alias_origin)
                    {
                    case 0:
                        ox=0;
                        oy=0;
                        break;
                        
                    case 1:
                        ox=(combo->width);
                        oy=0;
                        break;
                        
                    case 2:
                        ox=0;
                        oy=(combo->height);
                        break;
                        
                    case 3:
                        ox=(combo->width);
                        oy=(combo->height);
                        break;
                    }
                    
                    for(int cy=0; cy-oy+cystart<11&&cy<=combo->height; cy++)
                    {
                        for(int cx=0; cx-ox+cxstart<16&&cx<=combo->width; cx++)
                        {
                            if((cx+cxstart-ox>=0)&&(cy+cystart-oy>=0))
                            {
                                int c=cstart+((cy-oy)*16)+cx-ox;
                                int p=(cy*(combo->width+1))+cx;
                                
                                if(combo->combos[p])
                                {
                                    Map.AbsoluteScr(drawmap, drawscr)->data[c]=combo->combos[p];
                                    Map.AbsoluteScr(drawmap, drawscr)->cset[c]=wrap(combo->csets[p]+alias_cset_mod, 0, 11);
                                }
                            }
                        }
                    }
                }
                else
                {
                    int amap=0, ascr=0;
                    int lcheck = 1;
                    int laypos = 0;
                    int ox=0, oy=0;
                    
                    switch(alias_origin)
                    {
                    case 0:
                        ox=0;
                        oy=0;
                        break;
                        
                    case 1:
                        ox=(combo->width);
                        oy=0;
                        break;
                        
                    case 2:
                        ox=0;
                        oy=(combo->height);
                        break;
                        
                    case 3:
                        ox=(combo->width);
                        oy=(combo->height);
                        break;
                    }
                    
                    for(int cz=0; cz<7; cz++, lcheck<<=1)
                    {
                        if(!cz)
                        {
                            amap = Map.getCurrMap();
                            ascr = Map.getCurrScr();
                        }
                        else
                        {
                            if(cz==1) lcheck>>=1;
                            
                            if(combo->layermask&lcheck)
                            {
                                amap = Map.CurrScr()->layermap[cz-1]-1;
                                ascr = Map.CurrScr()->layerscreen[cz-1];
                                laypos++;
                            }
                        }
                        
                        for(int cy=0; cy-oy+cystart<11&&cy<=combo->height; cy++)
                        {
                            for(int cx=0; cx-ox+cxstart<16&&cx<=combo->width; cx++)
                            {
                                if((!cz)||/*(Map.CurrScr()->layermap[cz>0?cz-1:0])*/amap>=0)
                                {
                                    if((cz==0)||(combo->layermask&lcheck))
                                    {
                                        if((cx+cxstart-ox>=0)&&(cy+cystart-oy>=0))
                                        {
                                            int c=cstart+((cy-oy)*16)+cx-ox;
                                            int p=((cy*(combo->width+1))+cx)+((combo->width+1)*(combo->height+1)*laypos);
                                            
                                            if((combo->combos[p])&&(amap>=0))
                                            {
                                                Map.AbsoluteScr(amap, ascr)->data[c]=combo->combos[p];
                                                Map.AbsoluteScr(amap, ascr)->cset[c]=wrap(combo->csets[p]+alias_cset_mod, 0, 11);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                
                break;
            }
        }
        
        do_animations();
        refresh(rALL);
    }
}




void replace(int c)
{
    saved=false;
    Map.Ugo();
    int drawmap, drawscr;
    
    if(CurrentLayer==0)
    {
        drawmap=Map.getCurrMap();
        drawscr=Map.getCurrScr();
    }
    else
    {
        drawmap=Map.CurrScr()->layermap[CurrentLayer-1]-1;
        drawscr=Map.CurrScr()->layerscreen[CurrentLayer-1];
        
        if(drawmap<0)
        {
            return;
        }
    }
    
    int targetcombo = Map.AbsoluteScr(drawmap, drawscr)->data[c];
    int targetcset  = Map.AbsoluteScr(drawmap, drawscr)->cset[c];
    
    if(key[KEY_LSHIFT] || key[KEY_RSHIFT])
    {
        for(int i=0; i<176; i++)
        {
            if((Map.AbsoluteScr(drawmap, drawscr)->cset[i])==targetcset)
            {
                Map.AbsoluteScr(drawmap, drawscr)->cset[i]=CSet;
            }
        }
    }
    else
    {
        for(int i=0; i<176; i++)
        {
            if(((Map.AbsoluteScr(drawmap, drawscr)->data[i])==targetcombo) &&
                    ((Map.AbsoluteScr(drawmap, drawscr)->cset[i])==targetcset))
            {
                Map.AbsoluteScr(drawmap, drawscr)->data[i]=Combo;
                Map.AbsoluteScr(drawmap, drawscr)->cset[i]=CSet;
            }
        }
    }
    
    refresh(rMAP);
}

void draw_block(int start,int w,int h)
{
    saved=false;
    Map.Ugo();
    int drawmap, drawscr;
    
    if(CurrentLayer==0)
    {
        drawmap=Map.getCurrMap();
        drawscr=Map.getCurrScr();
    }
    else
    {
        drawmap=Map.CurrScr()->layermap[CurrentLayer-1]-1;
        drawscr=Map.CurrScr()->layerscreen[CurrentLayer-1];
        
        if(drawmap<0)
        {
            return;
        }
    }
    
    if(!(Map.AbsoluteScr(drawmap, drawscr)->valid&mVALID))
    {
        Map.CurrScr()->valid|=mVALID;
        Map.AbsoluteScr(drawmap, drawscr)->valid|=mVALID;
        Map.setcolor(Color);
    }
    
    for(int y=0; y<h && (y<<4)+start < 176; y++)
        for(int x=0; x<w && (start&15)+x < 16; x++)
        {
            Map.AbsoluteScr(drawmap, drawscr)->data[start+(y<<4)+x]=Combo+(y*4)+x;
            Map.AbsoluteScr(drawmap, drawscr)->cset[start+(y<<4)+x]=CSet;
            
        }
        
    refresh(rMAP+rSCRMAP);
}

void fill(mapscr* fillscr, int targetcombo, int targetcset, int sx, int sy, int dir, int diagonal, bool only_cset)
{
    if(!only_cset)
    {
        if((fillscr->data[((sy<<4)+sx)])!=targetcombo)
            return;
    }
    
    if((fillscr->cset[((sy<<4)+sx)])!=targetcset)
        return;
        
    if(!only_cset)
    {
        fillscr->data[((sy<<4)+sx)]=Combo;
    }
    
    fillscr->cset[((sy<<4)+sx)]=CSet;
    
    if((sy>0) && (dir!=down))                                 // && ((Map.CurrScr()->data[(((sy-1)<<4)+sx)]&0x7FF)==target))
        fill(fillscr, targetcombo, targetcset, sx, sy-1, up, diagonal, only_cset);
        
    if((sy<10) && (dir!=up))                                  // && ((Map.CurrScr()->data[(((sy+1)<<4)+sx)]&0x7FF)==target))
        fill(fillscr, targetcombo, targetcset, sx, sy+1, down, diagonal, only_cset);
        
    if((sx>0) && (dir!=right))                                // && ((Map.CurrScr()->data[((sy<<4)+sx-1)]&0x7FF)==target))
        fill(fillscr, targetcombo, targetcset, sx-1, sy, left, diagonal, only_cset);
        
    if((sx<15) && (dir!=left))                                // && ((Map.CurrScr()->data[((sy<<4)+sx+1)]&0x7FF)==target))
        fill(fillscr, targetcombo, targetcset, sx+1, sy, right, diagonal, only_cset);
        
    if(diagonal==1)
    {
        if((sy>0) && (sx>0) && (dir!=r_down))                   // && ((Map.CurrScr()->data[(((sy-1)<<4)+sx-1)]&0x7FF)==target))
            fill(fillscr, targetcombo, targetcset, sx-1, sy-1, l_up, diagonal, only_cset);
            
        if((sy<10) && (sx<15) && (dir!=l_up))                   // && ((Map.CurrScr()->data[(((sy+1)<<4)+sx+1)]&0x7FF)==target))
            fill(fillscr, targetcombo, targetcset, sx+1, sy+1, r_down, diagonal, only_cset);
            
        if((sx>0) && (sy<10) && (dir!=r_up))                    // && ((Map.CurrScr()->data[(((sy+1)<<4)+sx-1)]&0x7FF)==target))
            fill(fillscr, targetcombo, targetcset, sx-1, sy+1, l_down, diagonal, only_cset);
            
        if((sx<15) && (sy>0) && (dir!=l_down))                  // && ((Map.CurrScr()->data[(((sy-1)<<4)+sx+1)]&0x7FF)==target))
            fill(fillscr, targetcombo, targetcset, sx+1, sy-1, r_up, diagonal, only_cset);
    }
    
}


void fill2(mapscr* fillscr, int targetcombo, int targetcset, int sx, int sy, int dir, int diagonal, bool only_cset)
{
    if(!only_cset)
    {
        if((fillscr->data[((sy<<4)+sx)])==targetcombo)
            return;
    }
    
    if((fillscr->cset[((sy<<4)+sx)])==targetcset)
        return;
        
    if(!only_cset)
    {
        fillscr->data[((sy<<4)+sx)]=Combo;
    }
    
    fillscr->cset[((sy<<4)+sx)]=CSet;
    
    if((sy>0) && (dir!=down))                                 // && ((Map.CurrScr()->data[(((sy-1)<<4)+sx)]&0x7FF)!=target))
        fill2(fillscr, targetcombo, targetcset, sx, sy-1, up, diagonal, only_cset);
        
    if((sy<10) && (dir!=up))                                  // && ((Map.CurrScr()->data[(((sy+1)<<4)+sx)]&0x7FF)!=target))
        fill2(fillscr, targetcombo, targetcset, sx, sy+1, down, diagonal, only_cset);
        
    if((sx>0) && (dir!=right))                                // && ((Map.CurrScr()->data[((sy<<4)+sx-1)]&0x7FF)!=target))
        fill2(fillscr, targetcombo, targetcset, sx-1, sy, left, diagonal, only_cset);
        
    if((sx<15) && (dir!=left))                                // && ((Map.CurrScr()->data[((sy<<4)+sx+1)]&0x7FF)!=target))
        fill2(fillscr, targetcombo, targetcset, sx+1, sy, right, diagonal, only_cset);
        
    if(diagonal==1)
    {
        if((sy>0) && (sx>0) && (dir!=r_down))                   // && ((Map.CurrScr()->data[(((sy-1)<<4)+sx-1)]&0x7FF)!=target))
            fill2(fillscr, targetcombo, targetcset, sx-1, sy-1, l_up, diagonal, only_cset);
            
        if((sy<10) && (sx<15) && (dir!=l_up))                   // && ((Map.CurrScr()->data[(((sy+1)<<4)+sx+1)]&0x7FF)!=target))
            fill2(fillscr, targetcombo, targetcset, sx+1, sy+1, r_down, diagonal, only_cset);
            
        if((sx>0) && (sy<10) && (dir!=r_up))                    // && ((Map.CurrScr()->data[(((sy+1)<<4)+sx-1)]&0x7FF)!=target))
            fill2(fillscr, targetcombo, targetcset, sx-1, sy+1, l_down, diagonal, only_cset);
            
        if((sx<15) && (sy>0) && (dir!=l_down))                  // && ((Map.CurrScr()->data[(((sy-1)<<4)+sx+1)]&0x7FF)!=target))
            fill2(fillscr, targetcombo, targetcset, sx+1, sy-1, r_up, diagonal, only_cset);
    }
}


/**************************/
/*****     Mouse      *****/
/**************************/

void doxypos(byte &px2,byte &py2,int color,int mask)
{
    doxypos(px2,py2,color,mask,false,0,0,16,16);
}

void doxypos(byte &px2,byte &py2,int color,int mask, bool immediately, int cursoroffx, int cursoroffy, int iconw, int iconh)
{
    int tempcb=ComboBrush;
    ComboBrush=0;
	Backend::mouse->setCursorSprite(mouse_bmp[MOUSE_BMP_POINT_BOX][0]);
    
    int oldpx=px2, oldpy=py2;
    double startx=mapscreen_x()+(showedges()?(16*mapscreensize()):0);
    double starty=mapscreen_y()+(showedges()?(16*mapscreensize()):0);
    int startxint=mapscreen_x()+(showedges()?int(16*mapscreensize()):0);
    int startyint=mapscreen_y()+(showedges()?int(16*mapscreensize()):0);
    showxypos_x=px2;
    showxypos_y=py2;
    showxypos_w=iconw;
    showxypos_h=iconh;
    showxypos_color=vc(color);
    showxypos_icon=true;
    bool canedit=false;
    bool done=false;
    
    while(!done && (!(Backend::mouse->rightButtonClicked() || immediately)))
    {
        int x= Backend::mouse->getVirtualScreenX();
        int y= Backend::mouse->getVirtualScreenY();
        
        if(!Backend::mouse->anyButtonClicked() || immediately)
        {
            canedit=true;
        }
        
        if(canedit && Backend::mouse->leftButtonClicked() && isinRect(x,y,startxint,startyint,int(startx+(256*mapscreensize())-1),int(starty+(176*mapscreensize())-1)))
        {
			int bx = startxint;
			int by = startyint;
			int tx = int(startxint + (256 * mapscreensize()) - 1);
			int ty = int(startyint + (176 * mapscreensize()) - 1);
			Backend::mouse->setVirtualScreenMouseBounds(bx, by, tx, ty);
            
            while(Backend::mouse->leftButtonClicked())
            {
                x=int((Backend::mouse->getVirtualScreenX()-(showedges()?int(16*mapscreensize()):0))/mapscreensize())-cursoroffx;
                y=int((Backend::mouse->getVirtualScreenY()-16-(showedges()?int(16*mapscreensize()):0))/mapscreensize())-cursoroffy;
                showxypos_cursor_icon=true;
                showxypos_cursor_x=x&mask;
                showxypos_cursor_y=y&mask;
                do_animations();
                refresh(rALL | rNOCURSOR);
                int xpos, ypos;
                
                if(is_large())
                {
                    xpos = 450;
                    ypos = 405;
                }
                else
                {
                    xpos = 700;
                    ypos = 500;
                }
                
                textprintf_ex(screen,font,xpos,ypos,vc(15),vc(0),"%d %d %d %d",startxint,startyint,int(startxint+(256*mapscreensize())-1),int(startyint+(176*mapscreensize())-1));
                textprintf_ex(screen,font,xpos,ypos+10,vc(15),vc(0),"%d %d %d %d %d %d",x,y, Backend::mouse->getVirtualScreenX(), Backend::mouse->getVirtualScreenY(),showxypos_cursor_x,showxypos_cursor_y);
            }
            
            if(!Backend::mouse->anyButtonClicked())
            {
                px2=byte(x&mask);
                py2=byte(y&mask);
            }
            
			int mbx = 0;
			int mby = 0;
			int mtx = Backend::graphics->virtualScreenW() - 1;
			int mty = Backend::graphics->virtualScreenH() - 1;
			Backend::mouse->setVirtualScreenMouseBounds(mbx, mby, mtx, mty);
            done=true;
        }
        
        if(keypressed())
        {
            switch(readkey()>>8)
            {
            case KEY_ESC:
            case KEY_ENTER:
                goto finished;
            }
        }
        
        do_animations();
        refresh(rALL | rNOCURSOR);
        //if(zqwin_scale > 1)
        {
            //stretch_blit(screen, hw_screen, 0, 0, screen->w, screen->h, 0, 0, hw_screen->w, hw_screen->h);
        }
        // else
        {
            //blit(screen, hw_screen, 0, 0, 0, 0, screen->w, screen->h);
        }
    }
    
finished:
	Backend::mouse->setCursorSprite(mouse_bmp[MOUSE_BMP_NORMAL][0]);
    refresh(rMAP+rMENU);
    
    while(Backend::mouse->anyButtonClicked())
    {
		Backend::graphics->waitTick();
		Backend::graphics->showBackBuffer();
        /* do nothing */
    }
    
    showxypos_x=-1000;
    showxypos_y=-1000;
    showxypos_color=-1000;
    showxypos_ffc=-1000;
    showxypos_icon=false;
    showxypos_cursor_x=-1000;
    showxypos_cursor_y=-1000;
    showxypos_cursor_icon=false;
    
    if(px2!=oldpx||py2!=oldpy)
    {
        saved=false;
    }
    
    ComboBrush=tempcb;
}

void doflags()
{
	Backend::mouse->setCursorSprite(mouse_bmp[MOUSE_BMP_FLAG][0]);
    int of=Flags;
    Flags=cFLAGS;
    refresh(rMAP | rNOCURSOR);
    
    bool canedit=false;
    
    while(!(Backend::mouse->rightButtonClicked()))
    {
        int x= Backend::mouse->getVirtualScreenX();
        int y= Backend::mouse->getVirtualScreenY();
        double startx=mapscreen_x()+(showedges()?(16*mapscreensize()):0);
        double starty=mapscreen_y()+(showedges()?(16*mapscreensize()):0);
        int startxint=mapscreen_x()+(showedges()?int(16*mapscreensize()):0);
        int startyint=mapscreen_y()+(showedges()?int(16*mapscreensize()):0);
        int cx=(x-startxint)/int(16*mapscreensize());
        int cy=(y-startyint)/int(16*mapscreensize());
        int c=(cy*16)+cx;
        
        if(!Backend::mouse->anyButtonClicked())
            canedit=true;
            
        if(canedit && Backend::mouse->leftButtonClicked() && isinRect(x,y,startxint,startyint,int(startx+(256*mapscreensize())-1),int(starty+(176*mapscreensize())-1)))
        {
            saved=false;
            
            if(CurrentLayer==0)
            {
                Map.CurrScr()->sflag[c]=Flag;
            }
            else
            {
                // Notify if they are using a flag that doesn't work on this layer.
                if((Flag >= mfTRAP_H && Flag <= mfNOBLOCKS) || (Flag == mfFAIRY) || (Flag == mfMAGICFAIRY)
                        || (Flag == mfALLFAIRY) || (Flag == mfRAFT) || (Flag == mfRAFT_BRANCH)
                        || (Flag == mfDIVE_ITEM) || (Flag == mfARMOS_SECRET) || (Flag == mfNOENEMY)
                        || (Flag == mfBLOCKHOLE) || (Flag == mfZELDA))
                {
                    char buf[38];
                    sprintf(buf, "You are currently working on layer %d.", CurrentLayer);
                    jwin_alert("Notice",buf,"This combo flag only functions when placed on layer 0.",NULL,"O&K",NULL,'k',0,lfont);
                }
                
                TheMaps[(Map.CurrScr()->layermap[CurrentLayer-1]-1)*MAPSCRS+(Map.CurrScr()->layerscreen[CurrentLayer-1])].sflag[c]=Flag;
                //      Map.CurrScr()->sflag[c]=Flag;
            }
            
            refresh(rMAP | rNOCURSOR);
        }
        
        if(Backend::mouse->getWheelPosition() != 0)
        {
            for(int i=0; i<abs(Backend::mouse->getWheelPosition()); ++i)
            {
                if(Backend::mouse->getWheelPosition()>0)
                {
                    onIncreaseFlag();
                }
                else
                {
                    onDecreaseFlag();
                }
            }
            
			Backend::mouse->setWheelPosition(0);
        }
        
        if(keypressed())
        {
            switch(readkey()>>8)
            {
            case KEY_ESC:
            case KEY_ENTER:
                goto finished;
                
            case KEY_ASTERISK:
            case KEY_CLOSEBRACE:
                onIncreaseFlag();
                break;
                
            case KEY_SLASH_PAD:
            case KEY_OPENBRACE:
                onDecreaseFlag();
                break;
                
            case KEY_UP:
                onUp();
                break;
                
            case KEY_DOWN:
                onDown();
                break;
                
            case KEY_LEFT:
                onLeft();
                break;
                
            case KEY_RIGHT:
                onRight();
                break;
                
            case KEY_PGUP:
                onPgUp();
                break;
                
            case KEY_PGDN:
                onPgDn();
                break;
                
            case KEY_COMMA:
                onDecMap();
                break;
                
            case KEY_STOP:
                onIncMap();
                break;
            }
            
            // The cursor could've been overwritten by the Combo Brush?
			Backend::mouse->setCursorSprite(mouse_bmp[MOUSE_BMP_FLAG][0]);
        }
        
        do_animations();
        refresh(rALL | rNOCURSOR);
        //if(zqwin_scale > 1)
        {
            //stretch_blit(screen, hw_screen, 0, 0, screen->w, screen->h, 0, 0, hw_screen->w, hw_screen->h);
        }
        // else
        {
            //blit(screen, hw_screen, 0, 0, 0, 0, screen->w, screen->h);
        }
    }
    
finished:
    Flags=of;
	Backend::mouse->setCursorSprite(mouse_bmp[MOUSE_BMP_NORMAL][0]);
    refresh(rMAP+rMENU);
    
    while(Backend::mouse->anyButtonClicked())
    {
		Backend::graphics->waitTick();
		Backend::graphics->showBackBuffer();
        /* do nothing */
    }
}

// Drag FFCs around
void moveffc(int i, int cx, int cy)
{
    int ffx = int(Map.CurrScr()->ffx[i]/10000.0);
    int ffy = int(Map.CurrScr()->ffy[i]/10000.0);
    showxypos_ffc = i;
    doxypos((byte&)ffx,(byte&)ffy,15,0xFF,true,cx-ffx,cy-ffy,((1+(Map.CurrScr()->ffwidth[i]>>6))*16),((1+(Map.CurrScr()->ffheight[i]>>6))*16));
    
    if((ffx != int(Map.CurrScr()->ffx[i]/10000.0)) || (ffy != int(Map.CurrScr()->ffy[i]/10000.0)))
    {
        Map.CurrScr()->ffx[i] = ffx*10000;
        Map.CurrScr()->ffy[i] = ffy*10000;
        saved = false;
    }
}

void set_brush_width(int width);
void set_brush_height(int height);

int set_brush_width_1()
{
    set_brush_width(1);
    return D_O_K;
}
int set_brush_width_2()
{
    set_brush_width(2);
    return D_O_K;
}
int set_brush_width_3()
{
    set_brush_width(3);
    return D_O_K;
}
int set_brush_width_4()
{
    set_brush_width(4);
    return D_O_K;
}
int set_brush_width_5()
{
    set_brush_width(5);
    return D_O_K;
}
int set_brush_width_6()
{
    set_brush_width(6);
    return D_O_K;
}
int set_brush_width_7()
{
    set_brush_width(7);
    return D_O_K;
}
int set_brush_width_8()
{
    set_brush_width(8);
    return D_O_K;
}
int set_brush_width_9()
{
    set_brush_width(9);
    return D_O_K;
}
int set_brush_width_10()
{
    set_brush_width(10);
    return D_O_K;
}
int set_brush_width_11()
{
    set_brush_width(11);
    return D_O_K;
}
int set_brush_width_12()
{
    set_brush_width(12);
    return D_O_K;
}
int set_brush_width_13()
{
    set_brush_width(13);
    return D_O_K;
}
int set_brush_width_14()
{
    set_brush_width(14);
    return D_O_K;
}
int set_brush_width_15()
{
    set_brush_width(15);
    return D_O_K;
}
int set_brush_width_16()
{
    set_brush_width(16);
    return D_O_K;
}

int set_brush_height_1()
{
    set_brush_height(1);
    return D_O_K;
}
int set_brush_height_2()
{
    set_brush_height(2);
    return D_O_K;
}
int set_brush_height_3()
{
    set_brush_height(3);
    return D_O_K;
}
int set_brush_height_4()
{
    set_brush_height(4);
    return D_O_K;
}
int set_brush_height_5()
{
    set_brush_height(5);
    return D_O_K;
}
int set_brush_height_6()
{
    set_brush_height(6);
    return D_O_K;
}
int set_brush_height_7()
{
    set_brush_height(7);
    return D_O_K;
}
int set_brush_height_8()
{
    set_brush_height(8);
    return D_O_K;
}
int set_brush_height_9()
{
    set_brush_height(9);
    return D_O_K;
}
int set_brush_height_10()
{
    set_brush_height(10);
    return D_O_K;
}
int set_brush_height_11()
{
    set_brush_height(11);
    return D_O_K;
}

static MENU brush_width_menu[] =
{
    { (char *)"1",          set_brush_width_1,    NULL, 0, NULL },
    { (char *)"2",          set_brush_width_2,    NULL, 0, NULL },
    { (char *)"3",          set_brush_width_3,    NULL, 0, NULL },
    { (char *)"4",          set_brush_width_4,    NULL, 0, NULL },
    { (char *)"5",          set_brush_width_5,    NULL, 0, NULL },
    { (char *)"6",          set_brush_width_6,    NULL, 0, NULL },
    { (char *)"7",          set_brush_width_7,    NULL, 0, NULL },
    { (char *)"8",          set_brush_width_8,    NULL, 0, NULL },
    { (char *)"9",          set_brush_width_9,    NULL, 0, NULL },
    { (char *)"10",         set_brush_width_10,   NULL, 0, NULL },
    { (char *)"11",         set_brush_width_11,   NULL, 0, NULL },
    { (char *)"12",         set_brush_width_12,   NULL, 0, NULL },
    { (char *)"13",         set_brush_width_13,   NULL, 0, NULL },
    { (char *)"14",         set_brush_width_14,   NULL, 0, NULL },
    { (char *)"15",         set_brush_width_15,   NULL, 0, NULL },
    { (char *)"16",         set_brush_width_16,   NULL, 0, NULL },
    { NULL,                 NULL,                 NULL, 0, NULL }
};

static MENU brush_height_menu[] =
{

    { (char *)"1",          set_brush_height_1,   NULL, 0, NULL },
    { (char *)"2",          set_brush_height_2,   NULL, 0, NULL },
    { (char *)"3",          set_brush_height_3,   NULL, 0, NULL },
    { (char *)"4",          set_brush_height_4,   NULL, 0, NULL },
    { (char *)"5",          set_brush_height_5,   NULL, 0, NULL },
    { (char *)"6",          set_brush_height_6,   NULL, 0, NULL },
    { (char *)"7",          set_brush_height_7,   NULL, 0, NULL },
    { (char *)"8",          set_brush_height_8,   NULL, 0, NULL },
    { (char *)"9",          set_brush_height_9,   NULL, 0, NULL },
    { (char *)"10",         set_brush_height_10,  NULL, 0, NULL },
    { (char *)"11",         set_brush_height_11,  NULL, 0, NULL },
    { NULL,                 NULL,                 NULL, 0, NULL }
};

int set_flood();
int set_fill_4();
int set_fill_8();
int set_fill2_4();
int set_fill2_8();

void flood()
{
    // int start=0, w=0, h=0;
    int drawmap, drawscr;
    
    if(CurrentLayer==0)
    {
        drawmap=Map.getCurrMap();
        drawscr=Map.getCurrScr();
    }
    else
    {
        drawmap=Map.CurrScr()->layermap[CurrentLayer-1]-1;
        drawscr=Map.CurrScr()->layerscreen[CurrentLayer-1];
        
        if(drawmap<0)
        {
            return;
        }
    }
    
    saved=false;
    Map.Ugo();
    
    if(!(Map.AbsoluteScr(drawmap, drawscr)->valid&mVALID))
    {
        Map.CurrScr()->valid|=mVALID;
        Map.AbsoluteScr(drawmap, drawscr)->valid|=mVALID;
        Map.setcolor(Color);
    }
    
    /* for(int y=0; y<h && (y<<4)+start < 176; y++)
      for(int x=0; x<w && (start&15)+x < 16; x++)
      */
    if(!(key[KEY_LSHIFT]||key[KEY_RSHIFT]))
    {
        for(int i=0; i<176; i++)
        {
            Map.AbsoluteScr(drawmap, drawscr)->data[i]=Combo;
        }
    }
    
    for(int i=0; i<176; i++)
    {
        Map.AbsoluteScr(drawmap, drawscr)->cset[i]=CSet;
    }
    
    refresh(rMAP+rSCRMAP);
}

void fill_4()
{
    int drawmap, drawscr;
    
    if(CurrentLayer==0)
    {
        drawmap=Map.getCurrMap();
        drawscr=Map.getCurrScr();
    }
    else
    {
        drawmap=Map.CurrScr()->layermap[CurrentLayer-1]-1;
        drawscr=Map.CurrScr()->layerscreen[CurrentLayer-1];
        
        if(drawmap<0)
        {
            return;
        }
    }
    
    int x= Backend::mouse->getVirtualScreenX()-mapscreen_x()-(showedges()?(16*mapscreensize()):0);
    int y= Backend::mouse->getVirtualScreenY()-mapscreen_y()-(showedges()?(16*mapscreensize()):0);
    int by= (y>>4)/(mapscreensize());
    int bx= (x>>4)/(mapscreensize());
    
    if(Map.AbsoluteScr(drawmap,drawscr)->cset[(by<<4)+bx]!=CSet ||
            (Map.AbsoluteScr(drawmap,drawscr)->data[(by<<4)+bx]!=Combo &&
             !(key[KEY_LSHIFT]||key[KEY_RSHIFT])))
    {
        saved=false;
        Map.Ugo();
        
        if(!(Map.AbsoluteScr(drawmap, drawscr)->valid&mVALID))
        {
            Map.CurrScr()->valid|=mVALID;
            Map.AbsoluteScr(drawmap, drawscr)->valid|=mVALID;
            Map.setcolor(Color);
        }
        
        fill(Map.AbsoluteScr(drawmap, drawscr),
             (Map.AbsoluteScr(drawmap, drawscr)->data[(by<<4)+bx]),
             (Map.AbsoluteScr(drawmap, drawscr)->cset[(by<<4)+bx]), bx, by, 255, 0, (key[KEY_LSHIFT]||key[KEY_RSHIFT]));
        refresh(rMAP+rSCRMAP);
    }
}

void fill_8()
{
    int drawmap, drawscr;
    
    if(CurrentLayer==0)
    {
        drawmap=Map.getCurrMap();
        drawscr=Map.getCurrScr();
    }
    else
    {
        drawmap=Map.CurrScr()->layermap[CurrentLayer-1]-1;
        drawscr=Map.CurrScr()->layerscreen[CurrentLayer-1];
        
        if(drawmap<0)
        {
            return;
        }
    }
    
    int x= Backend::mouse->getVirtualScreenX()-mapscreen_x()-(showedges()?(16*mapscreensize()):0);
    int y= Backend::mouse->getVirtualScreenY()-mapscreen_y()-(showedges()?(16*mapscreensize()):0);
    int by= (y>>4)/(mapscreensize());
    int bx= (x>>4)/(mapscreensize());
    
    if(Map.AbsoluteScr(drawmap,drawscr)->cset[(by<<4)+bx]!=CSet ||
            (Map.AbsoluteScr(drawmap,drawscr)->data[(by<<4)+bx]!=Combo &&
             !(key[KEY_LSHIFT]||key[KEY_RSHIFT])))
    {
        saved=false;
        Map.Ugo();
        
        if(!(Map.AbsoluteScr(drawmap, drawscr)->valid&mVALID))
        {
            Map.CurrScr()->valid|=mVALID;
            Map.AbsoluteScr(drawmap, drawscr)->valid|=mVALID;
            Map.setcolor(Color);
        }
        
        fill(Map.AbsoluteScr(drawmap, drawscr),
             (Map.AbsoluteScr(drawmap, drawscr)->data[(by<<4)+bx]),
             (Map.AbsoluteScr(drawmap, drawscr)->cset[(by<<4)+bx]), bx, by, 255, 1, (key[KEY_LSHIFT]||key[KEY_RSHIFT]));
        refresh(rMAP+rSCRMAP);
    }
}

void fill2_4()
{
    int drawmap, drawscr;
    
    if(CurrentLayer==0)
    {
        drawmap=Map.getCurrMap();
        drawscr=Map.getCurrScr();
    }
    
    else
    {
        drawmap=Map.CurrScr()->layermap[CurrentLayer-1]-1;
        drawscr=Map.CurrScr()->layerscreen[CurrentLayer-1];
        
        if(drawmap<0)
        {
            return;
        }
    }
    
    int x= Backend::mouse->getVirtualScreenX()-mapscreen_x()-(showedges()?(16*mapscreensize()):0);
    int y= Backend::mouse->getVirtualScreenY()-mapscreen_y()-(showedges()?(16*mapscreensize()):0);;
    int by= (((y&0xF0))>>4)/(mapscreensize());
    int bx= (x>>4)/(mapscreensize());
    
    saved=false;
    Map.Ugo();
    
    if(!(Map.AbsoluteScr(drawmap, drawscr)->valid&mVALID))
    {
        Map.CurrScr()->valid|=mVALID;
        Map.AbsoluteScr(drawmap, drawscr)->valid|=mVALID;
        Map.setcolor(Color);
    }
    
    fill2(Map.AbsoluteScr(drawmap, drawscr), Combo, CSet, bx, by, 255, 0, (key[KEY_LSHIFT]||key[KEY_RSHIFT]));
    refresh(rMAP+rSCRMAP);
}

void fill2_8()
{
    int drawmap, drawscr;
    
    if(CurrentLayer==0)
    {
        drawmap=Map.getCurrMap();
        drawscr=Map.getCurrScr();
    }
    else
    {
        drawmap=Map.CurrScr()->layermap[CurrentLayer-1]-1;
        drawscr=Map.CurrScr()->layerscreen[CurrentLayer-1];
        
        if(drawmap<0)
        {
            return;
        }
    }
    
    int x= Backend::mouse->getVirtualScreenX()-mapscreen_x()-(showedges()?(16*mapscreensize()):0);
    int y= Backend::mouse->getVirtualScreenY()-mapscreen_y()-(showedges()?(16*mapscreensize()):0);;
    int by= (((y&0xF0))>>4)/(mapscreensize());
    int bx= (x>>4)/(mapscreensize());
    
    saved=false;
    Map.Ugo();
    
    if(!(Map.AbsoluteScr(drawmap, drawscr)->valid&mVALID))
    {
        Map.CurrScr()->valid|=mVALID;
        Map.AbsoluteScr(drawmap, drawscr)->valid|=mVALID;
        Map.setcolor(Color);
    }
    
    fill2(Map.AbsoluteScr(drawmap, drawscr), Combo, CSet, bx, by, 255, 1, (key[KEY_LSHIFT]||key[KEY_RSHIFT]));
    refresh(rMAP+rSCRMAP);
}

static MENU fill_menu[] =
{
    { (char *)"Flood",                   set_flood,   NULL, 0, NULL },
    { (char *)"Fill (4-way)",            set_fill_4,  NULL, 0, NULL },
    { (char *)"Fill (8-way)",            set_fill_8,  NULL, 0, NULL },
    { (char *)"Fill2 (4-way)",           set_fill2_4, NULL, 0, NULL },
    { (char *)"Fill2 (8-way)",           set_fill2_8, NULL, 0, NULL },
    { NULL,                              NULL,        NULL, 0, NULL }
};

int set_flood()
{
    for(int x=0; x<5; x++)
    {
        fill_menu[x].flags=0;
    }
    
    fill_menu[0].flags=D_SELECTED;
    fill_type=0;
    return D_O_K;
}

int set_fill_4()
{
    for(int x=0; x<5; x++)
    {
        fill_menu[x].flags=0;
    }
    
    fill_menu[1].flags=D_SELECTED;
    fill_type=1;
    return D_O_K;
}

int set_fill_8()
{
    for(int x=0; x<5; x++)
    {
        fill_menu[x].flags=0;
    }
    
    fill_menu[2].flags=D_SELECTED;
    fill_type=2;
    return D_O_K;
}

int set_fill2_4()
{
    for(int x=0; x<5; x++)
    {
        fill_menu[x].flags=0;
    }
    
    fill_menu[3].flags=D_SELECTED;
    fill_type=3;
    return D_O_K;
}

int set_fill2_8()
{
    for(int x=0; x<5; x++)
    {
        fill_menu[x].flags=0;
    }
    
    fill_menu[4].flags=D_SELECTED;
    fill_type=4;
    return D_O_K;
}

int draw_block_1_2()
{
    draw_block(mousecomboposition,1,2);
    return D_O_K;
}

int draw_block_2_1()
{
    draw_block(mousecomboposition,2,1);
    return D_O_K;
}

int draw_block_2_2()
{
    draw_block(mousecomboposition,2,2);
    return D_O_K;
}

int draw_block_2_3()
{
    draw_block(mousecomboposition,2,3);
    return D_O_K;
}

int draw_block_3_2()
{
    draw_block(mousecomboposition,3,2);
    return D_O_K;
}

int draw_block_4_2()
{
    draw_block(mousecomboposition,4,2);
    return D_O_K;
}

int draw_block_4_4()
{
    draw_block(mousecomboposition,4,4);
    return D_O_K;
}

static MENU draw_block_menu[] =
{
    { (char *)"1x2",                     draw_block_1_2,  NULL,    0, NULL },
    { (char *)"2x1",                     draw_block_2_1,  NULL,    0, NULL },
    { (char *)"2x2",                     draw_block_2_2,  NULL,    0, NULL },
    { (char *)"2x3",                     draw_block_2_3,  NULL,    0, NULL },
    { (char *)"3x2",                     draw_block_3_2,  NULL,    0, NULL },
    { (char *)"4x2",                     draw_block_4_2,  NULL,    0, NULL },
    { (char *)"4x4",                     draw_block_4_4,  NULL,    0, NULL },
    { NULL,                              NULL,            NULL,    0, NULL }
};

static MENU draw_rc_menu[] =
{
    { (char *)"Select Combo",            NULL,  NULL,              0, NULL },
    { (char *)"Scroll to Combo",         NULL,  NULL,              0, NULL },
    { (char *)"Edit Combo",              NULL,  NULL,              0, NULL },
    { (char *)"",                        NULL,  NULL,              0, NULL },
    { (char *)"Replace All",             NULL,  NULL,              0, NULL },
    { (char *)"Draw Block",		       NULL,  draw_block_menu,	0, NULL },
    { (char *)"Set Brush Width\t ",      NULL,  brush_width_menu,  0, NULL },
    { (char *)"Set Brush Height\t ",     NULL,  brush_height_menu, 0, NULL },
    { (char *)"Set Fill Type\t ",        NULL,  fill_menu,         0, NULL },
    { (char *)"",                        NULL,  NULL,              0, NULL },
    { (char *)"Follow Tile Warp",        NULL,  NULL,              0, NULL },
    { (char *)"Edit Tile Warp",          NULL,  NULL,              0, NULL },
    { (char *)"",                        NULL,  NULL,              0, NULL },
    { (char *)"Place + Edit FFC 1",      NULL,  NULL,              0, NULL },
    { (char *)"Paste FFC as FFC 1",      NULL,  NULL,              0, NULL },
    { NULL,                              NULL,  NULL,              0, NULL }
};

static MENU draw_ffc_rc_menu[] =
{
    { (char *)"Copy FFC",            NULL,  NULL,              0, NULL },
    { (char *)"Paste FFC data",           NULL,  NULL,              0, NULL },
    { (char *)"Edit FFC",            NULL,  NULL,              0, NULL },
    { (char *)"Clear FFC",           NULL,  NULL,              0, NULL },
    { NULL,                          NULL,  NULL,              0, NULL }
};

static MENU combosel_rc_menu[] =
{
    { (char *)"Edit Combo",         NULL,  NULL, 0, NULL },
    { (char *)"Open Combo Page",    NULL,  NULL, 0, NULL },
    { (char *)"Open Tile Page",     NULL,  NULL, 0, NULL },
    { (char *)"Combo Locations",    NULL,  NULL, 0, NULL },
    { (char *)"",                   NULL,  NULL, 0, NULL },
    { (char *)"Scroll to Page...",      NULL,  NULL, 0, NULL },
    { NULL,                         NULL,  NULL, 0, NULL }
};

static MENU fav_rc_menu[] =
{
    { (char *)"Scroll to Combo  ",       NULL,  NULL, 0, NULL },
    { (char *)"Edit Combo  ",         NULL,  NULL, 0, NULL },
    { (char *)"Remove Combo  ",          NULL,  NULL, 0, NULL },
    { NULL,                              NULL,  NULL, 0, NULL }
};

void set_brush_width(int width)
{
    for(int x=0; x<16; x++)
    {
        brush_width_menu[x].flags=0;
    }
    
    BrushWidth=width;
    brush_width_menu[width-1].flags=D_SELECTED;
    refresh(rALL);
}

void set_brush_height(int height)
{
    for(int x=0; x<11; x++)
    {
        brush_height_menu[x].flags=0;
    }
    
    BrushHeight=height;
    brush_height_menu[height-1].flags=D_SELECTED;
    refresh(rALL);
}

void restore_mouse()
{
    ComboBrushPause=1;
    Backend::mouse->setCursorSprite(mouse_bmp[MOUSE_BMP_NORMAL][0]);
}

static int comboa_cnt=0;
static int layer_cnt=0;

static DIALOG clist_dlg[] =
{
    // (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp)
    { jwin_win_proc,     60-12,   40,   200+24,  148,  vc(14),  vc(1),  0,       D_EXIT,          0,             0,       NULL, NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { jwin_abclist_proc,       72-12-4,   60+4,   176+24+8,  92+3,   jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,       D_EXIT,     0,             0,       NULL, NULL, NULL },
    { jwin_button_proc,     90,   163,  61,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "OK", NULL, NULL },
    { jwin_button_proc,     170,  163,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};


command_struct bic[cmdMAX];
int bic_cnt=-1;

void build_bic_list()
{
    int start=bic_cnt=0;
    
    for(int i=start; i<cmdMAX; i++)
    {
        if(commands[i].name[strlen(commands[i].name)-1]!=' ')
        {
            bic[bic_cnt].s = (char *)commands[i].name;
            bic[bic_cnt].i = i;
            ++bic_cnt;
        }
    }
    
    for(int i=start; i<bic_cnt; i++)
    {
        for(int j=i+1; j<bic_cnt; j++)
        {
            if(stricmp(bic[i].s,bic[j].s)>0 && strcmp(bic[j].s,""))
            {
                zc_swap(bic[i],bic[j]);
            }
        }
    }
}

const char *commandlist(int index, int *list_size)
{
    if(index<0)
    {
        *list_size = bic_cnt;
        return NULL;
    }
    
    return bic[index].s;
}

int select_command(const char *prompt,int cmd)
{
    if(bic_cnt==-1)
        build_bic_list();
        
    int index=0;
    
    for(int j=0; j<bic_cnt; j++)
    {
        if(bic[j].i == cmd)
        {
            index=j;
        }
    }
    
    clist_dlg[0].dp=(void *)prompt;
    clist_dlg[0].dp2=lfont;
    clist_dlg[2].d1=index;
    static ListData command_list(commandlist, &font);
    clist_dlg[2].dp=(void *) &command_list;

	DIALOG *clist_cpy = resizeDialog(clist_dlg, 1.5);
    
    int ret=zc_popup_dialog(clist_cpy,2);
    
    if(ret==0||ret==4)
    {
        Backend::mouse->setWheelPosition(0);
        return -1;
    }
    
    index = clist_cpy[2].d1;
	Backend::mouse->setWheelPosition(0);

	delete[] clist_cpy;

    return bic[index].i;
}


int onCommand(int cmd)
{
    restore_mouse();
    build_bic_list();
    int ret=select_command("Select Command",cmd);
    refresh(rALL);
    
    if(ret>=0)
    {
        saved=false;
    }
    else if(ret == -1)
    {
        return cmd;
    }
    
    return ret;
}

int onEditFFCombo(int);

static char paste_ffc_menu_text[21];
static char paste_ffc_menu_text2[21];
static char follow_warp_menu_text[21];
static char follow_warp_menu_text2[21];

void domouse()
{
    static bool mouse_down = false;
    static int scrolldelay = 0;
    int x= Backend::mouse->getVirtualScreenX();
	int y = Backend::mouse->getVirtualScreenY();
    double startx=mapscreen_x()+(showedges()?(16*mapscreensize()):0);
    double starty=mapscreen_y()+(showedges()?(16*mapscreensize()):0);
    int startxint=mapscreen_x()+(showedges()?int(16*mapscreensize()):0);
    int startyint=mapscreen_y()+(showedges()?int(16*mapscreensize()):0);
    int cx=(x-startxint)/int(16*mapscreensize());
    int cy=(y-startyint)/int(16*mapscreensize());
    int c=(cy*16)+cx;
    mousecomboposition=c;
    
    int redraw=0;
    
    update_combobrush();
    //  put_combo(brushbmp,0,0,Combo,CSet,0,0);
    
    if(!isinRect(x,y,tooltip_trigger.x,tooltip_trigger.y,tooltip_trigger.x+tooltip_trigger.w-1,tooltip_trigger.y+tooltip_trigger.h-1))
    {
        clear_tooltip();
    }
    
    // For some reason, this causes an invisible cursor in a windowed ZQuest...
    /*if(!isinRect(x,y,startxint,startyint,int(startx+(256*mapscreensize)-1),int(starty+(176*mapscreensize)-1)))
    {
      restore_mouse();
    }*/
    
    ++scrolldelay;
    
    if(MouseScroll &&
            (
                ((x>=combolist(0).x) && (x<combolist(0).x+(16*combolist(0).w)))||
                ((x>=combolist(1).x) && (x<combolist(1).x+(16*combolist(1).w)))||
                ((x>=combolist(2).x) && (x<combolist(2).x+(16*combolist(2).w)))
            ) && (key[KEY_LSHIFT] || key[KEY_RSHIFT] || (scrolldelay&3)==0))
    {
    
        int test_list=0;
        
        for(test_list=0; test_list<3; ++test_list)
        {
            if((x>=combolist(test_list).x) && (x<combolist(test_list).x+(16*combolist(test_list).w)))
            {
                break;
            }
        }
        
        if(test_list<3)
        {
            if(y>=combolist(test_list).y-mouse_scroll_h() && y<=combolist(test_list).y && First[test_list])
            {
                if((key[KEY_ZC_LCONTROL] || key[KEY_ZC_RCONTROL])&&(key[KEY_ALT] || key[KEY_ALTGR]))
                {
                    First[test_list]=0;
                }
                else if(key[KEY_ZC_LCONTROL] || key[KEY_ZC_RCONTROL])
                {
                    First[test_list]-=zc_min(First[test_list],256);
                }
                else if(key[KEY_ALT] || key[KEY_ALTGR])
                {
                    First[test_list]-=zc_min(First[test_list],(combolist(test_list).w*combolist(test_list).h));
                }
                else
                {
                    First[test_list]-=zc_min(First[test_list],combolist(test_list).w);
                }
                
                redraw|=rCOMBOS;
            }
            
            if(y>=combolist(test_list).y+(combolist(test_list).h*16)-1 && y<combolist(test_list).y+(combolist(test_list).h*16)+mouse_scroll_h()-1 && First[test_list]<(MAXCOMBOS-(combolist(test_list).w*combolist(test_list).h)))
            {
                int offset = combolist(test_list).w*combolist(test_list).h;
                
                if((key[KEY_ZC_LCONTROL] || key[KEY_ZC_RCONTROL])&&(key[KEY_ALT] || key[KEY_ALTGR]))
                {
                    First[test_list]=MAXCOMBOS-offset;
                }
                else if(key[KEY_ZC_LCONTROL] || key[KEY_ZC_RCONTROL])
                {
                    First[test_list] = zc_min(MAXCOMBOS-offset, First[test_list]+256);
                }
                else if(key[KEY_ALT] || key[KEY_ALTGR])
                {
                    First[test_list] = zc_min(MAXCOMBOS-offset, First[test_list]+ offset);
                }
                else
                {
                    First[test_list] = zc_min(MAXCOMBOS - offset, First[test_list] + combolist(test_list).w);
                }
                
                redraw|=rCOMBOS;
            }
        }
    }
    
//-------------
//tooltip stuff
//-------------
    if(isinRect(x,y,startxint,startyint,int(startx+(256*mapscreensize())-1),int(starty+(176*mapscreensize())-1)))
    {
        for(int i=MAXFFCS-1; i>=0; i--)
            if(Map.CurrScr()->ffdata[i]!=0 && (CurrentLayer<2 || (Map.CurrScr()->ffflags[i]&ffOVERLAY)))
            {
                int ffx = int(Map.CurrScr()->ffx[i]/10000.0);
                int ffy = int(Map.CurrScr()->ffy[i]/10000.0);
                int cx2 = (x-startxint)/mapscreensize();
                int cy2 = (y-startyint)/mapscreensize();
                
                if(cx2 >= ffx && cx2 < ffx+((1+(Map.CurrScr()->ffwidth[i]>>6))*16) && cy2 >= ffy && cy2 < ffy+((1+(Map.CurrScr()->ffheight[i]>>6))*16))
                {
                    // FFC tooltip
                    if(tooltip_current_ffc != i)
                    {
                        clear_tooltip();
                    }
                    
                    tooltip_current_ffc = i;
                    char msg[288];
                    sprintf(msg,"FFC: %d Combo: %d\nCSet: %d Type: %s\nScript: %s",
                            i+1, Map.CurrScr()->ffdata[i],Map.CurrScr()->ffcset[i],
                            combo_class_buf[combobuf[Map.CurrScr()->ffdata[i]].type].name,
                            Map.CurrScr()->ffscript[i]<=0 ? "(None)" : ffcmap[Map.CurrScr()->ffscript[i]-1].second.c_str());
                    update_tooltip(x, y, startxint, startyint, int(256*mapscreensize()),int(176*mapscreensize()), msg);
                    break;
                }
            }
            
        int drawmap;
        int drawscr;
        
        if(CurrentLayer==0)
        {
            drawmap=Map.getCurrMap();
            drawscr=Map.getCurrScr();
        }
        else
        {
            drawmap=Map.CurrScr()->layermap[CurrentLayer-1]-1;
            drawscr=Map.CurrScr()->layerscreen[CurrentLayer-1];
            
            if(drawmap<0)
            {
            }
        }
        
        if(tooltip_current_combo != c)
        {
            clear_tooltip();
        }
        
        tooltip_current_combo = c;
        char msg[288];
        memset(msg, 0, 256);
        sprintf(msg,"Pos: %d Combo: %d\nCSet: %d Flags: %d, %d\nCombo type: %s",
                c, Map.AbsoluteScr(drawmap, drawscr)->data[c],
                Map.AbsoluteScr(drawmap, drawscr)->cset[c], Map.CurrScr()->sflag[c],combobuf[Map.CurrScr()->data[c]].flag,
                combo_class_buf[combobuf[(Map.AbsoluteScr(drawmap, drawscr)->data[c])].type].name);
        update_tooltip(x, y, startxint, startyint, int(256*mapscreensize()),int(176*mapscreensize()), msg);
    }
    
    if(is_large())
    {
        for(int j=0; j<4; j++)
        {
            int xx = panel(8).x+14+(32*j);
            int yy = panel(8).y+12;
            
            if(isinRect(x,y,xx,yy,xx+20,yy+20))
            {
                char msg[160];
                sprintf(msg,
                        j==0 ? "Item Location" :
                        j==1 ? "Stairs Secret\nTriggered when a Trigger Push Block is pushed." :
                        j==2 ? "Arrival Square\nLink's location when he begins/resumes the game." :
                        "Combo Flags");
                update_tooltip(x,y,xx,yy,20,20,msg);
            }
        }
        
        // Warp Returns
        for(int j=0; j<4; j++)
        {
            int xx = panel(8).x+14+(32*j);
            int yy = panel(8).y+54;
            
            if(isinRect(x,y,xx,yy,xx+20,yy+20))
            {
                char msg[160];
                sprintf(msg,"Warp Return Square %c\nLink's destination after warping to this screen.",(char)('A'+j));
                update_tooltip(x,y,xx,yy,20,20,msg);
            }
        }
        
        // Enemies
        int epx = 2+panel(8).x+14+4*32;
        int epy = 2+panel(8).y+12;
        
        if(isinRect(x,y,epx,epy,epx+16*4+4,epy+16*3+4))
        {
            char msg[160];
            sprintf(msg,"Enemies that appear on this screen.");
            update_tooltip(x,y,epx,epy,16*4+4,16*3+4,msg);
        }
    }
    
    if(draw_mode!=dm_alias)
    {
        for(int j=0; j<3; ++j)
        {
            if(j==0||is_large())
            {
                if(isinRect(x,y,combolist(j).x,combolist(j).y,combolist(j).x+(combolist(j).w*16)-1,combolist(j).y+(combolist(j).h*16)-1))
                {
                    int cc=((x-combolist(j).x)>>4);
                    int cr=((y-combolist(j).y)>>4);
                    int c2=(cr*combolist(j).w)+cc+First[j];
                    char msg[160];
                    
                    if(combobuf[c2].flag != 0)
                        sprintf(msg, "Combo %d: %s\nInherent flag:%s", c2, combo_class_buf[combobuf[c2].type].name, flag_string[combobuf[c2].flag]);
                    else
                        sprintf(msg, "Combo %d: %s", c2, combo_class_buf[combobuf[c2].type].name);
                        
                    update_tooltip(x,y,combolist(j).x+(cc<<4),combolist(j).y+(cr<<4),16,16, msg);
                }
            }
        }
    }
    else
    {
        for(int j=0; j<3; ++j)
        {
            if(j==0||is_large())
            {
                if(isinRect(x,y,comboaliaslist(j).x,comboaliaslist(j).y,comboaliaslist(j).x+(comboaliaslist(j).w*16)-1,comboaliaslist(j).y+(comboaliaslist(j).h*16)-1))
                {
                    int cc=((x-comboaliaslist(j).x)>>4);
                    int cr=((y-comboaliaslist(j).y)>>4);
                    int c2=(cr*comboaliaslist(j).w)+cc+combo_alistpos[j];
                    char msg[80];
                    sprintf(msg, "Combo alias %d", c2);
                    update_tooltip(x,y,comboaliaslist(j).x+(cc<<4),comboaliaslist(j).y+(cr<<4),16,16, msg);
                }
            }
        }
    }
    
    // Mouse clicking stuff
    if(!Backend::mouse->anyButtonClicked())
    {
        mouse_down = false;
        canfill=true;
    }
    else if(Backend::mouse->leftButtonClicked())
    {
        //on the map screen
        if(isinRect(x,y,startxint,startyint,int(startx+(256*mapscreensize())-1),int(starty+(176*mapscreensize())-1)))
        {
            int cx2 = (x-startxint)/mapscreensize();
            int cy2 = (y-startyint)/mapscreensize();
            
            // Move items
            if(Map.CurrScr()->hasitem)
            {
                int ix = Map.CurrScr()->itemx;
                int iy = Map.CurrScr()->itemy;
                
                if(cx2 >= ix && cx2 < ix+16 && cy2 >= iy && cy2 < iy+16)
                    doxypos(Map.CurrScr()->itemx,Map.CurrScr()->itemy,11,0xF8,true,0,0,16,16);
            }
            
            // Move FFCs
            for(int i=MAXFFCS-1; i>=0; i--)
                if(Map.CurrScr()->ffdata[i]!=0 && (CurrentLayer<2 || (Map.CurrScr()->ffflags[i]&ffOVERLAY)))
                {
                    int ffx = int(Map.CurrScr()->ffx[i]/10000.0);
                    int ffy = int(Map.CurrScr()->ffy[i]/10000.0);
                    
                    if(cx2 >= ffx && cx2 < ffx+((1+(Map.CurrScr()->ffwidth[i]>>6))*16) && cy2 >= ffy && cy2 < ffy+((1+(Map.CurrScr()->ffheight[i]>>6))*16))
                    {
                        moveffc(i,cx2,cy2);
                        break;
                    }
                }
            
            if(key[KEY_ALT]||key[KEY_ALTGR])
            {
                int drawmap, drawscr;
                if(CurrentLayer==0)
                {
                    drawmap=Map.getCurrMap();
                    drawscr=Map.getCurrScr();
                }
                else
                {
                    drawmap=Map.CurrScr()->layermap[CurrentLayer-1]-1;
                    drawscr=Map.CurrScr()->layerscreen[CurrentLayer-1];
                }
                if(drawmap<0)
                    return;
                
                Combo=Map.AbsoluteScr(drawmap, drawscr)->data[c];
                if(key[KEY_LSHIFT]||key[KEY_RSHIFT])
                    CSet=Map.AbsoluteScr(drawmap, drawscr)->cset[c];
                if(key[KEY_ZC_LCONTROL] || key[KEY_ZC_RCONTROL])
                    First[current_combolist]=vbound(
                      (Map.AbsoluteScr(drawmap, drawscr)->data[c]/combolist(0).w*combolist(0).w)-(combolist(0).w*combolist(0).h/2),
                      0,
                      MAXCOMBOS-(combolist(0).w*combolist(0).h));
            }
            else if(key[KEY_ZC_LCONTROL] || key[KEY_ZC_RCONTROL])
            {
                if(canfill)
                {
                    switch(fill_type)
                    {
                    case 0:
                        flood();
                        break;
                        
                    case 1:
                        fill_4();
                        break;
                        
                    case 2:
                        fill_8();
                        break;
                        
                    case 3:
                        fill2_4();
                        break;
                        
                    case 4:
                        fill2_8();
                        break;
                    }
                    
                    canfill=false;
                }
            }
            else draw(key[KEY_LSHIFT] || key[KEY_RSHIFT]);
        }
        
        //on the map tabs
        if(is_large())
        {
            for(int btn=0; btn<(showedges()?9:8); ++btn)
            {
                char tbuf[10];
                sprintf(tbuf, "%d:%02X", map_page[btn].map+1, map_page[btn].screen);
                
                if(isinRect(x,y,mapscreen_x()+(btn*16*2*mapscreensize()),mapscreen_y()+((showedges()?13:11)*16*mapscreensize()),mapscreen_x()+(btn*16*2*mapscreensize())+map_page_bar(btn).w,mapscreen_y()+((showedges()?13:11)*16*mapscreensize())+map_page_bar(btn).h))
                {
                    if(do_text_button(map_page_bar(btn).x,map_page_bar(btn).y,map_page_bar(btn).w,map_page_bar(btn).h,tbuf,vc(1),vc(14),true))
                    {
                        map_page[current_mappage].map=Map.getCurrMap();
                        map_page[current_mappage].screen=Map.getCurrScr();
                        current_mappage=btn;
                        Map.setCurrMap(map_page[current_mappage].map);
                        Map.setCurrScr(map_page[current_mappage].screen);
                    }
                }
            }
        }
        
        //on the drawing mode button
        if(is_large())
        {
            if(isinRect(x,y,combolist_window().x-64,0,combolist_window().x-1,15))
            {
                if(do_text_button(combolist_window().x-64,0,64,16,dm_names[draw_mode],vc(1),vc(14),true))
                    onDrawingMode();
            }
        }
        
        //on the minimap()
        if(isinRect(x,y,minimap().x+3,minimap().y+12,minimap().x+minimap().w-5,minimap().y+minimap().h-4))
        {
            select_scr();
        }
        
        if(is_large())
        {
            if(isinRect(x,y,panel(8).x+16,panel(8).y+14,panel(8).x+16+15,panel(0).y+14+15))
            {
                onItem();
                
                if(Map.CurrScr()->hasitem)
                    doxypos(Map.CurrScr()->itemx,Map.CurrScr()->itemy,11,0xF8);
            }
            
            if(isinRect(x,y,panel(8).x+16+32,panel(8).y+14,panel(8).x+16+32+15,panel(0).y+14+15))
            {
                doxypos(Map.CurrScr()->stairx,Map.CurrScr()->stairy,14,0xF0);
            }
            
            if(isinRect(x,y,panel(8).x+16+(32*2),panel(8).y+14,panel(8).x+16+(32*2)+15,panel(8).y+14+15))
            {
                if(get_bit(quest_rules,qr_NOARRIVALPOINT))
                    jwin_alert("Obsolete Square","The arrival square is obsolete if you use the recommended",
                               "quest rule, 'Use Warp Return Points Only' It is included",
                               "only for backwards-compatibility purposes.","O&K",NULL,'k',0,lfont);
                               
                doxypos(Map.CurrScr()->warparrivalx,Map.CurrScr()->warparrivaly,10,0xF8);
            }
            
            if(isinRect(x,y,panel(8).x+16+(32*3),panel(8).y+14,panel(8).x+16+(32*3)+15,panel(8).y+14+15))
            {
                onFlags();
            }
            
            if(isinRect(x,y,panel(8).x+16,panel(8).y+56,panel(8).x+16+15,panel(8).y+56+15))
            {
                doxypos(Map.CurrScr()->warpreturnx[0],Map.CurrScr()->warpreturny[0],9,0xF8);
            }
            
            if(isinRect(x,y,panel(8).x+16+32,panel(8).y+56,panel(8).x+16+32+15,panel(8).y+56+15))
            {
                doxypos(Map.CurrScr()->warpreturnx[1],Map.CurrScr()->warpreturny[1],9,0xF8);
            }
            
            if(isinRect(x,y,panel(8).x+16+(32*2),panel(8).y+56,panel(8).x+16+(32*2)+15,panel(8).y+56+15))
            {
                doxypos(Map.CurrScr()->warpreturnx[2],Map.CurrScr()->warpreturny[2],9,0xF8);
            }
            
            if(isinRect(x,y,panel(8).x+16+(32*3),panel(8).y+56,panel(8).x+16+(32*3)+15,panel(8).y+56+15))
            {
                doxypos(Map.CurrScr()->warpreturnx[3],Map.CurrScr()->warpreturny[3],9,0xF8);
            }
            
            int epx = 2+panel(8).x+14+4*32;
            int epy = 2+panel(8).y+12;
            
            if(isinRect(x,y,epx,epy,epx+16*4,epy+16*3))
            {
                onEnemies();
            }
        }
        else
        {
            if(menutype==m_coords)
            {
                if(isinRect(x,y,panel(0).x+16,panel(0).y+6,panel(0).x+16+15,panel(0).y+6+15))
                {
                    onItem();
                    
                    if(Map.CurrScr()->hasitem)
                        doxypos(Map.CurrScr()->itemx,Map.CurrScr()->itemy,11,0xF8);
                }
                
                if(isinRect(x,y,panel(0).x+48,panel(0).y+6,panel(0).x+48+15,panel(0).y+6+15))
                {
                    doxypos(Map.CurrScr()->stairx,Map.CurrScr()->stairy,14,0xF0);
                }
                
                if(isinRect(x,y,panel(0).x+80,panel(0).y+6,panel(0).x+80+15,panel(0).y+6+15))
                {
                    if(get_bit(quest_rules,qr_NOARRIVALPOINT))
                        jwin_alert("Obsolete Square","The arrival square is obsolete if you use the",
                                   "'Use Warp Return Points Only' quest rule. It is included",
                                   "only for backwards-compatibility purposes.","O&K",NULL,'k',0,lfont);
                                   
                    doxypos(Map.CurrScr()->warparrivalx,Map.CurrScr()->warparrivaly,10,0xF8);
                }
                
                if(isinRect(x,y,panel(0).x+112,panel(0).y+6,panel(0).x+112+15,panel(0).y+6+15))
                {
                    onFlags();
                }
            }
            
            if(menutype==m_coords2)
            {
                if(isinRect(x,y,panel(7).x+16,panel(7).y+6,panel(7).x+16+15,panel(7).y+6+15))
                {
                    doxypos(Map.CurrScr()->warpreturnx[0],Map.CurrScr()->warpreturny[0],9,0xF8);
                }
                
                if(isinRect(x,y,panel(7).x+48,panel(7).y+6,panel(7).x+48+15,panel(7).y+6+15))
                {
                    doxypos(Map.CurrScr()->warpreturnx[1],Map.CurrScr()->warpreturny[1],9,0xF8);
                }
                
                if(isinRect(x,y,panel(7).x+80,panel(7).y+6,panel(7).x+80+15,panel(7).y+6+15))
                {
                    doxypos(Map.CurrScr()->warpreturnx[2],Map.CurrScr()->warpreturny[2],9,0xF8);
                }
                
                if(isinRect(x,y,panel(7).x+112,panel(7).y+6,panel(7).x+112+15,panel(7).y+6+15))
                {
                    doxypos(Map.CurrScr()->warpreturnx[3],Map.CurrScr()->warpreturny[3],9,0xF8);
                }
            }
            else if(menutype==m_layers)
            {
                if(isinRect(x, y, panel(6).x+9,panel(6).y+20,panel(6).x+9+8,panel(6).y+20+8))
                {
                    do_checkbox(menu1,panel(6).x+9,panel(6).y+20,vc(1),vc(14), LayerMaskInt[0]);
                }
                
                if(isinRect(x, y, panel(6).x+34,panel(6).y+20,panel(6).x+34+8,panel(6).y+20+8))
                {
                    do_checkbox(menu1,panel(6).x+34,panel(6).y+20,vc(1),vc(14), LayerMaskInt[1]);
                }
                
                if(isinRect(x, y, panel(6).x+59,panel(6).y+20,panel(6).x+59+8,panel(6).y+20+8))
                {
                    do_checkbox(menu1,panel(6).x+59,panel(6).y+20,vc(1),vc(14), LayerMaskInt[2]);
                }
                
                if(isinRect(x, y, panel(6).x+84,panel(6).y+20,panel(6).x+84+8,panel(6).y+20+8))
                {
                    do_checkbox(menu1,panel(6).x+84,panel(6).y+20,vc(1),vc(14), LayerMaskInt[3]);
                }
                
                if(isinRect(x, y, panel(6).x+109,panel(6).y+20,panel(6).x+109+8,panel(6).y+20+8))
                {
                    do_checkbox(menu1,panel(6).x+109,panel(6).y+20,vc(1),vc(14), LayerMaskInt[4]);
                }
                
                if(isinRect(x, y, panel(6).x+134,panel(6).y+20,panel(6).x+134+8,panel(6).y+20+8))
                {
                    do_checkbox(menu1,panel(6).x+134,panel(6).y+20,vc(1),vc(14), LayerMaskInt[5]);
                }
                
                if(isinRect(x, y, panel(6).x+159,panel(6).y+20,panel(6).x+159+8,panel(6).y+20+8))
                {
                    do_checkbox(menu1,panel(6).x+159,panel(6).y+20,vc(1),vc(14), LayerMaskInt[6]);
                }
                
                if(isinRect(x, y, panel(6).x+9,panel(6).y+30, panel(6).x+9+(6*25)+8,panel(6).y+30+8))
                {
                    do_layerradio(menu1,panel(6).x+9,panel(6).y+30,vc(1),vc(14), CurrentLayer);
                }
                
                redraw|=rMENU;
            }
        }
        
        if(draw_mode!=dm_alias)
        {
            for(int temp_counter=0; temp_counter<3; ++temp_counter)
            {
                int temp_x1=combolistscrollers(temp_counter).x;
                int temp_y1=combolistscrollers(temp_counter).y;
                int temp_x2=combolistscrollers(temp_counter).x+combolistscrollers(temp_counter).w-1;
                int temp_y2=combolistscrollers(temp_counter).y+combolistscrollers(temp_counter).h-2;
                
                int temp_x3=combolistscrollers(temp_counter).x;
                int temp_y3=combolistscrollers(temp_counter).y+combolistscrollers(temp_counter).h-1;
                int temp_x4=combolistscrollers(temp_counter).x+combolistscrollers(temp_counter).w-1;
                int temp_y4=combolistscrollers(temp_counter).y+combolistscrollers(temp_counter).h*2-3;
                
                if(is_large())
                {
                    temp_x1=combolistscrollers(temp_counter).x;
                    temp_y1=combolistscrollers(temp_counter).y;
                    temp_x2=combolistscrollers(temp_counter).x+combolistscrollers(temp_counter).w-1;
                    temp_y2=combolistscrollers(temp_counter).y+combolistscrollers(temp_counter).h-1;
                    
                    temp_x3=combolistscrollers(temp_counter).x+combolistscrollers(temp_counter).w;
                    temp_y3=combolistscrollers(temp_counter).y;
                    temp_x4=combolistscrollers(temp_counter).x+combolistscrollers(temp_counter).w*2-1;
                    temp_y4=combolistscrollers(temp_counter).y+combolistscrollers(temp_counter).h-1;
                }
                
                if(isinRect(x,y,temp_x1,temp_y1,temp_x2,temp_y2) && First[temp_counter]>0 && !mouse_down)
                {
                    if(key[KEY_ZC_LCONTROL] || key[KEY_ZC_RCONTROL])
                    {
                        First[temp_counter]-=zc_min(First[temp_counter],256);
                    }
                    else
                    {
                        First[temp_counter]-=zc_min(First[temp_counter],(combolist(0).w*combolist(0).h));
                    }
                    
                    redraw|=rCOMBOS;
                }
                else if(isinRect(x,y,temp_x3,temp_y3,temp_x4,temp_y4) && First[temp_counter]<(MAXCOMBOS-(combolist(0).w*combolist(0).h)) && !mouse_down)
                {
                    if(key[KEY_ZC_LCONTROL] || key[KEY_ZC_RCONTROL])
                    {
                        First[temp_counter]+=zc_min((MAXCOMBOS-256)-First[temp_counter],256);
                    }
                    else
                    {
                        First[temp_counter]+=zc_min((MAXCOMBOS-(combolist(0).w*combolist(0).h))-First[temp_counter],(combolist(0).w*combolist(0).h));
                    }
                    
                    redraw|=rCOMBOS;
                }
            }
            
            if((isinRect(x,y,panel(0).x+panel(0).w-28,panel(0).y+32,panel(0).x+panel(0).w-28+24,panel(0).y+32+5) && menutype==m_block && !mouse_down) ||
                    (isinRect(x,y,panel(6).x+panel(6).w-28,panel(6).y+36,panel(6).x+panel(6).w-28+24,panel(6).y+36+5) && menutype==m_layers && !mouse_down))
            {
                CSet=wrap(CSet+1,0,11);
                refresh(rCOMBOS+rMENU+rCOMBO);
            }
            
            if(isinRect(x,y,panel(0).x+panel(0).w-32,panel(0).y+39,panel(0).x+panel(0).w-32+28,panel(0).y+39+5) && menutype==m_block && !mouse_down)
            {
                bool validlayer=false;
                
                while(!validlayer)
                {
                    CurrentLayer=wrap(CurrentLayer+1,0,6);
                    
                    if((CurrentLayer==0)||(Map.CurrScr()->layermap[CurrentLayer-1]))
                    {
                        validlayer=true;
                    }
                }
                
                refresh(rMENU);
            }
            
            for(int j=0; j<3; ++j)
            {
                if(j==0||is_large())
                {
                    if(isinRect(x,y,combolist(j).x,combolist(j).y,combolist(j).x+(combolist(j).w*16)-1,combolist(j).y+(combolist(j).h*16)-1))
                    {
                        select_combo(j);
                    }
                }
            }
        }
        else
        {
            for(int j=0; j<3; ++j)
            {
                if(j==0||is_large())
                {
                    if(isinRect(x,y,comboaliaslist(j).x,comboaliaslist(j).y,comboaliaslist(j).x+(comboaliaslist(j).w*16)-1,comboaliaslist(j).y+(comboaliaslist(j).h*16)-1))
                    {
                        select_comboa(j);
                    }
                }
            }
        }
        
        //on the favorites list
        if(isinRect(x,y,favorites_list().x,favorites_list().y,favorites_list().x+(favorites_list().w*16)-1,favorites_list().y+(favorites_list().h*16)-1))
        {
            int row=vbound(((y-favorites_list().y)>>4),0,favorites_list().h-1);
            int col=vbound(((x-favorites_list().x)>>4),0,favorites_list().w-1);
            int f=(row*favorites_list().w)+col;
            
            if(key[KEY_LSHIFT] || key[KEY_RSHIFT] ||
               (draw_mode==dm_alias?favorite_comboaliases:favorite_combos)[f]==-1)
            {
                int tempcb=ComboBrush;
                ComboBrush=0;
                
                while(Backend::mouse->anyButtonClicked())
                {
                    x= Backend::mouse->getVirtualScreenX();
                    y= Backend::mouse->getVirtualScreenY();
                    
                    if(draw_mode != dm_alias)
                    {
                        if(favorite_combos[f]!=Combo)
                        {
                            favorite_combos[f]=Combo;
                            saved=false;
                        }
                    }
                    else
                    {
                        if(favorite_comboaliases[f]!=combo_apos)
                        {
                            favorite_comboaliases[f]=combo_apos;
                            saved=false;
                        }
                    }
                    
                    do_animations();
                    refresh(rALL | rFAVORITES);                    
                }
                
                ComboBrush=tempcb;
            }
            else if(key[KEY_ZC_LCONTROL] || key[KEY_ZC_RCONTROL])
            {
                int tempcb=ComboBrush;
                ComboBrush=0;
                
                while(Backend::mouse->anyButtonClicked())
                {
                    x= Backend::mouse->getVirtualScreenX();
                    y= Backend::mouse->getVirtualScreenY();
                    
                    if(draw_mode != dm_alias)
                    {
                        if(favorite_combos[f]!=-1)
                        {
                            favorite_combos[f]=-1;
                            saved=false;
                        }
                    }
                    else
                    {
                        if(favorite_comboaliases[f]!=-1)
                        {
                            favorite_comboaliases[f]=-1;
                            saved=false;
                        }
                    }
                    
                    do_animations();
                    refresh(rALL | rFAVORITES);
                    //if(zqwin_scale > 1)
                    {
                        //stretch_blit(screen, hw_screen, 0, 0, screen->w, screen->h, 0, 0, hw_screen->w, hw_screen->h);
                    }
                    //else
                    {
                        //blit(screen, hw_screen, 0, 0, 0, 0, screen->w, screen->h);
                    }
                }
                
                ComboBrush=tempcb;
            }
            else
            {
                select_favorite();
            }
        }
        
        //on the commands buttons
        if(is_large() /*&& rALL&rCOMMANDS*/) //do we really need to check that?
        {
            for(int cmd=0; cmd<(commands_list().w*commands_list().h); ++cmd)
            {
                int check_x=(cmd%commands_list().w)*command_buttonwidth+commands_list().x;
                int check_y=(cmd/commands_list().w)*command_buttonheight+commands_list().y;
                bool shift=(key[KEY_LSHIFT] || key[KEY_RSHIFT]);
                bool ctrl=(key[KEY_ZC_LCONTROL] || key[KEY_ZC_RCONTROL]);
                
                if(isinRect(x,y,check_x,check_y,check_x+command_buttonwidth,check_y+command_buttonheight)&&(commands[favorite_commands[cmd]].flags!=D_DISABLED||(shift||ctrl)))
                {
                    FONT *tfont=font;
                    font=pfont;
                    
                    if(do_text_button_reset(check_x,check_y,command_buttonwidth, command_buttonheight, favorite_commands[cmd]==cmdCatchall&&strcmp(catchall_string[Map.CurrScr()->room]," ")?catchall_string[Map.CurrScr()->room]:commands[favorite_commands[cmd]].name,vc(1),vc(14),true))
                    {
                        if(ctrl)
                        {
                            favorite_commands[cmd]=0;
                        }
                        else if(shift || favorite_commands[cmd]==0)
                        {
                            favorite_commands[cmd]=onCommand(favorite_commands[cmd]);
                        }
                        else
                        {
                            font=tfont;
                            int (*pfun)();
                            pfun=commands[favorite_commands[cmd]].command;
                            pfun();
                        }
                    }
                    
                    font=tfont;
                }
            }
        }
        
        // On the layer panel
        if(is_large())
        {
            for(int i=0; i<=6; ++i)
            {
                int rx = (i * (layerpanel_buttonwidth+23)) + layer_panel().x+6;
                int ry = layer_panel().y+16;
                
                if((i==0 || Map.CurrScr()->layermap[i-1]) && isinRect(x,y,rx,ry,rx+layerpanel_buttonwidth-1,ry+layerpanel_buttonheight-1))
                {
                    char tbuf[15];
                    
                    if(i!=0 && Map.CurrScr()->layermap[i-1])
                    {
                        sprintf(tbuf, "%s%d (%d:%02X)",
                                (i==2 && Map.CurrScr()->flags7&fLAYER2BG) || (i==3 && Map.CurrScr()->flags7&fLAYER3BG) ? "-":"",
                                i, Map.CurrScr()->layermap[i-1], Map.CurrScr()->layerscreen[i-1]);
                    }
                    else
                    {
                        sprintf(tbuf, "%d", i);
                    }
                    
                    if(do_text_button(rx, ry, layerpanel_buttonwidth, layerpanel_buttonheight, tbuf,vc(1),vc(14),true))
                    {
                        CurrentLayer = i;
                    }
                }
                
                if(isinRect(x,y,rx+layerpanel_buttonwidth+4,ry+2,rx+layerpanel_buttonwidth+12,ry+10))
                    do_checkbox(menu1,rx+layerpanel_buttonwidth+4,ry+2,vc(1),vc(14), LayerMaskInt[i]);
            }
        }
        
        mouse_down = true;
    }
    else if(Backend::mouse->rightButtonClicked())
    {
        if(isinRect(x,y,startxint,startyint, int(startx+(256*mapscreensize())-1), int(starty+(176*mapscreensize())-1)))
        {
            ComboBrushPause=1;
            refresh(rMAP);
            restore_mouse();
            ComboBrushPause=0;
            
            bool clickedffc = false;
            int earliestfreeffc = MAXFFCS;
            
            // FFC right-click menu
            // This loop also serves to find the free ffc with the smallest slot number.
            for(int i=MAXFFCS-1; i>=0; i--)
            {
                if(Map.CurrScr()->ffdata[i]==0 && i < earliestfreeffc)
                    earliestfreeffc = i;
                    
                if(clickedffc || !(Map.CurrScr()->valid&mVALID))
                    continue;
                    
                if(Map.CurrScr()->ffdata[i]!=0 && (CurrentLayer<2 || (Map.CurrScr()->ffflags[i]&ffOVERLAY)))
                {
                    int ffx = int(Map.CurrScr()->ffx[i]/10000.0);
                    int ffy = int(Map.CurrScr()->ffy[i]/10000.0);
                    int cx2 = (x-startxint)/mapscreensize();
                    int cy2 = (y-startyint)/mapscreensize();
                    
                    if(cx2 >= ffx && cx2 < ffx+((1+(Map.CurrScr()->ffwidth[i]>>6))*16) && cy2 >= ffy && cy2 < ffy+((1+(Map.CurrScr()->ffheight[i]>>6))*16))
                    {
                        draw_ffc_rc_menu[1].flags = (Map.getCopyFFC()>-1) ? 0 : D_DISABLED;
                        
                        int m = popup_menu(draw_ffc_rc_menu,x,y);
                        
                        switch(m)
                        {
                        case 0:
                            Map.CopyFFC(i);
                            break;
                            
                        case 1: // Paste Copied FFC
                        {
                            if(jwin_alert("Confirm Paste","Really replace the FFC with","the data of the copied FFC?",NULL,"&Yes","&No",'y','n',lfont)==1)
                            {
                                Map.PasteOneFFC(i);
                                saved=false;
                            }
                        }
                        break;
                        
                        case 2:
                            onEditFFCombo(i);
                            break;
                            
                        case 3:
                            if(jwin_alert("Confirm Clear","Really clear this Freeform Combo?",NULL,NULL,"&Yes","&No",'y','n',lfont)==1)
                            {
                                Map.CurrScr()->ffdata[i] = Map.CurrScr()->ffcset[i] = Map.CurrScr()->ffx[i] = Map.CurrScr()->ffy[i] = Map.CurrScr()->ffxdelta[i] =
                                                               Map.CurrScr()->ffydelta[i] = Map.CurrScr()->ffxdelta2[i] = Map.CurrScr()->ffydelta2[i] = Map.CurrScr()->ffflags[i] = Map.CurrScr()->ffscript[i] =
                                                                       Map.CurrScr()->fflink[i] = Map.CurrScr()->ffdelay[i] = 0;
                                Map.CurrScr()->ffwidth[i] = Map.CurrScr()->ffheight[i] = 15;
                                
                                for(int j=0; j<8; j++)
                                    Map.CurrScr()->initd[i][j] = 0;
                                    
                                for(int j=0; j<2; j++)
                                    Map.CurrScr()->inita[i][j] = 10000;
                                    
                                saved = false;
                            }
                            
                            break;
                        }
                        
                        clickedffc = true;
                        break;
                    }
                }
                
            }
            
            // Combo right-click menu
            if(!clickedffc)
            {
                // FFC-specific options
                if(earliestfreeffc < MAXFFCS)
                {
                    sprintf(paste_ffc_menu_text, "Place + Edit FFC %d",earliestfreeffc+1);
                    sprintf(paste_ffc_menu_text2,"Paste FFC as FFC %d",earliestfreeffc+1);
                    draw_rc_menu[13].text = paste_ffc_menu_text;
                    draw_rc_menu[13].flags = 0;
                    
                    if(Map.getCopyFFC()>-1)
                    {
                        draw_rc_menu[14].text = paste_ffc_menu_text2;
                        draw_rc_menu[14].flags = 0;
                    }
                    else draw_rc_menu[14].flags = D_DISABLED;
                }
                else
                {
                    draw_rc_menu[13].text = (char*)"Place + Edit FFC";
                    draw_rc_menu[14].text = (char*)"Paste FFC";
                    draw_rc_menu[14].flags = draw_rc_menu[13].flags = D_DISABLED;
                }
                
                int warpindex = Map.warpindex(Map.AbsoluteScr(Map.getCurrMap(), Map.getCurrScr())->data[c]);
                
                if(warpindex > -1)
                {
                    sprintf(follow_warp_menu_text, "Follow Tile Warp %c",warpindex==4 ? 'R' : 'A'+warpindex);
                    sprintf(follow_warp_menu_text2,"Edit Tile Warp %c",warpindex==4 ? 'R' : 'A'+warpindex);
                    draw_rc_menu[10].text = follow_warp_menu_text;
                    draw_rc_menu[11].text = follow_warp_menu_text2;
                    draw_rc_menu[10].flags = draw_rc_menu[11].flags = 0;
                }
                else
                {
                    draw_rc_menu[10].text = (char*)"Follow Tile Warp";
                    draw_rc_menu[11].text = (char*)"Edit Tile Warp";
                    draw_rc_menu[11].flags = draw_rc_menu[10].flags = D_DISABLED;
                }
                
                int m = popup_menu(draw_rc_menu,x,y);
                
                switch(m)
                {
                case 0:
                case 1:
                {
                    int drawmap, drawscr;
                    
                    if(CurrentLayer==0)
                    {
                        drawmap=Map.getCurrMap();
                        drawscr=Map.getCurrScr();
                    }
                    else
                    {
                        drawmap=Map.CurrScr()->layermap[CurrentLayer-1]-1;
                        drawscr=Map.CurrScr()->layerscreen[CurrentLayer-1];
                        
                        if(drawmap<0)
                        {
                            return;
                        }
                    }
                    
                    if(m==0)
                    {
                        Combo=Map.AbsoluteScr(drawmap, drawscr)->data[c];
                    }
                    
                    if(m==1||(key[KEY_LSHIFT]||key[KEY_RSHIFT]))
                    {
                        First[current_combolist]=vbound((Map.AbsoluteScr(drawmap, drawscr)->data[c]/combolist(0).w*combolist(0).w)-(combolist(0).w*combolist(0).h/2),0,MAXCOMBOS-(combolist(0).w*combolist(0).h));
                    }
                }
                break;
                
                case 2:
                {
                    int drawmap, drawscr;
                    
                    if(CurrentLayer==0)
                    {
                    
                        drawmap=Map.getCurrMap();
                        drawscr=Map.getCurrScr();
                    }
                    else
                    {
                        drawmap=Map.CurrScr()->layermap[CurrentLayer-1]-1;
                        drawscr=Map.CurrScr()->layerscreen[CurrentLayer-1];
                        
                        if(drawmap<0)
                        {
                            return;
                        }
                    }
                    
                    edit_combo(Map.AbsoluteScr(drawmap, drawscr)->data[c],true,Map.AbsoluteScr(drawmap, drawscr)->cset[c]);
                }
                break;
                
                case 4:
                    replace(c);
                    break;
                    
                case 10: // Follow Tile Warp
                {
                    if(warpindex>=4)
                    {
                        jwin_alert("Random Tile Warp","This is a random tile warp combo, so it chooses","randomly between the screen's four Tile Warps.",NULL,"O&K",NULL,'k',0,lfont);
                        warpindex=rand()&3;
                    }
                    
                    int tm = Map.getCurrMap();
                    int ts = Map.getCurrScr();
                    int wt = Map.CurrScr()->tilewarptype[warpindex];
                    
                    if(wt==wtCAVE || wt==wtNOWARP)
                    {
                        char buf[56];
                        sprintf(buf,"This screen's Tile Warp %c is set to %s,",'A'+warpindex,warptype_string[wt]);
                        jwin_alert(warptype_string[wt],buf,"so it doesn't lead to another screen.",NULL,"O&K",NULL,'k',0,lfont);
                        break;
                        break;
                    }
                    
                    Map.dowarp(0,warpindex);
                    
                    if(ts!=Map.getCurrScr() || tm!=Map.getCurrMap())
                    {
                        FlashWarpSquare = (TheMaps[tm*MAPSCRS+ts].warpreturnc>>(warpindex*2))&3;
                        FlashWarpClk = 32;
                    }
                    
                    break;
                }
                
                case 11: // Edit Tile Warp
                {
                    if(warpindex>=4)
                    {
                        jwin_alert("Random Tile Warp","This is a random tile warp combo, so it chooses","randomly between the screen's four Tile Warps.",NULL,"O&K",NULL,'k',0,lfont);
                        warpindex=0;
                    }
                    
                    if(warpindex > -1 && warpindex < 4)
                        onTileWarpIndex(warpindex);
                        
                    break;
                }
                
                case 13:
                {
                    Map.CurrScr()->ffx[earliestfreeffc] = (((x-startxint)&(~0x000F))/mapscreensize())*10000;
                    Map.CurrScr()->ffy[earliestfreeffc] = (((y-startyint)&(~0x000F))/mapscreensize())*10000;
                    Map.CurrScr()->ffdata[earliestfreeffc] = Combo;
                    Map.CurrScr()->ffcset[earliestfreeffc] = CSet;
                    onEditFFCombo(earliestfreeffc);
                }
                break;
                
                case 14:
                {
                    Map.CurrScr()->ffx[earliestfreeffc] = (((x-startxint)&(~0x000F))/mapscreensize())*10000;
                    Map.CurrScr()->ffy[earliestfreeffc] = (((y-startyint)&(~0x000F))/mapscreensize())*10000;
                    Map.PasteOneFFC(earliestfreeffc);
                }
                break;
                
                default:
                    break;
                }
            }
        }
        
        for(int j=0; j<3; ++j)
        {
            if(j==0||is_large())
            {
                if(draw_mode != dm_alias)
                {
                    if(isinRect(x,y,combolist(j).x,combolist(j).y,combolist(j).x+(combolist(j).w*16)-1,combolist(j).y+(combolist(j).h*16)-1))
                    {
                        select_combo(j);
                        
                        if(isinRect(Backend::mouse->getVirtualScreenX(), Backend::mouse->getVirtualScreenY(),combolist(j).x,combolist(j).y,combolist(j).x+(combolist(j).w*16)-1,combolist(j).y+(combolist(j).h*16)-1))
                        {
                            int m = popup_menu(combosel_rc_menu,x,y);
                            
                            switch(m)
                            {
                            case 0:
                                reset_combo_animations();
                                reset_combo_animations2();
                                edit_combo(Combo,true,CSet);
                                setup_combo_animations();
                                setup_combo_animations2();
                                redraw|=rALL;
                                break;
                                
                            case 1:
                                combo_screen(Combo>>8,Combo);
                                redraw|=rALL;
                                break;
                                
                            case 2:
                            {
                                int t = combobuf[Combo].tile;
                                int f = 0;
                                select_tile(t,f,0,CSet,true);
                                redraw|=rALL;
                                break;
                            }
                            
                            case 3:
                                onComboLocationReport();
                                break;
                                
                            case 5:
                            {
                                onGotoPage();
                                redraw|=rALL;
                                break;
                            }
                            break;
                            }
                        }
                    }
                }
                else
                {
                    if(isinRect(x,y,comboaliaslist(j).x,comboaliaslist(j).y,comboaliaslist(j).x+(comboaliaslist(j).w*16)-1,comboaliaslist(j).y+(comboaliaslist(j).h*16)-1))
                    {
                        select_comboa(j);
                        
                        if(isinRect(Backend::mouse->getVirtualScreenX(), Backend::mouse->getVirtualScreenY(),combolist(j).x,combolist(j).y,combolist(j).x+(combolist(j).w*16)-1,combolist(j).y+(combolist(j).h*16)-1))
                        {
                            comboa_cnt = combo_apos;
                            onEditComboAlias();
                            redraw|=rALL;
                        }
                    }
                }
            }
        }
        
        // Right click main panel
        if(is_large())
        {
            if(isinRect(x,y,panel(8).x+16,panel(8).y+14,panel(8).x+16+15,panel(0).y+14+15))
            {
                onItem();
            }
        }
        else
        {
            if(menutype==m_coords)
            {
                if(isinRect(x,y,panel(0).x+16,panel(0).y+6,panel(0).x+16+15,panel(0).y+6+15))
                {
                    onItem();
                }
            }
        }
        
        if((isinRect(x,y,panel(0).x+panel(0).w-28,panel(0).y+32,panel(0).x+panel(0).w-28+24,panel(0).y+32+5) && menutype==m_block && !mouse_down) ||
                (isinRect(x,y,panel(6).x+panel(6).w-28,panel(6).y+36,panel(6).x+panel(6).w-28+24,panel(6).y+36+5) && menutype==m_layers && !mouse_down))
        {
            CSet=wrap(CSet-1,0,11);
            refresh(rCOMBOS+rMENU+rCOMBO);
        }
        
        if(isinRect(x,y,panel(0).x+panel(0).w-32,panel(0).y+39,panel(0).x+panel(0).w-32+28,panel(0).y+39+5) && menutype==m_block && !mouse_down)
        {
            bool validlayer=false;
            
            while(!validlayer)
            {
                CurrentLayer=wrap(CurrentLayer-1,0,6);
                
                if((CurrentLayer==0)||(Map.CurrScr()->layermap[CurrentLayer-1]))
                {
                    validlayer=true;
                }
            }
            
            refresh(rMENU);
        }
        
        if(is_large())
        {
            for(int cmd=0; cmd<(commands_list().w*commands_list().h); ++cmd)
            {
                int check_x=(cmd%commands_list().w)*command_buttonwidth+commands_list().x;
                int check_y=(cmd/commands_list().w)*command_buttonheight+commands_list().y;
                
                if(isinRect(x,y,check_x,check_y,check_x+command_buttonwidth,check_y+command_buttonheight))
                {
                    FONT *tfont=font;
                    font=pfont;
                    
                    if(do_text_button_reset(check_x,check_y,command_buttonwidth, command_buttonheight, favorite_commands[cmd]==cmdCatchall&&strcmp(catchall_string[Map.CurrScr()->room]," ")?catchall_string[Map.CurrScr()->room]:commands[favorite_commands[cmd]].name,vc(1),vc(14),true))
                    {
                        favorite_commands[cmd]=onCommand(favorite_commands[cmd]);
                    }
                    
                    font=tfont;
                }
            }
        }
        
        if(isinRect(x,y,favorites_list().x,favorites_list().y,favorites_list().x+(favorites_list().w*16)-1,favorites_list().y+(favorites_list().h*16)-1))
        {
            bool valid=select_favorite();
            
            if(valid)
            {
                if(isinRect(Backend::mouse->getVirtualScreenX(), Backend::mouse->getVirtualScreenY(),favorites_list().x,favorites_list().y,favorites_list().x+(favorites_list().w*16)-1,favorites_list().y+(favorites_list().h*16)-1))
                {
                    int m = popup_menu(fav_rc_menu,x,y);
                    int row=vbound(((y-favorites_list().y)>>4),0,favorites_list().h-1);
                    int col=vbound(((x-favorites_list().x)>>4),0,favorites_list().w-1);
                    int f=(row*favorites_list().w)+col;
                    
                    switch(m)
                    {
                    case 0:
                        First[current_combolist]=vbound((Combo/combolist(0).w*combolist(0).w)-(combolist(0).w*combolist(0).h/2),0,MAXCOMBOS-(combolist(0).w*combolist(0).h));
                        break;
                        
                    case 1:
                        if(draw_mode != dm_alias)
                        {
                            reset_combo_animations();
                            reset_combo_animations2();
                            edit_combo(Combo,true,CSet);
                            setup_combo_animations();
                            setup_combo_animations2();
                        }
                        else
                        {
                            comboa_cnt = combo_apos;
                            onEditComboAlias();
                        }
                        
                        redraw|=rALL;
                        break;
                        
                    case 2:
                        if(draw_mode != dm_alias)
                        {
                            favorite_combos[f]=-1;
                        }
                        else
                        {
                            favorite_comboaliases[f]=-1;
                        }
                        
                        break;
                    }
                }
            }
        }
        
        mouse_down = true;
    }
    else if(Backend::mouse->middleButtonClicked())  //not sure what to do here yet
    {
    }
    
    if(Backend::mouse->getWheelPosition() !=0)
    {
        int z=0;
        
        for(int j=0; j<3; ++j)
        {
            z=abs(Backend::mouse->getWheelPosition());
            
            if(key[KEY_ALT]||key[KEY_ALTGR])
            {
                z*=combolist(j).h;
            }
            
            if(j==0||is_large())
            {
                if(draw_mode != dm_alias)
                {
                    if(isinRect(x,y,combolist(j).x,combolist(j).y,combolist(j).x+(combolist(j).w*16)-1,combolist(j).y+(combolist(j).h*16)-1))
                    {
                        if(Backend::mouse->getWheelPosition()<0)  //scroll down
                        {
                            First[current_combolist] = zc_min(MAXCOMBOS-combolist(j).w*combolist(j).h,
                                                              First[current_combolist] + combolist(j).w*z);
                            redraw|=rALL;
                        }
                        else //scroll up
                        {
                            if(First[current_combolist]>0)
                            {
                                First[current_combolist]-=zc_min(First[current_combolist],combolist(j).w*z);
                                //          refresh(rCOMBOS);
                                redraw|=rALL;
                            }
                        }
                    }
                }
                else
                {
                    if(isinRect(x,y,comboaliaslist(j).x,comboaliaslist(j).y,comboaliaslist(j).x+(comboaliaslist(j).w*16)-1,comboaliaslist(j).y+(comboaliaslist(j).h*16)-1))
                    {
                        if(Backend::mouse->getWheelPosition()<0)  //scroll down
                        {
                            combo_alistpos[current_comboalist] = zc_min(MAXCOMBOALIASES - comboaliaslist(j).w*comboaliaslist(j).h,
                                                                 combo_alistpos[current_comboalist]+comboaliaslist(j).w*z);
                            redraw|=rALL;
                        }
                        else //scroll up
                        {
                            if(combo_alistpos[current_comboalist]>0)
                            {
                                combo_alistpos[current_comboalist]-=zc_min(combo_alistpos[current_comboalist],comboaliaslist(j).w*z);
                                //          refresh(rCOMBOS);
                                redraw|=rALL;
                            }
                        }
                    }
                }
            }
        }
        
        z=abs(Backend::mouse->getWheelPosition());
        
        if((!is_large() && isinRect(x,y,minimap().x,minimap().y+8,minimap().x+63,minimap().y+8+35)) ||
                (is_large() && isinRect(x,y,minimap().x,minimap().y+8,minimap().x+145,minimap().y+8+85)))
        {
            for(int i=0; i<z; ++i)
            {
                if(Backend::mouse->getWheelPosition()>0) onIncMap();
                else onDecMap();
            }
        }
        
        if(isinRect(x,y,panel(0).x,panel(0).y,panel(0).x+191,panel(0).y+47) && !is_large())
        {
            for(int i=0; i<z; ++i)
            {
                if(Backend::mouse->getWheelPosition()>0)
                {
                    onPgUp();
                }
                else
                {
                    onPgDn();
                }
            }
        }
        
		Backend::mouse->setWheelPosition(0);
    }
}

static DIALOG showpal_dlg[] =
{
    /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp) */
    { jwin_win_proc,     24,   68,   272,  119,   vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "View Palette", NULL, NULL },
    { jwin_frame_proc,   30,   76+16,   260,  68,   0,       0,      0,       0,             FR_DEEP,       0,       NULL, NULL, NULL },
    { d_bitmap_proc,     32,   76+18,   256,  64,   0,       0,      0,       0,          0,             0,       NULL, NULL, NULL },
    { jwin_button_proc,     130,  144+18,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "OK", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

int onShowPal()
{
    float palscale = is_large() ? 1.5 : 1;
    
    BITMAP *palbmp = create_bitmap_ex(8,(int)(256*palscale),(int)(64*palscale));
    
    if(!palbmp)
        return D_O_K;
        
    showpal_dlg[0].dp2=lfont;
    
    for(int i=0; i<256; i++)
        rectfill(palbmp,(int)(((i&31)<<3)*palscale),(int)(((i&0xE0)>>2)*palscale), (int)((((i&31)<<3)+7)*palscale),(int)((((i&0xE0)>>2)+7)*palscale),i);
    showpal_dlg[2].dp=(void *)palbmp;

	DIALOG *showpal_cpy = resizeDialog(showpal_dlg, 1.5);
    
    zc_popup_dialog(showpal_cpy,2);
	delete[] showpal_cpy;
    destroy_bitmap(palbmp);
    return D_O_K;
}

static DIALOG csetfix_dlg[] =
{
    /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp) */
    { jwin_win_proc,        72,   80,   176+1,  96+1,   vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "CSet Fix", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { jwin_radio_proc,      104+22,  108,  80+1,   8+1,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Full Screen", NULL, NULL },
    { jwin_radio_proc,      104+22,  118+2,  80+1,   8+1,    vc(14),  vc(1),  0,       D_SELECTED, 0,             0, (void *) "Dungeon Floor", NULL, NULL },
    { d_dummy_proc,         120,  128,  80+1,   8+1,    vc(14),  vc(1),  0,       0,          1,             0,       NULL, NULL, NULL },
    { jwin_check_proc,      104+22,  128+4,  80+1,   8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "All Layers", NULL, NULL },
    { jwin_button_proc,     90,   152,  61,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "OK", NULL, NULL },
    { jwin_button_proc,     170,  152,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

int onCSetFix()
{
    restore_mouse();
    csetfix_dlg[0].dp2=lfont;
    int s=2,x2=14,y2=9;

	DIALOG *csetfix_cpy = resizeDialog(csetfix_dlg, 1.5);
        
    if(zc_popup_dialog(csetfix_cpy,-1)==6)
    {
        Map.Ugo();
        
        if(csetfix_cpy[2].flags&D_SELECTED)
        {
            s=0;
            x2=16;
            y2=11;
        }
        
        if(csetfix_cpy[5].flags&D_SELECTED)
        {
            /*
              int drawmap, drawscr;
              if (CurrentLayer==0)
              {
              drawmap=Map.getCurrMap();
              drawscr=Map.getCurrScr();
              }
              else
              {
              drawmap=Map.CurrScr()->layermap[CurrentLayer-1]-1;
              drawscr=Map.CurrScr()->layerscreen[CurrentLayer-1];
              if (drawmap<0)
              {
              return;
              }
              }
            
              saved=false;
              Map.Ugo();
            
              if(!(Map.AbsoluteScr(drawmap, drawscr)->valid&mVALID))
              {
              Map.CurrScr()->valid|=mVALID;
              Map.AbsoluteScr(drawmap, drawscr)->valid|=mVALID;
              Map.setcolor(Color);
              }
              for(int i=0; i<176; i++)
              {
              Map.AbsoluteScr(drawmap, drawscr)->data[i]=Combo;
              Map.AbsoluteScr(drawmap, drawscr)->cset[i]=CSet;
              }
              refresh(rMAP+rSCRMAP);
              */
        }
        
        for(int y=s; y<y2; y++)
        {
            for(int x=s; x<x2; x++)
            {
                Map.CurrScr()->cset[(y<<4)+x] = CSet;
            }
        }
        
        refresh(rMAP);
        saved = false;
    }

	delete[] csetfix_cpy;
    
    return D_O_K;
}

static DIALOG template_dlg[] =
{
    /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp) */
    { jwin_win_proc, 72,   80,   176+1,  116+1,   vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "NES Dungeon Template", NULL, NULL },
    { d_comboframe_proc,   178,  122+3,  20,  20,   0,       0,      0,       0,             FR_DEEP,       0,       NULL, NULL, NULL },
    { d_combo_proc,      180,  124+3,  16,   16,   0,       0,      0,       0,          0,             0,       NULL, NULL, NULL },
    //  { d_bitmap_proc,     180,  104,  16,   16,   0,       0,      0,       0,          0,             0,       NULL, NULL, NULL },
    { jwin_radio_proc,      104+33,  128+3,  64+1,   8+1,    vc(14),  vc(1),  0,       D_SELECTED, 0,             0, (void *) "Floor:", NULL, NULL },
    { jwin_radio_proc,      104+33,  148+3,  64+1,   8+1,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "No Floor", NULL, NULL },
    { jwin_button_proc,     90,   172,  61,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "OK", NULL, NULL },
    { jwin_button_proc,     170,  172,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { jwin_text_proc,       104,  102,    16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "This copies the contents of",        NULL,   NULL },
    { jwin_text_proc,       104,  112,    16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "screen 83 of the current map.",    NULL,   NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

int onTemplate()
{
    static bool donethis=false;
    
    if(!donethis||!(key[KEY_LSHIFT]||key[KEY_RSHIFT]))
    {
        template_dlg[2].d1=Combo;
        template_dlg[2].fg=CSet;
        donethis=true;
    }
    
    restore_mouse();
    
    if(Map.getCurrScr()==TEMPLATE)
        return D_O_K;
        
    //  BITMAP *floor_bmp = create_bitmap_ex(8,16,16);
    //  if(!floor_bmp) return D_O_K;
    template_dlg[0].dp2=lfont;
    //  put_combo(floor_bmp,0,0,Combo,CSet,0,0);
    //  template_dlg[2].dp=floor_bmp;

	DIALOG *template_cpy = resizeDialog(template_dlg, 1.5);
    
    if(zc_popup_dialog(template_cpy,-1)==5)
    {
        saved=false;
        Map.Ugo();
        Map.Template((template_cpy[3].flags==D_SELECTED) ? template_cpy[2].d1 : -1, template_cpy[2].fg);
        refresh(rMAP+rSCRMAP);
    }

	delete[] template_cpy;
    
    //  destroy_bitmap(floor_bmp);
    return D_O_K;
}

static DIALOG cpage_dlg[] =
{
    /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp) */
    { jwin_win_proc, 72,   20,   176+1,  212+1,  vc(14),  vc(1),  0,       0,          0,             0,       NULL, NULL, NULL },
    { d_ctext_proc,      160,  28,     0,  8,    vc(15),  vc(1),  0,       0,          0,             0, (void *) "Combo Page", NULL, NULL },
    { jwin_button_proc,     90,   182,  61,   21,   vc(14),  vc(1),  's',     D_EXIT,     0,             0, (void *) "&Set", NULL, NULL },
    { jwin_button_proc,     170,  182,  61,   21,   vc(14),  vc(1),  'c',     D_EXIT,     0,             0, (void *) "&Cancel", NULL, NULL },
    { jwin_button_proc,     90,   210,  61,   21,   vc(14),  vc(1),  'a',     D_EXIT,     0,             0, (void *) "Set &All", NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,       0,      0,       0,          KEY_F1,        0, (void *) onHelp, NULL, NULL },
    // 6
    { jwin_radio_proc,       76,  44,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "0"  , NULL, NULL },
    { jwin_radio_proc,       76,  52,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "1"  , NULL, NULL },
    { jwin_radio_proc,       76,  60,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "2"  , NULL, NULL },
    { jwin_radio_proc,       76,  68,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "3"  , NULL, NULL },
    { jwin_radio_proc,       76,  76,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "4"  , NULL, NULL },
    { jwin_radio_proc,       76,  84,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "5"  , NULL, NULL },
    { jwin_radio_proc,       76,  92,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "6"  , NULL, NULL },
    { jwin_radio_proc,       76, 100,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "7"  , NULL, NULL },
    { jwin_radio_proc,       76, 108,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "8"  , NULL, NULL },
    { jwin_radio_proc,       76, 116,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "9"  , NULL, NULL },
    { jwin_radio_proc,       76, 124,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "10" , NULL, NULL },
    { jwin_radio_proc,       76, 132,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "11" , NULL, NULL },
    { jwin_radio_proc,       76, 140,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "12" , NULL, NULL },
    { jwin_radio_proc,       76, 148,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "13" , NULL, NULL },
    { jwin_radio_proc,       76, 156,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "14" , NULL, NULL },
    { jwin_radio_proc,       76, 164,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "15" , NULL, NULL },
    { jwin_radio_proc,      120,  44,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "16" , NULL, NULL },
    { jwin_radio_proc,      120,  52,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "17" , NULL, NULL },
    { jwin_radio_proc,      120,  60,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "18" , NULL, NULL },
    { jwin_radio_proc,      120,  68,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "19" , NULL, NULL },
    { jwin_radio_proc,      120,  76,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "20" , NULL, NULL },
    { jwin_radio_proc,      120,  84,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "21" , NULL, NULL },
    { jwin_radio_proc,      120,  92,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "22" , NULL, NULL },
    { jwin_radio_proc,      120, 100,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "23" , NULL, NULL },
    { jwin_radio_proc,      120, 108,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "24" , NULL, NULL },
    { jwin_radio_proc,      120, 116,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "25" , NULL, NULL },
    { jwin_radio_proc,      120, 124,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "26" , NULL, NULL },
    { jwin_radio_proc,      120, 132,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "27" , NULL, NULL },
    { jwin_radio_proc,      120, 140,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "28" , NULL, NULL },
    { jwin_radio_proc,      120, 148,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "29" , NULL, NULL },
    { jwin_radio_proc,      120, 156,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "30" , NULL, NULL },
    { jwin_radio_proc,      120, 164,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "31" , NULL, NULL },
    { jwin_radio_proc,      164,  44,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "32" , NULL, NULL },
    { jwin_radio_proc,      164,  52,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "33" , NULL, NULL },
    { jwin_radio_proc,      164,  60,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "34" , NULL, NULL },
    { jwin_radio_proc,      164,  68,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "35" , NULL, NULL },
    { jwin_radio_proc,      164,  76,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "36" , NULL, NULL },
    { jwin_radio_proc,      164,  84,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "37" , NULL, NULL },
    { jwin_radio_proc,      164,  92,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "38" , NULL, NULL },
    { jwin_radio_proc,      164, 100,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "39" , NULL, NULL },
    { jwin_radio_proc,      164, 108,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "40" , NULL, NULL },
    { jwin_radio_proc,      164, 116,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "41" , NULL, NULL },
    { jwin_radio_proc,      164, 124,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "42" , NULL, NULL },
    { jwin_radio_proc,      164, 132,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "43" , NULL, NULL },
    { jwin_radio_proc,      164, 140,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "44" , NULL, NULL },
    { jwin_radio_proc,      164, 148,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "45" , NULL, NULL },
    { jwin_radio_proc,      164, 156,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "46" , NULL, NULL },
    { jwin_radio_proc,      164, 164,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "47" , NULL, NULL },
    { jwin_radio_proc,      208,  44,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "48" , NULL, NULL },
    { jwin_radio_proc,      208,  52,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "49" , NULL, NULL },
    { jwin_radio_proc,      208,  60,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "50" , NULL, NULL },
    { jwin_radio_proc,      208,  68,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "51" , NULL, NULL },
    { jwin_radio_proc,      208,  76,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "52" , NULL, NULL },
    { jwin_radio_proc,      208,  84,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "53" , NULL, NULL },
    { jwin_radio_proc,      208,  92,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "54" , NULL, NULL },
    { jwin_radio_proc,      208, 100,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "55" , NULL, NULL },
    { jwin_radio_proc,      208, 108,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "56" , NULL, NULL },
    { jwin_radio_proc,      208, 116,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "57" , NULL, NULL },
    { jwin_radio_proc,      208, 124,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "58" , NULL, NULL },
    { jwin_radio_proc,      208, 132,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "59" , NULL, NULL },
    { jwin_radio_proc,      208, 140,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "60" , NULL, NULL },
    { jwin_radio_proc,      208, 148,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "61" , NULL, NULL },
    { jwin_radio_proc,      208, 156,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "62" , NULL, NULL },
    { jwin_radio_proc,      208, 164,   33,   9,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "63" , NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

int onComboPage()
{
    for(int i=0; i<64; i++)
        cpage_dlg[i+6].flags = Map.CurrScr()->old_cpage==i?D_SELECTED:0;
        
    int ret = zc_popup_dialog(cpage_dlg,3);
    
    int p = 0;
    
    for(int i=0; i<64; i++)
    
        if(cpage_dlg[i+6].flags==D_SELECTED)
            p=i;
            
    if(ret==2)
    {
        saved=false;
        Map.CurrScr()->old_cpage = p;
    }
    
    if(ret==4 && jwin_alert("Confirm Overwrite","Set all combo pages","on this map?",NULL,"&Yes","&No",'y','n',lfont)==1)
    {
        saved=false;
        
        for(int i=0; i<=TEMPLATE; i++)
            Map.Scr(i)->old_cpage = p;
    }
    
    refresh(rALL);
    return D_O_K;
}

int d_sel_scombo_proc(int msg, DIALOG *d, int c)
{
    //these are here to bypass compiler warnings about unused arguments
    c=c;
    
    switch(msg)
    {
    case MSG_CLICK:
        while(Backend::mouse->anyButtonClicked())
        {
            int x = zc_min(zc_max(Backend::mouse->getVirtualScreenX() - d->x,0)>>4, 15);
            int y = zc_min(zc_max(Backend::mouse->getVirtualScreenY() - d->y,0)&0xF0, 160);
            
            if(x+y != d->d1)
            {
                d->d1 = x+y;
                d_sel_scombo_proc(MSG_DRAW,d,0);
				Backend::graphics->waitTick();
				Backend::graphics->showBackBuffer();
            }
        }
        
        break;
        
    case MSG_DRAW:
    {
        blit((BITMAP*)(d->dp),screen,0,0,d->x,d->y,d->w,d->h);
        int x = d->x + (((d->d1)&15)<<4);
        int y = d->y + ((d->d1)&0xF0);
        rect(screen,x,y,x+15,y+15,vc(15));
    }
    break;
    }
    
    return D_O_K;
}

static DIALOG cflag_dlg[] =
{
    // (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp)
    { jwin_win_proc,     60-12,   40,   200+24,  148,  vc(14),  vc(1),  0,       D_EXIT,          0,             0,       NULL, NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { jwin_abclist_proc,       72-12-4,   60+4,   176+24+8,  92+3,   jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,       D_EXIT,     0,             0,       NULL, NULL, NULL },
    { jwin_button_proc,     70,   163,  51,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "OK", NULL, NULL },
    { jwin_button_proc,     190,  163,  51,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { jwin_button_proc,     130,  163,  51,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Help", NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};


void ctype_help(int id)
{
    if(id < 0 || id >= cMAX) return;  // Sanity check
    
    if(id==0)
    {
        jwin_alert("Help","Select a Type, then click","this button to find out what it does.",NULL,"O&K",NULL,'k',0,lfont);
    }
    else if(id >= cCVUP && id <= cCVRIGHT)
    {
        char buf1[80];
        sprintf(buf1,"be moved %sward at 1/4 of his normal walking",id==cCVUP ? "up" : id==cCVDOWN ? "down" : id==cCVLEFT ? "left" : "right");
        jwin_alert(combo_class_buf[id].name,"While Link is standing on top of this, he will",buf1,"speed, until he collides with a solid combo.","O&K",NULL,'k',0,lfont);
    }
    else if((id >= cDAMAGE1 && id <= cDAMAGE4) || (id >= cDAMAGE5 && id <= cDAMAGE7))
    {
        char buf1[80];
        int lvl = (id < cDAMAGE5 ? (id - cDAMAGE1 + 1) : (id - cDAMAGE5 + 1));
        sprintf(buf1,"that protect against Damage Combo Level %d,",lvl);
        char buf2[80];
        int d = -combo_class_buf[id].modify_hp_amount/8;
        
        if(d==1)
            sprintf(buf2,"he is damaged for 1/2 of a heart.");
        else
            sprintf(buf2,"he is damaged for %d heart%s.", d/2, d == 2 ? "" : "s");
            
        jwin_alert(combo_class_buf[id].name,"If Link touches this combo without Boots",buf1,buf2, "O&K",NULL,'k',0,lfont);
    }
    else if(id >= cSLASHTOUCHY && id <= cBUSHNEXTTOUCHY)
    {
        char buf1[80];
        sprintf(buf1,"Identical to %s, but if slashing this combo",combotype_help_string[id*3]);
        jwin_alert(combo_class_buf[id].name,buf1,"changes it to another slash-affected combo,","then that combo will also change.","O&K",NULL,'k',0,lfont);
    }
    else if(id >= cSCRIPT1 && id <= cSCRIPT5)
    {
        jwin_alert(combo_class_buf[id].name,"This type has no built-in effect, but can be","given special significance with ZASM or ZScript.",NULL,"O&K",NULL,'k',0,lfont);
    }
    else
        jwin_alert(combo_class_buf[id].name,combotype_help_string[id*3],combotype_help_string[1+(id*3)],combotype_help_string[2+(id*3)],"O&K",NULL,'k',0,lfont);
}

void cflag_help(int id)
{
    if(id < 0 || id >= mfMAX) return;  // Sanity check
    
    if(id==0)
    {
        jwin_alert("Help","Select a Flag, then click","this button to find out what it does.",NULL,"O&K",NULL,'k',0,lfont);
    }
    else  if(id>=mfSECRETS01 && id <=mfSECRETS16)
    {
        char buf1[80];
        sprintf(buf1,"with Secret Combo %d. (Also, flagged Destructible Combos",id);
        jwin_alert(flag_string[id],"When Screen Secrets are triggered, this is replaced",buf1,"will use that Secret Combo instead of the Under Combo.)","O&K",NULL,'k',0,lfont);
    }
    else if(id >=37 && id <= 46)
    {
        char buf1[80];
        sprintf(buf1,"When the %d%s enemy in the Enemy List is spawned, it appears",(id-37)+1,((id-37)+1)==1?"st":((id-37)+1)==2?"nd":((id-37)+1)==3?"rd":"th");
        jwin_alert(flag_string[id],buf1,"on this flag instead of using the Enemy Pattern.","The uppermost, then leftmost, instance of this flag is used.","O&K",NULL,'k',0,lfont);
    }
    else if(id >= mfPUSHUDNS && id <= mfPUSHRINS)
    {
        char buf1[80];
        int t = ((id-mfPUSHUDNS) % 7);
        sprintf(buf1,"Allows Link to push the combo %s %s,",
                (t == 0) ? "up and down" : (t == 1) ? "left and right" : (t == 2) ? "in any direction" : (t == 3) ? "up" : (t == 4) ? "down" : (t == 5) ? "left" : "right",
                (id>=mfPUSHUDINS) ? "many times":"once");
        jwin_alert(flag_string[id],buf1,"triggering Block->Shutters but not Screen Secrets.","","O&K",NULL,'k',0,lfont);
    }
    else if(id >= mfSCRIPT1 && id <= mfSCRIPT5)
        jwin_alert(flag_string[id],"These flags have no built-in effect, but can be","given special significance with ZASM or ZScript.",NULL,"O&K",NULL,'k',0,lfont);
    else
        jwin_alert(flag_string[id],flag_help_string[id*3],flag_help_string[1+(id*3)],flag_help_string[2+(id*3)],"O&K",NULL,'k',0,lfont);
}

int select_cflag(const char *prompt,int index)
{
    cflag_dlg[0].dp=(void *)prompt;
    cflag_dlg[0].dp2=lfont;
    cflag_dlg[2].d1=index;
    ListData select_cflag_list(flaglist, &font);
    cflag_dlg[2].dp=(void *) &select_cflag_list;

	DIALOG *cflag_cpy = resizeDialog(cflag_dlg, 1.5);
    
    int ret;
    
    do
    {
        ret=zc_popup_dialog(cflag_cpy,2);
        
        if(ret==5)
        {
            int id = cflag_cpy[2].d1;
            cflag_help(id);
        }
    }
    while(ret==5);
    
    if(ret==0||ret==4)
    {
		Backend::mouse->setWheelPosition(0);
        return -1;
    }

	int retval = cflag_cpy[2].d1;

	delete[] cflag_cpy;
    
	return retval;
}

int select_flag(int &f)
{
    int ret=select_cflag("Flag Type",f);
    
    if(ret>=0)
    {
        f=ret;
        return true;
    }
    
    return false;
}

int d_scombo_proc(int msg,DIALOG *d,int c)
{
    //these are here to bypass compiler warnings about unused arguments
    c=c;
    
    switch(msg)
    {
    case MSG_CLICK:
    {
        int c2=d->d1;
        int cs=d->fg;
        int f=d->d2;
        
        if(key[KEY_ZC_LCONTROL] || key[KEY_ZC_RCONTROL])
        {
            while(Backend::mouse->anyButtonClicked())
            {
				Backend::graphics->waitTick();
				Backend::graphics->showBackBuffer();
                /* do nothing */
            }
            
            if(select_flag(f))
            {
                d->d2=f;
                
            }
        }
        else if(key[KEY_LSHIFT])
        {
            if(Backend::mouse->leftButtonClicked())
            {
                d->d1++;
                
                if(d->d1>=MAXCOMBOS) d->d1=0;
            }
            else if(Backend::mouse->rightButtonClicked())
            {
                d->d1--;
                
                if(d->d1<0) d->d1=MAXCOMBOS-1;
            }
        }
        else if(key[KEY_RSHIFT])
        {
            if(Backend::mouse->leftButtonClicked())
            {
                d->fg++;
                
                if(d->fg>11) d->fg=0;
            }
            else if(Backend::mouse->rightButtonClicked())
            {
                d->fg--;
                
                if(d->fg<0) d->fg=11;
            }
        }
        else if(key[KEY_ALT])
        {
            if(Backend::mouse->leftButtonClicked())
            {
                d->d1 = Combo;
                d->fg = CSet;
            }
        }
        else
        {
            if(select_combo_2(c2, cs))
            {
                d->d1=c2;
                d->fg=cs;
            }
        }
        
        return D_REDRAW;
    }
    break;
    
    case MSG_DRAW:
        if(is_large())
        {
            d->w = 32;
            d->h = 32;
        }
        
        BITMAP *buf = create_bitmap_ex(8,16,16);
        BITMAP *bigbmp = create_bitmap_ex(8,d->w,d->h);
        
        if(buf && bigbmp)
        {
            clear_bitmap(buf);
            
            if(d->d1)
            {
                putcombo(buf,0,0,d->d1,d->fg);
                
                if(Flags&cFLAGS)
                    put_flags(buf,0,0,d->d1,d->fg,cFLAGS,d->d2);
            }
            
            stretch_blit(buf, bigbmp, 0,0, 16, 16, 0, 0, d->w, d->h);
            destroy_bitmap(buf);
            blit(bigbmp,screen,0,0,d->x-(is_large() ? 1 : 0),d->y-(is_large() ? 1 : 0),d->w,d->h);
            destroy_bitmap(bigbmp);
        }
        
        
        /*BITMAP *buf = create_bitmap_ex(8,16,16);
        if(buf)
        {
          clear_bitmap(buf);
          if(d->d1)
            putcombo(buf,0,0,d->d1,d->fg);
        
          blit(buf,screen,0,0,d->x,d->y,d->w,d->h);
          destroy_bitmap(buf);
        }*/
        break;
    }
    
    return D_O_K;
}

int onSecretF();

static int secret_burn_list[] =
{
    // dialog control number
    4, 5, 6, 7, 48, 49, 50, 51, 92, 93, 94, 95, -1
};

static int secret_arrow_list[] =
{
    // dialog control number
    8, 9, 10, 52, 53, 54, 96, 97, 98, -1
};

static int secret_bomb_list[] =
{
    // dialog control number
    11, 12, 55, 56, 99, 100, -1
};

static int secret_boomerang_list[] =
{
    // dialog control number
    13, 14, 15, 57, 58, 59, 101, 102, 103, -1
};

static int secret_magic_list[] =
{
    // dialog control number
    16, 17, 60, 61, 104, 105, -1
};

static int secret_sword_list[] =
{
    // dialog control number
    18, 19, 20, 21, 22, 23, 24, 25, 62, 63, 64, 65, 66, 67, 68, 69, 106, 107, 108, 109, 110, 111, 112, 113, -1
};

static int secret_misc_list[] =
{
    // dialog control number
    26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, -1
};

static TABPANEL secret_tabs[] =
{
    // (text)
    { (char *)"Burn",       D_SELECTED,   secret_burn_list, 0, NULL },
    { (char *)"Arrow",       0,           secret_arrow_list, 0, NULL },
    { (char *)"Bomb",        0,           secret_bomb_list, 0, NULL },
    { (char *)"Boomerang",   0,           secret_boomerang_list, 0, NULL },
    { (char *)"Magic",       0,           secret_magic_list, 0, NULL },
    { (char *)"Sword",       0,           secret_sword_list, 0, NULL },
    { (char *)"Misc",        0,           secret_misc_list, 0, NULL },
    { NULL,                  0,           NULL, 0, NULL }
};

static DIALOG secret_dlg[] =
{
    // (dialog proc)            (x)     (y)     (w)     (h)     (fg)        (bg)    (key)        (flags)  (d1)         (d2)   (dp)                             (dp2)   (dp3)
    {  jwin_win_proc,             0,      0,    301,    212,    vc(14),     vc(1),      0,       D_EXIT,     0,           0,  NULL,        NULL,   NULL                },
    {  jwin_tab_proc,             6,     25,    289,    156,    0,          0,          0,       0,          0,           0, (void *) secret_tabs,            NULL, (void *)secret_dlg  },
    {  jwin_button_proc,         80,    187,     61,     21,    vc(14),     vc(1),     13,       D_EXIT,     0,           0, (void *) "OK",                   NULL,   NULL                },
    {  jwin_button_proc,        160,    187,     61,     21,    vc(14),     vc(1),     27,       D_EXIT,     0,           0, (void *) "Cancel",               NULL,   NULL                },
    // 4
    {  jwin_text_proc,           12,     53,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Blue Candle",          NULL,   NULL                },
    {  jwin_text_proc,           12,     75,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Red Candle",           NULL,   NULL                },
    {  jwin_text_proc,           12,     97,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Wand Fire",            NULL,   NULL                },
    {  jwin_text_proc,           12,    119,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Din's Fire",           NULL,   NULL                },
    //8
    {  jwin_text_proc,           12,     53,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Wooden Arrow",         NULL,   NULL                },
    {  jwin_text_proc,           12,     75,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Silver Arrow",         NULL,   NULL                },
    {  jwin_text_proc,           12,     97,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Golden Arrow",         NULL,   NULL                },
    //11
    {  jwin_text_proc,           12,     53,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Bomb",                 NULL,   NULL                },
    {  jwin_text_proc,           12,     75,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Super Bomb",           NULL,   NULL                },
    //13
    {  jwin_text_proc,           12,     53,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Wooden Boomerang",     NULL,   NULL                },
    {  jwin_text_proc,           12,     75,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Magic Boomerang",      NULL,   NULL                },
    {  jwin_text_proc,           12,     97,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Fire Boomerang",       NULL,   NULL                },
    //16
    {  jwin_text_proc,           12,     53,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Wand Magic",           NULL,   NULL                },
    {  jwin_text_proc,           12,     75,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Reflected Magic",      NULL,   NULL                },
    //18
    {  jwin_text_proc,           12,     53,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Sword",                NULL,   NULL                },
    {  jwin_text_proc,           12,     75,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "White Sword",          NULL,   NULL                },
    {  jwin_text_proc,           12,     97,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Magic Sword",          NULL,   NULL                },
    {  jwin_text_proc,           12,    119,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Master Sword",         NULL,   NULL                },
    {  jwin_text_proc,          160,     53,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Sword Beam",           NULL,   NULL                },
    {  jwin_text_proc,          160,     75,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "White Sword Beam",     NULL,   NULL                },
    {  jwin_text_proc,          160,     97,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Magic Sword Beam",     NULL,   NULL                },
    {  jwin_text_proc,          160,    119,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Master Sword Beam",    NULL,   NULL                },
    //26
    {  jwin_text_proc,           12,     53,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Stairs",               NULL,   NULL                },
    {  jwin_text_proc,           12,     75,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Reflected Fireball",   NULL,   NULL                },
    {  jwin_text_proc,           12,     97,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Hookshot",             NULL,   NULL                },
    {  jwin_text_proc,           12,    119,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Wand",                 NULL,   NULL                },
    {  jwin_text_proc,           12,    141,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Hammer",               NULL,   NULL                },
    {  jwin_text_proc,           12,    163,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Any Weapon",           NULL,   NULL                },
    //32
    {  jwin_ctext_proc,         211,     53,     16,     16,    vc(11),     vc(1),      0,       0,          0,           0, (void *) "Flags 16-31",          NULL,   NULL                },
    {  d_dummy_proc,              0,      0,      0,      0,    0,          0,          0,       0,          FR_DEEP,     0, (void *) "Secret Combo 02",      NULL,   NULL                },
    {  d_dummy_proc,              0,      0,      0,      0,    0,          0,          0,       0,          FR_DEEP,     0, (void *) "Secret Combo 03",      NULL,   NULL                },
    {  d_dummy_proc,              0,      0,      0,      0,    0,          0,          0,       0,          FR_DEEP,     0, (void *) "Secret Combo 04",      NULL,   NULL                },
    {  d_dummy_proc,              0,      0,      0,      0,    0,          0,          0,       0,          FR_DEEP,     0, (void *) "Secret Combo 05",      NULL,   NULL                },
    {  d_dummy_proc,              0,      0,      0,      0,    0,          0,          0,       0,          FR_DEEP,     0, (void *) "Secret Combo 06",      NULL,   NULL                },
    {  d_dummy_proc,              0,      0,      0,      0,    0,          0,          0,       0,          FR_DEEP,     0, (void *) "Secret Combo 07",      NULL,   NULL                },
    {  d_dummy_proc,              0,      0,      0,      0,    0,          0,          0,       0,          FR_DEEP,     0, (void *) "Secret Combo 08",      NULL,   NULL                },
    {  d_dummy_proc,              0,      0,      0,      0,    0,          0,          0,       0,          FR_DEEP,     0, (void *) "Secret Combo 09",      NULL,   NULL                },
    {  d_dummy_proc,              0,      0,      0,      0,    0,          0,          0,       0,          FR_DEEP,     0, (void *) "Secret Combo 10",      NULL,   NULL                },
    {  d_dummy_proc,              0,      0,      0,      0,    0,          0,          0,       0,          FR_DEEP,     0, (void *) "Secret Combo 11",      NULL,   NULL                },
    {  d_dummy_proc,              0,      0,      0,      0,    0,          0,          0,       0,          FR_DEEP,     0, (void *) "Secret Combo 12",      NULL,   NULL                },
    {  d_dummy_proc,              0,      0,      0,      0,    0,          0,          0,       0,          FR_DEEP,     0, (void *) "Secret Combo 13",      NULL,   NULL                },
    {  d_dummy_proc,              0,      0,      0,      0,    0,          0,          0,       0,          FR_DEEP,     0, (void *) "Secret Combo 14",      NULL,   NULL                },
    {  d_dummy_proc,              0,      0,      0,      0,    0,          0,          0,       0,          FR_DEEP,     0, (void *) "Secret Combo 15",      NULL,   NULL                },
    {  d_dummy_proc,              0,      0,      0,      0,    0,          0,          0,       0,          FR_DEEP,     0, (void *) "Secret Combo 16",      NULL,   NULL                },
    //48 (burn)
    {  jwin_frame_proc,         108,     47,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         108,     69,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         108,     91,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         108,    113,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    //52 (arrow)
    {  jwin_frame_proc,         108,     47,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         108,     69,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         108,     91,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    //55 (bomb)
    {  jwin_frame_proc,         108,     47,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         108,     69,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    //57 (boomerang)
    {  jwin_frame_proc,         108,     47,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         108,     69,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         108,     91,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    //60 (magic)
    {  jwin_frame_proc,         108,     47,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         108,     69,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    //62 (sword)
    {  jwin_frame_proc,         108,     47,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         108,     69,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         108,     91,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         108,    113,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         256,     47,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         256,     69,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         256,     91,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         256,    113,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    //70 (misc)
    {  jwin_frame_proc,         108,     47,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         108,     69,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         108,     91,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         108,    113,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         108,    135,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         108,    157,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    //76
    {  jwin_frame_proc,         168,     69,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         190,     69,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         212,     69,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         234,     69,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         168,     91,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         190,     91,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         212,     91,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         234,     91,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         168,    113,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         190,    113,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         212,    113,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         234,    113,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         168,    135,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         190,    135,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         212,    135,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    {  jwin_frame_proc,         234,    135,     20,     20,    0,          0,          0,       0,          FR_DEEP,     0,  NULL,                            NULL,   NULL                },
    
    //92 (burn)
    {  d_scombo_proc,           110,     49,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           110,     71,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           110,     93,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           110,    115,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    //96 (arrow)
    {  d_scombo_proc,           110,     49,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           110,     71,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           110,     93,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    //99 (bomb)
    {  d_scombo_proc,           110,     49,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           110,     71,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    //101 (boomerang)
    {  d_scombo_proc,           110,     49,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           110,     71,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           110,     93,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    //104 (magic)
    {  d_scombo_proc,           110,     49,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           110,     71,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    //106 (sword)
    {  d_scombo_proc,           110,     49,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           110,     71,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           110,     93,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           110,    115,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           258,     49,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           258,     71,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           258,     93,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           258,    115,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    //114 (misc)
    {  d_scombo_proc,           110,     49,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           110,     71,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           110,     93,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           110,    115,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           110,    137,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           110,    159,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    //120
    {  d_scombo_proc,           170,     71,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           192,     71,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           214,     71,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           236,     71,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           170,     93,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           192,     93,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           214,     93,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           236,     93,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           170,    115,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           192,    115,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           214,    115,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           236,    115,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           170,    137,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           192,    137,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           214,    137,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  d_scombo_proc,           236,    137,     16,     16,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    //136
    {  d_keyboard_proc,           0,      0,      0,      0,    0,          0,          0,       0,          KEY_F1,      0, (void *) onHelp,                 NULL,   NULL                },
    {  d_keyboard_proc,           0,      0,      0,      0,    0,          0,          'f',     0,          0,           0, (void *) onSecretF,              NULL,   NULL                },
    {  d_timer_proc,              0,      0,      0,      0,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                },
    {  NULL,                      0,      0,      0,      0,    0,          0,          0,       0,          0,           0,  NULL,                            NULL,   NULL                }
};

int onSecretF()
{
    Flags^=cFLAGS;
    object_message(secret_dlg+1, MSG_DRAW, 0);
    return D_O_K;
}


int onSecretCombo()
{
    secret_dlg[0].dp2=lfont;
    
    
    mapscr *s;
    
    if(CurrentLayer==0)
    {
        s=Map.CurrScr();
    }
    else
    {
        //   s=TheMaps[(Map.CurrScr()->layermap[CurrentLayer-1]-1)*MAPSCRS+(Map.CurrScr()->layerscreen[CurrentLayer-1])];
        s=Map.AbsoluteScr((Map.CurrScr()->layermap[CurrentLayer-1]-1), (Map.CurrScr()->layerscreen[CurrentLayer-1]));
    }
    
    char secretcombonumstr[27];
    sprintf(secretcombonumstr,"Secret Combos for Layer %d", CurrentLayer);
    secret_dlg[0].dp = secretcombonumstr;
    
    secret_dlg[92].d1 = s->secretcombo[sBCANDLE];
    secret_dlg[92].fg = s->secretcset[sBCANDLE];
    secret_dlg[92].d2 = s->secretflag[sBCANDLE];
    
    secret_dlg[93].d1 = s->secretcombo[sRCANDLE];
    secret_dlg[93].fg = s->secretcset[sRCANDLE];
    secret_dlg[93].d2 = s->secretflag[sRCANDLE];
    
    secret_dlg[94].d1 = s->secretcombo[sWANDFIRE];
    secret_dlg[94].fg = s->secretcset[sWANDFIRE];
    secret_dlg[94].d2 = s->secretflag[sWANDFIRE];
    
    secret_dlg[95].d1 = s->secretcombo[sDINSFIRE];
    secret_dlg[95].fg = s->secretcset[sDINSFIRE];
    secret_dlg[95].d2 = s->secretflag[sDINSFIRE];
    
    secret_dlg[96].d1 = s->secretcombo[sARROW];
    secret_dlg[96].fg = s->secretcset[sARROW];
    secret_dlg[96].d2 = s->secretflag[sARROW];
    
    secret_dlg[97].d1 = s->secretcombo[sSARROW];
    secret_dlg[97].fg = s->secretcset[sSARROW];
    secret_dlg[97].d2 = s->secretflag[sSARROW];
    
    secret_dlg[98].d1 = s->secretcombo[sGARROW];
    secret_dlg[98].fg = s->secretcset[sGARROW];
    secret_dlg[98].d2 = s->secretflag[sGARROW];
    
    secret_dlg[99].d1 = s->secretcombo[sBOMB];
    secret_dlg[99].fg = s->secretcset[sBOMB];
    secret_dlg[99].d2 = s->secretflag[sBOMB];
    
    secret_dlg[100].d1 = s->secretcombo[sSBOMB];
    secret_dlg[100].fg = s->secretcset[sSBOMB];
    secret_dlg[100].d2 = s->secretflag[sSBOMB];
    
    for(int i=0; i<3; i++)
    {
        secret_dlg[101+i].d1 = s->secretcombo[sBRANG+i];
        secret_dlg[101+i].fg = s->secretcset[sBRANG+i];
        secret_dlg[101+i].d2 = s->secretflag[sBRANG+i];
    }
    
    for(int i=0; i<2; i++)
    {
        secret_dlg[104+i].d1 = s->secretcombo[sWANDMAGIC+i];
        secret_dlg[104+i].fg = s->secretcset[sWANDMAGIC+i];
        secret_dlg[104+i].d2 = s->secretflag[sWANDMAGIC+i];
    }
    
    for(int i=0; i<8; i++)
    {
        secret_dlg[106+i].d1 = s->secretcombo[sSWORD+i];
        secret_dlg[106+i].fg = s->secretcset[sSWORD+i];
        secret_dlg[106+i].d2 = s->secretflag[sSWORD+i];
    }
    
    secret_dlg[114].d1 = s->secretcombo[sSTAIRS];
    secret_dlg[114].fg = s->secretcset[sSTAIRS];
    secret_dlg[114].d2 = s->secretflag[sSTAIRS];
    
    secret_dlg[115].d1 = s->secretcombo[sREFFIREBALL];
    secret_dlg[115].fg = s->secretcset[sREFFIREBALL];
    secret_dlg[115].d2 = s->secretflag[sREFFIREBALL];
    
    for(int i=0; i<4; i++)
    {
        secret_dlg[116+i].d1 = s->secretcombo[sHOOKSHOT+i];
        secret_dlg[116+i].fg = s->secretcset[sHOOKSHOT+i];
        secret_dlg[116+i].d2 = s->secretflag[sHOOKSHOT+i];
    }
    
    for(int i=0; i<16; i++)
    {
        secret_dlg[120+i].d1 = s->secretcombo[sSECRET01+i];
        secret_dlg[120+i].fg = s->secretcset[sSECRET01+i];
        secret_dlg[120+i].d2 = s->secretflag[sSECRET01+i];
    }

	DIALOG *secret_cpy = resizeDialog(secret_dlg, 1.5);
    
    if(is_large())
    {
        for(int i=48; i<92; i++)
        {
			secret_cpy[i].w = secret_cpy[i].h = 36;
        }
    }
    
    go();
    
    if(zc_do_dialog(secret_cpy,3) == 2)
    {
        saved = false;
        s->secretcombo[sBCANDLE] = secret_cpy[92].d1;
        s->secretcset[sBCANDLE] = secret_cpy[92].fg;
        s->secretflag[sBCANDLE] = secret_cpy[92].d2;
        
        s->secretcombo[sRCANDLE] = secret_cpy[93].d1;
        s->secretcset[sRCANDLE] = secret_cpy[93].fg;
        s->secretflag[sRCANDLE] = secret_cpy[93].d2;
        
        s->secretcombo[sWANDFIRE] = secret_cpy[94].d1;
        s->secretcset[sWANDFIRE] = secret_cpy[94].fg;
        s->secretflag[sWANDFIRE] = secret_cpy[94].d2;
        
        s->secretcombo[sDINSFIRE] = secret_cpy[95].d1;
        s->secretcset[sDINSFIRE] = secret_cpy[95].fg;
        s->secretflag[sDINSFIRE] = secret_cpy[95].d2;
        
        s->secretcombo[sARROW] = secret_cpy[96].d1;
        s->secretcset[sARROW] = secret_cpy[96].fg;
        s->secretflag[sARROW] = secret_cpy[96].d2;
        
        s->secretcombo[sSARROW] = secret_cpy[97].d1;
        s->secretcset[sSARROW] = secret_cpy[97].fg;
        s->secretflag[sSARROW] = secret_cpy[97].d2;
        
        s->secretcombo[sGARROW] = secret_cpy[98].d1;
        s->secretcset[sGARROW] = secret_cpy[98].fg;
        s->secretflag[sGARROW] = secret_cpy[98].d2;
        
        s->secretcombo[sBOMB] = secret_cpy[99].d1;
        s->secretcset[sBOMB] = secret_cpy[99].fg;
        s->secretflag[sBOMB] = secret_cpy[99].d2;
        
        s->secretcombo[sSBOMB] = secret_cpy[100].d1;
        s->secretcset[sSBOMB] = secret_cpy[100].fg;
        s->secretflag[sSBOMB] = secret_cpy[100].d2;
        
        for(int i=0; i<3; i++)
        {
            s->secretcombo[sBRANG+i] = secret_cpy[101+i].d1;
            s->secretcset[sBRANG+i] = secret_cpy[101+i].fg;
            s->secretflag[sBRANG+i] = secret_cpy[101+i].d2;
        }
        
        for(int i=0; i<2; i++)
        {
            s->secretcombo[sWANDMAGIC+i] = secret_cpy[104+i].d1;
            s->secretcset[sWANDMAGIC+i] = secret_cpy[104+i].fg;
            s->secretflag[sWANDMAGIC+i] = secret_cpy[104+i].d2;
        }
        
        for(int i=0; i<8; i++)
        {
            s->secretcombo[sSWORD+i] = secret_cpy[106+i].d1;
            s->secretcset[sSWORD+i] = secret_cpy[106+i].fg;
            s->secretflag[sSWORD+i] = secret_cpy[106+i].d2;
        }
        
        s->secretcombo[sSTAIRS] = secret_cpy[114].d1;
        s->secretcset[sSTAIRS] = secret_cpy[114].fg;
        s->secretflag[sSTAIRS] = secret_cpy[114].d2;
        
        s->secretcombo[sREFFIREBALL] = secret_cpy[115].d1;
        s->secretcset[sREFFIREBALL] = secret_cpy[115].fg;
        s->secretflag[sREFFIREBALL] = secret_cpy[115].d2;
        
        for(int i=0; i<4; i++)
        {
            s->secretcombo[sHOOKSHOT+i] = secret_cpy[116+i].d1;
            s->secretcset[sHOOKSHOT+i] = secret_cpy[116+i].fg;
            s->secretflag[sHOOKSHOT+i] = secret_cpy[116+i].d2;
        }
        
        for(int i=0; i<16; i++)
        {
            s->secretcombo[sSECRET01+i] = secret_cpy[120+i].d1;
            s->secretcset[sSECRET01+i] = secret_cpy[120+i].fg;
            s->secretflag[sSECRET01+i] = secret_cpy[120+i].d2;
        }
        
    }
    
    comeback();
	delete[] secret_cpy;
    return D_O_K;
}

static DIALOG under_dlg[] =
{
    /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp) */
    { jwin_win_proc,     72,   60,   176+1,120+1,vc(14),  vc(1),  0,       D_EXIT,     0,             0, (void *) "Under Combo", NULL, NULL },
    { jwin_text_proc,    115,  83,   20,   20,   vc(14),  vc(1),  0,       0,          0,             0, (void *) "Current", NULL, NULL },
    { d_comboframe_proc, 122,  92,   20,   20,   0,       0,      0,       0,          FR_DEEP,       0,       NULL, NULL, NULL },
    { d_combo_proc,      124,  94,   16,   16,   0,       0,      0,       D_NOCLICK,  0,             0,       NULL, NULL, NULL },
    { jwin_text_proc,    184,  83,   20,   20,   vc(14),  vc(1),  0,       0,          0,             0, (void *) "New", NULL, NULL },
    { d_comboframe_proc, 182,  92,   20,   20,   0,       0,      0,       0,          FR_DEEP,       0,       NULL, NULL, NULL },
    { d_combo_proc,      184,  94,   16,   16,   0,       0,      0,       0,          0,             0,       NULL, NULL, NULL },
    { jwin_button_proc,  90,   124,  61,   21,   vc(14),  vc(1),  's',     D_EXIT,     0,             0, (void *) "&Set", NULL, NULL },
    { jwin_button_proc,  170,  124,  61,   21,   vc(14),  vc(1),  'c',     D_EXIT,     0,             0, (void *) "&Cancel", NULL, NULL },
    { jwin_button_proc,  90,   152,  61,   21,   vc(14),  vc(1),  'a',     D_EXIT,     0,             0, (void *) "Set &All", NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,       0,      0,       0,          KEY_F1,        0, (void *) onHelp, NULL, NULL },
    { d_timer_proc,      0,    0,    0,    0,    0,       0,      0,       0,          0,             0,         NULL, NULL, NULL },
    { NULL,              0,    0,    0,    0,    0,       0,      0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

int onUnderCombo()
{
    under_dlg[0].dp2 = lfont;
    
    under_dlg[3].d1=Map.CurrScr()->undercombo;
    under_dlg[3].fg=Map.CurrScr()->undercset;
    
    under_dlg[6].d1=Combo;
    under_dlg[6].fg=CSet;

	DIALOG *under_cpy = resizeDialog(under_dlg, 1.5);
    
    if(is_large())
    {
        // Doesn't place "New" and "Current" text too well
		under_cpy[1].x=342;
		under_cpy[4].x=438;
    }
    
    int ret = zc_popup_dialog(under_cpy,-1);
    
    if(ret==7)
    {
        saved=false;
        Map.CurrScr()->undercombo = under_cpy[6].d1;
        Map.CurrScr()->undercset = under_cpy[6].fg;
    }
    
    if(ret==9 && jwin_alert("Confirm Overwrite","Set all Under Combos","on this map?",NULL,"&Yes","&No",'y','n',lfont)==1)
    {
        saved=false;
        
        for(int i=0; i<128; i++)
        {
            Map.Scr(i)->undercombo = under_cpy[6].d1;
            Map.Scr(i)->undercset = under_cpy[6].fg;
        }
    }
    
	delete[] under_cpy;
    return D_O_K;
}

static DIALOG list_dlg[] =
{
    // (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp)
    { jwin_win_proc,     60-12,   40,   200+24,  148,  vc(14),  vc(1),  0,       D_EXIT,          0,             0,       NULL, NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { jwin_list_proc,       72-12-4,   60+4,   176+24+8,  92+3,   jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,       D_EXIT,     0,             0,       NULL, NULL, NULL },
    { jwin_button_proc,     90,   163,  61,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "OK", NULL, NULL },
    { jwin_button_proc,     170,  163,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

void ilist_rclick_func(int index, int x, int y);
DIALOG ilist_dlg[] =
{
    // (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp)
    { jwin_win_proc,     60-12,   40,   200+24+24,  148,  vc(14),  vc(1),  0,       D_EXIT,          0,   0,       NULL, NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,            0,       NULL, NULL, NULL },
    { d_ilist_proc,       72-12-4,   60+4,   176+24+8,  92+3,   jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG], 0, D_EXIT, 0,  0,  NULL, NULL, NULL },
    { jwin_button_proc,     90,   163,  61,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "Edit", NULL, NULL },
    { jwin_button_proc,     170,  163,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Done", NULL, NULL },
    { jwin_button_proc,     220,   163,  61,   21,   vc(14),  vc(1),  13,     D_EXIT,     0,             0, (void *) "Edit", NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL, NULL,  NULL }
};

static DIALOG wlist_dlg[] =
{
    // (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp)
    { jwin_win_proc,     60-12,   40,   200+24+24,  148,  vc(14),  vc(1),  0,       D_EXIT,          0,             0,       NULL, NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { d_wlist_proc,       72-12-4,   60+4,   176+24+8,  92+3,   jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,       D_EXIT,     0,             0,       NULL, NULL, NULL },
    { jwin_button_proc,     90,   163,  61,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "Edit", NULL, NULL },
    { jwin_button_proc,     170,  163,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Done", NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

static DIALOG rlist_dlg[] =
{
    // (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp)
    { jwin_win_proc,     60-12,   40,   200+24,  148,  vc(14),  vc(1),  0,       D_EXIT,          0,             0,       NULL, NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { jwin_abclist_proc,       72-12-4,   60+4,   176+24+8,  92+3,   jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,       D_EXIT,     0,             0,       NULL, NULL, NULL },
    { jwin_button_proc,     70,   163,  51,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "OK", NULL, NULL },
    { jwin_button_proc,     190,  163,  51,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { jwin_button_proc,     130,  163,  51,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Help", NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};



/*
  typedef struct item_struct {
  char *s;
  int i;
  } item_struct;
  */
item_struct bii[iMax+1];
int bii_cnt=-1;

void build_bii_list(bool usenone)
{
    int start=bii_cnt=0;
    
    if(usenone)
    {
        bii[0].s = (char *)"(None)";
        bii[0].i = -2;
        bii_cnt=1;
        start=0;
    }
    
    for(int i=start; i<iMax; i++)
    {
        bii[bii_cnt].s = item_string[i];
        bii[bii_cnt].i = i;
        ++bii_cnt;
    }
    
    for(int i=start; i<bii_cnt-1; i++)
    {
        for(int j=i+1; j<bii_cnt; j++)
        {
            if(stricmp(bii[i].s,bii[j].s)>0 && strcmp(bii[j].s,""))
            {
                zc_swap(bii[i],bii[j]);
            }
        }
    }
}

const char *itemlist(int index, int *list_size)
{
    if(index<0)
    {
        *list_size = bii_cnt;
        return NULL;
    }
    
    return bii[index].s;
}

// disable items on dmaps stuff
int DI[iMax];
int nDI;

void initDI(int index)
{
    int j=0;
    
    for(int i=0; i<iMax; i++)
    {
        int index1=bii[i].i; // true index of item in dmap's DI list
        
        if(DMaps[index].disableditems[index1])
        {
            DI[j]=i;
            j++;
        }
    }
    
    nDI=j;
    
    for(int i=j; i<iMax; i++) DI[j]=0;
    
    return;
}

void insertDI(int id, int index)
{
    int trueid=bii[id].i;
    DMaps[index].disableditems[trueid] |= 1; //bit set
    initDI(index);
    return;
}

void deleteDI(int id, int index)
{
    int i=DI[id];
    int trueid=bii[i].i;
    DMaps[index].disableditems[trueid] &= (~1); // bit clear
    initDI(index);
    return;
}

const char *DIlist(int index, int *list_size)
{
    if(index<0)
    {
        *list_size = nDI;
        return NULL;
    }
    
    int i=DI[index];
    return bii[i].s;
    
}

int select_item(const char *prompt,int item,bool is_editor,int &exit_status)
{
    int index=0;
    
    for(int j=0; j<bii_cnt; j++)
    {
        if(bii[j].i == item)
        {
            index=j;
        }
    }
    
    ilist_dlg[0].dp=(void *)prompt;
    ilist_dlg[0].dp2=lfont;
    ilist_dlg[2].d1=index;
    ListData item_list(itemlist, &font);
    ilist_dlg[2].dp=(void *) &item_list;

	DIALOG *ilist_cpy = resizeDialog(ilist_dlg, 1.5);
        
    if(is_editor)
    {
		ilist_cpy[2].dp3 = (void *)&ilist_rclick_func;
		ilist_cpy[2].flags|=(D_USER<<1);
		ilist_cpy[3].dp = (void *)"Edit";
		ilist_cpy[4].dp = (void *)"Done";
		ilist_cpy[3].x = is_large()?285:90;
		ilist_cpy[4].x = is_large()?405:170;
		ilist_cpy[5].flags |= D_HIDDEN;
    }
    else
    {
		ilist_cpy[2].dp3 = NULL;
		ilist_cpy[2].flags&=~(D_USER<<1);
		ilist_cpy[3].dp = (void *)"OK";
		ilist_cpy[4].dp = (void *)"Cancel";
		ilist_cpy[3].x = is_large()?240:60;
		ilist_cpy[4].x = is_large()?350:135;
		ilist_cpy[5].flags &= ~D_HIDDEN;
    }
    
    exit_status=zc_popup_dialog(ilist_cpy,2);
    
    if(exit_status==0||exit_status==4)
    {
		Backend::mouse->setWheelPosition(0);
		delete[] ilist_cpy;
        return -1;
    }
    
    index = ilist_cpy[2].d1;
	delete[] ilist_cpy;
	Backend::mouse->setWheelPosition(0);
    return bii[index].i;
}

weapon_struct biw[wMAX];
int biw_cnt=-1;

void build_biw_list()
{
    int start=biw_cnt=0;
    
    for(int i=start; i<wMAX; i++)
    {
        biw[biw_cnt].s = (char *)weapon_string[i];
        biw[biw_cnt].i = i;
        ++biw_cnt;
    }
    
    for(int i=start; i<biw_cnt-1; i++)
    {
        for(int j=i+1; j<biw_cnt; j++)
            if(stricmp(biw[i].s,biw[j].s)>0 && strcmp(biw[j].s,""))
                zc_swap(biw[i],biw[j]);
    }
}

const char *weaponlist(int index, int *list_size)
{
    if(index<0)
    {
        *list_size = biw_cnt;
        return NULL;
    }
    
    return biw[index].s;
}

int select_weapon(const char *prompt,int weapon)
{
    if(biw_cnt==-1)
        build_biw_list();
        
    int index=0;
    
    for(int j=0; j<biw_cnt; j++)
    {
        if(biw[j].i == weapon)
        {
            index=j;
        }
    }
    
    wlist_dlg[0].dp=(void *)prompt;
    wlist_dlg[0].dp2=lfont;
    wlist_dlg[2].d1=index;
    ListData weapon_list(weaponlist, &font);
    wlist_dlg[2].dp=(void *) &weapon_list;

	DIALOG *wlist_cpy = resizeDialog(wlist_dlg, 1.5);
        
    int ret=zc_popup_dialog(wlist_cpy,2);
    
    if(ret==0||ret==4)
    {
		Backend::mouse->setWheelPosition(0);
		delete[] wlist_cpy;
        return -1;
    }
    
    index = wlist_cpy[2].d1;
	delete[] wlist_cpy;
	Backend::mouse->setWheelPosition(0);
    return biw[index].i;
}


item_struct bir[rMAX];
int bir_cnt=-1;

void build_bir_list()
{
    bir_cnt=0;
    
    for(int i=0; i<rMAX; i++)
    {
        if(roomtype_string[i][0]!='-')
        {
            bir[bir_cnt].s = (char *)roomtype_string[i];
            bir[bir_cnt].i = i;
            ++bir_cnt;
        }
    }
    
    for(int i=0; i<bir_cnt-1; i++)
    {
        for(int j=i+1; j<bir_cnt; j++)
        {
            if(stricmp(bir[i].s,bir[j].s)>0)
            {
                zc_swap(bir[i],bir[j]);
            }
        }
    }
}

const char *roomlist(int index, int *list_size)
{
    if(index<0)
    {
        *list_size = bir_cnt;
        return NULL;
    }
    
    return bir[index].s;
}

int select_room(const char *prompt,int room)
{
    if(bir_cnt==-1)
    {
        build_bir_list();
    }
    
    int index=0;
    
    for(int j=0; j<bir_cnt; j++)
    {
        if(bir[j].i == room)
        {
            index=j;
        }
    }
    
    rlist_dlg[0].dp=(void *)prompt;
    rlist_dlg[0].dp2=lfont;
    rlist_dlg[2].d1=index;
    ListData room_list(roomlist, &font);
    rlist_dlg[2].dp=(void *) &room_list;

	DIALOG *rlist_cpy = resizeDialog(rlist_dlg, 1.5);
    
    int ret;
    
    do
    {
        ret=zc_popup_dialog(rlist_cpy,2);
        
        if(ret==5)
        {
            int id = bir[rlist_cpy[2].d1].i;
            
            switch(id)
            {
            case rSP_ITEM:
                jwin_alert(roomtype_string[id],"If a Guy is set, he will offer an item to Link.","Also used for Item Cellar warps, and","'Armos/Chest->Item' and 'Dive For Item' combo flags.","O&K",NULL,'k',0,lfont);
                break;
                
            case rINFO:
                jwin_alert(roomtype_string[id],"Pay rupees to make one of three strings appear.","Strings and prices are set in","Misc. Data -> Info Types.","O&K",NULL,'k',0,lfont);
                break;
                
            case rMONEY:
                jwin_alert(roomtype_string[id],"If a Guy is set, he will offer rupees to Link.",NULL,NULL,"O&K",NULL,'k',0,lfont);
                break;
                
            case rGAMBLE:
                jwin_alert(roomtype_string[id],"The 'Money-Making Game' from The Legend of Zelda.","Risk losing up to 40 rupees for a","chance to win up to 50 rupees.","O&K",NULL,'k',0,lfont);
                break;
                
            case rREPAIR:
                jwin_alert(roomtype_string[id],"When the Guy's String vanishes,","Link loses a given amount of rupees.",NULL,"O&K",NULL,'k',0,lfont);
                break;
                
            case rRP_HC:
                jwin_alert(roomtype_string[id],"The Guy offers item 28 and item 30 to Link.","Taking one makes the other vanish forever.",NULL,"O&K",NULL,'k',0,lfont);
                break;
                
            case rGRUMBLE:
                jwin_alert(roomtype_string[id],"The Guy and his invisible wall won't vanish","until Link uses (and thus loses) a Bait item.","(Shutters won't open until the Guy vanishes, too.)","O&K",NULL,'k',0,lfont);
                break;
                
            case rTRIFORCE:
                jwin_alert(roomtype_string[id],"The Guy and his invisible wall won't vanish","unless Link has Triforces from levels 1-8.","(Shutters won't open until the Guy vanishes, too.)","O&K",NULL,'k',0,lfont);
                break;
                
            case rP_SHOP:
                jwin_alert(roomtype_string[id],"Similar to a Shop, but the items and String","won't appear until Link uses a Letter item.","(Or, if Link already has a Level 2 Letter item.)","O&K",NULL,'k',0,lfont);
                break;
                
            case rSHOP:
                jwin_alert(roomtype_string[id],"The Guy offers three items for a fee.","You can use the Shop as often as you want.","Items and prices are set in Misc. Data -> Shop Types.","O&K",NULL,'k',0,lfont);
                break;
                
            case rBOMBS:
                jwin_alert(roomtype_string[id],"The Guy offers to increase Link's Bombs","and Max. Bombs by 4, for a fee.","You can only buy it once.","O&K",NULL,'k',0,lfont);
                break;
                
            case rSWINDLE:
                jwin_alert(roomtype_string[id],"The Guy and his invisible wall won't vanish","until Link pays the fee or forfeits a Heart Container.","(Shutters won't open until the Guy vanishes, too.)","O&K",NULL,'k',0,lfont);
                break;
                
            case r10RUPIES:
                jwin_alert(roomtype_string[id],"10 instances of item 0 appear in a","diamond formation in the center of the screen.","No Guy or String needs to be set for this.","O&K",NULL,'k',0,lfont);
                break;
                
            case rWARP:
                jwin_alert(roomtype_string[id],"All 'Stair [A]' type combos send Link to","a destination in a given Warp Ring, based","on the combo's X position (<112, >136, or between).","O&K",NULL,'k',0,lfont);
                break;
                
            case rGANON:
                jwin_alert(roomtype_string[id],"Link holds up the Triforce, and Ganon appears.",NULL,"(Unless the current DMap's Dungeon Boss was beaten.)","O&K",NULL,'k',0,lfont);
                break;
                
            case rZELDA:
                jwin_alert(roomtype_string[id],"Four instances of enemy 85 appear","on the screen in front of the Guy.","(That's all it does.)","O&K",NULL,'k',0,lfont);
                break;
                
            case rMUPGRADE:
                jwin_alert(roomtype_string[id],"When the Guy's String finishes,","Link gains the 1/2 Magic Usage attribute.",NULL,"O&K",NULL,'k',0,lfont);
                break;
                
            case rLEARNSLASH:
                jwin_alert(roomtype_string[id],"When the Guy's String finishes,","Link gains the Slash attribute.",NULL,"O&K",NULL,'k',0,lfont);
                break;
                
            case rARROWS:
                jwin_alert(roomtype_string[id],"The Guy offers to increase Link's Arrows","and Max. Arrows by 10, for a fee.","You can only buy it once.","O&K",NULL,'k',0,lfont);
                break;
                
            case rTAKEONE:
                jwin_alert(roomtype_string[id],"The Guy offers three items.","Taking one makes the others vanish forever.","Item choices are set in Misc. Data -> Shop Types.","O&K",NULL,'k',0,lfont);
                break;
                
            default:
                jwin_alert("Help","Select a Room Type, then click","Help to find out what it does.",NULL,"O&K",NULL,'k',0,lfont);
                break;
            }
        }
    }
    while(ret==5);
    
    if(ret==0||ret==4)
    {
		Backend::mouse->setWheelPosition(0);
		delete[] rlist_cpy;
        return -1;
    }
    else
        index = rlist_cpy[2].d1;

	delete[] rlist_cpy;
        
	Backend::mouse->setWheelPosition(0);
    return bir[index].i;
}

static MENU seldata_rclick_menu[] =
{
    { (char *)"Copy",  NULL, NULL, 0, NULL },
    { (char *)"Paste", NULL, NULL, 0, NULL },
    { NULL,            NULL, NULL, 0, NULL }
};

static int seldata_copy;
static void (*seldata_paste_func)(int, int);

void seldata_rclick_func(int index, int x, int y)
{
    if(seldata_copy<0)
        seldata_rclick_menu[1].flags|=D_DISABLED;
    else
        seldata_rclick_menu[1].flags&=~D_DISABLED;
    
    int ret=popup_menu(seldata_rclick_menu, x, y);
    
    if(ret==0) // copy
        seldata_copy=index;
    else if(ret==1) // paste
    {
        seldata_paste_func(seldata_copy, index);
        saved=false;
    }
}

int select_data(const char *prompt,int index,const char *(proc)(int,int*), FONT *title_font, void (*copyFunc)(int, int))
{
    if(proc==NULL)
        return -1;
    
    list_dlg[0].dp=(void *)prompt;
    list_dlg[0].dp2=title_font;
    list_dlg[2].d1=index;
    ListData select_list(proc, &font);
    list_dlg[2].dp=(void *) &select_list;

	DIALOG *list_cpy = resizeDialog(list_dlg, 1.5);
    
    seldata_copy=-1;
    seldata_paste_func=copyFunc;
    if(copyFunc)
    {
		list_cpy[2].flags|=D_USER<<1;
		list_cpy[2].dp3=(void*)seldata_rclick_func;
    }
    else
    {
		list_cpy[2].flags&=~(D_USER<<1);
		list_cpy[2].dp3=0;
    }
    
    int ret=zc_popup_dialog(list_cpy,2);
    
    if(ret==0||ret==4)
    {
		Backend::mouse->setWheelPosition(0);
		delete[] list_cpy;
        return -1;
    }
    
	int retval = list_cpy[2].d1;
	delete[] list_cpy;
	return retval;
}

int select_data(const char *prompt,int index,const char *(proc)(int,int*), const char *b1, const char *b2, FONT *title_font, void (*copyFunc)(int, int))
{
    if(proc==NULL)
        return -1;
        
    list_dlg[0].dp=(void *)prompt;
    list_dlg[0].dp2=title_font;
    list_dlg[2].d1=index;
    ListData select_data_list(proc, &font);
    list_dlg[2].dp=(void *) &select_data_list;
    list_dlg[3].dp=(void *)b1;
    list_dlg[4].dp=(void *)b2;

	DIALOG *list_cpy = resizeDialog(list_dlg, 1.5);
    
    seldata_copy=-1;
    seldata_paste_func=copyFunc;
    if(copyFunc)
    {
		list_cpy[2].flags|=D_USER<<1;
		list_cpy[2].dp3=(void*)seldata_rclick_func;
    }
    else
    {
		list_cpy[2].flags&=~(D_USER<<1);
		list_cpy[2].dp3=0;
    }
    
    int ret = zc_popup_dialog(list_cpy,2);
	list_cpy[3].dp=(void *) "OK";
	list_cpy[4].dp=(void *) "Cancel";
    
    if(ret==0||ret==4)
    {
		Backend::mouse->setWheelPosition(0);
		delete[] list_cpy;
        return -1;
    }
    
	Backend::mouse->setWheelPosition(0);
	int retval = list_dlg[2].d1;
	delete[] list_cpy;
	return retval;
}

static int edit_scrdata1[] = // Flags 1
{
    //6,8,10,11,12,15,18,19,21,22,24,37,57,59,60,-1
    118,45,46,57,  119,21,58,22,24,54,55,8, //Ordered as they are on the dialog
    120,6,43,47,50,  121,37,42,12,135,23,  122,18,56,  -1
};

static int edit_scrdata3[] = // Flags 2
{
    //38,39,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,58,61,-1
    123,15,19,41,51,  124,38,39,48,49, 125,52,53,
    126,10,59,60,  127,11,44,128,129,130,131,132,  -1
};

static int edit_scrdata5[] = // Enemies
{
    7,16,17,25,36,107,108,109,110,111,112,113,20,114,115,116,-1
};

static int edit_scrdata2[] = // Data 1
{
    31,32,33,34,35,62,63,64,65,66,67,68,69,70,71,72,73,
    74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,
    93,94,97,98,133,134,-1
};

static int edit_scrdata4[] = // Data 2
{
    14,95,96,99,100,101,102,103,104,105,106,-1
};

static int edit_scrdata6[] = // Timed Warp
{
    26, 27, 28,29,30,40,117,-1
};

static TABPANEL scrdata_tabs[] =
{
    { (char *)"S.Flags 1", D_SELECTED, edit_scrdata1, 0, NULL },
    { (char *)"S.Flags 2", 0,          edit_scrdata3, 0, NULL },
    { (char *)"E.Flags",   0,          edit_scrdata5, 0, NULL },
    { (char *)"S.Data 1",  0,          edit_scrdata2, 0, NULL },
    { (char *)"S.Data 2",  0,          edit_scrdata4, 0, NULL },
    { (char *)"T.Warp",    0,          edit_scrdata6, 0, NULL },
    { NULL,                0,                   NULL, 0, NULL }
};


static char sfx_str_buf[42];

const char *sfxlist(int index, int *list_size)
{
    if(index>=0)
    {
        const SFXSample *sample = Backend::sfx->getSample(index);
        snprintf(sfx_str_buf, 42, "%d: %s",index, sample ? sample->name.c_str() : "(None)");
        return sfx_str_buf;
    }
    
    *list_size=Backend::sfx->numSlots();
    return NULL;
}

static char lenseffect_str_buf[15];

const char *lenseffectlist(int index, int *list_size)
{
    if(index>=0)
    {
        bound(index,0,12);
        
        if(index==0)
        {
            sprintf(lenseffect_str_buf,"Normal");
        }
        else if(index<7)
        {
            sprintf(lenseffect_str_buf,"Hide layer %d", index);
        }
        else
        {
            sprintf(lenseffect_str_buf,"Reveal layer %d", index-6);
        }
        
        return lenseffect_str_buf;
    }
    
    *list_size=13;
    return NULL;
}

static ListData nextmap_list(nextmaplist, &font);
static ListData ns_list(nslist,&font);
static ListData screenmidi_list(screenmidilist, &font);
static ListData sfx_list(sfxlist, &font);
static ListData lenseffect_list(lenseffectlist, &font);

static DIALOG scrdata_dlg[] =
{
    /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp) */
    { jwin_win_proc,       4,    53-29,   304+1+5+6,  156+1+38+7+10, vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "Screen Data", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    // 2
    { jwin_button_proc,     90,   176+37,  61,   21,   vc(14),  vc(1),  'k',     D_EXIT,     0,             0, (void *) "O&K", NULL, NULL },
    { jwin_button_proc,     170,  176+37,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { d_keyboard_proc,        0,    0,    0,    0,    0,       0,      0,       0,          KEY_F1,        0, (void *) onHelp, NULL, NULL },
    { jwin_tab_proc,        7,   46,   295+15,  147+17,    vc(14),   vc(1),      0,      0,          1,             0, (void *) scrdata_tabs, NULL, (void *)scrdata_dlg },
    // 6
    { jwin_check_proc,      165,   78,   160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Block->Shutters", NULL, NULL },
    //Moved to E. Flags
    { jwin_check_proc,      165,  148,   160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Enemies->Item", NULL, NULL },
    
    { jwin_check_proc,      15,   178,   160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Dark Room", NULL, NULL },
    { d_dummy_proc,         160,  56-24,     0,  8,    vc(15),  vc(1),  0,       0,          0,             0,       NULL, NULL, NULL },
    //S.Flags 2
    { jwin_check_proc,     165,   78,   160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Whistle->Stairs", NULL, NULL },
    { jwin_check_proc,     165,   118,   160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Toggle 'Allow Ladder'", NULL, NULL },
    
    { jwin_check_proc,     165,   148,   160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Use Maze Path", NULL, NULL },
    { d_dummy_proc,         160,  56-24,     0,  8,    vc(15),  vc(1),  0,       0,          0,             0,       NULL, NULL, NULL },
    //S.Data 2
    { jwin_check_proc,     140,   168,   160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Play Secret SFX On Screen Entry", NULL, NULL },
    
    { jwin_check_proc,      15,   78,   160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Combos Affect Midair Link", NULL, NULL },
    //E.Flags
    { jwin_check_proc,      15,   178,   160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Traps Ignore Walkability", NULL, NULL },
    { jwin_check_proc,      165,  158,   160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Enemies->Secret", NULL, NULL },
    //18
    { jwin_check_proc,      165, 188,   160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Hold Up Item", NULL, NULL },
    //S.Flags 2
    { jwin_check_proc,      15,   88,   160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Cycle Combos On Screen Init", NULL, NULL },
    //E. Flags
    { jwin_check_proc,      15,   158,  160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "All Enemies Are Invisible", NULL, NULL },
    
    { jwin_check_proc,      15,   118,  160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Invisible Link", NULL, NULL },
    { jwin_check_proc,      15,   138,  160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "No Subscreen", NULL, NULL },
    
    { jwin_check_proc,      165,  168,  160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Sprites Carry Over In Warps", NULL, NULL },
    
    { jwin_check_proc,       15,  148,  160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "...But Don't Offset Screen", NULL, NULL },
    //E. Flags
    { jwin_check_proc,      165,  138,  160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Enemies Always Return", NULL, NULL },
    
    // These five now appear on the Timed Warp tab.
    { jwin_check_proc,      15,  118,  160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Timed Warps are Direct", NULL, NULL },
    { jwin_check_proc,      15,  128,  160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Secrets Disable Timed Warp", NULL, NULL },
    // 28
    { jwin_text_proc,       15,   88,  128,    8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Timed Warp Ticks:", NULL, NULL },
    { d_ticsedit_proc,      15,   98,  36,      16,    vc(12),  vc(1),  0,       0,          5,             0,       NULL, NULL, NULL },
    { jwin_text_proc,17+2+36+1, 98+4,   0,   8,    vc(11),  vc(1),  0,       0,          0,             0,       NULL, NULL, NULL },
    
    { jwin_text_proc,          15,   68,     200,    8,    vc(14),   vc(1),      0,      0,          0,             0, (void *) "Screen State Carry Over:", NULL, NULL },
    { jwin_text_proc,          15,   88,     72,    8,    vc(14),   vc(1),      0,      0,          0,             0, (void *) "Next Map:", NULL, NULL },
    { jwin_text_proc,          15,   106,     96,    8,   vc(14),   vc(1),      0,      0,          0,             0, (void *) "Next Screen:", NULL, NULL },
    { jwin_droplist_proc,      90,   84,   54,   16,   jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,       0,          1,             0, (void *) &nextmap_list, NULL, NULL },
//  { jwin_edit_proc,          90,    84,    32-6,   16,  vc(12),   vc(1),  0,       0,          3,             0,   NULL, NULL, NULL },
    { jwin_droplist_proc,      90,   102,   54,   16,   jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,       0,          1,             0, (void *) &ns_list, NULL, NULL },
// { jwin_edit_proc,       17,   114,   32-6,   16,    vc(12),  vc(1),  0,       0,          2,             0,       NULL, NULL, NULL },
    // { d_hexedit_proc,      97,   102,   24-3,   16,    vc(12),  vc(1),  0,       0,          2,             0,       NULL, NULL, NULL },
    
    //Moved to E Flags
    { jwin_check_proc,      165,  168,  160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Enemies->Secret is Permanent", NULL, NULL },
    
    { jwin_check_proc,     165,  128,  160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Auto-Warps are Direct", NULL, NULL },
    //38
    { jwin_check_proc,      15,  128,  160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Save Point->Continue Here", NULL, NULL },
    { jwin_check_proc,      15,  138,  160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Save Game On Entry", NULL, NULL },
    // This now appears on the Timed Warp tab.
    { jwin_check_proc,      15,  138,  160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Timed Warp Is Random (A, B, C or D)", NULL, NULL },
    
    { jwin_check_proc,      15,   98,  160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Damage Combos Ignore Boots", NULL, NULL },
    //S.Flags 1
    { jwin_check_proc,     165,  138,  160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Sensitive Warps are Direct", NULL, NULL },
    { jwin_check_proc,     165,   88,  160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Secrets are Temporary", NULL, NULL },
    //44
    { jwin_check_proc,     165,  128,  160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Toggle 'No Diving'", NULL, NULL },
    //S.Flags 1
    { jwin_check_proc,      15,   78,  160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Treat as Interior Screen", NULL, NULL },
    { jwin_check_proc,      15,   88,  160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Treat as NES Dungeon Screen", NULL, NULL },
    { jwin_check_proc,     165,   98,  160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Hit All Triggers->Perm Secret", NULL, NULL },
    
    { jwin_check_proc,      15,  148,  160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Continue Here", NULL, NULL },
    { jwin_check_proc,      15,  158,  160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "No Continue Here After Warp", NULL, NULL },
    //50, S.Flags 1
    { jwin_check_proc,      165, 108,  160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Hit All Triggers->16-31", NULL, NULL },
    
    { jwin_check_proc,       15, 108,  160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Toggle Rings Affect Combos", NULL, NULL },
    { jwin_check_proc,       15, 178,  160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "FF Combos Wrap Around", NULL, NULL },
    { jwin_check_proc,       15, 188,  160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "No FFC Carryover", NULL, NULL },
    //S.Flags 1
    { jwin_check_proc,      15,  168,  160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Layer 3 Is Background", NULL, NULL },
    { jwin_check_proc,      15,  158,  160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Layer 2 Is Background", NULL, NULL },
    { jwin_check_proc,      165,  198,  160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Item Falls From Ceiling", NULL, NULL },
    { jwin_check_proc,      15,    98,  160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Sideview Gravity", NULL, NULL },
    { jwin_check_proc,      15,   128,  160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "No Link Marker in Minimap", NULL, NULL },
    //S. Flags 2
    { jwin_check_proc,      165,   88,  160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Whistle->Palette Change", NULL, NULL },
    { jwin_check_proc,      165,   98,  160+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Whistle->Dry Lake", NULL, NULL },
    { d_dummy_proc,         160,  56-24,     0,  8,    vc(15),  vc(1),  0,       0,          0,             0,       NULL, NULL, NULL },
    //62
    { jwin_ctext_proc,       225,   68,  180,    8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "No Reset     /   No Carry Over", NULL, NULL },
    { jwin_ctext_proc,       225,   78,  140,    8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Secrets", NULL, NULL },
    { jwin_ctext_proc,       225,   88,  140,    8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Item", NULL, NULL },
    { jwin_ctext_proc,       225,   98,  140,    8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Special Item", NULL, NULL },
    { jwin_ctext_proc,       225,   108,  140,    8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Lock Block", NULL, NULL },
    { jwin_ctext_proc,       225,   118,  140,    8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Boss Lock Block", NULL, NULL },
    { jwin_ctext_proc,       225,   128,  140,    8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Chest", NULL, NULL },
    { jwin_ctext_proc,       225,   138,  140,    8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Locked Chest", NULL, NULL },
    { jwin_ctext_proc,       225,   148,  140,    8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Boss Locked Chest", NULL, NULL },
    { jwin_ctext_proc,       225,   168,  140,    8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Door Down(D)", NULL, NULL },
    { jwin_ctext_proc,       225,   178,  140,    8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Door Left(D)", NULL, NULL },
    { jwin_ctext_proc,       225,   188,  140,    8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Door Right(D)", NULL, NULL },
    //74
    { jwin_check_proc,      160,  78,  8+1,  8+1,       vc(14),        vc(1),              0,  0,    1,             0,       NULL, NULL, NULL },
    { jwin_check_proc,      160,  88,  8+1,  8+1,       vc(14),        vc(1),              0,  0,          1,             0,       NULL, NULL, NULL },
    { jwin_check_proc,      160,  98,  8+1,  8+1,       vc(14),        vc(1),              0,  0,          1,             0,       NULL, NULL, NULL },
    { jwin_check_proc,      160,  108,  8+1,  8+1,      vc(14),        vc(1),              0,  0,          1,             0,       NULL, NULL, NULL },
    { jwin_check_proc,      160,  118,  8+1,  8+1,      vc(14),        vc(1),              0,  0,          1,             0,       NULL, NULL, NULL },
    { jwin_check_proc,      160,  128,  8+1,  8+1,      vc(14),        vc(1),              0,  0,          1,             0,       NULL, NULL, NULL },
    { jwin_check_proc,      160,  138,  8+1,  8+1,      vc(14),        vc(1),              0,  0,          1,             0,       NULL, NULL, NULL },
    { jwin_check_proc,      160,  148,  8+1,  8+1,      vc(14),        vc(1),              0,  0,          1,             0,       NULL, NULL, NULL },
    { jwin_check_proc,      160,  168,  8+1,  8+1,      vc(14),        vc(1),              0,  0,          1,             0,       NULL, NULL, NULL },
    { jwin_check_proc,      160,  178,  8+1,  8+1,      vc(14),        vc(1),              0,  0,          1,             0,       NULL, NULL, NULL },
    { jwin_check_proc,      160,  188,  8+1,  8+1,      vc(14),        vc(1),              0,  0,          1,             0,       NULL, NULL, NULL },
    { jwin_check_proc,      280,  78,  8+1,  8+1,       vc(14),        vc(1),              0,  0,          1,             0,       NULL, NULL, NULL },
    { jwin_check_proc,      280,  88,  8+1,  8+1,       vc(14),        vc(1),              0,  0,          1,             0,       NULL, NULL, NULL },
    { jwin_check_proc,      280,  98,  8+1,  8+1,       vc(14),        vc(1),              0,  0,          1,             0,       NULL, NULL, NULL },
    { jwin_check_proc,      280,  108,  8+1,  8+1,      vc(14),        vc(1),              0,  0,  1,             0,       NULL, NULL, NULL },
    { jwin_check_proc,      280,  118,  8+1,  8+1,      vc(14),        vc(1),              0,  0,  1,             0,       NULL, NULL, NULL },
    { jwin_check_proc,      280,  128,  8+1,  8+1,      vc(14),        vc(1),              0,  0,  1,             0,       NULL, NULL, NULL },
    { jwin_check_proc,      280,  138,  8+1,  8+1,      vc(14),        vc(1),              0,  0,  1,  0,       NULL, NULL, NULL },
    { jwin_check_proc,      280,  148,  8+1,  8+1,      vc(14),        vc(1),              0,  0,  1,  0,       NULL, NULL, NULL },
    //93
    { jwin_text_proc,       17,  130,    120,   8,      vc(11),        vc(1),              0,  0,  0,  0, (void *) "Screen MIDI:", NULL, NULL },
    { jwin_droplist_proc,   17,  138,   133,   16, jwin_pal[jcTEXTFG], jwin_pal[jcTEXTBG], 0,  0,  1,  0, (void *) &screenmidi_list, NULL, NULL },
    { jwin_text_proc,       17,   68,   200,    8,      vc(14),        vc(1),              0,  0,  0,  0, (void *) "Damage Combo Sensitivity:", NULL, NULL },
    { jwin_edit_proc,      140,   66,    40,   16,      vc(12),        vc(1),              0,  0,  1,  0,       NULL, NULL, NULL },
    //97
    { jwin_ctext_proc,     225,   158,  140,     8,     vc(14),        vc(1),              0,  0,  0,  0, (void *) "Door Up(D)", NULL, NULL },
    { jwin_check_proc,     160,   158,  8+1,   8+1,     vc(14),        vc(1),              0,  0,  1,  0,       NULL, NULL, NULL },
    
    { jwin_text_proc,       17,   88,   200,     8,     vc(14),         vc(1),             0,  0,  0,  0, (void *) "Ambient Sound:", NULL, NULL },
    { jwin_droplist_proc,  140,   86,   140,    16,          0,             0,             0,  0,  3,  0, (void *) & sfx_list, NULL, NULL },
    
    { jwin_text_proc,       17,   108,   200,    8,     vc(14),         vc(1),             0,  0,  0,  0, (void *) "Boss Roar Sound:", NULL, NULL },
    { jwin_droplist_proc,  140,   106,   140,   16,         0,             0,              0,  0,  3,  0, (void *) & sfx_list, NULL, NULL },
    
    { jwin_text_proc,       17,   148,   200,    8,     vc(14),         vc(1),             0,  0,  0,  0, (void *) "Secret Sound:", NULL, NULL },
    { jwin_droplist_proc,  140,   146,   140,   16,          0,             0,             0,  0,  3,  0, (void *) & sfx_list, NULL, NULL },
    { jwin_text_proc,       17,   128,   200,    8,     vc(14),         vc(1),             0,  0,  0,  0, (void *) "Hold Up Item Sound:", NULL, NULL },
    { jwin_droplist_proc,  140,   126,   140,   16,          0,             0,             0,  0,  3,  0, (void *) & sfx_list, NULL, NULL },
    //107
    { jwin_check_proc,      15,   78,   160+1,  8+1,     vc(14),        vc(1),             0,  0,  1,  0,       NULL, NULL, NULL }, // Zora
    { jwin_check_proc,      15,   88,   160+1,  8+1,     vc(14),        vc(1),             0,  0,  1,  0,       NULL, NULL, NULL }, // Corner Traps
    { jwin_check_proc,      15,   98,   160+1,  8+1,     vc(14),        vc(1),             0,  0,  1,  0,       NULL, NULL, NULL }, // Middle Traps
    { jwin_check_proc,      15,   108,  160+1,  8+1,     vc(14),        vc(1),             0,  0,  1,  0,       NULL, NULL, NULL }, // Falling Rocks
    { jwin_check_proc,      15,   118,  160+1,  8+1,     vc(14),        vc(1),             0,  0,  1,  0,       NULL, NULL, NULL }, // Statue Fire
    { jwin_check_proc,      15,   138,  160+1,  8+1,     vc(14),        vc(1),             0,  0,  1,  0, (void *) "First Enemy Is 'Ring Leader'", NULL, NULL },
    { jwin_check_proc,      15,   148,  160+1,  8+1,     vc(14),        vc(1),             0,  0,  1,  0, (void *) "First Enemy Carries Item", NULL, NULL },
    // 'Invisible Enemies' goes between these two
    { jwin_check_proc,      15,   168,  160+1,  8+1,     vc(14),        vc(1),             0,  0,  1,  0, (void *) "Dungeon Boss (Don't Return)", NULL, NULL },
    { jwin_text_proc,       15,   68,     200,    8,     vc(14),        vc(1),             0,  0,  0,  0, (void *) "Environmental Enemies:", NULL, NULL },
    { jwin_text_proc,       15,   128,    200,    8,     vc(14),        vc(1),             0,  0,  0,  0, (void *) "Enemy Flags:", NULL, NULL },
    { jwin_text_proc,       15,   68,     200,    8,     vc(14),        vc(1),             0,  0,  0,  0, (void *) "Timed Warp: After a given time, Side Warp A is triggered.", NULL, NULL },
    //118
    { jwin_text_proc,       15,   68,     120,    8,     vc(11),        vc(1),             0,  0,  0,  0, (void *) "Room Type", NULL, NULL },
    { jwin_text_proc,       15,  108,     120,    8,     vc(11),        vc(1),             0,  0,  0,  0, (void *) "View", NULL, NULL },
    { jwin_text_proc,      165,   68,     120,    8,     vc(11),        vc(1),             0,  0,  0,  0, (void *) "Secrets", NULL, NULL },
    { jwin_text_proc,      165,  118,     120,    8,     vc(11),        vc(1),             0,  0,  0,  0, (void *) "Warp", NULL, NULL },
    { jwin_text_proc,      165,  178,     120,    8,     vc(11),        vc(1),             0,  0,  0,  0, (void *) "Items", NULL, NULL },
    { jwin_text_proc,       15,   68,     120,    8,     vc(11),        vc(1),             0,  0,  0,  0, (void *) "Combos", NULL, NULL },
    { jwin_text_proc,       15,  118,     120,    8,     vc(11),        vc(1),             0,  0,  0,  0, (void *) "Save", NULL, NULL },
    { jwin_text_proc,       15,  168,     120,    8,     vc(11),        vc(1),             0,  0,  0,  0, (void *) "FFC", NULL, NULL },
    { jwin_text_proc,      165,   68,     120,    8,     vc(11),        vc(1),             0,  0,  0,  0, (void *) "Whistle", NULL, NULL },
    { jwin_text_proc,      165,  108,     120,    8,     vc(11),        vc(1),             0,  0,  0,  0, (void *) "Misc.", NULL, NULL },
    //128
    { jwin_check_proc,     165,  138,   160+1,  8+1,     vc(14),        vc(1),             0,  0,  1,  0, (void *) "General Use 1 (Scripts)", NULL, NULL },
    { jwin_check_proc,     165,  148,   160+1,  8+1,     vc(14),        vc(1),             0,  0,  1,  0, (void *) "General Use 2 (Scripts)", NULL, NULL },
    { jwin_check_proc,     165,  158,   160+1,  8+1,     vc(14),        vc(1),             0,  0,  1,  0, (void *) "General Use 3 (Scripts)", NULL, NULL },
    { jwin_check_proc,     165,  168,   160+1,  8+1,     vc(14),        vc(1),             0,  0,  1,  0, (void *) "General Use 4 (Scripts)", NULL, NULL },
    { jwin_check_proc,     165,  178,   160+1,  8+1,     vc(14),        vc(1),             0,  0,  1,  0, (void *) "General Use 5 (Scripts)", NULL, NULL },
    
    { jwin_text_proc,       17,  160,     120,    8,     vc(11),        vc(1),             0,  0,  0,  0, (void *) "Lens Effect:", NULL, NULL },
    { jwin_droplist_proc,   17,  168,     133,   16,          0,            0,             0,  0,  0,  0, (void *) & lenseffect_list, NULL, NULL },
    
    { jwin_check_proc,     165,  158,   160+1,  8+1,     vc(14),        vc(1),             0,  0,  1,  0, (void *) "Maze Overrides Side Warps", NULL, NULL },
    { NULL,                  0,    0,       0,    0,          0,            0,             0,  0,  0,  0,       NULL, NULL,  NULL }
};

int onScrData()
{
    restore_mouse();
    char timedstring[6];
// char nmapstring[4];
// char nscrstring[3];
    char csensstring[2];
    char tics_secs_str[80];
    sprintf(tics_secs_str, "=0.00 seconds");
    char zora_str[85];
    char ctraps_str[85];
    char mtraps_str[85];
    char fallrocks_str[85];
    char statues_str[94];
    
    sprintf(zora_str, "Zora");
    sprintf(ctraps_str, "Corner Traps");
    sprintf(mtraps_str, "Middle Traps");
    sprintf(fallrocks_str, "Falling Rocks");
    sprintf(statues_str, "Statues Shoot Fireballs");
    
    {
        bool foundzora = false;
        bool foundctraps = false;
        bool foundmtraps = false;
        bool foundfallrocks = false;
        bool foundstatues = false;
        
        for(int i=0; i<eMAXGUYS && !(foundzora && foundctraps && foundmtraps && foundfallrocks && foundstatues); i++)
        {
            if(!foundzora && guysbuf[i].flags2 & eneflag_zora)
            {
                sprintf(zora_str, "Zora (1 x %s)", guy_string[i]);
                foundzora = true;
            }
            
            if(!foundctraps && guysbuf[i].flags2 & eneflag_trap)
            {
                sprintf(ctraps_str, "Corner Traps (4 x %s)", guy_string[i]);
                foundctraps = true;
            }
            
            if(!foundmtraps && guysbuf[i].flags2 & eneflag_trp2)
            {
                sprintf(mtraps_str, "Middle Traps (2 x %s)", guy_string[i]);
                foundmtraps = true;
            }
            
            if(!foundfallrocks && guysbuf[i].flags2 & eneflag_rock)
            {
                sprintf(fallrocks_str, "Falling Rocks (3 x %s)", guy_string[i]);
                foundfallrocks = true;
            }
            
            if(!foundstatues && guysbuf[i].flags2 & eneflag_fire)
            {
                sprintf(statues_str, "Shooting Statues (%s per combo)", guy_string[i]);
                foundstatues = true;
            }
        }
    }
    scrdata_dlg[107].dp= zora_str;
    scrdata_dlg[108].dp= ctraps_str;
    scrdata_dlg[109].dp= mtraps_str;
    scrdata_dlg[110].dp= fallrocks_str;
    scrdata_dlg[111].dp= statues_str;
    
    scrdata_dlg[0].dp2=lfont;
    sprintf(timedstring,"%d",Map.CurrScr()->timedwarptics);
//  sprintf(nmapstring,"%d",(int)Map.CurrScr()->nextmap);
// sprintf(nscrstring,"%x",(int)Map.CurrScr()->nextscr);
    sprintf(csensstring,"%d",(int)Map.CurrScr()->csensitive);
    
    byte f = Map.CurrScr()->flags;
    
    for(int i=0; i<8; i++)
    {
        scrdata_dlg[i+6].flags = (f&1) ? D_SELECTED : 0;
        f>>=1;
    }
    
    f = Map.CurrScr()->flags2 >> 4;
    
    for(int i=0; i<4; i++)
    {
        scrdata_dlg[i+14].flags = (f&1) ? D_SELECTED : 0;
        f>>=1;
    }
    
    f = Map.CurrScr()->flags3;
    
    for(int i=0; i<8; i++)
    {
        scrdata_dlg[i+18].flags = (f&1) ? D_SELECTED : 0;
        f>>=1;
    }
    
    f = Map.CurrScr()->flags4;
    scrdata_dlg[26].flags = (f&4) ? D_SELECTED : 0;
    scrdata_dlg[27].flags = (f&8) ? D_SELECTED : 0;
    scrdata_dlg[29].dp=timedstring;
    scrdata_dlg[30].dp=tics_secs_str;
// scrdata_dlg[34].dp=nmapstring;
    scrdata_dlg[34].d1 = (Map.CurrScr()->nextmap);
// scrdata_dlg[35].dp=nscrstring;
    scrdata_dlg[35].d1 = (Map.CurrScr()->nextscr);
    scrdata_dlg[96].dp=csensstring;
    scrdata_dlg[100].d1= (int)Map.CurrScr()->oceansfx;
    scrdata_dlg[102].d1= (int)Map.CurrScr()->bosssfx;
    scrdata_dlg[104].d1= (int)Map.CurrScr()->secretsfx;
    scrdata_dlg[106].d1= (int)Map.CurrScr()->holdupsfx;
    scrdata_dlg[36].flags = (f&16) ? D_SELECTED : 0;
    //scrdata_dlg[37].flags = (f&32) ? D_SELECTED : 0;
    scrdata_dlg[38].flags = (f&64) ? D_SELECTED : 0;
    scrdata_dlg[39].flags = (f&128) ? D_SELECTED : 0;
    f = Map.CurrScr()->flags5;
    scrdata_dlg[40].flags = (f&1) ? D_SELECTED : 0;
    scrdata_dlg[41].flags = (f&2) ? D_SELECTED : 0;
    scrdata_dlg[37].flags = (f&4) ? D_SELECTED : 0;
    scrdata_dlg[42].flags = (f&8) ? D_SELECTED : 0;
    scrdata_dlg[43].flags = (f&16) ? D_SELECTED : 0;
    scrdata_dlg[44].flags = (f&64) ? D_SELECTED : 0;
    scrdata_dlg[53].flags = (f&128) ? D_SELECTED : 0;
    f = Map.CurrScr()->flags6;
    scrdata_dlg[45].flags = (f&1) ? D_SELECTED : 0;
    scrdata_dlg[46].flags = (f&2) ? D_SELECTED : 0;
    scrdata_dlg[47].flags = (f&4) ? D_SELECTED : 0;
    scrdata_dlg[48].flags = (f&8) ? D_SELECTED : 0;
    scrdata_dlg[49].flags = (f&16) ? D_SELECTED : 0;
    scrdata_dlg[50].flags = (f&32) ? D_SELECTED : 0;
    scrdata_dlg[51].flags = (f&64) ? D_SELECTED : 0;
    scrdata_dlg[52].flags = (f&128) ? D_SELECTED : 0;
    f = Map.CurrScr()->flags7;
    scrdata_dlg[54].flags = (f&1) ? D_SELECTED : 0;
    scrdata_dlg[55].flags = (f&2) ? D_SELECTED : 0;
    scrdata_dlg[56].flags = (f&4) ? D_SELECTED : 0;
    scrdata_dlg[57].flags = (f&8) ? D_SELECTED : 0;
    scrdata_dlg[58].flags = (f&16) ? D_SELECTED : 0;
    scrdata_dlg[59].flags = (f&64) ? D_SELECTED : 0;
    scrdata_dlg[60].flags = (f&128) ? D_SELECTED : 0;
    f = Map.CurrScr()->flags8;
    scrdata_dlg[128].flags = (f&1) ? D_SELECTED : 0;
    scrdata_dlg[129].flags = (f&2) ? D_SELECTED : 0;
    scrdata_dlg[130].flags = (f&4) ? D_SELECTED : 0;
    scrdata_dlg[131].flags = (f&8) ? D_SELECTED : 0;
    scrdata_dlg[132].flags = (f&16) ? D_SELECTED : 0;
    scrdata_dlg[135].flags = (f&32) ? D_SELECTED : 0;
    
    word g = Map.CurrScr()->noreset;
    scrdata_dlg[74].flags = (g&mSECRET) ? D_SELECTED : 0;
    scrdata_dlg[75].flags = (g&mITEM) ? D_SELECTED : 0;
    scrdata_dlg[76].flags = (g&mBELOW) ? D_SELECTED : 0;
    scrdata_dlg[77].flags = (g&mLOCKBLOCK) ? D_SELECTED : 0;
    scrdata_dlg[78].flags = (g&mBOSSLOCKBLOCK) ? D_SELECTED : 0;
    scrdata_dlg[79].flags = (g&mCHEST) ? D_SELECTED : 0;
    scrdata_dlg[80].flags = (g&mLOCKEDCHEST) ? D_SELECTED : 0;
    scrdata_dlg[81].flags = (g&mBOSSCHEST) ? D_SELECTED : 0;
    scrdata_dlg[82].flags = (g&mDOOR_DOWN) ? D_SELECTED : 0;
    scrdata_dlg[83].flags = (g&mDOOR_LEFT) ? D_SELECTED : 0;
    scrdata_dlg[84].flags = (g&mDOOR_RIGHT) ? D_SELECTED : 0;
    scrdata_dlg[98].flags = (g&mDOOR_UP) ? D_SELECTED : 0;
    g = Map.CurrScr()->nocarry;
    scrdata_dlg[85].flags = (g&mSECRET) ? D_SELECTED : 0;
    scrdata_dlg[86].flags = (g&mITEM) ? D_SELECTED : 0;
    scrdata_dlg[87].flags = (g&mBELOW) ? D_SELECTED : 0;
    scrdata_dlg[88].flags = (g&mLOCKBLOCK) ? D_SELECTED : 0;
    scrdata_dlg[89].flags = (g&mBOSSLOCKBLOCK) ? D_SELECTED : 0;
    scrdata_dlg[90].flags = (g&mCHEST) ? D_SELECTED : 0;
    scrdata_dlg[91].flags = (g&mLOCKEDCHEST) ? D_SELECTED : 0;
    scrdata_dlg[92].flags = (g&mBOSSCHEST) ? D_SELECTED : 0;
    
    scrdata_dlg[94].d1 = (Map.CurrScr()->screen_midi>=0)?(Map.CurrScr()->screen_midi+1):(-(Map.CurrScr()->screen_midi+1));
    scrdata_dlg[134].d1 = Map.CurrScr()->lens_layer==llNORMAL?0:(Map.CurrScr()->lens_layer&llLENSSHOWS?6:0)+(Map.CurrScr()->lens_layer&7)+1;
    
    byte h=Map.CurrScr()->enemyflags;
    
    for(int i=0; i<8; i++)
    {
        scrdata_dlg[i+107].flags = (h&1)?D_SELECTED:0;
        h>>=1;
    }

	DIALOG *scrdata_cpy = resizeDialog(scrdata_dlg, 1.5);
    
	if(zc_popup_dialog(scrdata_cpy,-1)==2)
    {
        f=0;
        
        for(int i=7; i>=0; i--)
        {
            f<<=1;
            f |= scrdata_cpy[i+6].flags & D_SELECTED ? 1:0;
        }
        
        Map.CurrScr()->flags = f;
        
        f=0;
        
        for(int i=3; i>=0; i--)
        {
            f<<=1;
            f |= scrdata_cpy[i+14].flags & D_SELECTED ? 1:0;
        }
        
        Map.CurrScr()->flags2 &= 0x0F;
        Map.CurrScr()->flags2 |= f<<4;
        
        f=0;
        
        for(int i=7; i>=0; i--)
        {
            f<<=1;
            f |= scrdata_cpy[i+18].flags & D_SELECTED ? 1:0;
        }
        
        Map.CurrScr()->flags3 = f;
        
        f=0;
        f |= scrdata_cpy[26].flags & D_SELECTED ? 4:0;
        f |= scrdata_cpy[27].flags & D_SELECTED ? 8:0;
        f |= scrdata_cpy[36].flags & D_SELECTED ? 16:0;
        //f |= scrdata_dlg[37].flags & D_SELECTED ? 32:0;
        f |= scrdata_cpy[38].flags & D_SELECTED ? 64:0;
        f |= scrdata_cpy[39].flags & D_SELECTED ? 128:0;
        Map.CurrScr()->flags4 = f;
        
        f=0;
        f |= scrdata_cpy[40].flags & D_SELECTED ? 1:0;
        f |= scrdata_cpy[41].flags & D_SELECTED ? 2:0;
        f |= scrdata_cpy[37].flags & D_SELECTED ? 4:0;
        f |= scrdata_cpy[42].flags & D_SELECTED ? 8:0;
        f |= scrdata_cpy[43].flags & D_SELECTED ? 16:0;
        f |= scrdata_cpy[44].flags & D_SELECTED ? 64:0;
        f |= scrdata_cpy[53].flags & D_SELECTED ? 128:0;
        Map.CurrScr()->flags5 = f;
        
        f=0;
        f |= scrdata_cpy[45].flags & D_SELECTED ? 1:0;
        f |= scrdata_cpy[46].flags & D_SELECTED ? 2:0;
        f |= scrdata_cpy[47].flags & D_SELECTED ? 4:0;
        f |= scrdata_cpy[48].flags & D_SELECTED ? 8:0;
        f |= scrdata_cpy[49].flags & D_SELECTED ? 16:0;
        f |= scrdata_cpy[50].flags & D_SELECTED ? 32:0;
        f |= scrdata_cpy[51].flags & D_SELECTED ? 64:0;
        f |= scrdata_cpy[52].flags & D_SELECTED ? 128:0;
        Map.CurrScr()->flags6 = f;
        
        f=0;
        f |= scrdata_cpy[54].flags & D_SELECTED ? 1:0;
        f |= scrdata_cpy[55].flags & D_SELECTED ? 2:0;
        f |= scrdata_cpy[56].flags & D_SELECTED ? 4:0;
        f |= scrdata_cpy[57].flags & D_SELECTED ? 8:0;
        f |= scrdata_cpy[58].flags & D_SELECTED ? 16:0;
        f |= scrdata_cpy[59].flags & D_SELECTED ? 64:0;
        f |= scrdata_cpy[60].flags & D_SELECTED ? 128:0;
        Map.CurrScr()->flags7 = f;
        
        f=0;
        f |= scrdata_cpy[128].flags & D_SELECTED ? 1:0;
        f |= scrdata_cpy[129].flags & D_SELECTED ? 2:0;
        f |= scrdata_cpy[130].flags & D_SELECTED ? 4:0;
        f |= scrdata_cpy[131].flags & D_SELECTED ? 8:0;
        f |= scrdata_cpy[132].flags & D_SELECTED ? 16:0;
        f |= scrdata_cpy[135].flags & D_SELECTED ? 32:0;
        Map.CurrScr()->flags8 = f;
        
        g=0;
        g |= scrdata_cpy[74].flags & D_SELECTED ? mSECRET:0;
        g |= scrdata_cpy[75].flags & D_SELECTED ? mITEM:0;
        g |= scrdata_cpy[76].flags & D_SELECTED ? mBELOW:0;
        g |= scrdata_cpy[77].flags & D_SELECTED ? mLOCKBLOCK:0;
        g |= scrdata_cpy[78].flags & D_SELECTED ? mBOSSLOCKBLOCK:0;
        g |= scrdata_cpy[79].flags & D_SELECTED ? mCHEST:0;
        g |= scrdata_cpy[80].flags & D_SELECTED ? mLOCKEDCHEST:0;
        g |= scrdata_cpy[81].flags & D_SELECTED ? mBOSSCHEST:0;
        g |= scrdata_cpy[82].flags & D_SELECTED ? mDOOR_DOWN:0;
        g |= scrdata_cpy[83].flags & D_SELECTED ? mDOOR_LEFT:0;
        g |= scrdata_cpy[84].flags & D_SELECTED ? mDOOR_RIGHT:0;
        g |= scrdata_cpy[98].flags & D_SELECTED ? mDOOR_UP:0;
        Map.CurrScr()->noreset = g;
        
        g=0;
        g |= scrdata_cpy[85].flags & D_SELECTED ? mSECRET:0;
        g |= scrdata_cpy[86].flags & D_SELECTED ? mITEM:0;
        g |= scrdata_cpy[87].flags & D_SELECTED ? mBELOW:0;
        g |= scrdata_cpy[88].flags & D_SELECTED ? mLOCKBLOCK:0;
        g |= scrdata_cpy[89].flags & D_SELECTED ? mBOSSLOCKBLOCK:0;
        g |= scrdata_cpy[90].flags & D_SELECTED ? mCHEST:0;
        g |= scrdata_cpy[91].flags & D_SELECTED ? mLOCKEDCHEST:0;
        g |= scrdata_cpy[92].flags & D_SELECTED ? mBOSSCHEST:0;
        Map.CurrScr()->nocarry = g;
        
        Map.CurrScr()->screen_midi = (scrdata_cpy[94].d1>1)?(scrdata_cpy[94].d1-1):(-(scrdata_cpy[94].d1+1));
        Map.CurrScr()->lens_layer = scrdata_cpy[134].d1==0?0:(scrdata_cpy[134].d1>=7?(llLENSSHOWS|(scrdata_cpy[134].d1-7)):(llLENSHIDES|(scrdata_cpy[134].d1-1)));
        Map.CurrScr()->nextmap = scrdata_cpy[34].d1;
        Map.CurrScr()->nextscr = scrdata_cpy[35].d1;
        
        refresh(rMAP+rSCRMAP+rMENU);
        Map.CurrScr()->timedwarptics=atoi(timedstring);
        Map.CurrScr()->csensitive=(atoi(csensstring)<=8?zc_max(1,atoi(csensstring)):Map.CurrScr()->csensitive);
        Map.CurrScr()->oceansfx= scrdata_cpy[100].d1;
        Map.CurrScr()->bosssfx= scrdata_cpy[102].d1;
        Map.CurrScr()->secretsfx= scrdata_cpy[104].d1;
        Map.CurrScr()->holdupsfx= scrdata_cpy[106].d1;
        
        h=0;
        
        for(int i=7; i>=0; i--)
        {
            h<<=1;
            h |= scrdata_cpy[107+i].flags & D_SELECTED ? 1:0;
        }
        
        Map.CurrScr()->enemyflags=h;
        
        saved=false;
    }

	delete[] scrdata_cpy;
    
    return D_O_K;
}

const char *nslist(int index, int *list_size)
{
    if(index>=0)
    {
        bound(index,0,MAXSCREENS);
        sprintf(ns_string, " %02X", index);
        return ns_string;
    }
    
    *list_size=MAXSCREENS;
    return NULL;
}

const char *flaglist(int index, int *list_size)
{
    if(index>=0)
    {
        if(index>=MAXFLAGS)
            index=MAXFLAGS-1;
            
        return flag_string[index];
    }
    
    *list_size=MAXFLAGS;
    return NULL;
}

const char *roomslist(int index, int *list_size)
{
    if(index>=0)
    {
        if(index>=MAXROOMTYPES)
            index=MAXROOMTYPES-1;
            
        return roomtype_string[index];
    }
    
    *list_size=MAXROOMTYPES;
    return NULL;
}

static char number_str_buf[32];
int number_list_size=1;
bool number_list_zero=false;

const char *numberlist(int index, int *list_size)
{
    if(index>=0)
    {
        bound(index,0,number_list_size-1);
        sprintf(number_str_buf,"%d",index+(number_list_zero?0:1));
        return number_str_buf;
    }
    
    *list_size=number_list_size;
    return NULL;
}

static char dmap_str_buf[37];
int dmap_list_size=1;
bool dmap_list_zero=true;

const char *dmaplist(int index, int *list_size)
{
    if(index>=0)
    {
        bound(index,0,dmap_list_size-1);
        sprintf(dmap_str_buf,"%3d-%s",index+(dmap_list_zero?0:1), DMaps[index].name);
        return dmap_str_buf;
    }
    
    *list_size=dmap_list_size;
    return NULL;
}

char *hexnumlist(int index, int *list_size)
{
    if(index>=0)
    {
        bound(index,0,number_list_size-1);
        sprintf(number_str_buf,"%X",index+(number_list_zero?0:1));
        return number_str_buf;
    }
    
    *list_size=number_list_size;
    return NULL;
}

const char *maplist(int index, int *list_size)
{
    if(index>=0)
    {
        bound(index,0,MAXMAPS2-1);
        sprintf(number_str_buf,"%d",index+1);
        return number_str_buf;
    }
    
    *list_size=MAXMAPS2;
    return NULL;
}

const char *gotomaplist(int index, int *list_size)
{
    if(index>=0)
    {
        bound(index,0,map_count-1);
        sprintf(number_str_buf,"%d",index+1);
        return number_str_buf;
    }
    
    *list_size = map_count;
    return NULL;
}

const char *nextmaplist(int index, int *list_size)
{
    if(index>=0)
    {
        bound(index,0,map_count);
        sprintf(number_str_buf,"%3d",index);
        return number_str_buf;
    }
    
    *list_size = map_count+1;
    return NULL;
}

const char *midilist(int index, int *list_size)
{
    if(index>=0)
    
    {
        bound(index,0,MAXCUSTOMMIDIS_ZQ-1);
        return midi_string[index];
    }
    
    *list_size=MAXCUSTOMMIDIS_ZQ;
    return NULL;
}

const char *screenmidilist(int index, int *list_size)
{
    if(index>=0)
    
    {
        bound(index,0,MAXCUSTOMMIDIS_ZQ);
        return screen_midi_string[index];
    }
    
    *list_size=MAXCUSTOMMIDIS_ZQ+1;
    return NULL;
}

const char *custommidilist(int index, int *list_size)
{
    if(index>=0)
    {
        bound(index,0,MAXCUSTOMMIDIS_ZQ-1);
        sprintf(number_str_buf,"%3d - %s",index+(number_list_zero?0:1),customtunes[index].data?customtunes[index].title:"(Empty)");
        return number_str_buf;
    }
    
    *list_size=number_list_size;
    return NULL;
}

const char *enhancedmusiclist(int index, int *list_size)
{
    index=index; //this is here to prevent unused parameter warnings
    list_size=list_size; //this is here to prevent unused parameter warnings
    /*if(index>=0)
    {
      bound(index,0,MAXMUSIC-1);
      sprintf(number_str_buf,"%3d - %s",index+(number_list_zero?0:1),enhancedMusic[index].filename[0]?enhancedMusic[index].title:"(Empty)" );
      return number_str_buf;
    }
    *list_size=number_list_size;*/
    return NULL;
}


const char *levelnumlist(int index, int *list_size)
{
    if(index>=0)
    {
        bound(index,0,0xFFF);
        sprintf(number_str_buf,"%.3X - %s",index,palnames[index]);
        return number_str_buf;
    }
    
    *list_size=MAXLEVELS;
    return NULL;
}

static char shop_str_buf[40];
int shop_list_size=1;

const char *shoplist(int index, int *list_size)
{
    if(index>=0)
    {
        bound(index,0,shop_list_size-1);
        sprintf(shop_str_buf,"%3d:  %s",index,misc.shop[index].name);
        return shop_str_buf;
    }
    
    *list_size=shop_list_size;
    return NULL;
}

static char info_str_buf[40];
int info_list_size=1;

const char *infolist(int index, int *list_size)
{
    if(index>=0)
    {
        bound(index,0,info_list_size-1);
        sprintf(info_str_buf,"%3d:  %s",index,misc.info[index].name);
        return info_str_buf;
    }
    
    *list_size=info_list_size;
    return NULL;
}



int onMapCount()
{
    bool makechange=true;
    bool willaffectlayers=false;
    int oldmapcount=map_count-1;
    
    int ret = select_data("Number of Maps",map_count-1,maplist, lfont);
    
    if(ret == oldmapcount)
        return D_O_K; //they selected the same number of maps they already have.
        
    {
        if(ret < 0)
            makechange=false;
        else if(ret<oldmapcount)
        {
            for(int i=0; i<(ret+1)*MAPSCRS; i++)
            {
                mapscr *layerchecker=&TheMaps[i];
                
                for(int j=0; j<6; j++)
                {
                    if(layerchecker->layermap[j]>(ret+1))
                    {
                        willaffectlayers=true;
                        break;
                    }
                }
                
                if(willaffectlayers)
                {
                    break;
                }
            }
            
            if(willaffectlayers)
            {
                if(jwin_alert("Confirm Change",
                              "This change will delete maps being used for",
                              "layers in the remaining maps. The map numbers",
                              "of the affected layers will be reset to 0.",
                              "O&K", "&Cancel", 'o', 'c', lfont)==2)
                {
                    makechange=false;
                }
            }
        }
        
        if(makechange)
        {
            saved = false;
            setMapCount2(ret+1);
            
            if(willaffectlayers)
            {
                for(int i=0; i<(ret+1)*MAPSCRS; i++)
                {
                    fix_layers(&TheMaps[i], false);
                }
            }
        }
    }
    
    refresh(rMAP+rSCRMAP+rMENU);
    return D_O_K;
}

int onGotoMap()
{
    int ret = select_data("Goto Map",Map.getCurrMap(),gotomaplist,lfont);
    
    if(ret >= 0)
    {
        int m=Map.getCurrMap();
        Map.setCurrMap(ret);
        
        if(m!=Map.getCurrMap())
        {
            memset(relational_tile_grid,(draw_mode==dm_relational?1:0),(11+(rtgyo*2))*(16+(rtgxo*2)));
        }
    }
    
    refresh(rALL);
    return D_O_K;
}

int onFlags()
{
    restore_mouse();
    int ret=select_cflag("Select Combo Flag",Flag);
	Backend::mouse->setWheelPosition(0);
    
    if(ret>=0)
    {
        Flag=ret;
        setFlagColor();
        refresh(rMENU);
        doflags();
    }
    
    return D_O_K;
}

static DIALOG usedcombo_list_dlg[] =
{
    // (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp)
    { jwin_win_proc,     60-12,   40,   200+24,  148,  vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "Combos Used", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { jwin_textbox_proc,       72-12,   60+4,   176+24+1,  92+4,   jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,       D_EXIT,     0,             0,      NULL, NULL, NULL },
    { jwin_button_proc,     130,   163,  61,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "OK", NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};



int onUsedCombos()
{
    restore_mouse();
    usedcombo_list_dlg[0].dp2=lfont;
    
    int usedcombos[7][300][2];
    char combolist_text[65536];
    char temptext[80];
    
    int drawmap=Map.getCurrMap();
    int drawscr=Map.getCurrScr();
    int counter[7];
    
    for(int layer=0; layer<7; ++layer)
    {
        counter[layer]=0;
        
        if(layer==0)
        {
            drawmap=Map.getCurrMap();
            drawscr=Map.getCurrScr();
        }
        else
        {
            if(Map.AbsoluteScr(Map.getCurrMap(), Map.getCurrScr())->layermap[layer-1]>0)
            {
                drawmap=Map.AbsoluteScr(Map.getCurrMap(), Map.getCurrScr())->layermap[layer-1]-1;
                drawscr=Map.AbsoluteScr(Map.getCurrMap(), Map.getCurrScr())->layerscreen[layer-1];
            }
            else
            {
                continue;
            }
        }
        
        usedcombos[layer][0][0]=Map.AbsoluteScr(drawmap, drawscr)->data[0];
        usedcombos[layer][0][1]=1;
        counter[layer]=1;
        
        for(int i=1; i<176; ++i)
        {
            bool used=false;
            
            for(int j=0; j<counter[layer]; ++j)
            {
                if(usedcombos[layer][j][0]==Map.AbsoluteScr(drawmap, drawscr)->data[i])
                {
                    ++usedcombos[layer][j][1];
                    used=true;
                    break;
                }
            }
            
            if(!used)
            {
                usedcombos[layer][counter[layer]][0]=Map.AbsoluteScr(drawmap, drawscr)->data[i];
                usedcombos[layer][counter[layer]][1]=1;
                ++counter[layer];
            }
        }
        
        for(int i=0; i<counter[layer]-1; i++)
        {
            for(int j=i+1; j<counter[layer]; j++)
            {
                if(usedcombos[layer][i][0]>usedcombos[layer][j][0])
                {
                    zc_swap(usedcombos[layer][i][0],usedcombos[layer][j][0]);
                    zc_swap(usedcombos[layer][i][1],usedcombos[layer][j][1]);
                }
            }
        }
    }
    
    sprintf(combolist_text, " ");
    
    for(int layer=0; layer<7; ++layer)
    {
        if(counter[layer]>0)
        {
            if(layer>0)
            {
                strcat(combolist_text, "\n");
            }
            
            sprintf(temptext, "Combos on layer %d\n-----------------\n", layer);
            strcat(combolist_text, temptext);
            
            for(int i=0; i<counter[layer]; i++)
            {
                if((i<counter[layer]-1) && (((usedcombos[layer][i][1]==usedcombos[layer][i+1][1]&&(usedcombos[layer][i][0]+1==usedcombos[layer][i+1][0])) && ((i==0) || ((usedcombos[layer][i][1]!=usedcombos[layer][i-1][1])||((usedcombos[layer][i][0]-1!=usedcombos[layer][i-1][0])))))))
                {
                    sprintf(temptext, "%5d ", usedcombos[layer][i][0]);
                    strcat(combolist_text, temptext);
                }
                else if(((i>0) && (((usedcombos[layer][i][1]==usedcombos[layer][i-1][1])&&((usedcombos[layer][i][0]-1==usedcombos[layer][i-1][0]))) && ((i==counter[layer]-1) || ((usedcombos[layer][i][1]!=usedcombos[layer][i+1][1])||((usedcombos[layer][i][0]+1!=usedcombos[layer][i+1][0])))))))
                {
                    sprintf(temptext, "- %5d (%3d)\n", usedcombos[layer][i][0],usedcombos[layer][i][1]);
                    strcat(combolist_text, temptext);
                }
                else if(((i==0) && ((usedcombos[layer][i][1]!=usedcombos[layer][i+1][1])||((usedcombos[layer][i][0]+1!=usedcombos[layer][i+1][0]))))||
                        ((i==counter[layer]-1) && ((usedcombos[layer][i][1]!=usedcombos[layer][i-1][1])||((usedcombos[layer][i][0]-1!=usedcombos[layer][i-1][0]))))||
                        ((i>0) && (i<counter[layer]-1) && ((usedcombos[layer][i][1]!=usedcombos[layer][i+1][1])||((usedcombos[layer][i][0]+1!=usedcombos[layer][i+1][0]))) && ((usedcombos[layer][i][1]!=usedcombos[layer][i-1][1])||((usedcombos[layer][i][0]-1!=usedcombos[layer][i-1][0])))))
                {
                    sprintf(temptext, "    %5d     (%3d)\n", usedcombos[layer][i][0],usedcombos[layer][i][1]);
                    strcat(combolist_text, temptext);
                }
            }
        }
    }
    
    strcat(combolist_text, "\n");
    usedcombo_list_dlg[2].dp=combolist_text;
    usedcombo_list_dlg[2].d2=0;

	DIALOG *usedcombo_list_cpy = resizeDialog(usedcombo_list_dlg, 1.5);
    
    zc_popup_dialog(usedcombo_list_cpy,2);
	delete[] usedcombo_list_cpy;
	Backend::mouse->setWheelPosition(0);
    return D_O_K;
}

int onItem()
{
    restore_mouse();
    build_bii_list(true);
    int exit_status;
    int current_item=Map.CurrScr()->hasitem != 0 ? Map.CurrScr()->item : -2;
    
    do
    {
        int ret=select_item("Select Item",current_item,false,exit_status);
        
        if(exit_status == 5)
        {
            if(ret>=0)  // Edit
            {
                current_item=ret;
                build_biw_list();
                edit_itemdata(ret);
            }
            else exit_status = -1;
        }
        else  if(exit_status==2 || exit_status==3)   // Double-click or OK
        {
            if(ret>=0)
            {
                saved=false;
                Map.CurrScr()->item=ret;
                Map.CurrScr()->hasitem = true;
            }
            else
            {
                saved=false;
                Map.CurrScr()->hasitem=false;
            }
        }
    }
    while(exit_status == 5);
    
    refresh(rMAP+rMENU);
    return D_O_K;
}

int onRType()
{
    if(prv_mode)
    {
        Map.set_prvscr(Map.get_prv_map(), Map.get_prv_scr());
        Map.set_prvcmb(0);
        return D_O_K;
    }
    
    restore_mouse();
    build_bir_list();
    int ret=select_room("Select Room Type",Map.CurrScr()->room);
    
    if(ret>=0)
    {
        saved=false;
        Map.CurrScr()->room=ret;
    }
    
    int c=Map.CurrScr()->catchall;
    
    switch(Map.CurrScr()->room)
    {
    case rSP_ITEM:
        Map.CurrScr()->catchall=bound(c,0,ITEMCNT-1);
        break;
        // etc...
    }
    
    refresh(rMENU);
    
    return D_O_K;
}

int onGuy()
{
    restore_mouse();
    build_big_list(true);
    int ret=select_guy("Select Guy",Map.CurrScr()->guy);
    
    if(ret>=0)
    {
        saved=false;
        Map.CurrScr()->guy=ret;
    }
    
    refresh(rMAP+rMENU);
    return D_O_K;
}

int onString()
{
    if(prv_mode)
    {
        Map.prv_secrets(false);
        refresh(rALL);
        return D_O_K;
    }
    
    restore_mouse();
    int ret=select_data("Select Message String",MsgStrings[Map.CurrScr()->str].listpos,msgslist,lfont);
    
    if(ret>=0)
    {
        saved=false;
        Map.CurrScr()->str=msglistcache[ret];
    }
    
    refresh(rMENU);
    return D_O_K;
}

int onEndString()
{
    int ret=select_data("Select Ending String",misc.endstring,msgslist,lfont);
    
    if(ret>=0)
    {
        saved=false;
        misc.endstring=msglistcache[ret];
    }
    
    refresh(rMENU);
    return D_O_K;
}

int onCatchall()
{
    if(prv_mode)
    {
        Map.set_prvadvance(1);
        return D_O_K;
    }
    
    if(data_menu[13].flags==D_DISABLED)
    {
        return D_O_K;
    }
    
    restore_mouse();
    int ret=-1;
    int rtype=Map.CurrScr()->room;
    
    switch(rtype)
    {
    case rSP_ITEM:
        int exit_status;
        build_bii_list(false);
        
        do
        {
            ret=select_item("Select Special Item",Map.CurrScr()->catchall,false,exit_status);
            
            if(exit_status == 5 && ret >= 0)
            {
                build_biw_list();
                edit_itemdata(ret);
            }
            else exit_status = -1;
        }
        while(exit_status == 5);
        
        break;
        
    case rINFO:
        info_list_size = 256;
        ret = select_data("Select Info Type",Map.CurrScr()->catchall,infolist,"OK","Cancel",lfont);
        break;
        
    case rTAKEONE:
        shop_list_size = 256;
        ret = select_data("Select \"Take One Item\" Type",Map.CurrScr()->catchall,shoplist,"OK","Cancel",lfont);
        break;
        
    case rP_SHOP:
    case rSHOP:
        shop_list_size = 256;
        ret = select_data("Select Shop Type",Map.CurrScr()->catchall,shoplist,"OK","Cancel",lfont);
        break;
        
    default:
        char buf[80]="Enter ";
        strcat(buf,catchall_string[rtype]);
        ret=getnumber(buf,Map.CurrScr()->catchall);
        break;
    }
    
    if(ret>=0)
    {
        if(ret != Map.CurrScr()->catchall)
            saved=false;
            
        Map.CurrScr()->catchall=ret;
    }
    
    refresh(rMENU);
    return D_O_K;
}

static ListData levelnum_list(levelnumlist, &font);

static DIALOG screen_pal_dlg[] =
{
    // (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp)
    { jwin_win_proc,      60-12,   40,   200-16,  96,  vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "Select Palette", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { jwin_droplist_proc, 72-12,   84+4,   161,  16,   jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,       0,     0,             0, (void *) &levelnum_list, NULL, NULL },
    { jwin_button_proc,   70,   111,  61,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "OK", NULL, NULL },
    { jwin_button_proc,   150,  111,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { jwin_text_proc,       72-12,   60+4,  168,  8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Note: This does not affect how the", NULL, NULL },
    { jwin_text_proc,       72-12,   72+4,  168,  8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "room will be displayed in-game!", NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};
//  return list_dlg[2].d1;

int onScreenPalette()
{
    restore_mouse();
    screen_pal_dlg[0].dp2=lfont;
    screen_pal_dlg[2].d1=Map.getcolor();

	DIALOG *screen_pal_cpy = resizeDialog(screen_pal_dlg, 1.5);
    
    if(zc_popup_dialog(screen_pal_cpy,2)==3)
    {
        saved=false;
        Map.setcolor(screen_pal_cpy[2].d1);
        refresh(rALL);
    }

	delete[] screen_pal_cpy;
    
    return D_O_K;
}

int onDecScrPal()
{
    restore_mouse();
    int c=Map.getcolor();
    c+=255;
    c=c%256;
    Map.setcolor(c);
    refresh(rALL);
    return D_O_K;
}

int onIncScrPal()
{
    restore_mouse();
    int c=Map.getcolor();
    c+=1;
    c=c%256;
    Map.setcolor(c);
    refresh(rALL);
    return D_O_K;
}

int d_ndroplist_proc(int msg,DIALOG *d,int c)
{
    int ret = jwin_droplist_proc(msg,d,c);
    
    // The only place this proc is used is in the info type editor.
    // If it's ever used anywhere else, this will probably need to be changed.
    // Maybe add a flag for it or something.
    int msgID=msg_at_pos(d->d1);
    
    switch(msg)
    {
    case MSG_DRAW:
    case MSG_CHAR:
    case MSG_CLICK:
        textprintf_ex(screen,font,d->x - 48,d->y + 4,jwin_pal[jcBOXFG],jwin_pal[jcBOX],"%5d",msgID);
    }
    
    return ret;
}

int d_idroplist_proc(int msg,DIALOG *d,int c)
{
    int ret = jwin_droplist_proc(msg,d,c);
    
	switch (msg)
	{
	case MSG_DRAW:
	case MSG_CHAR:
	case MSG_CLICK:
		int tile = bii[d->d1].i >= 0 ? itemsbuf[bii[d->d1].i].tile : 0;
		int cset = bii[d->d1].i >= 0 ? itemsbuf[bii[d->d1].i].csets & 15 : 0;
		int x = d->x + d->w + 4;
		int y = d->y - 2;
		int w = 16;
		int h = 16;

		if (is_large())
		{
			w = 32;
			h = 32;
			y -= 6;
		}

		BITMAP *buf = create_bitmap_ex(8, 16, 16);
		BITMAP *bigbmp = create_bitmap_ex(8, w, h);

		if (buf && bigbmp)
		{
			clear_bitmap(buf);

			if (tile)
				overtile16(buf, tile, 0, 0, cset, 0);

			stretch_blit(buf, bigbmp, 0, 0, 16, 16, 0, 0, w, h);
			destroy_bitmap(buf);
			jwin_draw_frame(screen, x, y, w + 4, h + 4, FR_DEEP);
			blit(bigbmp, screen, 0, 0, x + 2, y + 2, w, h);
			destroy_bitmap(bigbmp);
		}

	}
    
    return ret;
}

int d_nidroplist_proc(int msg,DIALOG *d,int c)
{
    int ret = d_idroplist_proc(msg,d,c);
    
    switch(msg)
    {
    case MSG_DRAW:
    case MSG_CHAR:
    case MSG_CLICK:
        textprintf_ex(screen,font,d->x - 48,d->y + 4,jwin_pal[jcBOXFG],jwin_pal[jcBOX],"%5d",bii[d->d1].i);
    }
    
    return ret;
}

int d_ilist_proc(int msg,DIALOG *d,int c)
{
    int ret = jwin_abclist_proc(msg,d,c);
    
    switch(msg)
    {
    case MSG_DRAW:
    case MSG_CHAR:
    case MSG_CLICK:
        
        int tile = 0;
        int cset = 0;
        
        if(bii[d->d1].i >-1)
        {
            tile= itemsbuf[bii[d->d1].i].tile;
            cset= itemsbuf[bii[d->d1].i].csets&15;
        }
        
        int x = d->x + d->w + 4;
        int y = d->y;
        int w = 16;
        int h = 16;
        
        if(is_large())
        {
            w = 32;
            h = 32;
        }
        
        BITMAP *buf = create_bitmap_ex(8,16,16);
        BITMAP *bigbmp = create_bitmap_ex(8,w,h);
        
        if(buf && bigbmp)
        {
            clear_bitmap(buf);
            
            if(tile)
                overtile16(buf, tile,0,0,cset,0);
                
            stretch_blit(buf, bigbmp, 0,0, 16, 16, 0, 0, w, h);
            destroy_bitmap(buf);
            jwin_draw_frame(screen,x,y,w+4,h+4,FR_DEEP);
            blit(bigbmp,screen,0,0,x+2,y+2,w,h);
            destroy_bitmap(bigbmp);
        }
        if(bii[d->d1].i>=0)
        {
            textprintf_ex(screen,spfont,x,y+20*(is_large()?2:1),jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"#%d  ",bii[d->d1].i);
            
            textprintf_ex(screen,spfont,x,y+26*(is_large()?2:1),jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"Pow:    ");
            textprintf_ex(screen,spfont,x+int(16*(is_large()?1.5:1)),y+26*(is_large()?2:1),jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"%d",itemsbuf[bii[d->d1].i].power);
        }
        
        // Might be a bit confusing for new users
        /*textprintf_ex(screen,is_large?font:spfont,x,y+32*(is_large?2:1),jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"Cost:   ");
        textprintf_ex(screen,is_large?font:spfont,x+int(16*(is_large?1.5:1)),y+32*(is_large?2:1),jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"%d",itemsbuf[bii[d->d1].i].magic);*/
        
    }
    
    return ret;
}

int d_wlist_proc(int msg,DIALOG *d,int c)
{
    int ret = jwin_abclist_proc(msg,d,c);
    
    switch(msg)
    {
    case MSG_DRAW:
    case MSG_CHAR:
    case MSG_CLICK:
        
        int tile = 0;
        int cset = 0;
        tile= wpnsbuf[biw[d->d1].i].tile;
        cset= wpnsbuf[biw[d->d1].i].csets&15;
        int x = d->x + d->w + 4;
        int y = d->y;
        int w = 16;
        int h = 16;
        
        if(is_large())
        {
            w = 32;
            h = 32;
        }
        
        BITMAP *buf = create_bitmap_ex(8,16,16);
        BITMAP *bigbmp = create_bitmap_ex(8,w,h);
        
        if(buf && bigbmp)
        {
            clear_bitmap(buf);
            
            if(tile)
                overtile16(buf, tile,0,0,cset,0);
                
            stretch_blit(buf, bigbmp, 0,0, 16, 16, 0, 0, w, h);
            destroy_bitmap(buf);
            jwin_draw_frame(screen,x,y,w+4,h+4,FR_DEEP);
            blit(bigbmp,screen,0,0,x+2,y+2,w,h);
            destroy_bitmap(bigbmp);
        }

		if (biw[d->d1].i >= 0)
		{
			textprintf_ex(screen, is_large() ? font : spfont, x, y + 20 * (is_large() ? 2 : 1), jwin_pal[jcTEXTFG], jwin_pal[jcBOX], "#%d   ", biw[d->d1].i);
		}
        
    }
    
    return ret;
}


/**********************************/
//        Triforce Pieces         //
/**********************************/

static byte triframe_points[9*4] =
{
    0,2,2,0,  2,0,4,2,  0,2,4,2,  1,1,3,1,  2,0,2,2,
    1,1,1,2,  1,1,2,2,  3,1,3,2,  3,1,2,2
};

int d_tri_frame_proc(int msg,DIALOG *d,int c)
{
    //these are here to bypass compiler warnings about unused arguments
    c=c;
    
    if(msg==MSG_DRAW)
    {
        int x[5],y[3];
        
        x[0]=d->x;
        x[1]=d->x+(d->w>>2);
        x[2]=d->x+(d->w>>1);
        x[3]=d->x+(d->w>>1)+(d->w>>2);
        x[4]=d->x+d->w;
        y[0]=d->y;
        y[1]=d->y+(d->h>>1);
        y[2]=d->y+d->h;
        
        byte *p = triframe_points;
        
        for(int i=0; i<9; i++)
        {
            line(screen,x[*p],y[*(p+1)],x[*(p+2)],y[*(p+3)],d->fg);
            p+=4;
        }
    }
    
    return D_O_K;
}

int d_tri_edit_proc(int msg,DIALOG *d,int c)
{
    jwin_button_proc(msg,d,c);
    
    if(msg==MSG_CLICK)
    {
        int v = getnumber("Piece Number",d->d1);
        
        if(v>=0)
        {
            bound(v,1,8);
            
            if(v!=d->d1)
            {
                DIALOG *tp = d - d->d2;
                
                for(int i=0; i<8; i++)
                {
                    if(tp->d1==v)
                    {
                        tp->d1 = d->d1;
                        ((char*)(tp->dp))[0] = d->d1+'0';
                        jwin_button_proc(MSG_DRAW,tp,0);
                    }
                    
                    ++tp;
                }
                
                d->d1 = v;
                ((char*)(d->dp))[0] = v+'0';
            }
        }
        
        d->flags = 0;
        jwin_button_proc(MSG_DRAW,d,0);
    }
    
    return D_O_K;
}

static DIALOG tp_dlg[] =
{
    /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp) */
    { jwin_win_proc,     56,   32,   208,  160,  vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "Triforce Pieces", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { d_tri_frame_proc,  64,   56,   192,    96,   jwin_pal[jcBOXFG],  jwin_pal[jcBOX],  0,       0,          0,             0,       NULL, NULL, NULL },
    // 3
    { d_tri_edit_proc,   138,  82,   17,   17,   vc(14),  vc(1),  0,       0,          0,             0, (void *) "1", NULL, NULL },
    { d_tri_edit_proc,   166,  82,   17,   17,   vc(14),  vc(1),  0,       0,          0,             1, (void *) "2", NULL, NULL },
    { d_tri_edit_proc,   90,   130,  17,   17,   vc(14),  vc(1),  0,       0,          0,             2, (void *) "3", NULL, NULL },
    { d_tri_edit_proc,   214,  130,  17,   17,   vc(14),  vc(1),  0,       0,          0,             3, (void *) "4", NULL, NULL },
    // 7
    { d_tri_edit_proc,   138,  110,  17,   17,   vc(14),  vc(1),  0,       0,          0,             4, (void *) "5", NULL, NULL },
    { d_tri_edit_proc,   118,  130,  17,   17,   vc(14),  vc(1),  0,       0,          0,             5, (void *) "6", NULL, NULL },
    { d_tri_edit_proc,   166,  110,  17,   17,   vc(14),  vc(1),  0,       0,          0,             6, (void *) "7", NULL, NULL },
    { d_tri_edit_proc,   186,  130,  17,   17,   vc(14),  vc(1),  0,       0,          0,             7, (void *) "8", NULL, NULL },
    // 11
    { jwin_button_proc,     90,   166,  61,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "OK", NULL, NULL },
    { jwin_button_proc,     170,  166,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

int onTriPieces()
{
    tp_dlg[0].dp2=lfont;
    char temptext[8][2];
    
    for(int i=0; i<8; i++)
    {
        tp_dlg[i+3].d1 = misc.triforce[i];
        //    ((char*)(tp_dlg[i+3].dp))[0] = misc.triforce[i]+'0';
        sprintf(temptext[i], "%d", misc.triforce[i]);
        tp_dlg[i+3].dp=temptext[i];
    }

	DIALOG *tp_cpy = resizeDialog(tp_dlg, 1.5);
    
    if(zc_popup_dialog(tp_cpy,-1) == 11)
    {
        saved=false;
        
        for(int i=0; i<8; i++)
            misc.triforce[i] = tp_cpy[i+3].d1;
    }
    
	delete[] tp_cpy;

    return D_O_K;
}


/**********************************/
/***********  onDMaps  ************/
/**********************************/

int d_maptile_proc(int msg,DIALOG *d,int c);
bool small_dmap=false;

static DIALOG dmapmaps_dlg[] =
{

    /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp) */
    { jwin_win_proc, 4,    18,   313,  217,  vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "Custom DMap Map Styles", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { jwin_button_proc,     93,   208,  61,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "OK", NULL, NULL },
    { jwin_button_proc,     168,  208,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { d_ctext_proc,      160,  38,    0,   8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Minimaps", NULL, NULL },
    { d_ctext_proc,      112,  46,     0,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Without Map", NULL, NULL },
    { d_ctext_proc,      208,  46,     0,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "With Map", NULL, NULL },
    
    { d_ctext_proc,      162,  110,    0,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Large Maps", NULL, NULL },
    { d_ctext_proc,      80,   118,    0,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Without Map", NULL, NULL },
    { d_ctext_proc,      240,  118,    0,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "With Map", NULL, NULL },
    // 5
    { d_maptile_proc,    72,   54,   80,   48,   0,       0,      0,       0,          0,             0,       NULL, NULL, NULL },
    { d_maptile_proc,    168,  54,   80,   48,   0,       0,      0,       0,          0,             0,       NULL, NULL, NULL },
    { d_maptile_proc,    8,    126,  144,  80,   0,       0,      0,       0,          0,             0,       NULL, NULL, NULL },
    { d_maptile_proc,    168,  126,  144,  80,   0,       0,      0,       0,          0,             0,       NULL, NULL, NULL },
    // 11
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

int d_hexedit_proc(int msg,DIALOG *d,int c)
{
    return jwin_hexedit_proc(msg,d,c);
}

int xtoi(char *hexstr)
{
    int val=0;
    
    while(isxdigit(*hexstr))
    {
        val<<=4;
        
        if(*hexstr<='9')
            val += *hexstr-'0';
        else val+= ((*hexstr)|0x20)-'a'+10;
        
        ++hexstr;
    }
    
    return val;
}

void drawgrid(BITMAP *dest,int x,int y,int grid,int fg,int bg,int div)
{
    if(div!=-1)
        rectfill(dest,x-1,y-1,x+63,y+3,div);
        
    for(int dx=0; dx<64; dx+=8)
    {
        if(grid&0x80)
            rectfill(dest,x+dx,y,x+dx+6,y+2,fg);
        else if(bg!=-1)
            rectfill(dest,x+dx,y,x+dx+6,y+2,bg);
            
        grid<<=1;
    }
}

void drawovergrid(BITMAP *dest,int x,int y,int grid,int color,int div)
{
    if(div!=-1)
        rectfill(dest,x-1,y-1,x+63,y+3,div);
        
    for(int dx=0; dx<64; dx+=4)
    {
        rectfill(dest,x+dx,y,x+dx+2,y+2,color);
        grid<<=1;
    }
}

void drawgrid(BITMAP *dest,int x,int y,int w, int h, int tw, int th, int *grid,int fg,int bg,int div)
{
    //these are here to bypass compiler warnings about unused arguments
    w=w;
    tw=tw;
    th=th;
    
    rectfill(dest,x,y,x+(8*8),y+(1*4),div);
    
    for(int dy=0; dy<h; dy++)
    {
        for(int dx=0; dx<64; dx+=8)
        {
            if(grid[0]&0x80)
                rectfill(dest,x+dx,y,x+dx+6,y+2,fg);
            else
                rectfill(dest,x+dx,y,x+dx+6,y+2,bg);
                
            grid[0]<<=1;
        }
    }
}

void drawgrid_s(BITMAP *dest,int x,int y,int grid,int fg,int bg,int div)
{
    rectfill(dest,x-1,y-1,x+63,y+3,div);
    
    for(int dx=0; dx<64; dx+=8)
    {
        rectfill(dest,x+dx,y,x+dx+6,y+2,bg);
        
        if(grid&0x80)
            rectfill(dest,x+dx+2,y,x+dx+4,y+2,fg);
            
        grid<<=1;
    }
}

void drawdmap(int dmap)
{
    int c;
    zcolors mc=misc.colors;
    
    switch((DMaps[dmap].type&dmfTYPE))
    {
    case dmDNGN:
    case dmCAVE:
        clear_bitmap(dmapbmp_small);
        
        if(DMaps[dmap].minimap_2_tile)
            ;
        // overworld_map_tile overrides the NES minimap. dungeon_map_tile does not.
        else for(int y=1; y<33; y+=4)
                drawgrid(dmapbmp_small,1,y,DMaps[dmap].grid[y>>2], DMaps[dmap].flags&dmfMINIMAPCOLORFIX ? mc.cave_fg : mc.dngn_fg, -1, -1);
                
        c=DMaps[dmap].compass;
        //  rectfill(dmapbmp,(c&15)*8+3,(c>>4)*4+1,(c&15)*8+5,(c>>4)*4+3,dvc(2*4));
        rectfill(dmapbmp_small,(c&15)*8+3,(c>>4)*4+1,(c&15)*8+5,(c>>4)*4+3,vc(4));
        c=DMaps[dmap].cont;
        rectfill(dmapbmp_small,(c&15)*8+3,(c>>4)*4+1,(c&15)*8+5,(c>>4)*4+3,vc(10));
        break;
        
    case dmOVERW:
        clear_bitmap(dmapbmp_small);
        
        if(DMaps[dmap].minimap_2_tile)
            ;
        else if(!mc.overworld_map_tile)
            for(int y=1; y<33; y+=4)
                drawovergrid(dmapbmp_small,1,y,DMaps[dmap].grid[y>>2],mc.overw_bg,vc(0));
                
        c=DMaps[dmap].cont;
        rectfill(dmapbmp_small,(c&15)*4+1,(c>>4)*4+1,(c&15)*4+3,(c>>4)*4+3,vc(10));
        break;
        
    case dmBSOVERW:
        clear_bitmap(dmapbmp_small);
        
        if(DMaps[dmap].minimap_2_tile)
            ;
        else if(!mc.overworld_map_tile)
            for(int y=1; y<33; y+=4)
                //    drawgrid_s(dmapbmp,1,y,DMaps[dmap].grid[y>>2],dvc(2*4),dvc(2*3),dvc(3+4));
                drawgrid_s(dmapbmp_small,1,y,DMaps[dmap].grid[y>>2],mc.bs_goal,mc.bs_dk,vc(14));
                
        c=DMaps[dmap].cont;
        rectfill(dmapbmp_small,(c&15)*8+3,(c>>4)*4+1,(c&15)*8+5,(c>>4)*4+3,vc(10));
        break;
    }
}

void drawdmap_screen(int x, int y, int w, int h, int dmap)
{
    BITMAP *tempbmp = create_bitmap_ex(8,w,h);
    clear_to_color(tempbmp, vc(0));
    zcolors mc=misc.colors;
    
//  rectfill(tempbmp,x,y,x+w-1,y+h-1,vc(0));

    if(DMaps[dmap].minimap_2_tile)
    {
        draw_block(tempbmp,0,0,DMaps[dmap].minimap_2_tile,DMaps[dmap].minimap_2_cset,5,3);
    }
    else if(((DMaps[dmap].type&dmfTYPE)==dmDNGN || (DMaps[dmap].type&dmfTYPE)==dmCAVE) && mc.dungeon_map_tile)
    {
        draw_block(tempbmp,0,0,mc.dungeon_map_tile,mc.dungeon_map_cset,5,3);
    }
    else if(((DMaps[dmap].type&dmfTYPE)==dmOVERW || (DMaps[dmap].type&dmfTYPE)==dmBSOVERW) && mc.overworld_map_tile)
    {
        draw_block(tempbmp,0,0,mc.overworld_map_tile,mc.overworld_map_cset,5,3);
    }
    
    masked_blit(dmapbmp_small,tempbmp,0,0,8,7,65,33);
    
    blit(tempbmp,screen,0,0,x,y,w,h);
    destroy_bitmap(tempbmp);
    
}

int d_dmaplist_proc(int msg,DIALOG *d,int c)
{
    if(msg==MSG_DRAW)
    {
        int dmap = d->d1;
        int *xy = (int*)(d->dp3);
        float temp_scale = 1;
        
        if(is_large())
        {
            temp_scale = 1.5; // Scale up by 1.5
        }
        
        drawdmap(dmap);
        
        if(xy[0]>-1000&&xy[1]>-1000)
        {
            int x = d->x+int((xy[0]-2)*temp_scale);
            int y = d->y+int((xy[1]-2)*temp_scale);
//      int w = is_large ? 84 : 71;
//      int h = is_large ? 52 : 39;
            int w = 84;
            int h = 52;
            jwin_draw_frame(screen,x,y,w,h,FR_DEEP);
            drawdmap_screen(x+2,y+2,w-4,h-4,dmap);
        }
        
        if(xy[2]>-1000&&xy[3]>-1000)
        {
            textprintf_ex(screen,is_large() ? lfont_l : font,d->x+int((xy[2])*temp_scale),d->y+int((xy[3])*temp_scale),jwin_pal[jcBOXFG],jwin_pal[jcBOX],"Map: %-3d",DMaps[d->d1].map+1);
        }
        
        if(xy[4]>-1000&&xy[5]>-1000)
        {
            textprintf_ex(screen,is_large() ? lfont_l : font,d->x+int((xy[4])*temp_scale),d->y+int((xy[5])*temp_scale),jwin_pal[jcBOXFG],jwin_pal[jcBOX],"Level: %-3d",DMaps[d->d1].level);
        }
    }
    
    return jwin_list_proc(msg,d,c);
}

int d_dropdmaplist_proc(int msg,DIALOG *d,int c)
{
    if(msg==MSG_DRAW)
    {
        int dmap = d->d1;
        int *xy = (int*)(d->dp3);
        float temp_scale = 1;
        
        if(is_large())
        {
            temp_scale = 1.5; // Scale up by 1.5
        }
        
        drawdmap(dmap);
        
        if(xy[0]>-1000&&xy[1]>-1000)
        {
            int x = d->x+int((xy[0]-2)*temp_scale);
            int y = d->y+int((xy[1]-2)*temp_scale);
//      int w = is_large ? 84 : 71;
//      int h = is_large ? 52 : 39;
            int w = 84;
            int h = 52;
            jwin_draw_frame(screen,x,y,w,h,FR_DEEP);
            drawdmap_screen(x+2,y+2,w-4,h-4,dmap);
        }
        
        if(xy[2]>-1000&&xy[3]>-1000)
        {
            textprintf_ex(screen,is_large() ? lfont_l : font,d->x+int((xy[2])*temp_scale),d->y+int((xy[3])*temp_scale),jwin_pal[jcBOXFG],jwin_pal[jcBOX],"Map: %-3d",DMaps[d->d1].map+1);
        }
        
        if(xy[4]>-1000&&xy[5]>-1000)
        {
            textprintf_ex(screen,is_large() ? lfont_l : font,d->x+int((xy[4])*temp_scale),d->y+int((xy[5])*temp_scale),jwin_pal[jcBOXFG],jwin_pal[jcBOX],"Level: %-3d",DMaps[d->d1].level);
        }
    }
    
    return jwin_droplist_proc(msg,d,c);
}

int d_dropdmaptypelist_proc(int msg,DIALOG *d,int c)
{
    int d1 = d->d1;
    int ret = jwin_droplist_proc(msg,d,c);
    
    if(msg==MSG_DRAW || d->d1!=d1)
    {
        small_dmap=(d->d1!=dmOVERW);
        object_message(d-3, MSG_DRAW, 0);
        (d-2)->flags&=~D_DISABLED;
        (d-2)->flags|=small_dmap?0:D_DISABLED;
        object_message(d-2, MSG_DRAW, 0);
        (d+35)->d1=small_dmap;
        object_message(d+35, MSG_DRAW, 0);        
    }
    
    return ret;
}

int d_grid_proc(int msg,DIALOG *d,int)
{
    int frame_thickness=int(2*(is_large()?1.5:1));
    int button_thickness=2;
    int header_width=int(4*(is_large()?1.5:1));
    int header_height=int(6*(is_large()?1.5:1));
    int cols=d->d1?8:16;
int col_width=(is_large() ? d->d1 ? 22:11:(d->d1?14:7));
    int l=(is_large()?10:7);
    
    switch(msg)
    {
    case MSG_DRAW:
    {
        BITMAP *tempbmp = create_bitmap_ex(8,SCREEN_W,SCREEN_H);
        clear_bitmap(tempbmp);
        int x=d->x;
        int y=d->y;
        int j=0, k=0;
        rectfill(tempbmp,x,y,x+d->w-1,y+header_height-1,jwin_pal[jcBOX]);
        
        for(j=0; j<8; ++j)
        {
            textprintf_ex(tempbmp,is_large()?nfont:spfont,x,y+header_height+frame_thickness+1+(j*l),jwin_pal[jcBOXFG],jwin_pal[jcBOX],"%d",j);
        }
        
        for(j=0; j<cols; ++j)
        {
            textprintf_ex(tempbmp,is_large()?nfont:spfont,x+header_width+frame_thickness+((col_width+1)/2)-(header_width/2)+(j*col_width),y,jwin_pal[jcBOXFG],jwin_pal[jcBOX],"%X",j);
        }
        
        jwin_draw_frame(tempbmp, x+header_width+(is_large() ? 1 : 0), y+header_height+(is_large() ? 1 : 0), (is_large()?180:116), (is_large()?84:60), FR_DEEP);
        
        for(j=0; j<8; ++j)
        {
            for(k=0; k<cols; ++k)
            {
                jwin_draw_frame(tempbmp, x+header_width+(k*col_width)+frame_thickness, y+header_height+(j*l)+frame_thickness, col_width, l, get_bit((byte *)d->dp,8*j+k)||!d->d1?FR_MEDDARK:FR_BOX);
                rectfill(tempbmp, x+header_width+(k*col_width)+frame_thickness+button_thickness, y+header_height+(j*l)+frame_thickness+button_thickness,
                         x+header_width+(k*col_width)+frame_thickness+col_width-button_thickness-1, y+header_height+(j*l)+frame_thickness+l-button_thickness-1, get_bit((byte *)d->dp,8*j+k)&&d->d1?jwin_pal[jcBOXFG]:jwin_pal[jcBOX]);
            }
        }
        
        masked_blit(tempbmp,screen,0,0,0,0,SCREEN_W,SCREEN_H);
        destroy_bitmap(tempbmp);
    }
    break;
    
    case MSG_LPRESS:
    {
        int xx = -1;
        int yy = -1;
        int set = -1; // Set or unset
        
        while(Backend::mouse->anyButtonClicked())  // Drag across to select multiple
        {
            int x=(Backend::mouse->getVirtualScreenX()-(d->x)-frame_thickness-header_width)/col_width;
            int y=(Backend::mouse->getVirtualScreenY()-(d->y)-frame_thickness-header_height)/l;
            
            if(xx != x || yy != y)
            {
                xx = x;
                yy = y;
                
                if(y>=0 && y<8 && x>=0 && x<cols)
                {
                    if(key[KEY_ALT]||key[KEY_ALTGR])
                    {
                        sprintf((char*)((d+2)->dp),"%d%X",y,x);
                        object_message((d+2), MSG_DRAW, 0);
                    }
                    
                    if(key[KEY_ZC_LCONTROL]||key[KEY_ZC_RCONTROL])
                    {
                        sprintf((char*)((d+4)->dp),"%d%X",y,x);
                        object_message((d+4), MSG_DRAW, 0);
                    }
                    
                    if(!(key[KEY_ALT]||key[KEY_ALTGR]||key[KEY_ZC_LCONTROL]||key[KEY_ZC_RCONTROL]))
                    {
                        if(set==-1)
                            set = !get_bit((byte *)d->dp,8*y+x);
                            
                        set_bit((byte *)d->dp,8*y+x,set);
                    }
                }
            }
            
            object_message(d, MSG_DRAW, 0);
			Backend::graphics->waitTick();
			Backend::graphics->showBackBuffer();
        }
    }
    break;
    }
    
    return D_O_K;
}

void drawxmap(int themap,int xoff,bool large)
{
    int cols=(large?8:16);
int col_width=(is_large() ? large ? 22:11:(large?14:7));
    int dot_width=int((large?4:3)*(is_large()?1.5:1));
    int dot_offset=int((large?5:2)*(is_large()?1.5:1));
    int l = is_large()?10:7;
    clear_to_color(dmapbmp_large,jwin_pal[jcBOX]);
    
    for(int y=0; y<8; y++)
    {
        for(int x=0; x<cols; x++)
        {
            if(!large||(x+xoff>=0 && x+xoff<=15))
            {
                mapscr *scr = &TheMaps[themap*MAPSCRS + y*16+x+(large?xoff:0)];
                rectfill(dmapbmp_large,x*col_width,y*l,x*col_width+col_width-1,(y*l)+l-1,scr->valid&mVALID ? lc1((scr->color)&15) : 0);
                
                if(scr->valid&mVALID && ((scr->color)&15)>0)
                {
                    rectfill(dmapbmp_large,x*col_width+dot_offset,y*l+2+(is_large() ? 1 : 0),x*col_width+dot_offset+dot_width-1,y*l+4+ (is_large() ? 1 : 0) *2,lc2((scr->color)&15));
                }
            }
        }
    }
}

int d_xmaplist_proc(int msg,DIALOG *d,int c)
{
    int d1 = d->d1;
    int ret = jwin_droplist_proc(msg,d,c);
    
    if(msg==MSG_DRAW || d->d1!=d1)
    {
        int *xy = (int*)(d->dp3);
        xy[0]=d->d1;
        drawxmap(xy[0],xy[1],small_dmap);
        
        if(xy[2]||xy[3])
        {
            int frame_thickness=int(2*(is_large()?1.5:1));
            int header_width=int(4*(is_large()?1.5:1));
            int header_height=int(6*(is_large()?1.5:1));
            int cols=small_dmap?8:16;
int col_width=(is_large() ? small_dmap ? 22:11:(small_dmap?14:7));
            int x=d->x+xy[2];
            int y=d->y+xy[3];
            int j=0;
            rectfill(screen,x,y-header_height-frame_thickness-(is_large() ? 1 : 0),int(x+116*(is_large()?1.5:1)-1),y-1,jwin_pal[jcBOX]);
            
            for(j=0; j<8; ++j)
            {
                textprintf_ex(screen,is_large()?nfont:spfont,x-header_width-frame_thickness,y+1+(j*(is_large()?10:7)),jwin_pal[jcBOXFG],jwin_pal[jcBOX],"%d",j);
            }
            
            for(j=0; j<cols; ++j)
            {
                textprintf_ex(screen,is_large()?nfont:spfont,x+((col_width+1)/2)-(header_width/2)+(j*col_width),y-header_height-frame_thickness,jwin_pal[jcBOXFG],jwin_pal[jcBOX],"%X",j);
            }
            
            jwin_draw_frame(screen, (x-frame_thickness)+(is_large() ? 1 : 0), (y-frame_thickness)+ (is_large() ? 1 : 0), is_large()?180:116, is_large()?84:60, FR_DEEP);
            blit(dmapbmp_large,screen,0,0,x,y, (is_large() ? 177 : 113), (is_large() ? 81 : 57));
        }
        
        //slider is disabled if
        (d+1)->flags&=~D_DISABLED;
        (d+1)->flags|=small_dmap?0:D_DISABLED;
        object_message(d+1, MSG_DRAW, 0);
    }
    
    return ret;
}

int xmapspecs[4] = {0,0,2,26};

int onXslider(void *dp3,int d2)
{
    int *x=(int *)dp3;
    int *y=x+1;

    xmapspecs[1]=d2-7;
    bound(xmapspecs[1],-7,15);
    drawxmap(xmapspecs[0],xmapspecs[1],small_dmap);
    blit(dmapbmp_large,screen,0,0,(*x)+xmapspecs[2],(*y)+xmapspecs[3], (is_large() ? 177 : 113), (is_large() ? 81 : 57));
	return D_O_K;
}

const char *dmaptype_str[dmMAX] = { "NES Dungeon","Overworld","Interior","BS-Overworld" };

const char *typelist(int index, int *list_size)
{
    if(index>=0)
    {
        bound(index,0,dmMAX-1);
        return dmaptype_str[index];
    }
    
    *list_size=dmMAX;
    return NULL;
}

bool edit_ins_mode=true;

void put_title_str(char *s,int x,int y,int fg,int bg,int pos,int lines,int cpl)
{
    int i=0;
    
    // text_mode(bg);
    for(int dy=0; dy<lines; dy++)
        for(int dx=0; dx<cpl; dx++)
        {
            if(edit_ins_mode)
            {
                textprintf_ex(screen,zfont,x+(dx<<3),y+(dy<<3),fg,bg,"%c",*(s+i));
            }
            else
            {
                //     text_mode(i==pos?vc(15):bg);
                
                textprintf_ex(screen,zfont,x+(dx<<3),y+(dy<<3),i==pos?bg:fg,i==pos?vc(15):bg,"%c",*(s+i));
            }
            
            ++i;
        }
        
    if(edit_ins_mode&&pos>-1)
    {
        //   text_mode(-1);
        textprintf_ex(screen,zfont,x+((pos%cpl)<<3),y+((pos/cpl)<<3),vc(15),-1,"_");
    }
}

int d_title_edit_proc(int msg,DIALOG *d,int c)
{
    char *s=(char*)(d->dp);
    
    switch(msg)
    {
    case MSG_WANTFOCUS:
        return D_WANTFOCUS;
        
    case MSG_CLICK:
        d->d2=((Backend::mouse->getVirtualScreenX()-d->x)>>3)+((Backend::mouse->getVirtualScreenY()-d->y)>>3)*10;
        bound(d->d2,0,19);
        put_title_str(s,d->x,d->y,jwin_pal[jcTEXTBG],jwin_pal[jcTEXTFG],d->d2,2,10);
        
        while(Backend::mouse->anyButtonClicked())
        {
			Backend::graphics->waitTick();
			Backend::graphics->showBackBuffer();
            /* do nothing */
        }
        
        break;
        
    case MSG_DRAW:
        if(!(d->flags & D_GOTFOCUS))
        {
            d->d2=-1;
        }
        
        put_title_str(s,d->x,d->y,jwin_pal[jcTEXTBG],jwin_pal[jcTEXTFG],d->d2,2,10);
        break;
        
    case MSG_CHAR:
        bool used=false;
        int k=c>>8;
        
        switch(k)
        {
        case KEY_INSERT:
            edit_ins_mode=!edit_ins_mode;
            used=true;
            break;
            
        case KEY_HOME:
            d->d2-=d->d2%10;
            used=true;
            break;
            
        case KEY_END:
            d->d2-=d->d2%10;
            d->d2+=9;
            used=true;
            break;
            
        case KEY_UP:
            if(d->d2>=10) d->d2-=10;
            
            used=true;
            break;
            
        case KEY_DOWN:
            if(d->d2<10) d->d2+=10;
            
            used=true;
            break;
            
        case KEY_LEFT:
            if(d->d2>0) --d->d2;
            
            used=true;
            break;
            
        case KEY_RIGHT:
            if(d->d2<19) ++d->d2;
            
            used=true;
            break;
            
        case KEY_BACKSPACE:
            if(d->d2>0)
                --d->d2;
                
        case KEY_DEL:
            strcpy(s+d->d2,s+d->d2+1);
            s[19]=' ';
            s[20]=0;
            used=true;
            break;
            
        default:
            if(isprint(c&255))
            {
                if(edit_ins_mode)
                {
                    for(int i=19; i>d->d2; i--)
                        s[i]=s[i-1];
                }
                
                s[d->d2]=c&255;
                
                if(d->d2<19)
                    ++d->d2;
                    
                used=true;
            }
        }
        
        put_title_str(s,d->x,d->y,jwin_pal[jcTEXTBG],jwin_pal[jcTEXTFG],d->d2,2,10);
        return used?D_USED_CHAR:D_O_K;
    }
    
    return D_O_K;
}

void put_intro_str(char *s,int x,int y,int fg,int bg,int pos)
{
    int i=0;
    
    // text_mode(bg);
    for(int dy=0; dy<3; dy++)
        for(int dx=0; dx<24; dx++)
        {
            if(edit_ins_mode)
            {
                textprintf_ex(screen,zfont,x+(dx<<3),y+(dy<<3),fg,bg,"%c",*(s+i));
            }
            else
            {
                //     text_mode(i==pos?vc(15):bg);
                textprintf_ex(screen,zfont,x+(dx<<3),y+(dy<<3),i==pos?bg:fg,i==pos?vc(15):bg,"%c",*(s+i));
            }
            
            ++i;
        }
        
    if(edit_ins_mode&&pos>-1)
    {
        //   text_mode(-1);
        textprintf_ex(screen,zfont,x+((pos%24)<<3),y+((pos/24)<<3),vc(15),-1,"_");
    }
}

int d_intro_edit_proc(int msg,DIALOG *d,int c)
{
    char *s=(char*)(d->dp);
    
    switch(msg)
    {
    case MSG_WANTFOCUS:
        return D_WANTFOCUS;
        
    case MSG_CLICK:
        d->d2=((Backend::mouse->getVirtualScreenX()-d->x)>>3)+((Backend::mouse->getVirtualScreenY()-d->y)>>3)*24;
        bound(d->d2,0,71);
        put_intro_str(s,d->x,d->y,jwin_pal[jcTEXTBG],jwin_pal[jcTEXTFG],d->d2);
        
        while(Backend::mouse->anyButtonClicked())
        {
			Backend::graphics->waitTick();
			Backend::graphics->showBackBuffer();
            /* do nothing */
        }
        
        break;
        
    case MSG_DRAW:
    
        if(!(d->flags & D_GOTFOCUS))
        {
            d->d2=-1;
            
        }
        
        put_intro_str(s,d->x,d->y,jwin_pal[jcTEXTBG],jwin_pal[jcTEXTFG],d->d2);
        break;
        
    case MSG_CHAR:
        bool used=false;
        int k=c>>8;
        
        switch(k)
        {
        case KEY_INSERT:
            edit_ins_mode=!edit_ins_mode;
            used=true;
            break;
            
        case KEY_HOME:
            d->d2-=d->d2%24;
            used=true;
            break;
            
        case KEY_END:
            d->d2-=d->d2%24;
            d->d2+=23;
            used=true;
            break;
            
        case KEY_UP:
            if(d->d2>=24) d->d2-=24;
            
            used=true;
            break;
            
        case KEY_DOWN:
            if(d->d2<48) d->d2+=24;
            
            used=true;
            break;
            
        case KEY_LEFT:
            if(d->d2>0) --d->d2;
            
            used=true;
            break;
            
        case KEY_RIGHT:
            if(d->d2<71) ++d->d2;
            
            used=true;
            break;
            
        case KEY_BACKSPACE:
            if(d->d2>0)
                --d->d2;
                
        case KEY_DEL:
            strcpy(s+d->d2,s+d->d2+1);
            s[71]=' ';
            s[72]=0;
            used=true;
            break;
            
        default:
            if(isprint(c&255))
            {
                if(edit_ins_mode)
                {
                    for(int i=71; i>d->d2; i--)
                        s[i]=s[i-1];
                }
                
                s[d->d2]=c&255;
                
                if(d->d2<71)
                    ++d->d2;
                    
                used=true;
            }
        }
        
        put_intro_str(s,d->x,d->y,jwin_pal[jcTEXTBG],jwin_pal[jcTEXTFG],d->d2);
        return used?D_USED_CHAR:D_O_K;
    }
    
    return D_O_K;
}

char dmap_title[21];
char dmap_name[33];
char dmap_intro[73];


static int editdmap_mechanics_list[] =
{
    // dialog control number
    19, 20, 21, 22, 23, 24, 25, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, -1
};

/*
static int editdmap_continue_list[] =
{
  120,-1
};
*/

static int editdmap_appearance_list[] =
{
    // dialog control number
    64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, -1
};

static int editdmap_music_list[] =
{
    // dialog control number
    82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, -1
};

static int editdmap_subscreenmaps_list[] =
{
    // dialog control number
    6, -1
};

static int editdmap_disableitems_list[] =
{
    // dialog control number
    100,101,102,103,104,105,-1
};

static int editdmap_flags_list[] =
{
    110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,-1
};

static TABPANEL editdmap_tabs[] =
{
    // (text)
    { (char *)"Mechanics",      D_SELECTED,  editdmap_mechanics_list,      0,  NULL },
    { (char *)"Appearance",     0,           editdmap_appearance_list,     0,  NULL },
    { (char *)"Music",          0,           editdmap_music_list,          0,  NULL },
    { (char *)"Maps",           0,           editdmap_subscreenmaps_list,  0,  NULL },
    { (char *)"Flags",          0,           editdmap_flags_list,          0,  NULL },
    { (char *)"Disable",        0, 		   editdmap_disableitems_list,   0,  NULL },
    { NULL,                     0,           NULL,                         0,  NULL }
};

static int editdmapmap_before_list[] =
{
    // dialog control number
    7, 8, 9, 10, 11, 12, -1
};

static int editdmapmap_after_list[] =
{
    // dialog control number
    13, 14, 15, 16, 17, 18, 26, 27, -1
};

static TABPANEL editdmapmap_tabs[] =
{
    // (text)
    { (char *)"Without Map",  D_SELECTED,  editdmapmap_before_list, 0, NULL },
    { (char *)"With Map",     0,           editdmapmap_after_list, 0, NULL },
    { NULL,                   0,           NULL, 0, NULL }
};

int dmap_tracks=0;
static char dmap_track_number_str_buf[32];
const char *dmaptracknumlist(int index, int *list_size)
{
    if(index>=0)
    {
        bound(index,0,255);
        sprintf(dmap_track_number_str_buf,"%02d",index+1);
        return dmap_track_number_str_buf;
    }
    
    *list_size=dmap_tracks;
    return NULL;
}

extern const char *subscreenlist_a(int index, int *list_size);
extern const char *subscreenlist_b(int index, int *list_size);

static ListData subscreen_list_a(subscreenlist_a, &font);
static ListData subscreen_list_b(subscreenlist_b, &font);
static ListData midi_list(midilist, &font);
static ListData dmaptracknum_list(dmaptracknumlist, &font);
static ListData type_list(typelist, &font);
static ListData gotomap_list(gotomaplist, &font);

static DIALOG editdmap_dlg[] =
{
    // (dialog proc)                (x)     (y)     (w)     (h)     (fg)                    (bg)                 (key)     (flags)   (d1)           (d2)   (dp)                                                   (dp2)                 (dp3)
    {  jwin_win_proc,                 0,      0,    312,    221,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    D_EXIT,      0,             0, (void *) "DMap Editor",                                NULL,                 NULL                  },
    {  jwin_button_proc,             89,    196,     61,     21,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],        13,    D_EXIT,      0,             0, (void *) "OK",                                         NULL,                 NULL                  },
    {  jwin_button_proc,            164,    196,     61,     21,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],        27,    D_EXIT,      0,             0, (void *) "Cancel",                                     NULL,                 NULL                  },
    {  jwin_text_proc,               10,     29,     48,      8,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           0,             0, (void *) "Name: ",                                     NULL,                 NULL                  },
    {  jwin_edit_proc,               40,     25,    168,     16,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,          20,             0,  NULL,                                                  NULL,                 NULL                  },
    //5
    {  jwin_tab_proc,                 6,     45,    300,    144,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           0,             0, (void *) editdmap_tabs,                                NULL, (void *)editdmap_dlg  },
    {  jwin_tab_proc,                10,     65,    292,    116,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           0,             0, (void *) editdmapmap_tabs,                             NULL, (void *)editdmap_dlg  },
    {  jwin_ctext_proc,              67,     87,      0,      8,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           0,             0, (void *) "Minimap",                                    NULL,                 NULL                  },
    {  jwin_frame_proc,              31,     95,     84,     52,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           FR_DEEP,       0,  NULL,                                                  NULL,                 NULL                  },
    {  d_maptile_proc,               33,     97,     80,     48,    0,                      0,                       0,    0,           0,             0,  NULL, (void*)0,             NULL                  },
    //10
    {  jwin_ctext_proc,             207,     87,      0,      8,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           0,             0, (void *) "Large",                                      NULL,                 NULL                  },
    {  jwin_frame_proc,             133,     95,    148,     84,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           FR_DEEP,       0,  NULL,                                                  NULL,                 NULL                  },
    {  d_maptile_proc,              135,     97,    144,     80,    0,                      0,                       0,    0,           0,             0,  NULL, (void*)0,             NULL                  },
    {  jwin_ctext_proc,              67,     87,      0,      8,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           0,             0, (void *) "Minimap",                                    NULL,                 NULL                  },
    {  jwin_frame_proc,              31,     95,     84,     52,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           FR_DEEP,       0,  NULL,                                                  NULL,                 NULL                  },
    //15
    {  d_maptile_proc,               33,     97,     80,     48,    0,                      0,                       0,    0,           0,             0,  NULL, (void*)0,             NULL                  },
    {  jwin_ctext_proc,             207,     87,      0,      8,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           0,             0, (void *) "Large",                                      NULL,                 NULL                  },
    {  jwin_frame_proc,             133,     95,    148,     84,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           FR_DEEP,       0,  NULL,                                                  NULL,                 NULL                  },
    {  d_maptile_proc,              135,     97,    144,     80,    0,                      0,                       0,    0,           0,             0,  NULL, (void*)0,             NULL                  },
    {  jwin_text_proc,               12,     69,     48,      8,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           0,             0, (void *) "Map:",                                       NULL,                 NULL                  },
    //20
    {  d_xmaplist_proc,              36,     65,     54,     16,    jwin_pal[jcTEXTFG],     jwin_pal[jcTEXTBG],      0,    0,           1,             0, (void *) &gotomap_list,                                NULL,                 xmapspecs             },
    {  jwin_slider_proc,             38,    151,    111,     10,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,          22,             0,  NULL, (void *) onXslider,   NULL                  },
    {  jwin_text_proc,              103,     69,     64,      8,    vc(14),                 vc(1),                   0,    0,           0,             0, (void *) "Type: ",                                     NULL,                 NULL                  },
    {  d_dropdmaptypelist_proc,     132,     65,     99,     16,    jwin_pal[jcTEXTFG],     jwin_pal[jcTEXTBG],      0,    0,           1,             0, (void *) &type_list,                                   NULL,                 NULL                  },
    {  jwin_text_proc,              243,     69,     48,      8,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           0,             0, (void *) "Level:",                                     NULL,                 NULL                  },
    //25
    {  jwin_edit_proc,              274,     65,     26,     16,    jwin_pal[jcTEXTFG],     jwin_pal[jcTEXTBG],      0,    0,           3,             0,  NULL,                                                  NULL,                 NULL                  },
    {  jwin_text_proc,              28,    150,     70,      8,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           0,             0, (void *) "Setting this tile disables",                                NULL,                 NULL                  },
    {  jwin_text_proc,              28,    158,     70,      8,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           0,             0, (void *) "the classic NES minimap.",                                NULL,                 NULL                  },
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    //30
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    //35
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    //40
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    //45
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    //50
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    //55
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    {  d_grid_proc,                 162,     83,    124,     66,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    {  jwin_text_proc,              162,    155,     48,      8,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           0,             0, (void *) "Compass: 0x",                                NULL,                 NULL                  },
    //60
    {  jwin_edit_proc,              218,    151,     21,     16,    jwin_pal[jcTEXTFG],     jwin_pal[jcTEXTBG],      0,    0,           2,             0,  NULL,                                                  NULL,                 NULL                  },
    {  jwin_text_proc,              162,    173,     48,      8,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           0,             0, (void *) "Continue: 0x",                               NULL,                 NULL                  },
    {  jwin_edit_proc,              218,    169,     21,     16,    jwin_pal[jcTEXTFG],     jwin_pal[jcTEXTBG],      0,    0,           2,             0,  NULL,                                                  NULL,                 NULL                  },
    {  jwin_check_proc,              76,    173,    113,      9,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           1,             0, (void *) "Continue here",                              NULL,                 NULL                  },
    {  jwin_text_proc,               12,     69,     48,      8,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           0,             0, (void *) "Color:",                                     NULL,                 NULL                  },
    //65
    {  jwin_droplist_proc,           42,     65,    161,     16,    jwin_pal[jcTEXTFG],     jwin_pal[jcTEXTBG],      0,    0,           1,             0, (void *) &levelnum_list,                               NULL,                 NULL                  },
    {  jwin_ctext_proc,              55,     85,      0,      8,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           0,             0, (void *) "DMap Title",                                 NULL,                 NULL                  },
    {  jwin_frame_proc,              13,     93,     84,     20,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           FR_DEEP,       0,  NULL,                                                  NULL,                 NULL                  },
    {  d_title_edit_proc,            15,     95,     80,     16,    jwin_pal[jcTEXTBG],     jwin_pal[jcTEXTFG],      0,    0,           0,             0, (void *) dmap_title,                                   NULL,                 NULL                  },
    {  jwin_ctext_proc,             201,     85,      0,      8,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           0,             0, (void *) "DMap Intro",                                 NULL,                 NULL                  },
    //70
    {  jwin_frame_proc,             103,     93,    196,     28,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           FR_DEEP,       0,  NULL,                                                  NULL,                 NULL                  },
    {  d_intro_edit_proc,           105,     95,    192,     24,    jwin_pal[jcTEXTBG],     jwin_pal[jcTEXTFG],      0,    0,           0,             0, (void *) dmap_intro,                                   NULL,                 NULL                  },
    {  jwin_frame_proc,              12,    127,    223,     44,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           FR_ETCHED,     0,  NULL,                                                  NULL,                 NULL                  },
    {  jwin_text_proc,               20,    124,     48,      8,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           0,             0, (void *) " Subscreens ",                               NULL,                 NULL                  },
    {  jwin_text_proc,               16,    137,     48,      8,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           0,             0, (void *) "Active:",                                    NULL,                 NULL                  },
    //75
    {  jwin_droplist_proc,           57,    133,    174,     16,    jwin_pal[jcTEXTFG],     jwin_pal[jcTEXTBG],      0,    0,           1,             0, (void *) &subscreen_list_a,                            NULL,                 NULL                  },
    {  jwin_text_proc,               16,    155,     48,      8,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           0,             0, (void *) "Passive:",                                   NULL,                 NULL                  },
    {  jwin_droplist_proc,           57,    151,    174,     16,    jwin_pal[jcTEXTFG],     jwin_pal[jcTEXTBG],      0,    0,           1,             0, (void *) &subscreen_list_b,                            NULL,                 NULL                  },
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    //80
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    {  jwin_text_proc,               12,     69,     48,      8,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           0,             0, (void *) "Midi:",                                      NULL,                 NULL                  },
    {  jwin_droplist_proc,           35,     65,    153,     16,    jwin_pal[jcTEXTFG],     jwin_pal[jcTEXTBG],      0,    0,           1,             0, (void *) &midi_list,                                   NULL,                 NULL                  },
    {  jwin_frame_proc,              12,     86,    176,     68,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           FR_ETCHED,     0,  NULL,                                                  NULL,                 NULL                  },
    //85
    {  jwin_text_proc,               20,     83,     48,      8,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           0,             0, (void *) " Enhanced Music ",                           NULL,                 NULL                  },
    {  jwin_frame_proc,              16,     92,    168,     16,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           FR_DEEP,       0,  NULL,                                                  NULL,                 NULL                  },
    {  jwin_text_proc,               19,     96,    162,     16,    jwin_pal[jcTEXTFG],     jwin_pal[jcTEXTBG],      0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    {  jwin_text_proc,               16,    114,     48,      8,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           0,             0, (void *) "Track:",                                     NULL,                 NULL                  },
    {  jwin_droplist_proc,           50,    110,    134,     16,    jwin_pal[jcTEXTFG],     jwin_pal[jcTEXTBG],      0,    0,           1,             0, (void *) &dmaptracknum_list,                           NULL,                 NULL                  },
    //90
    {  jwin_button_proc,             31,    129,     61,     21,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],        13,    D_EXIT,      0,             0, (void *) "Load",                                       NULL,                 NULL                  },
    {  jwin_button_proc,            108,    129,     61,     21,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],        13,    D_EXIT,      0,             0, (void *) "Clear",                                      NULL,                 NULL                  },
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    //95
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    //100
    {  jwin_text_proc,               12,     69,     48,      8,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           0,             0, (void *) "Disabled Items:",                            NULL,                 NULL                  },
    {  jwin_abclist_proc,            12,     81,    120,    104,    jwin_pal[jcTEXTFG],     jwin_pal[jcTEXTBG],      0,    D_EXIT,      0,             0,  NULL,                                                  NULL,                 NULL                  },
    {  jwin_abclist_proc,           177,     81,    120,    104,    jwin_pal[jcTEXTFG],     jwin_pal[jcTEXTBG],      0,    D_EXIT,      0,             0,  NULL,                                                  NULL,                 NULL                  },
    {  jwin_text_proc,              177,     69,     48,      8,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           0,             0, (void *) "All Items:",                                 NULL,                 NULL                  },
    {  jwin_button_proc,            146,    105,     20,     20,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],        13,    D_EXIT,      0,             0, (void *) "->",                                         NULL,                 NULL                  },
    //105
    {  jwin_button_proc,            146,    145,     20,     20,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],        13,    D_EXIT,      0,             0, (void *) "<-",                                         NULL,                 NULL                  },
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    {  d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    //110
    {  jwin_check_proc,              12,     65,    113,      9,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           1,             0, (void *) "Use Caves Instead Of Item Cellars",          NULL,                 NULL                  },
    {  jwin_check_proc,              12,     75,    113,      9,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           1,             0, (void *) "Allow 3-Stair Warp Rooms",                   NULL,                 NULL                  },
    {  jwin_check_proc,              12,     85,    113,      9,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           1,             0, (void *) "Allow Whistle Whirlwinds",                   NULL,                 NULL                  },
    {  jwin_check_proc,              12,    105,    113,      9,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           1,             0, (void *) "Special Rooms And Guys Are In Caves Only",   NULL,                 NULL                  },
    {  jwin_check_proc,              12,    115,    113,      9,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           1,             0, (void *) "Don't Display Compass Marker In Minimap",    NULL,                 NULL                  },
    {  jwin_check_proc,              12,    125,    113,      9,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           1,             0, (void *) "Underwater Wave Effect",                     NULL,                 NULL                  },
    {  jwin_check_proc,              12,     95,    113,      9,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           1,             0, (void *) "Whistle Whirlwind Returns Link To Start",    NULL,                 NULL                  },
    {  jwin_check_proc,              12,    135,    113,      9,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           1,             0, (void *) "Always Display Intro String",                NULL,                 NULL                  },
    {  jwin_check_proc,              12,    145,    113,      9,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           1,             0, (void *) "View Overworld Map By Pressing 'Map'",       NULL,                 NULL                  },
    {  jwin_check_proc,              12,    155,    113,      9,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           1,             0, (void *) "...But Only Show Screens Marked In Minimap",      NULL,                 NULL                  },
    {  jwin_check_proc,              12,    165,    113,      9,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           1,             0, (void *) "Use Minimap Foreground Color 2",      NULL,                 NULL                  },
    //121
    {  jwin_check_proc,             230,     65,    113,      9,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           1,             0, (void *) "Script 1",        					   NULL,                 NULL                  },
    {  jwin_check_proc,             230,     75,    113,      9,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           1,             0, (void *) "Script 2",        					   NULL,                 NULL                  },
    {  jwin_check_proc,             230,     85,    113,      9,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           1,             0, (void *) "Script 3",        					   NULL,                 NULL                  },
    {  jwin_check_proc,             230,     95,    113,      9,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           1,             0, (void *) "Script 4",        					   NULL,                 NULL                  },
    {  jwin_check_proc,             230,    105,    113,      9,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,    0,           1,             0, (void *) "Script 5",        					   NULL,                 NULL                  },
    {  d_timer_proc,                  0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  },
    {  NULL,                          0,      0,      0,      0,    0,                      0,                       0,    0,           0,             0,  NULL,                                                  NULL,                 NULL                  }
};

void editdmap(int index)
{
    char levelstr[4], compassstr[4], contstr[4], tmusicstr[56], dmapnumstr[60];
    char *tmfname;
    byte gridstring[8];
    static int xy[2];
    sprintf(levelstr,"%d",DMaps[index].level);
    sprintf(dmapnumstr,"Edit DMap (%d)",index);
    sprintf(compassstr,"%02X",DMaps[index].compass);
    sprintf(contstr,"%02X",DMaps[index].cont);
    sprintf(dmap_title,"%s",DMaps[index].title);
    sprintf(dmap_name,"%s",DMaps[index].name);
    sprintf(dmap_intro,"%s",DMaps[index].intro);
    sprintf(tmusicstr,"%s",DMaps[index].tmusic);
    editdmap_dlg[0].dp=dmapnumstr;
    editdmap_dlg[0].dp2=lfont;
    editdmap_dlg[4].dp=dmap_name;
    editdmap_dlg[9].d1 = DMaps[index].minimap_1_tile;
    editdmap_dlg[9].fg = DMaps[index].minimap_1_cset;
    editdmap_dlg[12].d1 = DMaps[index].largemap_1_tile;
    editdmap_dlg[12].fg = DMaps[index].largemap_1_cset;
    editdmap_dlg[15].d1 = DMaps[index].minimap_2_tile;
    editdmap_dlg[15].fg = DMaps[index].minimap_2_cset;
    editdmap_dlg[18].d1 = DMaps[index].largemap_2_tile;
    editdmap_dlg[18].fg = DMaps[index].largemap_2_cset;
    editdmap_dlg[20].d1=(DMaps[index].map>(map_count-1))?0:DMaps[index].map;
    xy[0]=editdmap_dlg[20].x;
    xy[1]=editdmap_dlg[20].y;
    editdmap_dlg[21].dp3=xy;
    xmapspecs[1]=DMaps[index].xoff;
    editdmap_dlg[21].d2=DMaps[index].xoff+7;
    editdmap_dlg[23].d1=(DMaps[index].type&dmfTYPE);
    editdmap_dlg[25].dp=levelstr;
    
    for(int i=0; i<32; i++)
    {
        editdmap_dlg[26+i].dp2=is_large()?nfont:spfont;
    }
    
    for(int i=0; i<8; i++)
    {
        for(int j=0; j<8; j++)
        {
            set_bit(gridstring,8*i+j,get_bit((byte *)(DMaps[index].grid+i),7-j));
        }
    }
    
    editdmap_dlg[58].dp=gridstring;
    editdmap_dlg[60].dp=compassstr;
    editdmap_dlg[62].dp=contstr;
    editdmap_dlg[63].flags = (DMaps[index].type&dmfCONTINUE) ? D_SELECTED : 0;
    editdmap_dlg[65].d1=DMaps[index].color;
    editdmap_dlg[75].d1=DMaps[index].active_subscreen;
    editdmap_dlg[77].d1=DMaps[index].passive_subscreen;
    editdmap_dlg[83].d1=DMaps[index].midi;
    editdmap_dlg[87].dp=tmusicstr;
    dmap_tracks=0;
    ZCMUSIC *tempdmapzcmusic = (ZCMUSIC*)zcmusic_load_file(tmusicstr);
    
    // Failed to load - try the quest directory
    if(tempdmapzcmusic==NULL)
    {
        char musicpath[256];
        replace_filename(musicpath, filepath, tmusicstr, 256);
        tempdmapzcmusic = (ZCMUSIC*)zcmusic_load_file(musicpath);
    }
    
    if(tempdmapzcmusic!=NULL)
    {
        dmap_tracks=zcmusic_get_tracks(tempdmapzcmusic);
        dmap_tracks=(dmap_tracks<2)?0:dmap_tracks;
    }
    
    zcmusic_unload_file(tempdmapzcmusic);
    editdmap_dlg[89].flags=(dmap_tracks<2)?D_DISABLED:0;
    editdmap_dlg[89].d1=vbound(DMaps[index].tmusictrack,0,dmap_tracks > 0 ? dmap_tracks-1 : 0);
    
    build_bii_list(false);
    initDI(index);
    ListData DI_list(DIlist, &font);
    ListData item_list(itemlist, &font);
    editdmap_dlg[101].dp = (void*)&DI_list;
    editdmap_dlg[101].d1 = 0;
    editdmap_dlg[102].dp = (void*)&item_list;
    editdmap_dlg[102].d1 = 0;
    
    editdmap_dlg[110].flags = (DMaps[index].flags& dmfCAVES)? D_SELECTED : 0;
    editdmap_dlg[111].flags = (DMaps[index].flags& dmf3STAIR)? D_SELECTED : 0;
    editdmap_dlg[112].flags = (DMaps[index].flags& dmfWHIRLWIND)? D_SELECTED : 0;
    editdmap_dlg[113].flags = (DMaps[index].flags& dmfGUYCAVES)? D_SELECTED : 0;
    editdmap_dlg[114].flags = (DMaps[index].flags& dmfNOCOMPASS)? D_SELECTED : 0;
    editdmap_dlg[115].flags = (DMaps[index].flags& dmfWAVY)? D_SELECTED : 0;
    editdmap_dlg[116].flags = (DMaps[index].flags& dmfWHIRLWINDRET)? D_SELECTED : 0;
    editdmap_dlg[117].flags = (DMaps[index].flags& dmfALWAYSMSG) ? D_SELECTED : 0;
    editdmap_dlg[118].flags = (DMaps[index].flags& dmfVIEWMAP) ? D_SELECTED : 0;
    editdmap_dlg[119].flags = (DMaps[index].flags& dmfDMAPMAP) ? D_SELECTED : 0;
    editdmap_dlg[120].flags = (DMaps[index].flags& dmfMINIMAPCOLORFIX) ? D_SELECTED : 0;
    
    editdmap_dlg[121].flags = (DMaps[index].flags& dmfSCRIPT1) ? D_SELECTED : 0;
    editdmap_dlg[122].flags = (DMaps[index].flags& dmfSCRIPT2) ? D_SELECTED : 0;
    editdmap_dlg[123].flags = (DMaps[index].flags& dmfSCRIPT3) ? D_SELECTED : 0;
    editdmap_dlg[124].flags = (DMaps[index].flags& dmfSCRIPT4) ? D_SELECTED : 0;
    editdmap_dlg[125].flags = (DMaps[index].flags& dmfSCRIPT5) ? D_SELECTED : 0;

	DIALOG *editdmap_cpy = resizeDialog(editdmap_dlg, 1.5);
    
    if(is_large())
    {
		editdmap_cpy[7].x+=4;
		editdmap_cpy[13].x+=4;
		editdmap_cpy[26].y-=12;
		editdmap_cpy[27].y-=12;
		editdmap_cpy[59].x+=10;
		editdmap_cpy[61].x+=10;
        
        int dest[6] = { 11, 17, 14, 8, 67, 70 };
        int src[6] = { 12, 12, 9, 9, 68, 71 };
        
        for(int i=0; i<6; i++)
        {
			editdmap_cpy[dest[i]].w = editdmap_cpy[src[i]].w+4;
			editdmap_cpy[dest[i]].h = editdmap_cpy[src[i]].h+4;
			editdmap_cpy[dest[i]].x = editdmap_cpy[src[i]].x-2;
			editdmap_cpy[dest[i]].y = editdmap_cpy[src[i]].y-2;
        }
		xmapspecs[2] = 3;
		xmapspecs[3] = 36;
    }
	else
	{
		xmapspecs[2] = 2;
		xmapspecs[3] = 24;
	}
	xy[0] = editdmap_cpy[20].x;
	xy[1] = editdmap_cpy[20].y;

    int ret=-1;
    
    while(ret!=0&&ret!=1&&ret!=2)
    {
        ret=zc_popup_dialog(editdmap_cpy,-1);
        
        switch(ret)
        {
        case 90:                                              //grab a filename for tracker music
        {
            if(getname("Load DMap Music",(char*)zcmusic_types,NULL,tmusicpath,false))
            {
                strcpy(tmusicpath,temppath);
                tmfname=get_filename(tmusicpath);
                
                if(strlen(tmfname)>55)
                {
                    jwin_alert("Error","Filename too long","(>55 characters",NULL,"O&K",NULL,'k',0,lfont);
                    temppath[0]=0;
                }
                else
                {
                    sprintf(tmusicstr,"%s",tmfname);
					editdmap_cpy[87].dp=tmusicstr;
                    dmap_tracks=0;
                    tempdmapzcmusic = (ZCMUSIC*)zcmusic_load_file(tmusicstr);
                    
                    // Failed to load - try the quest directory
                    if(tempdmapzcmusic==NULL)
                    {
                        char musicpath[256];
                        replace_filename(musicpath, filepath, tmusicstr, 256);
                        tempdmapzcmusic = (ZCMUSIC*)zcmusic_load_file(musicpath);
                    }
                    
                    if(tempdmapzcmusic!=NULL)
                    {
                        dmap_tracks=zcmusic_get_tracks(tempdmapzcmusic);
                        dmap_tracks=(dmap_tracks<2)?0:dmap_tracks;
                    }
                    
                    zcmusic_unload_file(tempdmapzcmusic);
					editdmap_cpy[89].flags=(dmap_tracks<2)?D_DISABLED:0;
					editdmap_cpy[89].d1=0;
                }
            }
        }
        break;
        
        case 91:                                              //clear tracker music
            memset(tmusicstr, 0, 56);
			editdmap_cpy[89].flags=D_DISABLED;
			editdmap_cpy[89].d1=0;
            break;
            
        case 104: 											// item disable "->"
            deleteDI(editdmap_cpy[101].d1, index);
            break;
            
        case 105: 											// item disable "<-"
        {
            // 101 is the disabled list, 102 the item list
            insertDI(editdmap_cpy[102].d1, index);
        }
        break;
        }
    }
    
    if(ret==1)
    {
        saved=false;
        sprintf(DMaps[index].name,"%s",dmap_name);
        DMaps[index].minimap_1_tile = editdmap_cpy[9].d1;
        DMaps[index].minimap_1_cset = editdmap_cpy[9].fg;
        DMaps[index].largemap_1_tile = editdmap_cpy[12].d1;
        DMaps[index].largemap_1_cset = editdmap_cpy[12].fg;
        DMaps[index].minimap_2_tile = editdmap_cpy[15].d1;
        DMaps[index].minimap_2_cset = editdmap_cpy[15].fg;
        DMaps[index].largemap_2_tile = editdmap_cpy[18].d1;
        DMaps[index].largemap_2_cset = editdmap_cpy[18].fg;
        DMaps[index].map = (editdmap_cpy[20].d1>(map_count-1))?0: editdmap_cpy[20].d1;
        DMaps[index].xoff = xmapspecs[1];
        DMaps[index].type= editdmap_cpy[23].d1|((editdmap_cpy[63].flags & D_SELECTED)?dmfCONTINUE:0);
        
        if((DMaps[index].type & dmfTYPE) == dmOVERW)
            DMaps[index].xoff = 0;
            
        DMaps[index].level=vbound(atoi(levelstr),0,MAXLEVELS-1);
        
        for(int i=0; i<8; i++)
        {
            for(int j=0; j<8; j++)
            {
                set_bit((byte *)(DMaps[index].grid+i),7-j,get_bit(gridstring,8*i+j));
            }
        }
        
        DMaps[index].compass = xtoi(compassstr);
        DMaps[index].cont = vbound(xtoi(contstr), -DMaps[index].xoff, 0x7F-DMaps[index].xoff);
        DMaps[index].color = editdmap_cpy[65].d1;
        DMaps[index].active_subscreen= editdmap_cpy[75].d1;
        DMaps[index].passive_subscreen= editdmap_cpy[77].d1;
        DMaps[index].midi = editdmap_cpy[83].d1;
        sprintf(DMaps[index].tmusic, "%s", tmusicstr);
        sprintf(DMaps[index].title,"%s",dmap_title);
        sprintf(DMaps[index].intro,"%s",dmap_intro);
        DMaps[index].tmusictrack = editdmap_cpy[89].d1;
        
        int f=0;
        f |= editdmap_cpy[110].flags & D_SELECTED ? dmfCAVES:0;
        f |= editdmap_cpy[111].flags & D_SELECTED ? dmf3STAIR:0;
        f |= editdmap_cpy[112].flags & D_SELECTED ? dmfWHIRLWIND:0;
        f |= editdmap_cpy[113].flags & D_SELECTED ? dmfGUYCAVES:0;
        f |= editdmap_cpy[114].flags & D_SELECTED ? dmfNOCOMPASS:0;
        f |= editdmap_cpy[115].flags & D_SELECTED ? dmfWAVY:0;
        f |= editdmap_cpy[116].flags & D_SELECTED ? dmfWHIRLWINDRET:0;
        f |= editdmap_cpy[117].flags & D_SELECTED ? dmfALWAYSMSG:0;
        f |= editdmap_cpy[118].flags & D_SELECTED ? dmfVIEWMAP:0;
        f |= editdmap_cpy[119].flags & D_SELECTED ? dmfDMAPMAP:0;
        f |= editdmap_cpy[120].flags & D_SELECTED ? dmfMINIMAPCOLORFIX:0;
        
        f |= editdmap_cpy[121].flags & D_SELECTED ? dmfSCRIPT1:0;
        f |= editdmap_cpy[122].flags & D_SELECTED ? dmfSCRIPT2:0;
        f |= editdmap_cpy[123].flags & D_SELECTED ? dmfSCRIPT3:0;
        f |= editdmap_cpy[124].flags & D_SELECTED ? dmfSCRIPT4:0;
        f |= editdmap_cpy[125].flags & D_SELECTED ? dmfSCRIPT5:0;
        DMaps[index].flags = f;
    }
	delete[] editdmap_cpy;
}

//int selectdmapxy[6] = {90,142,164,150,164,160};
int selectdmapxy[6] = {44,92,128,100,128,110};

static ListData dmap_list(dmaplist, &font);

static DIALOG selectdmap_dlg[] =
{
    /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp) */
    { jwin_win_proc,   72-44,   56-30,   176+88+1,  120+74+1,  vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "Select DMap", NULL, NULL },
    { d_timer_proc,        0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { d_dmaplist_proc,    46,   50,   64+72+88+1,   60+24+1+2,   jwin_pal[jcTEXTFG], jwin_pal[jcTEXTBG],  0,       D_EXIT,     0,             0, (void *) &dmap_list, NULL, selectdmapxy },
    { jwin_button_proc,   90,   152+44,  61,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "Edit", NULL, NULL },
    { jwin_button_proc,  170,  152+44,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Done", NULL, NULL },
    { d_keyboard_proc,     0,    0,    0,    0,    0,       0,      0,       0,          0,             KEY_DEL, (void *) close_dlg, NULL, NULL },
    { d_keyboard_proc,     0,    0,    0,    0,    0,       0,      0,       0,          0,             KEY_C, (void*)close_dlg, NULL, NULL },
    { d_keyboard_proc,     0,    0,    0,    0,    0,       0,      0,       0,          0,             KEY_V, (void*)close_dlg, NULL, NULL },
    { NULL,                0,    0,    0,    0,    0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

int onDmaps()
{
    int ret;
    char buf[40];
    dmap_list_size=MAXDMAPS;
    number_list_zero=true;
    selectdmap_dlg[0].dp2=lfont;

	DIALOG *selectdmap_cpy = resizeDialog(selectdmap_dlg, 1.5);
        
    ret=zc_popup_dialog(selectdmap_cpy,2);
    dmap* pSelectedDmap = 0;
    
    while(ret!=4&&ret!=0)
    {
        int d= selectdmap_cpy[2].d1;
        
        if(ret==6) //copy
		{
			pSelectedDmap = &DMaps[d];
		}
		else if(ret==7 && pSelectedDmap != 0 ) //paste
		{
			if( pSelectedDmap != &DMaps[d] )
			{
				::memcpy(&DMaps[d], pSelectedDmap, sizeof(dmap));
				saved=false;
			}
		}
        else if(ret==5)
        {
            sprintf(buf,"Delete DMap %d?",d);
            
            if(jwin_alert("Confirm Delete",buf,NULL,NULL,"&Yes","&No",'y','n',lfont)==1)
            {
                reset_dmap(d);
                saved=false;
            }
        }
        else
        {
            editdmap(d);
        }
        
        ret=zc_popup_dialog(selectdmap_cpy,2);
    }

	delete[] selectdmap_cpy;
    
    return D_O_K;
}

/*******************************/
/**********  onMidis  **********/
/*******************************/

static DIALOG editmidi_dlg[] =
{
    /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp) */
    { jwin_win_proc,     24,   20,   273,  189,  vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "MIDI Specs", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    // 2
    { jwin_text_proc,       56,   94-16,   48,   8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "MIDI:", NULL, NULL },
    { jwin_text_proc,       104,  94-16,   48,   8,    vc(11),  vc(1),  0,       0,          0,             0,       NULL, NULL, NULL },
    { jwin_text_proc,       56,   114,  48,   8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Name:", NULL, NULL },
    { jwin_edit_proc,       104,  114-4,  160,  16,   vc(12),  vc(1),  0,       0,          35,            0,       NULL, NULL, NULL },
    { jwin_text_proc,       56,   124-4+12,  56,   8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Volume:", NULL, NULL },
    { jwin_edit_proc,       120,  124-4+12-4,  32,   16,   vc(12),  vc(1),  0,       0,          3,             0,       NULL, NULL, NULL },
    // 8
    { jwin_check_proc,      176,  124+12-4,  80+1,   8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Loop", NULL, NULL },
    // 9
    { jwin_button_proc,     50,   72-24,   57,   21,   vc(14),  vc(1),  'l',     D_EXIT,     0,             0, (void *) "&Load", NULL, NULL },
    { jwin_button_proc,     116,  72-24,   33,   21,   vc(14),  vc(1),  0,       D_EXIT,     0,             0, (void *) "\x8D", NULL, NULL },
    { jwin_button_proc,     156,  72-24,   33,   21,   vc(14),  vc(1),  0,       D_EXIT,     0,             0, (void *) "\x8B", NULL, NULL },
    { jwin_button_proc,     196,  72-24,   33,   21,   vc(14),  vc(1),  0,       D_EXIT,     0,             0, (void *) "\x8B\x8B", NULL, NULL },
    { jwin_button_proc,     236,  72-24,   33,   21,   vc(14),  vc(1),  0,       D_EXIT,     0,             0, (void *) "\x8B\x8B\x8B", NULL, NULL },
    // 14
    { jwin_text_proc,       56,   134+4+12,  48,   8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Start:", NULL, NULL },
    { jwin_edit_proc,       112,  134+12,  32,   16,   vc(12),  vc(1),  0,       0,          5,             0,       NULL, NULL, NULL },
    { jwin_text_proc,       176,  134+12+4,  56,   8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Loop Start:", NULL, NULL },
    { jwin_edit_proc,       240,  134+12,  40,   16,   vc(12),  vc(1),  0,       0,          5,             0,       NULL, NULL, NULL },
    { jwin_text_proc,       176,  144+12+12,  48,   8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Loop End:", NULL, NULL },
    { jwin_edit_proc,       240,  144+12+12-4,  40,   16,   vc(12),  vc(1),  0,       0,          5,             0,       NULL, NULL, NULL },
    // 20
    { jwin_text_proc,       176,  94-16,   48,   8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Position:", NULL, NULL },
    { jwin_text_proc,       217,  94-16,   32,   8,    vc(11),  vc(1),  0,       0,          0,             0,       NULL, NULL, NULL },
    { jwin_text_proc,       176,  104-8,  48,   8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Length:", NULL, NULL },
    { jwin_text_proc,       216,  104-8,  32,   8,    vc(11),  vc(1),  0,       0,          0,             0,       NULL, NULL, NULL },
    { jwin_text_proc,       56,   104-8,  48,   8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Time:", NULL, NULL },
    { jwin_text_proc,       104,  104-8,  32,   8,    vc(11),  vc(1),  0,       0,          0,             0,       NULL, NULL, NULL },
    // 26
    { jwin_check_proc,      56,   144+12+12,  80+1,   8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Disable Saving", NULL, NULL },
    { jwin_button_proc,     90,   160+12+12,  61,   21,   vc(14),  vc(1),  'k',     D_EXIT,     0,             0, (void *) "O&K", NULL, NULL },
    { jwin_button_proc,     170,  160+12+12,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_F1,   0, (void *) onHelp, NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};


void edit_tune(int i)
{
    // TO DO : adapt for non-midi formats
    int ret,loop,volume;
    byte flags;
    long start,loop_start,loop_end;
    
    char title[36];
    char volume_str[8];
    char start_str[16];
    char loop_start_str[16];
    char loop_end_str[16];
    char len_str[16];
    char pos_str[16];
//  char format_str[8];
//  int format;

    void *data = customtunes[i].data;
    
    if(customtunes[i].format == MFORMAT_MIDI) get_midi_info((MIDI*) data,&Midi_Info);
    
    volume = customtunes[i].volume;
    loop = customtunes[i].loop;
    flags = customtunes[i].flags;
    start = customtunes[i].start;
    loop_start = customtunes[i].loop_start;
    loop_end = customtunes[i].loop_end;
    
    strcpy(title,customtunes[i].title);
    
    editmidi_dlg[0].dp2=lfont;

	DIALOG *editmidi_cpy = resizeDialog(editmidi_dlg, 1.5);
    
    if(is_large())
    {
		editmidi_cpy[13].dp2 = font;
		editmidi_cpy[12].dp2 = font;
		editmidi_cpy[11].dp2 = font;
		editmidi_cpy[10].dp2 = font;
    }
    
    do
    {
        sprintf(volume_str,"%d",volume);
        sprintf(start_str,"%ld",start);
        sprintf(loop_start_str,"%ld",loop_start);
        sprintf(loop_end_str,"%ld",loop_end);
        sprintf(len_str,"%d",Midi_Info.len_beats);
        sprintf(pos_str,"%ld",midi_pos);
        
		editmidi_cpy[3].dp = data?(void *) "Loaded":(void *) "Empty";
		editmidi_cpy[5].dp = title;
		editmidi_cpy[7].dp = volume_str;
		editmidi_cpy[8].flags = loop?D_SELECTED:0;
		editmidi_cpy[10].flags =
			editmidi_cpy[11].flags =
			editmidi_cpy[12].flags =
			editmidi_cpy[13].flags = (data==NULL)?D_DISABLED:D_EXIT;
		editmidi_cpy[15].dp = start_str;
		editmidi_cpy[17].dp = loop_start_str;
		editmidi_cpy[19].dp = loop_end_str;
		editmidi_cpy[21].dp = pos_str;
		editmidi_cpy[23].dp = len_str;
		editmidi_cpy[25].dp = timestr(Midi_Info.len_sec);
		editmidi_cpy[26].flags = (flags&tfDISABLESAVE)?D_SELECTED:0;
        
        DIALOG_PLAYER *p = init_dialog(editmidi_cpy,-1);
        
        while(update_dialog(p))
        {
            //      text_mode(vc(1));
            textprintf_ex(screen,is_large()? lfont_l : font, editmidi_cpy[0].x+int(193*(is_large()?1.5:1)), editmidi_cpy[0].y+int(58*(is_large()?1.5:1)),jwin_pal[jcBOXFG],jwin_pal[jcBOX],"%-5ld",midi_pos);
			Backend::graphics->waitTick();
			Backend::graphics->showBackBuffer();
        }
        
        ret = shutdown_dialog(p);
        
        loop = editmidi_cpy[8].flags?1:0;
        volume = vbound(atoi(volume_str),0,255); // Allegro can't play louder than 255.
        
        start = vbound(atol(start_str),0,zc_max(Midi_Info.len_beats-4,0));
        loop_start = vbound(atol(loop_start_str),-1,zc_max(Midi_Info.len_beats-4,-1));
        loop_end = vbound(atol(loop_end_str),-1,Midi_Info.len_beats);
        
        if(loop_end>0)
        {
            loop_end = vbound(loop_end,zc_max(loop_start+4,start+4),Midi_Info.len_beats);
        }
        
        flags = 0;
        flags |= editmidi_cpy[26].flags&D_SELECTED?tfDISABLESAVE:0;
        
        switch(ret)
        {
        case 9:
            if(getname("Load tune","mid;nsf",NULL,temppath,true))
            {
                stop_midi();
                
                if(data!=NULL && data!=customtunes[i].data)
                {
                    destroy_midi((MIDI*)data);
                }
                
                if((data=load_midi(temppath))==NULL)
                {
                    jwin_alert("Error","Error loading tune:",temppath,NULL,"Dang",NULL,13,27,lfont);
                }
                else
                {
                    char *t = get_filename(temppath);
                    int j;
                    
                    for(j=0; j<35 && t[j]!=0 && t[j]!='.'; j++)
                    {
                        title[j]=t[j];
                    }
                    
                    title[j]=0;
                }
                
                get_midi_info((MIDI*)data,&Midi_Info);
            }
            
            break;
            
        case 10:
            stop_midi();
            break;
            
        case 12:
            if(midi_pos>0)
            {
                int pos=midi_pos;
                stop_midi();
                midi_loop_end = -1;
                midi_loop_start = -1;
                play_midi((MIDI*)data,loop);
                set_volume(-1,volume);
                midi_loop_end = loop_end;
                midi_loop_start = loop_start;
                
                if(midi_loop_end<=0)
                {
                    pos = zc_min(pos+16,Midi_Info.len_beats);
                }
                else
                {
                    pos = zc_min(pos+16,midi_loop_end);
                }
                
                if(pos>0)
                {
                    midi_seek(pos);
                }
                
                break;
            }
            
            // else play it...
            
        case 13:
            if(midi_pos>0)
            {
                int pos=midi_pos;
                stop_midi();
                midi_loop_end = -1;
                midi_loop_start = -1;
                play_midi((MIDI*)data,loop);
                set_volume(-1,volume);
                midi_loop_end = loop_end;
                midi_loop_start = loop_start;
                
                if(midi_loop_end<0)
                {
                    pos = zc_min(pos+64,Midi_Info.len_beats);
                }
                
                else
                {
                    pos = zc_min(pos+64,midi_loop_end);
                }
                
                if(pos>0)
                {
                    midi_seek(pos);
                }
                
                break;
            }
            
            // else play it...
            
        case 11:
        {
            int pos=midi_pos;
            stop_midi();
            midi_loop_end = -1;
            midi_loop_start = -1;
            play_midi((MIDI*)data,loop);
            set_volume(-1,volume);
            midi_seek(pos<0?start:pos);
            midi_loop_end = loop_end;
            midi_loop_start = loop_start;
        }
        break;
        }
    }
    while(ret<26&&ret!=0);
    
    stop_midi();
    
    if(ret==27)
    {
        strcpy(customtunes[i].title,title);
        customtunes[i].volume = volume;
        customtunes[i].loop = loop;
        customtunes[i].start = start;
        customtunes[i].loop_start = loop_start;
        customtunes[i].loop_end = loop_end;
        customtunes[i].format = MFORMAT_MIDI;
        customtunes[i].flags = flags;
        
        if(data!=customtunes[i].data)
        {
            if(customtunes[i].data)
                destroy_midi((MIDI*)customtunes[i].data);
                
            customtunes[i].data = data;
        }
        
        saved=false;
    }
    
    if((ret==28||ret==0) && data!=customtunes[i].data)
    {
        if(data)
        {
            destroy_midi((MIDI*)data);
        }
    }

	delete[] editmidi_cpy;
}

int d_midilist_proc(int msg,DIALOG *d,int c)
{
    if(msg==MSG_DRAW)
    {
        int i = d->d1;
        int x = d->x+d->w+8;
        int y = d->y+4;
        
        textout_right_ex(screen,font,"Volume:",x+51,y+8+5,jwin_pal[jcBOXFG],jwin_pal[jcBOX]);
        textout_right_ex(screen,font,"Loop:",x+51,y+16+5,jwin_pal[jcBOXFG],jwin_pal[jcBOX]);
        textout_right_ex(screen,font,"Start:",x+51,y+24+5,jwin_pal[jcBOXFG],jwin_pal[jcBOX]);
        textout_right_ex(screen,font,"Loop Start:",x+51,y+32+5,jwin_pal[jcBOXFG],jwin_pal[jcBOX]);
        textout_right_ex(screen,font,"Loop End:",x+51,y+40+5,jwin_pal[jcBOXFG],jwin_pal[jcBOX]);
        
        textprintf_ex(screen,font,x+56,y+8+5,jwin_pal[jcBOXFG],jwin_pal[jcBOX],"%-3d",customtunes[i].volume);
        textprintf_ex(screen,font,x+56,y+16+5,jwin_pal[jcBOXFG],jwin_pal[jcBOX],"%s",customtunes[i].loop?"On ":"Off");
        textprintf_ex(screen,font,x+56,y+24+5,jwin_pal[jcBOXFG],jwin_pal[jcBOX],"%-5ld",customtunes[i].start);
        textprintf_ex(screen,font,x+56,y+32+5,jwin_pal[jcBOXFG],jwin_pal[jcBOX],"%-5ld",customtunes[i].loop_start);
        textprintf_ex(screen,font,x+56,y+40+5,jwin_pal[jcBOXFG],jwin_pal[jcBOX],"%-5ld",customtunes[i].loop_end);
    }
    
    return jwin_list_proc(msg,d,c);
}

//static ListData custommidi_list(custommidilist, is_large ? &sfont3 : &font);
static ListData custommidi_list(custommidilist, &font);

static DIALOG selectmidi_dlg[] =
{
    /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp) */
    { jwin_win_proc,     24,   20,   273,  189,  vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "Select music", NULL, NULL },
    { d_dummy_proc,      160,  56,     0,  8,    vc(15),  vc(1),  0,       0,          0,             0,       NULL, NULL, NULL },
    { d_midilist_proc,   31,   44,   164, (1+16)*8,   jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,       D_EXIT,     0,             0, (void *) &custommidi_list, NULL, NULL },
    { jwin_button_proc,     90,   160+12+12,  61,   21,   vc(14),  vc(1),  13,     D_EXIT,     0,             0, (void *) "Edit", NULL, NULL },
    { jwin_button_proc,     170,  160+12+12,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Done", NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,       0,      0,       0,          0,             KEY_DEL, (void *) close_dlg, NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

int onMidis()
{
    stopMusic();
    int ret;
    char buf[40];
    number_list_size=MAXCUSTOMTUNES;
    number_list_zero=false;
    strcpy(temppath,midipath);
    selectmidi_dlg[0].dp2=lfont;
	custommidi_list.font = is_large() ? &lfont_l : &font;

	DIALOG *selectmidi_cpy = resizeDialog(selectmidi_dlg, 1.5);
    
	selectmidi_cpy[2].dp2 = 0;
    
    go();
    ret=zc_do_dialog(selectmidi_cpy,2);
    
    while(ret!=4&&ret!=0)
    {
        int d= selectmidi_cpy[2].d1;
        
        if(ret==5)
        {
            sprintf(buf,"Delete music %d?",d+1);
            
            if(jwin_alert("Confirm Delete",buf,NULL,NULL,"&Yes","&No",'y','n',lfont)==1)
            {
                customtunes[d].reset(); // reset_midi(customMIDIs+d);
                saved=false;
            }
        }
        else
        {
            edit_tune(d);
        }
        
        ret=zc_do_dialog(selectmidi_cpy,2);
    }

	delete[] selectmidi_cpy;
    
    comeback();
    return D_O_K;
}

/*******************************/
/******  onEnhancedMusic  ******/
/*******************************/

static DIALOG editmusic_dlg[] =
{
    /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp) */
    { jwin_win_proc,     24,   20,   273,  189,  vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "Music Specs", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    // 2
    { jwin_text_proc,       56,   94-16,   48,   8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Music:", NULL, NULL },
    { jwin_text_proc,       104,  94-16,   48,   8,    vc(11),  vc(1),  0,       0,          0,             0,       NULL, NULL, NULL },
    { jwin_text_proc,       56,   114,  48,   8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Name:", NULL, NULL },
    { jwin_edit_proc,       104,  114-4,  160,  16,   vc(12),  vc(1),  0,       0,          19,            0,       NULL, NULL, NULL },
    { jwin_text_proc,       56,   124-4+12,  56,   8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Volume:", NULL, NULL },
    { jwin_edit_proc,       120,  124-4+12-4,  32,   16,   vc(12),  vc(1),  0,       0,          3,             0,       NULL, NULL, NULL },
    // 8
    { jwin_check_proc,      176,  124+12-4,  80+1,   8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Loop", NULL, NULL },
    // 9
    { jwin_button_proc,     50,   72-24,   57,   21,   vc(14),  vc(1),  'l',     D_EXIT,     0,             0, (void *) "&Load", NULL, NULL },
    { jwin_button_proc,     116,  72-24,   33,   21,   vc(14),  vc(1),  0,       D_EXIT,     0,             0, (void *) "\x8D", NULL, NULL },
    { jwin_button_proc,     156,  72-24,   33,   21,   vc(14),  vc(1),  0,       D_EXIT,     0,             0, (void *) "\x8B", NULL, NULL },
    { jwin_button_proc,     196,  72-24,   33,   21,   vc(14),  vc(1),  0,       D_EXIT,     0,             0, (void *) "\x8B\x8B", NULL, NULL },
    { jwin_button_proc,     236,  72-24,   33,   21,   vc(14),  vc(1),  0,       D_EXIT,     0,             0, (void *) "\x8B\x8B\x8B", NULL, NULL },
    // 14
    { jwin_text_proc,       56,   134+4+12,  48,   8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Start:", NULL, NULL },
    { jwin_edit_proc,       112,  134+12,  32,   16,   vc(12),  vc(1),  0,       0,          5,             0,       NULL, NULL, NULL },
    { jwin_text_proc,       176,  134+12+4,  56,   8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Loop Start:", NULL, NULL },
    { jwin_edit_proc,       240,  134+12,  40,   16,   vc(12),  vc(1),  0,       0,          5,             0,       NULL, NULL, NULL },
    { jwin_text_proc,       176,  144+12+12,  48,   8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Loop End:", NULL, NULL },
    { jwin_edit_proc,       240,  144+12+12-4,  40,   16,   vc(12),  vc(1),  0,       0,          5,             0,       NULL, NULL, NULL },
    // 20
    { jwin_text_proc,       176,  94-16,   48,   8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Position:", NULL, NULL },
    { jwin_text_proc,       217,  94-16,   32,   8,    vc(11),  vc(1),  0,       0,          0,             0,       NULL, NULL, NULL },
    { jwin_text_proc,       176,  104-8,  48,   8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Length:", NULL, NULL },
    { jwin_text_proc,       216,  104-8,  32,   8,    vc(11),  vc(1),  0,       0,          0,             0,       NULL, NULL, NULL },
    { jwin_text_proc,       56,   104-8,  48,   8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Time:", NULL, NULL },
    { jwin_text_proc,       104,  104-8,  32,   8,    vc(11),  vc(1),  0,       0,          0,             0,       NULL, NULL, NULL },
    // 26
    { jwin_button_proc,     90,   160+12+12,  61,   21,   vc(14),  vc(1),  'k',     D_EXIT,     0,             0, (void *) "O&K", NULL, NULL },
    { jwin_button_proc,     170,  160+12+12,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_F1,   0, (void *) onHelp, NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

int d_musiclist_proc(int msg,DIALOG *d,int c)
{
    return jwin_list_proc(msg,d,c);
}

static ListData enhancedmusic_list(enhancedmusiclist, &font);

static DIALOG selectmusic_dlg[] =
{
    /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp) */
    { jwin_win_proc,     24,   20,   273,  189,  vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "Select Enhanced Music", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { d_musiclist_proc,   31,   44,   164, (1+16)*8,   jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,       D_EXIT,     0,             0, (void *) &enhancedmusic_list, NULL, NULL },
    { jwin_button_proc,     90,   160+12+12,  61,   21,   vc(14),  vc(1),  13,     D_EXIT,     0,             0, (void *) "Edit", NULL, NULL },
    { jwin_button_proc,     170,  160+12+12,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Done", NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,       0,      0,       0,          0,             KEY_DEL, (void *) close_dlg, NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

int onEnhancedMusic()
{
    // to be taken out - the custom music can all be found in one place
    /*stopMusic();
    int ret;
    char buf[40];
    number_list_size=MAXCUSTOMMIDIS;
    number_list_zero=false;
    strcpy(temppath,midipath);
    selectmusic_dlg[0].dp2=lfont;
    go();
    ret=zc_do_dialog(selectmusic_dlg,2);
    while(ret!=4&&ret!=0)
    {
      int d=selectmusic_dlg[2].d1;
      if(ret==5)
      {
        sprintf(buf,"Delete MIDI %d?",d+1);
        if(jwin_alert("Confirm Delete",buf,NULL,NULL,"&Yes","&No",'y','n',lfont)==1)
        {
          reset_midi(customMIDIs+d);
          saved=false;
        }
      }
      else
      {
        edit_midi(d);
      }
      ret=zc_do_dialog(selectmusic_dlg,2);
    }
    comeback();
    */
    return D_O_K;
}

/*******************************/
/**********  onWarp  ***********/
/*******************************/

const char *warptypelist(int index, int *list_size)
{
    if(index>=0)
    {
        if(index>=MAXWARPTYPES)
            index=MAXWARPTYPES-1;
            
        return warptype_string[index];
    }
    
    *list_size=MAXWARPTYPES;
    //  *list_size=6;
    return NULL;
}

const char *warpeffectlist(int index, int *list_size)
{
    if(index>=0)
    {
        if(index>=MAXWARPEFFECTS)
            index=MAXWARPEFFECTS-1;
            
        return warpeffect_string[index];
    }
    
    *list_size=MAXWARPEFFECTS;
    return NULL;
}

//int warpdmapxy[6] = {188,131,188,111,188,120};
//int warpdmapxy[6] = {188-68,131-93,188-68,111-93,188-68,120-93};
int warpdmapxy[6] = {150,38,150,18,150,27};
//int warpdmapxy[6] = {2,25,0,17,36,17};

static int warp1_list[] =
{
    2,3,4,5,6,7,8,9,10,11,12,13,53,54,63,67,-1
};

static int warp2_list[] =
{
    17,18,19,20,21,22,23,24,25,26,27,28,55,56,64,68,-1
};

static int warp3_list[] =
{
    29,30,31,32,33,34,35,36,37,38,39,40,57,58,65,69,-1
};

static int warp4_list[] =
{
    41,42,43,44,45,46,47,48,49,50,51,52,59,60,66,70,-1
};

static TABPANEL warp_tabs[] =
{
    // (text)
    { (char *)"A",     D_SELECTED, warp1_list, 0, NULL },
    { (char *)"B",     0,          warp2_list, 0, NULL },
    { (char *)"C",     0,          warp3_list, 0, NULL },
    { (char *)"D",     0,          warp4_list, 0, NULL },
    { NULL,            0,          NULL,       0, NULL }
};

int onTileWarpIndex(int index)
{
    int i=-1;
    
    while(warp_tabs[++i].text != NULL)
        warp_tabs[i].flags = (i==index ? D_SELECTED : 0);
        
    onTileWarp();
    return D_O_K;
}

static char warpr_buf[10];
const char *warprlist(int index, int *list_size)
{
    if(index>=0)
    {
        bound(index,0,3);
        sprintf(warpr_buf,"%c",index+0x41);
        return warpr_buf;
    }
    
    *list_size=4;
    return NULL;
}

int d_wflag_proc(int msg,DIALOG *d,int c);

static ListData warp_dlg_list(warptypelist, &font);
static ListData warp_ret_list(warprlist, &font);

int d_warpdestscrsel_proc(int msg,DIALOG *d,int)
{
    DIALOG *td=(DIALOG *)d->dp3;
    
    if(msg==MSG_CLICK)
    {
        bool is_overworld=((DMaps[td[d->d1].d1].type&dmfTYPE)==dmOVERW);
        int x_clip  = is_overworld?0x0F:0x07;
        int x_scale = is_overworld?2:3;
        
        while(Backend::mouse->anyButtonClicked())
        {
            int x = zc_min(zc_max(Backend::mouse->getVirtualScreenX() - d->x,0)>>x_scale, x_clip);
            int y = zc_min((zc_max(Backend::mouse->getVirtualScreenY() - d->y,0)<<2)&0xF0, 0x70);
//      if(x+y != d->d1)
            {
                sprintf((char *)td[d->d1+1].dp, "%02X", y+x);
                object_message(&td[d->d1+1], MSG_DRAW, 0);
				Backend::graphics->waitTick();
				Backend::graphics->showBackBuffer();
            }
        }
    }
    
    return D_O_K;
}

static DIALOG warp_dlg[] =
{
    /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp) */
    { jwin_win_proc,             0,      0,    302,    188,    vc(14),                 vc(1),                   0,       D_EXIT,     0,             0,  NULL,                          NULL,   NULL              },
    { jwin_tab_proc,             6,     24,    290,    135,    vc(14),                 vc(1),                   0,       0,          1,             0, (void *) warp_tabs,            NULL, (void *)warp_dlg  },
    { jwin_text_proc,           61,     55,     40,      8,    vc(14),                 vc(1),                   0,       0,          0,             0, (void *) "Type:",              NULL,   NULL              },
    { jwin_text_proc,           29,     73,     40,      8,    vc(14),                 vc(1),                   0,       0,          0,             0, (void *) "DMap:",              NULL,   NULL              },
    { jwin_text_proc,           28,     91,     64,      8,    vc(14),                 vc(1),                   0,       0,          0,             0, (void *) "Screen:",         NULL,   NULL              },
    { jwin_text_proc,          146,     91,     64,      8,    vc(14),                 vc(1),                   0,       0,          0,             0, (void *) "Triggers:",          NULL,   NULL              },
    { jwin_frame_proc,         164,    109,     30,     30,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,       0,          FR_ETCHED,     0,  NULL,                          NULL,   NULL              },
    // 7
    { jwin_droplist_proc,       91,     51,    193,     16,    jwin_pal[jcTEXTFG],     jwin_pal[jcTEXTBG],      0,       0,          0,             0, (void *) &warp_dlg_list,       NULL,   NULL              },
    { d_dropdmaplist_proc,      59,     69,    225,     16,    jwin_pal[jcTEXTFG],     jwin_pal[jcTEXTBG],      0,       0,          0,             0, (void *) &dmap_list,           NULL,   warpdmapxy        },
    { jwin_hexedit_proc,        77,     87,     24,     16,    vc(12),                 vc(1),                   0,       0,          2,             0,  NULL,                          NULL,   NULL              },
    // 10
    { d_wflag_proc,            170,    106,     18,      8,    vc(4),                  vc(0),                   0,       0,          1,             0,  NULL,                          NULL,   NULL              },
    { d_wflag_proc,            170,    134,     18,      8,    vc(4),                  vc(0),                   0,       0,          1,             0,  NULL,                          NULL,   NULL              },
    { d_wflag_proc,            161,    115,      8,     18,    vc(4),                  vc(0),                   0,       0,          1,             0,  NULL,                          NULL,   NULL              },
    { d_wflag_proc,            189,    115,      8,     18,    vc(4),                  vc(0),                   0,       0,          1,             0,  NULL,                          NULL,   NULL              },
    // 14
    { jwin_button_proc,         61,    163,     41,     21,    vc(14),                 vc(1),                   'k',     D_EXIT,     0,             0, (void *) "O&K",                NULL,   NULL              },
    { jwin_button_proc,        121,    163,     41,     21,    vc(14),                 vc(1),                   'g',     D_EXIT,     0,             0, (void *) "&Go",                NULL,   NULL              },
    { jwin_button_proc,        181,    163,     61,     21,    vc(14),                 vc(1),                  27,       D_EXIT,     0,             0, (void *) "Cancel",             NULL,   NULL              },
    // 17
    { jwin_text_proc,           61,     55,     40,      8,    vc(14),                 vc(1),                   0,       0,          0,             0, (void *) "Type:",              NULL,   NULL              },
    { jwin_text_proc,           29,     73,     40,      8,    vc(14),                 vc(1),                   0,       0,          0,             0, (void *) "DMap:",              NULL,   NULL              },
    { jwin_text_proc,           28,     91,     64,      8,    vc(14),                 vc(1),                   0,       0,          0,             0, (void *) "Screen:",         NULL,   NULL              },
    { jwin_text_proc,          146,     91,     64,      8,    vc(14),                 vc(1),                   0,       0,          0,             0, (void *) "Triggers:",          NULL,   NULL              },
    { jwin_frame_proc,         164,    109,     30,     30,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,       0,          FR_ETCHED,     0,  NULL,                          NULL,   NULL              },
    // 22
    { jwin_droplist_proc,       91,     51,    193,     16,    jwin_pal[jcTEXTFG],     jwin_pal[jcTEXTBG],      0,       0,          0,             0, (void *) &warp_dlg_list,       NULL,   NULL              },
    { d_dropdmaplist_proc,      59,     69,    225,     16,    jwin_pal[jcTEXTFG],     jwin_pal[jcTEXTBG],      0,       0,          0,             0, (void *) &dmap_list,           NULL,   warpdmapxy        },
    { jwin_hexedit_proc,        77,     87,     24,     16,    vc(12),                 vc(1),                   0,       0,          2,             0,  NULL,                          NULL,   NULL              },
    // 25
    { d_wflag_proc,            170,    106,     18,      8,    vc(4),                  vc(0),                   0,       0,          1,             0,  NULL,                          NULL,   NULL              },
    { d_wflag_proc,            170,    134,     18,      8,    vc(4),                  vc(0),                   0,       0,          1,             0,  NULL,                          NULL,   NULL              },
    { d_wflag_proc,            161,    115,      8,     18,    vc(4),                  vc(0),                   0,       0,          1,             0,  NULL,                          NULL,   NULL              },
    { d_wflag_proc,            189,    115,      8,     18,    vc(4),                  vc(0),                   0,       0,          1,             0,  NULL,                          NULL,   NULL              },
    // 29
    { jwin_text_proc,           61,     55,     40,      8,    vc(14),                 vc(1),                   0,       0,          0,             0, (void *) "Type:",              NULL,   NULL              },
    { jwin_text_proc,           29,     73,     40,      8,    vc(14),                 vc(1),                   0,       0,          0,             0, (void *) "DMap:",              NULL,   NULL              },
    { jwin_text_proc,           28,     91,     64,      8,    vc(14),                 vc(1),                   0,       0,          0,             0, (void *) "Screen:",         NULL,   NULL              },
    { jwin_text_proc,          146,     91,     64,      8,    vc(14),                 vc(1),                   0,       0,          0,             0, (void *) "Triggers:",          NULL,   NULL              },
    { jwin_frame_proc,         164,    109,     30,     30,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,       0,          FR_ETCHED,     0,  NULL,                          NULL,   NULL              },
    // 34
    { jwin_droplist_proc,       91,     51,    193,     16,    jwin_pal[jcTEXTFG],     jwin_pal[jcTEXTBG],      0,       0,          0,             0, (void *) &warp_dlg_list,       NULL,   NULL              },
    { d_dropdmaplist_proc,      59,     69,    225,     16,    jwin_pal[jcTEXTFG],     jwin_pal[jcTEXTBG],      0,       0,          0,             0, (void *) &dmap_list,           NULL,   warpdmapxy        },
    { jwin_hexedit_proc,        77,     87,     24,     16,    vc(12),                 vc(1),                   0,       0,          2,             0,  NULL,                          NULL,   NULL              },
    // 37
    { d_wflag_proc,            170,    106,     18,      8,    vc(4),                  vc(0),                   0,       0,          1,             0,  NULL,                          NULL,   NULL              },
    { d_wflag_proc,            170,    134,     18,      8,    vc(4),                  vc(0),                   0,       0,          1,             0,  NULL,                          NULL,   NULL              },
    { d_wflag_proc,            161,    115,      8,     18,    vc(4),                  vc(0),                   0,       0,          1,             0,  NULL,                          NULL,   NULL              },
    { d_wflag_proc,            189,    115,      8,     18,    vc(4),                  vc(0),                   0,       0,          1,             0,  NULL,                          NULL,   NULL              },
    //41
    { jwin_text_proc,           61,     55,     40,      8,    vc(14),                 vc(1),                   0,       0,          0,             0, (void *) "Type:",              NULL,   NULL              },
    { jwin_text_proc,           29,     73,     40,      8,    vc(14),                 vc(1),                   0,       0,          0,             0, (void *) "DMap:",              NULL,   NULL              },
    { jwin_text_proc,           28,     91,     64,      8,    vc(14),                 vc(1),                   0,       0,          0,             0, (void *) "Screen:",         NULL,   NULL              },
    { jwin_text_proc,          146,     91,     64,      8,    vc(14),                 vc(1),                   0,       0,          0,             0, (void *) "Triggers:",          NULL,   NULL              },
    { jwin_frame_proc,         164,    109,     30,     30,    jwin_pal[jcBOXFG],      jwin_pal[jcBOX],         0,       0,          FR_ETCHED,     0,  NULL,                          NULL,   NULL              },
    // 46
    { jwin_droplist_proc,       91,     51,    193,     16,    jwin_pal[jcTEXTFG],     jwin_pal[jcTEXTBG],      0,       0,          0,             0, (void *) &warp_dlg_list,       NULL,   NULL              },
    { d_dropdmaplist_proc,      59,     69,    225,     16,    jwin_pal[jcTEXTFG],     jwin_pal[jcTEXTBG],      0,       0,          0,             0, (void *) &dmap_list,           NULL,   warpdmapxy        },
    { jwin_hexedit_proc,        77,     87,     24,     16,    vc(12),                 vc(1),                   0,       0,          2,             0,  NULL,                          NULL,   NULL              },
    // 49
    { d_wflag_proc,            170,    106,     18,      8,    vc(4),                  vc(0),                   0,       0,          1,             0,  NULL,                          NULL,   NULL              },
    { d_wflag_proc,            170,    134,     18,      8,    vc(4),                  vc(0),                   0,       0,          1,             0,  NULL,                          NULL,   NULL              },
    { d_wflag_proc,            161,    115,      8,     18,    vc(4),                  vc(0),                   0,       0,          1,             0,  NULL,                          NULL,   NULL              },
    { d_wflag_proc,            189,    115,      8,     18,    vc(4),                  vc(0),                   0,       0,          1,             0,  NULL,                          NULL,   NULL              },
    { jwin_text_proc,           29,    123,    100,      8,    vc(14),                 vc(1),                   0,       0,          0,             0, (void *) "Use Warp Return:",   NULL,   NULL              },
    { jwin_droplist_proc,       74,    133,     50,     16,    jwin_pal[jcTEXTFG],     jwin_pal[jcTEXTBG],      0,       0,          0,             0, (void *) &warp_ret_list,       NULL,   NULL              },
    { jwin_text_proc,           29,    123,    100,      8,    vc(14),                 vc(1),                   0,       0,          0,             0, (void *) "Use Warp Return:",   NULL,   NULL              },
    { jwin_droplist_proc,       74,    133,     50,     16,    jwin_pal[jcTEXTFG],     jwin_pal[jcTEXTBG],      0,       0,          0,             0, (void *) &warp_ret_list,       NULL,   NULL              },
    { jwin_text_proc,           29,    123,    100,      8,    vc(14),                 vc(1),                   0,       0,          0,             0, (void *) "Use Warp Return:",   NULL,   NULL              },
    { jwin_droplist_proc,       74,    133,     50,     16,    jwin_pal[jcTEXTFG],     jwin_pal[jcTEXTBG],      0,       0,          0,             0, (void *) &warp_ret_list,       NULL,   NULL              },
    { jwin_text_proc,           29,    123,    100,      8,    vc(14),                 vc(1),                   0,       0,          0,             0, (void *) "Use Warp Return:",   NULL,   NULL              },
    { jwin_droplist_proc,       74,    133,     50,     16,    jwin_pal[jcTEXTFG],     jwin_pal[jcTEXTBG],      0,       0,          0,             0, (void *) &warp_ret_list,       NULL,   NULL              },
    { d_keyboard_proc,           0,      0,      0,      0,    0,                      0,                       0,       0,          KEY_F1,        0, (void *) onHelp,               NULL,   NULL              },
    { d_timer_proc,              0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                          NULL,   NULL              },
    // 63
    { jwin_check_proc,        29,   107,    129,      9,    vc(14),                 vc(1),                   0,       0,          1,             0, (void *) "Combos Carry Over",            NULL,   NULL              },
    { jwin_check_proc,        29,   107,    129,      9,    vc(14),                 vc(1),                   0,       0,          1,             0, (void *) "Combos Carry Over",            NULL,   NULL              },
    { jwin_check_proc,        29,   107,    129,      9,    vc(14),                 vc(1),                   0,       0,          1,             0, (void *) "Combos Carry Over",            NULL,   NULL              },
    { jwin_check_proc,        29,   107,    129,      9,    vc(14),                 vc(1),                   0,       0,          1,             0, (void *) "Combos Carry Over",            NULL,   NULL              },
    // 67
    { d_warpdestscrsel_proc,   217,    114,     64,     32,    0,                      0,                       0,       0,          8,             0,  NULL,                          NULL, (void *)warp_dlg  },
    { d_warpdestscrsel_proc,   217,    114,     64,     32,    0,                      0,                       0,       0,         23,             0,  NULL,                          NULL, (void *)warp_dlg  },
    { d_warpdestscrsel_proc,   217,    114,     64,     32,    0,                      0,                       0,       0,         35,             0,  NULL,                          NULL, (void *)warp_dlg  },
    { d_warpdestscrsel_proc,   217,    114,     64,     32,    0,                      0,                       0,       0,         47,             0,  NULL,                          NULL, (void *)warp_dlg  },
    { d_dummy_proc,              0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                          NULL,   NULL              },
    { d_dummy_proc,              0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                          NULL,   NULL              },
    { d_dummy_proc,              0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                          NULL,   NULL              },
    { d_dummy_proc,              0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                          NULL,   NULL              },
    { d_dummy_proc,              0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                          NULL,   NULL              },
    { d_dummy_proc,              0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                          NULL,   NULL              },
    { d_dummy_proc,              0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                          NULL,   NULL              },
    { d_dummy_proc,              0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                          NULL,   NULL              },
    { d_dummy_proc,              0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                          NULL,   NULL              },
    { d_dummy_proc,              0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                          NULL,   NULL              },
    { d_dummy_proc,              0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                          NULL,   NULL              },
    { d_dummy_proc,              0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                          NULL,   NULL              },
    { d_dummy_proc,              0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                          NULL,   NULL              },
    { d_dummy_proc,              0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                          NULL,   NULL              },
    { d_dummy_proc,              0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                          NULL,   NULL              },
    { d_dummy_proc,              0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                          NULL,   NULL              },
    { d_dummy_proc,              0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                          NULL,   NULL              },
    { d_dummy_proc,              0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                          NULL,   NULL              },
    { d_dummy_proc,              0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                          NULL,   NULL              },
    { d_dummy_proc,              0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                          NULL,   NULL              },
    { d_dummy_proc,              0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                          NULL,   NULL              },
    { d_dummy_proc,              0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                          NULL,   NULL              },
    { d_dummy_proc,              0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                          NULL,   NULL              },
    { d_dummy_proc,              0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                          NULL,   NULL              },
    { d_dummy_proc,              0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                          NULL,   NULL              },
    { d_dummy_proc,              0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                          NULL,   NULL              },
    { d_dummy_proc,              0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                          NULL,   NULL              },
    { d_dummy_proc,              0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                          NULL,   NULL              },
    { NULL,                      0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                          NULL,   NULL              }
};

// Side warp flag procedure
int d_wflag_proc(int msg,DIALOG *d,int c)
{
    //these are here to bypass compiler warnings about unused arguments
    c=c;
    
    switch(msg)
    {
    case MSG_DRAW:
    {
        int c2=(d->flags&D_SELECTED)?d->fg:d->bg;
        
        /*if(!(d->d2&0x80))
          {
          c=d->bg;
          }*/
        if(d->d1==1)
        {
            jwin_draw_frame(screen,d->x,d->y,d->w,d->h, FR_DEEP);
            rectfill(screen,d->x+2, d->y+2, d->x+d->w-3, d->y+d->h-3,c2);
            
            if(d->flags&D_SELECTED)
            {
                int e=d->d2&3;
                
                if(d->w>d->h)
                    textprintf_centre_ex(screen,is_large() ? lfont_l : font, d->x+(d->w/2),d->y,jwin_pal[jcBOXFG],-1,"%c",e+0x41);
                else
                    textprintf_centre_ex(screen,is_large() ? lfont_l : font, d->x+(d->w/2),d->y+(d->h/2)-4,jwin_pal[jcBOXFG],-1,"%c",e+0x41);
            }
            
        }
        else
        {
            rectfill(screen,d->x, d->y, d->x+d->w-1, d->y+d->h-1,c2);
        }
    }
    break;
    
    case MSG_CLICK:
    {
        if(d->d1==1)
        {
            if(!(d->flags&D_SELECTED))
            {
                d->flags|=D_SELECTED;
                d->d2&=0x80;
                int g;
                
                if(d==&warp_dlg[10]||d==&warp_dlg[25]||d==&warp_dlg[37]||d==&warp_dlg[49]) g=0;
                else if(d==&warp_dlg[11]||d==&warp_dlg[26]||d==&warp_dlg[38]||d==&warp_dlg[50]) g=1;
                else if(d==&warp_dlg[12]||d==&warp_dlg[27]||d==&warp_dlg[39]||d==&warp_dlg[51]) g=2;
                else g=3;
                
                warp_dlg[10+g].flags = d->flags;
                warp_dlg[10+g].d2 = d->d2;
                warp_dlg[25+g].flags = d->flags;
                warp_dlg[25+g].d2 = d->d2;
                warp_dlg[37+g].flags = d->flags;
                warp_dlg[37+g].d2 = d->d2;
                warp_dlg[49+g].flags = d->flags;
                warp_dlg[49+g].d2 = d->d2;
            }
            else
            {
                if((d->d2&3)==3)
                {
                    d->flags^=D_SELECTED;
                    d->d2&=0x80;
                    int g;
                    
                    if(d==&warp_dlg[10]||d==&warp_dlg[25]||d==&warp_dlg[37]||d==&warp_dlg[49]) g=0;
                    else if(d==&warp_dlg[11]||d==&warp_dlg[26]||d==&warp_dlg[38]||d==&warp_dlg[50]) g=1;
                    else if(d==&warp_dlg[12]||d==&warp_dlg[27]||d==&warp_dlg[39]||d==&warp_dlg[51]) g=2;
                    else g=3;
                    
                    warp_dlg[10+g].flags = d->flags;
                    warp_dlg[10+g].d2 = d->d2;
                    warp_dlg[25+g].flags = d->flags;
                    warp_dlg[25+g].d2 = d->d2;
                    warp_dlg[37+g].flags = d->flags;
                    warp_dlg[37+g].d2 = d->d2;
                    warp_dlg[49+g].flags = d->flags;
                    warp_dlg[49+g].d2 = d->d2;
                }
                else
                {
                    int f=d->d2&3;
                    d->d2&=0x80;
                    f++;
                    d->d2|=f;
                    int g;
                    
                    if(d==&warp_dlg[10]||d==&warp_dlg[25]||d==&warp_dlg[37]||d==&warp_dlg[49]) g=0;
                    else if(d==&warp_dlg[11]||d==&warp_dlg[26]||d==&warp_dlg[38]||d==&warp_dlg[50]) g=1;
                    else if(d==&warp_dlg[12]||d==&warp_dlg[27]||d==&warp_dlg[39]||d==&warp_dlg[51]) g=2;
                    else g=3;
                    
                    warp_dlg[10+g].flags = d->flags;
                    warp_dlg[10+g].d2 = d->d2;
                    warp_dlg[25+g].flags = d->flags;
                    warp_dlg[25+g].d2 = d->d2;
                    warp_dlg[37+g].flags = d->flags;
                    warp_dlg[37+g].d2 = d->d2;
                    warp_dlg[49+g].flags = d->flags;
                    warp_dlg[49+g].d2 = d->d2;
                }
            }
        }
        else
        {
            d->flags^=D_SELECTED;
        }
        
        int c2=(d->flags&D_SELECTED)?d->fg:d->bg;
        
        if(d->d1==1)
        {
            jwin_draw_frame(screen,d->x,d->y,d->w,d->h, FR_DEEP);
            rectfill(screen,d->x+2, d->y+2, d->x+d->w-3, d->y+d->h-3,c2);
            
            if(d->flags&D_SELECTED)
            {
                int e=d->d2&3;
                
                if(d->w>d->h)
                    textprintf_centre_ex(screen,is_large()? lfont_l: font,d->x+(d->w/2),d->y,jwin_pal[jcBOXFG],-1,"%c",e+0x41);
                else
                    textprintf_centre_ex(screen,is_large()? lfont_l: font,d->x+(d->w/2),d->y+(d->h/2)-4,jwin_pal[jcBOXFG],-1,"%c",e+0x41);
            }
        }
        else
        {
            rectfill(screen,d->x, d->y, d->x+d->w-1, d->y+d->h-1,c2);
        }
        
        
        while(Backend::mouse->anyButtonClicked())
        {
			Backend::graphics->waitTick();
			Backend::graphics->showBackBuffer();
            /* do nothing */
        }
    }
    break;
    }
    
    return D_O_K;
}

#if 0
static int north_side_warp_list[] =
{
    // dialog control number
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, -1
};

static int south_side_warp_list[] =
{
    // dialog control number
    80, -1
};

static int west_side_warp_list[] =
{
    // dialog control number
    80, -1
};

static int east_side_warp_list[] =
{
    // dialog control number
    80, -1
};

static int wind_warp_1_list[] =
{
    // dialog control number
    80, -1
};

static int wind_warp_2_list[] =
{
    // dialog control number
    80, -1
};

static int wind_warp_3_list[] =
{
    // dialog control number
    80, -1
};

static int wind_warp_4_list[] =
{
    // dialog control number
    80, -1
};

static int wind_warp_5_list[] =
{
    // dialog control number
    80, -1
};

static int wind_warp_6_list[] =
{
    // dialog control number
    80, -1
};

static int wind_warp_7_list[] =
{
    // dialog control number
    80, -1
};

static int wind_warp_8_list[] =
{
    // dialog control number
    80, -1
};

static int wind_warp_9_list[] =
{
    // dialog control number
    80, -1
};


static int tile_warp_list[] =
{
    // dialog control number
    80, -1
};

static int side_warp_list[] =
{
    // dialog control number
    6, -1
};

static int item_warp_list[] =
{
    // dialog control number
    80, -1
};

static int wind_warp_list[] =
{
    // dialog control number
    7, -1
};

static int special_warp_list[] =
{
    // dialog control number
    80, -1
};

static int timed_warp_list[] =
{
    // dialog control number
    80, -1
};


#endif

int d_dmapscrsel_proc(int msg,DIALOG *d,int c)
{
    //these are here to bypass compiler warnings about unused arguments
    c=c;
    
    int ret = D_O_K;
    
    switch(msg)
    {
    case MSG_CLICK:
        sprintf((char*)((d+2)->dp),"%X%X",vbound((Backend::mouse->getVirtualScreenY()-d->y)/4,0,7),vbound((Backend::mouse->getVirtualScreenX()-d->x)/(((DMaps[(d-1)->d1].type&dmfTYPE)==1)?4:8),0,(((DMaps[(d-1)->d1].type&dmfTYPE)==1)?15:7)));
        object_message(d+2, MSG_DRAW, 0);
        break;
    }
    
    return ret;
}

int warpdestsel_x=-1;
int warpdestsel_y=-1;
int warpdestmap=-1;
int warpdestscr=-1;

int d_warpdestsel_proc(int msg,DIALOG *d,int c)
{
    //these are here to bypass compiler warnings about unused arguments
    c=c;
    
    int ret=D_O_K;
    static BITMAP *bmp=create_bitmap_ex(8,256,176);
    static bool inrect=false;
    static bool mousedown=false;
    
    switch(msg)
    {
    case MSG_START:
        loadlvlpal(Map.AbsoluteScr(warpdestmap,warpdestscr)->color);
        rebuild_trans_table();
        break;
        
    case MSG_DRAW:
    {
        jwin_draw_frame(screen, d->x, d->y, d->w, d->h, FR_DEEP);
        
        if(AnimationOn||CycleOn)
        {
            if(AnimationOn)
            {
                animate_combos();
            }
            
            if(CycleOn)
            {
                cycle_palette();
            }
        }
        
        animate_coords();
        Map.draw(bmp, 0, 0, 0, warpdestmap, warpdestscr);
        blit(icon_bmp[ICON_BMP_WARPDEST][coord_frame], bmp, 0, 0, Map.AbsoluteScr(warpdestmap,warpdestscr)->warparrivalx, Map.AbsoluteScr(warpdestmap,warpdestscr)->warparrivaly, 16, 16);
        int px2=((Backend::mouse->getVirtualScreenX()-d->x-2)&0xF8);
        int py2=((Backend::mouse->getVirtualScreenY()-d->y-2)&0xF8);
        
        if(isinRect(Backend::mouse->getVirtualScreenX(), Backend::mouse->getVirtualScreenY(), d->x+2,d->y+2,d->x+256+1,d->y+176+1))
        {
            if(Backend::mouse->anyButtonClicked())
            {
                if(!mousedown||!inrect)
                {
					Backend::mouse->setCursorSprite(mouse_bmp[MOUSE_BMP_BLANK][0]);
					int bx = d->x + 2;
					int by = d->y + 2;
					int tx = d->x + 256 + 1;
					int ty = d->y + 176 + 1;
					Backend::mouse->setVirtualScreenMouseBounds(bx, by, tx, ty);
                }
                
                rect(bmp, px2, py2, px2+15, py2+15, vc(15));
                warpdestsel_x=px2;
                warpdestsel_y=py2;
                mousedown=true;
            }
            else
            {
                if(mousedown||!inrect)
                {
					int bx = 0;
					int by = 0;
					int tx = Backend::graphics->virtualScreenW() - 1;
					int ty = Backend::graphics->virtualScreenH() - 1;
					Backend::mouse->setVirtualScreenMouseBounds(bx, by, tx, ty);
					Backend::mouse->setCursorSprite(mouse_bmp[MOUSE_BMP_POINT_BOX][0]);
                }
                
                mousedown=false;
            }
            
            inrect=true;
        }
        else
        {
			Backend::mouse->setCursorSprite(mouse_bmp[MOUSE_BMP_NORMAL][0]);
            inrect=false;
        }
        
        blit(bmp, screen, 0, 0, d->x+2, d->y+2, 256, 176);
    }
    break;
    
    case MSG_VSYNC:
        d->flags|=D_DIRTY;
        break;
        
    case MSG_END:
        loadlvlpal(Map.CurrScr()->color);
        rebuild_trans_table();
        break;
    }
    
    return ret;
}

int d_vsync_proc(int msg,DIALOG *d,int c)
{
    //these are here to bypass compiler warnings about unused arguments
    d=d;
    
    static clock_t tics;
    
    switch(msg)
    {
    case MSG_START:
        tics=clock()+(CLOCKS_PER_SEC/60);
        break;
        
    case MSG_IDLE:
        if(clock()>tics)
        {
            tics=clock()+(CLOCKS_PER_SEC/60);
            broadcast_dialog_message(MSG_VSYNC, c);
        }
        
        break;
    }
    
    return D_O_K;
}

#if 0
static DIALOG warpdestsel_dlg[] =
{
    /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp) */
    { jwin_win_proc,                 0,      0,    297,    234,    vc(14),                 vc(1),                   0,       D_EXIT,      0,             0, (void *) "Select Warp Destination",    NULL,    NULL       },
    { jwin_button_proc,              6,    207,     61,     21,    vc(0),                  vc(11),                 13,       D_EXIT,      0,             0, (void *) "OK",                         NULL,    NULL       },
    { jwin_button_proc,             70,    207,     93,     21,    vc(0),                  vc(11),                  0,       D_EXIT,      0,             0, (void *) "Use Warp Square",            NULL,    NULL       },
    { jwin_button_proc,            166,    207,     61,     21,    vc(0),                  vc(11),                  0,       D_EXIT,      0,             0, (void *) "Use Origin",                 NULL,    NULL       },
    { jwin_button_proc,            230,    207,     61,     21,    vc(0),                  vc(11),                 27,       D_EXIT,      0,             0, (void *) "Cancel",                     NULL,    NULL       },
    { d_keyboard_proc,               0,      0,      0,      0,    0,                      0,                       0,       0,           KEY_F1,        0, (void *) onHelp,                       NULL,    NULL       },
    { d_warpdestsel_proc,           19,     23,    260,    180,    0,                      0,                       0,       0,           0,             0,   NULL,                                  NULL,    NULL       },
    { d_vsync_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,           0,             0,   NULL,                                  NULL,    NULL       },
    { d_timer_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,           0,             0,   NULL,                                  NULL,    NULL       },
    { NULL,                          0,      0,      0,      0,    0,                      0,                       0,       0,           0,             0,   NULL,                                  NULL,    NULL       }
};

int d_warpbutton_proc(int msg,DIALOG *d,int c)
{
    int ret=jwin_button_proc(msg,d,c);
    
    if(ret==D_EXIT)
    {
        warpdestsel_dlg[0].dp2=lfont;
        warpdestmap=DMaps[(d-4)->d1].map;
        warpdestscr=DMaps[(d-4)->d1].xoff+xtoi((char*)((d-1)->dp));
        ret=zc_popup_dialog(warpdestsel_dlg,-1);
        
        switch(ret)
        {
        case 1:
            d->d1=warpdestsel_x;
            d->d2=warpdestsel_y;
            sprintf((char *)d->dp, "at: %dx%d", warpdestsel_x, warpdestsel_y);
            break;
            
        case 2:
            d->d1=-1;
            d->d2=-1;
            sprintf((char *)d->dp, "at: warp square");
            break;
            
        case 3:
            d->d1=-2;
            d->d2=-2;
            sprintf((char *)d->dp, "at: origin");
            break;
            
        default:
            break;
        }
        
        d->flags|=D_DIRTY;
    }
    
    return ret?D_O_K:D_O_K;
}
#endif

int jwin_minibutton_proc(int msg,DIALOG *d,int c)
{
    switch(msg)
    {
    case MSG_DRAW:
        jwin_draw_text_button(screen, d->x, d->y, d->w, d->h, (char*)d->dp, d->flags, false);
        return D_O_K;
        break;
    }
    
    return jwin_button_proc(msg,d,c);
}

int d_triggerbutton_proc(int msg,DIALOG *d,int c)
{
    static BITMAP *dummy=create_bitmap_ex(8, 1, 1);
    
    switch(msg)
    {
    case MSG_START:
        d->w=gui_textout_ln(dummy, font, (unsigned char *)d->dp, 0, 0, jwin_pal[jcMEDDARK], -1, 0)+4;
        d->h=text_height(font)+5;
        break;
        
    case MSG_GOTFOCUS:
        d->flags&=~D_GOTFOCUS;
        break;
        
    }
    
    return jwin_minibutton_proc(msg,d,c);
}

int d_alltriggerbutton_proc(int msg,DIALOG *d,int c)
{
    DIALOG *temp_d;
    int ret=d_triggerbutton_proc(msg,d,c);
    
    switch(msg)
    {
    case MSG_CLICK:
        temp_d=d-1;
        
        while(temp_d->proc==d_triggerbutton_proc)
        {
            temp_d->flags&=~D_SELECTED;
            temp_d->flags|=D_DIRTY;
            
            if(d->flags&D_SELECTED)
            {
                temp_d->flags|=D_SELECTED;
            }
            
            --temp_d;
        }
        
        break;
    }
    
    return ret;
}

int d_ticsedit_proc(int msg,DIALOG *d,int c)
{
    int ret = jwin_edit_proc(msg,d,c);
    
    if(msg==MSG_DRAW)
    {
        int tics=vbound(atoi((char*)d->dp),0,65535);
        sprintf((char*)(d+1)->dp,"%s %s",ticksstr(tics),tics==0?"(No Timed Warp)":"               ");
        object_message(d+1,MSG_DRAW,c);
    }
    
    return ret;
}

static ListData warp_effect_list(warpeffectlist,&font);

#if 0
static DIALOG warp_dlg2[] =
{
    /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp) */
    { jwin_win_proc,                 0,      0,    320,    240,    vc(14),                 vc(1),                   0,       D_EXIT,     0,             0, (void *) "Edit Warps",        NULL,   NULL              },
    { jwin_button_proc,             70,    215,     41,     21,    vc(14),                 vc(1),                   'k',     D_EXIT,     0,             0, (void *) "O&K",               NULL,   NULL              },
    { jwin_button_proc,            130,    215,     41,     21,    vc(14),                 vc(1),                   'g',     D_EXIT,     0,             0, (void *) "&Go",               NULL,   NULL              },
    { jwin_button_proc,            190,    215,     61,     21,    vc(14),                 vc(1),                  27,       D_EXIT,     0,             0, (void *) "Cancel",            NULL,   NULL              },
    { d_keyboard_proc,               0,      0,      0,      0,    0,                      0,                       0,       0,          KEY_F1,        0, (void *) onHelp,              NULL,   NULL              },
    //5
    { jwin_tab_proc,                 6,     25,    308,    184,    0,                      0,                       0,       0,          0,             0, (void *) warp_tabs,           NULL, (void *)warp_dlg  },
    { jwin_tab_proc,                10,     45,    300,    159,    0,                      0,                       0,       0,          0,             0, (void *) side_warp_tabs,      NULL, (void *)warp_dlg  },
    { jwin_tab_proc,                10,     45,    300,    159,    0,                      0,                       0,       0,          0,             0, (void *) wind_warp_tabs,      NULL, (void *)warp_dlg  },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    //10
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    //15
    { jwin_text_proc,               14,     69,      0,      8,    vc(11),                 vc(1),                   0,       0,          0,             0, (void *) "Type:",             NULL,   NULL              },
    { jwin_text_proc,               14,     87,      0,      8,    vc(11),                 vc(1),                   0,       0,          0,             0, (void *) "DMap:",             NULL,   NULL              },
    { jwin_droplist_proc,           43,     65,    111,     16,    jwin_pal[jcTEXTFG],     jwin_pal[jcTEXTBG],      0,       0,          0,             0, (void *) &warp_dlg_list,      NULL,   NULL              },
    { d_dropdmaplist_proc,          43,     83,    222,     16,    jwin_pal[jcTEXTFG],     jwin_pal[jcTEXTBG],      0,       0,          0,             0, (void *) &dmap_list,          NULL,   warpdmapxy        },
    { d_dmapscrsel_proc,            45,    108,     65,     33,    vc(14),                 vc(5),                   0,       0,          1,             0,  NULL,                         NULL,   NULL              },
    //20
    { jwin_text_proc,              116,    110,      0,      8,    vc(11),                 vc(1),                   0,       0,          0,             0, (void *) "Screen:",        NULL,   NULL              },
    { jwin_edit_proc,              166,    106,     21,     16,    vc(11),                 vc(1),                   0,       0,          2,             0,  NULL,                         NULL,   NULL              },
    { d_warpbutton_proc,           191,    104,     90,     21,    vc(14),                 vc(1),                   0,       D_EXIT,    -1,            -1,  NULL,                         NULL,   NULL              },
    { jwin_check_proc,             116,    121,    129,      9,    vc(14),                 vc(1),                   0,       0,          1,             0, (void *) "Enabled",           NULL,   NULL              },
    { jwin_check_proc,             116,    132,    129,      9,    vc(14),                 vc(1),                   0,       0,          1,             0, (void *) "Full Screen",       NULL,   NULL              },
    //25
    { jwin_text_proc,               14,    149,      0,      8,    vc(11),                 vc(1),                   0,       0,          0,             0, (void *) "Out Effect:",       NULL,   NULL              },
    { jwin_text_proc,               14,    167,      0,      8,    vc(11),                 vc(1),                   0,       0,          0,             0, (void *) "In Effect:",        NULL,   NULL              },
    { jwin_droplist_proc,           68,    145,    137,     16,    jwin_pal[jcTEXTFG],     jwin_pal[jcTEXTBG],      0,       0,          0,             0, (void *) &warp_effect_list,   NULL,   NULL              },
    { jwin_droplist_proc,           68,    163,    137,     16,    jwin_pal[jcTEXTFG],     jwin_pal[jcTEXTBG],      0,       0,          0,             0, (void *) &warp_effect_list,   NULL,   NULL              },
    { jwin_text_proc,               14,    185,      0,      8,    vc(11),                 vc(1),                   0,       0,          0,             0, (void *) "Tics:",             NULL,   NULL              },
    //30
    { d_ticsedit_proc,              40,    181,     36,     16,    vc(11),                 vc(1),                   0,       0,          5,             0,  NULL,                         NULL,   NULL              },
    { jwin_text_proc,               77,    185,      0,      8,    vc(11),                 vc(1),                   0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { jwin_frame_proc,             211,    132,     95,     64,    0,                      0,                       0,       0,          FR_ETCHED,     0,  NULL,                         NULL,   NULL              },
    { jwin_text_proc,              215,    129,     40,      8,    vc(0),                  vc(11),                  0,       0,          0,             0, (void *) " Triggers ",        NULL,   NULL              },
    { d_triggerbutton_proc,        215,    139,    129,      9,    vc(14),                 vc(1),                   0,       0,          1,             0, (void *) "Up",                NULL,   NULL              },
    //35
    { d_triggerbutton_proc,        229,    139,    129,      9,    vc(14),                 vc(1),                   0,       0,          1,             0, (void *) "Down",              NULL,   NULL              },
    { d_triggerbutton_proc,        254,    139,    129,      9,    vc(14),                 vc(1),                   0,       0,          1,             0, (void *) "Left",              NULL,   NULL              },
    { d_triggerbutton_proc,        277,    139,    129,      9,    vc(14),                 vc(1),                   0,       0,          1,             0, (void *) "Right",             NULL,   NULL              },
    { d_triggerbutton_proc,        221,    152,    129,      9,    vc(14),                 vc(1),                   0,       0,          1,             0, (void *) "A",                 NULL,   NULL              },
    { d_triggerbutton_proc,        230,    152,    129,      9,    vc(14),                 vc(1),                   0,       0,          1,             0, (void *) "B",                 NULL,   NULL              },
    //40
    { d_triggerbutton_proc,        239,    152,    129,      9,    vc(14),                 vc(1),                   0,       0,          1,             0, (void *) "X",                 NULL,   NULL              },
    { d_triggerbutton_proc,        249,    152,    129,      9,    vc(14),                 vc(1),                   0,       0,          1,             0, (void *) "Y",                 NULL,   NULL              },
    { d_triggerbutton_proc,        259,    152,    129,      9,    vc(14),                 vc(1),                   0,       0,          1,             0, (void *) "L",                 NULL,   NULL              },
    { d_triggerbutton_proc,        268,    152,    129,      9,    vc(14),                 vc(1),                   0,       0,          1,             0, (void *) "R",                 NULL,   NULL              },
    { d_triggerbutton_proc,        277,    152,    129,      9,    vc(14),                 vc(1),                   0,       0,          1,             0, (void *) "Map",               NULL,   NULL              },
    //45
    { d_triggerbutton_proc,        230,    165,    129,      9,    vc(14),                 vc(1),                   0,       0,          1,             0, (void *) "Select",            NULL,   NULL              },
    { d_triggerbutton_proc,        260,    165,    129,      9,    vc(14),                 vc(1),                   0,       0,          1,             0, (void *) "Start",             NULL,   NULL              },
    { d_alltriggerbutton_proc,     252,    179,    129,      9,    vc(14),                 vc(1),                   0,       0,          1,             0, (void *) "All",               NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { d_dummy_proc,                  0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              },
    { NULL,                          0,      0,      0,      0,    0,                      0,                       0,       0,          0,             0,  NULL,                         NULL,   NULL              }
};
#endif

int onTileWarp()
{
    int tempx5=warp_dlg[5].x;
    int tempx6=warp_dlg[6].x;
    int tempx10=warp_dlg[10].x;
    int tempx11=warp_dlg[11].x;
    int tempx12=warp_dlg[12].x;
    int tempx13=warp_dlg[13].x;
    
    int tempx20=warp_dlg[20].x;
    int tempx21=warp_dlg[21].x;
    int tempx25=warp_dlg[25].x;
    int tempx26=warp_dlg[26].x;
    int tempx27=warp_dlg[27].x;
    int tempx28=warp_dlg[28].x;
    
    int tempx32=warp_dlg[32].x;
    int tempx33=warp_dlg[33].x;
    int tempx37=warp_dlg[37].x;
    int tempx38=warp_dlg[38].x;
    int tempx39=warp_dlg[39].x;
    int tempx40=warp_dlg[40].x;
    
    int tempx44=warp_dlg[44].x;
    int tempx45=warp_dlg[45].x;
    int tempx49=warp_dlg[49].x;
    int tempx50=warp_dlg[50].x;
    int tempx51=warp_dlg[51].x;
    int tempx52=warp_dlg[52].x;
    restore_mouse();
    warp_dlg[0].dp=(void *) "Tile Warp";
    warp_dlg[0].dp2=lfont;
    warp_dlg[5].x = SCREEN_W+10;
    warp_dlg[6].x = SCREEN_W+10;
    warp_dlg[10].x = SCREEN_W+10;
    warp_dlg[11].x = SCREEN_W+10;
    warp_dlg[12].x = SCREEN_W+10;
    warp_dlg[13].x = SCREEN_W+10;
    warp_dlg[20].x = SCREEN_W+10;
    warp_dlg[21].x = SCREEN_W+10;
    warp_dlg[25].x = SCREEN_W+10;
    warp_dlg[26].x = SCREEN_W+10;
    warp_dlg[27].x = SCREEN_W+10;
    warp_dlg[28].x = SCREEN_W+10;
    warp_dlg[32].x = SCREEN_W+10;
    warp_dlg[33].x = SCREEN_W+10;
    warp_dlg[37].x = SCREEN_W+10;
    warp_dlg[38].x = SCREEN_W+10;
    warp_dlg[39].x = SCREEN_W+10;
    warp_dlg[40].x = SCREEN_W+10;
    warp_dlg[44].x = SCREEN_W+10;
    warp_dlg[45].x = SCREEN_W+10;
    warp_dlg[49].x = SCREEN_W+10;
    warp_dlg[50].x = SCREEN_W+10;
    warp_dlg[51].x = SCREEN_W+10;
    warp_dlg[52].x = SCREEN_W+10;
    
    for(int i=0; i<4; i++)
    {
        warp_dlg[10+i].d2 = 0;
        warp_dlg[25+i].d2 = 0;
        warp_dlg[37+i].d2 = 0;
        warp_dlg[49+i].d2 = 0;
    }
    
    char buf[10];
    char buf2[10];
    char buf3[10];
    char buf4[10];
    sprintf(buf,"%02X",Map.CurrScr()->tilewarpscr[0]);
    warp_dlg[7].d1=Map.CurrScr()->tilewarptype[0];
    warp_dlg[8].d1=Map.CurrScr()->tilewarpdmap[0];
    warp_dlg[9].dp=buf;
    
    sprintf(buf2,"%02X",Map.CurrScr()->tilewarpscr[1]);
    warp_dlg[22].d1=Map.CurrScr()->tilewarptype[1];
    warp_dlg[23].d1=Map.CurrScr()->tilewarpdmap[1];
    warp_dlg[24].dp=buf2;
    
    sprintf(buf3,"%02X",Map.CurrScr()->tilewarpscr[2]);
    warp_dlg[34].d1=Map.CurrScr()->tilewarptype[2];
    warp_dlg[35].d1=Map.CurrScr()->tilewarpdmap[2];
    warp_dlg[36].dp=buf3;
    
    sprintf(buf4,"%02X",Map.CurrScr()->tilewarpscr[3]);
    warp_dlg[46].d1=Map.CurrScr()->tilewarptype[3];
    warp_dlg[47].d1=Map.CurrScr()->tilewarpdmap[3];
    warp_dlg[48].dp=buf4;
    
    warp_dlg[63].flags = get_bit(&Map.CurrScr()->tilewarpoverlayflags,0)?D_SELECTED:0;
    warp_dlg[64].flags = get_bit(&Map.CurrScr()->tilewarpoverlayflags,1)?D_SELECTED:0;
    warp_dlg[65].flags = get_bit(&Map.CurrScr()->tilewarpoverlayflags,2)?D_SELECTED:0;
    warp_dlg[66].flags = get_bit(&Map.CurrScr()->tilewarpoverlayflags,3)?D_SELECTED:0;
    
    word j=Map.CurrScr()->warpreturnc;
    warp_dlg[54].d1=(j&3);
    warp_dlg[56].d1=((j>>2)&3);
    warp_dlg[58].d1=((j>>4)&3);
    warp_dlg[60].d1=((j>>6)&3);
    warp_dlg[2].fg=vc(14);
    warp_dlg[17].fg=vc(14);
    warp_dlg[29].fg=vc(14);
    warp_dlg[41].fg=vc(14);
    //warp_dlg[5].fg=vc(7);
    //for(int i=0; i<4; i++)
    //warp_dlg[10+i].d2 = 0;
    dmap_list_size=MAXDMAPS;
    dmap_list_zero=true;

	DIALOG *warp_cpy = resizeDialog(warp_dlg, 1.5);
    
    if(is_large())
    {
		warp_cpy[0].d1=2;
		warp_cpy[6].x += 2;
		warp_cpy[10].w -= 8;
		warp_cpy[10].y -= 4;
		warp_cpy[11].w -= 8;
		warp_cpy[11].y -= 2;
		warp_cpy[12].h -= 8;
		warp_cpy[12].x -= 2;
		warp_cpy[12].y -= 2;
		warp_cpy[13].h -= 8;
		warp_cpy[13].x += 1;
		warp_cpy[13].y -= 2;
            
		warp_cpy[21].x += 2;
		warp_cpy[25].w -= 8;
		warp_cpy[25].y -= 4;
		warp_cpy[26].w -= 8;
		warp_cpy[26].y -= 2;
		warp_cpy[27].h -= 8;
		warp_cpy[27].x -= 2;
		warp_cpy[27].y -= 2;
		warp_cpy[28].h -= 8;
		warp_cpy[28].x += 1;
		warp_cpy[28].y -= 2;
            
		warp_cpy[33].x += 2;
		warp_cpy[37].w -= 8;
		warp_cpy[37].y -= 4;
		warp_cpy[38].w -= 8;
		warp_cpy[38].y -= 2;
		warp_cpy[39].h -= 8;
		warp_cpy[39].x -= 2;
		warp_cpy[39].y -= 2;
		warp_cpy[40].h -= 8;
		warp_cpy[40].x += 1;
		warp_cpy[40].y -= 2;
            
		warp_cpy[45].x += 2;
		warp_cpy[49].w -= 8;
		warp_cpy[49].y -= 4;
		warp_cpy[50].w -= 8;
		warp_cpy[50].y -= 2;
		warp_cpy[51].h -= 8;
		warp_cpy[51].x -= 2;
		warp_cpy[51].y -= 2;
		warp_cpy[52].h -= 8;
		warp_cpy[52].x += 1;
		warp_cpy[52].y -= 2;
        
        for(int i=0; i<4; i++)
        {
			warp_cpy[i+67].x=493;
			warp_cpy[i+67].y=329;
			warp_cpy[i+67].w=64;
			warp_cpy[i+67].h=32;
        }
    }
    
    int ret=zc_popup_dialog(warp_cpy,-1);
    
    if(ret==14 || ret==15)
    {
        saved=false;
        Map.CurrScr()->tilewarpscr[0] = xtoi(buf);
        Map.CurrScr()->tilewarptype[0] = warp_cpy[7].d1;
        Map.CurrScr()->tilewarpdmap[0] = warp_cpy[8].d1;
        Map.CurrScr()->tilewarpscr[1] = xtoi(buf2);
        Map.CurrScr()->tilewarptype[1] = warp_cpy[22].d1;
        Map.CurrScr()->tilewarpdmap[1] = warp_cpy[23].d1;
        Map.CurrScr()->tilewarpscr[2] = xtoi(buf3);
        Map.CurrScr()->tilewarptype[2] = warp_cpy[34].d1;
        Map.CurrScr()->tilewarpdmap[2] = warp_cpy[35].d1;
        Map.CurrScr()->tilewarpscr[3] = xtoi(buf4);
        Map.CurrScr()->tilewarptype[3] = warp_cpy[46].d1;
        Map.CurrScr()->tilewarpdmap[3] = warp_cpy[47].d1;
        
        Map.CurrScr()->tilewarpoverlayflags=0;
        set_bit(&Map.CurrScr()->tilewarpoverlayflags,0,(warp_cpy[63].flags & D_SELECTED)?1:0);
        set_bit(&Map.CurrScr()->tilewarpoverlayflags,1,(warp_cpy[64].flags & D_SELECTED)?1:0);
        set_bit(&Map.CurrScr()->tilewarpoverlayflags,2,(warp_cpy[65].flags & D_SELECTED)?1:0);
        set_bit(&Map.CurrScr()->tilewarpoverlayflags,3,(warp_cpy[66].flags & D_SELECTED)?1:0);
        
        j=Map.CurrScr()->warpreturnc&0xFF00;
        word newWarpReturns=0;
        newWarpReturns|= warp_cpy[60].d1;
        newWarpReturns<<=2;
        newWarpReturns|= warp_cpy[58].d1;
        newWarpReturns<<=2;
        newWarpReturns|= warp_cpy[56].d1;
        newWarpReturns<<=2;
        newWarpReturns|= warp_cpy[54].d1;
        j|=newWarpReturns;
        Map.CurrScr()->warpreturnc = j;
        refresh(rMENU);
        
    }
    
    if(ret==15)
    {
        int index=0;
        
        if(warp_tabs[0].flags & D_SELECTED) index = 0;
        
        if(warp_tabs[1].flags & D_SELECTED) index = 1;
        
        if(warp_tabs[2].flags & D_SELECTED) index = 2;
        
        if(warp_tabs[3].flags & D_SELECTED) index = 3;
        
        FlashWarpSquare = -1;
        int tm = Map.getCurrMap();
        int ts = Map.getCurrScr();
        int thistype = Map.CurrScr()->tilewarptype[index];
        Map.dowarp(0,index);
        
        if((ts!=Map.getCurrScr() || tm!=Map.getCurrMap()) && thistype != wtCAVE && thistype != wtSCROLL)
        {
            FlashWarpSquare = (TheMaps[tm*MAPSCRS+ts].warpreturnc>>(index*2))&3;
            FlashWarpClk = 32;
        }
        
        refresh(rALL);
    }

	delete[] warp_cpy;
    
    warp_dlg[5].x = tempx5;
    warp_dlg[6].x = tempx6;
    warp_dlg[10].x = tempx10;
    warp_dlg[11].x = tempx11;
    warp_dlg[12].x = tempx12;
    warp_dlg[13].x = tempx13;
    
    warp_dlg[20].x = tempx20;
    warp_dlg[21].x = tempx21;
    warp_dlg[25].x = tempx25;
    warp_dlg[26].x = tempx26;
    warp_dlg[27].x = tempx27;
    warp_dlg[28].x = tempx28;
    
    warp_dlg[32].x = tempx32;
    warp_dlg[33].x = tempx33;
    warp_dlg[37].x = tempx37;
    warp_dlg[38].x = tempx38;
    warp_dlg[39].x = tempx39;
    warp_dlg[40].x = tempx40;
    
    warp_dlg[44].x = tempx44;
    warp_dlg[45].x = tempx45;
    warp_dlg[49].x = tempx49;
    warp_dlg[50].x = tempx50;
    warp_dlg[51].x = tempx51;
    warp_dlg[52].x = tempx52;
    
    for(int i=0; i<4; i++)
    {
        warp_dlg[10+i].d2 = 0x80;
        warp_dlg[25+i].d2 = 0x80;
        warp_dlg[37+i].d2 = 0x80;
        warp_dlg[49+i].d2 = 0x80;
    }
    
    return D_O_K;
}

int onSideWarp()
{
    restore_mouse();
    warp_dlg[0].dp=(void *) "Side Warp";
    warp_dlg[0].dp2=lfont;
    warp_dlg[7].flags = 0;
    warp_dlg[22].flags = 0;
    warp_dlg[34].flags = 0;
    warp_dlg[46].flags = 0;
    
    char buf[10];
    char buf2[10];
    char buf3[10];
    char buf4[10];
    sprintf(buf,"%02X",Map.CurrScr()->sidewarpscr[0]);
    warp_dlg[7].d1=Map.CurrScr()->sidewarptype[0];
    warp_dlg[8].d1=Map.CurrScr()->sidewarpdmap[0];
    warp_dlg[9].dp=buf;
    
    sprintf(buf2,"%02X",Map.CurrScr()->sidewarpscr[1]);
    warp_dlg[22].d1=Map.CurrScr()->sidewarptype[1];
    warp_dlg[23].d1=Map.CurrScr()->sidewarpdmap[1];
    warp_dlg[24].dp=buf2;
    
    sprintf(buf3,"%02X",Map.CurrScr()->sidewarpscr[2]);
    warp_dlg[34].d1=Map.CurrScr()->sidewarptype[2];
    warp_dlg[35].d1=Map.CurrScr()->sidewarpdmap[2];
    warp_dlg[36].dp=buf3;
    
    sprintf(buf4,"%02X",Map.CurrScr()->sidewarpscr[3]);
    warp_dlg[46].d1=Map.CurrScr()->sidewarptype[3];
    warp_dlg[47].d1=Map.CurrScr()->sidewarpdmap[3];
    warp_dlg[48].dp=buf4;
    
    warp_dlg[63].flags = get_bit(&Map.CurrScr()->sidewarpoverlayflags,0)?D_SELECTED:0;
    warp_dlg[64].flags = get_bit(&Map.CurrScr()->sidewarpoverlayflags,1)?D_SELECTED:0;
    warp_dlg[65].flags = get_bit(&Map.CurrScr()->sidewarpoverlayflags,2)?D_SELECTED:0;
    warp_dlg[66].flags = get_bit(&Map.CurrScr()->sidewarpoverlayflags,3)?D_SELECTED:0;
    
    word j=Map.CurrScr()->warpreturnc>>8;
    warp_dlg[54].d1=(j&3);
    warp_dlg[56].d1=((j>>2)&3);
    warp_dlg[58].d1=((j>>4)&3);
    warp_dlg[60].d1=((j>>6)&3);
    
    warp_dlg[2].fg=warp_dlg[5].fg=vc(14);
    warp_dlg[17].fg=warp_dlg[20].fg=vc(14);
    warp_dlg[29].fg=warp_dlg[32].fg=vc(14);
    warp_dlg[41].fg=warp_dlg[44].fg=vc(14);
    byte f=Map.CurrScr()->flags2;
    byte h=Map.CurrScr()->sidewarpindex;
    byte g=f&240;
    
    for(int i=0; i<4; i++)
    {
        warp_dlg[10+i].d2 = 0x80;
        warp_dlg[25+i].d2 = 0x80;
        warp_dlg[37+i].d2 = 0x80;
        warp_dlg[49+i].d2 = 0x80;
        
        if(f&1)
        {
            warp_dlg[10+i].flags = D_SELECTED ;
            warp_dlg[10+i].d2 |= h&3;
            warp_dlg[25+i].flags = D_SELECTED ;
            warp_dlg[25+i].d2 |= h&3;
            warp_dlg[37+i].flags = D_SELECTED ;
            warp_dlg[37+i].d2 |= h&3;
            warp_dlg[49+i].flags = D_SELECTED ;
            warp_dlg[49+i].d2 |= h&3;
        }
        else
        {
            warp_dlg[10+i].flags = 0;
            warp_dlg[25+i].flags = 0;
            warp_dlg[37+i].flags = 0;
            warp_dlg[49+i].flags = 0;
        }
        
        f>>=1;
        h>>=2;
    }
    
    dmap_list_size=MAXDMAPS;
    dmap_list_zero=true;

	DIALOG *warp_cpy = resizeDialog(warp_dlg, 1.5);
    
    if(is_large())
    {
		warp_cpy[0].d1=2;
		warp_cpy[6].x += 2;
		warp_cpy[10].w -= 8;
		warp_cpy[10].y -= 4;
		warp_cpy[11].w -= 8;
		warp_cpy[11].y -= 2;
		warp_cpy[12].h -= 8;
		warp_cpy[12].x -= 2;
		warp_cpy[12].y -= 2;
		warp_cpy[13].h -= 8;
		warp_cpy[13].x += 1;
		warp_cpy[13].y -= 2;
            
		warp_cpy[21].x += 2;
		warp_cpy[25].w -= 8;
		warp_cpy[25].y -= 4;
		warp_cpy[26].w -= 8;
		warp_cpy[26].y -= 2;
		warp_cpy[27].h -= 8;
		warp_cpy[27].x -= 2;
		warp_cpy[27].y -= 2;
		warp_cpy[28].h -= 8;
		warp_cpy[28].x += 1;
		warp_cpy[28].y -= 2;
            
		warp_cpy[33].x += 2;
		warp_cpy[37].w -= 8;
		warp_cpy[37].y -= 4;
		warp_cpy[38].w -= 8;
		warp_cpy[38].y -= 2;
		warp_cpy[39].h -= 8;
		warp_cpy[39].x -= 2;
		warp_cpy[39].y -= 2;
		warp_cpy[40].h -= 8;
		warp_cpy[40].x += 1;
		warp_cpy[40].y -= 2;
            
		warp_cpy[45].x += 2;
		warp_cpy[49].w -= 8;
		warp_cpy[49].y -= 4;
		warp_cpy[50].w -= 8;
		warp_cpy[50].y -= 2;
		warp_cpy[51].h -= 8;
		warp_cpy[51].x -= 2;
		warp_cpy[51].y -= 2;
		warp_cpy[52].h -= 8;
		warp_cpy[52].x += 1;
		warp_cpy[52].y -= 2;
    }
    
    int ret=zc_popup_dialog(warp_cpy,-1);
    
    if(ret==14 || ret==15)
    {
        saved=false;
        Map.CurrScr()->sidewarpscr[0] = xtoi(buf);
        Map.CurrScr()->sidewarptype[0] = warp_cpy[7].d1;
        Map.CurrScr()->sidewarpdmap[0] = warp_cpy[8].d1;
        Map.CurrScr()->sidewarpscr[1] = xtoi(buf2);
        Map.CurrScr()->sidewarptype[1] = warp_cpy[22].d1;
        Map.CurrScr()->sidewarpdmap[1] = warp_cpy[23].d1;
        Map.CurrScr()->sidewarpscr[2] = xtoi(buf3);
        Map.CurrScr()->sidewarptype[2] = warp_cpy[34].d1;
        Map.CurrScr()->sidewarpdmap[2] = warp_cpy[35].d1;
        Map.CurrScr()->sidewarpscr[3] = xtoi(buf4);
        Map.CurrScr()->sidewarptype[3] = warp_cpy[46].d1;
        Map.CurrScr()->sidewarpdmap[3] = warp_cpy[47].d1;
        
        Map.CurrScr()->sidewarpoverlayflags=0;
        set_bit(&Map.CurrScr()->sidewarpoverlayflags,0,(warp_cpy[63].flags & D_SELECTED)?1:0);
        set_bit(&Map.CurrScr()->sidewarpoverlayflags,1,(warp_cpy[64].flags & D_SELECTED)?1:0);
        set_bit(&Map.CurrScr()->sidewarpoverlayflags,2,(warp_cpy[65].flags & D_SELECTED)?1:0);
        set_bit(&Map.CurrScr()->sidewarpoverlayflags,3,(warp_cpy[66].flags & D_SELECTED)?1:0);
        
        f=0;
        h=0;
        
        for(int i=3; i>=0; i--)
        {
            f<<=1;
            h<<=2;
            //f |= warp_dlg[49+i].flags&D_SELECTED ? 1 : 0;
            //f |= warp_dlg[37+i].flags&D_SELECTED ? 1 : 0;
            //f |= warp_dlg[25+i].flags&D_SELECTED ? 1 : 0;
            f |= warp_cpy[10+i].flags&D_SELECTED ? 1 : 0;
            int t=0;
            /*if(warp_dlg[10+i].flags&D_SELECTED) t=0;
              else if(warp_dlg[25+i].flags&D_SELECTED) t=1;
              else if(warp_dlg[37+i].flags&D_SELECTED) t=2;
              else if(warp_dlg[49+i].flags&D_SELECTED) t=3;*/
            t= warp_cpy[10+i].d2&3;
            h|=t;
        }
        
        f+=g;
        Map.CurrScr()->flags2 = f;
        Map.CurrScr()->sidewarpindex = h;
        
        j=Map.CurrScr()->warpreturnc&0x00FF;
        word newWarpReturns=0;
        newWarpReturns|= warp_cpy[60].d1;
        newWarpReturns<<=2;
        newWarpReturns|= warp_cpy[58].d1;
        newWarpReturns<<=2;
        newWarpReturns|= warp_cpy[56].d1;
        newWarpReturns<<=2;
        newWarpReturns|= warp_cpy[54].d1;
        newWarpReturns<<=8;
        j|=newWarpReturns;
        Map.CurrScr()->warpreturnc = j;
        refresh(rMENU);
    }
    
    if(ret==15)
    {
        int index=0;
        
        if(warp_tabs[0].flags & D_SELECTED) index = 0;
        
        if(warp_tabs[1].flags & D_SELECTED) index = 1;
        
        if(warp_tabs[2].flags & D_SELECTED) index = 2;
        
        if(warp_tabs[3].flags & D_SELECTED) index = 3;
        
        FlashWarpSquare = -1;
        int tm = Map.getCurrMap();
        int ts = Map.getCurrScr();
        int thistype = Map.CurrScr()->sidewarptype[index];
        Map.dowarp(1,index);
        
        if((ts!=Map.getCurrScr() || tm!=Map.getCurrMap()) && thistype != wtSCROLL)
        {
            FlashWarpSquare = (TheMaps[tm*MAPSCRS+ts].warpreturnc>>(8+index*2))&3;
            FlashWarpClk = 0x20;
        }
        
        refresh(rALL);
    }

	delete[] warp_cpy;
    
    return D_O_K;
}

/*******************************/
/*********** onPath ************/
/*******************************/

const char *dirlist(int index, int *list_size)
{
    if(index>=0)
    {
        if(index>3)
            index=3;
            
        return dirstr[index];
    }
    
    *list_size=4;
    return NULL;
}

static ListData path_dlg_list(dirlist, &font);

static DIALOG path_dlg[] =
{
    /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp) */
    { jwin_win_proc,      80,   57,   161,  164,  vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "Maze Path", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { jwin_text_proc,       94,   106,   192,  8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "1st", NULL, NULL },
    { jwin_text_proc,       94,   124,  192,  8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "2nd", NULL, NULL },
    { jwin_text_proc,       94,   142,  192,  8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "3rd", NULL, NULL },
    { jwin_text_proc,       94,   160,  192,  8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "4th", NULL, NULL },
    { jwin_text_proc,       94,   178,  192,  8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Exit", NULL, NULL },
    { jwin_droplist_proc,   140,  102,   80+1,   16,   jwin_pal[jcTEXTFG], jwin_pal[jcTEXTBG],  0,       0,          0,             0, (void *) &path_dlg_list, NULL, NULL },
    { jwin_droplist_proc,   140,  120,   80+1,   16,   jwin_pal[jcTEXTFG], jwin_pal[jcTEXTBG],  0,       0,          0,             0, (void *) &path_dlg_list, NULL, NULL },
    { jwin_droplist_proc,   140,  138,  80+1,   16,   jwin_pal[jcTEXTFG], jwin_pal[jcTEXTBG],  0,       0,          0,             0, (void *) &path_dlg_list, NULL, NULL },
    { jwin_droplist_proc,   140,  156,  80+1,   16,   jwin_pal[jcTEXTFG], jwin_pal[jcTEXTBG],  0,       0,          0,             0, (void *) &path_dlg_list, NULL, NULL },
    { jwin_droplist_proc,   140,  174,  80+1,   16,   jwin_pal[jcTEXTFG], jwin_pal[jcTEXTBG],  0,       0,          0,             0, (void *) &path_dlg_list, NULL, NULL },
    { jwin_button_proc,     90,   194,  61,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "OK", NULL, NULL },
    { jwin_button_proc,     170,  194,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_F1,   0, (void *) onHelp, NULL, NULL },
    { jwin_text_proc,       87,   82,   192,  8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "A Lost Woods-style maze screen", NULL, NULL },
    { jwin_text_proc,       87,   92,   192,  8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "with a normal and secret exit.", NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

int onPath()
{
    restore_mouse();
    path_dlg[0].dp2=lfont;
    
    for(int i=0; i<4; i++)
        path_dlg[i+7].d1 = Map.CurrScr()->path[i];
        
    path_dlg[11].d1 = Map.CurrScr()->exitdir;

	DIALOG *path_cpy = resizeDialog(path_dlg, 1.5);
    
    int ret;
    
    do
    {
        ret=zc_popup_dialog(path_cpy,7);
        
        if(ret==12) for(int i=0; i<4; i++)
            {
                if(path_cpy[i+7].d1 == path_cpy[11].d1)
                {
                    if(jwin_alert("Exit Problem","One of the path's directions is","also the normal Exit direction! Continue?",NULL,"Yes","No",'y','n',lfont)==2)
                        ret = -1;
                        
                    break;
                }
            }
    }
    while(ret == -1);
    
    if(ret==12)
    {
        saved=false;
        
        for(int i=0; i<4; i++)
            Map.CurrScr()->path[i] = path_cpy[i+7].d1;
            
        Map.CurrScr()->exitdir = path_cpy[11].d1;
        
        if(!(Map.CurrScr()->flags&fMAZE))
            if(jwin_alert("Screen Flag","Turn on the 'Use Maze Path' Screen Flag?","(Go to 'Screen Data' to turn it off.)",NULL,"Yes","No",'y','n',lfont)==1)
                Map.CurrScr()->flags |= fMAZE;
    }
	
	delete[] path_cpy;
    
    refresh(rMAP+rMENU);
    return D_O_K;
}

/********************************/
/********* onInfoTypes **********/
/********************************/

static DIALOG editinfo_dlg[] =
{
    // (dialog proc)     (x)   (y)   (w)   (h)   (fg)                 (bg)                  (key)    (flags)     (d1)           (d2)     (dp)
    { jwin_win_proc,       0,   10,  208,  204,  vc(14),              vc(1),                  0,      D_EXIT,     0,             0,       NULL, NULL, NULL },
    { d_timer_proc,        0,    0,    0,    0,  0,                   0,                      0,           0,     0,             0,       NULL, NULL, NULL },
    { jwin_text_proc,     24,   60,   48,    8,  vc(7),               vc(1),                  0,           0,     0,             0, (void *) "1st", NULL, NULL },
    { jwin_text_proc,     24,  106,   48,    8,  vc(7),               vc(1),                  0,           0,     0,             0, (void *) "2nd", NULL, NULL },
    { jwin_text_proc,     24,  152,   48,    8,  vc(7),               vc(1),                  0,           0,     0,             0, (void *) "3rd", NULL, NULL },
    { jwin_text_proc,     56,   60,   88,    8,  vc(14),              vc(1),                  0,           0,     0,             0, (void *) "Price:", NULL, NULL },
    { jwin_text_proc,     56,  106,   88,    8,  vc(14),              vc(1),                  0,           0,     0,             0, (void *) "Price:", NULL, NULL },
    { jwin_text_proc,     56,  152,   88,    8,  vc(14),              vc(1),                  0,           0,     0,             0, (void *) "Price:", NULL, NULL },
    // 8
    { jwin_edit_proc,     86,   56,   32,   16,  vc(12),              vc(1),                  0,           0,     5,             0,       NULL, NULL, NULL },
    { d_ndroplist_proc,   56,   74,  137,   16,  jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],     0,           0,     0,             0,       NULL, NULL, NULL },
    { jwin_edit_proc,     86,  102,   32,   16,  vc(12),              vc(1),                  0,           0,     5,             0,       NULL, NULL, NULL },
    { d_ndroplist_proc,   56,  120,  137,   16,  jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],     0,           0,     0,             0,       NULL, NULL, NULL },
    { jwin_edit_proc,     86,  148,   32,   16,  vc(12),              vc(1),                  0,           0,     5,             0,       NULL, NULL, NULL },
    { d_ndroplist_proc,   56,  166,  137,   16,  jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],     0,           0,     0,             0,       NULL, NULL, NULL },
    { jwin_text_proc,     24,   42,   88,    8,  vc(14),              vc(1),                  0,           0,     0,             0, (void *) "Name:", NULL, NULL },
    { jwin_edit_proc,     56,   38,  137,   16,  vc(12),              vc(1),                  0,           0,    31,             0,       NULL, NULL, NULL },
    // 16
    { jwin_button_proc,   34,  188,   61,   21,  vc(14),              vc(1),                 13,      D_EXIT,     0,             0, (void *) "OK", NULL, NULL },
    { jwin_button_proc,  114,  188,   61,   21,  vc(14),              vc(1),                 27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { NULL,                0,    0,    0,    0,  0,                   0,                      0,           0,     0,             0,       NULL,                           NULL,  NULL }
};

void EditInfoType(int index)
{
    char ps1[6],ps2[6],ps3[6];
    char infoname[32];
    char caption[40];
    
    int str1, str2, str3;
    
    sprintf(caption,"Info Data %d",index);
    editinfo_dlg[0].dp = caption;
    editinfo_dlg[0].dp2 = lfont;
    
    sprintf(ps1,"%d",misc.info[index].price[0]);
    sprintf(ps2,"%d",misc.info[index].price[1]);
    sprintf(ps3,"%d",misc.info[index].price[2]);
    snprintf(infoname,32,"%s",misc.info[index].name);
    editinfo_dlg[8].dp  = ps1;
    editinfo_dlg[10].dp = ps2;
    editinfo_dlg[12].dp = ps3;
    editinfo_dlg[15].dp = infoname;
    str1 = misc.info[index].str[0];
    str2 = misc.info[index].str[1];
    str3 = misc.info[index].str[2];
    editinfo_dlg[9].d1  = MsgStrings[str1].listpos;
    editinfo_dlg[11].d1 = MsgStrings[str2].listpos;
    editinfo_dlg[13].d1 = MsgStrings[str3].listpos;
//  ListData msgs_list(msgslist, is_large ? &sfont3 : &font);
    ListData msgs_list(msgslist2, is_large() ? &lfont_l : &font);
    editinfo_dlg[9].dp  =
        editinfo_dlg[11].dp =
            editinfo_dlg[13].dp = (void *) &msgs_list;

	DIALOG *editinfo_cpy = resizeDialog(editinfo_dlg, 1.5);
            
    int ret = zc_popup_dialog(editinfo_cpy,-1);
    
    if(ret==16)
    {
        saved=false;
        misc.info[index].price[0] = vbound(atoi(ps1), 0, 65535);
        misc.info[index].price[1] = vbound(atoi(ps2), 0, 65535);
        misc.info[index].price[2] = vbound(atoi(ps3), 0, 65535);
        snprintf(misc.info[index].name,32,"%s",infoname);
        str1 = editinfo_cpy[9].d1;
        str2 = editinfo_cpy[11].d1;
        str3 = editinfo_cpy[13].d1;
        misc.info[index].str[0] = msg_at_pos(str1);
        misc.info[index].str[1] = msg_at_pos(str2);
        misc.info[index].str[2] = msg_at_pos(str3);
        
        //move 0s to the end
        word swaptmp;
        
        if(misc.info[index].str[0] == 0)
        {
            //possibly permute the infos
            if(misc.info[index].str[1] != 0)
            {
                //swap
                swaptmp = misc.info[index].str[0];
                misc.info[index].str[0] = misc.info[index].str[1];
                misc.info[index].str[1] = swaptmp;
                swaptmp = misc.info[index].price[0];
                misc.info[index].price[0] = misc.info[index].price[1];
                misc.info[index].price[1] = swaptmp;
            }
            else if(misc.info[index].str[2] != 0)
            {
                //move info 0 to 1, 1 to 2, and 2 to 0
                swaptmp = misc.info[index].str[0];
                misc.info[index].str[0] = misc.info[index].str[2];
                misc.info[index].str[2] = misc.info[index].str[1];
                misc.info[index].str[1] = swaptmp;
                swaptmp = misc.info[index].price[0];
                misc.info[index].price[0] = misc.info[index].price[2];
                misc.info[index].price[2] = misc.info[index].price[1];
                misc.info[index].price[1] = swaptmp;
            }
        }
        
        if(misc.info[index].str[1] == 0 && misc.info[index].str[2] != 0)
            //swap
        {
            swaptmp = misc.info[index].str[1];
            misc.info[index].str[1] = misc.info[index].str[2];
            misc.info[index].str[2] = swaptmp;
            swaptmp = misc.info[index].price[1];
            misc.info[index].price[1] = misc.info[index].price[2];
            misc.info[index].price[2] = swaptmp;
        }
    }

	delete[] editinfo_cpy;
}

int onInfoTypes()
{
    info_list_size = 256;
    
    int index = select_data("Info Types",0,infolist,"Edit","Done",lfont);
    
    while(index!=-1)
    {
        EditInfoType(index);
        
        index = select_data("Info Types",index,infolist,"Edit","Done",lfont);
    }
    
    return D_O_K;
}

/********************************/
/********* onShopTypes **********/
/********************************/

static DIALOG editshop_dlg[] =
{
    // (dialog proc)     (x)   (y)   (w)   (h)   (fg)                 (bg)                  (key)    (flags)     (d1)           (d2)     (dp)
    { jwin_win_proc,       0,   10,  221,  204,  vc(14),              vc(1),                  0,      D_EXIT,     0,             0,       NULL, NULL, NULL },
    { d_timer_proc,        0,    0,    0,    0,  0,                   0,                      0,           0,     0,             0,       NULL, NULL, NULL },
    { jwin_text_proc,     24,   60,   48,    8,  vc(7),               vc(1),                  0,           0,     0,             0, (void *) "1st", NULL, NULL },
    { jwin_text_proc,     24,  106,   48,    8,  vc(7),               vc(1),                  0,           0,     0,             0, (void *) "2nd", NULL, NULL },
    { jwin_text_proc,     24,  152,   48,    8,  vc(7),               vc(1),                  0,           0,     0,             0, (void *) "3rd", NULL, NULL },
    { jwin_text_proc,     56,   60,   88,    8,  vc(14),              vc(1),                  0,           0,     0,             0, (void *) "Price:", NULL, NULL },
    { jwin_text_proc,     56,  106,   88,    8,  vc(14),              vc(1),                  0,           0,     0,             0, (void *) "Price:", NULL, NULL },
    { jwin_text_proc,     56,  152,   88,    8,  vc(14),              vc(1),                  0,           0,     0,             0, (void *) "Price:", NULL, NULL },
    // 8
    { jwin_edit_proc,     86,   56,   32,   16,  vc(12),              vc(1),                  0,           0,     5,             0,       NULL, NULL, NULL },
    { d_nidroplist_proc,  56,   74,  137,   16,  jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],     0,           0,     0,             0,       NULL, NULL, NULL },
    { jwin_edit_proc,     86,  102,   32,   16,  vc(12),              vc(1),                  0,           0,     5,             0,       NULL, NULL, NULL },
    { d_nidroplist_proc,  56,  120,  137,   16,  jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],     0,           0,     0,             0,       NULL, NULL, NULL },
    { jwin_edit_proc,     86,  148,   32,   16,  vc(12),              vc(1),                  0,           0,     5,             0,       NULL, NULL, NULL },
    { d_nidroplist_proc,  56,  166,  137,   16,  jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],     0,           0,     0,             0,       NULL, NULL, NULL },
    { jwin_text_proc,     24,   42,   88,    8,  vc(14),              vc(1),                  0,           0,     0,             0, (void *) "Name:", NULL, NULL },
    { jwin_edit_proc,     56,   38,  137,   16,  vc(12),              vc(1),                  0,           0,    31,             0,       NULL, NULL, NULL },
    
    // 16
    { jwin_button_proc,   40,  188,   61,   21,  vc(14),              vc(1),                 13,      D_EXIT,     0,             0, (void *) "OK", NULL, NULL },
    { jwin_button_proc,  121,  188,   61,   21,  vc(14),              vc(1),                 27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { NULL,                0,    0,    0,    0,  0,                   0,                      0,      0,          0,             0,       NULL,                           NULL,  NULL }
};

void EditShopType(int index)
{

    build_bii_list(true);
    char ps1[6],ps2[6],ps3[6];
    char shopname[32];
    char caption[40];
    
    sprintf(caption,"Shop Data %d",index);
    editshop_dlg[0].dp = caption;
    editshop_dlg[0].dp2=lfont;
    
    sprintf(ps1,"%d",misc.shop[index].price[0]);
    sprintf(ps2,"%d",misc.shop[index].price[1]);
    sprintf(ps3,"%d",misc.shop[index].price[2]);
    sprintf(shopname,"%s",misc.shop[index].name);
    editshop_dlg[8].dp  = ps1;
    editshop_dlg[10].dp = ps2;
    editshop_dlg[12].dp = ps3;
    editshop_dlg[15].dp = shopname;
    
//  ListData item_list(itemlist, is_large ? &sfont3 : &font);
    ListData item_list(itemlist, is_large() ? &lfont_l : &font);
    
    editshop_dlg[9].dp  = (void *) &item_list;
    editshop_dlg[11].dp  = (void *) &item_list;
    editshop_dlg[13].dp  = (void *) &item_list;
    
    for(int i=0; i<3; ++i)
    {
        if(misc.shop[index].hasitem[i])
        {
            for(int j=0; j<bii_cnt; j++)
            {
                if(bii[j].i == misc.shop[index].item[i])
                {
                    editshop_dlg[9+(i<<1)].d1  = j;
                }
            }
        }
        else
        {
            editshop_dlg[9+(i<<1)].d1 = -2;
        }
    }

	DIALOG *editshop_cpy = resizeDialog(editshop_dlg, 1.5);
    
    int ret = zc_popup_dialog(editshop_cpy,-1);
    
    if(ret==16)
    {
        saved=false;
        misc.shop[index].price[0] = vbound(atoi(ps1), 0, 65535);
        misc.shop[index].price[1] = vbound(atoi(ps2), 0, 65535);
        misc.shop[index].price[2] = vbound(atoi(ps3), 0, 65535);
        snprintf(misc.shop[index].name, 32, "%s",shopname);
        
        for(int i=0; i<3; ++i)
        {
            if(bii[editshop_cpy[9+(i<<1)].d1].i == -2)
            {
                misc.shop[index].hasitem[i] = 0;
                misc.shop[index].item[i] = 0;
                misc.shop[index].price[i] = 0;
            }
            else
            {
                misc.shop[index].hasitem[i] = 1;
                misc.shop[index].item[i] = bii[editshop_cpy[9+(i<<1)].d1].i;
            }
        }
        
        //filter all the 0 items to the end (yeah, bubble sort; sue me)
        word swaptmp;
        
        for(int j=0; j<3-1; j++)
        {
            for(int k=0; k<2-j; k++)
            {
                if(misc.shop[index].hasitem[k]==0)
                {
                    swaptmp = misc.shop[index].item[k];
                    misc.shop[index].item[k] = misc.shop[index].item[k+1];
                    misc.shop[index].item[k+1] = swaptmp;
                    swaptmp = misc.shop[index].price[k];
                    misc.shop[index].price[k] = misc.shop[index].price[k+1];
                    misc.shop[index].price[k+1] = swaptmp;
                    swaptmp = misc.shop[index].hasitem[k];
                    misc.shop[index].hasitem[k] = misc.shop[index].item[k+1];
                    misc.shop[index].hasitem[k+1] = swaptmp;
                }
            }
        }
    }

	delete[] editshop_cpy;
}

int onShopTypes()
{
    shop_list_size = 256;
    
    int index = select_data("Shop Types",0,shoplist,"Edit","Done",lfont);
    
    while(index!=-1)
    {
        EditShopType(index);
        index = select_data("Shop Types",index,shoplist,"Edit","Done",lfont);
    }
    
    return D_O_K;
}

/***********************************/
/********* onItemDropSets **********/
/***********************************/

static char item_drop_set_str_buf[40];
int item_drop_set_list_size=MAXITEMDROPSETS;

const char *itemdropsetlist(int index, int *list_size)
{
    if(index>=0)
    {
        bound(index,0,item_drop_set_list_size-1);
        sprintf(item_drop_set_str_buf,"%3d:  %s",index,item_drop_sets[index].name);
        return item_drop_set_str_buf;
    }
    
    *list_size=item_drop_set_list_size;
    return NULL;
}

int d_itemdropedit_proc(int msg,DIALOG *d,int c);

static int edititemdropset_1_list[] =
{
    // dialog control number
    10, 11, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,24,25,26,27,28, -1
};

static int edititemdropset_2_list[] =
{
    // dialog control number
    12, 13, 29, 30, 31, 32, 33,34,35,36,37,38,39,40,41,42,43, -1
};

static TABPANEL edititemdropset_tabs[] =
{
    // (text)
    { (char *)" Page 1 ",       D_SELECTED,   edititemdropset_1_list,  0, NULL },
    { (char *)" Page 2 ",       0,            edititemdropset_2_list,  0, NULL },
    { NULL,                     0,            NULL,                    0, NULL }
};

static DIALOG edititemdropset_dlg[] =
{
    // (dialog proc)     (x)   (y)   (w)   (h)   (fg)                 (bg)                  (key)    (flags)     (d1)           (d2)     (dp)
    { jwin_win_proc,       0,    0,  320,  240,  vc(14),              vc(1),                  0,      D_EXIT,     0,             0,       NULL, NULL, NULL },
    { d_timer_proc,        0,    0,    0,    0,  0,                   0,                      0,           0,     0,             0,       NULL, NULL, NULL },
    
    // 2
    { jwin_button_proc,   89,  213,   61,   21,  vc(14),              vc(1),                 13,      D_EXIT,     0,             0, (void *) "OK", NULL, NULL },
    { jwin_button_proc,  169,  213,   61,   21,  vc(14),              vc(1),                 27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    
    // 4
    { jwin_text_proc,      9,   29,   88,    8,  vc(14),              vc(1),                  0,           0,     0,             0, (void *) "Name:", NULL, NULL },
    { jwin_edit_proc,     39,   25,  275,   16,  vc(12),              vc(1),                  0,           0,    32,             0,       NULL, NULL, NULL },
    { jwin_text_proc,      9,   47,   88,    8,  vc(14),              vc(1),                  0,           0,     0,             0, (void *) "Nothing Chance:", NULL, NULL },
    { d_itemdropedit_proc,     84,   43,   26,   16,  vc(12),              vc(1),                  0,           0,     5,             0,       NULL, NULL, (void *)edititemdropset_dlg },
    
    { jwin_tab_proc,       4,   65,  312,   143, vc(0),               vc(15),                 0,           0,     0,             0, (void *) edititemdropset_tabs,                    NULL, (void *)edititemdropset_dlg },
    { jwin_text_proc,     114,   43+4,   26,   16,   vc(14),              vc(1),                  0,           0,     0,             0,       NULL, NULL, NULL },
    // 10
    { jwin_text_proc,     10,   87,   88,    8,  vc(14),              vc(1),                  0,           0,     0,             0, (void *) "Chance:", NULL, NULL },
    { jwin_text_proc,     56,   87,   88,    8,  vc(14),              vc(1),                  0,           0,     0,             0, (void *) "Item:", NULL, NULL },
    { jwin_text_proc,     10,   87,   88,    8,  vc(14),              vc(1),                  0,           0,     0,             0, (void *) "Chance:", NULL, NULL },
    { jwin_text_proc,     56,   87,   88,    8,  vc(14),              vc(1),                  0,           0,     0,             0, (void *) "Item:", NULL, NULL },
    
    // 14
    { d_itemdropedit_proc,      9,   96,   26,   16,  vc(12),              vc(1),                  0,           0,     5,             0,       NULL, NULL, (void *)edititemdropset_dlg },
    { d_idroplist_proc,   55,   96,  233,   16,  jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],     0,           0,     0,             0,       NULL, NULL, NULL },
    { jwin_text_proc,     37,   96+4,   26,   16,  vc(14),              vc(1),                  0,           0,     0,             0,       NULL, NULL, NULL },
    { d_itemdropedit_proc,      9,  118,   26,   16,  vc(12),              vc(1),                  0,           0,     5,             0,       NULL, NULL, (void *)edititemdropset_dlg },
    { d_idroplist_proc,   55,  118,  233,   16,  jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],     0,           0,     0,             0,       NULL, NULL, NULL },
    { jwin_text_proc,     37,  118+4,   26,   16,  vc(14),              vc(1),                  0,           0,     0,             0,       NULL, NULL, NULL },
    { d_itemdropedit_proc,      9,  140,   26,   16,  vc(12),              vc(1),                  0,           0,     5,             0,       NULL, NULL, (void *)edititemdropset_dlg },
    { d_idroplist_proc,   55,  140,  233,   16,  jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],     0,           0,     0,             0,       NULL, NULL, NULL },
    { jwin_text_proc,     37,  140+4,   26,   16,  vc(14),              vc(1),                  0,           0,     0,             0,       NULL, NULL, NULL },
    { d_itemdropedit_proc,      9,  162,   26,   16,  vc(12),              vc(1),                  0,           0,     5,             0,       NULL, NULL, (void *)edititemdropset_dlg },
    { d_idroplist_proc,   55,  162,  233,   16,  jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],     0,           0,     0,             0,       NULL, NULL, NULL },
    { jwin_text_proc,       37,  162+4,   26,   16,  vc(14),              vc(1),                  0,           0,     0,             0,       NULL, NULL, NULL },
    { d_itemdropedit_proc,      9,  184,   26,   16,  vc(12),              vc(1),                  0,           0,     5,             0,       NULL, NULL, (void *)edititemdropset_dlg },
    { d_idroplist_proc,   55,  184,  233,   16,  jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],     0,           0,     0,             0,       NULL, NULL, NULL },
    { jwin_text_proc,       37,  184+4,   26,   16,  vc(14),              vc(1),                  0,           0,     0,             0,       NULL, NULL, NULL },
// 29
    { d_itemdropedit_proc,      9,   96,   26,   16,  vc(12),              vc(1),                  0,           0,     5,             0,       NULL, NULL, (void *)edititemdropset_dlg },
    { d_idroplist_proc,   55,   96,  233,   16,  jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],     0,           0,     0,             0,       NULL, NULL, NULL },
    { jwin_text_proc,     37,   96+4,   26,   16,  vc(14),              vc(1),                  0,           0,     0,             0,       NULL, NULL, NULL },
    { d_itemdropedit_proc,      9,  118,   26,   16,  vc(12),              vc(1),                  0,           0,     5,             0,       NULL, NULL, (void *)edititemdropset_dlg },
    { d_idroplist_proc,   55,  118,  233,   16,  jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],     0,           0,     0,             0,       NULL, NULL, NULL },
    { jwin_text_proc,      37,  118+4,   26,   16,  vc(14),              vc(1),                  0,           0,     0,             0,       NULL, NULL, NULL },
    { d_itemdropedit_proc,      9,  140,   26,   16,  vc(12),              vc(1),                  0,           0,     5,             0,       NULL, NULL, (void *)edititemdropset_dlg },
    { d_idroplist_proc,   55,  140,  233,   16,  jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],     0,           0,     0,             0,       NULL, NULL, NULL },
    { jwin_text_proc,      37,  140+4,   26,   16,  vc(14),              vc(1),                  0,           0,     0,             0,       NULL, NULL, NULL },
    { d_itemdropedit_proc,      9,  162,   26,   16,  vc(12),              vc(1),                  0,           0,     5,             0,       NULL, NULL, (void *)edititemdropset_dlg },
    { d_idroplist_proc,   55,  162,  233,   16,  jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],     0,           0,     0,             0,       NULL, NULL, NULL },
    { jwin_text_proc,      37,  162+4,   26,   16,  vc(14),              vc(1),                  0,           0,     0,             0,       NULL, NULL, NULL },
    { d_itemdropedit_proc,      9,  184,   26,   16,  vc(12),              vc(1),                  0,           0,     5,             0,       NULL, NULL, (void *)edititemdropset_dlg },
    { d_idroplist_proc,   55,  184,  233,   16,  jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],     0,           0,     0,             0,       NULL, NULL, NULL },
    { jwin_text_proc,      37,  184+4,   26,   16,  vc(14),              vc(1),                  0,           0,     0,             0,       NULL, NULL, NULL },
    { NULL,                0,    0,    0,    0,  0,                   0,                      0,      0,          0,             0,       NULL, NULL,  NULL }
};

int d_itemdropedit_proc(int msg,DIALOG *d,int c)
{
    int ret = jwin_edit_proc(msg,d,c);
	DIALOG *parent = (DIALOG *)d->dp3;
    
    if(msg==MSG_DRAW)
    {
        int t = atoi((char*)parent[7].dp);
        
        for(int i=0; i<10; ++i)
        {
            t += atoi((char*)parent[14+(i*3)].dp);
        }
        
        {
            int t2 = (int)(100*atoi((char*)parent[7].dp) / zc_max(t,1));
            sprintf((char*)parent[9].dp,"%d%%%s",t2, is_large() && t2 <= 11 ? " ":"");
            object_message(&parent[9],MSG_DRAW,c);
        }
        
        for(int i=0; i<10; ++i)
        {
            int t2 = (int)(100*atoi((char*)parent[14+(i*3)].dp) / zc_max(t,1));
            sprintf((char*)parent[16+(i*3)].dp,"%d%%%s",t2, is_large() && t2 <= 11 ? " ":"");
            object_message(&parent[16+(i*3)],MSG_DRAW,c);
        }
        
    }
    
    return ret;
}

void EditItemDropSet(int index)
{
    build_bii_list(true);
    char chance[11][10];
    char itemdropsetname[64];
    char caption[40];
    char percent_str[11][5];
    
    sprintf(caption,"Item Drop Set Data %d",index);
    edititemdropset_dlg[0].dp = caption;
    edititemdropset_dlg[0].dp2=lfont;
    
    sprintf(itemdropsetname,"%s",item_drop_sets[index].name);
    edititemdropset_dlg[5].dp = itemdropsetname;
    
    sprintf(chance[0],"%d",item_drop_sets[index].chance[0]);
    edititemdropset_dlg[7].dp = chance[0];
    
    ListData item_list(itemlist, is_large() ? &lfont_l : &font);
    sprintf(percent_str[0],"    ");
    edititemdropset_dlg[9].dp  = percent_str[0];
    
    for(int i=0; i<10; ++i)
    {
        sprintf(chance[i+1],"%d",item_drop_sets[index].chance[i+1]);
        edititemdropset_dlg[14+(i*3)].dp  = chance[i+1];
        edititemdropset_dlg[15+(i*3)].dp  = (void *) &item_list;
        sprintf(percent_str[i+1],"    ");
        edititemdropset_dlg[16+(i*3)].dp  = percent_str[i+1];
        
        if(item_drop_sets[index].chance[i+1]==0)
        {
            edititemdropset_dlg[15+(i*3)].d1  = -2;
        }
        else
        {
            for(int j=0; j<bii_cnt; j++)
            {
                if(bii[j].i == item_drop_sets[index].item[i])
                {
                    edititemdropset_dlg[15+(i*3)].d1  = j;
                }
            }
        }
    }

	DIALOG *edititemdropset_cpy = resizeDialog(edititemdropset_dlg, 1.5);
    
    int ret = zc_popup_dialog(edititemdropset_cpy,-1);
    
    if(ret==2)
    {
        saved=false;
        
        sprintf(item_drop_sets[index].name,"%s",itemdropsetname);
        
        item_drop_sets[index].chance[0]=atoi(chance[0]);
        
        for(int i=0; i<10; ++i)
        {
            item_drop_sets[index].chance[i+1]=atoi(chance[i+1]);
            
            if(bii[edititemdropset_cpy[15+(i*3)].d1].i == -2)
            {
                item_drop_sets[index].chance[i+1]=0;
            }
            else
            {
                item_drop_sets[index].item[i] = bii[edititemdropset_cpy[15+(i*3)].d1].i;
            }
            
            if(item_drop_sets[index].chance[i+1]==0)
            {
                item_drop_sets[index].item[i] = 0;
            }
        }
    }

	delete[] edititemdropset_cpy;
}

int count_item_drop_sets()
{
    int count=0;
    bool found=false;
    
    for(count=255; (count>0); --count)
    {
        for(int i=0; (i<11); ++i)
        {
            if(item_drop_sets[count].chance[i]!=0)
            {
                found=true;
                break;
            }
        }
        
        if(found)
        {
            break;
        }
    }
    
    return count+1;
}

static void copyDropSet(int src, int dest)
{
    for(int i=0; i<11; i++)
    {
        if(i<10)
            item_drop_sets[dest].item[i]=item_drop_sets[src].item[i];
        item_drop_sets[dest].chance[i]=item_drop_sets[src].chance[i];
    }
}

int onItemDropSets()
{
    item_drop_set_list_size = MAXITEMDROPSETS;
    
    int index = select_data("Item Drop Sets",0,itemdropsetlist,"Edit","Done",lfont, copyDropSet);
    
    while(index!=-1)
    {
        EditItemDropSet(index);
        index = select_data("Item Drop Sets",index,itemdropsetlist,"Edit","Done",lfont, copyDropSet);
    }
    
    return D_O_K;
}

/********************************/
/********* onWarpRings **********/
/********************************/

int curr_ring;

void EditWarpRingScr(int ring,int index)
{
    char caption[40],buf[10];
    restore_mouse();
    int tempx5=warp_dlg[5].x;
    int tempx6=warp_dlg[6].x;
    int tempx10=warp_dlg[10].x;
    int tempx11=warp_dlg[11].x;
    int tempx12=warp_dlg[12].x;
    int tempx13=warp_dlg[13].x;
    
    int tempx[100];
    
    for(int m=17; m<100; m++)
    {
        tempx[m-17]=warp_dlg[m].x;
        
        if(m!=67)
        {
            warp_dlg[m].x = SCREEN_W+10;
        }
    }
    
    /*int tempx20=warp_dlg[20].x;
      int tempx21=warp_dlg[21].x;
      int tempx25=warp_dlg[25].x;
      int tempx26=warp_dlg[26].x;
      int tempx27=warp_dlg[27].x;
      int tempx28=warp_dlg[28].x;
    
      int tempx32=warp_dlg[32].x;
      int tempx33=warp_dlg[33].x;
      int tempx37=warp_dlg[37].x;
      int tempx38=warp_dlg[38].x;
      int tempx39=warp_dlg[39].x;
      int tempx40=warp_dlg[40].x;
    
      int tempx44=warp_dlg[44].x;
      int tempx45=warp_dlg[45].x;
      int tempx49=warp_dlg[49].x;
      int tempx50=warp_dlg[50].x;
      int tempx51=warp_dlg[51].x;
      int tempx52=warp_dlg[52].x;*/
    
    warp_dlg[5].x = SCREEN_W+10;
    warp_dlg[6].x = SCREEN_W+10;
    warp_dlg[10].x = SCREEN_W+10;
    warp_dlg[11].x = SCREEN_W+10;
    warp_dlg[12].x = SCREEN_W+10;
    warp_dlg[13].x = SCREEN_W+10;
    
    /*warp_dlg[20].x = SCREEN_W+10;
      warp_dlg[21].x = SCREEN_W+10;
      warp_dlg[25].x = SCREEN_W+10;
      warp_dlg[26].x = SCREEN_W+10;
      warp_dlg[27].x = SCREEN_W+10;
      warp_dlg[28].x = SCREEN_W+10;
      warp_dlg[32].x = SCREEN_W+10;
      warp_dlg[33].x = SCREEN_W+10;
      warp_dlg[37].x = SCREEN_W+10;
      warp_dlg[38].x = SCREEN_W+10;
      warp_dlg[39].x = SCREEN_W+10;
      warp_dlg[40].x = SCREEN_W+10;
      warp_dlg[44].x = SCREEN_W+10;
      warp_dlg[45].x = SCREEN_W+10;
      warp_dlg[49].x = SCREEN_W+10;
      warp_dlg[50].x = SCREEN_W+10;
      warp_dlg[51].x = SCREEN_W+10;
      warp_dlg[52].x = SCREEN_W+10;*/
    for(int i=0; i<4; i++)
    {
        warp_dlg[10+i].d2 = 0;
        warp_dlg[25+i].d2 = 0;
        warp_dlg[37+i].d2 = 0;
        warp_dlg[49+i].d2 = 0;
    }
    
    sprintf(caption,"Ring %d  Warp %d",ring,index+1);
    warp_dlg[0].dp = (void *)caption;
    warp_dlg[0].dp2=lfont;
    
    warp_dlg[1].dp = NULL;
    warp_dlg[1].dp3 = NULL;
    
    sprintf(buf,"%02X",misc.warp[ring].scr[index]);
    warp_dlg[8].d1=misc.warp[ring].dmap[index];
    warp_dlg[9].dp=buf;
    warp_dlg[24].dp=buf;
    warp_dlg[36].dp=buf;
    warp_dlg[48].dp=buf;
    warp_dlg[2].fg=warp_dlg[5].fg=vc(7);
    
    for(int i=0; i<4; i++)
        warp_dlg[10+i].d2 = 0;
        
    dmap_list_size=MAXDMAPS;
    dmap_list_zero=true;

	DIALOG *warp_cpy = resizeDialog(warp_dlg, 1.5);
    
    int ret=zc_popup_dialog(warp_cpy,-1);
    
    if(ret==14 || ret==15)
    {
        saved=false;
        misc.warp[ring].dmap[index] = warp_cpy[8].d1;
        misc.warp[ring].scr[index] = xtoi(buf);
    }
    
    if(ret==15)
    {
        Map.dowarp2(ring,index);
        refresh(rALL);
    }

	delete[] warp_cpy;
    
    warp_dlg[5].x = tempx5;
    warp_dlg[6].x = tempx6;
    warp_dlg[10].x = tempx10;
    warp_dlg[11].x = tempx11;
    warp_dlg[12].x = tempx12;
    warp_dlg[13].x = tempx13;
    
    for(int m=17; m<100; m++)
    {
        warp_dlg[m].x=tempx[m-17];
    }
        
    for(int i=0; i<4; i++)
    {
        warp_dlg[10+i].d2 = 0x80;
        warp_dlg[25+i].d2 = 0x80;
        warp_dlg[37+i].d2 = 0x80;
        warp_dlg[49+i].d2 = 0x80;
    }
    
    warp_dlg[1].dp = (void *) warp_tabs;
    warp_dlg[1].dp3 = (void *)warp_dlg;
    
}

int d_warplist_proc(int msg,DIALOG *d,int c)
{
    if(msg==MSG_DRAW)
    {
        int *xy = (int*)(d->dp3);
        int ring = curr_ring;
        int dmap = misc.warp[ring].dmap[d->d1];
        float temp_scale = 1;
        
        if(is_large())
        {
            temp_scale = 1.5; // Scale up by 1.5
        }
        
        drawdmap(dmap);
        
        if(xy[0]||xy[1])
        {
            int x = d->x+int((xy[0]-2)*temp_scale);
            int y = d->y+int((xy[1]-2)*temp_scale);
//      int w = is_large ? 84 : 71;
//      int h = is_large ? 52 : 39;
            int w = 84;
            int h = 52;
            jwin_draw_frame(screen,x,y,w,h,FR_DEEP);
            drawdmap_screen(x+2,y+2,w-4,h-4,dmap);
        }
        
        if(xy[2]||xy[3])
        {
            textprintf_ex(screen,font,d->x+int(xy[2]*temp_scale),d->y+int(xy[3]*temp_scale),jwin_pal[jcBOXFG],jwin_pal[jcBOX],"Map: %d ",DMaps[dmap].map+1);
        }
        
        if(xy[4]||xy[5])
        {
            textprintf_ex(screen,font,d->x+int(xy[4]*temp_scale),d->y+int(xy[5]*temp_scale),jwin_pal[jcBOXFG],jwin_pal[jcBOX],"Level:%2d ",DMaps[dmap].level);
        }
        
        if(xy[6]||xy[7])
        {
            textprintf_ex(screen,font,d->x+int(xy[6]*temp_scale),d->y+int(xy[7]*temp_scale),jwin_pal[jcBOXFG],jwin_pal[jcBOX],"Scr: 0x%02X ",misc.warp[ring].scr[d->d1]);
        }
    }
    
    return jwin_list_proc(msg,d,c);
}

int d_wclist_proc(int msg,DIALOG *d,int c)
{
    int d1 = d->d1;
    int ret = jwin_droplist_proc(msg,d,c);
    misc.warp[curr_ring].size=d->d1+3;
    
    if(d->d1 != d1)
        return D_CLOSE;
        
    return ret;
}

const char *wclist(int index, int *list_size)
{
    static char buf[2];
    
    if(index>=0)
    {
        if(index>6)
            index=6;
            
        sprintf(buf,"%d",index+3);
        return buf;
    }
    
    *list_size=7;
    return NULL;
}

//int warpringdmapxy[8] = {160,116,160,90,160,102,160,154};
int warpringdmapxy[8] = {80,26,80,0,80,12,80,78};

static ListData number_list(numberlist, &font);
static ListData wc_list(wclist, &font);

static DIALOG warpring_dlg[] =
{
    // (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp)
    { jwin_win_proc,          0,      0,    193,    166,    vc(14),                 vc(1),                   0,    D_EXIT,     0,          0,  NULL,                    NULL,   NULL            },
    { d_timer_proc,           0,      0,      0,      0,    0,                      0,                       0,    0,          0,          0,  NULL,                    NULL,   NULL            },
    { jwin_text_proc,        16,     33,     48,      8,    vc(14),                 vc(1),                   0,    0,          0,          0, (void *) "Count:",       NULL,   NULL            },
    { d_wclist_proc,         72,     29,     48,     16,    jwin_pal[jcTEXTFG],     jwin_pal[jcTEXTBG],      0,    0,          1,          0, (void *) &wc_list,       NULL,   NULL            },
    // 4
    { d_warplist_proc,       16,     50,     65,     71,    jwin_pal[jcTEXTFG],     jwin_pal[jcTEXTBG],      0,    D_EXIT,     0,          0, (void *) &number_list,   NULL,   warpringdmapxy  },
    { jwin_button_proc,      26,    140,     61,     21,    vc(14),                 vc(1),                  13,    D_EXIT,     0,          0, (void *) "Edit",         NULL,   NULL            },
    { jwin_button_proc,     106,    140,     61,     21,    vc(14),                 vc(1),                  27,    D_EXIT,     0,          0, (void *) "Done",         NULL,   NULL            },
    { d_keyboard_proc,        0,      0,      0,      0,    0,                      0,                       0,    0,          KEY_F1,     0, (void *) onHelp,         NULL,   NULL            },
    { NULL,                   0,      0,      0,      0,    0,                      0,                       0,    0,          0,          0,  NULL,                    NULL,   NULL            }
};

int select_warp()
{
    misc.warp[curr_ring].size = vbound(misc.warp[curr_ring].size,3,9);
    number_list_zero = false;
    
    int ret=4;

	DIALOG *warpring_cpy = resizeDialog(warpring_dlg, 1.5);
    
    do
    {
        number_list_size = misc.warp[curr_ring].size;
		warpring_cpy[3].d1 = misc.warp[curr_ring].size-3;
        ret = zc_popup_dialog(warpring_cpy,ret);
    }
    while(ret==3);
    
    if(ret==6 || ret==0)
    {
		delete[] warpring_cpy;
        return -1;
    }

	int retval = warpring_cpy[4].d1;
	delete[] warpring_cpy;

    return retval;
}

void EditWarpRing(int ring)
{
    char buf[40];
    sprintf(buf,"Ring %d Warps",ring);
    warpring_dlg[0].dp = buf;
    warpring_dlg[0].dp2 = lfont;
    curr_ring = ring;
    
    int index = select_warp();
    
    while(index!=-1)
    {
        EditWarpRingScr(ring,index);
        index = select_warp();
    }
}

int onWarpRings()
{
    number_list_size = 9;
    number_list_zero = true;
    
    int index = select_data("Warp Rings",0,numberlist,"Edit","Done",lfont);
    
    while(index!=-1)
    {
        EditWarpRing(index);
        number_list_size = 9;
        number_list_zero = true;
        index = select_data("Warp Rings",index,numberlist,"Edit","Done",lfont);
    }
    
    return D_O_K;
}

/********************************/
/********** onEnemies ***********/
/********************************/


const char *pattern_list(int index, int *list_size)
{

    if(index<0)
    {
        *list_size = MAXPATTERNS;
        return NULL;
    }
    
    return pattern_string[index];
}

static ListData pattern_dlg_list(pattern_list, &font);

static DIALOG pattern_dlg[] =
{
    /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp) */
    { jwin_win_proc, 72,   56,   176+1,  164+1,   vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "Enemy Pattern", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { jwin_list_proc,       80,   124,   160+1,  58,   jwin_pal[jcTEXTFG],jwin_pal[jcTEXTBG],  0,       D_EXIT,     0,             0, (void *) &pattern_dlg_list, NULL, NULL },
    // 3
    { jwin_button_proc,     90,   190,  61,   21,   vc(14),  vc(1),  'k',     D_EXIT,     0,             0, (void *) "O&K", NULL, NULL },
    { jwin_button_proc,     170,  190,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { jwin_text_proc,     90,  78,  61,   10,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Classic: pseudorandom locations near", NULL, NULL },
    { jwin_text_proc,     90,  88,  61,   10,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "the middle of the screen.", NULL, NULL },
    { jwin_text_proc,     90,  102,  61,   10,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Random: any available location", NULL, NULL },
    { jwin_text_proc,     90,  112,  61,   10,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "at a sufficient distance from Link.", NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

int onPattern()
{
    byte p=Map.CurrScr()->pattern;
    pattern_dlg[0].dp2 = lfont;
    pattern_dlg[2].d1  = p;

	DIALOG *pattern_cpy = resizeDialog(pattern_dlg, 1.5);
    
    if(zc_popup_dialog(pattern_cpy,2) < 4)
    {
        saved=false;
        Map.CurrScr()->pattern = pattern_cpy[2].d1;
    }

	delete[] pattern_cpy;
    
    refresh(rMENU);
    return D_O_K;
}

// Now just calls onScrData()
int onEnemyFlags()
{
    int i=-1;
    
    while(scrdata_tabs[++i].text != NULL)
        scrdata_tabs[i].flags = (i==2 ? D_SELECTED : 0);
        
    onScrData();
    return D_O_K;
}

const char *enemy_viewer(int index, int *list_size)
{
    if(index<0)
    {
        *list_size=10;
        
        return NULL;
    }
    
    int guy=Map.CurrScr()->enemy[index];
    return guy>=eOCTO1S ? guy_string[guy] : (char *) "(None)";
}

enemy_struct bie[eMAXGUYS];
enemy_struct ce[100];
int enemy_type=0,bie_cnt=-1,ce_cnt;

enemy_struct big[zqMAXGUYS];
enemy_struct cg[100];
int guy_type=0,big_cnt=-1,cg_cnt;

void build_bie_list(bool hide)
{
    bie[0].s = (char *)"(None)";
    bie[0].i = 0;
    bie_cnt=1;
    
    for(int i=eOCTO1S; i<eMAXGUYS; i++)
    {
        if(i >= OLDMAXGUYS || old_guy_string[i][strlen(old_guy_string[i])-1]!=' ' || !hide)
        {
            bie[bie_cnt].s = (char *)guy_string[i];
            bie[bie_cnt].i = i;
            ++bie_cnt;
        }
    }
    
    for(int i=0; i<bie_cnt-1; i++)
    {
        for(int j=i+1; j<bie_cnt; j++)
        {
            if(strcmp(bie[i].s,bie[j].s)>0)
            {
                zc_swap(bie[i],bie[j]);
            }
        }
    }
}

void build_big_list(bool hide)
{
    big[0].s = (char *)"(None)";
    big[0].i = 0;
    big_cnt=1;
    
    for(int i=gABEI; i<gDUMMY1; i++)
    {
        if(guy_string[i][strlen(guy_string[i])-1]!=' ' || !hide)
        {
            big[big_cnt].s = (char *)guy_string[i];
            big[big_cnt].i = i;
            ++big_cnt;
        }
    }
    
    for(int i=0; i<big_cnt-1; i++)
    {
        for(int j=i+1; j<big_cnt; j++)
        {
            if(strcmp(big[i].s,big[j].s)>0)
            {
                zc_swap(big[i],big[j]);
            }
        }
    }
}

const char *enemylist(int index, int *list_size)
{
    if(index<0)
    {
        *list_size = enemy_type ? ce_cnt : bie_cnt;
        return NULL;
    }
    
    return enemy_type ? ce[index].s : bie[index].s;
}

const char *guylist(int index, int *list_size)
{
    if(index<0)
    {
        *list_size = guy_type ? cg_cnt : big_cnt;
        return NULL;
    }
    
    return guy_type ? cg[index].s : big[index].s;
}

void elist_rclick_func(int index, int x, int y);
DIALOG elist_dlg[] =
{
    /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp) */
    { jwin_win_proc,     50,   40,   200+24+24,  145,  vc(14),  vc(1),  0,       D_EXIT,          0,             0,       NULL, NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { d_enelist_proc,    62,   68,   188,  88,   jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,       D_EXIT,     0,             0,       NULL, NULL, NULL },
    { jwin_button_proc,     90,   160,  61,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "Edit", NULL, NULL },
    { jwin_button_proc,     170,  160,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Done", NULL, NULL },
    { jwin_button_proc,     220,   160,  61,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "Edit", NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

static DIALOG glist_dlg[] =
{
    /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp) */
    { jwin_win_proc,     50,   40,   220,  145,  vc(14),  vc(1),  0,       D_EXIT,          0,             0,       NULL, NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { jwin_abclist_proc,    62,   68,   196,  88,   jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,       D_EXIT,     0,             0,       NULL, NULL, NULL },
    { jwin_button_proc,     70,   160,  51,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "OK", NULL, NULL },
    { jwin_button_proc,     190,  160,  51,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { jwin_button_proc,     130,  160,  51,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Help", NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

int efrontfacingtile(int id)
{
    int anim = get_bit(quest_rules,qr_NEWENEMYTILES)?guysbuf[id].e_anim:guysbuf[id].anim;
    int usetile = 0;
    
    switch(anim)
    {
    case aAQUA:
        if(!(get_bit(quest_rules,qr_NEWENEMYTILES) && guysbuf[id].misc1))
            break;
            
    case aWALLM:
    case aGHOMA:
        usetile=1;
        break;
        
        //Fallthrough
    case a2FRM4DIR:
    case aWALK:
        usetile=2;
        break;
        
    case aLEV:
    case a3FRM4DIR:
        usetile=3;
        break;
        
    case aLANM:
        usetile = !(get_bit(quest_rules,qr_NEWENEMYTILES))?0:4;
        break;
        
    case aNEWDONGO:
    case a4FRM8EYE:
    case aNEWWIZZ:
    case aARMOS4:
    case aNEWTEK:
    case aNEWWALLM:
    case a4FRM4DIRF:
    case a4FRM4DIR:
    case a4FRM8DIRF:
    case a4FRMPOS8DIR:
    case a4FRMPOS8DIRF:
    case a4FRMPOS4DIR:
    case a4FRMPOS4DIRF:
        usetile=4;
        break;
        
    case aDONGO:
        usetile=6;
        break;
        
    case aDONGOBS:
        usetile=24;
        break;
        
    case aNEWLEV:
        usetile=40;
        break;
        
    case aNEWZORA:
        if(guysbuf[id].family==eeZORA)
            usetile=44;
            
        break;
        
    case aGLEEOK:
        if(!get_bit(quest_rules,qr_NEWENEMYTILES))
            usetile = (guysbuf[id].s_tile - guysbuf[id].tile)+1;
        else
            usetile = (guysbuf[id].misc8);
            
        break;
    }
    
    return zc_max(get_bit(quest_rules, qr_NEWENEMYTILES) ? -guysbuf[id].e_tile
                  : -guysbuf[id].tile, usetile);
}

static ListData enemy_dlg_list(enemy_viewer, &font);

int enelist_proc(int msg,DIALOG *d,int c,bool use_abc_list)
{
    int ret;
    
    /* copy/paste enemy dialog bug. -Don't change this unless you test it first! -Gleeok */
    if(use_abc_list)
        ret= jwin_abclist_proc(msg,d,c); // This one's better for the full list
    else
        ret= jwin_list_proc(msg,d,c);
        
    if(msg==MSG_DRAW||msg==MSG_CHAR)
    {
        int id;
        
        // Conveniently hacking the Select Enemy and Screen Enemy dialogs together -L
        if(d->dp == &enemy_dlg_list)
        {
            id = Map.CurrScr()->enemy[d->d1];
        }
        else
        {
            id = bie[d->d1].i;
        }
        
        int tile = get_bit(quest_rules, qr_NEWENEMYTILES) ? guysbuf[id].e_tile
                   : guysbuf[id].tile;
        int cset = guysbuf[id].cset;
        int x = d->x + int(195 * (is_large() ? 1.5:1));
        int y = d->y + int(2 * (is_large() ? 1.5:1));
        int w = 20;
        int h = 20;
        
        if(is_large())
        {
            w = 36;
            h = 36;
        }
        
        BITMAP *buf = create_bitmap_ex(8,20,20);
        BITMAP *bigbmp = create_bitmap_ex(8,w,h);
        
        if(buf && bigbmp)
        {
            clear_bitmap(buf);
            
            if(tile)
                overtile16(buf, tile+efrontfacingtile(id),2,2,cset,0);
                
            stretch_blit(buf, bigbmp, 2,2, 17, 17, 2, 2,w-2, h-2);
            destroy_bitmap(buf);
            jwin_draw_frame(bigbmp,0,0,w,h,FR_DEEP);
            blit(bigbmp,screen,0,0,x,y,w,h);
            destroy_bitmap(bigbmp);
        }
        
        /*
            rectfill(screen, x, y+20*(is_large?2:1), x+int(w*(is_large?1.5:1))-1, y+32*(is_large?2:1)-1, vc(4));
        */
        textprintf_ex(screen,is_large()?font:spfont,x,y+20*(is_large()?2:1),jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"#%d   ",id);
        
        textprintf_ex(screen,is_large()?font:spfont,x,y+26*(is_large()?2:1),jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"HP :");
        textprintf_ex(screen,is_large()?font:spfont,x+int(14*(is_large()?1.5:1)),y+26*(is_large()?2:1),jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"%d   ",guysbuf[id].hp);
        
        textprintf_ex(screen,is_large()?font:spfont,x,y+32*(is_large()?2:1),jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"Dmg:");
        textprintf_ex(screen,is_large()?font:spfont,x+int(14*(is_large()?1.5:1)),y+32*(is_large()?2:1),jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"%d   ",guysbuf[id].dp);
    }
    
    return ret;
}

int select_enemy(const char *prompt,int enemy,bool hide,bool is_editor,int &exit_status)
{
    //if(bie_cnt==-1)
    {
        build_bie_list(hide);
    }
    int index=0;
    
    for(int j=0; j<bie_cnt; j++)
    {
        if(bie[j].i == enemy)
        {
            index=j;
        }
    }
    
    elist_dlg[0].dp=(void *)prompt;
    elist_dlg[0].dp2=lfont;
    elist_dlg[2].d1=index;
    ListData enemy_list(enemylist, &font);
    elist_dlg[2].dp=(void *) &enemy_list;

	DIALOG *elist_cpy = resizeDialog(elist_dlg, 1.5);
    
	if(is_editor)
    {
		elist_cpy[2].dp3 = (void *)&elist_rclick_func;
		elist_cpy[2].flags|=(D_USER<<1);
		elist_cpy[3].dp = (void *)"Edit";
		elist_cpy[4].dp = (void *)"Done";
		elist_cpy[3].x = is_large()?285:90;
		elist_cpy[4].x = is_large()?405:170;
		elist_cpy[5].flags |= D_HIDDEN;
    }
    else
    {
		elist_cpy[2].dp3 = NULL;
		elist_cpy[2].flags&=~(D_USER<<1);
		elist_cpy[3].dp = (void *)"OK";
		elist_cpy[4].dp = (void *)"Cancel";
		elist_cpy[3].x = is_large()?240:60;
		elist_cpy[4].x = is_large()?350:135;
		elist_cpy[5].flags &= ~D_HIDDEN;
    }
    
    exit_status=zc_popup_dialog(elist_cpy,2);
    
    if(exit_status==0||exit_status==4)
    {
		delete[] elist_cpy;
        return -1;
    }
    
    index = elist_cpy[2].d1;
	delete[] elist_cpy;
    return bie[index].i;
}

int select_guy(const char *prompt,int guy)
{
    //  if(bie_cnt==-1)
    {
        build_big_list(true);
    }
    
    int index=0;
    
    for(int j=0; j<big_cnt; j++)
    {
        if(big[j].i == guy)
        {
            index=j;
        }
    }
    
    glist_dlg[0].dp=(void *)prompt;
    glist_dlg[0].dp2=lfont;
    glist_dlg[2].d1=index;
    ListData guy_list(guylist, &font);
    glist_dlg[2].dp=(void *) &guy_list;

	DIALOG *glist_cpy = resizeDialog(glist_dlg, 1.5);
    
    int ret;
    
    do
    {
        ret=zc_popup_dialog(glist_cpy,2);
        
        if(ret==5)
        {
            int id = big[glist_cpy[2].d1].i;
            
            switch(id)
            {
            case gABEI:
                jwin_alert(old_guy_string[id],"The old man. Uses tile 84.",NULL,NULL,"O&K",NULL,'k',0,lfont);
                break;
                
            case gAMA:
                jwin_alert(old_guy_string[id],"The old woman. Uses tile 85.",NULL,NULL,"O&K",NULL,'k',0,lfont);
                break;
                
            case gDUDE:
                jwin_alert(old_guy_string[id],"The shopkeeper. Uses tile 86.",NULL,NULL,"O&K",NULL,'k',0,lfont);
                break;
                
            case gMOBLIN:
                jwin_alert(old_guy_string[id],"The generous Moblin. Uses tile 116.",NULL,NULL,"O&K",NULL,'k',0,lfont);
                break;
                
            case gGORIYA:
                jwin_alert(old_guy_string[id],"The hungry Goriya. Uses tile 132.","He isn't entirely necessary to make","use of the 'Feed the Goriya' Room Type.","O&K",NULL,'k',0,lfont);
                break;
                
            case gFIRE:
                jwin_alert(old_guy_string[id],"A sentient flame. Uses tile 65, and","flips horizontally as it animates.",NULL,"O&K",NULL,'k',0,lfont);
                break;
                
            case gFAIRY:
                jwin_alert(old_guy_string[id],"A fairy. Uses tiles 63 and 64. Even if the","DMap uses 'Special Rooms/Guys In Caves Only'","she will still appear in regular screens.","O&K",NULL,'k',0,lfont);
                break;
                
            case gZELDA:
                jwin_alert(old_guy_string[id],"The princess. Uses tiles 35 and 36.","Approaching her won't cause the game to end.","(Unless you touch a Zelda combo flag.)","O&K",NULL,'k',0,lfont);
                break;
                
            case gABEI2:
                jwin_alert(old_guy_string[id],"A different old man. Uses tile 87.",NULL,NULL,"O&K",NULL,'k',0,lfont);
                break;
                
            case gEMPTY:
                jwin_alert(old_guy_string[id],"An invisible Guy. Uses tile 259, which is","usually empty. Use it when you just want the","String to appear without a visible Guy.","O&K",NULL,'k',0,lfont);
                break;
                
            default:
                jwin_alert("Help","Select a Guy, then click","Help to find out what it is.",NULL,"O&K",NULL,'k',0,lfont);
                break;
            }
        }
    }
    while(ret==5);
    
    if(ret==0||ret==4)
    {
		delete[] glist_cpy;
        return -1;
    }
    
    
    index = glist_cpy[2].d1;
	delete[] glist_cpy;
    return big[index].i;
}

unsigned char check[2] = { (unsigned char)'\x81',0 };

static DIALOG enemy_dlg[] =
{
    /* (dialog proc)         (x)     (y)    (w)     (h)     (fg)                    (bg)                   (key)    (flags)      (d1)        (d2)  (dp) */
    { jwin_win_proc,          0,      0,    240,    190,    vc(14),                 vc(1),                   0,       D_EXIT,     0,           0, (void *) "Enemies",          NULL,   NULL  },
    { d_timer_proc,           0,      0,      0,      0,    0,                      0,                       0,       0,          0,           0,  NULL,                        NULL,   NULL  },
    { d_enelistnoabc_proc,        14,     24,    188,     97,    jwin_pal[jcTEXTFG],     jwin_pal[jcTEXTBG],      0,       D_EXIT,     0,           0, (void *) &enemy_dlg_list,    NULL,   NULL  },
    { jwin_button_proc,      12,    130,    109,     21,    vc(14),                 vc(1),                   'e',     D_EXIT,     0,           0, (void *) "Paste &Enemies",   NULL,   NULL  },
    { d_dummy_proc,          210,    24,     20,     20,    vc(11),                 vc(1),                   0,       0,          0,           0,  NULL,                        NULL,   NULL  },
    { jwin_button_proc,     127,    130,     42,     21,    vc(14),                 vc(1),                   'f',     D_EXIT,     0,           0, (void *) "&Flags",           NULL,   NULL  },
    { jwin_button_proc,     175,    130,     53,     21,    vc(14),                 vc(1),                   'p',     D_EXIT,     0,           0, (void *) "&Pattern",         NULL,   NULL  },
    { d_keyboard_proc,        0,      0,      0,      0,    0,                      0,                       'c',     0,          0,           0, (void *) close_dlg,          NULL,   NULL  },
    { d_keyboard_proc,        0,      0,      0,      0,    0,                      0,                       'v',     0,          0,           0, (void *) close_dlg,          NULL,   NULL  },
    { d_keyboard_proc,        0,      0,      0,      0,    0,                      0,                       0,       0,          KEY_DEL,     0, (void *) close_dlg,          NULL,   NULL  },
    // 10
    { jwin_button_proc,      50,    156,     61,     21,    vc(14),                 vc(1),                  'k',       D_EXIT,     0,           0, (void *) "O&K",               NULL,   NULL  },
    { jwin_button_proc,     130,    156,     61,     21,    vc(14),                 vc(1),                  27,       D_EXIT,     0,           0, (void *) "Cancel",           NULL,   NULL  },
    { d_keyboard_proc,        0,      0,      0,      0,    0,                      0,                      27,       0,          0,           0, (void *) close_dlg,          NULL,   NULL  },
    { jwin_text_proc,         4,    208,      8,      8,    vc(14),                 vc(1),                   0,       0,          0,           0, (void *) check,              NULL,   NULL  },
    { d_keyboard_proc,        0,      0,      0,      0,    0,                      0,                       0,       0,          KEY_F1,      0, (void *) onHelp,             NULL,   NULL  },
    { NULL,                   0,      0,      0,      0,    0,                      0,                       0,       0,          0,           0,  NULL,                        NULL,   NULL  }
};

int onEnemies()
{
    word oldenemy[10];
    memcpy(oldenemy,Map.CurrScr()->enemy,10*sizeof(word));
    restore_mouse();
    char buf[24] = " ";
    int ret;
    int copy=-1;
    
    build_bie_list(true);
    
    enemy_dlg[0].dp2=lfont;
    
    if(Map.CanPaste())
    {
        enemy_dlg[3].flags=D_EXIT;
        sprintf(buf,"Past&e (from %d:%02X)",(Map.CopyScr()>>8)+1,Map.CopyScr()&255);
    }
    else
    {
        enemy_dlg[3].flags=D_DISABLED;
        sprintf(buf,"Past&e from screen");
    }
    
    enemy_dlg[3].dp=buf;
    enemy_dlg[2].d1=0;

	DIALOG *enemy_cpy = resizeDialog(enemy_dlg, 1.5);
    
    do
    {
        if(copy==-1)
        {
			enemy_cpy[13].y=Backend::graphics->virtualScreenH();
        }
        else
        {
			enemy_cpy[13].y=(int)((copy<<3)*(is_large()?1.6:1))+ enemy_cpy[2].y+4;
        }
        
        if(is_large())
        {
            // Fix d_enelist_proc
			enemy_cpy[2].dp2 = 0;
            ((ListData *)enemy_cpy[2].dp)->font = &lfont_l;
        }
        
        ret = zc_do_dialog(enemy_cpy,2);
        
        switch(ret)
        {
        case 2:
        {
            int exit_status;
            int i = enemy_cpy[2].d1;
            
            do
            {
                int enemy = Map.CurrScr()->enemy[i];
                enemy = select_enemy("Select Enemy",enemy,true,false,exit_status);
                
                if(enemy>=0)
                {
                    if(exit_status==5 && enemy > 0)
                    {
                        edit_enemydata(enemy);
                    }
                    else
                    {
                        saved=false;
                        Map.CurrScr()->enemy[i] = enemy;
                    }
                }
            }
            while(exit_status==5);
        }
        break;
        
        case 3:
            saved=false;
            Map.PasteEnemies();
            break;
            
        case 5:
            onEnemyFlags();
            break;
            
        case 6:
            onPattern();
            break;
            
        case 7:
            copy = enemy_cpy[2].d1;
            break;
            
        case 8:
            saved=false;
            
            if(copy>=0)
            {
                Map.CurrScr()->enemy[enemy_cpy[2].d1] = Map.CurrScr()->enemy[copy];
            }
            
            break;
            
        case 9:
            saved=false;
            Map.CurrScr()->enemy[enemy_cpy[2].d1] = 0;
            break;
            
        case 0:
        case 11: //cancel
            memcpy(Map.CurrScr()->enemy,oldenemy,10*sizeof(word));
            break;
            
        case 10: //ok
        {
            bool end = false;
            
            for(int i=0; i<10; i++)
            {
                if(Map.CurrScr()->enemy[i]==0)
                    end = true;
                else if(end)
                {
                    if(jwin_alert("Inactive Enemies","Enemies won't appear if they're preceded"," by '(None)' in the list! Continue?",NULL,"Yes","No",'y','n',lfont)==2)
                        ret=-1;
                        
                    break;
                }
            }
            
            break;
        }
        }
    }
    while(ret<10&&ret!=0);
	
	delete[] enemy_cpy;
    
    refresh(rALL);
    return D_O_K;
}

/*******************************/
/********** onHeader ***********/
/*******************************/

char author[65],title[65],password[32];

int d_showedit_proc(int msg,DIALOG *d,int c)
{
    int ret = jwin_edit_proc(msg,d,c);
    
    if(msg==MSG_DRAW)
    {
        (d+1)->proc(MSG_DRAW,d+1,0);
    }
    
    return ret;
}

static DIALOG header_dlg[] =
{
    /* (dialog proc)        (x)   (y)   (w)   (h)   (fg)                (bg)              (key)    (flags)       (d1)           (d2)     (dp) */
    { jwin_win_proc,        20,   16,   280,  205,  vc(14),             vc(1),            0,       D_EXIT,        0,             0, (void *) "Quest Header", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { jwin_text_proc,       26,   40,    96,    8,  jwin_pal[jcBOXFG],  jwin_pal[jcBOX],  0,       0,             0,             0, (void *) "ZQ Version:", NULL, NULL },
    { jwin_text_proc,       103,  40,    96,    8,  jwin_pal[jcBOXFG],  jwin_pal[jcBOX],  0,       D_DISABLED,    0,             0,       NULL, NULL, NULL },
    { jwin_text_proc,       26,   72-16,    96,    8,  jwin_pal[jcBOXFG],  jwin_pal[jcBOX],  0,       0,             0,             0, (void *) "Quest Number:", NULL, NULL },
    { jwin_edit_proc,       100,  68-16,    32,   16,  vc(12),             vc(1),            0,       0,             2,             0,       NULL, NULL, NULL },
    { jwin_text_proc,       26,   90-16,    96,    8,  jwin_pal[jcBOXFG],  jwin_pal[jcBOX],  0,       0,             0,             0, (void *) "Quest Ver:", NULL, NULL },
    { jwin_edit_proc,       100,  86-16,    80,   16,  vc(12),             vc(1),            0,       0,             8,             0,       NULL, NULL, NULL },
    { jwin_text_proc,       26,   108-16,   96,    8,  jwin_pal[jcBOXFG],  jwin_pal[jcBOX],  0,       0,             0,             0, (void *) "Min. Ver:", NULL, NULL },
    { jwin_edit_proc,       100,  104-16,   80,   16,  vc(12),             vc(1),            0,       0,             8,             0,       NULL, NULL, NULL },
    // 10
    { jwin_text_proc,       26,   126-16,   96,    8,  jwin_pal[jcBOXFG],  jwin_pal[jcBOX],  0,       0,             0,             0, (void *) "Title:", NULL, NULL },
    { d_showedit_proc,      66,   122-16,   80,   16,  vc(12),             vc(1),            0,       0,            64,             0,       title, NULL, NULL },
    { jwin_textbox_proc,    26,   138-16,   120,  40,  vc(11),  vc(1),  0,       0,          64,            0,       title, NULL, NULL },
    { jwin_text_proc,       174,  126-16,   96,   8,    jwin_pal[jcBOXFG],  jwin_pal[jcBOX],  0,       0,          0,             0, (void *) "Author:", NULL, NULL },
    { d_showedit_proc,      214,  122-16,   80,   16,    vc(12),  vc(1),  0,       0,          64,            0,       author, NULL, NULL },
    { jwin_textbox_proc,    174,  138-16,   120,  40,   vc(11),  vc(1),  0,       0,          64,            0,       author, NULL, NULL },
    // 16
    { jwin_button_proc,     110,  204-32,  101,   21,   vc(14),             vc(1),            13,      D_EXIT,        0,             0, (void *) "Change Password", NULL, NULL },
    { jwin_button_proc,     90,   204-8,  61,   21,   vc(14),             vc(1),            13,      D_EXIT,        0,             0, (void *) "OK", NULL, NULL },
    { jwin_button_proc,     170,  204-8,  61,   21,   vc(14),             vc(1),            27,      D_EXIT,        0,             0, (void *) "Cancel", NULL, NULL },
    { d_keyboard_proc,      0,    0,    0,    0,    0,                  0,                0,       0,             KEY_F1,        0, (void *) onHelp, NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

static DIALOG password_dlg[] =
{
    /* (dialog proc)        (x)   (y)   (w)   (h)   (fg)                (bg)              (key)    (flags)       (d1)           (d2)     (dp) */
    { jwin_win_proc,        0,    0,   300,  111-32,  vc(14),             vc(1),            0,       D_EXIT,        0,             0, (void *) "Set Password", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { jwin_text_proc,       6,   30,   96,    8,  jwin_pal[jcBOXFG],  jwin_pal[jcBOX],  0,       0,             0,             0, (void *) "Enter new password:", NULL, NULL },
    { jwin_edit_proc,       104,  26,   190,   16,  vc(12),             vc(1),            0,       0,             255,             0,       NULL, NULL, NULL },
    { jwin_check_proc,      6,   42,   128+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Save key file", NULL, NULL },
    { jwin_button_proc,     80,   54,  61,   21,   vc(14),             vc(1),            13,      D_EXIT,        0,             0, (void *) "OK", NULL, NULL },
    { jwin_button_proc,    160,   54,  61,   21,   vc(14),             vc(1),            27,      D_EXIT,        0,             0, (void *) "Cancel", NULL, NULL },
    { d_keyboard_proc,      0,    0,    0,    0,    0,                  0,                0,       0,             KEY_F1,        0, (void *) onHelp, NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};



int onHeader()
{
    char zver_str[11],q_num[8],version[10],minver[10];
    
    bool resize=!(get_debug()||key[KEY_LSHIFT]||key[KEY_RSHIFT]);
    
    if(resize)
    {
        for(int i=6; i<19; ++i)
        {
            header_dlg[i].y-=18;
        }
        
        header_dlg[0].h-=18;
        header_dlg[4].proc=d_dummy_proc;
        header_dlg[5].proc=d_dummy_proc;
    }
    
    jwin_center_dialog(header_dlg);
    
    sprintf(zver_str,"%d.%02X (%d)",header.zelda_version>>8,header.zelda_version&0xFF,header.build);
    sprintf(q_num,"%d",header.quest_number);
    strcpy(version,header.version);
    strcpy(minver,header.minver);
    strcpy(author,header.author);
    strcpy(title,header.title);
    
    header_dlg[0].dp2 = lfont;
    header_dlg[3].dp = zver_str;
    header_dlg[5].dp = q_num;
    header_dlg[7].dp = version;
    header_dlg[9].dp = minver;
    
    password_dlg[0].dp2=lfont;
    char pwd[256];
    memset(pwd,0,256);
    password_dlg[3].dp=pwd;
    password_dlg[4].flags=header.use_keyfile?D_SELECTED:0;

	DIALOG *header_cpy = resizeDialog(header_dlg, 1.5);
	DIALOG *password_cpy = resizeDialog(password_dlg, 1.5);
    
    int ret;
    
    bool done = false;
    while (!done)
    {
        ret = zc_popup_dialog(header_cpy, -1);

        switch (ret)
        {
        case 16:
        {
            int subret = zc_popup_dialog(password_cpy, -1);

            if (subret == 5)
            {
                header.use_keyfile = password_cpy[4].flags&D_SELECTED ? 1 : 0;
                set_questpwd(pwd, header.use_keyfile != 0);
            }
        }
        case 17:
        {
            if (strcmp(version, minver) < 0)
            {
                jwin_alert("Required Version Too Low", "Quest version must be >= required version", "or users cannot load the quest", NULL, "&OK", NULL, 13, 0, lfont);
            }
            else
            {
                saved = false;
                header.quest_number = atoi(q_num);
                strcpy(header.author, author);
                strcpy(header.title, title);
                strcpy(header.version, version);
                strcpy(header.minver, minver);
                done = true;
            }
            break;
        }
        default:
            done = true;
            break;
        }
    }               
    
    if(resize)
    {
        for(int i=6; i<19; ++i)
        {
			header_dlg[i].y+=18;
        }
        
		header_dlg[0].h+=18;
		header_dlg[4].proc=jwin_text_proc;
		header_dlg[5].proc=jwin_edit_proc;
    }
    
    jwin_center_dialog(header_cpy);
    
	delete[] header_cpy;
	delete[] password_cpy;
    return D_O_K;
}


static ZCHEATS tmpcheats;


static DIALOG cheats_dlg[] =
{
    /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp) */
    { jwin_win_proc,      32,   44,   256,  142,  vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "Cheat Codes", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    // 2
    { jwin_button_proc,     90,   160,  61,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "OK", NULL, NULL },
    { jwin_button_proc,     170,  160,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { d_keyboard_proc,       0,    0,    0,    0,    0,       0,      0,       0,          KEY_F1,        0, (void *) onHelp, NULL, NULL },
    // 5
    { jwin_check_proc,      104,  72,   0,    9,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Enable Cheats", NULL, NULL },
    { jwin_text_proc,       40,   72,   0,    8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Lvl  Code", NULL, NULL },
    { jwin_text_proc,       48,   90,   8,    8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "1", NULL, NULL },
    { jwin_text_proc,       48,   108,  8,    8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "2", NULL, NULL },
    { jwin_text_proc,       48,   126,  8,    8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "3", NULL, NULL },
    { jwin_text_proc,       48,   144,  8,    8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "4", NULL, NULL },
    // 11
    { jwin_edit_proc,       61,   86,   210,  16,    vc(12),  vc(1),  0,       0,          40,            0,       tmpcheats.codes[0], NULL, NULL },
    { jwin_edit_proc,       61,   104,  210,  16,    vc(12),  vc(1),  0,       0,          40,            0,       tmpcheats.codes[1], NULL, NULL },
    { jwin_edit_proc,       61,   122,  210,  16,    vc(12),  vc(1),  0,       0,          40,            0,       tmpcheats.codes[2], NULL, NULL },
    { jwin_edit_proc,       61,   140,  210,  16,    vc(12),  vc(1),  0,       0,          40,            0,       tmpcheats.codes[3], NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

int onCheats()
{
    cheats_dlg[0].dp2=lfont;
    tmpcheats = zcheats;
    cheats_dlg[5].flags = zcheats.flags ? D_SELECTED : 0;

	DIALOG *cheats_cpy = resizeDialog(cheats_dlg, 1.5);
        
    int ret = zc_popup_dialog(cheats_cpy, 3);
    
    if(ret == 2)
    {
        saved = false;
        zcheats = tmpcheats;
        zcheats.flags = (cheats_cpy[5].flags & D_SELECTED) ? 1 : 0;
    }
	delete[] cheats_cpy;
    return D_O_K;
}

const char *subscrtype_str[ssdtMAX+1] = { "Original","New Subscreen","Revision 2","BS Zelda Original","BS Zelda Modified","BS Zelda Enhanced","BS Zelda Complete","Zelda 3","Custom" };

const char *subscrtypelist(int index, int *list_size)
{
    if(index>=0)
    {
        bound(index,0,ssdtMAX);
        return subscrtype_str[index];
    }
    
    *list_size=ssdtMAX+1;
    return NULL;
}

static ListData subscreen_type_dlg_list(subscrtypelist, &font);

static DIALOG subscreen_type_dlg[] =
{
    /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp) */
    { jwin_win_proc,     83,   32,   154,  70,  vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "Subscreen Type", NULL, NULL },
    { jwin_button_proc,     89,  77,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "OK", NULL, NULL },
    { jwin_button_proc,     170,  77,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { jwin_droplist_proc,   107-8,  57,   106+15,  16,   jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,       0,          1,             0, (void *) &subscreen_type_dlg_list, NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

int onSubscreen()
{
    int tempsubscreen=zinit.subscreen;
    subscreen_type_dlg[0].dp2=lfont;
    subscreen_type_dlg[3].d1=zinit.subscreen;

	DIALOG *subscreen_type_cpy = resizeDialog(subscreen_type_dlg, 1.5);
    
    int ret = zc_popup_dialog(subscreen_type_cpy,2);
    
    if(ret==1)
    {
        if(subscreen_type_cpy[3].d1!=tempsubscreen)
        {
            zinit.subscreen= subscreen_type_cpy[3].d1;
            
            if(zinit.subscreen!=ssdtMAX)  //custom
            {
                if(tempsubscreen==ssdtMAX)
                {
                    if(jwin_alert("Reset Custom Subscreens","This will delete all of your custom subscreens!","Proceed?",NULL,"&OK","&Cancel",13,27,lfont)==2)
                    {
                        zinit.subscreen=ssdtMAX;
						delete[] subscreen_type_cpy;
                        return D_O_K;
                    }
                }
                
                reset_subscreens();
                setupsubscreens();
            }
            
            saved=false;
        }
    }
    
	delete[] subscreen_type_cpy;
    return D_O_K;
}

bool do_x_button(BITMAP *dest, int x, int y)
{
    bool over=false;
    
    while(Backend::mouse->anyButtonClicked())
    {
        if(isinRect(Backend::mouse->getVirtualScreenX(), Backend::mouse->getVirtualScreenY(),x,y,x+15,y+13))
        {
            if(!over)
            {
                draw_x_button(dest, x, y, D_SELECTED);
                over=true;
            }
        }
        else
        {
            if(over)
            {
                draw_x_button(dest, x, y, 0);
                over=false;
            }
        }
        
		Backend::graphics->waitTick();
		Backend::graphics->showBackBuffer();
    }
    
    return over;
}


int d_dummy_proc(int, DIALOG*, int)
{
    return D_O_K;
}

int d_maptile_proc(int msg, DIALOG *d, int)
{
    switch(msg)
    {
    case MSG_CLICK:
        if(select_tile(d->d1,d->d2,1,d->fg,true, 0, true))
            return D_REDRAW;
            
        break;
        
    case MSG_DRAW:
    {
        int dw = d->w;
        int dh = d->h;
        
        if(is_large() && d->dp2==(void*)1)
        {
            dw /= 2;
            dh /= 2;
        }
        
        BITMAP *buf = create_bitmap_ex(8,dw,dh);
        
        if(buf)
        {
            clear_bitmap(buf);
            
            for(int y=0; y<dh; y+=16)
                for(int x=0; x<dw; x+=16)
                {
                    if(d->d1)
                        puttile16(buf,d->d1+(y>>4)*20+(x>>4),x,y,d->fg,0);
                }
                
            if(is_large() && d->dp2==(void*)1)
                stretch_blit(buf,screen,0,0,dw,dh,d->x-(is_large() ? 1 : 0),d->y- (is_large() ? 1 : 0),dw*(is_large()?2:1),dh*(is_large()?2:1));
            else
                blit(buf,screen,0,0,d->x,d->y,dw,dh);
                
            destroy_bitmap(buf);
        }
    }
    }
    
    return D_O_K;
}

static int last_combo=0;
static int last_cset=0;
static combo_alias temp_aliases[MAXCOMBOALIASES];

static char comboa_str_buf[32];

int d_comboalist_proc(int msg,DIALOG *d,int c)
{
    int d1 = d->d1;
    int ret = jwin_droplist_proc(msg,d,c);
    comboa_cnt = d->d1;
    
    if(d1!=d->d1)
    {
        set_comboaradio(temp_aliases[comboa_cnt].layermask);
        return D_REDRAW;
    }
    
    return ret;
}

const char *comboalist(int index, int *list_size)
{
    if(index>=0)
    {
        bound(index,0,MAXCOMBOALIASES-1);
        sprintf(comboa_str_buf,"%d",index);
        return comboa_str_buf;
    }
    
    *list_size=MAXCOMBOALIASES;
    return NULL;
}

extern int scheme[jcMAX];

int d_comboa_proc(int msg,DIALOG *d,int c)
{
    //these are here to bypass compiler warnings about unused arguments
    c=c;
    
    combo_alias *combo;
    combo = &temp_aliases[comboa_cnt];
    int position;
    int cur_layer, temp_layer;
    int lay_count=0;
    int size = is_large()? 2 : 1;
    
    int cx1=(Backend::mouse->getVirtualScreenX()-d->x-(120-(combo->width*8)));
    int cy1=(Backend::mouse->getVirtualScreenY()-d->y-(80-(combo->height*8)));
    int cx=cx1/(16*size);
    int cy=cy1/(16*size);
    
    int co,cs;
    
    
    switch(msg)
    {
    case MSG_CLICK:
        Z_message("click (%d, %d) (%d, %d)\n", cx1, cy1, cx, cy);
        
        if((cx>combo->width)||(cx1<0))
            return D_O_K;
            
        if((cy>combo->height)||(cy1<0))
            return D_O_K;
            
        for(int j=0; j<layer_cnt; j++)
        {
            if(combo->layermask&(1<<j))
                lay_count++;
        }
        
        position=(lay_count)*(combo->width+1)*(combo->height+1);
        position+=(cy*(combo->width+1))+cx;
        
        if(key[KEY_LSHIFT]||key[KEY_RSHIFT])
        {
            combo->combos[position] = 0;
            combo->csets[position] = 0;
            
            while(Backend::mouse->anyButtonClicked())
            {
				Backend::graphics->waitTick();
				Backend::graphics->showBackBuffer();
                /* do nothing */
            }
            
            return D_REDRAW;
        }
        
        co=combo->combos[position];
        cs=combo->csets[position];
        
        if((co==0)||(key[KEY_ZC_LCONTROL]))
        {
            co=last_combo;
            cs=last_cset;
        }
        
        if((select_combo_2(co,cs)))
        {
            last_combo = co;
            last_cset = cs;
            
            combo->combos[position]=co;
            combo->csets[position]=cs;
        }
        
        return D_REDRAW;
        break;
        
    case MSG_DRAW:
        BITMAP *buf = create_bitmap_ex(8,d->w,d->h);
        
        if(buf)
        {
            clear_bitmap(buf);
            
            for(int z=0; z<=comboa_lmasktotal(combo->layermask); z++)
            {
                int k=0;
                cur_layer=0;
                temp_layer=combo->layermask;
                
                while((temp_layer!=0)&&(k<z))
                {
                    if(temp_layer&1)
                    {
                        k++;
                    }
                    
                    cur_layer++;
                    temp_layer = temp_layer>>1;
                }
                
                for(int y=0; (y<d->h)&&((y/16)<=combo->height); y+=16)
                {
                    for(int x=0; (x<d->w)&&((x/16)<=combo->width); x+=16)
                    {
                        int cpos = (z*(combo->width+1)*(combo->height+1))+(((y/16)*(combo->width+1))+(x/16));
                        
                        if(combo->combos[cpos])
                        {
                            if(!((d-1)->flags&D_SELECTED)||(cur_layer==layer_cnt))
                            {
                                if(z==0)
                                {
                                    puttile16(buf,combobuf[combo->combos[cpos]].tile,x,y,combo->csets[cpos],combobuf[combo->combos[cpos]].flip);
                                }
                                else
                                {
                                    overtile16(buf,combobuf[combo->combos[cpos]].tile,x,y,combo->csets[cpos],combobuf[combo->combos[cpos]].flip);
                                }
                            }
                        }
                    }
                }
            }
            
            rectfill(screen, d->x-2,d->y-2,d->x+256+2,d->y+176+2,jwin_pal[jcBOX]);
            int dx = 120-(combo->width*8)+d->x;
            int dy = 80-(combo->height*8)+d->y;
            stretch_blit(buf,screen,0,0,(combo->width+1)*16,(combo->height+1)*16,dx,dy,(combo->width+1)*16*size,(combo->height+1)*16*size);
            //blit(buf,screen,0,0,120-(combo->width*8)+d->x,80-(combo->height*8)+d->y,(combo->width+1)*16,(combo->height+1)*16);
            (d-11)->w = (combo->width+1)*16*size+2+(is_large()?0:2);
            (d-11)->h = (combo->height+1)*16*size+2+(is_large()?0:2);
            (d-11)->x = 120-(combo->width*8)+4*size+(is_large()?2:0)+(d-14)->x;
            (d-11)->y = 80-(combo->height*8)+25*size+(is_large()?2:0)+(d-14)->y;
            object_message((d-11),MSG_DRAW,0);
            
            destroy_bitmap(buf);
        }
        
        break;
    }
    
    return D_O_K;
}

void draw_combo_alias_thumbnail(BITMAP *dest, combo_alias *combo, int x, int y, int size)
{
    if(!combo->combo)
    {
        int cur_layer, temp_layer;
        
        int cw=combo->width+1;
        int ch=combo->height+1;
        int dw=cw<<4;
        int dh=ch<<4;
        int sw=16, sh=16, sx=0, sy=0;
        
        if(cw<ch)
        {
            sw=((cw<<4)/ch);
            sx=((16-sw)>>1);
        }
        else
        {
            sh=((ch<<4)/cw);
            sy=((16-sh)>>1);
        }
        
        BITMAP *buf = create_bitmap_ex(8,dw,dh);
        BITMAP *buf2 = create_bitmap_ex(8, 16*size, 16*size);
        clear_bitmap(buf);
        clear_bitmap(buf2);
        
        if(buf&&(combo->width>0||combo->height>0||combo->combos[0]>0))
        {
            clear_bitmap(buf);
            
            for(int z=0; z<=comboa_lmasktotal(combo->layermask); z++)
            {
                int k=0;
                cur_layer=0;
                temp_layer=combo->layermask;
                
                while((temp_layer!=0)&&(k<z))
                {
                    if(temp_layer&1)
                    {
                        k++;
                    }
                    
                    cur_layer++;
                    temp_layer = temp_layer>>1;
                }
                
                for(int y2=0; (y2<dh)&&((y2>>4)<=combo->height); y2+=16)
                {
                    for(int x2=0; (x2<dw)&&((x2>>4)<=combo->width); x2+=16)
                    {
                        int cpos = (z*(combo->width+1)*(combo->height+1))+(((y2/16)*(combo->width+1))+(x2/16));
                        
                        if(combo->combos[cpos])
                        {
                            if(z==0)
                            {
                                puttile16(buf,combobuf[combo->combos[cpos]].tile,x2,y2,combo->csets[cpos],combobuf[combo->combos[cpos]].flip);
                            }
                            else
                            {
                                overtile16(buf,combobuf[combo->combos[cpos]].tile,x2,y2,combo->csets[cpos],combobuf[combo->combos[cpos]].flip);
                            }
                        }
                    }
                }
            }
            
            stretch_blit(buf, buf2, 0, 0, (cw*16), (ch*16), sx*size, sy*size, sw*size, sh*size);
            blit(buf2, dest, 0, 0, x, y, 16*size, 16*size);
        }
        else
        {
            rectfill(dest,x,y,x+16*size-1,y+16*size-1,0);
            rectfill(dest,x+3*size,y+3*size,x+12*size,y+12*size,vc(4));
        }
        
        if(buf)
            destroy_bitmap(buf);
            
        if(buf2)
            destroy_bitmap(buf2);
    }
    else
    {
        if(combobuf[combo->combo].tile>0)
        {
            put_combo(dest,x, y, combo->combo, combo->cset,0,0);
        }
        else
        {
            rectfill(dest,x,y,x+16*size-1,y+16*size-1,0);
            rectfill(dest,x+3*size,y+3*size,x+12*size,y+12*size,vc(4));
        }
    }
}

int d_comboat_proc(int msg,DIALOG *d,int)
{
    switch(msg)
    {
    case MSG_CLICK:
    {
        int c2;
        int cs;
        c2=temp_aliases[comboa_cnt].combo;
        cs=temp_aliases[comboa_cnt].cset;
        
        if(Backend::mouse->rightButtonClicked())  //right mouse button
        {
            if(c2==0&&cs==0&&!(Backend::mouse->leftButtonClicked()))
            {
                return D_O_K;
            }
            
            temp_aliases[comboa_cnt].combo=0;
            temp_aliases[comboa_cnt].cset=0;
        }
        
        if(Backend::mouse->leftButtonClicked())  //left mouse button
        {
            if(select_combo_2(c2, cs))
            {
                temp_aliases[comboa_cnt].combo=c2;
                temp_aliases[comboa_cnt].cset=cs;
            }
            
            return D_REDRAW;
        }
        else
        {
            return D_REDRAWME;
        }
    }
    break;
    
    case MSG_DRAW:
        draw_combo_alias_thumbnail(screen, &temp_aliases[comboa_cnt], d->x-(is_large() ? 1 : 0), d->y- (is_large() ? 1 : 0),(is_large() ? 2 : 1));
        break;
        
    default:
        break;
    }
    
    return D_O_K;
}

int d_comboa_radio_proc(int msg,DIALOG *d,int c);

static DIALOG orgcomboa_dlg[] =
{
    /* (dialog proc)     (x)   (y)    (w)   (h)    (fg)      (bg)     (key)    (flags)       (d1)           (d2)      (dp) */
    { jwin_win_proc,         0,    0,   200,  161,   vc(14),   vc(1),       0,     D_EXIT,       0,             0, (void *) "Organize Combo Aliases", NULL, NULL },
    { jwin_button_proc,     27,   130,  61,   21,   vc(14),  vc(1),  'k',     D_EXIT,     0,             0, (void *) "O&K", NULL, NULL },
    { jwin_button_proc,     112,  130,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    
    { jwin_radio_proc,		10,	   40,	33,		9,	vc(14),	 vc(1),	  0,		0,				0,			0,	(void*) "Copy", NULL, NULL },
    { jwin_text_proc,     10,   50,   33,		9,       0,       0,      0,       0,          0,             0, (void *) "", NULL, NULL },
    // { jwin_radio_proc,		10,	   50,	33,		9,	vc(14),	 vc(1),	  0,		0,				0,			0,			(void*) "Move", NULL, NULL },
    { jwin_radio_proc,		10,	   60,	33,		9,	vc(14),	 vc(1),	  0,		0,				0,			0,	(void*) "Swap", NULL, NULL },
    /* 6 */  { jwin_edit_proc,      110,   35,   32,   16,    vc(12),  vc(1),  0,       0,          4,             0,       NULL, NULL, NULL },
    { jwin_edit_proc,      110,   55,   32,   16,    vc(12),  vc(1),  0,       0,          4,             0,       NULL, NULL, NULL },
    { jwin_text_proc,     60,   40,   80,   8,       0,       0,      0,       0,          0,             0, (void *) "Source", NULL, NULL },
    { jwin_text_proc,     60,   60,   80,   8,       0,       0,      0,       0,          0,             0, (void *) "Dest", NULL, NULL},
    { jwin_radio_proc,		10,	   80,		60,		9,	vc(14),	 vc(1),	  0,		0,				0,			0,	(void*) "Insert new (before source)", NULL, NULL },
    { jwin_radio_proc,		10,	   100,		60,		9,	vc(14),	 vc(1),	  0,		0,				0,			0,	(void*) "Delete source", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

static DIALOG newcomboa_dlg[] =
{
    /* (dialog proc)     (x)   (y)    (w)   (h)    (fg)      (bg)     (key)    (flags)       (d1)           (d2)      (dp) */
    { jwin_win_proc,         0,    0,   200,  161,   vc(14),   vc(1),       0,     D_EXIT,       0,             0, (void *) "Combo Alias Properties", NULL, NULL },
    { jwin_button_proc,     27,   130,  61,   21,   vc(14),  vc(1),  'k',     D_EXIT,     0,             0, (void *) "O&K", NULL, NULL },
    { jwin_button_proc,     112,  130,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { jwin_text_proc,     24,   34,   80,   8,       0,       0,      0,       0,          0,             0, (void *) "Alias Width", NULL, NULL },
    { jwin_text_proc,     24,   52,   80,   8,       0,       0,      0,       0,          0,             0, (void *) "Alias Height", NULL, NULL },
    { jwin_text_proc,     24,   70,   100,   8,       0,       0,      0,       0,          0,             0, (void *) "Layers to Draw On:", NULL, NULL },
    { jwin_edit_proc,      104,   30,   28-6,   16,    vc(12),  vc(1),  0,       0,          2,             0,       NULL, NULL, NULL },
    { jwin_edit_proc,      122,   48,   28-6,   16,    vc(12),  vc(1),  0,       0,          2,             0,       NULL, NULL, NULL },
    { jwin_check_proc,     24,   86,   24,   9,    vc(12),  vc(1),  0,       0,          1,             0, (void *) "1", NULL, NULL },
    { jwin_check_proc,     50,   86,   24,   9,    vc(12),  vc(1),  0,       0,          1,             0, (void *) "2", NULL, NULL },
    { jwin_check_proc,     76,   86,   24,   9,    vc(12),  vc(1),  0,       0,          1,             0, (void *) "3", NULL, NULL },
    { jwin_check_proc,     102,   86,   24,   9,    vc(12),  vc(1),  0,       0,          1,             0, (void *) "4", NULL, NULL },
    { jwin_check_proc,     128,   86,   24,   9,    vc(12),  vc(1),  0,       0,          1,             0, (void *) "5", NULL, NULL },
    { jwin_check_proc,     154,   86,   24,   9,    vc(12),  vc(1),  0,       0,          1,             0, (void *) "6", NULL, NULL },
    
    
    // { jwin_text_proc,     24,   106,   80,   8,       0,       0,      0,       0,          0,             0,       (void *) "Copy to :", NULL, NULL },
    //15
    // { jwin_edit_proc,      100,   100,   28-6,   16,    vc(12),  vc(1),  0,       0,          2,             0,       NULL, NULL, NULL },
    // { jwin_check_proc,     84,   106,   24,   9,    vc(12),  vc(1),  0,       0,          1,             0,       (void *) "", NULL, NULL },
    
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

bool swapComboAlias(int source, int dest)
{
    if(source==dest) return false;
    
    combo_alias *combo=&temp_aliases[source], *oldcombo=&temp_aliases[dest];
    
    byte w=oldcombo->width;
    oldcombo->width=combo->width;
    combo->width=w;
    
    byte h=oldcombo->height;
    oldcombo->height=combo->height;
    combo->height=h;
    
    byte l=oldcombo->layermask;
    oldcombo->layermask=combo->layermask;
    combo->layermask=l;
    
    word c=oldcombo->combo;
    oldcombo->combo=combo->combo;
    combo->combo=c;
    
    c=oldcombo->cset;
    oldcombo->cset=combo->cset;
    combo->cset=c;
    
    word* cp = oldcombo->combos;
    oldcombo->combos=combo->combos;
    combo->combos = cp;
    
    byte *sp = oldcombo->csets;
    oldcombo->csets=combo->csets;
    combo->csets=sp;
    
    return true;
}


bool copyComboAlias(int source, int dest)
{
    if(source==dest) return false;
    
    // al_trace("count is %i\n", comboa_cnt);
    // if (dest > comboa_cnt-1) return false;
    // al_trace("Copying %i to %i\n",source, dest);
    
    combo_alias *combo, *oldcombo;
    combo = &temp_aliases[source];
    oldcombo = &temp_aliases[dest];
    
    int new_count=(comboa_lmasktotal(combo->layermask)+1)*(combo->width+1)*(combo->height+1);
    
    if(oldcombo->combos != NULL) delete[] oldcombo->combos;
    
    if(oldcombo->csets != NULL) delete[] oldcombo->csets;
    
    word *new_combos = new word[new_count];
    byte *new_csets = new byte[new_count];
    
    memcpy(new_combos, combo->combos, sizeof(word)*new_count);
    memcpy(new_csets, combo->csets, sizeof(byte)*new_count);
    
    oldcombo->combos=new_combos;
    oldcombo->csets=new_csets;
    
    oldcombo->width=combo->width;
    oldcombo->height=combo->height;
    oldcombo->layermask=combo->layermask;
    oldcombo->combo=combo->combo;
    oldcombo->cset=combo->cset;
    
    
    return true;
}

int getcurrentcomboalias();

int onOrgComboAliases()
{
    char cSrc[4];
    char cDest[4];
    int iSrc;
    int iDest;
    
    sprintf(cSrc,"0");
    sprintf(cDest,"0");
    orgcomboa_dlg[0].dp2=lfont;
    orgcomboa_dlg[6].dp= cSrc;
    orgcomboa_dlg[7].dp= cDest;

	DIALOG *orgcomboa_cpy = resizeDialog(orgcomboa_dlg, 1.5);
    
    int ret = zc_popup_dialog(orgcomboa_cpy,-1);
    
	if (ret != 1)
	{
		delete[] orgcomboa_cpy;
		return ret;
	}
    
    iSrc=atoi((char*)orgcomboa_cpy[6].dp);
    iDest=atoi((char*)orgcomboa_cpy[7].dp);
    
    if(iSrc<0 || iSrc > MAXCOMBOALIASES-1)
    {
        char buf[60];
        snprintf(buf, 60, "Invalid source (range 0-%d)", MAXCOMBOALIASES-1);
        buf[59]='\0';
        jwin_alert("Error",buf,NULL,NULL,"O&K",NULL,'k',0,lfont);
		delete[] orgcomboa_cpy;
        return ret;
    }
    
    // 10,11=ins, del
    if(orgcomboa_cpy[10].flags & D_SELECTED)  //insert
    {
        for(int j=MAXCOMBOALIASES-1; j>iSrc; --j)  copyComboAlias(j-1,j);
		delete[] orgcomboa_cpy;
        return ret;
    }
    
    if(orgcomboa_cpy[11].flags & D_SELECTED)  //delete
    {
        for(int j=iSrc; j<MAXCOMBOALIASES-1; ++j)  copyComboAlias(j+1,j);
		delete[] orgcomboa_cpy;
        return ret;
    }
    
    if(iSrc == iDest)
    {
        jwin_alert("Error","Source and dest can't be the same.",NULL,NULL,"O&K",NULL,'k',0,lfont);
		delete[] orgcomboa_cpy;
        return ret;
    }
    
    if(iDest < 0 || iDest > MAXCOMBOALIASES-1)
    {
        char buf[60];
        snprintf(buf, 60, "Invalid dest (range 0-%d)", MAXCOMBOALIASES-1);
        buf[59]='\0';
        
        jwin_alert("Error",buf,NULL,NULL,"O&K",NULL,'k',0,lfont);
		delete[] orgcomboa_cpy;
        return ret;
    }
    
    if(orgcomboa_cpy[3].flags & D_SELECTED)  //copy
    {
        copyComboAlias(iSrc,iDest);
    }
    
    if(orgcomboa_cpy[5].flags & D_SELECTED)  //swap
    {
        swapComboAlias(iSrc,iDest);
    }
    
	delete[] orgcomboa_cpy;
    
    return ret;
}

int onNewComboAlias()
{
    combo_alias *combo;
    combo = &temp_aliases[comboa_cnt];
    
    char cwidth[3];
    char cheight[3];
    // char cp[3];
    
    word temp_combos[16*11*7];
    byte temp_csets[16*11*7];
    sprintf(cwidth, "%d", combo->width+1);
    sprintf(cheight, "%d", combo->height+1);
    int old_count = (comboa_lmasktotal(combo->layermask)+1)*(combo->width+1)*(combo->height+1);
    int old_width=combo->width;
    int old_height=combo->height;
    int oldlayer=combo->layermask;
    
    for(int i=0; i<old_count; i++)
    {
        temp_csets[i] = combo->csets[i];
        temp_combos[i] = combo->combos[i];
    }
    
    newcomboa_dlg[0].dp2 = lfont;
    newcomboa_dlg[6].dp = cwidth;
    newcomboa_dlg[7].dp = cheight;
    newcomboa_dlg[8].flags = (combo->layermask&1)? D_SELECTED : 0;
    newcomboa_dlg[9].flags = (combo->layermask&2)? D_SELECTED : 0;
    newcomboa_dlg[10].flags = (combo->layermask&4)? D_SELECTED : 0;
    newcomboa_dlg[11].flags = (combo->layermask&8)? D_SELECTED : 0;
    newcomboa_dlg[12].flags = (combo->layermask&16)? D_SELECTED : 0;
    newcomboa_dlg[13].flags = (combo->layermask&32)? D_SELECTED : 0;

	DIALOG *newcomboa_cpy = resizeDialog(newcomboa_dlg, 1.5);
    
    int ret = zc_popup_dialog(newcomboa_cpy,-1);
    
    if(ret==1)
    {
        combo->width = ((atoi(cwidth)-1)<16)?zc_max(0,(atoi(cwidth)-1)):15;
        combo->height = ((atoi(cheight)-1)<11)?zc_max(0,(atoi(cheight)-1)):10;
        combo->layermask=0;
        combo->layermask |= (newcomboa_cpy[8].flags&D_SELECTED)?1:0;
        combo->layermask |= (newcomboa_cpy[9].flags&D_SELECTED)?2:0;
        combo->layermask |= (newcomboa_cpy[10].flags&D_SELECTED)?4:0;
        combo->layermask |= (newcomboa_cpy[11].flags&D_SELECTED)?8:0;
        combo->layermask |= (newcomboa_cpy[12].flags&D_SELECTED)?16:0;
        combo->layermask |= (newcomboa_cpy[13].flags&D_SELECTED)?32:0;
        
        int new_count = (comboa_lmasktotal(combo->layermask)+1)*(combo->width+1)*(combo->height+1);
        
        if(combo->combos != NULL)
        {
            delete[] combo->combos;
        }
        
        if(combo->csets != NULL)
        {
            delete[] combo->csets;
        }
        
        combo->combos = new word[new_count];
        combo->csets = new byte[new_count];
        
        int j=1;
        int old_size=(old_width+1)*(old_height+1);
        int new_start[7] =
        {
            0,
            ((combo->width+1)*(combo->height+1)*(1)),
            ((combo->width+1)*(combo->height+1)*(2)),
            ((combo->width+1)*(combo->height+1)*(3)),
            ((combo->width+1)*(combo->height+1)*(4)),
            ((combo->width+1)*(combo->height+1)*(5)),
            ((combo->width+1)*(combo->height+1)*(6))
        };
        int new_layers[6] = {0,0,0,0,0,0};
        int temp_layer = combo->layermask;
        int temp_old = oldlayer;
        int old_layers[6] = {0,0,0,0,0,0};
        int k=1;
        
        for(int i=0; (i<6)&&(temp_layer!=0); j++,temp_layer>>=1,temp_old>>=1)
        {
            if(temp_layer&1)
            {
                new_layers[i] = j;
                //if(oldlayer&(1<<(j-1))) old_layers[i] = k++;
                i++;
            }
            
            if(temp_old&1)
            {
                if(temp_layer&1)
                {
                    old_layers[i-1] = k;
                }
                
                k++;
            }
        }
        
        for(int i=0; i<new_count; i++)
        {
            if(i>=new_start[6])
            {
                //oldl=oldlayer>>(new_layers[5]-1);
                j=i-new_start[6];
                
                if(((j/(combo->width+1))<=old_height)&&((j%(combo->width+1))<=old_width)&&(oldlayer&(1<<(new_layers[5]-1))))
                {
                    combo->combos[i] = temp_combos[((j%(combo->width+1))+((old_width+1)*(j/(combo->width+1))))+(old_size*old_layers[5])];
                    combo->csets[i] = temp_csets[((j%(combo->width+1))+((old_width+1)*(j/(combo->width+1))))+(old_size*old_layers[5])];
                }
                else
                {
                    combo->combos[i] = 0;
                    combo->csets[i] = 0;
                }
            }
            else if(i>=new_start[5])
            {
                //oldl=oldlayer>>(new_layers[4]-1);
                j=i-new_start[5];
                
                if(((j/(combo->width+1))<=old_height)&&((j%(combo->width+1))<=old_width)&&(oldlayer&(1<<(new_layers[4]-1))))
                {
                    combo->combos[i] = temp_combos[((j%(combo->width+1))+((old_width+1)*(j/(combo->width+1))))+(old_size*old_layers[4])];
                    combo->csets[i] = temp_csets[((j%(combo->width+1))+((old_width+1)*(j/(combo->width+1))))+(old_size*old_layers[4])];
                }
                else
                {
                    combo->combos[i] = 0;
                    combo->csets[i] = 0;
                }
            }
            else if(i>=new_start[4])
            {
                //oldl=oldlayer>>(new_layers[3]-1);
                j=i-new_start[4];
                
                if(((j/(combo->width+1))<=old_height)&&((j%(combo->width+1))<=old_width)&&(oldlayer&(1<<(new_layers[3]-1))))
                {
                    combo->combos[i] = temp_combos[((j%(combo->width+1))+((old_width+1)*(j/(combo->width+1))))+(old_size*old_layers[3])];
                    combo->csets[i] = temp_csets[((j%(combo->width+1))+((old_width+1)*(j/(combo->width+1))))+(old_size*old_layers[3])];
                }
                else
                {
                    combo->combos[i] = 0;
                    combo->csets[i] = 0;
                }
            }
            else if(i>=new_start[3])
            {
                //oldl=oldlayer>>(new_layers[2]-1);
                j=i-new_start[3];
                
                if(((j/(combo->width+1))<=old_height)&&((j%(combo->width+1))<=old_width)&&(oldlayer&(1<<(new_layers[2]-1))))
                {
                    combo->combos[i] = temp_combos[((j%(combo->width+1))+((old_width+1)*(j/(combo->width+1))))+(old_size*old_layers[2])];
                    combo->csets[i] = temp_csets[((j%(combo->width+1))+((old_width+1)*(j/(combo->width+1))))+(old_size*old_layers[2])];
                }
                else
                {
                    combo->combos[i] = 0;
                    combo->csets[i] = 0;
                }
            }
            else if(i>=new_start[2])
            {
                //oldl=oldlayer>>(new_layers[1]-1);
                j=i-new_start[2];
                
                if(((j/(combo->width+1))<=old_height)&&((j%(combo->width+1))<=old_width)&&(oldlayer&(1<<(new_layers[1]-1))))
                {
                    combo->combos[i] = temp_combos[((j%(combo->width+1))+((old_width+1)*(j/(combo->width+1))))+(old_size*old_layers[1])];
                    combo->csets[i] = temp_csets[((j%(combo->width+1))+((old_width+1)*(j/(combo->width+1))))+(old_size*old_layers[1])];
                }
                else
                {
                    combo->combos[i] = 0;
                    combo->csets[i] = 0;
                }
            }
            else if(i>=new_start[1])
            {
                //oldl=oldlayer>>(new_layers[0]-1);
                j=i-new_start[1];
                
                if(((j/(combo->width+1))<=old_height)&&((j%(combo->width+1))<=old_width)&&(oldlayer&(1<<(new_layers[0]-1))))
                {
                    combo->combos[i] = temp_combos[((j%(combo->width+1))+((old_width+1)*(j/(combo->width+1))))+(old_size*old_layers[0])];
                    combo->csets[i] = temp_csets[((j%(combo->width+1))+((old_width+1)*(j/(combo->width+1))))+(old_size*old_layers[0])];
                }
                else
                {
                    combo->combos[i] = 0;
                    combo->csets[i] = 0;
                }
            }
            else if(i>=new_start[0])
            {
                if(((i/(combo->width+1))<=old_height)&&((i%(combo->width+1))<=old_width))
                {
                    combo->combos[i] = temp_combos[(i%(combo->width+1))+((old_width+1)*(i/(combo->width+1)))];
                    combo->csets[i] = temp_csets[(i%(combo->width+1))+((old_width+1)*(i/(combo->width+1)))];
                }
                else
                {
                    combo->combos[i] = 0;
                    combo->csets[i] = 0;
                }
            }
        }
        
        set_comboaradio(combo->layermask);
        // copy aliases
        /*if (newcomboa_dlg[16].flags)
        {
          copyComboAlias(getcurrentcomboalias(),atoi((char*) newcomboa_dlg[15].dp));
          al_trace("src: %i, dest: %i\n", getcurrentcomboalias(),atoi((char*) newcomboa_dlg[15].dp));
        }*/
    }

	delete[] newcomboa_cpy;
    
    return ret;
}

int d_orgcomboa_proc(int msg, DIALOG *d, int c)
{
    //these are here to bypass compiler warnings about unused arguments
    c=c;
    
    int down=0;
    int selected=(d->flags&D_SELECTED)?1:0;
    int last_draw;
    
    switch(msg)
    {
    
    case MSG_DRAW:
    {
        FONT *tfont=font;
        font=is_large()?lfont_l:nfont;
        jwin_draw_text_button(screen, d->x, d->y, d->w, d->h, (char*)d->dp, d->flags, true);
        font=tfont;
    }
    break;
    
    case MSG_WANTFOCUS:
        return D_WANTFOCUS;
        
    case MSG_KEY:
        /* close dialog? */
        onOrgComboAliases();
        return D_REDRAW;
       
    case MSG_CLICK:
        last_draw = 0;
        
        /* track the mouse until it is released */
        while(Backend::mouse->anyButtonClicked())
        {
            down = mouse_in_rect(d->x, d->y, d->w, d->h);
            
            /* redraw? */
            if(last_draw != down)
            {
                if(down != selected)
                    d->flags |= D_SELECTED;
                else
                    d->flags &= ~D_SELECTED;
                    
                object_message(d, MSG_DRAW, 0);
                last_draw = down;
            }
            
            /* let other objects continue to animate */
            broadcast_dialog_message(MSG_IDLE, 0);
        }
        
        /* redraw in normal state */
        if(down)
        {
            if(d->flags&D_EXIT)
            {
                d->flags &= ~D_SELECTED;
                object_message(d, MSG_DRAW, 0);
            }
        }
        
        /* should we close the dialog? */
        if(down)
        {
            onOrgComboAliases();
            return D_REDRAW;
        }
        
        break;
    }
    
    return D_O_K;
}

int d_comboabutton_proc(int msg, DIALOG *d, int c)
{
    //these are here to bypass compiler warnings about unused arguments
    c=c;
    
    int down=0;
    int selected=(d->flags&D_SELECTED)?1:0;
    int last_draw;
    
    switch(msg)
    {
    
    case MSG_DRAW:
    {
        FONT *tfont=font;
        font=is_large()?lfont_l:nfont;
        jwin_draw_text_button(screen, d->x, d->y, d->w, d->h, (char*)d->dp, d->flags, true);
        font=tfont;
    }
    break;
    
    case MSG_WANTFOCUS:
        return D_WANTFOCUS;
        
    case MSG_KEY:
        /* close dialog? */
        onNewComboAlias();
        return D_REDRAW;
        
        
    case MSG_CLICK:
        last_draw = 0;
        
        /* track the mouse until it is released */
        while(Backend::mouse->anyButtonClicked())
        {
            down = mouse_in_rect(d->x, d->y, d->w, d->h);
            
            /* redraw? */
            if(last_draw != down)
            {
                if(down != selected)
                    d->flags |= D_SELECTED;
                else
                    d->flags &= ~D_SELECTED;
                    
                object_message(d, MSG_DRAW, 0);
                last_draw = down;
            }
            
            /* let other objects continue to animate */
            broadcast_dialog_message(MSG_IDLE, 0);
        }
        
        /* redraw in normal state */
        if(down)
        {
            if(d->flags&D_EXIT)
            {
                d->flags &= ~D_SELECTED;
                object_message(d, MSG_DRAW, 0);
            }
        }
        
        /* should we close the dialog? */
        if(down)
        {
            onNewComboAlias();
            return D_REDRAW;
        }
        
        break;
    }
    
    return D_O_K;
}

int d_comboacheck_proc(int msg, DIALOG *d, int c)
{
    int temp = d->flags&D_SELECTED;
    int ret=jwin_checkfont_proc(msg,d,c);
    
    if(temp != (d->flags&D_SELECTED))
    {
        return D_REDRAW;
    }
    
    return ret;
}

static ListData comboa_list(comboalist, &font);

static DIALOG editcomboa_dlg[] =
{
    /* (dialog proc)     (x)   (y)    (w)   (h)   (fg)      (bg)     (key)    (flags)       (d1)           (d2)      (dp) */
    { jwin_win_proc,        0,    0,  320,  240,  vc(14),   vc(1),      0,       D_EXIT,     0,             0, (void *) "Combo Alias Edit", NULL, NULL },
    { jwin_button_proc,     148,  212,  61,   21,   vc(14),  vc(1),  'k',     D_EXIT,     0,             0, (void *) "O&K", NULL, NULL },
    { jwin_button_proc,     232,  212,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { jwin_frame_proc,      4+121,   28+81,   1,   1,       0,       0,      0,       0,          FR_DEEP,       0,       NULL, NULL, NULL },
    { d_comboabutton_proc,   25,  212,  81,   21,   vc(14),  vc(1),  'p',     D_EXIT,     0,             0, (void *) "&Properties", NULL, NULL },
    { d_comboalist_proc,    266,   25,   50,  16,   jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,       0,     0,             0, (void *) &comboa_list, NULL, NULL },
    { d_comboa_radio_proc,  285,   44,  30,   8+1,    vc(14),  vc(1),  0,       D_SELECTED,          0,             0, (void *) "0", NULL, NULL },
    { d_comboa_radio_proc,  285,   54,  30,   8+1,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "1", NULL, NULL },
    { d_comboa_radio_proc,  285,   64,  30,   8+1,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "2", NULL, NULL },
    { d_comboa_radio_proc,  285,   74,  30,   8+1,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "3", NULL, NULL },
    { d_comboa_radio_proc,  285,   84,   30,   8+1,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "4", NULL, NULL },
    
    { d_comboa_radio_proc,  285,   94,   30,   8+1,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "5", NULL, NULL },
    { d_comboa_radio_proc,  285,   104,  30,   8+1,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "6", NULL, NULL },
    { d_comboacheck_proc,     285,   164,   17,   9,    vc(12),  vc(1),  0,       0,          1,             0,      NULL, NULL, NULL },
    { d_comboa_proc,         6,    27,  256,  176,  0,   0,      0,       0,     0,             0,       NULL, NULL, NULL },
    { jwin_ctext_proc,     290,   176,   27,   8,   0,        0,       0,      0,        0,              0, (void *) "Only Show", NULL, NULL },
    { jwin_ctext_proc,     290,   186,   27,   8,   0,        0,       0,      0,        0,              0, (void *) "Current", NULL, NULL },
    { jwin_ctext_proc,     290,   196,   27,   8,   0,        0,       0,      0,        0,              0, (void *) "Layer", NULL, NULL },
    { jwin_ctext_proc,     290,   122,   27,   8,   0,        0,       0,      0,        0,              0, (void *) "Thumbnail", NULL, NULL },
    { jwin_frame_proc,     280,   132,   20,   20,  0,        0,      0,       0,         FR_DEEP,       0,       NULL, NULL, NULL },
    { d_comboat_proc,      282,   134,   16,   16,  0,   0,      0,       0,     0,             0,       NULL, NULL, NULL },
    
    //21
    { d_orgcomboa_proc,   106,  212,  21,   21,   vc(14),  vc(1),  'p',     D_EXIT,     0,             0, (void *) "&Org", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

int getcurrentcomboalias()
{
    return editcomboa_dlg[5].d1;
}

int d_comboa_radio_proc(int msg,DIALOG *d,int c)
{
    int temp = layer_cnt;
    int ret = jwin_radiofont_proc(msg,d,c);
    
    if(editcomboa_dlg[6].flags&D_SELECTED) layer_cnt=0;
    else if(editcomboa_dlg[7].flags&D_SELECTED) layer_cnt=1;
    else if(editcomboa_dlg[8].flags&D_SELECTED) layer_cnt=2;
    else if(editcomboa_dlg[9].flags&D_SELECTED) layer_cnt=3;
    else if(editcomboa_dlg[10].flags&D_SELECTED) layer_cnt=4;
    else if(editcomboa_dlg[11].flags&D_SELECTED) layer_cnt=5;
    else if(editcomboa_dlg[12].flags&D_SELECTED) layer_cnt=6;
    
    if(temp != layer_cnt)
    {
        return D_REDRAW;
    }
    
    return ret;
}

int set_comboaradio(byte layermask)
{
    if(editcomboa_dlg[7].flags&D_SELECTED) editcomboa_dlg[7].flags &= ~D_SELECTED;
    
    if(editcomboa_dlg[8].flags&D_SELECTED) editcomboa_dlg[8].flags &= ~D_SELECTED;
    
    if(editcomboa_dlg[9].flags&D_SELECTED) editcomboa_dlg[9].flags &= ~D_SELECTED;
    
    if(editcomboa_dlg[10].flags&D_SELECTED) editcomboa_dlg[10].flags &= ~D_SELECTED;
    
    if(editcomboa_dlg[11].flags&D_SELECTED) editcomboa_dlg[11].flags &= ~D_SELECTED;
    
    if(editcomboa_dlg[12].flags&D_SELECTED) editcomboa_dlg[12].flags &= ~D_SELECTED;
    
    if(!(layermask&1)) editcomboa_dlg[7].flags |= D_DISABLED;
    else editcomboa_dlg[7].flags &= ~D_DISABLED;
    
    if(!(layermask&2)) editcomboa_dlg[8].flags |= D_DISABLED;
    else editcomboa_dlg[8].flags &= ~D_DISABLED;
    
    if(!(layermask&4)) editcomboa_dlg[9].flags |= D_DISABLED;
    else editcomboa_dlg[9].flags &= ~D_DISABLED;
    
    if(!(layermask&8)) editcomboa_dlg[10].flags |= D_DISABLED;
    else editcomboa_dlg[10].flags &= ~D_DISABLED;
    
    if(!(layermask&16)) editcomboa_dlg[11].flags |= D_DISABLED;
    else editcomboa_dlg[11].flags &= ~D_DISABLED;
    
    if(!(layermask&32)) editcomboa_dlg[12].flags |= D_DISABLED;
    else editcomboa_dlg[12].flags &= ~D_DISABLED;
    
    editcomboa_dlg[6].flags |= D_SELECTED;
    layer_cnt=0;
    return 1;
}

int onEditComboAlias()
{
    reset_combo_animations();
    reset_combo_animations2();
    
    for(int i=0; i<MAXCOMBOALIASES; i++)
    {
        if(temp_aliases[i].combos != NULL)
        {
            delete[] temp_aliases[i].combos;
        }
        
        if(temp_aliases[i].csets != NULL)
        {
            delete[] temp_aliases[i].csets;
        }
        
        temp_aliases[i].width=combo_aliases[i].width;
        temp_aliases[i].height=combo_aliases[i].height;
        temp_aliases[i].layermask=combo_aliases[i].layermask;
        int tcount = (comboa_lmasktotal(temp_aliases[i].layermask)+1)*(temp_aliases[i].width+1)*(temp_aliases[i].height+1);
        temp_aliases[i].combos = new word[tcount];
        temp_aliases[i].csets = new byte[tcount];
        
        for(int j=0; j<tcount; j++)
        {
            temp_aliases[i].combos[j] = combo_aliases[i].combos[j];
            temp_aliases[i].csets[j] = combo_aliases[i].csets[j];
        }
        
        temp_aliases[i].combo=combo_aliases[i].combo;
        temp_aliases[i].cset=combo_aliases[i].cset;
        //memcpy(temp_aliases[i].combos,combo_aliases[i].combos,sizeof(word)*tcount);
        //memcpy(temp_aliases[i].csets,combo_aliases[i].csets,sizeof(byte)*tcount);
    }
    
    editcomboa_dlg[0].dp2 = lfont;
    set_comboaradio(temp_aliases[comboa_cnt].layermask);
    editcomboa_dlg[5].d1 = comboa_cnt;

	DIALOG *editcomboa_cpy = resizeDialog(editcomboa_dlg, 2.0);
    
    if(is_large())
    {
        for(int i=6; i<=12; i++)
        {
			editcomboa_cpy[i].w=30*1.5;
			editcomboa_cpy[i].h=9*1.5;
        }
            
		editcomboa_cpy[13].w=17*1.5;
		editcomboa_cpy[13].h=9*1.5;
		editcomboa_cpy[4].w=81*1.5;
		editcomboa_cpy[4].h=21*1.5;
		editcomboa_cpy[4].dp2=lfont_l;
		editcomboa_cpy[21].w=21*1.5;
		editcomboa_cpy[21].h=21*1.5;
		editcomboa_cpy[21].dp2=lfont_l;
    }
    
    int ret=zc_popup_dialog(editcomboa_cpy,-1);
    
    if(ret==1)
    {
        saved=false;
        
        for(int i=0; i<MAXCOMBOALIASES; i++)
        {
            if(combo_aliases[i].combos != NULL)
            {
                delete[] combo_aliases[i].combos;
            }
            
            if(combo_aliases[i].csets != NULL)
            {
                delete[] combo_aliases[i].csets;
            }
            
            combo_aliases[i].width=temp_aliases[i].width;
            combo_aliases[i].height=temp_aliases[i].height;
            combo_aliases[i].layermask=temp_aliases[i].layermask;
            int tcount = (comboa_lmasktotal(combo_aliases[i].layermask)+1)*(combo_aliases[i].width+1)*(combo_aliases[i].height+1);
            combo_aliases[i].combos = new word[tcount];
            combo_aliases[i].csets = new byte[tcount];
            
            for(int j=0; j<tcount; j++)
            {
                combo_aliases[i].combos[j] = temp_aliases[i].combos[j];
                combo_aliases[i].csets[j] = temp_aliases[i].csets[j];
            }
            
            combo_aliases[i].combo=temp_aliases[i].combo;
            combo_aliases[i].cset=temp_aliases[i].cset;
            //memcpy(combo_aliases[i].combos,temp_aliases[i].combos,sizeof(word)*tcount);
            //memcpy(combo_aliases[i].csets,temp_aliases[i].csets,sizeof(byte)*tcount);
        }
    }

	delete[] editcomboa_cpy;
    
    setup_combo_animations();
    setup_combo_animations2();
    return D_O_K;
}

static char ffcombo_str_buf[32];
static char fflink_str_buf[32];

BITMAP* ffcur;

const char *ffcombolist(int index, int *list_size)
{
    if(index>=0)
    {
        bound(index,0,31);
        sprintf(ffcombo_str_buf,"%d",index+1);
        return ffcombo_str_buf;
    }
    
    *list_size=32;
    return NULL;
}

const char *fflinklist(int index, int *list_size)
{
    if(index>=0)
    {
        bound(index,0,32);
        
        if(index)
            sprintf(fflink_str_buf,"%d",index);
        else sprintf(fflink_str_buf,"(None)");
        
        return fflink_str_buf;
    }
    
    *list_size=33;
    return NULL;
}

static ListData ffcombo_list(ffcombolist, &font);

static DIALOG ffcombo_sel_dlg[] =
{
    { jwin_win_proc,        0,    0,  200,   179,  vc(14),   vc(1),      0,       D_EXIT,     0,             0, (void *) "Choose Freeform Combo", NULL, NULL },
    { jwin_button_proc,     35,   152,   61,   21,  vc(14),   vc(1),     13,       D_EXIT,     0,             0, (void *) "Edit", NULL, NULL },
    { jwin_button_proc,    104,   152,   61,   21,  vc(14),   vc(1),     27,       D_EXIT,     0,             0, (void *) "Done", NULL, NULL },
    { d_ffcombolist_proc,  11,   24,   49,   16,   jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,       0,          1,             0, (void *) &ffcombo_list, NULL, (void *)ffcombo_sel_dlg },
    { d_comboframe_proc,   68,  23,   20,   20,   0,       0,      0,       0,             FR_DEEP,       0,       NULL, NULL, NULL },
    { d_bitmap_proc,     70,  25,   16,   16,   0,       0,      0,       0,             0,             0,       NULL, NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

int d_ffcombolist_proc(int msg,DIALOG *d,int c)
{
	DIALOG *parent = (DIALOG *)d->dp3;
    int ret = jwin_droplist_proc(msg,d,c);
    int d1 = d->d1;
    int x= parent[0].x;
    int y= parent[0].y;
    FONT *tempfont=(is_large()?font:spfont);
    int x2=text_length(tempfont, "Move Delay:")+4;
    
    switch(msg)
    {
    case MSG_DRAW:
        if(!ffcur) return D_O_K;
        
        BITMAP *buf = create_bitmap_ex(8,16,16);
        
        if(buf)
        {
            clear_bitmap(buf);
            putcombo(buf,0,0,Map.CurrScr()->ffdata[d1],Map.CurrScr()->ffcset[d1]);
            stretch_blit(buf, ffcur, 0,0, 16, 16, 0, 0, ffcur->w, ffcur->h);
            destroy_bitmap(buf);
        }
        
        object_message(&parent[5],MSG_DRAW,0);
        
        int xd = x+int(68*(is_large()?1.5:1));
        int y2 = y+int(55*(is_large()?1.5:1));
        int yd = is_large() ? 9 : 6;
        
        rectfill(screen,xd,y2,x+196*int(is_large()?1.5:1),y+127*int(is_large()?1.5:1),jwin_pal[jcBOX]);
        
        textprintf_ex(screen,tempfont,xd,y2,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"Combo #:");
        textprintf_ex(screen,tempfont,xd+x2,y2,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"%d",Map.CurrScr()->ffdata[d1]);
        
        textprintf_ex(screen,tempfont,xd,y2+yd,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"CSet #:");
        textprintf_ex(screen,tempfont,xd+x2,y2+yd,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"%d",Map.CurrScr()->ffcset[d1]);
        
        textprintf_ex(screen,tempfont,xd,y2+yd*2,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"X Pos:");
        textprintf_ex(screen,tempfont,xd+x2,y2+yd*2,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"%.4f",Map.CurrScr()->ffx[d1]/10000.0);
        
        textprintf_ex(screen,tempfont,xd,y2+yd*3,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"Y Pos:");
        textprintf_ex(screen,tempfont,xd+x2,y2+yd*3,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"%.4f",Map.CurrScr()->ffy[d1]/10000.0);
        
        textprintf_ex(screen,tempfont,xd,y2+yd*4,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"X Speed:");
        textprintf_ex(screen,tempfont,xd+x2,y2+yd*4,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"%.4f",Map.CurrScr()->ffxdelta[d1]/10000.0);
        
        textprintf_ex(screen,tempfont,xd,y2+yd*5,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"Y Speed:");
        textprintf_ex(screen,tempfont,xd+x2,y2+yd*5,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"%.4f",Map.CurrScr()->ffydelta[d1]/10000.0);
        
        textprintf_ex(screen,tempfont,xd,y2+yd*6,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"X Accel:");
        textprintf_ex(screen,tempfont,xd+x2,y2+yd*6,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"%.4f",Map.CurrScr()->ffxdelta2[d1]/10000.0);
        
        textprintf_ex(screen,tempfont,xd,y2+yd*7,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"Y Accel:");
        textprintf_ex(screen,tempfont,xd+x2,y2+yd*7,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"%.4f",Map.CurrScr()->ffydelta2[d1]/10000.0);
        
        textprintf_ex(screen,tempfont,xd,y2+yd*8,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"Linked To:");
        textprintf_ex(screen,tempfont,xd+x2,y2+yd*8,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"%d",Map.CurrScr()->fflink[d1]);
        
        textprintf_ex(screen,tempfont,xd,y2+yd*9,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"Move Delay:");
        textprintf_ex(screen,tempfont,xd+x2,y2+yd*9,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"%d",Map.CurrScr()->ffdelay[d1]);
        
        textprintf_ex(screen,tempfont,xd,y2+yd*10,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"Combo W:");
        textprintf_ex(screen,tempfont,xd+x2,y2+yd*10,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"%d",(Map.CurrScr()->ffwidth[d1]&63)+1);
        
        textprintf_ex(screen,tempfont,xd,y2+yd*11,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"Combo H:");
        textprintf_ex(screen,tempfont,xd+x2,y2+yd*11,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"%d",(Map.CurrScr()->ffheight[d1]&63)+1);
        
        textprintf_ex(screen,tempfont,xd,y2+yd*12,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"Tile W:");
        textprintf_ex(screen,tempfont,xd+x2,y2+yd*12,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"%d",(Map.CurrScr()->ffwidth[d1]>>6)+1);
        
        textprintf_ex(screen,tempfont,xd,y2+yd*13,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"Tile H:");
        textprintf_ex(screen,tempfont,xd+x2,y2+yd*13,jwin_pal[jcTEXTFG],jwin_pal[jcBOX],"%d",(Map.CurrScr()->ffheight[d1]>>6)+1);
        
        break;
    }
    
    return ret;
}
int onSelectFFCombo()
{
    ffcombo_sel_dlg[0].dp2 = lfont;
    ffcombo_sel_dlg[3].d1 = ff_combo;
    ffcur = create_bitmap_ex(8,is_large()?32:16,is_large()?32:16);
    
    if(!ffcur) return D_O_K;
    
    putcombo(ffcur,0,0,Map.CurrScr()->ffdata[ff_combo],Map.CurrScr()->ffcset[ff_combo]);
    ffcombo_sel_dlg[5].dp = ffcur;

	DIALOG *ffcombo_sel_cpy = resizeDialog(ffcombo_sel_dlg, 1.5);
    
    if(is_large())
    {
		ffcombo_sel_cpy[5].x--;
		ffcombo_sel_cpy[5].y--;
    }
    
    int ret=zc_popup_dialog(ffcombo_sel_cpy,0);
    
    while(ret==1)
    {
        ff_combo = ffcombo_sel_cpy[3].d1;
        onEditFFCombo(ff_combo);
        ret=zc_popup_dialog(ffcombo_sel_cpy,0);
    }
	delete[] ffcombo_sel_cpy;
    destroy_bitmap(ffcur);
    return D_O_K;
}

static int ffcombo_data_list[] =
{
    4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,54,55,-1
};

static int ffcombo_flag_list[] =
{
    31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,-1
};

static int ffcombo_arg_list[] =
{
    56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,-1
};

static TABPANEL ffcombo_tabs[] =
{
    // (text)
    { (char *)"Data",        D_SELECTED,  ffcombo_data_list, 0, NULL },
    { (char *)"Flags",       0,           ffcombo_flag_list, 0, NULL },
    { (char *)"Arguments",   0,           ffcombo_arg_list , 0, NULL },
    { NULL,                  0,           NULL,              0, NULL }
};

const char *ffscriptlist(int index, int *list_size);

static ListData fflink_list(fflinklist, &font);
static ListData ffscript_list(ffscriptlist, &font);

static DIALOG ffcombo_dlg[] =
{
    /* (dialog proc)     (x)   (y)    (w)   (h)   (fg)      (bg)     (key)    (flags)       (d1)           (d2)      (dp) */
    { jwin_win_proc,        0,    0,  240,   215,   vc(14),   vc(1),   0,       D_EXIT,     0,             0, (void *) "Edit Freeform Combo      ", NULL, NULL },
    { jwin_tab_proc,   10,  23,  220,  165,   0,       0,      0,       0,             0,       0, (void *) ffcombo_tabs, NULL, (void *)ffcombo_dlg },
    { jwin_button_proc,    30+10,  171+20,   61,    21,   vc(14),   vc(1),   13,      D_EXIT,     0,             0, (void *) "OK", NULL, NULL },
    { jwin_button_proc,   100+10,  171+20,   61,    21,   vc(14),   vc(1),   27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    // 4
    { jwin_ctext_proc,    126+10,   25+20,   70,    36,   0,        0,       0,       D_DISABLED, 0,             0, (void *) "COMBO", NULL, NULL },
    { d_comboframe_proc,    116+10,   31+20,   20,    20,   0,        0,       0,       0,          FR_DEEP,       0,       NULL, NULL, NULL },
    { d_combo_proc,       118+10,   33+20,   16,    16,   0,        0,       0,       0,          0,             0,       NULL, NULL, NULL },
    // 7
    { jwin_text_proc,       6+10,   29+20,   70,    36,   0,        0,       0,       0,          0,             0, (void *) "Link to:", NULL, NULL },
    { jwin_text_proc,       6+10,   47+20,   36,    36,   0,        0,       0,       0,          0,             0, (void *) "X Pos:", NULL, NULL },
    { jwin_text_proc,       6+10,   65+20,   36,    36,   0,        0,       0,       0,          0,             0, (void *) "Y Pos:", NULL, NULL },
    { jwin_text_proc,       6+10,   83+20,   70,    36,   0,        0,       0,       0,          0,             0, (void *) "X Speed:", NULL, NULL },
    { jwin_text_proc,       6+10,  101+20,   70,    36,   0,        0,       0,       0,          0,             0, (void *) "Y Speed:", NULL, NULL },
    { jwin_text_proc,       6+10,  119+20,   70,    36,   0,        0,       0,       0,          0,             0, (void *) "X Accel:", NULL, NULL },
    { jwin_text_proc,       6+10,  137+20,   70,    36,   0,        0,       0,       0,          0,             0, (void *) "Y Accel:", NULL, NULL },
    { jwin_text_proc,     112+10,  137+20,   70,    12,   0,        0,       0,       0,          0,             0, (void *) "A. Delay:", NULL, NULL },
    // 15
    { jwin_droplist_proc,  50+10,   25+20,   60,    16,   jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],   0,       0,          1,             0, (void *) &fflink_list, NULL, NULL },
    { jwin_edit_proc,      50+10,   43+20,   56,    16,   vc(12),   vc(1),   0,       0,          9,             0,       NULL, NULL, NULL },
    { jwin_edit_proc,      50+10,   61+20,   56,    16,   vc(12),   vc(1),   0,       0,          9,             0,       NULL, NULL, NULL },
    { jwin_edit_proc,      50+10,   79+20,   56,    16,   vc(12),   vc(1),   0,       0,          9,             0,       NULL, NULL, NULL },
    { jwin_edit_proc,      50+10,   97+20,   56,    16,   vc(12),   vc(1),   0,       0,          9,             0,       NULL, NULL, NULL },
    { jwin_edit_proc,      50+10,  115+20,   56,    16,   vc(12),   vc(1),   0,       0,          9,             0,       NULL, NULL, NULL },
    { jwin_edit_proc,      50+10,  133+20,   56,    16,   vc(12),   vc(1),   0,       0,          9,             0,       NULL, NULL, NULL },
    { jwin_edit_proc,     156+10,  133+20,   32,    16,   vc(12),   vc(1),   0,       0,          4,             0,       NULL, NULL, NULL },
    //23
    { jwin_text_proc,       112+10,  65+20,   70,    36,   0,        0,       0,       0,          0,             0, (void *) "Combo W:", NULL, NULL },
    { jwin_text_proc,       112+10,  83+20,   70,    36,   0,        0,       0,       0,          0,             0, (void *) "Combo H:", NULL, NULL },
    { jwin_text_proc,       112+10,  101+20,   70,    36,   0,        0,       0,       0,          0,             0, (void *) "Tile W:", NULL, NULL },
    { jwin_text_proc,       112+10,  119+20,   70,    36,   0,        0,       0,       0,          0,             0, (void *) "Tile H:", NULL, NULL },
    //27
    { jwin_edit_proc,      156+10,  61+20,   32,    16,   vc(12),   vc(1),   0,       0,          2,             0,       NULL, NULL, NULL },
    { jwin_edit_proc,      156+10,  79+20,   32,    16,   vc(12),   vc(1),   0,       0,          2,             0,       NULL, NULL, NULL },
    { jwin_edit_proc,      156+10,  97+20,   24,    16,   vc(12),   vc(1),   0,       0,          1,             0,       NULL, NULL, NULL },
    { jwin_edit_proc,      156+10,  115+20,   24,    16,   vc(12),   vc(1),   0,       0,          1,             0,       NULL, NULL, NULL },
    //31
    { jwin_text_proc,       6+10,  25+20,   160,    36,   0,        0,       0,       0,          0,             0, (void *) "Flags (Normal):", NULL, NULL },
    { jwin_text_proc,       6+10,  105+20,   160,    36,   0,        0,       0,       0,          0,             0, (void *) "Flags (Changer specific):", NULL, NULL },
    //33
    { jwin_check_proc,      6+10,  35+20,  80+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Draw Over", NULL, NULL },
    { jwin_check_proc,      6+10,  45+20,  80+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Translucent", NULL, NULL },
    { jwin_check_proc,     80+10,  35+20,  80+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Carry-Over", NULL, NULL },
    { jwin_check_proc,     80+10,  45+20,  80+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Stationary", NULL, NULL },
    { jwin_check_proc,      6+10,  55+20,  80+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Is a Changer (Invisible, Ethereal)", NULL, NULL },
    { jwin_check_proc,      6+10,  65+20,  80+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Run Script at Screen Init", NULL, NULL },
    { jwin_check_proc,      6+10,  75+20,  80+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Only Visible to Lens of Truth", NULL, NULL },
    { jwin_check_proc,      6+10,  85+20,  80+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Script Restarts When Carried Over", NULL, NULL },
    //41
    { jwin_check_proc,    154+10,  35+20,  80+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Ethereal", NULL, NULL },
    { jwin_check_proc,      6+10,  95+20,  80+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Active While Link is Holding an Item", NULL, NULL },
    { d_dummy_proc,      6+10,  25+20,     0,  8,    vc(15),  vc(1),  0,       0,          0,             0,       NULL, NULL, NULL },
    { d_dummy_proc,      6+10,  25+20,     0,  8,    vc(15),  vc(1),  0,       0,          0,             0,       NULL, NULL, NULL },
    { d_dummy_proc,      6+10,  25+20,     0,  8,    vc(15),  vc(1),  0,       0,          0,             0,       NULL, NULL, NULL },
    { d_dummy_proc,      6+10,  25+20,     0,  8,    vc(15),  vc(1),  0,       0,          0,             0,       NULL, NULL, NULL },
    { d_dummy_proc,      6+10,  25+20,     0,  8,    vc(15),  vc(1),  0,       0,          0,             0,       NULL, NULL, NULL },
    { d_dummy_proc,      6+10,  25+20,     0,  8,    vc(15),  vc(1),  0,       0,          0,             0,       NULL, NULL, NULL },
    //49
    { jwin_check_proc,      6+10,  115+20,  80+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Swap with next FFC", NULL, NULL },
    { jwin_check_proc,      6+10,  125+20,  80+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Swap with prev. FFC", NULL, NULL },
    { jwin_check_proc,      6+10,  135+20,  80+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Increase combo ID", NULL, NULL },
    { jwin_check_proc,      6+10,  145+20,  80+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Decrease combo ID", NULL, NULL },
    { jwin_check_proc,      6+10,  155+20,  80+1,  8+1,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Change combo/cset to this", NULL, NULL },
    //54
    { jwin_text_proc,       6+10,  155+20,   70,    12,   0,        0,       0,       0,          0,             0, (void *) "Script:", NULL, NULL },
    { jwin_droplist_proc,   50+10,   151+20,  150,    16,   jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],   0,       0,          1,             0, (void *) &ffscript_list, NULL, NULL },
    //56
    { jwin_text_proc,       6+10,   29+20,   24,    36,   0,        0,       0,       0,          0,             0, (void *) "D0:", NULL, NULL },
    { jwin_text_proc,       6+10,   47+20,   24,    36,   0,        0,       0,       0,          0,             0, (void *) "D1:", NULL, NULL },
    { jwin_text_proc,       6+10,   65+20,   24,    36,   0,        0,       0,       0,          0,             0, (void *) "D2:", NULL, NULL },
    { jwin_text_proc,       6+10,   83+20,   24,    36,   0,        0,       0,       0,          0,             0, (void *) "D3:", NULL, NULL },
    { jwin_text_proc,       6+10,  101+20,   24,    36,   0,        0,       0,       0,          0,             0, (void *) "D4:", NULL, NULL },
    { jwin_text_proc,       6+10,  119+20,   24,    36,   0,        0,       0,       0,          0,             0, (void *) "D5:", NULL, NULL },
    { jwin_text_proc,       6+10,  137+20,   24,    36,   0,        0,       0,       0,          0,             0, (void *) "D6:", NULL, NULL },
    { jwin_text_proc,       6+10,  155+20,   24,    12,   0,        0,       0,       0,          0,             0, (void *) "D7:", NULL, NULL },
    //64
    { jwin_edit_proc,      34+10,   25+20,   72,    16,   vc(12),   vc(1),   0,       0,          12,             0,       NULL, NULL, NULL },
    { jwin_edit_proc,      34+10,   43+20,   72,    16,   vc(12),   vc(1),   0,       0,          12,             0,       NULL, NULL, NULL },
    { jwin_edit_proc,      34+10,   61+20,   72,    16,   vc(12),   vc(1),   0,       0,          12,             0,       NULL, NULL, NULL },
    { jwin_edit_proc,      34+10,   79+20,   72,    16,   vc(12),   vc(1),   0,       0,          12,             0,       NULL, NULL, NULL },
    { jwin_edit_proc,      34+10,   97+20,   72,    16,   vc(12),   vc(1),   0,       0,          12,             0,       NULL, NULL, NULL },
    { jwin_edit_proc,      34+10,  115+20,   72,    16,   vc(12),   vc(1),   0,       0,          12,             0,       NULL, NULL, NULL },
    { jwin_edit_proc,      34+10,  133+20,   72,    16,   vc(12),   vc(1),   0,       0,          12,             0,       NULL, NULL, NULL },
    { jwin_edit_proc,      34+10,  151+20,   72,    16,   vc(12),   vc(1),   0,       0,          12,             0,       NULL, NULL, NULL },
    //72
    { jwin_text_proc,       112+10,  29+20,   24,    36,   0,        0,       0,       0,          0,             0, (void *) "A1:", NULL, NULL },
    { jwin_text_proc,       112+10,  47+20,   24,    36,   0,        0,       0,       0,          0,             0, (void *) "A2:", NULL, NULL },
    //74
    { jwin_edit_proc,      140+10,  25+20,   32,    16,   vc(12),   vc(1),   0,       0,          2,             0,       NULL, NULL, NULL },
    { jwin_edit_proc,      140+10,  43+20,   32,    16,   vc(12),   vc(1),   0,       0,          2,             0,       NULL, NULL, NULL },
    { jwin_text_proc,      220, 151+20,   24,    36,   0,        0,       0,       0,          0,             0, (void*) "", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};


char *strip_decimals(char *string)
{
    int len=(int)strlen(string);
    char *src=(char *)zc_malloc(len+1);
    char *tmpsrc=src;
    memcpy(src,string,len+1);
    memset(src,0,len+1);
    
    for(unsigned int i=0; string[i]&&i<=strlen(string); i++)
    {
        *tmpsrc=string[i];
        
        if(*tmpsrc=='.')
        {
            while(string[i+1]=='.'&&i<=strlen(string))
            {
                i++;
            }
        }
        
        tmpsrc++;
    }
    
    memcpy(string,src,len);
    zc_free(src);
    return string;
}

// Unused??? -L 6/6/11
char *clean_numeric_string(char *string)
{
    bool found_sign=false;
    bool found_decimal=false;
    int len=(int)strlen(string);
    char *src=(char *)zc_malloc(len+1);
    char *tmpsrc=src;
    memcpy(src,string,len+1);
    memset(src,0,len+1);
    
    // strip out non-numerical characters
    for(unsigned int i=0; string[i]&&i<=strlen(string); i++)
    {
        *tmpsrc=string[i];
        
        if(*tmpsrc!='.'&&*tmpsrc!='-'&&*tmpsrc!='+'&&!isdigit(*tmpsrc))
        {
            while(*tmpsrc!='.'&&*tmpsrc!='-'&&*tmpsrc!='+'&&!isdigit(*tmpsrc))
            {
                i++;
            }
        }
        
        tmpsrc++;
    }
    
    len=(int)strlen(src);
    char *src2=(char *)zc_malloc(len+1);
    tmpsrc=src2;
    memcpy(src,src2,len+1);
    memset(src2,0,len+1);
    
    // second purge
    for(unsigned int i=0; src[i]&&i<=strlen(src); i++)
    {
        *tmpsrc=src[i];
        
        if(*tmpsrc=='-'||*tmpsrc=='+')
        {
            if(found_sign||found_decimal)
            {
                while(*tmpsrc=='-'||*tmpsrc=='+')
                {
                    i++;
                }
            }
            
            found_sign=true;
        }
        
        if(*tmpsrc=='.')
        {
            if(found_decimal)
            {
                while(*tmpsrc=='.')
                {
                    i++;
                }
            }
            
            found_decimal=true;
        }
        
        tmpsrc++;
    }
    
    sprintf(string, "%s", src2);
    zc_free(src);
    zc_free(src2);
    return string;
}

script_struct biffs[NUMSCRIPTFFC]; //ff script
int biffs_cnt = -1;
script_struct biitems[NUMSCRIPTFFC]; //item script
int biitems_cnt = -1;
//static char ffscript_str_buf[32];

void build_biffs_list()
{
    biffs[0].first = "(None)";
    biffs[0].second = -1;
    biffs_cnt = 1;
    
    for(int i = 0; i < NUMSCRIPTFFC - 1; i++)
    {
        if(ffcmap[i].second.length()==0)
            continue;
            
        std::stringstream ss;
        ss << ffcmap[i].second << " (" << i+1 << ")"; // The word 'slot' preceding all of the numbers is a bit cluttersome. -L.
        biffs[biffs_cnt].first = ss.str();
        biffs[biffs_cnt].second = i;
        biffs_cnt++;
    }
    
    // Blank out the rest of the list
    for(int i=biffs_cnt; i<NUMSCRIPTFFC; i++)
    {
        biffs[i].first="";
        biffs[i].second=-1;
    }
    
    //Bubble sort! (doesn't account for gaps between scripts)
    for(int i = 0; i < biffs_cnt - 1; i++)
    {
        for(int j = i + 1; j < biffs_cnt; j++)
        {
            if(stricmp(biffs[i].first.c_str(),biffs[j].first.c_str()) > 0 && strcmp(biffs[j].first.c_str(),""))
                zc_swap(biffs[i],biffs[j]);
        }
    }
    
    biffs_cnt = 0;
    
    for(int i = 0; i < NUMSCRIPTFFC; i++)
        if(biffs[i].first.length() > 0)
            biffs_cnt = i+1;
}

void build_biitems_list()
{
    biitems[0].first = "(None)";
    biitems[0].second = -1;
    biitems_cnt = 1;
    
    for(int i = 0; i < NUMSCRIPTITEM - 1; i++, biitems_cnt++)
    {
        std::stringstream ss;
        
        if(itemmap[i].second != "")
            ss << itemmap[i].second << " (" << i+1 << ")";
            
        biitems[biitems_cnt].first = ss.str();
        biitems[biitems_cnt].second = i;
    }
    
    for(int i = 0; i < biitems_cnt - 1; i++)
    {
        for(int j = i + 1; j < biitems_cnt; j++)
        {
            if(stricmp(biitems[i].first.c_str(), biitems[j].first.c_str()) > 0 && strcmp(biitems[j].first.c_str(),""))
                zc_swap(biitems[i], biitems[j]);
        }
    }
    
    biitems_cnt = 0;
    
    for(int i = 0; i < NUMSCRIPTITEM; i++)
        if(biitems[i].first.length() > 0)
            biitems_cnt = i+1;
}

const char *ffscriptlist(int index, int *list_size)
{
    if(index < 0)
    {
        *list_size = biffs_cnt;
        return NULL;
    }
    
    return biffs[index].first.c_str();
}

static char itemscript_str_buf[32];

char *itemscriptlist(int index, int *list_size)
{
    if(index>=0)
    {
        bound(index,0,255);
        sprintf(itemscript_str_buf,"%d: %s",index, ffcmap[index-1].second.c_str());
        return itemscript_str_buf;
    }
    
    *list_size=256;
    return NULL;
}

static char ffscript_str_buf2[32];

const char *ffscriptlist2(int index, int *list_size)
{
    if(index>=0)
    {
        char buf[20];
        bound(index,0,510);
        
        if(ffcmap[index].second=="")
            strcpy(buf, "<none>");
        else
        {
            strncpy(buf, ffcmap[index].second.c_str(), 19);
            buf[19]='\0';
        }
        
        sprintf(ffscript_str_buf2,"%d: %s",index+1, buf);
        return ffscript_str_buf2;
    }
    
    *list_size=511;
    return NULL;
}

static char itemscript_str_buf2[32];

const char *itemscriptlist2(int index, int *list_size)
{
    if(index>=0)
    {
        char buf[20];
        bound(index,0,254);
        
        if(itemmap[index].second=="")
            strcpy(buf, "<none>");
        else
        {
            strncpy(buf, itemmap[index].second.c_str(), 19);
            buf[19]='\0';
        }
        
        sprintf(itemscript_str_buf2,"%d: %s",index+1, buf);
        return itemscript_str_buf2;
    }
    
    *list_size=255;
    return NULL;
}

static char gscript_str_buf2[32];

const char *gscriptlist2(int index, int *list_size)
{
    if(index >= 0)
    {
        bound(index,0,3);
        
        char buf[20];
        
        if(globalmap[index].second == "")
            strcpy(buf, "<none>");
        else
        {
            strncpy(buf, globalmap[index].second.c_str(), 19);
            buf[19]='\0';
        }
        
        if(index==0)
            sprintf(gscript_str_buf2,"Initialization: %s", buf);
            
        if(index==1)
            sprintf(gscript_str_buf2,"Active: %s", buf);
            
        if(index==2)
            sprintf(gscript_str_buf2,"onExit: %s", buf);
            
        if(index==3)
            sprintf(gscript_str_buf2,"onContinue: %s", buf);
            
        return gscript_str_buf2;
    }
    
    if(list_size != NULL)
        *list_size=4;
        
    return NULL;
}

static DIALOG compile_dlg[] =
{
    //						x		y		w		h		fg		bg		key	flags	d1	d2	dp
    { jwin_win_proc,		0,		0,		200,	118,	vc(14),	vc(1),	0,	D_EXIT,	0,	0,	(void *) "Compile ZScript", NULL, NULL },
    { jwin_button_proc,		109,	89,		61,		21,		vc(14),	vc(1),	27,	D_EXIT,	0,	0,	(void *) "Cancel", NULL, NULL },
    { jwin_button_proc,		131,	30,		61,		21,		vc(14),	vc(1),	'e',	D_EXIT,	0,	0,	(void *) "&Edit", NULL, NULL },
    { jwin_button_proc,		30,		60,		61,		21,		vc(14), vc(1),	'i',	D_EXIT,	0,	0,	(void *) "&Import", NULL, NULL },
    { jwin_text_proc,		8,		35,		61,		21,		vc(14),	vc(1),	0,	0,		0,	0,	(void *) zScriptBytes, NULL, NULL },
    { jwin_button_proc,		30,		89,		61,		21,		vc(14),	vc(1),  'c',	D_EXIT,	0,	0,	(void *) "&Compile!", NULL, NULL },
    { jwin_button_proc,		109,	60,		61,		21,		vc(14),	vc(1),	'x',	D_EXIT,	0,	0,	(void *) "E&xport", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,   NULL,  NULL }
};

static int as_ffc_list[] = { 4, 5, 6, -1};
static int as_global_list[] = { 7, 8, 9, -1}; //Why does putting 15 in here not place my message only on the global tab? ~Joe
static int as_item_list[] = { 10, 11, 12, -1};

static TABPANEL assignscript_tabs[] =
{
    // (text)
    { (char *)"FFC",     D_SELECTED,  as_ffc_list,    0, NULL },
    { (char *)"Global",	 0,         as_global_list, 0, NULL },
    { (char *)"Item",		 0,         as_item_list,   0, NULL },
    { NULL,                0,           NULL,         0, NULL }
};

const char *assignffclist(int index, int *list_size)
{
    if(index<0)
    {
        *list_size = (int)ffcmap.size();
        return NULL;
    }
    
    return ffcmap[index].first.c_str();
}

const char *assigngloballist(int index, int *list_size)
{
    if(index<0)
    {
        *list_size = (int)globalmap.size();
        return NULL;
    }
    
    return globalmap[index].first.c_str();
}

const char *assignitemlist(int index, int *list_size)
{
    if(index<0)
    {
        *list_size = (int)itemmap.size();
        return NULL;
    }
    
    return itemmap[index].first.c_str();
}

const char *assignffcscriptlist(int index, int *list_size)
{
    if(index<0)
    {
        *list_size = (int)asffcscripts.size();
        return NULL;
    }
    
    return asffcscripts[index].c_str();
}

const char *assignglobalscriptlist(int index, int *list_size)
{
    if(index<0)
    {
        *list_size = (int)asglobalscripts.size();
        return NULL;
    }
    
    return asglobalscripts[index].c_str();
}

const char *assignitemscriptlist(int index, int *list_size)
{
    if(index<0)
    {
        *list_size = (int)asitemscripts.size();
        return NULL;
    }
    
    return asitemscripts[index].c_str();
}

static ListData assignffc_list(assignffclist, &font);
static ListData assignffcscript_list(assignffcscriptlist, &font);
static ListData assignglobal_list(assigngloballist, &font);
static ListData assignglobalscript_list(assignglobalscriptlist, &font);
static ListData assignitem_list(assignitemlist, &font);
static ListData assignitemscript_list(assignitemscriptlist, &font);

static DIALOG assignscript_dlg[] =
{
    //						x		y		w		h		fg		bg		key	flags	d1	d2	dp
    { jwin_win_proc,		  0,	0,		320,	220,	vc(14),	vc(1),	0,	D_EXIT,	0,	0,	(void *) "Assign Compiled Script", NULL, NULL },
    { jwin_tab_proc,		  6,	25,		308,	130,	0,		0,		0,	0,		0,  0,  assignscript_tabs, NULL, (void*)assignscript_dlg },
    { jwin_button_proc,	  251,	191,	61,		21,		vc(14),	vc(1),	27,	D_EXIT,	0,	0,	(void *) "Cancel", NULL, NULL },
    { jwin_button_proc,	  182,	191,	61,		21,		vc(14), vc(1),	'k',	    D_EXIT,	0,	0,	(void *) "O&K", NULL, NULL },
    { jwin_abclist_proc,    10,	45,		136,	105,	jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,0,0, 0, (void *)&assignffc_list, NULL, NULL },
    { jwin_abclist_proc,    174,	45,		136,	105,	jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,0,0, 0, (void *)&assignffcscript_list, NULL, NULL },
    { jwin_button_proc,	  154,	93,		15,		10,		vc(14),	vc(1),	0,	D_EXIT,	0,	0,	(void *) "<<", NULL, NULL },
    { jwin_abclist_proc,    10,	45,		136,	105,	jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,0,0, 0, (void *)&assignglobal_list, NULL, NULL },
    { jwin_abclist_proc,    174,	45,		136,	105,	jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,0,0, 0, (void *)&assignglobalscript_list, NULL, NULL },
    { jwin_button_proc,	  154,	93,		15,		10,		vc(14),	vc(1),	0,	D_EXIT,	0,	0,	(void *) "<<", NULL, NULL },
    { jwin_abclist_proc,    10,	45,		136,	105,	jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,0,0, 0, (void *)&assignitem_list, NULL, NULL },
    { jwin_abclist_proc,    174,	45,		136,	105,	jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,0,0, 0, (void *)&assignitemscript_list, NULL, NULL },
    { jwin_button_proc,	  154,	93,		15,		10,		vc(14),	vc(1),	0,	D_EXIT,	0,	0,	(void *) "<<", NULL, NULL },
    
    { jwin_check_proc,      22,  195,   90,   8,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Output ZASM code to allegro.log", NULL, NULL },
    { jwin_text_proc,       22,  158,   90,   8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Slots with matching names have been updated. Scripts marked", NULL, NULL },
    { jwin_text_proc,       22,  168,  90,   8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "with ** were not found in the buffer and will not function.", NULL, NULL },
    { jwin_text_proc,       22,  178,  90,   8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Global scripts named 'Init' will be appended to '~Init'", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,        NULL, NULL, NULL }
    
};

//editbox_data zscript_edit_data;

static DIALOG edit_zscript_dlg[] =
{
    /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)      (d2)      (dp) */
    { jwin_win_proc,        0,   0,   320,  240,  0,       vc(15), 0,      D_EXIT,       0,          0, (void *) "Edit ZScript", NULL, NULL },
	{ jwin_frame_proc,   4,   23,   320 - 8,  240 - 27,   0,       0,      0,       0,             FR_DEEP,       0,       NULL, NULL, NULL },
    { d_editbox_proc,    6,   25,   320-12,  240-6-25,  0,       0,      0,       0/*D_SELECTED*/,          0,        0,        NULL, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,       0,      0,       0,          0,        KEY_ESC, (void *) close_dlg, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,       0,      0,       0,          0,        KEY_F12, (void *) onSnapshot, NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

void doEditZScript(int bg,int fg)
{
    string old = zScript;
    EditboxModel *em = new EditboxModel(zScript, new EditboxScriptView(&edit_zscript_dlg[2],(is_large()?sfont3:sfont2),fg,bg,BasicEditboxView::HSTYLE_EOTEXT), false, (char *)"zscript.txt");
	if(is_large())
	{
		edit_zscript_dlg[0].w = 800;
		edit_zscript_dlg[0].h = 600;
		edit_zscript_dlg[1].w = 800 - 8;
		edit_zscript_dlg[1].h = 600 - 27;
		edit_zscript_dlg[2].w = 800 - 8 - 4;
		edit_zscript_dlg[2].h = 600 - 27 - 4;
	}
	else
	{
		edit_zscript_dlg[0].w = 320;
		edit_zscript_dlg[0].h = 240;
		edit_zscript_dlg[1].w = 320 - 8;
		edit_zscript_dlg[1].h = 240 - 27;
		edit_zscript_dlg[2].w = 320 - 8 - 4;
		edit_zscript_dlg[2].h = 240 - 6 - 25;
	}
    edit_zscript_dlg[0].dp2= lfont;
    edit_zscript_dlg[2].dp = em;
    edit_zscript_dlg[2].bg = bg;
    
    zc_popup_dialog(edit_zscript_dlg,2);
    
    if(jwin_alert("ZScript Buffer","Save changes to buffer?",NULL,NULL,"Yes","No",'y','n',lfont)==2)
        zScript = old;
    else
        saved=false;
        
    delete em;
}

static ListData ffscript_sel_dlg_list(ffscriptlist2, &font);

static DIALOG ffscript_sel_dlg[] =
{
    { jwin_win_proc,        0,    0,    200, 159, vc(14),   vc(1),      0,       D_EXIT,     0,             0, (void *) "Choose Slot And Name", NULL, NULL },
    { jwin_text_proc,       8,    80,   36,  8,   vc(14),   vc(1),     0,       0,          0,             0, (void *) "Name:", NULL, NULL },
    { jwin_edit_proc,       44,   80-4, 146, 16,  vc(12),   vc(1),     0,       0,          19,            0,       NULL, NULL, NULL },
    { jwin_button_proc,     35,   132,  61,   21, vc(14),   vc(1),     13,       D_EXIT,     0,             0, (void *) "Load", NULL, NULL },
    { jwin_button_proc,     104,  132,  61,   21, vc(14),   vc(1),     27,       D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { jwin_droplist_proc,   26,   45,   146,   16, jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,       0,          1,             0, (void *) &ffscript_sel_dlg_list, NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

static ListData itemscript_sel_dlg_list(itemscriptlist2, &font);

static DIALOG itemscript_sel_dlg[] =
{
    { jwin_win_proc,        0,    0,    200, 159, vc(14),   vc(1),      0,       D_EXIT,     0,             0, (void *) "Choose Slot And Name", NULL, NULL },
    { jwin_text_proc,       8,    80,   36,  8,   vc(14),   vc(1),     0,       0,          0,             0, (void *) "Name:", NULL, NULL },
    { jwin_edit_proc,       44,   80-4, 146, 16,  vc(12),   vc(1),     0,       0,          19,            0,       NULL, NULL, NULL },
    { jwin_button_proc,     35,   132,  61,   21, vc(14),   vc(1),     13,       D_EXIT,     0,             0, (void *) "Load", NULL, NULL },
    { jwin_button_proc,     104,  132,  61,   21, vc(14),   vc(1),     27,       D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { jwin_droplist_proc,   26,   45,   146,   16, jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,       0,          1,             0, (void *) &itemscript_sel_dlg_list, NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

static ListData gscript_sel_dlg_list(gscriptlist2, &font);

static DIALOG gscript_sel_dlg[] =
{
    { jwin_win_proc,        0,    0,    200, 159, vc(14),   vc(1),      0,       D_EXIT,     0,             0, (void *) "Choose Slot And Name", NULL, NULL },
    { jwin_text_proc,       8,    80,   36,  8,   vc(14),   vc(1),     0,       0,          0,             0, (void *) "Name:", NULL, NULL },
    { jwin_edit_proc,       44,   80-4, 146, 16,  vc(12),   vc(1),     0,       0,          19,            0,       NULL, NULL, NULL },
    { jwin_button_proc,     35,   132,  61,   21, vc(14),   vc(1),     13,       D_EXIT,     0,             0, (void *) "Load", NULL, NULL },
    { jwin_button_proc,     104,  132,  61,   21, vc(14),   vc(1),     27,       D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { jwin_droplist_proc,   26,   45,   146,   16, jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,       0,          1,             0, (void *) &gscript_sel_dlg_list, NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};


int onCompileScript()
{
    compile_dlg[0].dp2 = lfont;

	DIALOG *compile_cpy = resizeDialog(compile_dlg, 1.5);
    
    for(;;) //while(true)
    {
        sprintf(zScriptBytes, "%d Bytes in Buffer", (int)(zScript.size()));
        int ret = zc_popup_dialog(compile_cpy,5);
        
        switch(ret)
        {
        case 0:
        case 1:
            //Cancel
            return D_O_K;
            
        case 2:
            //Edit
            doEditZScript(vc(15),vc(0));
            break;
            
        case 3:
        {
            //Load from File
            if(zScript.size() > 0)
            {
                if(jwin_alert("Confirm Overwrite","Loading will erase the current buffer.","Proceed anyway?",NULL,"Yes","No",'y','n',lfont)==2)
                    break;
                    
                zScript.clear();
            }
            
            if(!getname("Load ZScript (.z)","z",NULL,datapath,false))
                break;
                
            FILE *zscript = fopen(temppath,"r");
            
            if(zscript == NULL)
            {
                jwin_alert("Error","Cannot open specified file!",NULL,NULL,"O&K",NULL,'k',0,lfont);
                break;
            }
            
            char c = fgetc(zscript);
            
            while(!feof(zscript))
            {
                zScript += c;
                c = fgetc(zscript);
            }
            
            fclose(zscript);
            saved = false;
            break;
        }
        
        case 6:
            //Export
        {
            if(!getname("Save ZScript (.z)", "z", NULL,datapath,false))
                break;
                
            if(exists(temppath))
            {
                if(jwin_alert("Confirm Overwrite","File already exists.","Overwrite?",NULL,"Yes","No",'y','n',lfont)==2)
                    break;
            }
            
            FILE *zscript = fopen(temppath,"w");
            
            if(!zscript)
            {
                jwin_alert("Error","Unable to open file for writing!",NULL,NULL,"O&K",NULL,'k',0,lfont);
                break;
            }
            
            int written = (int)fwrite(zScript.c_str(), sizeof(char), zScript.size(), zscript);
            
            if(written != (int)zScript.size())
                jwin_alert("Error","IO error while writing script to file!",NULL,NULL,"O&K",NULL,'k',0,lfont);
                
            fclose(zscript);
            break;
        }
        
        case 5:
            //Compile!
            FILE *tempfile = fopen("tmp","w");
            
            if(!tempfile)
            {
                jwin_alert("Error","Unable to create a temporary file in current directory!",NULL,NULL,"O&K",NULL,'k',0,lfont);
                return D_O_K;
            }
            
            fwrite(zScript.c_str(), sizeof(char), zScript.size(), tempfile);
            fclose(tempfile);
            box_start(1, "Compile Progress", lfont, sfont,true);
            gotoless_not_equal = (0 != get_bit(quest_rules, qr_GOTOLESSNOTEQUAL)); // Used by BuildVisitors.cpp
            ScriptsData *result = compile("tmp");
            unlink("tmp");
            box_end(true);
            refresh(rALL);
            
            if(result == NULL)
            {
                jwin_alert("Error","There were compile errors.","Compilation halted.",NULL,"O&K",NULL,'k',0,lfont);
                break;
            }
            
            std::map<string, ScriptType> stypes = result->scriptTypes;
            std::map<string, vector<Opcode *> > scriptsmap = result->theScripts;
            delete result;
            asffcscripts.clear();
            asffcscripts.push_back("<none>");
            asglobalscripts.clear();
            asglobalscripts.push_back("<none>");
            asitemscripts.clear();
            asitemscripts.push_back("<none>");
            
            for(std::map<string, ScriptType>::iterator it = stypes.begin(); it != stypes.end(); it++)
            {
                switch(it->second)
                {
                case SCRIPTTYPE_FFC:
                {
                    asffcscripts.push_back(it->first);
                    break;
                }
                
                case SCRIPTTYPE_GLOBAL:
                {
                    if(it->first != "~Init") //Don't allow assigning the allocate memory script, bad things could happen
                        asglobalscripts.push_back(it->first);
                        
                    break;
                }
                
                case SCRIPTTYPE_ITEM:
                {
                    asitemscripts.push_back(it->first);
                    break;
                }
                }
            }
            
            assignscript_dlg[0].dp2 = lfont;
            assignscript_dlg[4].d1 = -1;
            assignscript_dlg[5].d1 = -1;
            assignscript_dlg[7].d1 = -1;
            assignscript_dlg[8].d1 = -1;
            assignscript_dlg[10].d1 = -1;
            assignscript_dlg[11].d1 = -1;
            assignscript_dlg[13].flags = 0;
            
            //assign scripts to slots
            for(;;) //while(true)
            {
                char temp[100];
                
                for(int i = 0; i < NUMSCRIPTFFC-1; i++)
                {
                    if(ffcmap[i].second == "")
                        sprintf(temp, "Slot %d: <none>", i+1);
                    else if(scriptsmap.find(ffcmap[i].second) != scriptsmap.end())
                        sprintf(temp, "Slot %d: %s", i+1, ffcmap[i].second.c_str());
                    else // Previously loaded script not found
                        sprintf(temp, "Slot %d: **%s**", i+1, ffcmap[i].second.c_str());
                    ffcmap[i].first = temp;
                }
                
                for(int i = 0; i < NUMSCRIPTGLOBAL; i++)
                {
                    char buffer[64];
                    const char* format;
                    const char* asterisks;
                    switch(i)
                    {
                        case 0: format="Initialization: %s%s%s"; break;
                        case 1: format="Active: %s%s%s"; break;
                        case 2: format="onExit: %s%s%s"; break;
                        case 3: format="onContinue: %s%s%s"; break;
                    }
                    if(globalmap[i].second == "")
                        asterisks="";
                    else if(scriptsmap.find(globalmap[i].second) != scriptsmap.end())
                        asterisks="";
                    else // Unloaded
                        asterisks="**";
                    snprintf(buffer, 50, format, asterisks, globalmap[i].second.c_str(), asterisks);
                    globalmap[i].first=buffer;
                }
                
                for(int i = 0; i < NUMSCRIPTITEM-1; i++)
                {
                    if(itemmap[i].second == "")
                        sprintf(temp, "Slot %d: <none>", i+1);
                    else if(scriptsmap.find(itemmap[i].second) != scriptsmap.end())
                        sprintf(temp, "Slot %d: %s", i+1, itemmap[i].second.c_str());
                    else // Previously loaded script not found
                        sprintf(temp, "Slot %d: **%s**", i+1, itemmap[i].second.c_str());
                    itemmap[i].first = temp;
                }
                
				DIALOG *assignscript_cpy = resizeDialog(assignscript_dlg, 1.5);
                    
                int ret2 = zc_popup_dialog(assignscript_cpy,3);
                
                switch(ret2)
                {
                case 0:
                case 2:
                    //Cancel
                    return D_O_K;
                    
                case 3:
                {
                
                    //OK
                    bool output = (assignscript_cpy[13].flags == D_SELECTED);
                    
                    for(std::map<int, pair<string,string> >::iterator it = ffcmap.begin(); it != ffcmap.end(); it++)
                    {
                        if(it->second.second != "")
                        {
                            tempfile = fopen("tmp","w");
                            
                            if(!tempfile)
                            {
                                jwin_alert("Error","Unable to create a temporary file in current directory!",NULL,NULL,"O&K",NULL,'k',0,lfont);
                                return D_O_K;
                            }
                            
                            if(output)
                            {
                                al_trace("\n");
                                al_trace("%s",it->second.second.c_str());
                                al_trace("\n");
                            }
                            
                            for(vector<Opcode *>::iterator line = scriptsmap[it->second.second].begin(); line != scriptsmap[it->second.second].end(); line++)
                            {
                                string theline = (*line)->printLine();
                                fwrite(theline.c_str(), sizeof(char), theline.size(),tempfile);
                                
                                if(output)
                                {
                                    al_trace("%s",theline.c_str());
                                }
                            }
                            
                            fclose(tempfile);
							int scriptid = it->first + 1;
							if(scriptid+1 > (int)scripts.ffscripts.size())
								scripts.ffscripts.resize(scriptid + 1);
							scripts.ffscripts[scriptid].version = ZASM_VERSION;
							scripts.ffscripts[scriptid].type = SCRIPT_FFC;
							scripts.ffscripts[scriptid].name_len = (int16_t)it->second.second.length() + 1;
							delete[] scripts.ffscripts[scriptid].name;
							scripts.ffscripts[scriptid].name = new char[scripts.ffscripts[scriptid].name_len];
							strcpy(scripts.ffscripts[scriptid].name, it->second.second.c_str());
                            parse_script_file(&scripts.ffscripts[scriptid].commands,"tmp",false);
							int len = 0;
							while (scripts.ffscripts[scriptid].commands[len].command != 0xFFFF)
								len++;
							scripts.ffscripts[scriptid].commands_len = len + 1;
                        }
                        else
                        {
							scripts.ffscripts[it->first + 1] = ZAsmScript();
                        }
                    }
                    
                    for(std::map<int, pair<string,string> >::iterator it = globalmap.begin(); it != globalmap.end(); it++)
                    {
                        if(it->second.second != "")
                        {
                            tempfile = fopen("tmp","w");
                            
                            if(!tempfile)
                            {
                                jwin_alert("Error","Unable to create a temporary file in current directory!",NULL,NULL,"O&K",NULL,'k',0,lfont);
                                return D_O_K;
                            }
                            
                            if(output)
                            {
                                al_trace("\n");
                                al_trace("%s",it->second.second.c_str());
                                al_trace("\n");
                            }
                            
                            for(vector<Opcode *>::iterator line = scriptsmap[it->second.second].begin(); line != scriptsmap[it->second.second].end(); line++)
                            {
                                string theline = (*line)->printLine();
                                fwrite(theline.c_str(), sizeof(char), theline.size(),tempfile);
                                
                                if(output)
                                {
                                    al_trace("%s",theline.c_str());
                                }
                            }
                            
                            fclose(tempfile);

							int scriptid = it->first;
							if(scriptid +1 > (int)scripts.globalscripts.size())
								scripts.globalscripts.resize(scriptid + 1);
							scripts.globalscripts[scriptid].version = ZASM_VERSION;
							scripts.globalscripts[scriptid].type = SCRIPT_GLOBAL;
							scripts.globalscripts[scriptid].name_len = int16_t(it->second.second.length()) + 1;
							delete[] scripts.globalscripts[scriptid].name;
							scripts.globalscripts[scriptid].name = new char[scripts.globalscripts[scriptid].name_len];
							strcpy(scripts.globalscripts[scriptid].name, it->second.second.c_str());
							parse_script_file(&scripts.globalscripts[scriptid].commands, "tmp", false);
							int len = 0;
							while (scripts.globalscripts[scriptid].commands[len].command != 0xFFFF)
								len++;
							scripts.globalscripts[scriptid].commands_len = len + 1;
                        }
						else
						{
							scripts.globalscripts[it->first] = ZAsmScript();
						}
                    }
                    
                    for(std::map<int, pair<string,string> >::iterator it = itemmap.begin(); it != itemmap.end(); it++)
                    {
						if (it->second.second != "")
						{
							tempfile = fopen("tmp", "w");

							if (!tempfile)
							{
								jwin_alert("Error", "Unable to create a temporary file in current directory!", NULL, NULL, "O&K", NULL, 'k', 0, lfont);
								return D_O_K;
							}

							if (output)
							{
								al_trace("\n");
								al_trace("%s", it->second.second.c_str());
								al_trace("\n");
							}

							for (vector<Opcode *>::iterator line = scriptsmap[it->second.second].begin(); line != scriptsmap[it->second.second].end(); line++)
							{
								string theline = (*line)->printLine();
								fwrite(theline.c_str(), sizeof(char), theline.size(), tempfile);

								if (output)
								{
									al_trace("%s", theline.c_str());
								}
							}

							fclose(tempfile);

							int scriptid = it->first+1;
							if (scriptid + 1 > (int)scripts.itemscripts.size())
								scripts.itemscripts.resize(scriptid + 1);
							scripts.itemscripts[scriptid].version = ZASM_VERSION;
							scripts.itemscripts[scriptid].type = SCRIPT_ITEM;
							scripts.itemscripts[scriptid].name_len = (int16_t)it->second.second.length() + 1;
							delete[] scripts.itemscripts[scriptid].name;
							scripts.itemscripts[scriptid].name = new char[scripts.itemscripts[scriptid].name_len];
							strcpy(scripts.itemscripts[scriptid].name, it->second.second.c_str());
							parse_script_file(&scripts.itemscripts[scriptid].commands, "tmp", false);
							int len = 0;
							while (scripts.itemscripts[scriptid].commands[len].command != 0xFFFF)
								len++;
							scripts.itemscripts[scriptid].commands_len = len + 1;							
						}
						else
						{
							scripts.itemscripts[it->first + 1] = ZAsmScript();
						}
                    }
                    
                    unlink("tmp");
                    jwin_alert("Done!","ZScripts successfully loaded into script slots",NULL,NULL,"O&K",NULL,'k',0,lfont);
                    build_biffs_list();
                    build_biitems_list();
                    
                    for(map<string, vector<Opcode *> >::iterator it = scriptsmap.begin(); it != scriptsmap.end(); it++)
                    {
                        for(vector<Opcode *>::iterator it2 = it->second.begin(); it2 != it->second.end(); it2++)
                        {
                            delete *it2;
                        }
                    }
                    
                    return D_O_K;
                }
                
                case 6:
                    //<<, FFC
                {
                    int lind = assignscript_cpy[4].d1;
                    int rind = assignscript_cpy[5].d1;
                    
                    if(lind < 0 || rind < 0)
                        break;
                        
                    if(asffcscripts[rind] == "<none>")
                    {
                        ffcmap[lind].second = "";
                    }
                    else
                    {
                        ffcmap[lind].second = asffcscripts[rind];
                    }
                    
                    break;
                }
                
                case 9:
                    //<<, Global
                {
                    int lind = assignscript_cpy[7].d1;
                    int rind = assignscript_cpy[8].d1;
                    
                    if(lind < 0 || rind < 0)
                        break;
                        
                    if(lind == 0)
                    {
                        jwin_alert("Error","ZScript reserves this slot.",NULL,NULL,"O&K",NULL,'k',0,lfont);
                        break;
                    }
                    
                    if(asglobalscripts[rind] == "<none>")
                    {
                        globalmap[lind].second = "";
                    }
                    else
                    {
                        globalmap[lind].second = asglobalscripts[rind];
                    }
                    
                    break;
                }
                
                case 12:
                    //<<, ITEM
                {
                    int lind = assignscript_cpy[10].d1;
                    int rind = assignscript_cpy[11].d1;
                    
                    if(lind < 0 || rind < 0)
                        break;
                        
                    if(asitemscripts[rind] == "<none>")
                    {
                        itemmap[lind].second = "";
                    }
                    else
                    {
                        itemmap[lind].second = asitemscripts[rind];
                    }
                    
                    break;
                }
                }

				delete[] assignscript_cpy;
            }
            
            break;
        }
    }

	delete[] compile_cpy;
    
// return D_O_K;//unreachable
}

int onImportFFScript()
{
    char name[20]="";
    
    ffscript_sel_dlg[0].dp2 = lfont;
    ffscript_sel_dlg[2].dp = name;
    ffscript_sel_dlg[5].d1 = 0;

	DIALOG *ffscript_sel_cpy = resizeDialog(ffscript_sel_dlg, 1.5);
    
	int ret=zc_popup_dialog(ffscript_sel_cpy,0);
    
    if(ret==3)
    {
        if(parse_script(scripts.ffscripts[ffscript_sel_cpy[5].d1+1], SCRIPT_FFC)==D_O_K)
        {
            if(strlen((char *)ffscript_sel_cpy[2].dp)>0)
                ffcmap[ffscript_sel_cpy[5].d1].second=(char *)ffscript_sel_cpy[2].dp;
            else
                ffcmap[ffscript_sel_cpy[5].d1].second="ASM script";
                
            build_biffs_list();
        }
    }

	delete[] ffscript_sel_cpy;
    
    return D_O_K;
}

int onImportItemScript()
{
    char name[20]="";
    
    itemscript_sel_dlg[0].dp2 = lfont;
    itemscript_sel_dlg[2].dp = name;
    itemscript_sel_dlg[5].d1 = 0;

	DIALOG *itemscript_sel_cpy = resizeDialog(itemscript_sel_dlg, 1.5);
    
    int ret=zc_popup_dialog(itemscript_sel_cpy,0);
    
    if(ret==3)
    {
        if(parse_script(scripts.itemscripts[itemscript_sel_cpy[5].d1+1], SCRIPT_ITEM)==D_O_K)
        {
            if(strlen((char *)itemscript_sel_cpy[2].dp)>0)
                itemmap[itemscript_sel_cpy[5].d1].second=(char *)itemscript_sel_cpy[2].dp;
            else
                itemmap[itemscript_sel_cpy[5].d1].second="ASM script";
                
            build_biitems_list();
        }
    }

	delete[] itemscript_sel_cpy;
    
    return D_O_K;
}

int onImportGScript()
{
    char name[20]="";
    
    gscript_sel_dlg[0].dp2 = lfont;
    gscript_sel_dlg[2].dp = name;
    gscript_sel_dlg[5].d1 = 0;

	DIALOG *gscript_sel_cpy = resizeDialog(gscript_sel_dlg, 1.5);
    
    int ret=zc_popup_dialog(gscript_sel_cpy,0);
    
    if(ret==3)
    {
        if(parse_script(scripts.globalscripts[gscript_sel_cpy[5].d1], SCRIPT_GLOBAL)==D_O_K)
        {
            if(strlen((char *)gscript_sel_cpy[2].dp)>0)
                globalmap[gscript_sel_cpy[5].d1].second=(char *)gscript_sel_cpy[2].dp;
            else
                globalmap[gscript_sel_cpy[5].d1].second="ASM script";
        }
    }

	delete[] gscript_sel_cpy;
    
    return D_O_K;
}

int onEditFFCombo(int ffcombo)
{
    char xystring[8][10];
    char wstring[4][10];
    char dastring[10][13];
    sprintf(xystring[0],"%.4f",Map.CurrScr()->ffx[ffcombo]/10000.0);
    sprintf(xystring[1],"%.4f",Map.CurrScr()->ffy[ffcombo]/10000.0);
    sprintf(xystring[2],"%.4f",Map.CurrScr()->ffxdelta[ffcombo]/10000.0);
    sprintf(xystring[3],"%.4f",Map.CurrScr()->ffydelta[ffcombo]/10000.0);
    sprintf(xystring[4],"%.4f",Map.CurrScr()->ffxdelta2[ffcombo]/10000.0);
    sprintf(xystring[5],"%.4f",Map.CurrScr()->ffydelta2[ffcombo]/10000.0);
    sprintf(xystring[6],"%d",Map.CurrScr()->ffdelay[ffcombo]);
    
    sprintf(wstring[0],"%d",(Map.CurrScr()->ffwidth[ffcombo]&63)+1);
    sprintf(wstring[1],"%d",(Map.CurrScr()->ffheight[ffcombo]&63)+1);
    sprintf(wstring[2],"%d",(Map.CurrScr()->ffwidth[ffcombo]>>6)+1);
    sprintf(wstring[3],"%d",(Map.CurrScr()->ffheight[ffcombo]>>6)+1);
    
    sprintf(dastring[0],"%.4f",Map.CurrScr()->initd[ffcombo][0]/10000.0);
    sprintf(dastring[1],"%.4f",Map.CurrScr()->initd[ffcombo][1]/10000.0);
    sprintf(dastring[2],"%.4f",Map.CurrScr()->initd[ffcombo][2]/10000.0);
    sprintf(dastring[3],"%.4f",Map.CurrScr()->initd[ffcombo][3]/10000.0);
    sprintf(dastring[4],"%.4f",Map.CurrScr()->initd[ffcombo][4]/10000.0);
    sprintf(dastring[5],"%.4f",Map.CurrScr()->initd[ffcombo][5]/10000.0);
    sprintf(dastring[6],"%.4f",Map.CurrScr()->initd[ffcombo][6]/10000.0);
    sprintf(dastring[7],"%.4f",Map.CurrScr()->initd[ffcombo][7]/10000.0);
    sprintf(dastring[8],"%ld",Map.CurrScr()->inita[ffcombo][0]/10000);
    sprintf(dastring[9],"%ld",Map.CurrScr()->inita[ffcombo][1]/10000);
    
    char wtitle[80];
    sprintf(wtitle,"Edit Freeform Combo (#%d)", ffcombo+1);
    ffcombo_dlg[0].dp2 = lfont;
    ffcombo_dlg[0].dp = wtitle;
    ffcombo_dlg[4].dp2 = spfont;
    
    ffcombo_dlg[6].d1 = Map.CurrScr()->ffdata[ffcombo];
    ffcombo_dlg[6].fg = Map.CurrScr()->ffcset[ffcombo];
    
    ffcombo_dlg[15].d1 = Map.CurrScr()->fflink[ffcombo];
    ffcombo_dlg[16].dp = xystring[0];
    ffcombo_dlg[17].dp = xystring[1];
    ffcombo_dlg[18].dp = xystring[2];
    ffcombo_dlg[19].dp = xystring[3];
    ffcombo_dlg[20].dp = xystring[4];
    ffcombo_dlg[21].dp = xystring[5];
    ffcombo_dlg[22].dp = xystring[6];
    
    ffcombo_dlg[27].dp = wstring[0];
    ffcombo_dlg[28].dp = wstring[1];
    ffcombo_dlg[29].dp = wstring[2];
    ffcombo_dlg[30].dp = wstring[3];
    
    ffcombo_dlg[64].dp = dastring[0];
    ffcombo_dlg[65].dp = dastring[1];
    ffcombo_dlg[66].dp = dastring[2];
    ffcombo_dlg[67].dp = dastring[3];
    ffcombo_dlg[68].dp = dastring[4];
    ffcombo_dlg[69].dp = dastring[5];
    ffcombo_dlg[70].dp = dastring[6];
    ffcombo_dlg[71].dp = dastring[7];
    ffcombo_dlg[74].dp = dastring[8];
    ffcombo_dlg[75].dp = dastring[9];
    
    build_biffs_list();
    int index = 0;
    
    for(int j = 0; j < biffs_cnt; j++)
    {
        if(biffs[j].second == Map.CurrScr()->ffscript[ffcombo] - 1)
        {
            index = j;
        }
    }
    
    ffcombo_dlg[55].d1 = index;
    
    int f=Map.CurrScr()->ffflags[ffcombo];
    ffcombo_dlg[33].flags = (f&ffOVERLAY) ? D_SELECTED : 0;
    ffcombo_dlg[34].flags = (f&ffTRANS) ? D_SELECTED : 0;
    ffcombo_dlg[35].flags = (f&ffCARRYOVER) ? D_SELECTED : 0;
    ffcombo_dlg[36].flags = (f&ffSTATIONARY) ? D_SELECTED : 0;
    ffcombo_dlg[37].flags = (f&ffCHANGER) ? D_SELECTED : 0;
    ffcombo_dlg[38].flags = (f&ffPRELOAD) ? D_SELECTED : 0;
    ffcombo_dlg[39].flags = (f&ffLENSVIS) ? D_SELECTED : 0;
    ffcombo_dlg[40].flags = (f&ffSCRIPTRESET) ? D_SELECTED : 0;
    ffcombo_dlg[41].flags = (f&ffETHEREAL) ? D_SELECTED : 0;
    ffcombo_dlg[42].flags = (f&ffIGNOREHOLDUP) ? D_SELECTED : 0;
    
    ffcombo_dlg[49].flags = (f&ffSWAPNEXT) ? D_SELECTED : 0;
    ffcombo_dlg[50].flags = (f&ffSWAPPREV) ? D_SELECTED : 0;
    ffcombo_dlg[51].flags = (f&ffCHANGENEXT) ? D_SELECTED : 0;
    ffcombo_dlg[52].flags = (f&ffCHANGEPREV) ? D_SELECTED : 0;
    ffcombo_dlg[53].flags = (f&ffCHANGETHIS) ? D_SELECTED : 0;

	DIALOG *ffcombo_cpy = resizeDialog(ffcombo_dlg, 1.5);
        
    int ret = -1;
    
    do
    {
        ret=zc_popup_dialog(ffcombo_cpy,0);
        
        // A polite warning about FFC 0 and scripts
        if(ret==2 && !ffcombo_cpy[6].d1 && ffcombo_cpy[55].d1>0)
            if(jwin_alert("Inactive FFC","FFCs that use Combo 0 cannot run scripts! Continue?",NULL,NULL,"Yes","No",'y','n',lfont)==2)
                ret=-1;
    }
    while(ret<0);
    
    if(ret==2)
    {
        saved=false;
        Map.CurrScr()->ffdata[ffcombo] = ffcombo_cpy[6].d1;
        Map.CurrScr()->ffcset[ffcombo] = ffcombo_cpy[6].fg;
        Map.CurrScr()->fflink[ffcombo] = ffcombo_cpy[15].d1;
        Map.CurrScr()->ffx[ffcombo] = vbound(ffparse(xystring[0]),-320000, 2880000);
        Map.CurrScr()->ffy[ffcombo] = vbound(ffparse(xystring[1]),-320000, 2080000);
        Map.CurrScr()->ffxdelta[ffcombo] = vbound(ffparse(xystring[2]),-1280000, 1280000);
        Map.CurrScr()->ffydelta[ffcombo] = vbound(ffparse(xystring[3]),-1280000, 1280000);
        Map.CurrScr()->ffxdelta2[ffcombo] = vbound(ffparse(xystring[4]),-1280000, 1280000);
        Map.CurrScr()->ffydelta2[ffcombo] = vbound(ffparse(xystring[5]),-1280000, 1280000);
        Map.CurrScr()->ffdelay[ffcombo] = atoi(xystring[6])<10000?zc_max(0,atoi(xystring[6])):9999;
        Map.CurrScr()->ffscript[ffcombo] = biffs[ffcombo_cpy[55].d1].second + 1;
        
        int cw = atoi(wstring[0])<65?zc_max(1,atoi(wstring[0])):64;
        int ch = atoi(wstring[1])<65?zc_max(1,atoi(wstring[1])):64;
        int tw = atoi(wstring[2])<5?zc_max(1,atoi(wstring[2])):4;
        int th = atoi(wstring[3])<5?zc_max(1,atoi(wstring[3])):4;
        Map.CurrScr()->ffwidth[ffcombo] = (cw-1)+((tw-1)<<6);
        Map.CurrScr()->ffheight[ffcombo] = (ch-1)+((th-1)<<6);
        
        Map.CurrScr()->initd[ffcombo][0] = vbound(ffparse(dastring[0]),-2147483647, 2147483647);
        Map.CurrScr()->initd[ffcombo][1] = vbound(ffparse(dastring[1]),-2147483647, 2147483647);
        Map.CurrScr()->initd[ffcombo][2] = vbound(ffparse(dastring[2]),-2147483647, 2147483647);
        Map.CurrScr()->initd[ffcombo][3] = vbound(ffparse(dastring[3]),-2147483647, 2147483647);
        Map.CurrScr()->initd[ffcombo][4] = vbound(ffparse(dastring[4]),-2147483647, 2147483647);
        Map.CurrScr()->initd[ffcombo][5] = vbound(ffparse(dastring[5]),-2147483647, 2147483647);
        Map.CurrScr()->initd[ffcombo][6] = vbound(ffparse(dastring[6]),-2147483647, 2147483647);
        Map.CurrScr()->initd[ffcombo][7] = vbound(ffparse(dastring[7]),-2147483647, 2147483647);
        
        Map.CurrScr()->inita[ffcombo][0] = vbound(atoi(dastring[8])*10000,0,320000);
        Map.CurrScr()->inita[ffcombo][1] = vbound(atoi(dastring[9])*10000,0,320000);
        
        f=0;
        f |= (ffcombo_cpy[33].flags&D_SELECTED) ? ffOVERLAY : 0;
        f |= (ffcombo_cpy[34].flags&D_SELECTED) ? ffTRANS : 0;
        f |= (ffcombo_cpy[35].flags&D_SELECTED) ? ffCARRYOVER : 0;
        f |= (ffcombo_cpy[36].flags&D_SELECTED) ? ffSTATIONARY : 0;
        f |= (ffcombo_cpy[37].flags&D_SELECTED) ? ffCHANGER : 0;
        f |= (ffcombo_cpy[38].flags&D_SELECTED) ? ffPRELOAD : 0;
        f |= (ffcombo_cpy[39].flags&D_SELECTED) ? ffLENSVIS : 0;
        f |= (ffcombo_cpy[40].flags&D_SELECTED) ? ffSCRIPTRESET : 0;
        f |= (ffcombo_cpy[41].flags&D_SELECTED) ? ffETHEREAL : 0;
        f |= (ffcombo_cpy[42].flags&D_SELECTED) ? ffIGNOREHOLDUP : 0;
        
        f |= (ffcombo_cpy[49].flags&D_SELECTED) ? ffSWAPNEXT : 0;
        f |= (ffcombo_cpy[50].flags&D_SELECTED) ? ffSWAPPREV : 0;
        f |= (ffcombo_cpy[51].flags&D_SELECTED) ? ffCHANGENEXT : 0;
        f |= (ffcombo_cpy[52].flags&D_SELECTED) ? ffCHANGEPREV : 0;
        f |= (ffcombo_cpy[53].flags&D_SELECTED) ? ffCHANGETHIS : 0;
        Map.CurrScr()->ffflags[ffcombo] = f;
        
        if(Map.CurrScr()->ffdata[ffcombo]!=0)
        {
            Map.CurrScr()->numff|=(1<<ffcombo);
        }
        else
        {
            Map.CurrScr()->numff&=~(1<<ffcombo);
        }
    }

	delete[] ffcombo_cpy;
    
    return D_O_K;
}

static DIALOG sfxlist_dlg[] =
{
    // (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp)
    { jwin_win_proc,     60-12,   40,   200+24,  148,  vc(14),  vc(1),  0,       D_EXIT,          0,             0,       NULL, NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { jwin_abclist_proc,       72-12-4,   60+4,   176+24+8,  92+3,   jwin_pal[jcTEXTFG],  jwin_pal[jcTEXTBG],  0,       D_EXIT,     0,             0,       NULL, NULL, NULL },
    { jwin_button_proc,     60,   163,  41,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "Edit", NULL, NULL },
    { jwin_button_proc,     126,  163,  41,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Done", NULL, NULL },
    { jwin_button_proc,     192,  163,  41,   21,   vc(14),  vc(1),  0,      D_EXIT,     0,             0, (void *) "New", NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

int select_sfx(const char *prompt,int index)
{
    sfxlist_dlg[0].dp=(void *)prompt;
    sfxlist_dlg[0].dp2=lfont;
    sfxlist_dlg[2].d1=index;
    sfxlist_dlg[2].dp=(void *) & sfx_list;

	DIALOG *sfxlist_cpy = resizeDialog(sfxlist_dlg, 1.5);
    
    int ret=zc_popup_dialog(sfxlist_cpy,2);
    
    if(ret==0||ret==4)
    {
		Backend::mouse->setWheelPosition(0);
		delete[] sfxlist_cpy;
        return -1;
    }
    if(ret==5)
    {
        index = Backend::sfx->numSlots();
        delete[] sfxlist_cpy;
        return index;
    }
    
    index = sfxlist_cpy[2].d1;
	delete[] sfxlist_cpy;
	Backend::mouse->setWheelPosition(0);
    return index;
}

static DIALOG sfx_edit_dlg[] =
{
    { jwin_win_proc,           0,     0,  200,   159,  vc(14),              vc(1),                 0,       D_EXIT,     0,             0, (void *) "SFX", NULL, NULL },
    { jwin_button_proc,       35,   132,   61,    21,  vc(14),              vc(1),                13,       D_EXIT,     0,             0, (void *) "OK", NULL, NULL },
    { jwin_button_proc,      104,   132,   61,    21,  vc(14),              vc(1),                27,       D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { jwin_button_proc,        5,    78,   61,    21,  vc(14),              vc(1),                 0,       D_EXIT,     0,             0, (void *) "Load", NULL, NULL },
    { jwin_button_proc,       35,   105,   61,    21,  vc(14),              vc(1),                 0,       D_EXIT,     0,             0, (void *) "Play", NULL, NULL },
    { jwin_button_proc,      104,   105,   61,    21,  vc(14),              vc(1),                 0,       D_EXIT,     0,             0, (void *) "Stop", NULL, NULL },
    { jwin_button_proc,      135,    78,   61,    21,  vc(14),              vc(1),                 0,       D_EXIT,     0,             0, (void *) "Default", NULL, NULL },
    { jwin_edit_proc,     36,    25,   154,    16,  vc(12),  vc(1),    0,       0,         36,             0,       NULL, NULL, NULL },
    { jwin_text_proc,     8,    30,     16,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Name:", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { jwin_button_proc,       70,   78,    61,    21,  vc(14),              vc(1),                 0,       D_EXIT,     0,             0, (void *) "Save", NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

/*****************/
/*****  SFX  *****/
/*****************/

int onSelectSFX()
{
    int index = select_sfx("Select SFX",0);
    
    while(index >= 0)
    {
        if(index)
            onEditSFX(index);
            
        index = select_sfx("Select SFX",index);
    }
    
    refresh(rMAP+rCOMBOS);
    return D_O_K;
}

int onEditSFX(int index)
{
    Backend::sfx->stopAll();
    stop_midi();
    set_volume(255,-1);
    int ret;
    sfx_edit_dlg[0].dp2=lfont;

    const SFXSample *sample = Backend::sfx->getSample(index);
    
    char sfxnumstr[50];
    snprintf(sfxnumstr, 50, "SFX %d: %s", index, sample == NULL ? "(None)" : sample->name.c_str());
    sfx_edit_dlg[0].dp = sfxnumstr;
    
    char *name = new char[sample == NULL ? 7 : sample->name.length() + 1];
    strcpy(name,sample==NULL ? "(None)" : sample->name.c_str());
    sfx_edit_dlg[7].dp = name;

	DIALOG *sfx_edit_cpy = resizeDialog(sfx_edit_dlg, 1.5);

    SFXSample *oldsample = NULL;
    if (sample)
    {
        oldsample = new SFXSample(sample->sample);
        oldsample->iscustom = sample->iscustom;
        oldsample->name = sample->name;
    }
    
    do
    {
        ret=zc_popup_dialog(sfx_edit_cpy,1);
        
        switch(ret)
        {
        case 1:
            saved= false;
            Backend::sfx->stopAll();
            break;
            
        case 2:
        case 0:
            // Fall Through
            Backend::sfx->stopAll();                                  
            if (oldsample)
            {
                Backend::sfx->loadSample(index, oldsample->sample, oldsample->name);
                Backend::sfx->setIsCustom(index, oldsample->iscustom);
            }
            else
                Backend::sfx->clearSample(index);
            break;
            
        case 3:
            if(getname("Open .WAV file", "wav", NULL,temppath, true))
            {
                SAMPLE * temp_sample;
                packfile_password("");
                if((temp_sample = load_wav(temppath))==NULL)
                {
                    jwin_alert("Error","Could not open file",temppath,NULL,"OK",NULL,13,27,lfont);
                }
                else
                {
                    char sfxtitle[36];
                    char *t = get_filename(temppath);
                    int j;
                    
                    for(j=0; j<35 && t[j]!=0 && t[j]!='.'; j++)
                    {
                        sfxtitle[j]=t[j];
                    }
                    
                    sfxtitle[j]=0;
                    delete[] name;
                    name = new char[strlen(sfxtitle) + 1];
                    strcpy(name,sfxtitle);
                    sfx_edit_cpy[7].dp = name;
                    Backend::sfx->stopAll();
                    Backend::sfx->loadSample(index, *temp_sample, sfxtitle);
                    destroy_sample(temp_sample);
                }
            }
            
            break;
            
        case 4:
        {
            Backend::sfx->stopAll();
            Backend::sfx->play(index, 128);
        }
        break;
        
        case 5:
            Backend::sfx->stopAll();
            break;
            
        case 6:
            Backend::sfx->stopAll();
            if (index < Z35)
                Backend::sfx->loadDefaultSample(index, sfxdata, old_sfx_string);
            else
                Backend::sfx->clearSample(index);

            delete[] name;
            sample = Backend::sfx->getSample(index);
            name = new char[sample == NULL ? 7 : sample->name.length() + 1];
            strcpy(name, sample == NULL ? "(None)" : sample->name.c_str());
            sfx_edit_cpy[7].dp = name;
            
            break;
        case 10:
            if (Backend::sfx->getSample(index) != NULL)
            {
                if (getname("Save .WAV file", "wav", NULL, temppath, true))
                {
                    if(!Backend::sfx->saveWAV(index, temppath))
                    {
                        jwin_alert("Error", "Could not write file", temppath, NULL, "OK", NULL, 13, 27, lfont);
                    }
                }
            }
            break;
        }        

    }
    while(ret>2);

	delete[] sfx_edit_cpy;
    delete[] name;
    if (oldsample)
        delete oldsample;
    
    return D_O_K;
}


static DIALOG mapstyles_dlg[] =
{
    /* (dialog proc)     (x)   (y)    (w)   (h)   (fg)      (bg)     (key)    (flags)       (d1)           (d2)      (dp) */
    { jwin_win_proc,        0,    0,  307,  186,  vc(14),   vc(1),      0,       D_EXIT,     0,             0, (void *) "Map Styles", NULL, NULL },
    { jwin_ctext_proc,     24,   34,   36,   36,       0,       0,      0,       0,          0,             0, (void *) "Frame", NULL, NULL },       //frame
    { jwin_ctext_proc,     68,   26,   20,   20,       0,       0,      0,       0,          0,             0, (void *) "Triforce", NULL, NULL },       //triforce fragment
    { jwin_ctext_proc,     68,   34,   20,   20,       0,       0,      0,       0,          0,             0, (void *) "Fragment", NULL, NULL },       //triforce fragment
    { jwin_ctext_proc,    152,   26,  100,   52,       0,       0,      0,       0,          0,             0, (void *) "Triforce Frame", NULL, NULL },       //triforce frame
    { jwin_ctext_proc,    152,   34,  100,   52,       0,       0,      0,       0,          0,             0, (void *) "(Normal or Double Sized)", NULL, NULL },       //triforce frame
    { jwin_ctext_proc,    260,   34,   84,   52,       0,       0,      0,       0,          0,             0, (void *) "Overworld Map", NULL, NULL },       //overworld map
    { jwin_ctext_proc,     24,   82,   20,   20,       0,       0,      0,       0,          0,             0, (void *) "Heart", NULL, NULL },       //heart container piece
    { jwin_ctext_proc,     24,   90,   20,   20,       0,       0,      0,       0,          0,             0, (void *) "Container", NULL, NULL },       //heart container piece
    { jwin_ctext_proc,     24,   98,   20,   20,       0,       0,      0,       0,          0,             0, (void *) "Piece", NULL, NULL },       //heart container piece
    { jwin_ctext_proc,    260,   98,   84,   52,       0,       0,      0,       0,          0,             0, (void *) "Dungeon Map", NULL, NULL },       //dungeon map
    // 11
    { jwin_frame_proc,      6,   42,   36,   36,       0,       0,      0,       0,          FR_DEEP,       0,       NULL, NULL, NULL },             //frame
    //  { jwin_frame_proc,     50,   42,   36,   52,       0,       0,      0,       0,          FR_DEEP,       0,       NULL, NULL, NULL }, //bs triforce fragment
    { jwin_frame_proc,     58,   42,   20,   20,       0,       0,      0,       0,          FR_DEEP,       0,       NULL, NULL, NULL },             //normal triforce fragment
    //  { jwin_frame_proc,     94,   42,  116,  116,       0,       0,      0,       0,          FR_DEEP,       0,       NULL, NULL, NULL }, //bs triforce frame
    { jwin_frame_proc,    102,   42,  100,   52,       0,       0,      0,       0,          FR_DEEP,       0,       NULL, NULL, NULL },             //normaltriforce frame
    { jwin_frame_proc,    218,   42,   84,   52,       0,       0,      0,       0,          FR_DEEP,       0,       NULL, NULL, NULL },             //overworld map
    { jwin_frame_proc,     14,  106,   20,   20,       0,       0,      0,       0,          FR_DEEP,       0,       NULL, NULL, NULL },             //heart container piece
    { jwin_frame_proc,    218,  106,   84,   52,       0,       0,      0,       0,          FR_DEEP,       0,       NULL, NULL, NULL },             //dungeon map
    // 17
    { d_maptile_proc,       8,   44,   32,   32,       0,       0,      0,       0,          0,             0,       NULL, (void*)1, NULL },             //frame
    //  { d_maptile_proc,      52,   44,   32,   48,       0,       0,      0,       0,          0,             0,       NULL, NULL, NULL }, //bs triforce fragment
    { d_maptile_proc,      60,   44,   16,   16,       0,       0,      0,       0,          0,             0,       NULL, (void*)1, NULL },             //normal triforce fragment
    //  { d_maptile_proc,      96,   44,  112,  112,       0,       0,      0,       0,          0,             0,       NULL, NULL, NULL }, //bs triforce frame
    { d_maptile_proc,     104,   44,   96,   48,       0,       0,      0,       0,          0,             0,       NULL, (void*)1, NULL },             //normal triforce frame
    { d_maptile_proc,     220,   44,   80,   48,       0,       0,      0,       0,          0,             0,       NULL, (void*)1, NULL },             //overworld map
    { d_maptile_proc,      16,  108,   16,   16,       0,       0,      0,       0,          0,             0,       NULL, (void*)1, NULL },             //heart container piece
    { d_maptile_proc,     220,  108,   80,   48,       0,       0,      0,       0,          0,             0,       NULL, (void*)1, NULL },             //dungeon map
    // 23
    { jwin_button_proc,    83,  162,   61,   21,  vc(14),   vc(1),     13,       D_EXIT,     0,             0, (void *) "OK", NULL, NULL },
    { jwin_button_proc,   163,  162,   61,   21,  vc(14),   vc(1),     27,       D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

int onMapStyles()
{
    if(mapstyles_dlg[0].d1<1)
    {
        mapstyles_dlg[12].x=mapstyles_dlg[0].x+50;
        mapstyles_dlg[12].w=36;
        mapstyles_dlg[12].h=52;
        mapstyles_dlg[13].x=mapstyles_dlg[0].x+94;
        mapstyles_dlg[13].w=116;
        mapstyles_dlg[13].h=116;
        mapstyles_dlg[18].x=mapstyles_dlg[12].x+2;
        mapstyles_dlg[18].w=mapstyles_dlg[12].w-4;
        mapstyles_dlg[18].h=mapstyles_dlg[12].h-4;
        mapstyles_dlg[19].x=mapstyles_dlg[13].x+2;
        mapstyles_dlg[19].w=mapstyles_dlg[13].w-4;
        mapstyles_dlg[19].h=mapstyles_dlg[13].h-4;
    }
    
    mapstyles_dlg[0].dp2 = lfont;
    mapstyles_dlg[17].d1  = misc.colors.blueframe_tile;
    mapstyles_dlg[17].fg  = misc.colors.blueframe_cset;
    mapstyles_dlg[18].d1  = misc.colors.triforce_tile;
    mapstyles_dlg[18].fg  = misc.colors.triforce_cset;
    mapstyles_dlg[19].d1  = misc.colors.triframe_tile;
    mapstyles_dlg[19].fg  = misc.colors.triframe_cset;
    mapstyles_dlg[20].d1  = misc.colors.overworld_map_tile;
    mapstyles_dlg[20].fg  = misc.colors.overworld_map_cset;
    mapstyles_dlg[21].d1 = misc.colors.HCpieces_tile;
    mapstyles_dlg[21].fg = misc.colors.HCpieces_cset;
    mapstyles_dlg[22].d1  = misc.colors.dungeon_map_tile;
    mapstyles_dlg[22].fg  = misc.colors.dungeon_map_cset;

	DIALOG *mapstyles_cpy = resizeDialog(mapstyles_dlg, 2.0);
        
    go();
    int ret = zc_do_dialog(mapstyles_cpy,-1);
    comeback();
    
    if(ret==23)
    {
        misc.colors.blueframe_tile     = mapstyles_cpy[17].d1;
        misc.colors.blueframe_cset     = mapstyles_cpy[17].fg;
        misc.colors.triforce_tile      = mapstyles_cpy[18].d1;
        misc.colors.triforce_cset      = mapstyles_cpy[18].fg;
        misc.colors.triframe_tile      = mapstyles_cpy[19].d1;
        misc.colors.triframe_cset      = mapstyles_cpy[19].fg;
        misc.colors.overworld_map_tile = mapstyles_cpy[20].d1;
        misc.colors.overworld_map_cset = mapstyles_cpy[20].fg;
        misc.colors.HCpieces_tile      = mapstyles_cpy[21].d1;
        misc.colors.HCpieces_cset      = mapstyles_cpy[21].fg;
        misc.colors.dungeon_map_tile   = mapstyles_cpy[22].d1;
        misc.colors.dungeon_map_cset   = mapstyles_cpy[22].fg;
        saved=false;
    }

	delete[] mapstyles_cpy;
    
    return D_O_K;
}

int d_misccolors_old_proc(int msg,DIALOG *d,int c)
{
    //these are here to bypass compiler warnings about unused arguments
    c=c;
    
    if(msg==MSG_DRAW)
    {
        textout_ex(screen,font,"0123456789ABCDEF",d->x+8,d->y,d->fg,d->bg);
        textout_ex(screen,font,"0",d->x,d->y+8,d->fg,d->bg);
        textout_ex(screen,font,"1",d->x,d->y+16,d->fg,d->bg);
        textout_ex(screen,font,"5",d->x,d->y+24,d->fg,d->bg);
        
        for(int i=0; i<32; i++)
        {
            int px2 = d->x+((i&15)<<3)+8;
            int py2 = d->y+((i>>4)<<3)+8;
            rectfill(screen,px2,py2,px2+7,py2+7,i);
        }
        
        for(int i=0; i<16; i++)
        {
            int px2 = d->x+(i<<3)+8;
            rectfill(screen,px2,d->y+24,px2+7,d->y+31,i+80);
        }
    }
    
    return D_O_K;
}

int hexclicked=-1;

int d_misccolors_hexedit_proc(int msg,DIALOG *d,int c)
{
    switch(msg)
    {
    case MSG_GOTFOCUS:
        hexclicked=((int)(size_t)(d->dp3))+20;
        break;
        
    case MSG_LOSTFOCUS:
        hexclicked=-1;
        break;
    }
    
    return d_hexedit_proc(msg,d,c);
}


int d_misccolors_proc(int msg,DIALOG *d,int c);

static int misccolor1_list[] =
{
    // dialog control number
    4, 5, 6, 7, 8, 20, 21, 22, 23, 24, 36, 37, 38, 39, 40, -1
};

static int misccolor2_list[] =
{
    // dialog control number
    9, 10, 11, 12, 13, 25, 26, 27, 28, 29, 41, 42, 43, 44, 45, -1
};

static int misccolor3_list[] =
{
    // dialog control number
    14, 15, 16, 17, 18, 30, 31, 32, 33, 34, 46, 47, 48, 49, 50, -1
};

static int misccolor4_list[] =
{
    19, 35, 51, 54, 55, 56, -1
};

static TABPANEL misccolor_tabs[] =
{
    // (text)
    { (char *)"1",  D_SELECTED,  misccolor1_list, 0, NULL },
    { (char *)"2",  0,           misccolor2_list, 0, NULL },
    { (char *)"3",  0,           misccolor3_list, 0, NULL },
    { (char *)"4",  0,           misccolor4_list, 0, NULL },
    { NULL,         0,           NULL, 0, NULL }
};

int d_misccolors_tab_proc(int msg,DIALOG *d,int c);

static DIALOG misccolors_dlg[] =
{
    // (dialog proc)        (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp)
    { jwin_win_proc,        2,   21,    316,  197-23,  vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "Misc Colors", NULL, NULL },
    //  { jwin_frame_proc,      98-84+1+2,   52+8-6+4,   132,  100,  vc(15),  vc(1),  0,       0,          FR_DEEP,             0,       NULL, NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { d_misccolors_proc,    92-84+1+2,   44+8-6+4,   128+8,  96+8,   vc(9),   vc(1),  0,       0,          0,             0,       NULL, NULL, NULL },
    //3
    { d_misccolors_tab_proc,  150+14-2+10-15,   60-14,  150-10+15,  144-20-10,    jwin_pal[jcBOXFG],  jwin_pal[jcBOX],      0,      0,          0,             0, (void *) misccolor_tabs, NULL, (void *)misccolors_dlg },
    //4
    { jwin_text_proc,       215-25-12-15,    76-4,     0,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Text:", NULL, NULL },
    { jwin_text_proc,       215-25-12-15,    94-4,     0,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Caption:", NULL, NULL },
    { jwin_text_proc,       215-25-12-15,    112-4,    0,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Overworld Minmap:", NULL, NULL },
    { jwin_text_proc,       215-25-12-15,    130-4,    0,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Minimap Background:", NULL, NULL },
    { jwin_text_proc,       215-25-12-15,    148-4,    0,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Minimap Foreground 1:", NULL, NULL },
    { jwin_text_proc,       215-25-12-15,   76-4,      0,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Minimap Foreground 2:", NULL, NULL },
    { jwin_text_proc,       215-25-12-15,   94-4,      0,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "BS Minimap Dark:", NULL, NULL },
    { jwin_text_proc,       215-25-12-15,   112-4,     0,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "BS Minimap Goal:", NULL, NULL },
    { jwin_text_proc,       215-25-12-15,   130-4,     0,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Compass Mark (Light):", NULL, NULL },
    { jwin_text_proc,       215-25-12-15,   148-4,     0,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Compass Mark (Dark):", NULL, NULL },
    { jwin_text_proc,       215-25-12-15,   76-4,      0,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Subscreen Background:", NULL, NULL },
    { jwin_text_proc,       215-25-12-15,   94-4,      0,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Subscreen Shadow:", NULL, NULL },
    { jwin_text_proc,       215-25-12-15,   112-4,     0,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Triforce Frame:", NULL, NULL },
    { jwin_text_proc,       215-25-12-15,   130-4,     0,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Big Map Background:", NULL, NULL },
    { jwin_text_proc,       215-25-12-15,   148-4,     0,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Big Map Foreground:", NULL, NULL },
    { jwin_text_proc,       215-25-12-15,   76-4,      0,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Link's Position:", NULL, NULL },
    
    //20
    { d_misccolors_hexedit_proc,       294-25+14+2,   76-8,    21,   16,    vc(11),  vc(1),  0,       0,          2,             0,       NULL, NULL, (void *)0, },
    { d_misccolors_hexedit_proc,       294-25+14+2,   94-8,    21,   16,    vc(11),  vc(1),  0,       0,          2,             0,       NULL, NULL, (void *)1, },
    { d_misccolors_hexedit_proc,       294-25+14+2,   112-8,   21,   16,    vc(11),  vc(1),  0,       0,          2,             0,       NULL, NULL, (void *)2, },
    { d_misccolors_hexedit_proc,       294-25+14+2,   130-8,   21,   16,    vc(11),  vc(1),  0,       0,          2,             0,       NULL, NULL, (void *)3, },
    { d_misccolors_hexedit_proc,       294-25+14+2,   148-8,   21,   16,    vc(11),  vc(1),  0,       0,          2,             0,       NULL, NULL, (void *)4, },
    { d_misccolors_hexedit_proc,       294-25+14+2,   76-8,    21,   16,    vc(11),  vc(1),  0,       0,          2,             0,       NULL, NULL, (void *)5, },
    { d_misccolors_hexedit_proc,       294-25+14+2,   94-8,    21,   16,    vc(11),  vc(1),  0,       0,          2,             0,       NULL, NULL, (void *)6, },
    { d_misccolors_hexedit_proc,       294-25+14+2,   112-8,   21,   16,    vc(11),  vc(1),  0,       0,          2,             0,       NULL, NULL, (void *)7, },
    { d_misccolors_hexedit_proc,       294-25+14+2,   130-8,   21,   16,    vc(11),  vc(1),  0,       0,          2,             0,       NULL, NULL, (void *)8, },
    { d_misccolors_hexedit_proc,       294-25+14+2,   148-8,   21,   16,    vc(11),  vc(1),  0,       0,          2,             0,       NULL, NULL, (void *)9, },
    { d_misccolors_hexedit_proc,       294-25+14+2,   76-8,    21,   16,    vc(11),  vc(1),  0,       0,          2,             0,       NULL, NULL, (void *)10, },
    { d_misccolors_hexedit_proc,       294-25+14+2,   94-8,    21,   16,    vc(11),  vc(1),  0,       0,          2,             0,       NULL, NULL, (void *)11, },
    { d_misccolors_hexedit_proc,       294-25+14+2,   112-8,   21,   16,    vc(11),  vc(1),  0,       0,          2,             0,       NULL, NULL, (void *)12, },
    { d_misccolors_hexedit_proc,       294-25+14+2,   130-8,   21,   16,    vc(11),  vc(1),  0,       0,          2,             0,       NULL, NULL, (void *)13, },
    { d_misccolors_hexedit_proc,       294-25+14+2,   148-8,   21,   16,    vc(11),  vc(1),  0,       0,          2,             0,       NULL, NULL, (void *)14, },
    { d_misccolors_hexedit_proc,       294-25+14+2,   76-8,    21,   16,    vc(11),  vc(1),  0,       0,          2,             0,       NULL, NULL, (void *)15, },
    
    //36
    { jwin_text_proc,       283-25+14+2,    76-4,     0,    8,    vc(11),  vc(1),  0,       0,          2,             0, (void *) "0x", NULL, NULL },
    { jwin_text_proc,       283-25+14+2,    94-4,     0,    8,    vc(11),  vc(1),  0,       0,          2,             0, (void *) "0x", NULL, NULL },
    { jwin_text_proc,       283-25+14+2,    112-4,    0,    8,    vc(11),  vc(1),  0,       0,          2,             0, (void *) "0x", NULL, NULL },
    { jwin_text_proc,       283-25+14+2,    130-4,    0,    8,    vc(11),  vc(1),  0,       0,          2,             0, (void *) "0x", NULL, NULL },
    { jwin_text_proc,       283-25+14+2,    148-4,    0,    8,    vc(11),  vc(1),  0,       0,          2,             0, (void *) "0x", NULL, NULL },
    { jwin_text_proc,       283-25+14+2,   76-4,     0,    8,    vc(11),  vc(1),  0,       0,          2,             0, (void *) "0x", NULL, NULL },
    { jwin_text_proc,       283-25+14+2,   94-4,     0,    8,    vc(11),  vc(1),  0,       0,          2,             0, (void *) "0x", NULL, NULL },
    { jwin_text_proc,       283-25+14+2,   112-4,    0,    8,    vc(11),  vc(1),  0,       0,          2,             0, (void *) "0x", NULL, NULL },
    { jwin_text_proc,       283-25+14+2,   130-4,    0,    8,    vc(11),  vc(1),  0,       0,          2,             0, (void *) "0x", NULL, NULL },
    { jwin_text_proc,       283-25+14+2,   148-4,    0,    8,    vc(11),  vc(1),  0,       0,          2,             0, (void *) "0x", NULL, NULL },
    { jwin_text_proc,       283-25+14+2,   76-4,     0,    8,    vc(11),  vc(1),  0,       0,          2,             0, (void *) "0x", NULL, NULL },
    { jwin_text_proc,       283-25+14+2,   94-4,     0,    8,    vc(11),  vc(1),  0,       0,          2,             0, (void *) "0x", NULL, NULL },
    { jwin_text_proc,       283-25+14+2,   112-4,    0,    8,    vc(11),  vc(1),  0,       0,          2,             0, (void *) "0x", NULL, NULL },
    { jwin_text_proc,       283-25+14+2,   130-4,    0,    8,    vc(11),  vc(1),  0,       0,          2,             0, (void *) "0x", NULL, NULL },
    { jwin_text_proc,       283-25+14+2,   148-4,    0,    8,    vc(11),  vc(1),  0,       0,          2,             0, (void *) "0x", NULL, NULL },
    { jwin_text_proc,       283-25+14+2,   76-4,     0,    8,    vc(11),  vc(1),  0,       0,          2,             0, (void *) "0x", NULL, NULL },
    
    //52
    { jwin_button_proc,     90,   190-20,  61,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "OK", NULL, NULL },
    { jwin_button_proc,     170,  190-20,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { jwin_text_proc,       215-25-12-15,   94-4,      0,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Message Text:", NULL, NULL },
    { d_misccolors_hexedit_proc,       294-25+14+2,   94-8,    21,   16,    vc(11),  vc(1),  0,       0,          2,             0,       NULL, NULL, (void *)35, },
    { jwin_text_proc,		  283-25+14+2,   94-4,    0,    8,    vc(11),  vc(1),  0,       0,          2,             0, (void *) "0x", NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

int d_misccolors_tab_proc(int msg,DIALOG *d,int c)
{

    switch(msg)
    {
    case MSG_WANTFOCUS:
        return D_WANTFOCUS;
        break;
    }
    
    return jwin_tab_proc(msg,d,c);
}


int d_misccolors_proc(int msg,DIALOG *d,int c)
{
    //these are here to bypass compiler warnings about unused arguments
    c=c;
    int mul=8;
    
    if(is_large())
        mul=(int)(mul*1.5);
        
    switch(msg)
    {
    case MSG_CLICK:
        if(hexclicked!=-1)
        {
            int color_col=vbound(((Backend::mouse->getVirtualScreenX()-d->x-8)/mul),0,15);
            int color_row=vbound(((Backend::mouse->getVirtualScreenY()-d->y-10)/mul),0,11);
            sprintf((char*)misccolors_dlg[hexclicked].dp,"%X%X",color_row,color_col);
            object_message(misccolors_dlg+hexclicked,MSG_DRAW,0);
        }
        
        break;
        
    case MSG_DRAW:
        for(int i=0; i<10; i++)
        {
            textprintf_centre_ex(screen,font,d->x+8+4+(i*mul),d->y,jwin_pal[jcBOXFG],jwin_pal[jcBOX], "%d", i);
        }
        
        for(int i=0; i<6; i++)
        {
            textprintf_centre_ex(screen,font,d->x+8+4+((10+i)*mul),d->y,jwin_pal[jcBOXFG],jwin_pal[jcBOX], "%c", i+'A');
        }
        
        for(int i=0; i<10; i++)
        {
            textprintf_right_ex(screen,font,d->x+6,d->y+(i*mul)+10,jwin_pal[jcBOXFG],jwin_pal[jcBOX], "%d", i);
        }
        
        for(int i=0; i<2; i++)
        {
            textprintf_right_ex(screen,font,d->x+6,d->y+((i+10)*mul)+10,jwin_pal[jcBOXFG],jwin_pal[jcBOX], "%c", i+'A');
        }
        
		jwin_draw_frame(screen, d->x + 6, d->y + 8, int(132 * (is_large() ? 1.5 : 1)) - (is_large() ? 2 : 1), int(100 * (is_large() ? 1.5 : 1)) - (is_large() ? 2 : 1), FR_DEEP);
        
        for(int i=0; i<192; i++)
        {
            int px2 = d->x+int(((i&15)<<3)*(is_large()?1.5 : 1))+8;
            int py2 = d->y+int(((i>>4)<<3)*(is_large()?1.5 : 1))+8+2;
            rectfill(screen,px2,py2,px2+(mul-1),py2+(mul-1),i);
        }
        
        break;
    }
    
    return D_O_K;
}


int onMiscColors()
{
    char buf[17][3];
    byte *si = &(misc.colors.text);
    misccolors_dlg[0].dp2=lfont;
    
    for(int i=0; i<16; i++)
    {
        sprintf(buf[i],"%02X",*(si++));
        sprintf(buf[16], "%02X", misc.colors.msgtext);
        misccolors_dlg[i+20].dp = buf[i];
        misccolors_dlg[55].dp = buf[16];
    }

	DIALOG *misccolors_cpy = resizeDialog(misccolors_dlg, 1.5);
    
    if(zc_popup_dialog(misccolors_cpy,0)==52)
    {
        saved=false;
        si = &(misc.colors.text);
        
        for(int i=0; i<16; i++)
        {
            *si = xtoi(buf[i]);
            ++si;
        }
        
        misc.colors.msgtext = xtoi(buf[16]);
    }

	delete[] misccolors_cpy;
    
    return D_O_K;
}

// ****  Palette cycling  ****

static int palclk[3];
static int palpos[3];

void reset_pal_cycling()
{
    for(int i=0; i<3; i++)
        palclk[i]=palpos[i]=0;
}

void cycle_palette()
{
    if(!get_bit(quest_rules,qr_FADE))
        return;
        
    int level = Map.CurrScr()->color;
    bool refreshpal = false;
    
    for(int i=0; i<3; i++)
    {
        palcycle c = misc.cycles[level][i];
        
        if(c.count&0xF0)
        {
            if(++palclk[i] >= c.speed)
            {
                palclk[i]=0;
                
                if(++palpos[i] >= (c.count>>4))
                    palpos[i]=0;
                    
                byte *si = colordata + CSET(level*pdLEVEL+poFADE1+1+palpos[i])*3;
                
                si += (c.first&15)*3;
                
                for(int col=c.first&15; col<=(c.count&15); col++)
                {
                    RAMpal[CSET(c.first>>4)+col] = _RGB(si);
                    si+=3;
                }
                
                refreshpal = true;
            }
        }
    }
    
    if(refreshpal)
    {
        rebuild_trans_table();
        Backend::palette->setPaletteRange(RAMpal,0,192);
    }
}


/********************/
/******  Help  ******/
/********************/

static DIALOG help_dlg[] =
{
    /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)      (d2)      (dp) */
//  { jwin_textbox_proc,    4,   2+21,   320-8,  240-6-21,  0,       0,      0,       0,          0,        0,        NULL, NULL, NULL },
    { jwin_win_proc,        0,   0,  0,  0,  0,       vc(15), 0,      D_EXIT,       0,          0, (void *) "ZQuest Help", NULL, NULL },
    { jwin_frame_proc,   4,   23,   0,  0,   0,       0,      0,       0,             FR_DEEP,       0,       NULL, NULL, NULL },
    { d_editbox_proc,    6,   25,   0,  0,  0,       0,      0,       0/*D_SELECTED*/,          0,        0,       NULL, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,       0,      0,       0,          0,        KEY_ESC, (void *) close_dlg, NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,       0,      0,       0,          0,        KEY_F12, (void *) onSnapshot, NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

void doHelp(int bg,int fg)
{
	help_dlg[0].w = is_large() ? 800 : 320;
	help_dlg[0].h = is_large() ? 600 : 240;
	help_dlg[1].w = is_large() ? 800 - 8 : 320 - 8;
	help_dlg[1].h = is_large() ? 600 - 27 : 240 - 27;
	help_dlg[2].w = is_large() ? 800 - 8 - 4 : 320 - 8 - 4;
	help_dlg[2].h = is_large() ? 600 - 27 - 4 : 240 - 270 - 4;
    help_dlg[0].dp2= lfont;
    help_dlg[2].dp = new EditboxModel(helpstr, new EditboxWordWrapView(&help_dlg[2],is_large()?sfont3:font,fg,bg,BasicEditboxView::HSTYLE_EOTEXT),true);
    help_dlg[2].bg = bg;
	jwin_center_dialog(help_dlg);
    zc_popup_dialog(help_dlg,2);
    delete(EditboxModel*)(help_dlg[2].dp);
}

int onHelp()
{
    restore_mouse();
    doHelp(vc(15),vc(0));
    return D_O_K;
}

static DIALOG layerdata_dlg[] =
{
    /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp) */
    { jwin_win_proc,     16-12,   20+32,   288+1+24,  200+1-32-16,  vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "Layer Data", NULL, NULL },
    { jwin_button_proc,     170,  180,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { jwin_button_proc,     90,   180,  61,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "OK", NULL, NULL },
    // 3
    { jwin_rtext_proc,       72,   88,    40,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Map:", NULL, NULL },
    { jwin_rtext_proc,       72,   88+18,    48,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Screen:", NULL, NULL },
    { jwin_rtext_proc,       72,   88+36,    56,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "Transparent:", NULL, NULL },
    { jwin_ctext_proc,       89,  76,   8,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "1", NULL, NULL },
    { jwin_ctext_proc,       89+40,  76,   8,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "2", NULL, NULL },
    { jwin_ctext_proc,       89+80,  76,   8,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "3", NULL, NULL },
    { jwin_ctext_proc,       89+120,  76,   8,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "4", NULL, NULL },
    { jwin_ctext_proc,       89+160,  76,   8,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "5", NULL, NULL },
    { jwin_ctext_proc,       89+200,  76,   8,  8,    vc(11),  vc(1),  0,       0,          0,             0, (void *) "6", NULL, NULL },
    
    //12
    { jwin_edit_proc,      76,   76+8,   32-6,   16,    vc(12),  vc(1),  0,       0,          3,             0,       NULL, NULL, NULL },
    { d_hexedit_proc,      76,   76+18+8,   24-3,   16,    vc(12),  vc(1),  0,       0,          2,             0,       NULL, NULL, NULL },
    { jwin_check_proc,     76,   76+40+8,   17,   9,    vc(12),  vc(1),  0,       0,          1,             0,       NULL, NULL, NULL },
    
    { jwin_edit_proc,      76+40,   76+8,   32-6,   16,    vc(12),  vc(1),  0,       0,          3,             0,       NULL, NULL, NULL },
    { d_hexedit_proc,      76+40,   76+18+8,   24-3,   16,    vc(12),  vc(1),  0,       0,          2,             0,       NULL, NULL, NULL },
    { jwin_check_proc,     76+40,  76+40+8,   17,   9,    vc(12),  vc(1),  0,       0,          1,             0,       NULL, NULL, NULL },
    
    { jwin_edit_proc,      76+80,   76+8,   32-6,   16,    vc(12),  vc(1),  0,       0,          3,             0,       NULL, NULL, NULL },
    { d_hexedit_proc,      76+80,   76+18+8,   24-3,   16,    vc(12),  vc(1),  0,       0,          2,             0,       NULL, NULL, NULL },
    { jwin_check_proc,     76+80,  76+40+8,   17,   9,    vc(12),  vc(1),  0,       0,          1,             0,       NULL, NULL, NULL },
    
    { jwin_edit_proc,      76+120,   76+8,   32-6,   16,    vc(12),  vc(1),  0,       0,          3,             0,       NULL, NULL, NULL },
    { d_hexedit_proc,      76+120,   76+18+8,   24-3,   16,    vc(12),  vc(1),  0,       0,          2,             0,       NULL, NULL, NULL },
    { jwin_check_proc,     76+120,  76+40+8,   17,   9,    vc(12),  vc(1),  0,       0,          1,             0,       NULL, NULL, NULL },
    
    { jwin_edit_proc,      76+160,   76+8,   32-6,   16,    vc(12),  vc(1),  0,       0,          3,             0,       NULL, NULL, NULL },
    { d_hexedit_proc,      76+160,   76+18+8,   24-3,   16,    vc(12),  vc(1),  0,       0,          2,             0,       NULL, NULL, NULL },
    { jwin_check_proc,     76+160,  76+40+8,   17,   9,    vc(12),  vc(1),  0,       0,          1,             0,       NULL, NULL, NULL },
    
    { jwin_edit_proc,      76+200,   76+8,   32-6,   16,    vc(12),  vc(1),  0,       0,          3,             0,       NULL, NULL, NULL },
    { d_hexedit_proc,      76+200,   76+18+8,   24-3,   16,    vc(12),  vc(1),  0,       0,          2,             0,       NULL, NULL, NULL },
    { jwin_check_proc,     76+200,  76+40+8,   17,   9,    vc(12),  vc(1),  0,       0,          1,             0,       NULL, NULL, NULL },
    
    //30
    { jwin_button_proc,     76,  76+40+18+8,  30,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Auto", NULL, NULL },
    { jwin_button_proc,     76+40,  76+40+18+8,  30,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Auto", NULL, NULL },
    { jwin_button_proc,     76+80,  76+40+18+8,  30,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Auto", NULL, NULL },
    { jwin_button_proc,     76+120,  76+40+18+8,  30,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Auto", NULL, NULL },
    { jwin_button_proc,     76+160,  76+40+18+8,  30,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Auto", NULL, NULL },
    { jwin_button_proc,     76+200,  76+40+18+8,  30,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Auto", NULL, NULL },
    
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
    
};

int edit_layers(mapscr* tempscr)
{
    char buf[6][2][8];
    layerdata_dlg[0].dp2 = lfont;
    
    for(int x=0; x<6; x++)
    {
        sprintf(buf[x][0],"%d",tempscr->layermap[x]);
        sprintf(buf[x][1],"%02X",tempscr->layerscreen[x]);
    }
    
    for(int x=0; x<6; x++)
    {
        for(int y=0; y<2; y++)
        {
            layerdata_dlg[(x*3)+y+12].dp = buf[x][y];
        }
    }
    
    for(int x=0; x<6; x++)
    {
        layerdata_dlg[(x*3)+2+12].flags = (tempscr->layeropacity[x]<255) ? D_SELECTED : 0;
    }

	DIALOG *layerdata_cpy = resizeDialog(layerdata_dlg, 1.5);
        
    int ret=zc_popup_dialog(layerdata_cpy,0);
    
    if(ret>=2)
    {
        for(int x=0; x<6; x++)
        {
        
            tempscr->layermap[x]=atoi(buf[x][0]);
            
            if(tempscr->layermap[x]>map_count)
            {
                tempscr->layermap[x]=0;
            }
            
            tempscr->layerscreen[x]=xtoi(buf[x][1]);
            
            if(xtoi(buf[x][1])>=MAPSCRS)
            {
                tempscr->layerscreen[x]=0;
            }
            
            //      tempscr->layeropacity[x]=layerdata_dlg[(x*9)+8+19].flags & D_SELECTED ? 128:255;
            tempscr->layeropacity[x]= layerdata_cpy[(x*3)+2+12].flags & D_SELECTED ? 128:255;
        }
        
        //  } else if (ret>72&&ret<79) {
        //    return (ret-72);
    }

	delete[] layerdata_cpy;
    
    return ret;
}

static DIALOG autolayer_dlg[] =
{
    /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)     (bg)    (key)    (flags)     (d1)           (d2)     (dp) */
    { jwin_win_proc,        64,   32+48,   192+1,  184+1-64-28,  vc(14),  vc(1),  0,       D_EXIT,          0,             0, (void *) "Autolayer Setup", NULL, NULL },
    { jwin_text_proc,       76,   56+48,   136,   8,    vc(14),  vc(1),  0,       0,          0,             0, (void *) "Map for layer ?: ", NULL, NULL },
    { jwin_edit_proc,       212,  56+48,   32,   16,    vc(12),  vc(1),  0,       0,          3,             0,       NULL, NULL, NULL },
    { jwin_check_proc,      76,   56+18+48,   153,   8,    vc(14),  vc(1),  0,       0,          1,             0, (void *) "Overwrite current", NULL, NULL },
    
    //5
    { jwin_button_proc,     90,   188-40,  61,   21,   vc(14),  vc(1),  13,      D_EXIT,     0,             0, (void *) "OK", NULL, NULL },
    { jwin_button_proc,     170,  188-40,  61,   21,   vc(14),  vc(1),  27,      D_EXIT,     0,             0, (void *) "Cancel", NULL, NULL },
    { d_keyboard_proc,   0,    0,    0,    0,    0,    0,    0,       0,       KEY_F1,   0, (void *) onHelp, NULL, NULL },
    { d_timer_proc,         0,    0,     0,    0,    0,       0,       0,       0,          0,          0,         NULL, NULL, NULL },
    { NULL,                 0,    0,    0,    0,   0,       0,       0,       0,          0,             0,       NULL,                           NULL,  NULL }
};

void autolayer(mapscr* tempscr, int layer, int al[6][3])
{
    char tbuf[80],mlayer[80];
    autolayer_dlg[0].dp2=lfont;
    sprintf(tbuf, "Map for layer %d: ", layer+1);
    autolayer_dlg[1].dp=tbuf;
    sprintf(mlayer, "%d", tempscr->layermap[layer]);
    autolayer_dlg[2].dp=mlayer;

	DIALOG *autolayer_cpy = resizeDialog(autolayer_dlg, 1.5);
    
    int ret=zc_popup_dialog(autolayer_cpy,0);
    
    if(ret==4)
    {
        int lmap=vbound(atoi(mlayer),0,Map.getMapCount());
        al[layer][0]=lmap;
        tempscr->layermap[layer]=lmap;
        tempscr->layerscreen[layer]=Map.getCurrScr();
        al[layer][1]= autolayer_cpy[3].flags & D_SELECTED?1:0;
        al[layer][2]=1;
    }

	delete[] autolayer_cpy;
}

int findblankcombo()
{
    for(int i=0; i<MAXCOMBOS; i++)
    {
    
        if(!combobuf[i].flip&&!combobuf[i].walk&&!combobuf[i].type&&
                !combobuf[i].csets&&!combobuf[i].frames&&!combobuf[i].speed&&
                !combobuf[i].nextcombo&&!combobuf[i].nextcset&&
                blank_tile_table[combobuf[i].tile])
        {
            return i;
        }
    }
    
    return 0;
}

int onLayers()
{
    mapscr tempscr=*Map.CurrScr();
    int blankcombo=findblankcombo();
    int al[6][3];                                             //autolayer[layer][0=map, 1=overwrite current][go]
    
    for(int i=0; i<6; i++)
    {
        al[i][0]=tempscr.layermap[i];
        al[i][1]=0;
        al[i][2]=0;
    }
    
    int ret;
    
    do
    {
        ret=edit_layers(&tempscr);
        
        if(ret>2)                                               //autolayer button
        {
            autolayer(&tempscr, ret-30, al);
        }
    }
    while(ret>2);                                             //autolayer button
    
    if(ret==2)                                                //OK
    {
        saved=false;
        TheMaps[Map.getCurrMap()*MAPSCRS+Map.getCurrScr()]=tempscr;
        
        for(int i=0; i<6; i++)
        {
            int tm=tempscr.layermap[i]-1;
            
            if(tm!=al[i][0]-1)
            {
                al[i][2]=0;
            }
            
            int ts=tempscr.layerscreen[i];
            
            if(tm>0)
            {
                if(!(TheMaps[tm*MAPSCRS+ts].valid&mVALID))
                {
                    TheMaps[tm*MAPSCRS+ts].valid=mVALID+mVERSION;
                    
                    for(int k=0; k<176; k++)
                    {
                        TheMaps[tm*MAPSCRS+ts].data[k]=blankcombo;
                    }
                }
            }
            
            if(al[i][2]>0)
            {
                for(int j=0; j<128; j++)
                {
                    if((TheMaps[Map.getCurrMap()*MAPSCRS+j].layermap[i]==0) || (al[i][1]))
                    {
                        if(TheMaps[Map.getCurrMap()*MAPSCRS+j].layermap[i]==0)
                        {
                        }
                        
                        if((TheMaps[Map.getCurrMap()*MAPSCRS+j].layermap[i]==0) && (al[i][1]))
                        {
                        }
                        
                        if(al[i][1])
                        {
                        }
                        
                        TheMaps[Map.getCurrMap()*MAPSCRS+j].layermap[i]=al[i][0];
                        TheMaps[Map.getCurrMap()*MAPSCRS+j].layerscreen[i]=al[i][0]?j:0;
                        
                        if(al[i][0])
                        {
                            if(!(TheMaps[(al[i][0]-1)*MAPSCRS+j].valid&mVALID))
                            {
                                TheMaps[(al[i][0]-1)*MAPSCRS+j].valid=mVALID+mVERSION;
                                
                                for(int k=0; k<176; k++)
                                {
                                    TheMaps[(al[i][0]-1)*MAPSCRS+j].data[k]=blankcombo;
                                }
                            }
                        }
                    }
                }
            }
        }
        
        Map.Ugo();
    }
    
    // Check that the working layer wasn't just disabled
    if(CurrentLayer>0 && tempscr.layermap[CurrentLayer-1]==0)
    {
        CurrentLayer=0;
    }
    
    return D_O_K;
}


char *itoa(int i)
{
    static char itoaret[500];
    sprintf(itoaret, "%d", i);
    return itoaret;
}

//unsigned int col_diff[3*128];
/*
  void bestfit_init(void)
  {
  int i;

  for (i=1; i<64; i++)

  {
  int k = i * i;
  col_diff[0  +i] = col_diff[0  +128-i] = k * (59 * 59);
  col_diff[128+i] = col_diff[128+128-i] = k * (30 * 30);
  col_diff[256+i] = col_diff[256+128-i] = k * (11 * 11);
  }
  }
  */
void create_rgb_table2(RGB_MAP *table, AL_CONST PALETTE pal, void (*callback)(int pos))
{
#define UNUSED 65535
#define LAST 65532

    /* macro add adds to single linked list */
#define add(i)    (next[(i)] == UNUSED ? (next[(i)] = LAST, \
                                          (first != LAST ? (next[last] = (i)) : (first = (i))), \
                                          (last = (i))) : 0)
    
    /* same but w/o checking for first element */
#define add1(i)   (next[(i)] == UNUSED ? (next[(i)] = LAST, \
                                          next[last] = (i), \
                                          (last = (i))) : 0)
    /* calculates distance between two colors */
#define dist(a1, a2, a3, b1, b2, b3) \
          (col_diff[ ((a2) - (b2)) & 0x7F] + \
           (col_diff + 128)[((a1) - (b1)) & 0x7F] + \
           (col_diff + 256)[((a3) - (b3)) & 0x7F])
    
    /* converts r,g,b to position in array and back */
#define pos(r, g, b) \
          (((r) / 2) * 32 * 32 + ((g) / 2) * 32 + ((b) / 2))
    
#define depos(pal, r, g, b) \
          ((b) = ((pal) & 31) * 2, \
           (g) = (((pal) >> 5) & 31) * 2, \
           (r) = (((pal) >> 10) & 31) * 2)
    
    /* is current color better than pal1? */
#define better(r1, g1, b1, pal1) \
          (((int)dist((r1), (g1), (b1), \
                      (pal1).r, (pal1).g, (pal1).b)) > (int)dist2)
    
    /* checking of position */
#define dopos(rp, gp, bp, ts) \
          if ((rp > -1 || r > 0) && (rp < 1 || r < 61) && \
              (gp > -1 || g > 0) && (gp < 1 || g < 61) && \
              (bp > -1 || b > 0) && (bp < 1 || b < 61)) \
        {                     \
          i = first + rp * 32 * 32 + gp * 32 + bp; \
          if (!data[i])       \
          {                   \
            data[i] = val;    \
            add1(i);          \
          }                   \
          else if ((ts) && (data[i] != val)) \
            {                 \
              dist2 = (rp ? (col_diff+128)[(r+2*rp-pal[val].r) & 0x7F] : r2) + \
                (gp ? (col_diff    )[(g+2*gp-pal[val].g) & 0x7F] : g2) + \
                (bp ? (col_diff+256)[(b+2*bp-pal[val].b) & 0x7F] : b2); \
              if (better((r+2*rp), (g+2*gp), (b+2*bp), pal[data[i]])) \
              {               \
                data[i] = val; \
                add1(i);      \
              }               \
            }                 \
        }
    
    int i, curr, r, g, b, val, dist2;
    unsigned int r2, g2, b2;
    unsigned short next[32*32*32];
    unsigned char *data;
    int first = LAST;
    int last = LAST;
    int count = 0;
    int cbcount = 0;
    
#define AVERAGE_COUNT   18000
    
    if(col_diff[1] == 0)
        bestfit_init();
        
    memset(next, 255, sizeof(next));
    memset(table->data, 0, sizeof(char)*32*32*32);
    
    
    data = (unsigned char *)table->data;
    
    /* add starting seeds for floodfill */
    for(i=1; i<PAL_SIZE; i++)
    {
        curr = pos(pal[i].r, pal[i].g, pal[i].b);
        
        if(next[curr] == UNUSED)
        {
            data[curr] = i;
            add(curr);
        }
    }
    
    /* main floodfill: two versions of loop for faster growing in blue axis */
    //   while (first != LAST) {
    while(first < LAST)
    {
        depos(first, r, g, b);
        
        /* calculate distance of current color */
        val = data[first];
        r2 = (col_diff+128)[((pal[val].r)-(r)) & 0x7F];
        g2 = (col_diff)[((pal[val].g)-(g)) & 0x7F];
        b2 = (col_diff+256)[((pal[val].b)-(b)) & 0x7F];
        
        /* try to grow to all directions */
#ifdef _MSC_VER
#pragma warning(disable:4127)
#endif
        dopos(0, 0, 1, 1);
        dopos(0, 0,-1, 1);
        dopos(1, 0, 0, 1);
        dopos(-1, 0, 0, 1);
        dopos(0, 1, 0, 1);
        dopos(0,-1, 0, 1);
#ifdef _MSC_VER
#pragma warning(default:4127)
#endif
        
        /* faster growing of blue direction */
        if((b > 0) && (data[first-1] == val))
        {
            b -= 2;
            first--;
            b2 = (col_diff+256)[((pal[val].b)-(b)) & 0x7F];
            
#ifdef _MSC_VER
#pragma warning(disable:4127)
#endif
            dopos(-1, 0, 0, 0);
            dopos(1, 0, 0, 0);
            dopos(0,-1, 0, 0);
            dopos(0, 1, 0, 0);
#ifdef _MSC_VER
#pragma warning(default:4127)
#endif
            
            first++;
        }
        
        /* get next from list */
        i = first;
        first = next[first];
        next[i] = UNUSED;
        
        /* second version of loop */
        //      if (first != LAST) {
        if(first < LAST)
        {
        
            depos(first, r, g, b);
            
            val = data[first];
            r2 = (col_diff+128)[((pal[val].r)-(r)) & 0x7F];
            g2 = (col_diff)[((pal[val].g)-(g)) & 0x7F];
            b2 = (col_diff+256)[((pal[val].b)-(b)) & 0x7F];
            
#ifdef _MSC_VER
#pragma warning(disable:4127)
#endif
            dopos(0, 0, 1, 1);
            dopos(0, 0,-1, 1);
            dopos(1, 0, 0, 1);
            dopos(-1, 0, 0, 1);
            dopos(0, 1, 0, 1);
            dopos(0,-1, 0, 1);
#ifdef _MSC_VER
#pragma warning(default:4127)
#endif
            
            if((b < 61) && (data[first + 1] == val))
            {
                b += 2;
                first++;
                b2 = (col_diff+256)[((pal[val].b)-(b)) & 0x7f];
#ifdef _MSC_VER
#pragma warning(disable:4127)
#endif
                dopos(-1, 0, 0, 0);
                dopos(1, 0, 0, 0);
                dopos(0,-1, 0, 0);
                dopos(0, 1, 0, 0);
#ifdef _MSC_VER
#pragma warning(default:4127)
#endif
                
                first--;
            }
            
            i = first;
            first = next[first];
            next[i] = UNUSED;
        }
        
        count++;
        
        if(count == (cbcount+1)*AVERAGE_COUNT/256)
        {
            if(cbcount < 256)
            {
                if(callback)
                    callback(cbcount);
                    
                cbcount++;
            }
        }
        
    }
    
    /* only the transparent (pink) color can be mapped to index 0 */
    if((pal[0].r == 63) && (pal[0].g == 0) && (pal[0].b == 63))
        table->data[31][0][31] = 0;
        
    if(callback)
        while(cbcount < 256)
            callback(cbcount++);
}

void rebuild_trans_table()
{
    create_rgb_table2(&zq_rgb_table, RAMpal, NULL);
    create_zc_trans_table(&trans_table, RAMpal, 128, 128, 128);
    memcpy(&trans_table2, &trans_table, sizeof(COLOR_MAP));
    
    for(int q=0; q<PAL_SIZE; q++)
    {
        trans_table2.data[0][q] = q;
        trans_table2.data[q][q] = q;
    }
}

int isFullScreen()
{
    return !is_windowed_mode();
}

void hit_close_button()
{
    close_button_quit=true;
    return;
}

/********************/
/******  MAIN  ******/
/********************/

/*
  enum { jcBOX, jcLIGHT, jcMEDLT, jcMEDDARK, jcDARK, jcBOXFG,
  jcTITLEL, jcTITLER, jcTITLEFG, jcTEXTBG, jcTEXTFG, jcSELBG, jcSELFG,
  jcMAX };

  enum { light gray, white, off-white, gray, dark gray, black,
  jcTITLEL, jcTITLER, jcTITLEFG, jcTEXTBG, jcTEXTFG, jcSELBG, jcSELFG,
  jcMAX };
  */

void switch_out()
{
    zcmusic_pause(zcmusic, ZCM_PAUSE);
    midi_pause();
}

void switch_in()
{
    if(quit)
        return;
        
    BITMAP *ts=screen;
    screen=menu1;
    jwin_menu_proc(MSG_DRAW, &dialogs[0], 0);
    screen=ts;
    zcmusic_pause(zcmusic, ZCM_RESUME);
    midi_resume();
}

void Z_eventlog(const char *format,...)
{
    format=format; //to prevent a compiler warning
}

void PopulateInitDialog();
int get_currdmap()
{
    return zinit.start_dmap;
}

int get_dlevel()
{
    return DMaps[zinit.start_dmap].level;
}

int get_currscr()
{
    return Map.getCurrScr();
}

int get_homescr()
{
    return DMaps[zinit.start_dmap].cont;
}

int current_item(int item_type)
{
    //TODO remove as special case?? -DD
    if(item_type==itype_shield)
    {
        return 2;
    }
    
    //find lowest item of that class
    int lowestid = -1;
    int ret = 0;
    
    for(int i=0; i<MAXITEMS; i++)
    {
        if(itemsbuf[i].family == item_type && (lowestid==-1 || itemsbuf[i].fam_type < ret))
        {
            lowestid = i;
            ret = itemsbuf[i].fam_type;
        }
    }
    
    return ret;
}

int current_item_power(int itemtype)
{
    itemtype=itemtype;
    return 1;
}

int current_item_id(int itemtype, bool checkmagic)
{
    checkmagic=checkmagic;
    
    for(int i=0; i<MAXITEMS; i++)
    {
        if(itemsbuf[i].family==itemtype)
            return i;
    }
    
    return -1;
}


bool can_use_item(int item_type, int item)
{
    //these are here to bypass compiler warnings about unused arguments
    item_type=item_type;
    item=item;
    
    return true;
}

bool has_item(int item_type, int it)
{
    //these are here to bypass compiler warnings about unused arguments
    item_type=item_type;
    it=it;
    
    return true;
}

int get_bmaps(int si)
{
    //these are here to bypass compiler warnings about unused arguments
    si=si;
    
    return 255;
}

bool no_subscreen()
{
    return false;
}

int Awpn=0, Bwpn=0, Bpos=0;
sprite_list Sitems, Lwpns;


int main(int argc, char **argv)
{
    switch(IS_BETA)
    {
    case -1:
        Z_title("ZQuest %s Alpha (Build %d)",VerStr(ZELDA_VERSION), VERSION_BUILD);
        break;
        
    case 1:
        Z_title("ZQuest %s Beta (Build %d)",VerStr(ZELDA_VERSION), VERSION_BUILD);
        break;
        
    case 0:
        Z_title("ZQuest %s (Build %d)",VerStr(ZELDA_VERSION), VERSION_BUILD);
    }
    
    // Before anything else, let's register our custom trace handler:
    register_trace_handler(zc_trace_handler);
    
    PopulateInitDialog();
    
    memrequested+=sizeof(zctune)*MAXCUSTOMMIDIS_ZQ;
    Z_message("Allocating tunes buffer (%s)... ", byte_conversion2(sizeof(zctune)*MAXCUSTOMMIDIS_ZQ,memrequested,-1,-1));
    customtunes = (zctune*)zc_malloc(sizeof(class zctune)*MAXCUSTOMMIDIS_ZQ);
    memset(customtunes, 0, sizeof(class zctune)*MAXCUSTOMMIDIS_ZQ);
    
    if(!customtunes)
    {
        Z_error("Error");
        quit_game();
    }
    
    Z_message("OK\n");                                      // Allocating MIDI buffer...
    
    /*memrequested+=sizeof(emusic)*MAXMUSIC;
    Z_message("Allocating Enhanced Music buffer (%s)... ", byte_conversion2(sizeof(emusic)*MAXMUSIC,memrequested,-1,-1));
    enhancedMusic = (emusic*)zc_malloc(sizeof(emusic)*MAXMUSIC);
    if(!enhancedMusic)
    {
      Z_error("Error");
    }
    Z_message("OK\n");                                      // Allocating Enhanced Music buffer...
    */
    if(!get_qst_buffers())
    {
        Z_error("Error");
        quit_game();
    }
    
    memrequested+=sizeof(newcombo)*MAXCOMBOS;
    Z_message("Allocating combo undo buffer (%s)... ", byte_conversion2(sizeof(newcombo)*MAXCOMBOS,memrequested,-1,-1));
    undocombobuf = (newcombo*)zc_malloc(sizeof(newcombo)*MAXCOMBOS);
    
    if(!undocombobuf)
    {
        Z_error("Error: no memory for combo undo buffer!");
        quit_game();
    }
    
    Z_message("OK\n");                                      // Allocating combo undo buffer...
    
    memrequested+=(NEWMAXTILES*sizeof(tiledata));
    Z_message("Allocating new tile undo buffer (%s)... ", byte_conversion2(NEWMAXTILES*sizeof(tiledata),memrequested,-1,-1));
    
    if((newundotilebuf=(tiledata*)zc_malloc(NEWMAXTILES*sizeof(tiledata)))==NULL)
    {
        Z_error("Error: no memory for tile undo buffer!");
        quit_game();
    }
    
    memset(newundotilebuf, 0, NEWMAXTILES*sizeof(tiledata));
    Z_message("OK\n");                                        // Allocating new tile buffer...
    
    Z_message("Resetting new tile buffer...");
    newtilebuf = (tiledata*)zc_malloc(NEWMAXTILES*sizeof(tiledata));
    
    for(int j=0; j<NEWMAXTILES; j++)
        newtilebuf[j].data=NULL;
        
    Z_message("OK\n");
    
    memrequested+=(2048*5);
    Z_message("Allocating file path buffers (%s)... ", byte_conversion2(2048*7,memrequested,-1,-1));
    filepath=(char*)zc_malloc(2048);
    temppath=(char*)zc_malloc(2048);
    datapath=(char*)zc_malloc(2048);
    midipath=(char*)zc_malloc(2048);
    imagepath=(char*)zc_malloc(2048);
    tmusicpath=(char*)zc_malloc(2048);
    last_timed_save=(char*)zc_malloc(2048);
    
    if(!filepath || !datapath || !temppath || !imagepath || !midipath || !tmusicpath || !last_timed_save)
    {
        Z_error("Error: no memory for file paths!");
        quit_game();
    }
    
    Z_message("OK\n");                                      // Allocating file path buffers...
    
    srand(time(0));
    
    
    set_uformat(U_ASCII);
    Z_message("Initializing Allegro... ");
    
    allegro_init();
	
	three_finger_flag=false;
    register_bitmap_file_type("GIF",  load_gif, save_gif);
    jpgalleg_init();
    loadpng_init();
    
    set_config_file("ag.cfg");
    
    if(install_timer() < 0)
    {
        Z_error(allegro_error);
        quit_game();
    }
    
    if(install_keyboard() < 0)
    {
        Z_error(allegro_error);
        quit_game();
    }
    
    if(install_mouse() < 0)
    {
        Z_error(allegro_error);
        quit_game();
    }
    
            
    LOCK_VARIABLE(dclick_status);
    LOCK_VARIABLE(dclick_time);
    lock_dclick_function();
    install_int(dclick_check, 20);
    
    //set_gfx_mode(GFX_TEXT,80,50,0,0);
	set_color_conversion(COLORCONV_NONE);
	Backend::initializeBackend();
    
    Z_message("OK\n");                                      // Initializing Allegro...
    
    Z_message("Loading data files:\n");
    
    resolve_password(datapwd);
    packfile_password(datapwd);
    
    
    sprintf(fontsdat_sig,"Fonts.Dat %s Build %d",VerStr(FONTSDAT_VERSION), FONTSDAT_BUILD);
    
    Z_message("Fonts.Dat...");
    
    if((fontsdata=load_datafile("fonts.dat"))==NULL)
    {
        Z_error("failed");
        quit_game();
    }
    
    if(strncmp((char*)fontsdata[0].dat,fontsdat_sig,24))
    {
        Z_error("\nIncompatible version of fonts.dat.\nPlease upgrade to %s Build %d",VerStr(FONTSDAT_VERSION), FONTSDAT_BUILD);
        quit_game();
    }
    
    Z_message("OK\n");
    
    Z_message("ZQuest.Dat...");
    
    if((zcdata=load_datafile("zquest.dat"))==NULL)
    {
        Z_error("failed");
        quit_game();
    }
    
    datafile_str=(char *)"zquest.dat";
    Z_message("OK\n");
    
    
    sprintf(qstdat_sig,"QST.Dat %s Build %d",VerStr(QSTDAT_VERSION), QSTDAT_BUILD);
    
    Z_message("QST.Dat...");
    PACKFILE *f=pack_fopen_password("qst.dat#_SIGNATURE", F_READ_PACKED, datapwd);
    
    if(!f)
    {
        Z_error("failed");
        quit_game();
    }
    
    char qstdat_read_sig[52];
    memset(qstdat_read_sig, 0, 52);
    int pos=0;
    
    while(!pack_feof(f))
    {
        if(!p_getc(&(qstdat_read_sig[pos++]),f,true))
        {
            pack_fclose(f);
            Z_error("failed");
            quit_game();
        }
    }
    
    pack_fclose(f);
    
    if(strncmp(qstdat_read_sig,qstdat_sig,22))
    {
        Z_error("\nIncompatible version of qst.dat.\nPlease upgrade to %s Build %d",VerStr(QSTDAT_VERSION), QSTDAT_BUILD);
        quit_game();
    }
    
    Z_message("OK\n");
    
    
    //setPackfilePassword(NULL);
    packfile_password("");
    
    sprintf(sfxdat_sig,"SFX.Dat %s Build %d",VerStr(SFXDAT_VERSION), SFXDAT_BUILD);
    
    Z_message("SFX.Dat...");
    
    if((sfxdata=load_datafile("sfx.dat"))==NULL)
    {
        Z_error("failed %s", allegro_error);
        quit_game();
    }
    
    if(strncmp((char*)sfxdata[0].dat,sfxdat_sig,22) || sfxdata[Z35].type != DAT_ID('S', 'A', 'M', 'P'))
    {
        Z_error("\nIncompatible version of sfx.dat.\nPlease upgrade to %s Build %d",VerStr(SFXDAT_VERSION), SFXDAT_BUILD);
        quit_game();
    }
    
    Z_message("OK\n");
    
    deffont=font;
    nfont = (FONT*)fontsdata[FONT_GUI_PROP].dat;
    font = nfont;
    pfont = (FONT*)fontsdata[FONT_8xPROP_THIN].dat;
    lfont = (FONT*)fontsdata[FONT_LARGEPROP].dat;
    lfont_l = (FONT*)fontsdata[FONT_LARGEPROP_L].dat;
    zfont = (FONT*)fontsdata[FONT_NES].dat;
    z3font = (FONT*)fontsdata[FONT_Z3].dat;
    z3smallfont = (FONT*)fontsdata[FONT_Z3SMALL].dat;
    mfont = (FONT*)fontsdata[FONT_MATRIX].dat;
    ztfont = (FONT*)fontsdata[FONT_ZTIME].dat;
    sfont = (FONT*)fontsdata[FONT_6x6].dat;
    sfont2 = (FONT*)fontsdata[FONT_6x4].dat;
    sfont3 = (FONT*)fontsdata[FONT_12x8].dat;
    spfont = (FONT*)fontsdata[FONT_6xPROP].dat;
    ssfont1 = (FONT*)fontsdata[FONT_SUBSCREEN1].dat;
    ssfont2 = (FONT*)fontsdata[FONT_SUBSCREEN2].dat;
    ssfont3 = (FONT*)fontsdata[FONT_SUBSCREEN3].dat;
    ssfont4 = (FONT*)fontsdata[FONT_SUBSCREEN4].dat;
    gblafont = (FONT*)fontsdata[FONT_GB_LA].dat;
    goronfont = (FONT*)fontsdata[FONT_GORON].dat;
    zoranfont = (FONT*)fontsdata[FONT_ZORAN].dat;
    hylian1font = (FONT*)fontsdata[FONT_HYLIAN1].dat;
    hylian2font = (FONT*)fontsdata[FONT_HYLIAN2].dat;
    hylian3font = (FONT*)fontsdata[FONT_HYLIAN3].dat;
    hylian4font = (FONT*)fontsdata[FONT_HYLIAN4].dat;
    gboraclefont = (FONT*)fontsdata[FONT_GB_ORACLE].dat;
    gboraclepfont = (FONT*)fontsdata[FONT_GB_ORACLE_P].dat;
    dsphantomfont = (FONT*)fontsdata[FONT_DS_PHANTOM].dat;
    dsphantompfont = (FONT*)fontsdata[FONT_DS_PHANTOM_P].dat;
    
    for(int i=0; i<MAXCUSTOMTUNES; i++)
    {
        customtunes[i].data=NULL;
        midi_string[i+4]=customtunes[i].title;
    }
    
    for(int i=0; i<MAXCUSTOMTUNES; i++)
    {
        customtunes[i].data=NULL;
        screen_midi_string[i+5]=customtunes[i].title;
    }
    
    for(int i=0; i<4; i++)
    {
        for(int j=0; j<MAXSUBSCREENITEMS; j++)
        {
            memset(&custom_subscreen[i].objects[j],0,sizeof(subscreen_object));
        }
    }
    
    int helpsize = file_size_ex_password("zquest.txt","");
    
    if(helpsize==0)
    {
        Z_error("Error: zquest.txt not found.");
        quit_game();
    }
    
    helpbuf = (char*)zc_malloc(helpsize<65536?65536:helpsize*2+1);
    
    if(!helpbuf)
    {
        Z_error("Error allocating help buffer.");
        quit_game();
    }
    
    //if(!readfile("zquest.txt",helpbuf,helpsize))
    FILE *hb = fopen("zquest.txt", "r");
    
    if(!hb)
    {
        Z_error("Error loading zquest.txt.");
        quit_game();
    }
    
    char c = fgetc(hb);
    int index=0;
    
    while(!feof(hb))
    {
        helpbuf[index] = c;
        index++;
        c = fgetc(hb);
    }
    
    fclose(hb);
    
    helpbuf[helpsize]=0;
    helpstr = helpbuf;
    Z_message("OK\n");                                      // loading data files...
    
    init_qts();
    
    filepath[0]=temppath[0]=0;
    
#ifdef ALLEGRO_MACOSX
    const char *default_path="../../../";
    sprintf(filepath, "../../../");
    sprintf(temppath, "../");
#else
    const char *default_path="";
#endif
    
    strcpy(datapath,get_config_string("zquest",data_path_name,default_path));
    strcpy(midipath,get_config_string("zquest",midi_path_name,default_path));
    strcpy(imagepath,get_config_string("zquest",image_path_name,default_path));
    strcpy(tmusicpath,get_config_string("zquest",tmusic_path_name,default_path));
    chop_path(datapath);
    chop_path(midipath);
    chop_path(imagepath);
    chop_path(tmusicpath);
    
    MouseScroll                    = get_config_int("zquest","mouse_scroll",0);
    InvalidStatic                  = get_config_int("zquest","invalid_static",1);
    TileProtection                 = get_config_int("zquest","tile_protection",1);
    ShowGrid                       = get_config_int("zquest","show_grid",0);
    GridColor                      = get_config_int("zquest","grid_color",15);
    SnapshotFormat                 = get_config_int("zquest","snapshot_format",3);
    SavePaths                      = get_config_int("zquest","save_paths",1);
    CycleOn                        = get_config_int("zquest","cycle_on",1);
    ShowFPS                        = get_config_int("zquest","showfps",0)!=0;
    ComboBrush                     = get_config_int("zquest","combo_brush",0);
    BrushPosition                  = get_config_int("zquest","brush_position",0);
    FloatBrush                     = get_config_int("zquest","float_brush",0);
    RulesetDialog                  = get_config_int("zquest","rulesetdialog",1);
    EnableTooltips                 = get_config_int("zquest","enable_tooltips",1);
    ShowFFScripts                  = get_config_int("zquest","showffscripts",1);
    ShowSquares                    = get_config_int("zquest","showsquares",1);
    ShowInfo                       = get_config_int("zquest","showinfo",1);
    
    OpenLastQuest                  = get_config_int("zquest","open_last_quest",0);
    ShowMisalignments              = get_config_int("zquest","show_misalignments",0);
    AnimationOn                    = get_config_int("zquest","animation_on",1);
    AutoBackupRetention            = get_config_int("zquest","auto_backup_retention",2);
    AutoSaveInterval               = get_config_int("zquest","auto_save_interval",6);
    AutoSaveRetention              = get_config_int("zquest","auto_save_retention",2);
    UncompressedAutoSaves          = get_config_int("zquest","uncompressed_auto_saves",1);
    OverwriteProtection            = get_config_int("zquest","overwrite_prevention",0)!=0;
    ImportMapBias                  = get_config_int("zquest","import_map_bias",0);
    
    KeyboardRepeatDelay           = get_config_int("zquest","keyboard_repeat_delay",300);
    KeyboardRepeatRate            = get_config_int("zquest","keyboard_repeat_rate",80);
    
    ForceExit                     = get_config_int("zquest","force_exit",0);
   
#ifdef _WIN32
    zqUseWin32Proc                 = get_config_int("zquest","zq_win_proc_fix",0);
#endif
    
   // 1 <= zcmusic_bufsz <= 128
    zcmusic_bufsz = vbound(get_config_int("zquest","zqmusic_bufsz",64),1,128);
    int tempvalue                  = get_config_int("zquest","layer_mask",-1);
    
	LayerMask[0]=byte(tempvalue&0xFF);
    LayerMask[1]=byte((tempvalue>>8)&0xFF);
    
    for(int x=0; x<7; x++)
    {
        LayerMaskInt[x]=get_bit(LayerMask,x);
    }
    
    DuplicateAction[0]             = get_config_int("zquest","normal_duplicate_action",2);
    DuplicateAction[1]             = get_config_int("zquest","horizontal_duplicate_action",0);
    DuplicateAction[2]             = get_config_int("zquest","vertical_duplicate_action",0);
    DuplicateAction[3]             = get_config_int("zquest","both_duplicate_action",0);
    LeechUpdate                    = get_config_int("zquest","leech_update",500);
    LeechUpdateTiles               = get_config_int("zquest","leech_update_tiles",1);
    OnlyCheckNewTilesForDuplicates = get_config_int("zquest","only_check_new_tiles_for_duplicates",0);
    gui_colorset                   = get_config_int("zquest","gui_colorset",0);
    
    strcpy(last_timed_save,get_config_string("zquest","last_timed_save",""));
    
    midi_volume                    = get_config_int("zeldadx", "midi", 255);
    
    set_keyboard_rate(KeyboardRepeatDelay,KeyboardRepeatRate);

	Backend::graphics->readConfigurationOptions("zquest");

	if (used_switch(argc, argv, "-fullscreen"))
	{
		Backend::graphics->setFullscreen(true);
	}
	else if (used_switch(argc, argv, "-windowed"))
	{
		Backend::graphics->setFullscreen(false);
	}

	if (used_switch(argc, argv, "-0bit"))
	{
		Backend::graphics->setUseNativeColorDepth(true);
	}
	else if (used_switch(argc, argv, "-8bit"))
	{
		Backend::graphics->setUseNativeColorDepth(false);
	}
	

    int res_arg = used_switch(argc,argv,"-res");
    if(res_arg && (argc>(res_arg+2)))
    {
        int resx = atoi(argv[res_arg+1]);
        int resy = atoi(argv[res_arg+2]);
	Backend::graphics->setScreenResolution(resx, resy);
    }
    
    tooltip_box.x=-1;
    tooltip_box.y=-1;
    tooltip_box.w=0;
    tooltip_box.h=0;
    
    tooltip_trigger.x=-1;
    tooltip_trigger.y=-1;
    tooltip_trigger.w=0;
    tooltip_trigger.h=0;
        
    for(int i=0; i<MAXFAVORITECOMBOS; ++i)
    {
        favorite_combos[i]=-1;
    }
    
    for(int i=0; i<MAXFAVORITECOMBOALIASES; ++i)
    {
        favorite_comboaliases[i]=-1;
    }
    
    char cmdnametitle[20];
    
    for(int x=0; x<MAXFAVORITECOMMANDS; ++x)
    {
        sprintf(cmdnametitle, "command%02d", x+1);
        favorite_commands[x]=get_config_int("zquest",cmdnametitle,0);
    }
    
    
    if(used_switch(argc,argv,"-d"))
    {
        resolve_password(zquestpwd);
        set_debug(!strcmp(zquestpwd,get_config_string("zquest","debug_this","")));
    }
    
    char qtnametitle[20];
    char qtpathtitle[20];
    
    for(int x=1; x<MAXQTS; ++x)
    {
        sprintf(qtnametitle, "%s%d", qtname_name, x);
        sprintf(qtpathtitle, "%s%d", qtpath_name, x);
        strcpy(QuestTemplates[x].name,get_config_string("zquest",qtnametitle,""));
        strcpy(QuestTemplates[x].path,get_config_string("zquest",qtpathtitle,""));
        
        if(QuestTemplates[x].name[0]==0)
        {
            qt_count=x;
            break;
        }
    }
    
    Z_message("Initializing sound driver... ");
    
    if(used_switch(argc,argv,"-s"))
    {
        Z_message("skipped\n");
    }
    else
    {
        if(install_sound(DIGI_AUTODETECT,DIGI_AUTODETECT,NULL))
        {
        
            Z_message("Sound driver not available.  Sound disabled.\n");
        }
        else
        {
            Z_message("OK\n");
        }
    }
    
    if(used_switch(argc,argv,"-q"))
    {
        Z_message("-q switch used, quitting program.\n");
        quit_game();
        exit(0);
    }
    
    zcmusic_init();
    
	Z_message("initializing graphics\n");
	std::vector<std::pair<int, int> > desiredModes;
	desiredModes.push_back(std::pair<int, int>(800, 600));
	desiredModes.push_back(std::pair<int, int>(320, 240));
	Backend::graphics->registerVirtualModes(desiredModes);
	if (!Backend::graphics->initialize())
	{
		quit_game();
	}

	Z_message("done graphics\n");
    
    set_close_button_callback((void (*)()) hit_close_button);
    
	Backend::graphics->registerSwitchCallbacks(switch_in, switch_out);

    position_mouse(Backend::graphics->screenW()/2,Backend::graphics->screenH()/2);
    
    center_zq_class_dialogs();
    center_zq_custom_dialogs();
    center_zq_files_dialogs();
    center_zq_subscreen_dialogs();
    center_zq_tiles_dialogs();
    center_zquest_dialogs();
    
	//create these conservatively sized for large mode
	//could be (re)allocated on the fly if memory usage is a concern (in small mode)
	//but this is simpler for now -DD
    //screen2 = create_bitmap_ex(8,Backend::graphics->virtualScreenW(), Backend::graphics->virtualScreenH());
	screen2 = create_bitmap_ex(8, 800, 600);
    //tmp_scr = create_bitmap_ex(8, Backend::graphics->virtualScreenW(), Backend::graphics->virtualScreenH());
	tmp_scr = create_bitmap_ex(8, 800, 600);
    //menu1 = create_bitmap_ex(8, Backend::graphics->virtualScreenW(), Backend::graphics->virtualScreenH());
	menu1 = create_bitmap_ex(8, 800, 600);
    clear_bitmap(menu1);
    //menu3 = create_bitmap_ex(8, Backend::graphics->virtualScreenW(), Backend::graphics->virtualScreenH());
	menu3 = create_bitmap_ex(8, 800, 600);
    //mapscreenbmp = create_bitmap_ex(8,16*(showedges()?18:16),16*(showedges()?13:11));
	mapscreenbmp = create_bitmap_ex(8, 16 * 18, 16 * 13);
    dmapbmp_small = create_bitmap_ex(8,65,33);
    //dmapbmp_large = create_bitmap_ex(8,(is_large()?177:113),(is_large()?81:57));
	dmapbmp_large = create_bitmap_ex(8, 177, 81);
    //brushbmp = create_bitmap_ex(8,256*mapscreensize(), 176*mapscreensize());
	brushbmp = create_bitmap_ex(8, 512, 352);
    //brushscreen = create_bitmap_ex(8,(256+(showedges()?16:0))*mapscreensize(), (176+(showedges()?16:0))*mapscreensize());
	brushscreen = create_bitmap_ex(8, 256 + 32, 176 + 32);
    tooltipbmp = create_bitmap_ex(8,256,256); // Decrease size at your own risk.
    clear_bitmap(tooltipbmp);
    
    if(!screen2 || !tmp_scr || !menu1 || !menu3 || !dmapbmp_large || !dmapbmp_large || !brushbmp || !brushscreen)// || !brushshadowbmp )
    {
        Z_message("Error creating bitmaps\n");
        allegro_exit();
        quit_game();
        return 1;
    }
    
    Backend::palette->setPalette((RGB*)zcdata[PAL_ZQUEST].dat);
    Backend::palette->getPalette(RAMpal);
    
    switch(gui_colorset)
    {
        /*
          enum
          {
          jcBOX, jcLIGHT, jcMEDLT, jcMEDDARK, jcDARK, jcBOXFG,
          jcTITLEL, jcTITLER, jcTITLEFG, jcTEXTBG, jcTEXTFG, jcSELBG, jcSELFG,
          jcMAX
          };
          */
    case 1:  //Windows 98
    {
        RAMpal[dvc(1)] = _RGB(0*63/255,   0*63/255,   0*63/255);
        RAMpal[dvc(2)] = _RGB(128*63/255, 128*63/255, 128*63/255);
        RAMpal[dvc(3)] = _RGB(192*63/255, 192*63/255, 192*63/255);
        RAMpal[dvc(4)] = _RGB(223*63/255, 223*63/255, 223*63/255);
        RAMpal[dvc(5)] = _RGB(255*63/255, 255*63/255, 255*63/255);
        RAMpal[dvc(6)] = _RGB(255*63/255, 255*63/255, 225*63/255);
        RAMpal[dvc(7)] = _RGB(255*63/255, 225*63/255, 160*63/255);
        RAMpal[dvc(8)] = _RGB(0*63/255,   0*63/255,  80*63/255);
        
        byte palrstart=  0*63/255, palrend=166*63/255,
             palgstart=  0*63/255, palgend=202*63/255,
             palbstart=128*63/255, palbend=240*63/255,
             paldivs=7;
             
        for(int i=0; i<paldivs; i++)
        {
            RAMpal[dvc(15-paldivs+1)+i].r = palrstart+((palrend-palrstart)*i/(paldivs-1));
            RAMpal[dvc(15-paldivs+1)+i].g = palgstart+((palgend-palgstart)*i/(paldivs-1));
            RAMpal[dvc(15-paldivs+1)+i].b = palbstart+((palbend-palbstart)*i/(paldivs-1));
        }
        
        jwin_pal[jcBOX]    =dvc(3);
        jwin_pal[jcLIGHT]  =dvc(5);
        jwin_pal[jcMEDLT]  =dvc(4);
        jwin_pal[jcMEDDARK]=dvc(2);
        jwin_pal[jcDARK]   =dvc(1);
        jwin_pal[jcBOXFG]  =dvc(1);
        jwin_pal[jcTITLEL] =dvc(9);
        jwin_pal[jcTITLER] =dvc(15);
        jwin_pal[jcTITLEFG]=dvc(7);
        jwin_pal[jcTEXTBG] =dvc(5);
        jwin_pal[jcTEXTFG] =dvc(1);
        jwin_pal[jcSELBG]  =dvc(8);
        jwin_pal[jcSELFG]  =dvc(6);
    }
    break;
    
    case 2:  //Windows 99
    {
        RAMpal[dvc(1)] = _RGB(0*63/255,   0*63/255,   0*63/255);
        RAMpal[dvc(2)] = _RGB(64*63/255,  64*63/255,  64*63/255);
        RAMpal[dvc(3)] = _RGB(128*63/255, 128*63/255, 128*63/255);
        RAMpal[dvc(4)] = _RGB(192*63/255, 192*63/255, 192*63/255);
        RAMpal[dvc(5)] = _RGB(223*63/255, 223*63/255, 223*63/255);
        RAMpal[dvc(6)] = _RGB(255*63/255, 255*63/255, 255*63/255);
        RAMpal[dvc(7)] = _RGB(255*63/255, 255*63/255, 225*63/255);
        RAMpal[dvc(8)] = _RGB(255*63/255, 225*63/255, 160*63/255);
        RAMpal[dvc(9)] = _RGB(0*63/255,   0*63/255,  80*63/255);
        
        byte palrstart=  0*63/255, palrend=166*63/255,
             palgstart=  0*63/255, palgend=202*63/255,
             palbstart=128*63/255, palbend=240*63/255,
             paldivs=6;
             
        for(int i=0; i<paldivs; i++)
        {
            RAMpal[dvc(15-paldivs+1)+i].r = palrstart+((palrend-palrstart)*i/(paldivs-1));
            RAMpal[dvc(15-paldivs+1)+i].g = palgstart+((palgend-palgstart)*i/(paldivs-1));
            RAMpal[dvc(15-paldivs+1)+i].b = palbstart+((palbend-palbstart)*i/(paldivs-1));
        }
        
        jwin_pal[jcBOX]    =dvc(4);
        jwin_pal[jcLIGHT]  =dvc(6);
        jwin_pal[jcMEDLT]  =dvc(5);
        jwin_pal[jcMEDDARK]=dvc(3);
        jwin_pal[jcDARK]   =dvc(2);
        jwin_pal[jcBOXFG]  =dvc(1);
        jwin_pal[jcTITLEL] =dvc(10);
        jwin_pal[jcTITLER] =dvc(15);
        jwin_pal[jcTITLEFG]=dvc(8);
        jwin_pal[jcTEXTBG] =dvc(6);
        jwin_pal[jcTEXTFG] =dvc(1);
        jwin_pal[jcSELBG]  =dvc(9);
        jwin_pal[jcSELFG]  =dvc(7);
    }
    break;
    
    case 3:  //Windows 2000 Blue
    {
        RAMpal[dvc(1)] = _RGB(0*63/255,   0*63/255,   0*63/255);
        RAMpal[dvc(2)] = _RGB(16*63/255,  15*63/255, 116*63/255);
        RAMpal[dvc(3)] = _RGB(82*63/255,  80*63/255, 182*63/255);
        RAMpal[dvc(4)] = _RGB(162*63/255, 158*63/255, 250*63/255);
        RAMpal[dvc(5)] = _RGB(255*63/255, 255*63/255, 255*63/255);
        RAMpal[dvc(6)] = _RGB(255*63/255, 255*63/255, 127*63/255);
        RAMpal[dvc(7)] = _RGB(255*63/255, 225*63/255,  63*63/255);
        RAMpal[dvc(8)] = _RGB(0*63/255,   0*63/255,  80*63/255);
        
        byte palrstart=  0*63/255, palrend=162*63/255,
             palgstart=  0*63/255, palgend=158*63/255,
             palbstart= 80*63/255, palbend=250*63/255,
             paldivs=7;
             
        for(int i=0; i<paldivs; i++)
        {
            RAMpal[dvc(15-paldivs+1)+i].r = palrstart+((palrend-palrstart)*i/(paldivs-1));
            RAMpal[dvc(15-paldivs+1)+i].g = palgstart+((palgend-palgstart)*i/(paldivs-1));
            RAMpal[dvc(15-paldivs+1)+i].b = palbstart+((palbend-palbstart)*i/(paldivs-1));
        }
        
        jwin_pal[jcBOX]    =dvc(4);
        jwin_pal[jcLIGHT]  =dvc(5);
        jwin_pal[jcMEDLT]  =dvc(4);
        jwin_pal[jcMEDDARK]=dvc(3);
        jwin_pal[jcDARK]   =dvc(2);
        jwin_pal[jcBOXFG]  =dvc(1);
        jwin_pal[jcTITLEL] =dvc(9);
        jwin_pal[jcTITLER] =dvc(15);
        jwin_pal[jcTITLEFG]=dvc(7);
        jwin_pal[jcTEXTBG] =dvc(5);
        jwin_pal[jcTEXTFG] =dvc(1);
        jwin_pal[jcSELBG]  =dvc(8);
        jwin_pal[jcSELFG]  =dvc(6);
    }
    break;
    
    case 687:  //Windows 2000 Gold (6-87 was the North American release date of LoZ)
    {
        RAMpal[dvc(1)] = _RGB(0*63/255,   0*63/255,   0*63/255);
        RAMpal[dvc(2)] = _RGB(64*63/255,  64*63/255,  43*63/255);
        RAMpal[dvc(3)] = _RGB(170*63/255, 154*63/255,  96*63/255);
        RAMpal[dvc(4)] = _RGB(223*63/255, 200*63/255, 128*63/255); // Old Gold
        RAMpal[dvc(5)] = _RGB(240*63/255, 223*63/255, 136*63/255);
        RAMpal[dvc(6)] = _RGB(255*63/255, 223*63/255, 128*63/255);
        RAMpal[dvc(7)] = _RGB(255*63/255, 223*63/255, 128*63/255);
        RAMpal[dvc(8)] = _RGB(255*63/255, 225*63/255, 160*63/255);
        RAMpal[dvc(9)] = _RGB(80*63/255,  80*63/255,   0*63/255);
        
        byte palrstart=128*63/255, palrend=240*63/255,
             palgstart=128*63/255, palgend=202*63/255,
             palbstart=  0*63/255, palbend=166*63/255,
             paldivs=6;
             
        for(int i=0; i<paldivs; i++)
        {
            RAMpal[dvc(15-paldivs+1)+i].r = palrstart+((palrend-palrstart)*i/(paldivs-1));
            RAMpal[dvc(15-paldivs+1)+i].g = palgstart+((palgend-palgstart)*i/(paldivs-1));
            RAMpal[dvc(15-paldivs+1)+i].b = palbstart+((palbend-palbstart)*i/(paldivs-1));
        }
        
        jwin_pal[jcBOX]    =dvc(4);
        jwin_pal[jcLIGHT]  =dvc(6);
        jwin_pal[jcMEDLT]  =dvc(5);
        jwin_pal[jcMEDDARK]=dvc(3);
        jwin_pal[jcDARK]   =dvc(2);
        jwin_pal[jcBOXFG]  =dvc(1);
        jwin_pal[jcTITLEL] =dvc(10);
        jwin_pal[jcTITLER] =dvc(15);
        jwin_pal[jcTITLEFG]=dvc(8);
        jwin_pal[jcTEXTBG] =dvc(6);
        jwin_pal[jcTEXTFG] =dvc(1);
        jwin_pal[jcSELBG]  =dvc(9);
        jwin_pal[jcSELFG]  =dvc(7);
    }
    break;
    
    case 4104:  //Windows 2000 Easter (4-1-04 is April Fools Day, the date of this release)
    {
        RAMpal[dvc(1)] = _RGB(0*63/255,   0*63/255,   0*63/255);
        RAMpal[dvc(2)] = _RGB(64*63/255,  64*63/255,  64*63/255);
        RAMpal[dvc(3)] = _RGB(128*63/255, 128*63/255, 128*63/255);
        RAMpal[dvc(4)] = _RGB(252*63/255, 186*63/255, 188*63/255);
        RAMpal[dvc(5)] = _RGB(254*63/255, 238*63/255, 238*63/255);
        RAMpal[dvc(6)] = _RGB(244*63/255, 243*63/255, 161*63/255);
        RAMpal[dvc(7)] = _RGB(120*63/255, 173*63/255, 189*63/255);
        RAMpal[dvc(8)] = _RGB(220*63/255, 183*63/255, 227*63/255);
        
        byte palrstart=244*63/255, palrend=220*63/255,
             palgstart=243*63/255, palgend=183*63/255,
             palbstart=161*63/255, palbend=227*63/255,
             paldivs=7;
             
        for(int i=0; i < paldivs; i++)
        {
            RAMpal[dvc(15-paldivs+1)+i].r = palrstart+((palrend-palrstart)*i/(paldivs-1));
            RAMpal[dvc(15-paldivs+1)+i].g = palgstart+((palgend-palgstart)*i/(paldivs-1));
            RAMpal[dvc(15-paldivs+1)+i].b = palbstart+((palbend-palbstart)*i/(paldivs-1));
        }
        
        jwin_pal[jcBOX]    =dvc(4);
        jwin_pal[jcLIGHT]  =dvc(5);
        jwin_pal[jcMEDLT]  =dvc(4);
        jwin_pal[jcMEDDARK]=dvc(3);
        jwin_pal[jcDARK]   =dvc(2);
        jwin_pal[jcBOXFG]  =dvc(7);
        jwin_pal[jcTITLEL] =dvc(9);
        jwin_pal[jcTITLER] =dvc(15);
        jwin_pal[jcTITLEFG]=dvc(7);
        jwin_pal[jcTEXTBG] =dvc(5);
        jwin_pal[jcTEXTFG] =dvc(7);
        jwin_pal[jcSELBG]  =dvc(8);
        jwin_pal[jcSELFG]  =dvc(6);
    }
    break;
    
    default:  //Windows 2000
    {
        RAMpal[dvc(1)] = _RGB(0*63/255,   0*63/255,   0*63/255);
        RAMpal[dvc(2)] = _RGB(66*63/255,  65*63/255,  66*63/255);
        RAMpal[dvc(3)] = _RGB(132*63/255, 130*63/255, 132*63/255);
        RAMpal[dvc(4)] = _RGB(212*63/255, 208*63/255, 200*63/255);
        RAMpal[dvc(5)] = _RGB(255*63/255, 255*63/255, 255*63/255);
        RAMpal[dvc(6)] = _RGB(255*63/255, 255*63/255, 225*63/255);
        RAMpal[dvc(7)] = _RGB(255*63/255, 225*63/255, 160*63/255);
        RAMpal[dvc(8)] = _RGB(0*63/255,   0*63/255,  80*63/255);
        
        byte palrstart= 10*63/255, palrend=166*63/255,
             palgstart= 36*63/255, palgend=202*63/255,
             palbstart=106*63/255, palbend=240*63/255,
             paldivs=7;
             
        for(int i=0; i<paldivs; i++)
        {
            RAMpal[dvc(15-paldivs+1)+i].r = palrstart+((palrend-palrstart)*i/(paldivs-1));
            RAMpal[dvc(15-paldivs+1)+i].g = palgstart+((palgend-palgstart)*i/(paldivs-1));
            RAMpal[dvc(15-paldivs+1)+i].b = palbstart+((palbend-palbstart)*i/(paldivs-1));
        }
        
        jwin_pal[jcBOX]    =dvc(4);
        jwin_pal[jcLIGHT]  =dvc(5);
        jwin_pal[jcMEDLT]  =dvc(4);
        jwin_pal[jcMEDDARK]=dvc(3);
        jwin_pal[jcDARK]   =dvc(2);
        jwin_pal[jcBOXFG]  =dvc(1);
        jwin_pal[jcTITLEL] =dvc(9);
        jwin_pal[jcTITLER] =dvc(15);
        jwin_pal[jcTITLEFG]=dvc(7);
        jwin_pal[jcTEXTBG] =dvc(5);
        jwin_pal[jcTEXTFG] =dvc(1);
        jwin_pal[jcSELBG]  =dvc(8);
        jwin_pal[jcSELFG]  =dvc(6);
    }
    break;
    }
    
    gui_bg_color=jwin_pal[jcBOX];
    gui_fg_color=jwin_pal[jcBOXFG];
    gui_mg_color=jwin_pal[jcMEDDARK];
    
    jwin_set_colors(jwin_pal);
    Backend::palette->setPalette(RAMpal);
    clear_to_color(screen,vc(0));
    
    //clear the midis (to keep loadquest from crashing by trying to destroy a garbage midi)
    for(int i=0; i<MAXCUSTOMMIDIS_ZQ; ++i)
    {
        customtunes[i].data=NULL;
    }
    
    Backend::sfx->loadDefaultSamples(Z35, sfxdata, old_sfx_string);
    
    for(int i=0; i<WPNCNT; i++)
    {
        weapon_string[i] = new char[64];
        memset(weapon_string[i], 0, 64);
    }
    
    for(int i=0; i<ITEMCNT; i++)
    {
        item_string[i] = new char[64];
        memset(item_string[i], 0, 64);
    }
    
    for(int i=0; i<eMAXGUYS; i++)
    {
        guy_string[i] = new char[64];
        memset(guy_string[i], 0, 64);
    }
    
	scripts = GameScripts();
    
    zScript = std::string();
    strcpy(zScriptBytes, "0 Bytes in Buffer");
    
    load_mice();
    Backend::mouse->setCursorSprite(mouse_bmp[MOUSE_BMP_NORMAL][0]);
	Backend::mouse->setCursorVisibility(true);
    //Display annoying beta warning message
#if IS_BETA
    char *curcontrol = getBetaControlString();
    const char *oldcontrol = get_config_string("zquest", "beta_warning", "");
    
    if(strcmp(curcontrol, oldcontrol))
    {
        jwin_alert("       !!WARNING - This is BETA!!", "This version of ZQuest might corrupt your quest or crash.", "Features might change or disappear with no warning.", "Build quests at your OWN RISK!", "OK", NULL, 0, 0, lfont);
    }
    
    delete[] curcontrol;
#endif
    
    // A bit of festivity
    {
        time_t rawtime;
        time(&rawtime);
        
        if(strncmp(ctime(&rawtime)+4,"Jan  1",6)==0)
        {
            jwin_alert("Hooray!", "Happy New Year!", NULL, NULL, "OK", NULL, 0, 0, lfont);
        }
    }
    
    load_icons();
    bool load_last_timed_save=false;
    
    if((last_timed_save[0]!=0)&&(exists(last_timed_save)))
    {
        if(jwin_alert("ZQuest","It appears that ZQuest crashed last time.","Would you like to load the last timed save?",NULL,"&Yes","&No",'y','n',lfont)==1)
        {
            int ret = load_quest(last_timed_save,true,true);
            
            if(ret == qe_OK)
            {
                replace_extension(filepath,last_timed_save,"qst",2047);
                load_last_timed_save=true;
                saved=false;
            }
            else
            {
                jwin_alert("Error","Unable to reload the last timed save.",NULL,NULL,"OK",NULL,13,27,lfont);
            }
        }
    }
    
    if(!load_last_timed_save)
    {
        strcpy(filepath,get_config_string("zquest",last_quest_name,""));
        
        if(argc>1 && argv[1][0]!='-')
        {
            replace_extension(temppath,argv[1],"qst",2047);
            int ret = load_quest(temppath,true,true);
            
            if(ret == qe_OK)
            {
                first_save=true;
                strcpy(filepath,temppath);
                refresh(rALL);
            }
        }
        else if(OpenLastQuest&&filepath[0]&&exists(filepath)&&!used_switch(argc,argv,"-new"))
        {
            int ret = load_quest(filepath,true,true);
            
            if(ret == qe_OK)
            {
                first_save=true;
                refresh(rALL);
            }
            else
            {
                filepath[0]=temppath[0]=0;
                first_save=false;
#ifdef ALLEGRO_MACOSX
                sprintf(filepath, "../../../");
                sprintf(temppath, "../");
#endif
            }
        }
        else
        {
            init_quest(NULL);
            
            if(RulesetDialog)
                PickRuleset();
                
            //otherwise the blank quest gets the name of the last loaded quest... not good! -DD
            filepath[0]=temppath[0]=0;
            first_save=false;
#ifdef ALLEGRO_MACOSX
            sprintf(filepath, "../../../");
            sprintf(temppath, "../");
#endif
        }
    }

    if (used_switch(argc, argv, "-dump_zasm"))
	{
		printZAsm();
		quit_game();
		exit(0);
	}
    
    for(int x=0; x<MAXITEMS; x++)
    {
        lens_hint_item[x][0]=0;
        lens_hint_item[x][1]=0;
    }
    
    for(int x=0; x<MAXWPNS; x++)
    {
        lens_hint_weapon[x][0]=0;
        lens_hint_weapon[x][1]=0;
    }
    
    load_selections();
    load_arrows();
    clear_to_color(menu1,vc(0));
    refresh(rALL);
    DIALOG_PLAYER *player2=init_dialog(dialogs,-1);
    
    Backend::palette->getPalette(RAMpal);
    
    rgb_map = &zq_rgb_table;
    
    Map.setCurrMap(zinit.last_map);
    Map.setCurrScr(zinit.last_screen);
    //  setup_combo_animations();
    refresh(rALL);
    brush_width_menu[0].flags=D_SELECTED;
    brush_height_menu[0].flags=D_SELECTED;
    fill_menu[1].flags=D_SELECTED;
    
    rebuild_trans_table();
    
    set_display_switch_callback(SWITCH_OUT, switch_out);
    set_display_switch_callback(SWITCH_IN, switch_in);
    
    quit=!update_dialog(player2);
    //clear_keybuf();
    etc_menu[10].flags=commands[cmdChangeTrack].flags=D_DISABLED;
    
    fix_drawing_mode_menu();
    
    
#ifdef _WIN32
    
    if(zqUseWin32Proc != FALSE)
    {
        al_trace("Config file warning: \"zq_win_proc_fix\" enabled switch found. This can cause crashes on some computers.\n");
        win32data.zqSetDefaultThreadPriority(0);
        win32data.zqSetCustomCallbackProc(win_get_window());
    }
    
#endif
    
    time(&auto_save_time_start);
    
    while(!quit)
    {
    
#ifdef _WIN32
    
        try   // I *think* it might throw here.
        {
            if(zqUseWin32Proc != FALSE)
                win32data.Update(); //experimental win32 fixes
        }
        catch(...)
        {
            set_gfx_mode(GFX_TEXT,0,0,0,0);
            allegro_message("ZQ-Windows Fatal Error: Set \"zq_win_proc_fix = 0\" in config file.");
            exit(1);
        }
        
#endif
        
        check_autosave();
        
        
        ++alignment_arrow_timer;
        
        if(alignment_arrow_timer>63)
        {
            alignment_arrow_timer=0;
        }
        
        if(strcmp(catchall_string[Map.CurrScr()->room]," "))
        {
            static char ca_menu_str[40];
            sprintf(ca_menu_str,"%s\tA",catchall_string[Map.CurrScr()->room]);
            data_menu[13].text=ca_menu_str;
            data_menu[13].flags=commands[cmdCatchall].flags=0;
        }
        else
        {
            data_menu[13].text=(char *)"Catch All\tA";
            data_menu[13].flags=commands[cmdCatchall].flags=D_DISABLED;
        }
        
        file_menu[2].flags =
            file_menu[4].flags =
                dialogs[18].flags =
                    commands[cmdSave].flags =
                        commands[cmdRevert].flags = (saved | disable_saving|OverwriteProtection) ? D_DISABLED : 0;
                        
        file_menu[3].flags =
            commands[cmdSaveAs].flags = disable_saving ? D_DISABLED : 0;
            
        edit_menu[0].flags =
            commands[cmdUndo].flags = Map.CanUndo() ? 0 : D_DISABLED;
            
        edit_menu[2].flags =
            edit_menu[3].flags =
                edit_menu[4].flags =
                    edit_menu[5].flags =
                        paste_menu[0].flags =
                            paste_menu[1].flags =
                                paste_item_menu[0].flags =
                                    paste_item_menu[1].flags =
                                        paste_item_menu[2].flags =
                                            paste_item_menu[3].flags =
                                                paste_item_menu[4].flags =
                                                        paste_item_menu[5].flags =
                                                                paste_item_menu[6].flags =
                                                                        paste_item_menu[7].flags =
                                                                                paste_item_menu[8].flags =
                                                                                        paste_item_menu[9].flags =
                                                                                                paste_item_menu[10].flags =
                                                                                                        commands[cmdPaste].flags =
                                                                                                                commands[cmdPasteAll].flags =
                                                                                                                        commands[cmdPasteToAll].flags =
                                                                                                                                commands[cmdPasteAllToAll].flags =
                                                                                                                                        commands[cmdPasteUnderCombo].flags =
                                                                                                                                                commands[cmdPasteSecretCombos].flags =
                                                                                                                                                        commands[cmdPasteFFCombos].flags =
                                                                                                                                                                commands[cmdPasteScreenData].flags =
                                                                                                                                                                        commands[cmdPasteWarps].flags =
                                                                                                                                                                                commands[cmdPasteWarpLocations].flags =
                                                                                                                                                                                        commands[cmdPasteEnemies].flags =
                                                                                                                                                                                                commands[cmdPasteRoom].flags =
                                                                                                                                                                                                        commands[cmdPasteGuy].flags =
                                                                                                                                                                                                                commands[cmdPasteDoors].flags =
                                                                                                                                                                                                                        commands[cmdPasteLayers].flags = Map.CanPaste() ? 0 : D_DISABLED;
                                                                                                                                                                                                                        
        edit_menu[1].flags =
            edit_menu[6].flags =
                commands[cmdCopy].flags =
                    commands[cmdDelete].flags = (Map.CurrScr()->valid&mVALID) ? 0 : D_DISABLED;
                    
        tool_menu[0].flags =
            data_menu[7].flags =
                commands[cmdTemplate].flags =
                    commands[cmdDoors].flags = (Map.getCurrScr()<TEMPLATE) ? 0 : D_DISABLED;
                    
        defs_menu[1].flags =
            commands[cmdDefault_Tiles].flags = 0;
            
        // Are some things selected?
        view_menu[3].flags=(Flags&cWALK)?D_SELECTED:0; // Show Walkability
        view_menu[4].flags=(Flags&cFLAGS)?D_SELECTED:0; // Show Flags
        view_menu[5].flags=(Flags&cCSET)?D_SELECTED:0; // Show CSet
        view_menu[6].flags=(Flags&cCTYPE)?D_SELECTED:0; // Show Type
        view_menu[11].flags=(ShowGrid)?D_SELECTED:0; // Show Grid
        view_menu[10].flags=(ShowFFScripts)?D_SELECTED:0; // Show Script Names
        view_menu[9].flags=(ShowSquares)?D_SELECTED:0; // Show Squares
        view_menu[8].flags=(!is_large())?D_DISABLED:(ShowInfo)?D_SELECTED:0; // Show Info
        
        maps_menu[1].flags=(Map.getCurrMap()<map_count && map_count>0) ? 0 : D_DISABLED;
        maps_menu[2].flags=(Map.getCurrMap()>0)? 0 : D_DISABLED;            

		setVideoModeMenuFlags();
        
        quit = !update_dialog(player2);
        
        //clear_keybuf();
        if(close_button_quit)
        {
            close_button_quit=false;
            
            if(onExit()==D_CLOSE)
            {
                quit=true;
            }
        }
    }
   
    quit_game();
	Backend::shutdownBackend();
    
    if(ForceExit) //last resort fix to the allegro process hanging bug.
        exit(0);
        
    allegro_exit();
    
    return 0;
// memset(qtpathtitle,0,10);//UNREACHABLE
}

END_OF_MAIN()


void remove_locked_params_on_exit()
{
    al_trace("Removing timers. \n");
    remove_int(dclick_check);
}


void cleanup_datafiles_on_exit()
{
    al_trace("Cleaning datafiles. \n");
    
    if(zcdata) unload_datafile(zcdata);
    
    if(fontsdata) unload_datafile(fontsdata);
    
    if(sfxdata) unload_datafile(sfxdata);
}


void destroy_bitmaps_on_exit()
{
    al_trace("Cleaning bitmaps...");
    destroy_bitmap(screen2);
    destroy_bitmap(tmp_scr);
    destroy_bitmap(menu1);
    destroy_bitmap(menu3);
    destroy_bitmap(mapscreenbmp);
    destroy_bitmap(dmapbmp_small);
    destroy_bitmap(dmapbmp_large);
    destroy_bitmap(brushbmp);
    destroy_bitmap(brushscreen);
    destroy_bitmap(tooltipbmp);
    al_trace("...");
	Backend::mouse->setCursorVisibility(false);
    
    for(int i=0; i<MOUSE_BMP_MAX*4; i++)
        destroy_bitmap(mouse_bmp[i/4][i%4]);
        
    for(int i=0; i<ICON_BMP_MAX*4; i++)
        destroy_bitmap(icon_bmp[i/4][i%4]);
        
    for(int i=0; i<2; i++)
        destroy_bitmap(select_bmp[i]);
        
    for(int i=0; i<MAXARROWS; i++)
        destroy_bitmap(arrow_bmp[i]);
        
    al_trace(" OK. \n");
}


void quit_game()
{
    deallocate_biic_list();
    
    
    last_timed_save[0]=0;
    save_config_file();
    Backend::palette->setPalette(black_palette);
    stop_midi();
    //if(scrtmp) {destroy_bitmap(screen); screen = hw_screen;}
    
    remove_locked_params_on_exit();
    
    al_trace("Cleaning aliases. \n");
    
    for(int i=0; i<MAXCOMBOALIASES; i++)
    {
        if(combo_aliases[i].combos != NULL)
        {
            delete[] combo_aliases[i].combos;
        }
        
        if(combo_aliases[i].csets != NULL)
        {
            delete[] combo_aliases[i].csets;
        }
        
        if(temp_aliases[i].combos != NULL)
        {
            delete[] temp_aliases[i].combos;
        }
        
        if(temp_aliases[i].csets != NULL)
        {
            delete[] temp_aliases[i].csets;
        }
    }
    
    al_trace("Cleaning subscreens. \n");
    
    for(int i=0; i<4; i++)
    {
        for(int j=0; j<MAXSUBSCREENITEMS; j++)
        {
            switch(custom_subscreen[i].objects[j].type)
            {
            case ssoTEXT:
            case ssoTEXTBOX:
            case ssoCURRENTITEMTEXT:
            case ssoCURRENTITEMCLASSTEXT:
                if(custom_subscreen[i].objects[j].dp1 != NULL) delete[](char *)custom_subscreen[i].objects[j].dp1;
                
                break;
            }
        }
    }
    
    al_trace("Cleaning sfx. \n");
    
    Backend::sfx->loadDefaultSamples(Z35, sfxdata, old_sfx_string);
    
    for(int i=0; i<WPNCNT; i++)
    {
        delete [] weapon_string[i];
    }
    
    for(int i=0; i<ITEMCNT; i++)
    {
        delete [] item_string[i];
    }
    
    for(int i=0; i<eMAXGUYS; i++)
    {
        delete [] guy_string[i];
    }
    
    al_trace("Cleaning script buffer. \n");
    
	scripts = GameScripts();
    
    al_trace("Cleaning qst buffers. \n");
    del_qst_buffers();
    
    
    al_trace("Cleaning midis. \n");
    
    if(customtunes)
    {
        for(int i=0; i<MAXCUSTOMMIDIS_ZQ; i++)
            customtunes[i].reset();
            
        zc_free(customtunes);
    }
    
    al_trace("Cleaning undotilebuf. \n");
    
    if(undocombobuf) zc_free(undocombobuf);
    
    if(newundotilebuf)
    {
        for(int i=0; i<NEWMAXTILES; i++)
            if(newundotilebuf[i].data) zc_free(newundotilebuf[i].data);
            
        zc_free(newundotilebuf);
    }
    
    if(filepath) zc_free(filepath);
    
    if(temppath) zc_free(temppath);
    
    if(datapath) zc_free(datapath);
    
    if(midipath) zc_free(midipath);
    
    if(imagepath) zc_free(imagepath);
    
    if(tmusicpath) zc_free(tmusicpath);
    
    if(last_timed_save) zc_free(last_timed_save);
    
    cleanup_datafiles_on_exit();
    destroy_bitmaps_on_exit();
    __zc_debug_malloc_free_print_memory_leaks(); //this won't do anything without debugging for it defined.
    
}

void center_zquest_dialogs()
{
    jwin_center_dialog(assignscript_dlg);
    jwin_center_dialog(autolayer_dlg);
    jwin_center_dialog(cheats_dlg);
    jwin_center_dialog(clist_dlg);
    jwin_center_dialog(cpage_dlg);
    center_zq_cset_dialogs();
    jwin_center_dialog(change_track_dlg);
    jwin_center_dialog(compile_dlg);
    jwin_center_dialog(csetfix_dlg);
    jwin_center_dialog(dmapmaps_dlg);
    center_zq_door_dialogs();
    jwin_center_dialog(editcomboa_dlg);
    jwin_center_dialog(editdmap_dlg);
    jwin_center_dialog(editinfo_dlg);
    jwin_center_dialog(editmidi_dlg);
    jwin_center_dialog(editmsg_dlg);
    jwin_center_dialog(editmusic_dlg);
    jwin_center_dialog(editshop_dlg);
    jwin_center_dialog(elist_dlg);
    jwin_center_dialog(enemy_dlg);
    jwin_center_dialog(ffcombo_dlg);
    jwin_center_dialog(ffcombo_sel_dlg);
    jwin_center_dialog(ffscript_sel_dlg);
    jwin_center_dialog(getnum_dlg);
    jwin_center_dialog(glist_dlg);
    jwin_center_dialog(gscript_sel_dlg);
    jwin_center_dialog(header_dlg);
    jwin_center_dialog(help_dlg);
    jwin_center_dialog(ilist_dlg);
    center_zq_init_dialog();
    jwin_center_dialog(itemscript_sel_dlg);
    jwin_center_dialog(layerdata_dlg);
    jwin_center_dialog(list_dlg);
    jwin_center_dialog(loadmap_dlg);
    jwin_center_dialog(mapstyles_dlg);
    jwin_center_dialog(misccolors_dlg);
    jwin_center_dialog(newcomboa_dlg);
    jwin_center_dialog(options_dlg);
    jwin_center_dialog(orgcomboa_dlg);
    jwin_center_dialog(path_dlg);
    jwin_center_dialog(password_dlg);
    jwin_center_dialog(pattern_dlg);
    jwin_center_dialog(rlist_dlg);
    center_zq_rules_dialog();
    jwin_center_dialog(scrdata_dlg);
    jwin_center_dialog(screen_pal_dlg);
    jwin_center_dialog(secret_dlg);
    jwin_center_dialog(selectdmap_dlg);
    jwin_center_dialog(selectmidi_dlg);
    jwin_center_dialog(selectmusic_dlg);
    jwin_center_dialog(sfxlist_dlg);
    jwin_center_dialog(sfx_edit_dlg);
    jwin_center_dialog(showpal_dlg);
    jwin_center_dialog(strlist_dlg);
    jwin_center_dialog(subscreen_type_dlg);
    jwin_center_dialog(template_dlg);
    center_zq_tiles_dialog();
    jwin_center_dialog(tp_dlg);
    jwin_center_dialog(under_dlg);
    jwin_center_dialog(warp_dlg);
    jwin_center_dialog(warpring_dlg);
    jwin_center_dialog(wlist_dlg);
}


void animate_coords()
{
    coord_frame=(coord_timer>>3)&3;
    
    if(++coord_timer>=(1<<5))
    {
        coord_timer=0;
    }
}

static int help_pos=0;

static const char *help_list[] =
{
    "PREVIEW MODE",
    "PgUp/PgDn - Scroll through hotkey list",
    "Esc/Enter - Exit Preview Mode",
    "R - Restore screen to original state",
    "C - Toggle combo cycling On/Off",
    "S - Trigger screen secrets",
    "Q/W/F - These still work",
    "P - Pause everything",
    "A - Advance frame-by-frame",
    "1-4 - Trigger tile warp A-D",
    "5-8 - Trigger side warp A-D",
    "9 - Enable timed warps",
    "",
    "",
};

void do_animations()
{
    if(AnimationOn||CycleOn)
    {
        if(AnimationOn)
        {
            animate_combos();
            update_freeform_combos();
        }
        
        if(CycleOn)
        {
            cycle_palette();
        }
    }
    
    animate_coords();    
}

void do_previewtext()
{
    //Put in help areas
    textprintf_ex(menu1,font,panel(8).x+1,panel(8).y+3,vc(0),-1,"%s",help_list[help_pos]);
    textprintf_ex(menu1,font,panel(8).x+1,panel(8).y+8+3,vc(0),-1,"%s",help_list[help_pos+1]);
    textprintf_ex(menu1,font,panel(8).x+1,panel(8).y+16+3,vc(0),-1,"%s",help_list[help_pos+2]);
    textprintf_ex(menu1,font,panel(8).x+1,panel(8).y+24+3,vc(0),-1,"%s",help_list[help_pos+3]);
    textprintf_ex(menu1,font,panel(8).x+1,panel(8).y+32+3,vc(0),-1,"%s",help_list[help_pos+4]);
    
    if(!is_large()) return;
    
    textprintf_ex(menu1,font,panel(8).x+1,panel(8).y+40+3,vc(0),-1,"%s",help_list[help_pos+5]);
    textprintf_ex(menu1,font,panel(8).x+1,panel(8).y+48+3,vc(0),-1,"%s",help_list[help_pos+6]);
    textprintf_ex(menu1,font,panel(8).x+1,panel(8).y+56+3,vc(0),-1,"%s",help_list[help_pos+7]);
    textprintf_ex(menu1,font,panel(8).x+1,panel(8).y+64+3,vc(0),-1,"%s",help_list[help_pos+8]);
    textprintf_ex(menu1,font,panel(8).x+1,panel(8).y+72+3,vc(0),-1,"%s",help_list[help_pos+9]);
    textprintf_ex(menu1,font,panel(8).x+1,panel(8).y+81+3,vc(0),-1,"%s",help_list[help_pos+10]);
    textprintf_ex(menu1,font,panel(8).x+1,panel(8).y+90+3,vc(0),-1,"%s",help_list[help_pos+11]);
}



int d_nbmenu_proc(int msg,DIALOG *d,int c)
{
    static int ret=D_O_K;
    domouse();
    do_animations();
    refresh(rALL);
    
    //  if (msg!=MSG_IDLE)
    if(msg==MSG_GOTMOUSE||msg==MSG_XCHAR)
        //  if (0)
    {
        ComboBrushPause=1;
        refresh(rMAP);
        ComboBrushPause=0;
        restore_mouse();
        clear_tooltip();
    }
    
    Backend::graphics->waitTick();
	Backend::graphics->showBackBuffer();
    ret = jwin_menu_proc(msg,d,c);
    
    
    return ret;
}

bool prv_press=false;

void dopreview()
{
    
    refresh(rMAP);
    
    while(!(Backend::mouse->anyButtonClicked()))
    {
        //ret = jwin_menu_proc(msg,d,c);
        if(keypressed())
        {
            if(!prv_press)
            {
                prv_press=true;
                
                switch(readkey()>>8)
                {
                case KEY_ESC:
                case KEY_ENTER:
                case KEY_ENTER_PAD:
                    goto finished;
                    break;
                    
                case KEY_F:
                    Flags^=cFLAGS;
                    refresh(rMAP);
                    break;
                    
                case KEY_R:
                    onRType();
                    break;
                    
                case KEY_S:
                    onString();
                    break;
                    
                    /*
                              case KEY_E:
                                Map.prv_secrets(true);
                                refresh(rALL);
                                break;
                    */
                case KEY_C:
                    onCopy();
                    break;
                    
                case KEY_A:
                    onCatchall();
                    break;
                    
                case KEY_P:
                    onP();
                    break;
                    
                case KEY_1:
                    Map.prv_dowarp(0,0);
                    prv_warp=0;
                    break;
                    
                case KEY_2:
                    Map.prv_dowarp(0,1);
                    prv_warp=0;
                    break;
                    
                case KEY_3:
                    Map.prv_dowarp(0,2);
                    prv_warp=0;
                    break;
                    
                case KEY_4:
                    Map.prv_dowarp(0,3);
                    prv_warp=0;
                    break;
                    
                case KEY_5:
                    Map.prv_dowarp(1,0);
                    prv_warp=0;
                    break;
                    
                case KEY_6:
                    Map.prv_dowarp(1,1);
                    prv_warp=0;
                    break;
                    
                case KEY_7:
                    Map.prv_dowarp(1,2);
                    prv_warp=0;
                    break;
                    
                case KEY_8:
                    Map.prv_dowarp(1,3);
                    prv_warp=0;
                    break;
                    
                case KEY_9:
                    if(prv_twon)
                    {
                        prv_twon=0;
                        Map.set_prvtime(0);
                        prv_warp=0;
                    }
                    else
                    {
                        Map.set_prvtime(Map.get_prvscr()->timedwarptics);
                        prv_twon=1;
                    }
                    
                    break;
                    
                case KEY_W:
                    onShowWalkability();
                    break;
                    
                case KEY_Q:
                    onShowComboInfoCSet();
                    break;
                    
                case KEY_PGUP:
                    help_pos--;
                    
                    if(help_pos<0)
                    {
                        help_pos=0;
                    }
                    
                    break;
                    
                case KEY_PGDN:
                    help_pos++;
                    
                    if(help_pos>(is_large() ? 1 : 9))
                    {
                        help_pos--;
                    }
                    
                    break;
                }
            }
            else
            {
                readkey();
            }
        }
        else
        {
            prv_press=false;
        }
        
        if(prv_warp)
        {
            Map.prv_dowarp(1,0);
            prv_warp=0;
        }
        
        if(Map.get_prvfreeze())
        {
            if(Map.get_prvadvance())
            {
                do_animations();
                Map.set_prvadvance(0);                
            }
        }
        else
        {
            do_animations();
            Map.set_prvadvance(0);
        }
        
        refresh(rALL);

		if (Map.get_prvtime())
		{
			Map.set_prvtime(Map.get_prvtime() - 1);
			if (!Map.get_prvtime())
			{
				prv_warp = 1;
			}
		}
    }
    
finished:
    //Flags=of;
    reset_combo_animations();
    reset_combo_animations2();
    Backend::mouse->setCursorSprite(mouse_bmp[MOUSE_BMP_NORMAL][0]);
    prv_mode=0;
    Map.set_prvcmb(0);
    Map.set_prvadvance(0);
    Map.set_prvfreeze(0);
    Map.set_prvtime(0);
    prv_warp=0;
    loadlvlpal(Map.getcolor());
    rebuild_trans_table();
    refresh(rMAP+rMENU);
    
    while(Backend::mouse->anyButtonClicked())
    {
		Backend::graphics->waitTick();
		Backend::graphics->showBackBuffer();
        /* do nothing */
    }
}


int onZQVidMode()
{
    char str_a[80], str_b[80], str_c[80];
    str_a[0]=0;
    str_b[0]=0;
    str_c[0]=0;
    
    sprintf(str_c,"%dx%d 8-bit",Backend::graphics->screenW(),Backend::graphics->screenH());
    jwin_alert("Video Mode",Backend::graphics->videoModeString(),str_b,str_c,"OK",NULL,13,27,lfont);
    return D_O_K;
}

bool is_zquest()
{
    return true;
}

bool screenIsScrolling()
{
    return false;
}

int save_config_file()
{
    char cmdnametitle[20];
    char qtnametitle[20];
    char qtpathtitle[20];
    char *datapath2=(char *)zc_malloc(2048);
    char *midipath2=(char *)zc_malloc(2048);
    char *imagepath2=(char *)zc_malloc(2048);
    char *tmusicpath2=(char *)zc_malloc(2048);
    strcpy(datapath2, datapath);
    strcpy(midipath2, midipath);
    strcpy(imagepath2, imagepath);
    strcpy(tmusicpath2, tmusicpath);
    chop_path(datapath2);
    chop_path(midipath2);
    chop_path(imagepath2);
    chop_path(tmusicpath2);

	packfile_password("");
    
    set_config_string("zquest",data_path_name,datapath2);
    set_config_string("zquest",midi_path_name,midipath2);
    set_config_string("zquest",image_path_name,imagepath2);
    set_config_string("zquest",tmusic_path_name,tmusicpath2);
    set_config_string("zquest",last_quest_name,filepath);
    set_config_string("zquest","last_timed_save",last_timed_save);
    set_config_int("zquest","mouse_scroll",MouseScroll);
    set_config_int("zquest","invalid_static",InvalidStatic);
    set_config_int("zquest","tile_protection",TileProtection);
    set_config_int("zquest","showinfo",ShowInfo);
    set_config_int("zquest","show_grid",ShowGrid);
    set_config_int("zquest","grid_color",GridColor);
    set_config_int("zquest","snapshot_format",SnapshotFormat);
    set_config_int("zquest","save_paths",SavePaths);
    set_config_int("zquest","cycle_on",CycleOn);
    set_config_int("zquest","showfps",ShowFPS);
    set_config_int("zquest","combo_brush",ComboBrush);
    set_config_int("zquest","brush_position",BrushPosition);
    set_config_int("zquest","float_brush",FloatBrush);
    set_config_int("zquest","open_last_quest",OpenLastQuest);
    set_config_int("zquest","show_misalignments",ShowMisalignments);
    set_config_int("zquest","showffscripts",ShowFFScripts);
    set_config_int("zquest","showsquares",ShowSquares);
    
    set_config_int("zquest","animation_on",AnimationOn);
    set_config_int("zquest","auto_backup_retention",AutoBackupRetention);
    set_config_int("zquest","auto_save_interval",AutoSaveInterval);
    set_config_int("zquest","auto_save_retention",AutoSaveRetention);
    set_config_int("zquest","uncompressed_auto_saves",UncompressedAutoSaves);
    set_config_int("zquest","overwrite_prevention",OverwriteProtection);
    set_config_int("zquest","import_map_bias",ImportMapBias);
    
    set_config_int("zquest","keyboard_repeat_delay",KeyboardRepeatDelay);
    set_config_int("zquest","keyboard_repeat_rate",KeyboardRepeatRate);
    
    set_config_int("zquest","zqmusic_bufsz",zcmusic_bufsz);
    set_config_int("zquest","rulesetdialog",RulesetDialog);
    set_config_int("zquest","enable_tooltips",EnableTooltips);
    
    for(int x=0; x<7; x++)
    {
        set_bit(LayerMask,x, LayerMaskInt[x]);
    }
    
    int tempvalue=LayerMask[0]+(LayerMask[1]<<8);
    set_config_int("zquest","layer_mask",tempvalue);
    set_config_int("zquest","normal_duplicate_action",DuplicateAction[0]);
    set_config_int("zquest","horizontal_duplicate_action",DuplicateAction[1]);
    set_config_int("zquest","vertical_duplicate_action",DuplicateAction[2]);
    set_config_int("zquest","both_duplicate_action",DuplicateAction[3]);
    set_config_int("zquest","leech_update",LeechUpdate);
    set_config_int("zquest","leech_update_tiles",LeechUpdateTiles);
    set_config_int("zquest","only_check_new_tiles_for_duplicates",OnlyCheckNewTilesForDuplicates);
    set_config_int("zquest","gui_colorset",gui_colorset);
    
    for(int x=0; x<MAXFAVORITECOMMANDS; ++x)
    {
        sprintf(cmdnametitle, "command%02d", x+1);
        set_config_int("zquest",cmdnametitle,favorite_commands[x]);
    }
    
    for(int x=1; x<qt_count+1; x++)
    {
        sprintf(qtnametitle, qtname_name, x);
        sprintf(qtpathtitle, qtpath_name, x);
        
        if(QuestTemplates[x].path[0]!=0)
        {
            set_config_string("zquest",qtnametitle,QuestTemplates[x].name);
            set_config_string("zquest",qtpathtitle,QuestTemplates[x].path);
        }
        else
        {
            break;
        }
    }
    
    //purge old section names here
    set_config_string("zquest","auto_backup",NULL);
    
    //save the beta warning confirmation info
    //10% chance of showing the warning again. heh -DD
    if(rand() % 10 < 9)
    {
        char *uniquestr = getBetaControlString();
        set_config_string("zquest", "beta_warning", uniquestr);
        delete[] uniquestr;
    }
    else
    {
        set_config_string("zquest", "beta_warning", "");
    }
    
    set_config_int("zquest","force_exit",ForceExit);
    
#ifdef _WIN32
    set_config_int("zquest","zq_win_proc_fix",zqUseWin32Proc);
#endif
	Backend::graphics->writeConfigurationOptions("zquest");
    
    
    flush_config_file();
    zc_free(datapath2);
    zc_free(midipath2);
    zc_free(imagepath2);
    zc_free(tmusicpath2);
    return 0;
}

int d_timer_proc(int msg, DIALOG *d, int c)
{
    //these are here to bypass compiler warnings about unused arguments
    c=c;
    d=d;
    
    switch(msg)
    {
    case MSG_IDLE:
#ifdef _WIN32
        if(zqUseWin32Proc != FALSE)
            win32data.Update(); //experimental win32 fixes
            
#endif
            
        // This has been crashing on Windows, and it saves plenty without it
        //check_autosave();
        break;
    }
    
    return D_O_K;
}

void check_autosave()
{
    if(AutoSaveInterval>0)
    {
        time(&auto_save_time_current);
        auto_save_time_diff = difftime(auto_save_time_current,auto_save_time_start);
        
        if(auto_save_time_diff>AutoSaveInterval*60)
        {
			Backend::mouse->setCursorSprite(mouse_bmp[MOUSE_BMP_NORMAL][0]);
            if(first_save)
                replace_extension(last_timed_save, filepath, "qt0", 2047);
            else
                strcpy(last_timed_save, "untitled.qt0");
            go();
            
            if((header.zelda_version != ZELDA_VERSION || header.build != VERSION_BUILD) && first_save)
            {
                jwin_alert("Auto Save","This quest was saved in an older version of ZQuest.","If you wish to use the autosave feature, you must manually","save the files in this version first.","OK",NULL,13,27,lfont);
                time(&auto_save_time_start);
                comeback();
                return;
            }
            
            int ret = save_quest(last_timed_save, true);
            
            if(ret)
            {
                jwin_alert("Error","Timed save did not complete successfully.",NULL,NULL,"O&K",NULL,'k',0,lfont);
                last_timed_save[0]=0;
            }
            
//        jwin_alert("Timed Save","A timed save should happen here",NULL,NULL,"OK",NULL,13,27,lfont);
            save_config_file();
            time(&auto_save_time_start);
            comeback();
        }
    }
}

void flushItemCache() {}
void ringcolor(bool forceDefault)
{
    forceDefault=forceDefault;
}

//annoying beta message :)
char *getBetaControlString()
{
    char *result = new char[11];
    const char *compiledate = __DATE__;
    const char *compiletime = __TIME__;
    int i=0;
    byte tempbyte;
    
    for(i=0; i<zc_min(10, zc_min((int)strlen(compiledate),(int)strlen(compiletime))); i++)
    {
        tempbyte = (compiledate[i]*compiletime[i])^i;
        tempbyte = zc_max(tempbyte, 33);
        tempbyte = zc_min(126, tempbyte);
        result[i] = tempbyte;
    }
    
    for(int j=i; j<11; ++j)
    {
        result[j] = '\0';
    }
    
    return result;
}

bool item_disabled(int)
{
    return false;
}

int onCmdExit()
{
    // replaces onExit for the -large button command "Exit"
    close_button_quit = true;
    return 0;
}

//remember to adjust this number in zquest.h if it changes here!
//P.S: Must be listed in the same order as the enum in zquest.h. No exceptions! -L
command_pair commands[cmdMAX]=
{
    { "(None)",                             0,                     NULL                                                    },
    { "About",                              0, (intF) onAbout                                          },
    { "Catch All",                          0, (intF) onCatchall                                       },
    { "Change track",                       0, (intF) changeTrack                                      },
    { "Cheats",                             0, (intF) onCheats                                         },
    { "Color Set Fix",                      0, (intF) onCSetFix                                        },
    { "Combo Alias Mode",                   0, (intF) onDrawingModeAlias                               },
    { "Edit Combo Aliases",                 0, (intF) onEditComboAlias                                 },
    { "Combos",                             0, (intF) onCombos                                         },
    { "Compile ZScript",                    0, (intF) onCompileScript                                  },
    { "Copy",                               0, (intF) onCopy                                           },
    { "Default Combos",                     0, (intF) onDefault_Combos                                 },
    { "Delete Map",                         0, (intF) onDeleteMap                                      },
    { "Delete Screen",                      0, (intF) onDelete                                         },
    { "DMaps",                              0, (intF) onDmaps                                          },
    { "Door Combo Sets",                    0, (intF) onDoorCombos                                     },
    { "Edit Doors",                         0, (intF) onDoors                                          },
    { "Paste Doors",                        0, (intF) onPasteDoors                                     },
    { "Dungeon Carving Mode",               0, (intF) onDrawingModeDungeon                             },
    { "End String",                         0, (intF) onEndString                                      },
    { "Enemy Editor",                       0, (intF) onCustomEnemies                                  },
    { "Default Enemies",				  0, (intF) onDefault_Guys                                   },
    { "Set Enemies",                        0, (intF) onEnemies                                        },
    { "Paste Enemies",                      0, (intF) onPasteEnemies                                   },
    { "Enhanced Music ",                    0, (intF) onEnhancedMusic                                  },
    { "Exit",                               0, (intF) onCmdExit                                        },
    { "Export Combos",                      0, (intF) onExport_Combos                                  },
    { "Export DMaps",                       0, (intF) onExport_DMaps                                   },
    { "Export Map",                         0, (intF) onExport_Map                                     },
    { "Export Palettes",                    0, (intF) onExport_Pals                                    },
    { "Export Quest Template",              0, (intF) onExport_ZQT                                     },
    { "Export Strings",                     0, (intF) onExport_Msgs                                    },
    { "Export Subscreen",                   0, (intF) onExport_Subscreen                               },
    { "Export Tiles",                       0, (intF) onExport_Tiles                                   },
    { "Export Unencoded Quest",             0, (intF) onExport_UnencodedQuest                          },
    { "Export Graphics Pack",               0, (intF) onExport_ZGP                                     },
    { "Flags",                              0, (intF) onFlags                                          },
    { "Paste Freeform Combos",              0, (intF) onPasteFFCombos                                  },
    { "Freeform Combos",                    0, (intF) onSelectFFCombo                                  },
    { "Game icons",                         0, (intF) onIcons                                          },
    { "Goto Map",                           0, (intF) onGotoMap                                        },
    { "Guy",                                0, (intF) onGuy                                            },
    { "Paste Guy/String",                   0, (intF) onPasteGuy                                       },
    { "Header",                             0, (intF) onHeader                                         },
    { "Help",                               0, (intF) onHelp                                           },
    { "Import ASM FFC Script",              0, (intF) onImportFFScript                                 },
    { "Import ASM Global Script",           0, (intF) onImportGScript                                  },
    { "Import ASM Item Script",             0, (intF) onImportItemScript                               },
    { "Import Combos",                      0, (intF) onImport_Combos                                  },
    { "Import DMaps",                       0, (intF) onImport_DMaps                                   },
    { "Import Graphics Pack",               0, (intF) onImport_ZGP                                     },
    { "Import Map",                         0, (intF) onImport_Map                                     },
    { "Import Palettes",                    0, (intF) onImport_Pals                                    },
    { "Import Quest Template",              0, (intF) onImport_ZQT                                     },
    { "Import Strings",                     0, (intF) onImport_Msgs                                    },
    { "Import Subscreen",                   0, (intF) onImport_Subscreen                               },
    { "Import Tiles",                       0, (intF) onImport_Tiles                                   },
    { "Import Unencoded Quest",             0, (intF) onImport_UnencodedQuest                          },
    { "Info Types",                         0, (intF) onInfoTypes                                      },
    { "Init Data",                          0, (intF) onInit                                           },
    { "Integrity Check (All) ",             0, (intF) onIntegrityCheckAll                              },
    { "Integrity Check (Screens) ",         0, (intF) onIntegrityCheckRooms                            },
    { "Integrity Check (Warps) ",           0, (intF) onIntegrityCheckWarps                            },
    { "Set Item",                           0, (intF) onItem                                           },
    { "Item Locations Report",              0, (intF) onItemLocationReport                             },
    { "Item Editor",                        0, (intF) onCustomItems                                    },
    { "Layers",                             0, (intF) onLayers                                         },
    { "Paste Layers",                       0, (intF) onPasteLayers                                    },
    { "Palettes - Levels",                  0, (intF) onColors_Levels                                  },
    { "Link Sprite",                        0, (intF) onCustomLink                                     },
    { "List Combos Used",                   0, (intF) onUsedCombos                                     },
    { "Palettes - Main",                    0, (intF) onColors_Main                                    },
    { "Map Count",                          0, (intF) onMapCount                                       },
    { "Default Map Styles",                 0, (intF) onDefault_MapStyles                              },
    { "Map Styles",                         0, (intF) onMapStyles                                      },
    { "Master Subscreen Type",              0, (intF) onSubscreen                                      },
    { "Message String",                     0, (intF) onString                                         },
    { "MIDIs",                              0, (intF) onMidis                                          },
    { "Misc Colors",                        0, (intF) onMiscColors                                     },
    { "New",                                0, (intF) onNew                                            },
    { "Normal Mode",                        0, (intF) onDrawingModeNormal                              },
    { "Open",                               0, (intF) onOpen                                           },
    { "Options",                            0, (intF) onOptions                                        },
    { "Palette",                            0, (intF) onScreenPalette                                  },
    { "Default Palettes",                   0, (intF) onDefault_Pals                                   },
    { "Paste",                              0, (intF) onPaste                                          },
    { "Paste All",                          0, (intF) onPasteAll                                       },
    { "Paste All To All",                   0, (intF) onPasteAllToAll                                  },
    { "Paste To All",                       0, (intF) onPasteToAll                                     },
    { "Maze Path",                          0, (intF) onPath                                           },
    { "Play Music",                         0, (intF) playMusic                                        },
    { "Preview Mode",                       0, (intF) onPreviewMode                                    },
    { "Quest Templates",                    0, (intF) onQuestTemplates                                 },
    { "Apply Template to All",              0, (intF) onReTemplate                                     },
    { "Relational Mode",                    0, (intF) onDrawingModeRelational                          },
    { "Revert",                             0, (intF) onRevert                                         },
    { "Room Type",                          0, (intF) onRType                                          },
    { "Paste Room Type Data",               0, (intF) onPasteRoom                                      },
    { "Rules - Animation",                  0, (intF) onAnimationRules                                 },
    { "Save",                               0, (intF) onSave                                           },
    { "Save as",                            0, (intF) onSaveAs                                         },
    { "Paste Screen Data",                  0, (intF) onPasteScreenData                                },
    { "Screen Data",                        0, (intF) onScrData                                        },
    { "Paste Secret Combos",                0, (intF) onPasteSecretCombos                              },
    { "Secret Combos",                      0, (intF) onSecretCombo                                    },
    { "SFX Data",                           0, (intF) onSelectSFX                                      },
    { "Shop Types",                         0, (intF) onShopTypes                                      },
    { "Side Warp",                          0, (intF) onSideWarp                                       },
    { "Palettes - Sprites",                 0, (intF) onColors_Sprites                                 },
    { "Default Weapon Sprites",             0, (intF) onDefault_Weapons                                },
    { "Stop Tunes",                         0, (intF) stopMusic                                        },
    { "Strings",                            0, (intF) onStrings                                        },
    { "Subscreens",                         0, (intF) onEditSubscreens                                 },
    { "Take Snapshot",                      0, (intF) onSnapshot                                       },
    { "The Travels of Link",                0, (intF) playTune1                                        },
    { "NES Dungeon Template",               0, (intF) onTemplate                                       },
    { "Edit Templates",                     0, (intF) onTemplates                                      },
    { "Tile Warp",                          0, (intF) onTileWarp                                       },
    { "Default Tiles",                      0, (intF) onDefault_Tiles                                  },
    { "Tiles",                              0, (intF) onTiles                                          },
    { "Toggle Grid",                        0, (intF) onToggleGrid                                     },
    { "Triforce Pieces",                    0, (intF) onTriPieces                                      },
    { "Under Combo",                        0, (intF) onUnderCombo                                     },
    { "Paste Undercombo",                   0, (intF) onPasteUnderCombo                                },
    { "Undo",                               0, (intF) onUndo                                           },
    { "Video Mode",                         0, (intF) onZQVidMode                                      },
    { "View Map",                           0, (intF) onViewMap                                        },
    { "View Palette",                       0, (intF) onShowPal                                        },
    { "View Pic",                           0, (intF) onViewPic                                        },
    { "Paste Warp Return",                  0, (intF) onPasteWarpLocations                             },
    { "Warp Rings",                         0, (intF) onWarpRings                                      },
    { "Paste Warps",                        0, (intF) onPasteWarps                                     },
    { "Weapons/Misc",                       0, (intF) onCustomWpns                                     },
    { "View Darkness",                      0, (intF) onShowDarkness                                   },
    { "Toggle Walkability",                 0, (intF) onShowWalkability                                },
    { "Toggle Flags",                       0, (intF) onShowFlags                                      },
    { "Toggle CSets",                       0, (intF) onShowCSet                                       },
    { "Toggle Types",                       0, (intF) onShowCType                                      },
    { "Rules - Combos",                     0, (intF) onComboRules                                     },
    { "Rules - Items",                      0, (intF) onItemRules                                      },
    { "Rules - Enemies",                    0, (intF) onEnemyRules                                     },
    { "Rules - NES Fixes",                  0, (intF) onFixesRules                                     },
    { "Rules - Other",                      0, (intF) onMiscRules                                      },
    { "Item Drop Set Editor",               0, (intF) onItemDropSets                                   },
    { "Default Items",                      0, (intF) onDefault_Items                                  },
    { "Paste Palette",                      0, (intF) onPastePalette                                   },
    { "Rules - Compatibility",              0, (intF) onCompatRules                                    }
};

/********************************/
/*****      Tool Tips      ******/
/********************************/
int strchrnum(char *str, char c)
{
    for(int i=0; str[i]; ++i)
    {
        if(str[i]==c)
        {
            return i;
        }
    }
    
    return -1;
}

int get_longest_line_length(FONT *f, char *str)
{
    int maxlen=0;
    //char *kill=(char *)calloc(strlen(str),1);
    char *tmpstr=str;
    char temp=0;
    //sprintf(tmpstr, "%s", str);
    int t=0;
    int new_t=-1;
    
    while(tmpstr[t])
    {
        t=strchrnum(tmpstr, '\n');
        
        if(t==-1)
        {
            t=(int)strlen(tmpstr);
        }
        
        if((unsigned int)t!=strlen(tmpstr))
        {
            new_t=t+1;
        }
        else
        {
            new_t=-1;
        }
        
        temp = tmpstr[t];
        tmpstr[t]=0;
        maxlen=zc_max(maxlen,text_length(f, tmpstr));
        tmpstr[t]=temp;
        
        if(new_t!=-1)
        {
            tmpstr+=new_t;
        }
    }
    
    //zc_free(kill);
    return maxlen;
}

int count_lines(char *str)
{
    int count=1;
    
    for(word i=0; i<strlen(str); ++i)
    {
        if(str[i]=='\n')
        {
            ++count;
        }
    }
    
    return count;
}

void update_tooltip(int x, int y, int trigger_x, int trigger_y, int trigger_w, int trigger_h, char *tipmsg)
{
    if(!EnableTooltips)
    {
        return;
    }
    
    tooltip_trigger.x=trigger_x;
    tooltip_trigger.y=trigger_y;
    tooltip_trigger.w=trigger_w;
    tooltip_trigger.h=trigger_h;
    
    
    if(x<0||y<0) //if we want to clear the tooltip
    {
        tooltip_box.x=x;
        tooltip_box.y=y;
        tooltip_box.w=0;
        tooltip_box.h=0;
        tooltip_timer=0;
        return; //cancel
    }
    
    y+=16;
    
    if(tooltip_timer<=tooltip_maxtimer)
    {
        ++tooltip_timer;
    }
    
    if(tooltip_timer==tooltip_maxtimer)
    {
        tooltip_box.x=x;
        tooltip_box.y=y;
        int lines=count_lines(tipmsg);
        tooltip_box.w=get_longest_line_length(font, tipmsg)+8+1;
        tooltip_box.h=(lines*text_height(font))+8+1;
        
        if(tooltip_box.x+tooltip_box.w>=Backend::graphics->virtualScreenW())
        {
            tooltip_box.x=(Backend::graphics->virtualScreenW() - tooltip_box.w);
        }
        
        if(tooltip_box.y+tooltip_box.h>=Backend::graphics->virtualScreenH())
        {
            tooltip_box.y=(Backend::graphics->virtualScreenH() - tooltip_box.h);
        }
        
        rectfill(tooltipbmp, 1, 1, tooltip_box.w-3, tooltip_box.h-3, jwin_pal[jcTEXTBG]);
        rect(tooltipbmp, 0, 0, tooltip_box.w-2, tooltip_box.h-2, jwin_pal[jcTEXTFG]);
        vline(tooltipbmp, tooltip_box.w-1, 0,           tooltip_box.h-1, jwin_pal[jcTEXTFG]);
        hline(tooltipbmp,           1, tooltip_box.h-1, tooltip_box.w-2, jwin_pal[jcTEXTFG]);
        tooltipbmp->line[tooltip_box.w-1][0]=0;
        tooltipbmp->line[0][tooltip_box.h-1]=0;
        
        //char *kill=(char *)calloc(strlen(tipmsg)*2,1);
        char *tmpstr=tipmsg;
        char temp = 0;
        //sprintf(tmpstr, "%s", tipmsg);
        int t=0;
        int new_t=-1;
        int i=0;
        
        while(tmpstr[t])
        {
            t=strchrnum(tmpstr, '\n');
            
            if(t==-1)
            {
                t=(int)strlen(tmpstr);
            }
            
            if((unsigned int)t!=strlen(tmpstr))
            {
                new_t=t+1;
            }
            else
            {
                new_t=-1;
            }
            
            temp = tmpstr[t];
            tmpstr[t]=0;
            textprintf_ex(tooltipbmp, font, 4, (i*text_height(font))+4, vc(0), -1, "%s", tmpstr);
            tmpstr[t]=temp;
            ++i;
            
            if(new_t!=-1)
            {
                tmpstr+=new_t;
                t=0;
            }
        }
        
        //zc_free(kill);
    }
    
    return;
}

void clear_tooltip()
{
    update_tooltip(-1, -1, -1, -1, 0, 0, NULL);
}

void printZAsm()
{
	for (std::map<int, pair<string, string> >::iterator it = ffcmap.begin();
		 it != ffcmap.end(); ++it)
	{
		if (it->second.second != "")
		{
			ZAsmScript& script = scripts.ffscripts[it->first];
			int32_t commands_len = script.commands_len;
			zasm* commands = script.commands;
			if (commands_len == 0 || commands[0].command == 0xFFFF) continue;
			al_trace("\n%s\n", it->second.second.c_str());
			for (int i = 0; i < commands_len && commands[i].command != 0xFFFF; ++i)
			{
				al_trace("  %s  ; %d %d %d\n", to_string(commands[i]).c_str(),
						 commands[i].command, commands[i].arg1, commands[i].arg2);
			}
		}
	}

	for (std::map<int, pair<string, string> >::iterator it = globalmap.begin();
		 it != globalmap.end(); ++it)
	{
		if (it->second.second != "")
		{
			ZAsmScript& script = scripts.globalscripts[it->first];
			int32_t commands_len = script.commands_len;
			zasm* commands = script.commands;
			if (commands_len == 0 || commands[0].command == 0xFFFF) continue;
			al_trace("\n%s\n", it->second.second.c_str());
			for (int i = 0; i < commands_len && commands[i].command != 0xFFFF; ++i)
				al_trace("  %s\n", to_string(commands[i]).c_str());
		}
	}

	for (std::map<int, pair<string, string> >::iterator it = itemmap.begin();
		 it != itemmap.end(); ++it)
	{
		if (it->second.second != "")
		{
			ZAsmScript& script = scripts.itemscripts[it->first];
			int32_t commands_len = script.commands_len;
			zasm* commands = script.commands;
			if (commands_len == 0 || commands[0].command == 0xFFFF) continue;
			al_trace("\n%s\n", it->second.second.c_str());
			for (int i = 0; i < commands_len && commands[i].command != 0xFFFF; ++i)
				al_trace("  %s\n", to_string(commands[i]).c_str());
		}
	}
}

/////////////////////////////////////////////////
// zc_malloc
/////////////////////////////////////////////////

//Want Logging:
//Set this to 1 to allow zc_malloc/zc_free to track pointers and
//write logging data to allegro.log
#define ZC_DEBUG_MALLOC_WANT_LOGGING_INFO 0


#include <set>
#include "vectorset.h"

#if (defined(NDEBUG) || !defined(_DEBUG)) && (ZC_DEBUG_MALLOC_ENABLED) && (ZC_DEBUG_MALLOC_WANT_LOGGING_INFO) //this is not fun with debug
#define ZC_WANT_DETAILED_MALLOC_LOGGING 1
#endif

#if ZC_WANT_DETAILED_MALLOC_LOGGING
size_t totalBytesAllocated = 0;
//typedef vectorset<void*> debug_malloc_pool_type; //to slow for zquest (size is huge)
typedef std::set<void*> debug_malloc_pool_type;
debug_malloc_pool_type debug_zc_malloc_allocated_pool;
#endif

void* __zc_debug_malloc(size_t numBytes, const char* file, int line)
{
#ifdef ZC_WANT_DETAILED_MALLOC_LOGGING
    static bool zcDbgMallocInit = false;
    
    if(!zcDbgMallocInit)
    {
        zcDbgMallocInit = true;
        //debug_zc_malloc_allocated_pool.reserve(1 << 17);
        //yeah. completely ridiculous... there's no reason zc should ever need this many..
        //BUT it does... go figure
    }
    
    totalBytesAllocated += numBytes;
    
    al_trace("INFO: %i : %s, line %i, %u bytes, pool size %u, total %u,",
             0,
             file,
             line,
             numBytes,
             debug_zc_malloc_allocated_pool.size(),
             totalBytesAllocated / 1024
            );
#endif
            
    ZC_MALLOC_ALWAYS_ASSERT(numBytes != 0);
    void* p = malloc(numBytes);
    
#ifdef ZC_WANT_DETAILED_MALLOC_LOGGING
    al_trace("at address %x\n", (int)p);
    
    if(!p)
        al_trace("____________ ERROR: __zc_debug_malloc: returned null. out of memory.\n");
        
    //debug_malloc_pool_type::insert_iterator_type it = debug_zc_malloc_allocated_pool.insert(p);
    std::pair< std::set<void*>::iterator, bool > it = debug_zc_malloc_allocated_pool.insert(p);
    
    if(!it.second)
        al_trace("____________ ERROR: malloc returned identical address to one in use... No way Jose!\n");
        
#endif
        
    return p;
}


void __zc_debug_free(void* p, const char* file, int line)
{
    ZC_MALLOC_ALWAYS_ASSERT(p != 0);
    
#ifdef ZC_WANT_DETAILED_MALLOC_LOGGING
    al_trace("INFO: %i : %s line %i, freeing memory at address %x\n", 0, file, line, (int)p);
    
    size_t numErased = debug_zc_malloc_allocated_pool.erase(p);
    
    if(numErased == 0)
        al_trace("____________ ERROR: __zc_debug_free: no known ptr to memory exists. ..attempting to free it anyways.\n");
        
#endif
        
    free(p);
}


void __zc_debug_malloc_free_print_memory_leaks()
{
#if ZC_WANT_DETAILED_MALLOC_LOGGING
    al_trace("LOGGING INFO FROM debug_zc_malloc_allocated_pool:\n");
    
    for(debug_malloc_pool_type::iterator it = debug_zc_malloc_allocated_pool.begin();
            it != debug_zc_malloc_allocated_pool.end();
            ++it
       )
    {
        al_trace("block at address %x.\n", (int)*it);
    }
    
#endif
}


void __zc_always_assert(bool e, const char* expression, const char* file, int line)
{
    //for best results set a breakpoint in here.
    if(!e)
    {
        char buf[1024];
        sprintf("ASSERTION FAILED! : %s, %s line %i\n", expression, file, line);
        
        al_trace("%s", buf);
        set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
        allegro_message("%s", buf);
        //...
    }
}


/* end */

