// CONSOLE
#include <stdp>
#include <stdr>
#include <conint>
#include <filep>

#include <curl.h>
#include <ShlObj.h>
#include <propvarutil.h>
#include <propkey.h>

#pragma comment( lib, "propsys" )
#pragma comment( lib, "shell32" )
#pragma comment( lib, "ole32" )

// Only 2342 of my 6737 screenshots has comments 34.8%
// Steam max screenshot description length: 140 symbols

#define INIT_SCR_SIZE 8388608

ui64 total_scrs;
ui64 skipped_scrs;
ui64 max_page;
ui64 counter;
ui64 spd_counter;
ui64 total_time;

HANDLE pause_event, oh, ih;

ui64 NOTHROW PARAMNOUSE writeFuncTxt(char *ptr, ui64 size, ui64 nmemb, void *userdata)
{ // ptr points to the delivered data, and the size it is nmemb; size is always 1; not \0 term!
	txtadd(*((txt *)userdata), ptr, nmemb);
	return nmemb;
}

ui64 NOTHROW PARAMNOUSE writeFuncImg(char *ptr, ui64 size, ui64 nmemb, void *userdata)
{
	((binf *)userdata)->Add(ptr, nmemb);
	return nmemb;
}

txt formatDateTime(const txt &dt, SYSTEMTIME *md)
{
	txt day, month;
	if(dt[0] >= '0' && dt[0] <= '9') // 21 Jan, 2013 @ 12:44pm
	{
		ui64 daye = txtf(dt, 0, ' ') - 1; // Day end position
		day = txtsp(dt, 0, daye);
		month = txts(dt, daye+2, 3);
	}
	else // Jan 21, 2013 @ 6:44am
	{
		ui64 mone = txtf(dt, 0, ' ') - 1; // Month end position
		month = txtsp(dt, 0, mone);
		day = txtsp(dt, mone+2, txtf(dt, mone, ',')-1);
	}
	
	if(~day == 1)
	{
		day = '0' + day;
	}
	
	md->wDay = (WORD)t2i(day);
	md->wDayOfWeek = 0;
	
	if(month == L("Jan"))
	{
		month = L("01");
	}
    else if(month == L("Feb"))
	{
		month = L("02");
	}
    else if(month == L("Mar"))
	{
		month = L("03");
	}
    else if(month == L("Apr"))
	{
		month = L("04");
	}
    else if(month == L("May"))
	{
		month = L("05");
	}
	else if(month == L("Jun"))
	{
		month = L("06");
	}
	else if(month == L("Jul"))
	{
		month = L("07");
	}
    else if(month == L("Aug"))
	{
		month = L("08");
	}
    else if(month == L("Sep"))
	{
		month = L("09");
	}
    else if(month == L("Oct"))
	{
		month = L("10");
	}
    else if(month == L("Nov"))
	{
		month = L("11");
	}
    else if(month == L("Dec"))
	{
		month = L("12");
	}
	
	md->wMonth = (WORD)t2i(month);
	
	txt year;
	ui64 comma = txtf(dt, 0, ',');
	if(comma != NFND)
	{
		year = txts(dt, comma+2, 4);
		md->wYear = (WORD)t2i(year);
	}
	else
	{
		SYSTEMTIME st;
		GetLocalTime(&st);
		year = i2t(st.wYear);
		md->wYear = st.wYear;
	}
	
	// 20 Jan, 2013 @ 6:29am
	// 2 Jul @ 11:38am
	// 5 Oct, 2022 @ 8:01pm
	
	txt hour, minute;
	ui64 dogp = txtf(dt, 0, '@'); // Doge position
	if(dt[dogp+3] == ':')
	{
		char a_p = dt[dogp+6]; // AM or PM?!
		hour = txt('0') + dt[dogp+2];
		if(a_p == 'p')
		{
			ui64 h = t2i(hour);
			hour = i2t(h + 12);
		}

		minute = txts(dt, dogp+4, 2);
	}
	else
	{
		char a_p = dt[dogp+7];
		hour = txts(dt, dogp+2, 2);
		ui64 h = t2i(hour);
		if(a_p == 'p')
		{
			if(h != 12)
			{
				hour = i2t(h + 12);
			}
		}
		else
		{
			if(h == 12)
			{
				hour = L("00");
			}
		}

		minute = txts(dt, dogp+5, 2);
	}
	
	md->wHour = (WORD)t2i(hour);
	md->wMinute = (WORD)t2i(minute);
	md->wSecond = 34;
	md->wMilliseconds = 0;
	
	return year + '_' + month + '_' + day + ' ' + hour + '_' + minute;
}

txt getTzOffset()
{
	TIME_ZONE_INFORMATION tzi;
	GetTimeZoneInformation(&tzi);
	
	// From steam web page [shared_global.js]: var tzOffset = now.getTimezoneOffset() * -1 * 60;
	return i2t(tzi.Bias * -1 * 60);
}

void cleanDescript(txt &dsp)
{
	for(ui64 i = 0; i < ~dsp; ++i)
	{
		if(dsp[i] == '&')
		{
			txt entity = txtsp(dsp, i, txtf(dsp, i, ';'));
			if(entity == L("&amp;") || entity == L("&#38;"))
			{
				txtr(dsp, i, ~entity, '&');
			}
			else if(entity == L("&apos;") || entity == L("&#39;"))
			{
				txtr(dsp, i, ~entity, '\'');
			}
			else if(entity == L("&quot;") || entity == L("&#34;"))
			{
				txtr(dsp, i, ~entity, '"');
			}
			else if(entity == L("&lt;") || entity == L("&#60;"))
			{
				txtr(dsp, i, ~entity, '<');
			}
			else if(entity == L("&gt;") || entity == L("&#62;"))
			{
				txtr(dsp, i, ~entity, '>');
			}
			else if(entity == L("&nbsp;") || entity == L("&#160;"))
			{
				txtr(dsp, i, ~entity, ' ');
			}
			else
			{
				txtd(dsp, i, ~entity);
				--i;
			}
		}
	}
}

void cleanName(txt &name)
{
	/*	
 		< (less than)
		> (greater than)
		: (colon - sometimes works, but is actually NTFS Alternate Data Streams)
		" (double quote)
		/ (forward slash)
		\ (backslash)
		| (vertical bar or pipe)
		? (question mark)
		* (asterisk)
	*/	
	
	for(ui64 i = 0; i < ~name; ++i)
	{
		switch(name[i])
		{
		case '<':
		case '>':
		case ':':
		case '"':
		case '/':
		case '\\':
		case '|':
		case '?':
		case '*':
		case '\n':
		case '\r':
		case '\t':
		case '\v':
		case '\f':
			txtd(name, i, 1);
			--i;
			break;
		case '&':
		{
			txt entity = txtsp(name, i, txtf(name, i, ';'));
			if(entity == L("&amp;") || entity == L("&#38;"))
			{
				txtr(name, i, ~entity, '&');
			}
			else if(entity == L("&apos;") || entity == L("&#39;"))
			{
				txtr(name, i, ~entity, '\'');
			}
			else
			{
				txtd(name, i, ~entity);
				--i;
			}
		}	break;
		default:
			break;
		}
	}
	
	// Remove trailing spaces and periods
	ui64 idx = ~name;
	while(name[--idx] == ' ' || name[idx] == '.');
	if(idx != ~name-1)
	{
		txtd(name, idx+1, ~name - idx - 1);
	}
}

void fixLink(txt &link)
{
	for(ui64 i = 0; i < ~link; ++i)
	{
		switch(link[i])
		{
		case '\n':
		case '\r':
		case ' ':
		case '\t':
		case '\v':
		case '\f':
			txtd(link, i, 1);
			--i;
			break;
		default:
			break;
		}
	}
}

ui32 past_pctg = UI32_MAX;
bool64 tot_scrs_set, max_page_set;
ui64 past_sec_need = UI64_MAX;

void updateGUI(ui64 cur_pn, const txt &cur_scr)
{
	COORD comp, xy;
	comp.X = 0;
	comp.Y = 0;
	
	ui64 len = 60;
	
	// BLUE       0x90 --> BACKGROUND_INTENSITY | BACKGROUND_BLUE
	// DARKBLUE   0x10 --> BACKGROUND_BLUE
	// DARKGRAY   0x80 --> BACKGROUND_INTENSITY
	WORD blue = 0x10, dgray = 0x80;
	DWORD attr_wr;
	
	float pctg = (float)counter/(float)total_scrs * (float)len;
	ui64 full = (ui64)pctg;
	ui64 empty = len - full;
	
	COORD empty_pos = comp;
	empty_pos.X += (SHORT)full;
	
	FillConsoleOutputAttribute(oh, blue, (DWORD)full, comp, &attr_wr);
	FillConsoleOutputAttribute(oh, dgray, (DWORD)empty, empty_pos, &attr_wr);
	
	ui32 int_pctg = (ui32)(pctg/60.0f * 100.0f);
	if(int_pctg != past_pctg)
	{
		xy = { 61 , 0 };
		SetConsoleCursorPosition(oh, xy);
		p|int_pctg|'%';
		past_pctg = int_pctg;
	}
	
	xy = { 0 , 1 };
	SetConsoleCursorPosition(oh, xy);
	p|SPN(30)|counter;
	
	if(!tot_scrs_set)
	{
		p|'/'|total_scrs;
		tot_scrs_set = true;
	}
	
	xy = { 0 , 2 };
	SetConsoleCursorPosition(oh, xy);
	p|SPN(30)|cur_pn;
	
	if(!max_page_set)
	{
		p|'/'|max_page;
		max_page_set = true;
	}
	
	xy = { 0 , 3 };
	SetConsoleCursorPosition(oh, xy);
	
	// My STD printer lib does not support floats yet, (-_-) yes.. yes...
	float avg_spd = 1000.0f/((float)total_time/(float)spd_counter);
	p|CLL|L("Avg. Speed: ")|(ui64)avg_spd|'.';
	p|SPC('0')|SP(2)|(ui64)(avg_spd * 100.0f - float((ui64)avg_spd) * 100.0f)|DP|L(" scr/s");
	
	xy = { 0 , 4 };
	SetConsoleCursorPosition(oh, xy);
	
	ui64 sec_need = ui64(float(total_scrs - counter)/avg_spd);
	if(sec_need != past_sec_need)
	{
		p|CLL;
		if(sec_need < 60)
		{
			p|L(" Time Left: ")|sec_need|L(" sec");
		}
		else if(sec_need < 3600)
		{
			p|L(" Time Left: ")|sec_need/60|L(" min ")|sec_need % 60|L(" sec");
		}
		else
		{
			p|L(" Time Left: ")|sec_need/3600|L(" hrs ")|sec_need % 3600/60|L(" min ")|sec_need % 3600 % 60|L(" sec");
		}
	}
	
	xy = { 0 , 5 };
	SetConsoleCursorPosition(oh, xy);
	
	p|CLL|L("Just saved: ")|cur_scr;
	
	xy = { 0 , 7 };
	SetConsoleCursorPosition(oh, xy);
}

void resetErrorOutput()
{
	//////////////////////////////////////////////
	WaitForSingleObject(pause_event, INFINITE); // PAUSE BREAK
	//////////////////////////////////////////////
	COORD xy = { 0 , 7 };
	SetConsoleCursorPosition(oh, xy);
	p|S(632);
	SetConsoleCursorPosition(oh, xy);
}

void clearErrorOutput()
{
	//////////////////////////////////////////////
	WaitForSingleObject(pause_event, INFINITE); // PAUSE BREAK
	//////////////////////////////////////////////
	p|CLS;
	COORD xy = { 0 , 7 };
	SetConsoleCursorPosition(oh, xy);
}

bool pause_on;

#pragma warning( suppress : 4100 )
DWORD __declspec(nothrow) inputThread(LPVOID lp)
{
	COORD xy;
	xy.X = 36;
	xy.Y = 21;
	
	CONSOLE_SCREEN_BUFFER_INFO con_inf;
	GetConsoleScreenBufferInfo(oh, &con_inf);
	
	INPUT_RECORD ir[128];
	DWORD nread;
	while(1)
	{
		ReadConsoleInput(ih, ir, 128, &nread);
		for(DWORD i = 0; i < nread; ++i)
		{
			if(ir[i].EventType == KEY_EVENT)
			{
				const KEY_EVENT_RECORD* k = &ir[i].Event.KeyEvent;
				if(k->wVirtualKeyCode == VK_SPACE && !k->bKeyDown)
				{
					if(WaitForSingleObject(pause_event, 1) == WAIT_OBJECT_0) // Event is signalled
					{
						ResetEvent(pause_event);
						Sleep(2000);
						SetConsoleCursorPosition(oh, xy);
						p|B|"PAUSED";
						SetConsoleCursorPosition(oh, con_inf.dwCursorPosition);
					}
					else
					{
						SetConsoleCursorPosition(oh, xy);
						p|"      ";
						SetConsoleCursorPosition(oh, con_inf.dwCursorPosition);
						SetEvent(pause_event);
					}
				}
			}
		}
	}
}

void pause_thread()
{
	ResetEvent(pause_event);
	WaitForSingleObject(pause_event, INFINITE);
}

HRESULT writeComment(wtxt jpg, wtxt comment)
{
	HRESULT res;
	IPropertyStore* ps; // Func below does not support \\?\ long path (-_-) therefore, skip it
	res = SHGetPropertyStoreFromParsingName((const wchar_t *)jpg+4, nullptr, GPS_READWRITE, IID_PPV_ARGS(&ps));
	
	if(SUCCEEDED(res) && ps)
	{
		PROPVARIANT prop_comment;
		PropVariantInit(&prop_comment);
		
		res = InitPropVariantFromString(comment, &prop_comment);
		if(SUCCEEDED(res))
		{
			res = ps->SetValue(PKEY_Comment, prop_comment);
			if(SUCCEEDED(res))
			{
				res = ps->Commit();
			}
		}
		
		PropVariantClear(&prop_comment);
		ps->Release();
	}
	
	return res;
}

ui64 txtfiw(const txt &t, ui64 pos, const txt &fnd) // Ignore White Spaces
{
	
	if(~fnd > ~t)
	{
		return NFND;
	}
	
	if(pos > ~t - ~fnd)
	{
		return NFND;
	}
	
	const char *fb = (const char *)fnd-1, *fe = (const char *)fnd + ~fnd;
	const char *tb = (const char *)t + pos - 1, *te = (const char *)t + ~t - ~fnd;
	const char *tb_cur = NULL;
	
	while(tb != te)
	{
		tb_cur = tb;
		while(++fb != fe)
		{
		skip_white_space:
			++tb_cur;
			if(*tb_cur == 0x20 || *tb_cur == 0xA ||
				*tb_cur == 0xD || *tb_cur == 0x9 || *tb_cur == 0xC)
			{
				goto skip_white_space;
			}
			
			
			if(*fb != *tb_cur)
			{
				fb = (const char *)fnd-1;
				break;
			}				
		}
		
		++tb;
		if(fb == fe)
		{
			return ui64(tb - (const char *)t);
		}
	}
	
	return NFND;
}

ui64 txtffn(const txt &t, ui64 pos) // Find first number
{
	if(pos >= ~t)
	{
		return NFND;
	}
	
	const char *tb = (const char *)t + pos - 1, *te = (const char *)t + ~t;
	
	while(++tb != te)
	{	
		if(*tb >= 0x30 && *tb <= 0x39)
		{
			return ui64(tb - (const char *)t);
		}
	}
	
	return NFND;
}

BOOL NOTHROW conBreakHandler(DWORD ctrl_sig)
{
	if(ctrl_sig != CTRL_C_EVENT && ctrl_sig != CTRL_BREAK_EVENT)
	{
		return FALSE;
	}
	
	p|CLS|RCON;
	return FALSE;
}

bool64 console_initialised;

void conInit()
{
	if(console_initialised)
	{
		return;
	}
	
	p|SCBW(79,22)|SDCA(BLACK_BG_WHITE_FG)|CLS;
	SetConsoleTitle(L"Steam Screenshot Downloader");
	SetConsoleCtrlHandler(conBreakHandler, TRUE);
	console_initialised = true;
}

cwstr usage = WL(
	"Usage: steamscrd [user_id] [options]\n"
	"Options:\n"
	"[-app=XXXX] --> game ID to download screenshots from\n"
	"[-spage=XX] --> number of page to start download from\n\n"
	"[-sls=XXX...XXX] --> steamLoginSecure Cookie\n\n"
	"[-sid=XXX...XXX] --> sessionid Cookie\n\n"
	"For more info and to report problems visit:\n"
	"https://github.com/ScienceDiscoverer/steamscrd");

inline void argError(cwstr arg)
{
	p|"Error in argument "|R|arg|" detected!"|N;
	p|usage|N|"Press any key to exit..."|P;
}

int wmain(ui64 argc, wchar_t **argv)
{
	oh = GetStdHandle(STD_OUTPUT_HANDLE);
	ih = GetStdHandle(STD_INPUT_HANDLE);
	
	txt user_id = 127;		// First argument
	txt game_id = 127;		// -app=4269
	ui64 start_page = 1;	// -spage=34
	txt sls_cookie = 1023;	// -sls=773772734...aSAdfdD
	txt sid_cookie = 127;	// -sid=773...dDsA
	
	cwstr app_arg = WL("-app=");
	cwstr page_arg = WL("-spage=");
	cwstr sls_arg = WL("-sls=");
	cwstr sid_arg = WL("-sid=");
	cwstr unknown_arg = WL("UNKNOWN ARGUMENT");
	
	if(argc > 1)
	{
		user_id = wt2u8(cwstr({ argv[1], strl(argv[1]) }));
		
		wtxt arg = 63;
		for(ui64 i = 2; i < argc; ++i)
		{
			arg = { argv[i], strl(argv[i]) };
			if(txtseq(arg, 0, app_arg))
			{
				if(~arg == app_arg.s) // Empty argument
				{
					argError(app_arg);
					return 1;
				}
				
				game_id = wt2u8(txts(arg, app_arg.s, TEND));
				
				if(txtnui(game_id)) // ID is not an unsigned integer
				{
					argError(app_arg);
					return 1;
				}
			}
			else if(txtseq(arg, 0, page_arg))
			{
				if(~arg == page_arg.s) // Empty argument
				{
					argError(page_arg);
					return 1;
				}
				
				wtxt pn = txts(arg, page_arg.s, TEND);
				
				if(txtnui(pn)) // Page number is not an unsigned integer
				{
					argError(page_arg);
					return 1;
				}
				
				start_page = t2i(pn);
			}
			else if(txtseq(arg, 0, sls_arg))
			{
				if(~arg == sls_arg.s) // Empty argument
				{
					argError(sls_arg);
					return 1;
				}
				
				sls_cookie = wt2u8(txts(arg, sls_arg.s, TEND));;
			}
			else if(txtseq(arg, 0, sid_arg))
			{
				if(~arg == sid_arg.s) // Empty argument
				{
					argError(sid_arg);
					return 1;
				}
				
				sid_cookie = wt2u8(txts(arg, sid_arg.s, TEND));
			}
			else
			{
				argError(unknown_arg);
				return 1;
			}
		}
	}
	else // Manual Input
	{
		conInit();
		p|CLS;
		
		p|SCP(0,2)|"Enter steam user ID to start download."|N;
		p|"Additional space-separated options:"|N;
		p|"[app="|G|"XXXX"|"] --> game "|G|"ID"|" to download screenshots from"|N;
		p|"[spage="|G|"XX"|"] --> "|G|"NUMBER"|" of page to start download from"|N|N;
		p|"For more info and to report problems visit:"|N;
		p|B|"https://github.com/ScienceDiscoverer/steamscrd"|SCP(0,0);
		
		txt input = 127;
	retry_input:
		r > input;
		
		ui64 fsp = txtf(input, 0, ' '); // First space
		if(fsp != NFND) // Additional arguments detected
		{
			ui64 app_st = txtf(input, fsp, L("app="));
			ui64 page_st = txtf(input, fsp, L("spage="));
			
			if(app_st == NFND && page_st == NFND)
			{
				p|SCP(0,1)|CLL|R|"No valid arguments found!"|" Retry."|SCP(0,0)|CLL;
				goto retry_input;
			}
			
			if(app_st != NFND)
			{
				app_st += L("app=").s;
				if(app_st >= ~input) // Empty argument
				{
					p|SCP(0,1)|CLL|R|"Error in [app=] argument!"|" Retry."|SCP(0,0)|CLL;
					goto retry_input;
				}
				
				ui64 app_ed = txtf(input, app_st, ' ');
				app_ed = app_ed == NFND ? TEND : app_ed - 1;
				txtsp(game_id, input, app_st, app_ed);
				
				if(txtnui(game_id)) // ID is not an unsigned integer
				{
					p|SCP(0,1)|CLL|R|"Error in [app=] argument!"|" Retry."|SCP(0,0)|CLL;
					goto retry_input;
				}
			}
			if(page_st != NFND)
			{
				page_st += L("spage=").s;
				if(page_st >= ~input) // Empty argument
				{
					p|SCP(0,1)|CLL|R|"Error in [spage=] argument!"|" Retry."|SCP(0,0)|CLL;
					goto retry_input;
				}
				
				ui64 page_ed = txtf(input, page_st, ' ');
				page_ed = page_ed == NFND ? TEND : page_ed - 1;
				txt pn = txtsp(input, page_st, page_ed);
				
				if(txtnui(pn)) // ID is not an unsigned integer
				{
					p|SCP(0,1)|CLL|R|"Error in [spage=] argument!"|" Retry."|SCP(0,0)|CLL;
					goto retry_input;
				}
				
				start_page = t2i(pn);
			}
		}
		
		ui64 user_ed = txtf(input, 0, ' ');
		user_ed = user_ed == NFND ? TEND : user_ed - 1;
		txtsp(user_id, input, 0, user_ed);
	}
	
	conInit();
	SetConsoleMode(ih, ENABLE_EXTENDED_FLAGS | ENABLE_QUICK_EDIT_MODE | ENABLE_PROCESSED_INPUT);
	
	// Initialise COM library
	
	CoInitialize(NULL);
	
	p|DC|CLS;
	
	pause_event = CreateEventA(NULL, TRUE, TRUE, NULL);
	CreateThread(NULL, 0, inputThread, NULL, 0, NULL);
	
	// Initialise https library
	
	CURL *curl;
	CURLcode res;

	curl_global_init(CURL_GLOBAL_ALL);

	curl = curl_easy_init();

	if(!curl)
	{
		p|"Failed to initialise CURL! Press "|B|"SPACE"|" to exit..."|N;
		pause_thread();
		curl_global_cleanup();
		return 1;
	}
	
	clearErrorOutput();
	
	// https://steamcommunity.com/id/sciencediscoverer/screenshots/?sort=oldestfirst&browserfilter=myfiles&view=grid&privacy=14&l=english&p=
	// https://steamcommunity.com/id/sciencediscoverer/screenshots/?appid=404410&sort=oldestfirst&browserfilter=myfiles&view=grid&privacy=14&l=english&p=
	
	txt burl = L("https://steamcommunity.com/id/");
	burl += user_id, burl += L("/screenshots/?");
	if(game_id != empty)
	{
		burl += L("appid="), burl += game_id, burl += '&';
	}
	burl += L("sort=oldestfirst&browserfilter=myfiles&view=grid&privacy=14&l=english&p=");
	
	// Get total amount of screenshots and total number of grid pages
	bool64 profile_url_tried = false;
	txt grid_page = 262143, scr_page = 262143, full_page_link = 1024;
retry_url:
	txt fpage_url = burl + '1';
	curl_easy_setopt(curl, CURLOPT_URL,	(const char *)fpage_url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFuncTxt);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &grid_page);
	
retry_first_grid_load:
	res = curl_easy_perform(curl);
	if(res != CURLE_OK)
	{
		p|RS|"ERR"|res|DS|"; Failed to load first grid page! Retrying in 2 sec...";
		txtclr(grid_page);
		Sleep(2000);
		clearErrorOutput();
		goto retry_first_grid_load;
	}
	
	//<div id="image_wall">
	//...
	//<div style="float:left; padding-bottom: 5px;">Showing 1 - 50 of 6737</div>
	ui64 imgwp = txtf(grid_page, 0, L("id=\"image_wall\"")); // image_wall position
	ui64 showp = txtf(grid_page, imgwp, L("Showing")); // Showing position
	
	if(showp == NFND)
	{
		if(txtf(grid_page, 0, L("The specified profile could not be found")) != NFND)
		{
			if(!profile_url_tried)
			{
				p|SCP(0,0)|R|"Failed to locate user with ID "|RD|user_id|R|"! Trying profile ID URL...";
				txtr(burl, 27, 2, L("profiles")); // Try to get through by using raw 64 bit profile ID
				profile_url_tried = true;
				txtclr(grid_page);
				goto retry_url;
			}
			
			p|R|"Failed to locate user profile!";
			p|N|"Press "|B|"SPACE"|" to exit...";
			pause_thread();
			p|CLS|RCON;
			return 1;
		}
		
		p|R|"Failed to load any screenshots on the first grid page!"|" Retrying in 2 seconds...";
		txtclr(grid_page);
		Sleep(2000);
		clearErrorOutput();
		goto retry_first_grid_load;
	}
	
	if(profile_url_tried)
	{
		p|SCP(0,0)|CLL;
	}
	
	ui64 totap = txtf(grid_page, showp+7, L("of")) + 3; // Total amount position
	ui64 totep = txtf(grid_page, totap, '<') - 1; // Total amount end position
	total_scrs = t2i(txtsp(grid_page, totap, totep));
	
	//<a class=L("pagingPageLink") href=L("?p=135& ... &privacy=30")>135</a> - multiple of them, this one always last
	ui64 lpplp = txtf(grid_page, totep, L("pagingPageLink")) + 14; // Last pagingPageLink position
	ui64 cpplp = lpplp; // Current pagingPageLink position
	while(cpplp != NFND)
	{
		lpplp = cpplp + 14;
		cpplp = txtf(grid_page, lpplp, L("pagingPageLink"));
	}
	
	ui64 mpbp = txtf(grid_page, lpplp, '>') + 1; // Max page beginning position
	ui64 mpep = txtf(grid_page, mpbp, '<') - 1; // Max page end position
	
	txt max_page_txt = txtsp(grid_page, mpbp, mpep);
	if(max_page_txt == empty || txtnui(max_page_txt)) // No max page present, means that screenshots fit into one page
	{
		max_page = 1;
	}
	else
	{
		max_page = t2i(max_page_txt);
	}
	
	// From [shared_global.js]: document.cookie = "timezoneOffset=" + tzOffset + "," + isDST + ";
	txt tz_offset = L("timezoneOffset=") + getTzOffset() + L(",0;");
	
	// Set up screenshots folder
	wtxt path = MAX_PATH;
	txtssz(path, GetModuleFileNameW(NULL, path, MAX_PATH));
	
	// \\?\" prefix is supposed to make a maximum total path length of 32,767 characters possible...
	path = WL("\\\\?\\") + txtsp(path, 0, txtfe(path, ~path-1, '\\'));
	path += t2u16(user_id), path += '\\';
	
	CreateDirectoryW(path, NULL);
	
	clearErrorOutput(); // Move error output to the middle of the screen
	
	// Get individual screenshot pages links from the grid
	
	HANDLE f;
	ui64 t_beg, t_end; // Timers for measuring screenshot saving speed
	binf scrshot = INIT_SCR_SIZE;
	ui64 tot_down = 0; // Total amount of screenshots actually downloaded
	bool64 skip_adult_only = false;
	
	// Set up base cookies including time zone cookie to get accurate local screenshot date
	txt cookies = tz_offset;
	if(sls_cookie != empty && sid_cookie != empty)
	{
		cookies += L(" steamLoginSecure=") + sls_cookie + ';';
		cookies += L(" sessionid=") + sid_cookie + ';';
	}
	curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "");
	curl_easy_setopt(curl, CURLOPT_COOKIE, (const char *)cookies);
	
	for(ui64 pn = start_page; pn <= max_page; ++pn) // Current screenshot grid page number
	{
		txtclr(grid_page);
		ui64 spos = 0; // Current grid page html search position
		
		txt gurl = burl + i2t(pn);
		curl_easy_setopt(curl, CURLOPT_URL, (const char *)gurl);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFuncTxt);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &grid_page);
		
	retry_grid_load:
		res = curl_easy_perform(curl);
		if(res != CURLE_OK)
		{
			p|RS|"ERR"|res|DS|"; Failed to access screenshot grid page #"|pn|"! Retrying in 2 sec...";
			txtclr(grid_page);
			Sleep(2000);
			resetErrorOutput();
			goto retry_grid_load;
		}
		
		ui64 lnkp = txtfiw(grid_page, spos, L("?id=")); // Link position
		
		if(lnkp == NFND) // Grid page failed to load correctly
		{
			p|R|"Failed to load any screenshots on page #"|R|pn|"! Retrying in 2 seconds...";
			txtclr(grid_page);
			Sleep(2000);
			resetErrorOutput();
			goto retry_grid_load;
		}
		
		// Check if grid page is not broken (duplicates injected)
		
		//<div style="float:left; padding-bottom: 5px;">Showing 451 - 500 of 6737</div>
		imgwp = txtf(grid_page, 0, L("id=\"image_wall\"")); // image_wall position
		ui64 finbp = txtf(grid_page, imgwp, L("Showing")) + 8; // First grid image number beginning position
		ui64 finep = txtf(grid_page, finbp, '-') - 2; // First grid image number end position
		ui64 linep = txtf(grid_page, finep+4, ' ') - 1; // Last grid image number end position
		ui64 fg_img_n = t2i(txtsp(grid_page, finbp, finep)); // First grid image number (Showing [451] - 500 of 6737)
		ui64 lg_img_n = t2i(txtsp(grid_page, finep+4, linep));  // Last grid image number (Showing 451 - [500] of 6737)
		
		if(counter != fg_img_n - 1)
		{
			//p|"Current internal screenshot index ("|R|counter|") does not match steam grid page index ("|B|fg_img_n-1|")!"|N;
			//p|"Correcting in 2 sec";
			counter = fg_img_n - 1;
			//Sleep(2000);
			//resetErrorOutput();
		}

		ui64 links_found = 1;
		while(1)
		{
			lnkp = txtfiw(grid_page, lnkp+4, L("?id=")); // Link position
			
			if(lnkp == NFND)
			{
				break;
			}
			
			++links_found;
		}
		
		if(links_found != lg_img_n - fg_img_n + 1) // Duplicates found or some screenshots was not loaded
		{
			i64 dupez = (i64)links_found - i64(lg_img_n - fg_img_n + 1);
			p|RS|dupez|RDS|" duplicate";
			if(dupez > 1 || dupez < -1)
			{
				p|'s';
			}
			p|DS|" found on page #"|R|pn|"! Reloading in 2 seconds...";
			
			txtclr(grid_page);
			Sleep(2000);
			resetErrorOutput();
			goto retry_grid_load;
		}
		
		while(1)
		{
			//////////////////////////////////////////////
			WaitForSingleObject(pause_event, INFINITE); // PAUSE BREAK
			//////////////////////////////////////////////
			t_beg = GetTickCount64();
			
			lnkp = txtfiw(grid_page, spos, L("?id=")); // Link position
			
			if(lnkp == NFND)
			{
				break;
			}
			
			//<a href="https://steamcommunity.com/sharedfiles/filedetails/?id=692881692"
			ui64 lnp = txtffn(grid_page, lnkp+4); // Link position
			ui64 lep = txtf(grid_page, lnp, '"'); // Link end position
			txt link = txtsp(grid_page, lnp, lep-1);
			
			spos = lep;
			
			// Extract description from grid tile to avoid profanity filter in full screenshot page
			
			/* Description is present:
			<div class="imgWallHoverBottom">
				<div class="imgWallHoverDescription ">
					<q class="ellipsis">baaaakaaaaaaaa &gt;______&lt;</q>
				</div>
			</div>
			
			No description is present:
			<div class="imgWallHoverBottom"></div> */
			
			ui64 whbbp = txtf(grid_page, lep, L("imgWallHoverBottom")) + 18; // Wall Hover Bottom beginning position
			ui64 whbep = txtf(grid_page, whbbp, L("</div>")); // Wall Hover Bottom end position
			txt descript_div = txtsp(grid_page, whbbp, whbep);
			
			//<q class="ellipsis">OH NO!!! HUMAN FLESH EATING HARES!! WE AREDOOMED1!!</q>
			ui64 sdp = txtf(descript_div, 0, L("ellipsis")); // Screenshot description position
			
			txt descript;
			if(sdp != NFND)
			{
				ui64 dspp = txtf(descript_div, sdp+8, '>') + 1; // Description position
				ui64 dsep = txtf(descript_div, dspp, '<') - 1; // Description end position
				descript = txtsp(descript_div, dspp, dsep);
				cleanDescript(descript);
			}
			
			// Visit screenshot individual page to get some info and direct link
			
		retry_scr_page_url:
			curl_easy_setopt(curl, CURLOPT_COOKIE, (const char *)cookies);
			txtclr(scr_page);
			
			full_page_link = L("https://steamcommunity.com/sharedfiles/filedetails/?id=") + link;
			
			curl_easy_setopt(curl, CURLOPT_URL, (const char *)full_page_link);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFuncTxt);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &scr_page);
			
			bool64 kcookie_tried = false; // Kinky Cookie strategy tried
			bool64 lcookie_tried = false; // Login Cookies strategy tried
			
		retry_scr_page:
			res = curl_easy_perform(curl);
			if(res != CURLE_OK)
			{
				p|'['|RS|'E'|res|DS|"] ";
				
				if(res == CURLE_URL_MALFORMAT) // The URL was not properly formatted. 
				{
					p|"URL MALFORMAT: ["|link|"]. Trying to fix in 2 sec...";
					fixLink(link);
					txtclr(scr_page);
					Sleep(2000);
					resetErrorOutput();
					goto retry_scr_page_url;
				}
				else
				{
					p|"Failed to access screenshot page #"|counter+1|"! Retrying in 2 sec...";
					txtclr(scr_page);
					Sleep(2000);
					resetErrorOutput();
					goto retry_scr_page;
				}
			}
			
			ui64 flp = txtf(scr_page, 0, L("https://steamuserimages-a.akamaihd.net")); // Full link position
			
			if(flp == NFND)
			{
				if(kcookie_tried)
				{
					if(skip_adult_only)
					{
						++skipped_scrs;
						resetErrorOutput();
						t_end = GetTickCount64();
						total_time += t_end - t_beg;
						updateGUI(pn, L("SKIPPED"));
						continue;
					}
					
					if(lcookie_tried)
					{
						p|"Failed to fetch "|R|"Adult Only"|" screenshot!"|N;
						p|"Type "|B|"retry"|", "|Y|"skip"|" or "|R|"skipall"|":"|N|RM|EC;
						txt choice = 31;
						r > choice;
						resetErrorOutput();
						SetConsoleMode(ih, ENABLE_EXTENDED_FLAGS | ENABLE_QUICK_EDIT_MODE | ENABLE_PROCESSED_INPUT);
						p|DC;
						
						if(choice == L("retry"))
						{
							++skipped_scrs;
							goto retry_scr_page;
						}
						else if(choice == L("skip"))
						{
							++skipped_scrs;
							t_end = GetTickCount64();
							total_time += t_end - t_beg;
							updateGUI(pn, L("SKIPPED"));
							continue;
						}
						else if(choice == L("skipall"))
						{
							++skipped_scrs;
							skip_adult_only = true;
							t_end = GetTickCount64();
							total_time += t_end - t_beg;
							updateGUI(pn, L("SKIPPED"));
							continue;
						}
					}
					
					if(sls_cookie == empty || sid_cookie == empty)
					{
						p|"Screenshot from "|R|"Adult Only"|" game detected!"|N;
						p|"Input "|M|"steamLoginSecure"|" cookie below, or press "|C|"ENTER"|" to skip:"|N|RM|EC;
						r > sls_cookie;
						resetErrorOutput();
						p|"Input "|M|"sessionid"|" cookie below:\n";
						r > sid_cookie;
						SetConsoleMode(ih, ENABLE_EXTENDED_FLAGS | ENABLE_QUICK_EDIT_MODE | ENABLE_PROCESSED_INPUT);
						p|DC;
						resetErrorOutput();
						
						cookies += L(" steamLoginSecure=") + sls_cookie + ';';
						cookies += L(" sessionid=") + sid_cookie + ';';
					}
					
					p|"Found "|R|"Adult Only"|" screenshot! Feeding "|M|"The Login Cookies"|" to server in 2 sec...";
					
					curl_easy_setopt(curl, CURLOPT_COOKIE, (const char *)cookies);
					txtclr(scr_page);
					Sleep(2000);
					resetErrorOutput();
					lcookie_tried = true;
					goto retry_scr_page;
				}
				else
				{
					p|"Found some "|R|"mature"|" content! Feeding "|M|"The Kinky Cookie"|" to server in 2 sec...";
					txt allow_kinky_cookie = L(" wants_mature_content_item_") + link + L("=1;");
					txt tmp_cookie = cookies + allow_kinky_cookie;
					curl_easy_setopt(curl, CURLOPT_COOKIE, (const char *)tmp_cookie);
					txtclr(scr_page);
					Sleep(2000);
					resetErrorOutput();
					kcookie_tried = true;
					goto retry_scr_page;
				}
			}
			
			ui64 fep = txtf(scr_page, flp, L("/?")); // Full link end position
			txt full_image_link = txtsp(scr_page, flp, fep);
			
			//<a href=L("https://steamcommunity.com/app/221260/screenshots/")>Little Inferno</a>
			//<div class="screenshotAppName">AM2R</div> ---> Removed from Steam Store
			ui64 anp = txtf(scr_page, fep, L("screenshotAppName")); // screenshotAppName position
			
			txt scr_app_name = 511;
			txt app_id = L("UNKNOWN");
			txt name = L("_UNKNOWN_");
			
			if(anp != NFND)
			{
				ui64 ane = txtf(scr_page, anp, L("</div>")); // screenshotAppName end position
				scr_app_name = txtsp(scr_page, anp, ane);
				
				ui64 idp = txtf(scr_app_name, 0, L("/app/")); // Steam Application ID position
				if(idp != NFND)
				{
					idp += 5;
					ui64 iep = txtf(scr_app_name, idp, '/') - 1; // Steam Application ID end position
					app_id = txtsp(scr_app_name, idp, iep);
					ui64 nmp = txtf(scr_app_name, iep, '>') + 1; // Name position
					ui64 nep = txtf(scr_app_name, nmp, '<') - 1; // Name end position
					name = txtsp(scr_app_name, nmp, nep);
				}
				else
				{
					ui64 nmp = txtf(scr_app_name, 0, '>') + 1; // Name position
					ui64 nep = txtf(scr_app_name, nmp, '<') - 1; // Name end position
					name = txtsp(scr_app_name, nmp, nep);
				}
				
				cleanName(name);
			}
			
			//<div class="detailsStatRight">22 Jan, 2013 @ 3:58am</div>
			ui64 dsp0 = txtf(scr_page, fep, L("detailsStatRight")) + 16; // Skip first detailsStatRight
			ui64 dsp = txtf(scr_page, dsp0, L("detailsStatRight")) + 16; // Skip second detailsStatRight
			ui64 dtp = txtf(scr_page, dsp, '>') + 1; // Datetime position
			ui64 dep = txtf(scr_page, dtp, '<') - 1; // Datetime end position
			txt datetime = txtsp(scr_page, dtp, dep);
			
			// Access and download full size screenshot file
			
			curl_easy_setopt(curl, CURLOPT_URL, (const char *)full_image_link);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFuncImg);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &scrshot);
			
		retry_raw_scr_page:
			res = curl_easy_perform(curl);
			if(res != CURLE_OK)
			{
				p|RS|"ERR"|res|DS|"; Failed to access raw screenshot page #"|counter+1|"! Retrying in 2 sec...";
				scrshot.Clear();
				Sleep(2000);
				resetErrorOutput();
				goto retry_raw_scr_page;
			}

			wtxt wname = t2u16(name);
			wtxt npath = path + wname + '\\'; // Named path
			CreateDirectoryW(npath, NULL);
			
			SYSTEMTIME mod_date;
			wtxt fname = npath + wname + ' ' + t2u16(app_id) + ' ' + t2u16(formatDateTime(datetime, &mod_date)) + ' ' + i2wt(counter++) + WL(".jpg");
			++spd_counter;
			
			filep fp = fname;
			if(!fp)
			{
				p|PE|" In CreateFileW line "|__LINE__|": fname = "|I|fname|I|N|"Press "|B|"SPACE"|" to skip this file..."|N;
				pause_thread();
				resetErrorOutput();
				goto skip_file_write;
			}
			fp|scrshot|FC;
			scrshot.Clear();
			++tot_down;
			
			// Save screenshot description in the XP Comment JPG metadata field
			
			if(sdp != NFND)
			{
				if(writeComment(fname, t2u16(descript)) != S_OK) // TOFIX THIS CREATES \R\N NEWLINES IN THE MIDDLE OF COMMENTS IN RARE CASES
				{
					p|"Failed to write comment into "|B|fname|N;
					p|"Press "|B|"SPACE"|" to continue..."|N;
					pause_thread();
					resetErrorOutput();
				}
			}
			
			// Update file's modify date to match upload date
			
			SYSTEMTIME utc_mod_date;
			TzSpecificLocalTimeToSystemTime(NULL, &mod_date, &utc_mod_date);
			FILETIME mod_ftime;
			SystemTimeToFileTime(&utc_mod_date, &mod_ftime);
			
			// Create or open File or Device =================================================================
			f = CreateFile(
				fname,						//   [I]  Name of the file or device to create/open
				FILE_WRITE_ATTRIBUTES,		//   [I]  Requested access GENERIC_READ|GENERIC_WRITE|0
				NULL,						//   [I]  Sharing mode FILE_SHARE_READ|WRITE|DELETE|0
				NULL,						// [I|O]  SECURITY_ATTRIBUTES for file, handle inheritability
				OPEN_EXISTING,				//   [I]  Action to take if file/device exist or not
				NULL,						//   [I]  Attributes and flags for file/device
				NULL);						// [I|O]  Handle to template file to copy attributes from
			// ===============================================================================================
			
			SetFileTime(f, &mod_ftime, NULL, &mod_ftime); // Access time will be download date
			CloseHandle(f); // Always check the handle!
			
		skip_file_write:
			
			t_end = GetTickCount64();
			total_time += t_end - t_beg;
			
			updateGUI(pn, name + ' ' + datetime);
		}
	}
	
	curl_easy_cleanup(curl);
	curl_global_cleanup();
	
	CoUninitialize();
	
	p|SCP(12,4)|CLL|G|"Finished"; // Cover up some "Time Left" rounding errors (1 second left after finishing)
	
	resetErrorOutput();
	
	if(start_page > max_page)
	{
		p|R|"Start page "|RD|start_page|R|" is bigger than user's max page "|RD|max_page|R|'!';
	}
	else if(tot_down == total_scrs)
	{
		p|GD|"All "|G|total_scrs|GD|" screenshots where downloaded successfully!";
	}
	else
	{
		p|G|tot_down|GD|" from "|G|total_scrs|GD|" screenshots where downloaded successfully!";
	}
	
	if(skipped_scrs != 0)
	{
		p|N|Y|skipped_scrs|" screenshots where "|R|"SKIPPED"|" by user...";
	}
	
	p|N|"Press "|B|"SPACE"|" to exit...";
	pause_thread();
	p|RCON|CLS;
	return 0;
}