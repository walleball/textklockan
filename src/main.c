#include "pebble.h"

static Window *s_wndMain;
static TextLayer *g_layerOneOfOne;
static TextLayer *g_layerOneOfTwo, *g_layerTwoOfTwo;
static TextLayer *g_layerOneOfThree, *g_layerTwoOfThree, *g_layerThreeOfThree;
static TextLayer *g_layerOneOfFour, *g_layerTwoOfFour, *g_layerThreeOfFour, *g_layerFourOfFour;

// static GFont g_fontLite;
static GFont g_fontRegular;
//static GFont g_fontBold;
static GFont g_fontExtraBold;

WatchInfoModel g_model;
static int g_nTop;
static int g_nWidth;

static int g_nBatteryLevel;
static bool g_bConnected;
static bool g_bConnectedCallback;

static int GetEventStrings(struct tm *ptmTime, char* pszEvent1, char* pszEvent2)
{
	strcpy(pszEvent1, "");
	strcpy(pszEvent2, "");

	int nMinute = ptmTime->tm_min;
	int nHour = ptmTime->tm_hour;
	int nYear = ptmTime->tm_year + 1900;
	int nMonth = ptmTime->tm_mon + 1;
	int nDay = ptmTime->tm_mday;

	// Kalle Anka
	if (nMonth == 12 && nDay == 24)
	{
		if (nHour == 14 && nMinute >= 38)
		{
			strcpy(pszEvent1, "KALLE");
			strcpy(pszEvent2, "ANKA");
			return nMinute - 60;
		}
	}
	// Nyår
	if (nMonth == 12 && nDay == 31)
	{
		if (nHour == 23 && nMinute >= 38)
		{
			snprintf(pszEvent1, 32, "%d", nYear + 1);
			return nMinute - 60;
		}
	}

	return 0;
}

int GetMinutesUntil(int hour1, int minute1, int hour2, int minute2)
{
	if ((hour1 > hour2) || (hour1 == hour2 && minute1 > minute2))
	{
		return -GetMinutesUntil(hour2, minute2, hour1, minute1);
	}

	int nMinutes = minute2 - minute1;
	if (nMinutes < 0)
	{
		nMinutes = nMinutes + 60;
		hour1 = hour1 + 1;
	}
	nMinutes = nMinutes + 60 * (hour2 - hour1);
	return nMinutes;
}

struct entry
{
	int wday;
	char subject[16];
	int hour;
	int minute;
};

struct entry schedule[] = {
	{ 1, "MA", 8, 10 },
	{ 1, "Mentor", 9, 20 },
	{ 1, "IDH", 9, 55 },
	{ 1, "Lunch", 11, 5 },
	{ 1, "EN", 11, 55 },
	{ 1, "SV", 12, 55 },
	{ 1, "HKK", 14, 0 },
	{ 1, "FRI", 15, 35 },

	{ 2, "EN", 9, 50 },
	{ 2, "SV", 10, 20 },
	{ 2, "IDH", 11, 25 },
	{ 2, "Lunch", 12, 35 },
	{ 2, "NO", 13, 20 },
	{ 2, "Elev Val", 14, 30 },
	{ 2, "FRI", 15, 30 },

	{ 3, "MA", 8, 10 },
	{ 3, "SP2", 9, 10 },
	{ 3, "SO", 10, 5 },
	{ 3, "Lunch", 11, 15 },
	{ 3, "SL", 12, 15 },
	{ 3, "EN", 13, 30 },
	{ 3, "SV", 14, 10 },
	{ 3, "FRI", 14, 45 },

	{ 4, "MA", 8, 10 },
	{ 4, "NO", 9, 15 },
	{ 4, "SO", 10, 10 },
	{ 4, "Lunch", 11, 10 },
	{ 4, "SV", 11, 55 },
	{ 4, "KLR/KLM", 13, 10 },
	{ 4, "FRI", 13, 50 },

	{ 5, "SP2", 9, 5 },
	{ 5, "SO", 10, 5 },
	{ 5, "NO", 11, 25 },
	{ 5, "Lunch", 12, 15 },
	{ 5, "SV", 13, 0 },
	{ 5, "FRI", 13, 45 },

	{ 6, "krysset", 10, 3 }
};

#define MONDAY		1
#define TUESDAY		2
#define WEDNESDAY	3
#define THURSDAY	4
#define FRIDAY		5
#define SATURDAY	6
#define SUNDAY		7


static int CheckScheduleEvent(struct tm *ptmTime, int nWeekDay, char* pszSubject, int nHour, int nMinute, char* pszString1, char* pszString2)
{
	if (ptmTime->tm_wday != nWeekDay)
		return false;
	int nMinutesUntil = GetMinutesUntil(ptmTime->tm_hour, ptmTime->tm_min, nHour, nMinute);
	if (nMinutesUntil >= 0 && nMinutesUntil <= 22)
	{
		strcpy(pszString1, pszSubject);
		strftime(pszString2, 16, clock_is_24h_style() ? "%H:%M" : "%I:%M", ptmTime);
		return -nMinutesUntil;
	}
	return 0;
}

#define CHECK_SCHEDULE_EVENT(EVENT_DAY, EVENT_SUBJECT, EVENT_HOUR, EVENT_MINUTE)   \
	nMinutesUntil = CheckScheduleEvent(ptmTime, EVENT_DAY, EVENT_SUBJECT, EVENT_HOUR, EVENT_MINUTE, pszString1, pszString2); \
	if (nMinutesUntil != 0) \
		return nMinutesUntil;

static int GetScheduleStrings(struct tm *ptmTime, char* pszString1, char* pszString2)
{
	strcpy(pszString1, "");
	strcpy(pszString2, "");

#if 0
	int nMinutesUntil;
  
	CHECK_SCHEDULE_EVENT(MONDAY, "skolgång", 7, 50)
	CHECK_SCHEDULE_EVENT(MONDAY, "frukost", 9, 30)
	CHECK_SCHEDULE_EVENT(MONDAY, "skolslut", 15, 35)

	CHECK_SCHEDULE_EVENT(TUESDAY, "skolgång", 9, 30)
	CHECK_SCHEDULE_EVENT(TUESDAY, "fika", 15, 15)
	CHECK_SCHEDULE_EVENT(TUESDAY, "skolslut", 15, 30)

	CHECK_SCHEDULE_EVENT(WEDNESDAY, "skolgång", 7, 50)
	CHECK_SCHEDULE_EVENT(WEDNESDAY, "frukost", 9, 30)
	CHECK_SCHEDULE_EVENT(WEDNESDAY, "skolslut", 14, 45)

	CHECK_SCHEDULE_EVENT(THURSDAY, "skolgång", 7, 50)
	CHECK_SCHEDULE_EVENT(THURSDAY, "frukost", 9, 30)
	CHECK_SCHEDULE_EVENT(THURSDAY, "skolslut", 13, 50)

	CHECK_SCHEDULE_EVENT(FRIDAY, "skolgång", 8, 45)
	CHECK_SCHEDULE_EVENT(FRIDAY, "frukost", 9, 30)
	CHECK_SCHEDULE_EVENT(FRIDAY, "skolslut", 13, 45)

	CHECK_SCHEDULE_EVENT(SATURDAY, "krysset", 10, 3)
#endif
	return 0;
}

char *szHours[12] = { "TOLV", "ETT", "TVÅ", "TRE", "FYRA", "FEM", "SEX",
"SJU", "ÅTTA", "NIO", "TIO", "ELVA" };

static int GetHourStrings(struct tm *ptmTime, char* pszEvent1, char* pszEvent2)
{
	strcpy(pszEvent1, "");
	strcpy(pszEvent2, "");

	int nMinute = GetEventStrings(ptmTime, pszEvent1, pszEvent2);
	if (strlen(pszEvent1) != 0)
	{
		return nMinute;
	}

	nMinute = GetScheduleStrings(ptmTime, pszEvent1, pszEvent2);
	if (strlen(pszEvent1) != 0)
	{
		return nMinute;
	}

	nMinute = ptmTime->tm_min;
	int nHour = ptmTime->tm_hour;

	if (nMinute >= 38)
	{
		strcpy(pszEvent1, szHours[(nHour + 1) % 12]);
		return nMinute - 60;
	}
	else if (nMinute >= 23)
	{
		strcpy(pszEvent1, "HALV");
		strcpy(pszEvent2, szHours[(nHour + 1) % 12]);
		return nMinute - 30;
	}
	else // if (nMinute >= 0 && nMinute < 23)
	{
		strcpy(pszEvent1, szHours[nHour % 12]);
		return nMinute;
	}
}


static void getTimeStrings(struct tm *ptmTime, char* pszString1, char* pszString2, char* pszString3, char* pszString4)
{
	strcpy(pszString1, "");
	strcpy(pszString2, "");
	strcpy(pszString3, "");
	strcpy(pszString4, "");

  if (g_bConnected && !g_bConnectedCallback)
  {
    g_bConnected = false;
    // duoble pulse to warn of lost connection
		vibes_double_pulse();
		vibes_double_pulse();
 		strcpy(pszString1, "VAR ÄR");
 		strcpy(pszString2, "MOBILEN?");
		strftime(pszString3, 16, clock_is_24h_style() ? "%H:%M" : "%I:%M", ptmTime);
		return;
	}
  
	if (!g_bConnected)
	{
		strftime(pszString1, 16, clock_is_24h_style() ? "%H:%M" : "%I:%M", ptmTime);
		return;
	}

	static char szEvent1[32];
	static char szEvent2[32];

	int nMinute = GetHourStrings(ptmTime, szEvent1, szEvent2);

	if (nMinute <= -18)
	{
		strcpy(pszString1, "TJUGO I");
		strcpy(pszString2, szEvent1);
		strcpy(pszString3, szEvent2);
	}
	else if (nMinute <= -13)
	{
		strcpy(pszString1, "KVART I");
		strcpy(pszString2, szEvent1);
		strcpy(pszString3, szEvent2);
	}
	else if (nMinute <= -8)
	{
		strcpy(pszString1, "TIO I");
		strcpy(pszString2, szEvent1);
		strcpy(pszString3, szEvent2);
	}
	else if (nMinute <= -3)
	{
		strcpy(pszString1, "FEM I");
		strcpy(pszString2, szEvent1);
		strcpy(pszString3, szEvent2);
	}
	else if (nMinute <= -2)
	{
		strcpy(pszString1, "SNART");
		strcpy(pszString2, szEvent1);
		strcpy(pszString3, szEvent2);
	}
	else if (nMinute <= -1)
	{
		strcpy(pszString1, "STRAX");
		strcpy(pszString2, szEvent1);
		strcpy(pszString3, szEvent2);
	}
	else if (nMinute <= 0)
	{
		strcpy(pszString1, "PRICK");
		strcpy(pszString2, szEvent1);
		strcpy(pszString3, szEvent2);
	}
	else if (nMinute <= 1)
	{
		strcpy(pszString1, "STRAX");
		strcpy(pszString2, "ÖVER");
		strcpy(pszString3, szEvent1);
		strcpy(pszString4, szEvent2);
	}
	else if (nMinute <= 2)
	{
		strcpy(pszString1, "LITE");
		strcpy(pszString2, "ÖVER");
		strcpy(pszString3, szEvent1);
		strcpy(pszString4, szEvent2);
	}
	else if (nMinute <= 7)
	{
		strcpy(pszString1, "FEM");
		strcpy(pszString2, "ÖVER");
		strcpy(pszString3, szEvent1);
		strcpy(pszString4, szEvent2);
	}
	else if (nMinute <= 12)
	{
		strcpy(pszString1, "TIO");
		strcpy(pszString2, "ÖVER");
		strcpy(pszString3, szEvent1);
		strcpy(pszString4, szEvent2);
	}
	else if (nMinute <= 17)
	{
		strcpy(pszString1, "KVART");
		strcpy(pszString2, "ÖVER");
		strcpy(pszString3, szEvent1);
		strcpy(pszString4, szEvent2);
		if (nMinute == ptmTime->tm_min && (ptmTime->tm_hour % 12) == 3)
		{
			strcpy(pszString3, "DRAG");
			strcpy(pszString4, "HÅLET");
		}
	}
	else
	{
		strcpy(pszString1, "TJUGO");
		strcpy(pszString2, "ÖVER");
		strcpy(pszString3, szEvent1);
		strcpy(pszString4, szEvent2);
	}

}


static void UpdateDisplay(struct tm *ptmTime)
{
	static char szText1[32];
	static char szText2[32];
	static char szText3[32];
	static char szText4[32];

  if (ptmTime == NULL)
  {
    time_t temp = time(NULL);
    ptmTime = localtime(&temp);
  }
 
	getTimeStrings(ptmTime, szText1, szText2, szText3, szText4);

	// hide all layers
	text_layer_set_size(g_layerOneOfOne, GSize(0, 0));
	text_layer_set_size(g_layerOneOfTwo, GSize(0, 0));
	text_layer_set_size(g_layerTwoOfTwo, GSize(0, 0));
	text_layer_set_size(g_layerOneOfThree, GSize(0, 0));
	text_layer_set_size(g_layerTwoOfThree, GSize(0, 0));
	text_layer_set_size(g_layerThreeOfThree, GSize(0, 0));
	text_layer_set_size(g_layerOneOfFour, GSize(0, 0));
	text_layer_set_size(g_layerTwoOfFour, GSize(0, 0));
	text_layer_set_size(g_layerThreeOfFour, GSize(0, 0));
	text_layer_set_size(g_layerFourOfFour, GSize(0, 0));

	// determine battery color
	GColor colorText = GColorWhite;
	if (g_model == WATCH_INFO_MODEL_PEBBLE_ORIGINAL)
	{
		if (g_nBatteryLevel <= 20)
		{
			time_t temp = time(NULL);
			struct tm *ptmTime = localtime(&temp);
			snprintf(szText1, 32, "%d%%", g_nBatteryLevel);
			strftime(szText2, 16, clock_is_24h_style() ? "%H:%M" : "%I:%M", ptmTime);
			strcpy(szText3, "");
		}
	}
	else // (g_model != WATCH_INFO_MODEL_PEBBLE_ORIGINAL)
	{
		if (g_nBatteryLevel <= 5)
		{
			colorText = GColorRed;
		}
		else if (g_nBatteryLevel <= 10)
		{
			colorText = GColorOrange;
		}
		else if (g_nBatteryLevel <= 20)
		{
			colorText = GColorChromeYellow;
		}
		else if (g_nBatteryLevel <= 30)
		{
			colorText = GColorYellow;
		}
		else if (g_nBatteryLevel <= 40)
		{
			colorText = GColorGreen;
		}
	}

	// set color of all layers
	text_layer_set_text_color(g_layerOneOfOne, colorText);
	text_layer_set_text_color(g_layerOneOfTwo, colorText);
	text_layer_set_text_color(g_layerTwoOfTwo, colorText);
	text_layer_set_text_color(g_layerOneOfThree, colorText);
	text_layer_set_text_color(g_layerTwoOfThree, colorText);
	text_layer_set_text_color(g_layerThreeOfThree, colorText);
	text_layer_set_text_color(g_layerOneOfFour, colorText);
	text_layer_set_text_color(g_layerTwoOfFour, colorText);
	text_layer_set_text_color(g_layerThreeOfFour, colorText);
	text_layer_set_text_color(g_layerFourOfFour, colorText);

	// unhide relevant layers
	if (strlen(szText2) == 0)
	{
		text_layer_set_size(g_layerOneOfOne, GSize(g_nWidth, 50));
		text_layer_set_text(g_layerOneOfOne, szText1);
	}
	else if (strlen(szText3) == 0)
	{
		text_layer_set_size(g_layerOneOfTwo, GSize(g_nWidth, 50));
		text_layer_set_size(g_layerTwoOfTwo, GSize(g_nWidth, 50));
		text_layer_set_text(g_layerOneOfTwo, szText1);
		text_layer_set_text(g_layerTwoOfTwo, szText2);
	}
	else if (strlen(szText4) == 0)
	{
		text_layer_set_size(g_layerOneOfThree, GSize(g_nWidth, 50));
		text_layer_set_size(g_layerTwoOfThree, GSize(g_nWidth, 50));
		text_layer_set_size(g_layerThreeOfThree, GSize(g_nWidth, 50));
		text_layer_set_text(g_layerOneOfThree, szText1);
		text_layer_set_text(g_layerTwoOfThree, szText2);
		text_layer_set_text(g_layerThreeOfThree, szText3);
	}
	else
	{
		text_layer_set_size(g_layerOneOfFour, GSize(g_nWidth, 50));
		text_layer_set_size(g_layerTwoOfFour, GSize(g_nWidth, 50));
		text_layer_set_size(g_layerThreeOfFour, GSize(g_nWidth, 50));
		text_layer_set_size(g_layerFourOfFour, GSize(g_nWidth, 50));
		text_layer_set_text(g_layerOneOfFour, szText1);
		text_layer_set_text(g_layerTwoOfFour, szText2);
		text_layer_set_text(g_layerThreeOfFour, szText3);
		text_layer_set_text(g_layerFourOfFour, szText4);
	}
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed)
{
  if (tick_time->tm_sec == 0)
  {
    UpdateDisplay(tick_time);
  }
  else if (tick_time->tm_sec == 30)
  {
    // move the time up half a minute
    tick_time->tm_min += 1;
    if (tick_time->tm_min == 60)
    {
      tick_time->tm_min = 0;
      tick_time->tm_hour += 1;
      if (tick_time->tm_hour == 24)
      {
        tick_time->tm_hour = 0;  
      }
    }
    UpdateDisplay(tick_time);
  }
}

static void battery_callback(BatteryChargeState state)
{
	// Record the new battery level
	g_nBatteryLevel = state.charge_percent;
//	UpdateDisplay(NULL);
}

static void bluetooth_callback(bool connected) 
{
  g_bConnectedCallback = connected;
  if (connected && !g_bConnected)
  {
    g_bConnected = true;
		// Issue a vibrating alert
		vibes_double_pulse();
  	UpdateDisplay(NULL);
  }
}


static TextLayer* CreateLayer(int y, int h, GFont font)
{
	TextLayer* layer = text_layer_create(GRect(0, g_nTop + y, g_nWidth, h));
	// Improve the layout to be more like a watchface
	text_layer_set_text_color(layer, GColorWhite);
	text_layer_set_text(layer, "");
	//	text_layer_set_font(g_layerOneOfTwo, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK)); // FONT_KEY_BITHAM_42_LIGHT
	text_layer_set_font(layer, font);
	text_layer_set_background_color(layer, GColorBlack);
	text_layer_set_text_alignment(layer, GTextAlignmentCenter);

	return layer;
}

static void main_window_load(Window *window) {
	// Get information about the Window
	Layer *window_layer = window_get_root_layer(window);
	window_set_background_color(window, GColorBlack);
	GRect bounds = layer_get_bounds(window_layer);

	g_model = watch_info_get_model();
	g_model = WATCH_INFO_MODEL_PEBBLE_ORIGINAL;
	g_model = WATCH_INFO_MODEL_PEBBLE_TIME_ROUND_20;

	//	g_nTop = PBL_IF_ROUND_ELSE(6, 0);
	g_nTop = PBL_IF_ROUND_ELSE(3, 0);
	g_nWidth = bounds.size.w;
	g_nBatteryLevel = 100;
	g_bConnected = true;
  g_bConnectedCallback = true;

	// Create GFont
	// 	g_fontLite = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_OPENSANS_LIGHT_40));
	g_fontRegular = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_OPENSANS_REGULAR_32));
	//	g_fontBold = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_OPENSANS_BOLD_40));
	g_fontExtraBold = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_OPENSANS_EXTRABOLD_40));

	int nRegularHeight = 40;
	int nBoldHeight = 45;
	int nSpacing = 40;

	int nOffset1 = 60;
	int nOffset2 = 40;
	int nOffset3 = 20;
	int nOffset4 = 0;

	// Create the TextLayer with specific bounds
	g_layerOneOfOne = CreateLayer(nOffset1, nBoldHeight, g_fontExtraBold);
	layer_add_child(window_layer, text_layer_get_layer(g_layerOneOfOne));

	g_layerOneOfTwo = CreateLayer(nOffset2, nRegularHeight, g_fontRegular);
	g_layerTwoOfTwo = CreateLayer(nOffset2 + nSpacing, nBoldHeight, g_fontExtraBold);
	layer_add_child(window_layer, text_layer_get_layer(g_layerOneOfTwo));
	layer_add_child(window_layer, text_layer_get_layer(g_layerTwoOfTwo));

	g_layerOneOfThree = CreateLayer(nOffset3, nRegularHeight, g_fontRegular);
	g_layerTwoOfThree = CreateLayer(nOffset3 + nSpacing, nRegularHeight, g_fontRegular);
	g_layerThreeOfThree = CreateLayer(nOffset3 + nSpacing + nSpacing, nBoldHeight, g_fontExtraBold);
	layer_add_child(window_layer, text_layer_get_layer(g_layerOneOfThree));
	layer_add_child(window_layer, text_layer_get_layer(g_layerTwoOfThree));
	layer_add_child(window_layer, text_layer_get_layer(g_layerThreeOfThree));

	g_layerOneOfFour = CreateLayer(nOffset4, nRegularHeight, g_fontRegular);
	g_layerTwoOfFour = CreateLayer(nOffset4 + nSpacing, nRegularHeight, g_fontRegular);
	g_layerThreeOfFour = CreateLayer(nOffset4 + nSpacing + nSpacing, nRegularHeight, g_fontRegular);
	g_layerFourOfFour = CreateLayer(nOffset4 + nSpacing + nSpacing + nSpacing, nBoldHeight, g_fontExtraBold);
	layer_add_child(window_layer, text_layer_get_layer(g_layerOneOfFour));
	layer_add_child(window_layer, text_layer_get_layer(g_layerTwoOfFour));
	layer_add_child(window_layer, text_layer_get_layer(g_layerThreeOfFour));
	layer_add_child(window_layer, text_layer_get_layer(g_layerFourOfFour));

	/*
	// Create the TextLayer with specific bounds
	//  s_time_layer = text_layer_create(
	//      GRect(0, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w, 50));
	g_layerOneOfTwo = text_layer_create(GRect(0, 35, bounds.size.w, 50));
	// Improve the layout to be more like a watchface
	text_layer_set_text_color(g_layerOneOfTwo, GColorWhite);
	text_layer_set_text(g_layerOneOfTwo, "");
	//	text_layer_set_font(g_layerOneOfTwo, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK)); // FONT_KEY_BITHAM_42_LIGHT
	text_layer_set_font(g_layerOneOfTwo, g_fontLite);
	text_layer_set_background_color(g_layerOneOfTwo, GColorBlack);


	//  text_layer_set_background_color(s_time_layer, GColorClear);
	//  text_layer_set_text_color(s_time_layer, GColorBlack);
	//  text_layer_set_text(s_time_layer, "00:00");
	//  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
	text_layer_set_text_alignment(g_layerOneOfTwo, GTextAlignmentCenter);

	// Add it as a child layer to the Window's root layer
	layer_add_child(window_layer, text_layer_get_layer(g_layerOneOfTwo));

	g_layerTwoOfTwo = text_layer_create(GRect(0, 80, bounds.size.w, 50));

	// Improve the layout to be more like a watchface
	text_layer_set_text_color(g_layerTwoOfTwo, GColorWhite);
	text_layer_set_text(g_layerTwoOfTwo, "");
	//	text_layer_set_font(g_layerTwoOfTwo, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
	text_layer_set_font(g_layerTwoOfTwo, g_fontRegular);
	text_layer_set_background_color(g_layerTwoOfTwo, GColorBlack);
	text_layer_set_text_alignment(g_layerTwoOfTwo, GTextAlignmentCenter);

	// Add it as a child layer to the Window's root layer
	layer_add_child(window_layer, text_layer_get_layer(g_layerTwoOfTwo));
	*/

	// show time on startup
	UpdateDisplay(NULL);
}

static void main_window_unload(Window *window) {
	// Destroy TextLayer
	text_layer_destroy(g_layerTwoOfTwo);
	text_layer_destroy(g_layerOneOfTwo);
}


static void init() {
	// Create main Window element and assign to pointer
	s_wndMain = window_create();

	// Set handlers to manage the elements inside the Window
	window_set_window_handlers(s_wndMain, (WindowHandlers) {
		.load = main_window_load,
			.unload = main_window_unload
	});

	// Show the Window on the watch, with animated=true
	window_stack_push(s_wndMain, true);

	// Make sure the time is displayed from the start
	//  time_t temp = time(NULL); 
	//  struct tm *tick_time = localtime(&temp);
	//  char s_szTime[32];
	//  getTimeString(s_szTime, sizeof(s_szTime), tick_time);
	//	UpdateDisplay();

	// Register with TickTimerService
	tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
	// Register for battery level updates
	battery_state_service_subscribe(battery_callback);
	// Register for Bluetooth connection updates
	connection_service_subscribe((ConnectionHandlers)
	{
		.pebble_app_connection_handler = bluetooth_callback
	});

}

static void deinit() {
	// Destroy Window
	window_destroy(s_wndMain);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}
