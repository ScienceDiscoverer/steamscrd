# Steam Screenshot Downloader

[![GitHub release (with filter)](https://img.shields.io/github/v/release/ScienceDiscoverer/steamscrd)](../../releases/latest)
&nbsp;&nbsp;&nbsp;![GitHub all releases](https://img.shields.io/github/downloads/ScienceDiscoverer/steamscrd/total)
&nbsp;&nbsp;&nbsp;[![Sponsor Badge](https://img.shields.io/badge/%E2%98%95_Sponsor-blue)](https://www.buymeacoffee.com/sciencediscoverer)

Download all the screenshots from a public Steam profile. No sign-in required. Screenshot description and upload date are embedded directly into the `JPG` file, saved into separate folder for each game. Profanity filter and restricted content is not a problem for this tool.

![Program GIF demo](readme/steamscrd.gif)

## How to use

`steamscrd` supports both command line and direct user input interfaces.

### Command line

```
Usage: steamscrd [user_id] [options]
	Options:
	[-app=XXXX] --> game ID to download screenshots from
	[-spage=XX] --> number of page to start download from
	[-sls=XXX...XXX] --> steamLoginSecure Cookie
	[-sid=XXX...XXX] --> sessionid Cookie
```

### Direct input

Launching `steamscrd` without any command line argument activates direct input interface. Just follow the instructions and type in or copy-paste your `ID` and any optional parameters, as needed. Then press `ENTER`.

## How to get `user ID` / `app ID`

`user ID` is a Steam user's unique custom identifier. You can find it by visiting the profile page:

```
https://steamcommunity.com/id/[>>>sciencediscoverer<<<]
```
If said user never tried to set up a unique Steam profile ID in the options, his/her profile address will look like this:

```
https://steamcommunity.com/profiles/[>>>765611980XXXXXXXX<<<]
```
You can use this number instead.

`app ID` is a Steam game's unique identifier. You can get it from the game's store page:

```
https://store.steampowered.com/app/[>>>292030<<<]/_3/
```

## Screenshots from "Adult Only" games

Due to specifics of Steam's age restriction implementation it is not possible to download all restricted screenshots without being logged in. Only screenshots from games that are not marked as "Adult Only" can be downloaded this way.

In order to also download screenshots from "Adult Only" games you will need to extract some cookies from your browser of choice. This can be done in 2 ways. Either by using command line arguments `-sls` and `-sid` or by inputting the cookies directly, when the program detects failure of the no-login method. This cookies are stored only in RAM while the program runs and discarded after you close it.

Also be sure to ensure that your `Adult Only Sexual Content` options are ticked in the Steam's `Store preferences`.

Here is instruction of how to get this cookies from Firefox:

![Firefox Instruction](readme/ff_cookies.png)

Login to Steam -> Go to any `steamcommunity.com` page -> Open developer tools `F12` -> `Storage` -> `Cookies` -> `steamcommunity.com`.

* Double-click and copy `steamLoginSecure` value and paste it into the program, press `Enter`.
* Repeat the same for `sessionid` cookie.

Here is instruction for Edge:

![Edge Instruction](readme/edge_cookies.png)

Login to Steam -> Go to any `steamcommunity.com` page -> Open developer tools `F12` -> `Application` -> `Cookies` -> `steamcommunity.com`.

Chrome should be pretty similar.

## Private and Friends Only screenshots

Just enter the `steamLoginSecure` and `sessionid` cookies as described above. This will allow you to additionally download "Friends Only" and "Private" (Hidden) screenshots from your library without the need to make them all public.

## Tips

* You can view screenshot comments by enabling `Comments` column in the Explorer. Switch to `View -> Details`, right-click on any column title and press `More...`, tick `Comments`, press `OK`. You can also use image viewr like [IrfanView](https://www.irfanview.com/main_download_engl.htm) to view comment while actually enjoying the image itself. Use [this instructions](https://irfanview-forum.de/forum/program/feature-requests/11035-) to enable the comment.
* You can pause download at any time by pressing `SPACE`. Unpause by pressing `SPACE` again.
* If you (for whatever reason) want to have all your screenshots in a single folder, instead of organised into separate folder for each game, you can use this `CMD` command to copy/move all images to a single folder: `for /r D:\<YOURID> %I in (*.jpg) do copy "%I" D:\ALL_SCREENS`.

## How to build

Just double-click the `msvc_build.cmd` file. Yep. As easy as this. Well, that is, if you have your Visual Studio installed, of course. And if your VS `vcvars64.bat` file is located at this path:

```
set "bat_dir=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
```

If not, first of all, make sure the [Visual Studio](https://visualstudio.microsoft.com/downloads/) is installed. In the setup wizard you will see a huge list of useless features and bloat. Only the C++ build tools are actually needed. Switch to `Individual components` and try to find this in the humongous list:

```
MSVC vXXX - VS 202X C++ x64/x86 build tools (Latest)
```

After this you will have to merely locate the `vcvars64.bat` file and copy-paste the path to it into `msvc_build.cmd`. Then just launch it and wait until it does all the job for you. Yes, you don't even need to install or open the Visual Studio IDE itself. Ever. I recommend you to try out my [Notepad++ C++ IDE](https://github.com/ScienceDiscoverer/npp_msvc_ide), if you like the idea of ditching the bloated, laggy and overengineered VS!

## Full video

[![Steam Screenshot Downloader Thumbnail](http://img.youtube.com/vi/uIicDGLsv4s/maxresdefault.jpg)](http://www.youtube.com/watch?v=uIicDGLsv4s "Steam Screenshot Downloader")