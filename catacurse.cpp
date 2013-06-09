#if (defined _WIN32 || defined WINDOWS)
#include "catacurse.h"
#include "options.h"
#include <cstdlib>
#include <fstream>

//CAT-s:
#include <SDL.h>
#include <SDL_mixer.h>


//***********************************
//Globals                           *
//***********************************

WINDOW *mainwin;
const WCHAR *szWindowClass = (L"CataCurseWindow");    //Class name :D
HINSTANCE WindowINST;   //the instance of the window
HWND WindowHandle;      //the handle of the window
HDC WindowDC;           //Device Context of the window, used for backbuffer
int WindowX;            //X pos of the actual window, not the curses window
int WindowY;            //Y pos of the actual window, not the curses window
int WindowWidth;        //Width of the actual window, not the curses window
int WindowHeight;       //Height of the actual window, not the curses window
int lastchar;          //the last character that was pressed, resets in getch
int inputdelay;         //How long getch will wait for a character to be typed
//WINDOW *_windows;  //Probably need to change this to dynamic at some point
//int WindowCount;        //The number of curses windows currently in use
HDC backbuffer;         //an off-screen DC to prevent flickering, lower cpu
HBITMAP backbit;        //the bitmap that is used in conjunction wth the above
int fontwidth;          //the width of the font, background is always this size
int fontheight;         //the height of the font, background is always this size
int halfwidth;          //half of the font width, used for centering lines
int halfheight;          //half of the font height, used for centering lines
HFONT font;             //Handle to the font created by CreateFont
RGBQUAD *windowsPalette;  //The coor palette, 16 colors emulates a terminal
pairs *colorpairs;   //storage for pair'ed colored, should be dynamic, meh
unsigned char *dcbits;  //the bits of the screen image, for direct access
char szDirectory[MAX_PATH] = "";


//CATs: *** whole lot *** 
Mix_Music *music[10];
Mix_Chunk *sound[99];

int currentMusic= -1;
int currentMP3= 99;
int musicVolume= -1;
int loopVol= 127;

int musicPause= -1;
int currentSound= -1;
int currentChannel= 7;

void initAudio()
{
   SDL_Init(SDL_INIT_AUDIO);
   Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 4096);

//*** background music
	music[0] = Mix_LoadMUS("./data/audio/bkg_tense.wav");
	music[1] = Mix_LoadMUS("./data/audio/bkg_indoors.wav");
	music[2] = Mix_LoadMUS("./data/audio/bkg_underground.wav");
	music[3] = Mix_LoadMUS("./data/audio/bkg_windy.wav");

	music[4] = Mix_LoadMUS("./data/audio/bkg_mp3-1.ogg");
	music[5] = Mix_LoadMUS("./data/audio/bkg_mp3-2.ogg");
	music[6] = Mix_LoadMUS("./data/audio/bkg_mp3-3.ogg");

//	music[7] = Mix_LoadMUS("./data/audio/bkg_mp3-4.ogg");
//	music[8] = Mix_LoadMUS("./data/audio/bkg_mp3-5.ogg");

//	music[9] = Mix_LoadMUS("./data/audio/bkg_tense2.ogg");

//*** sound effects
	sound[0] = Mix_LoadWAV("./data/audio/menuClick.wav");
	sound[1] = Mix_LoadWAV("./data/audio/menuOpen.wav");
	sound[2] = Mix_LoadWAV("./data/audio/menuClose.wav");
	sound[3] = Mix_LoadWAV("./data/audio/menuWrong.wav");

	sound[4] = Mix_LoadWAV("./data/audio/actionGet.wav");
	sound[5] = Mix_LoadWAV("./data/audio/actionDrop.wav");
	sound[6] = Mix_LoadWAV("./data/audio/actionUse.wav");

//	sound[7] = Mix_LoadWAV("./data/audio/hearBeat1.wav");

	sound[8] = Mix_LoadWAV("./data/audio/pain1.wav");
	sound[9] = Mix_LoadWAV("./data/audio/pain2.wav");
	sound[10] = Mix_LoadWAV("./data/audio/alert.wav");
	sound[11] = Mix_LoadWAV("./data/audio/died.wav");
	sound[12] = Mix_LoadWAV("./data/audio/gameOver.wav");

	sound[13] = Mix_LoadWAV("./data/audio/monster05.wav");
	sound[14] = Mix_LoadWAV("./data/audio/monster06.wav");
	sound[15] = Mix_LoadWAV("./data/audio/monster07.wav");
	sound[16] = Mix_LoadWAV("./data/audio/monster08.wav");
	sound[17] = Mix_LoadWAV("./data/audio/monster01.wav");
	sound[18] = Mix_LoadWAV("./data/audio/monster02.wav");
	sound[19] = Mix_LoadWAV("./data/audio/monster03.wav");
	sound[20] = Mix_LoadWAV("./data/audio/monster04.wav");
	sound[21] = Mix_LoadWAV("./data/audio/monster09.wav");
	sound[22] = Mix_LoadWAV("./data/audio/monster10.wav");

	sound[23] = Mix_LoadWAV("./data/audio/sparks1.wav");
	sound[24] = Mix_LoadWAV("./data/audio/sparks2.wav");

	sound[25] = Mix_LoadWAV("./data/audio/glassBreak1.wav");
	sound[26] = Mix_LoadWAV("./data/audio/smash1.wav");
	sound[27] = Mix_LoadWAV("./data/audio/splash1.wav");
	sound[28] = Mix_LoadWAV("./data/audio/butcher.wav");
	sound[29] = Mix_LoadWAV("./data/audio/splat1.wav");

	sound[30] = Mix_LoadWAV("./data/audio/atmos1.wav");
	sound[31] = Mix_LoadWAV("./data/audio/atmos2.wav");

	sound[33] = Mix_LoadWAV("./data/audio/unload1.wav");
	sound[34] = Mix_LoadWAV("./data/audio/reload1.wav");
	sound[35] = Mix_LoadWAV("./data/audio/reload2.wav");

//	sound[38] = 
//	sound[39] = 
//	sound[50] = 

	sound[41] = Mix_LoadWAV("./data/audio/shellDrop.wav");
	sound[42] = Mix_LoadWAV("./data/audio/ricochet1.wav");
	sound[43] = Mix_LoadWAV("./data/audio/ricochet2.wav");

	sound[44] = Mix_LoadWAV("./data/audio/punch1.wav");
	sound[45] = Mix_LoadWAV("./data/audio/punch2.wav");
	sound[46] = Mix_LoadWAV("./data/audio/punch3.wav");
	sound[47] = Mix_LoadWAV("./data/audio/punchMiss.wav");

	sound[48] = Mix_LoadWAV("./data/audio/explosion1.wav");
	sound[49] = Mix_LoadWAV("./data/audio/explosion2.wav");
	sound[50] = Mix_LoadWAV("./data/audio/bigCrash.wav");

	sound[51] = Mix_LoadWAV("./data/audio/loopEngine1.wav");
//
//

	sound[54] = Mix_LoadWAV("./data/audio/cut.wav");
	sound[55] = Mix_LoadWAV("./data/audio/step.wav");
	sound[56] = Mix_LoadWAV("./data/audio/open.wav");
	sound[57] = Mix_LoadWAV("./data/audio/close.wav");
	sound[58] = Mix_LoadWAV("./data/audio/alarm1.wav");
	sound[59] = Mix_LoadWAV("./data/audio/unlock.wav");
	sound[60] = Mix_LoadWAV("./data/audio/crowbar.wav");

//
//
	sound[63] = Mix_LoadWAV("./data/audio/bigCrash.wav");

	sound[64] = Mix_LoadWAV("./data/audio/loopRain1.wav");
	sound[65] = Mix_LoadWAV("./data/audio/loopRain2.wav");
	sound[66] = Mix_LoadWAV("./data/audio/loopRain3.wav");
	sound[67] = Mix_LoadWAV("./data/audio/loopRain4.wav");


	sound[68] = Mix_LoadWAV("./data/audio/gunFire1.wav");
	sound[69] = Mix_LoadWAV("./data/audio/gunFire2.wav");
	sound[70] = Mix_LoadWAV("./data/audio/gunFire3.wav");
	sound[71] = Mix_LoadWAV("./data/audio/gunFire4.wav");

	sound[72] = Mix_LoadWAV("./data/audio/arrowShoot.wav");
	sound[73] = Mix_LoadWAV("./data/audio/throw.wav");
	sound[74] = Mix_LoadWAV("./data/audio/fireLight.wav");


	sound[75] = Mix_LoadWAV("./data/audio/thunder1.wav");
	sound[76] = Mix_LoadWAV("./data/audio/thunder2.wav");
	sound[77] = Mix_LoadWAV("./data/audio/thunder3.wav");
//CAT: too short
	sound[78] = Mix_LoadWAV("./data/audio/thunder4.wav"); 
	

//*** NOISE, too many, too similar?
	sound[79] = Mix_LoadWAV("./data/audio/noiseDig.wav");
	sound[80] = Mix_LoadWAV("./data/audio/noiseBrush.wav");
	sound[81] = Mix_LoadWAV("./data/audio/noiseBump.wav");
	sound[82] = Mix_LoadWAV("./data/audio/noiseClang.wav");
	sound[83] = Mix_LoadWAV("./data/audio/noiseCleng.wav");
	sound[84] = Mix_LoadWAV("./data/audio/noiseCling.wav");
	sound[85] = Mix_LoadWAV("./data/audio/noiseCrack.wav");
	sound[86] = Mix_LoadWAV("./data/audio/noiseCrash.wav");
	sound[87] = Mix_LoadWAV("./data/audio/noiseGlass.wav");
	sound[88] = Mix_LoadWAV("./data/audio/noiseKlang.wav");
	sound[89] = Mix_LoadWAV("./data/audio/noiseKleng.wav");
	sound[90] = Mix_LoadWAV("./data/audio/noiseKlong.wav");
	sound[91] = Mix_LoadWAV("./data/audio/noiseSmack.wav");
	sound[92] = Mix_LoadWAV("./data/audio/noiseSwoosh.wav");
	sound[93] = Mix_LoadWAV("./data/audio/noiseThump.wav");
	sound[94] = Mix_LoadWAV("./data/audio/noiseWhack.wav");

	sound[95] = Mix_LoadWAV("./data/audio/shriek1.wav");
	sound[96] = Mix_LoadWAV("./data/audio/monsterDie1.wav");
//	sound[97] = Mix_LoadWAV("./data/audio/monsterDie2.wav");

	sound[98] = Mix_LoadWAV("./data/audio/lockOn.wav");


//Mix_HaltMusic();
//Mix_FreeMusic(music);
//Mix_PlayingMusic() == 0

}

void playMusic(int ID) 
{
   musicPause--;
   if(musicPause < 0 && currentMusic != ID && !mp3Track())
   {
	fadeMusic(127);
	Mix_PlayMusic(music[ID], -1);
	currentMusic= ID;
	musicPause= 0;
   }
}

bool mp3Track()
{
   return
	 ( (currentMusic > 3)
		&& (currentMusic < 9)
		&& (Mix_PlayingMusic() != 0) );
}

void mp3Next()
{
	currentMP3++;
	if(currentMP3 > 6 || currentMP3 < 4)
		currentMP3= 4;

	fadeMusic(127);
	Mix_PlayMusic(music[currentMP3], 0);

	currentMusic= currentMP3;
}

void mp3Stop()
{
   Mix_HaltMusic();
   currentMusic= -1;
}

/* 1st attempt
void fadeMusic(int delay)
{
	musicVolume-= (int)(20/delay);
      Mix_VolumeMusic(musicVolume);
	
	if(musicPause < 0)
		musicPause= delay;

//why does it wait until finished?
//	Mix_FadeOutMusic(delay*500);
}
*/

void fadeMusic(int dVolume)
{
	musicVolume+= dVolume;
	if(musicVolume > 110)
		musicVolume= 110;
	else
	if(musicVolume < 0)
		musicVolume= 0;

      Mix_VolumeMusic(musicVolume);
}


void stopMusic(int delay) 
{
   Mix_HaltMusic();
   currentMusic= -1;
   musicPause= delay;
}


void playSound(int ID) 
{
//CAT:  || (ID>12 && ID<23) -- stack in a single channel?
   if(ID == 0) 
	currentChannel= 7;	
   else
	currentChannel= -1;

   Mix_Volume(-1, 120);
   Mix_PlayChannel(currentChannel, sound[ID], 0);

   loopVolume(127);
}


void playLoop(int ID) 
{
   if(currentSound != ID)	
   {
	loopVolume(127);
	Mix_PlayChannel(0, sound[ID], -1);
	currentSound= ID;
   }
}

//CAT: don't need ID
//CAT: void stopLoop(void) 
void stopLoop(int ID) 
{
//CAT: don't need ID, should be:
//   if(currentSound >= 0)	

   if(currentSound >= 0 && (currentSound == ID || ID == -99))	
   {
	Mix_HaltChannel(0);
	currentSound= -1;
   }
}

void loopVolume(int volume)
{
	if(volume > 80)
		volume= 80;

	loopVol= volume;
	Mix_Volume(0, loopVol);
}




//***********************************
//Non-curses, Window functions      *
//***********************************

//Registers, creates, and shows the Window!!
bool WinCreate()
{
    int success;
    WNDCLASSEXW WindowClassType;
    int WinBorderHeight;
    int WinBorderWidth;
    int WinTitleSize;
    unsigned int WindowStyle;
    const WCHAR *szTitle=  (L"Cataclysm: Dark Days Ahead - 0.3 Prerelease :: MGSmod");
    WinTitleSize = GetSystemMetrics(SM_CYCAPTION);      //These lines ensure
    WinBorderWidth = GetSystemMetrics(SM_CXDLGFRAME) * 2;  //that our window will
    WinBorderHeight = GetSystemMetrics(SM_CYDLGFRAME) * 2; // be a perfect size
    WindowClassType.cbSize = sizeof(WNDCLASSEXW);
    WindowClassType.style = 0;//No point in having a custom style, no mouse, etc
    WindowClassType.lpfnWndProc = ProcessMessages;//the procedure that gets msgs
    WindowClassType.cbClsExtra = 0;
    WindowClassType.cbWndExtra = 0;
    WindowClassType.hInstance = WindowINST;// hInstance
    WindowClassType.hIcon = LoadIcon(WindowINST, IDI_APPLICATION);//Default Icon
    WindowClassType.hIconSm = LoadIcon(WindowINST, IDI_APPLICATION);//Default Icon
    WindowClassType.hCursor = LoadCursor(NULL, IDC_ARROW);//Default Pointer
    WindowClassType.lpszMenuName = NULL;
    WindowClassType.hbrBackground = 0;//Thanks jday! Remove background brush
    WindowClassType.lpszClassName = szWindowClass;
    success = RegisterClassExW(&WindowClassType);
    if ( success== 0){
        return false;
    }
    WindowStyle = WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME) & ~(WS_MAXIMIZEBOX);
    WindowHandle = CreateWindowExW(WS_EX_APPWINDOW || WS_EX_TOPMOST * 0,
                                   szWindowClass , szTitle,WindowStyle, WindowX,
                                   WindowY, WindowWidth + (WinBorderWidth) * 1,
                                   WindowHeight + (WinBorderHeight +
                                   WinTitleSize) * 1, 0, 0, WindowINST, NULL);
    if (WindowHandle == 0){
        return false;
    }
    ShowWindow(WindowHandle,5);

//CAT-s:
    initAudio();

    return true;
};

//Unregisters, releases the DC if needed, and destroys the window.
void WinDestroy()
{
    if ((WindowDC > 0) && (ReleaseDC(WindowHandle, WindowDC) == 0)){
        WindowDC = 0;
    }
    if ((!WindowHandle == 0) && (!(DestroyWindow(WindowHandle)))){
        WindowHandle = 0;
    }
    if (!(UnregisterClassW(szWindowClass, WindowINST))){
        WindowINST = 0;
    }
};

//This function processes any Windows messages we get. Keyboard, OnClose, etc
LRESULT CALLBACK ProcessMessages(HWND__ *hWnd,unsigned int Msg,
                                 WPARAM wParam, LPARAM lParam)
{
    switch (Msg){
        case WM_CHAR:               //This handles most key presses
            lastchar=(int)wParam;
            switch (lastchar){
                case 13:            //Reroute ENTER key for compatilbity purposes
                    lastchar=10;
                    break;
                case 8:             //Reroute BACKSPACE key for compatilbity purposes
                    lastchar=127;
                    break;
            };
            break;
        case WM_KEYDOWN:                //Here we handle non-character input
            switch (wParam){
                case VK_LEFT:
                    lastchar = KEY_LEFT;
                    break;
                case VK_RIGHT:
                    lastchar = KEY_RIGHT;
                    break;
                case VK_UP:
                    lastchar = KEY_UP;
                    break;
                case VK_DOWN:
                    lastchar = KEY_DOWN;
                    break;
                default:
                    break;
            };
        case WM_ERASEBKGND:
            return 1;               //We don't want to erase our background
        case WM_PAINT:              //Pull from our backbuffer, onto the screen
            BitBlt(WindowDC, 0, 0, WindowWidth, WindowHeight, backbuffer, 0, 0,SRCCOPY);
            ValidateRect(WindowHandle,NULL);
            break;
        case WM_DESTROY:
            exit(0);//A messy exit, but easy way to escape game loop
        default://If we didnt process a message, return the default value for it
            return DefWindowProcW(hWnd, Msg, wParam, lParam);
    };
    return 0;
};

//The following 3 methods use mem functions for fast drawing
inline void VertLineDIB(int x, int y, int y2,int thickness, unsigned char color)
{
    int j;
    for (j=y; j<y2; j++)
        memset(&dcbits[x+j*WindowWidth],color,thickness);
};
inline void HorzLineDIB(int x, int y, int x2,int thickness, unsigned char color)
{
    int j;
    for (j=y; j<y+thickness; j++)
        memset(&dcbits[x+j*WindowWidth],color,x2-x);
};
inline void FillRectDIB(int x, int y, int width, int height, unsigned char color)
{
    int j;
    for (j=y; j<y+height; j++)
        memset(&dcbits[x+j*WindowWidth],color,width);
};

void DrawWindow(WINDOW *win)
{
    int i,j,drawx,drawy;
    char tmp;
    for (j=0; j<win->height; j++){
        if (win->line[j].touched)
            for (i=0; i<win->width; i++){
                win->line[j].touched=false;
                drawx=((win->x+i)*fontwidth);
                drawy=((win->y+j)*fontheight);//-j;
                if (((drawx+fontwidth)<=WindowWidth) && ((drawy+fontheight)<=WindowHeight)){
                tmp = win->line[j].chars[i];
                int FG = win->line[j].FG[i];
                int BG = win->line[j].BG[i];
                FillRectDIB(drawx,drawy,fontwidth,fontheight,BG);

                if ( tmp > 0){
                //if (tmp==95){//If your font doesnt draw underscores..uncomment
                //        HorzLineDIB(drawx,drawy+fontheight-2,drawx+fontwidth,1,FG);
                //    } else { // all the wa to here
                    int color = RGB(windowsPalette[FG].rgbRed,windowsPalette[FG].rgbGreen,windowsPalette[FG].rgbBlue);
                    SetTextColor(backbuffer,color);
                    ExtTextOut(backbuffer,drawx,drawy,0,NULL,&tmp,1,NULL);
                //    }     //and this line too.
                } else if (  tmp < 0 ) {
                    switch (tmp) {
                    case -60://box bottom/top side (horizontal line)
                        HorzLineDIB(drawx,drawy+halfheight,drawx+fontwidth,1,FG);
                        break;
                    case -77://box left/right side (vertical line)
                        VertLineDIB(drawx+halfwidth,drawy,drawy+fontheight,2,FG);
                        break;
                    case -38://box top left
                        HorzLineDIB(drawx+halfwidth,drawy+halfheight,drawx+fontwidth,1,FG);
                        VertLineDIB(drawx+halfwidth,drawy+halfheight,drawy+fontheight,2,FG);
                        break;
                    case -65://box top right
                        HorzLineDIB(drawx,drawy+halfheight,drawx+halfwidth,1,FG);
                        VertLineDIB(drawx+halfwidth,drawy+halfheight,drawy+fontheight,2,FG);
                        break;
                    case -39://box bottom right
                        HorzLineDIB(drawx,drawy+halfheight,drawx+halfwidth,1,FG);
                        VertLineDIB(drawx+halfwidth,drawy,drawy+halfheight+1,2,FG);
                        break;
                    case -64://box bottom left
                        HorzLineDIB(drawx+halfwidth,drawy+halfheight,drawx+fontwidth,1,FG);
                        VertLineDIB(drawx+halfwidth,drawy,drawy+halfheight+1,2,FG);
                        break;
                    case -63://box bottom north T (left, right, up)
                        HorzLineDIB(drawx,drawy+halfheight,drawx+fontwidth,1,FG);
                        VertLineDIB(drawx+halfwidth,drawy,drawy+halfheight,2,FG);
                        break;
                    case -61://box bottom east T (up, right, down)
                        VertLineDIB(drawx+halfwidth,drawy,drawy+fontheight,2,FG);
                        HorzLineDIB(drawx+halfwidth,drawy+halfheight,drawx+fontwidth,1,FG);
                        break;
                    case -62://box bottom south T (left, right, down)
                        HorzLineDIB(drawx,drawy+halfheight,drawx+fontwidth,1,FG);
                        VertLineDIB(drawx+halfwidth,drawy+halfheight,drawy+fontheight,2,FG);
                        break;
                    case -59://box X (left down up right)
                        HorzLineDIB(drawx,drawy+halfheight,drawx+fontwidth,1,FG);
                        VertLineDIB(drawx+halfwidth,drawy,drawy+fontheight,2,FG);
                        break;
                    case -76://box bottom east T (left, down, up)
                        VertLineDIB(drawx+halfwidth,drawy,drawy+fontheight,2,FG);
                        HorzLineDIB(drawx,drawy+halfheight,drawx+halfwidth,1,FG);
                        break;
                    default:
                        // SetTextColor(DC,_windows[w].line[j].chars[i].color.FG);
                        // TextOut(DC,drawx,drawy,&tmp,1);
                        break;
                    }
                    };//switch (tmp)
                }//(tmp < 0)
            };//for (i=0;i<_windows[w].width;i++)
    };// for (j=0;j<_windows[w].height;j++)
    win->draw=false;                //We drew the window, mark it as so

//CAT-g:
  InvalidateRect(WindowHandle,NULL,true);
  UpdateWindow(WindowHandle);

};

//Check for any window messages (keypress, paint, mousemove, etc)
void CheckMessages()
{
    MSG msg;
    while (PeekMessage(&msg, 0 , 0, 0, PM_REMOVE)){
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
};

//***********************************
//Psuedo-Curses Functions           *
//***********************************

//Basic Init, create the font, backbuffer, etc
WINDOW *initscr(void)
{
   // _windows = new WINDOW[20];         //initialize all of our variables
    BITMAPINFO bmi;
    lastchar=-1;
    inputdelay=-1;
    std::string typeface;
char * typeface_c;
std::ifstream fin;
fin.open("data\\FONTDATA");
 if (!fin.is_open()){
     MessageBox(WindowHandle, "Failed to open FONTDATA, loading defaults.",
                NULL, 0);
     fontheight=16;
     fontwidth=8;
 } else {
     getline(fin, typeface);
     typeface_c= new char [typeface.size()+1];
     strcpy (typeface_c, typeface.c_str());
     fin >> fontwidth;
     fin >> fontheight;
     if ((fontwidth <= 4) || (fontheight <=4)){
         MessageBox(WindowHandle, "Invalid font size specified!",
                    NULL, 0);
        fontheight=16;
        fontwidth=8;
     }
 }
    halfwidth=fontwidth / 2;
    halfheight=fontheight / 2;
    WindowWidth= (55 + (OPTIONS[OPT_VIEWPORT_X] * 2 + 1)) * fontwidth;
    WindowHeight= (OPTIONS[OPT_VIEWPORT_Y] * 2 + 1) *fontheight;
    WindowX=(GetSystemMetrics(SM_CXSCREEN) / 2)-WindowWidth/2;    //center this
    WindowY=(GetSystemMetrics(SM_CYSCREEN) / 2)-WindowHeight/2;   //sucker
    WinCreate();    //Create the actual window, register it, etc
    CheckMessages();    //Let the message queue handle setting up the window
    WindowDC = GetDC(WindowHandle);
    backbuffer = CreateCompatibleDC(WindowDC);
    ZeroMemory(&bmi, sizeof(BITMAPINFO));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = WindowWidth;
    bmi.bmiHeader.biHeight = -WindowHeight;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount=8;
    bmi.bmiHeader.biCompression = BI_RGB;   //store it in uncompressed bytes
    bmi.bmiHeader.biSizeImage = WindowWidth * WindowHeight * 1;
    bmi.bmiHeader.biClrUsed=16;         //the number of colors in our palette
    bmi.bmiHeader.biClrImportant=16;    //the number of colors in our palette
    backbit = CreateDIBSection(0, &bmi, DIB_RGB_COLORS, (void**)&dcbits, NULL, 0);
    DeleteObject(SelectObject(backbuffer, backbit));//load the buffer into DC

 int nResults = AddFontResourceExA("data\\termfont",FR_PRIVATE,NULL);
   if (nResults>0){
    font = CreateFont(fontheight, fontwidth, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                      ANSI_CHARSET, OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
                      PROOF_QUALITY, FF_MODERN, typeface_c);   //Create our font

  } else {
      MessageBox(WindowHandle, "Failed to load default font, using FixedSys.",
                NULL, 0);
       font = CreateFont(fontheight, fontwidth, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                      ANSI_CHARSET, OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
                      PROOF_QUALITY, FF_MODERN, "FixedSys");   //Create our font
   }
    //FixedSys will be user-changable at some point in time??
    SetBkMode(backbuffer, TRANSPARENT);//Transparent font backgrounds
    SelectObject(backbuffer, font);//Load our font into the DC
//    WindowCount=0;

    delete typeface_c;
    mainwin = newwin((OPTIONS[OPT_VIEWPORT_Y] * 2 + 1),(55 + (OPTIONS[OPT_VIEWPORT_Y] * 2 + 1)),0,0);
    return mainwin;   //create the 'stdscr' window and return its ref
};

WINDOW *newwin(int nlines, int ncols, int begin_y, int begin_x)
{
    int i,j;
    WINDOW *newwindow = new WINDOW;
    //newwindow=&_windows[WindowCount];
    newwindow->x=begin_x;
    newwindow->y=begin_y;
    newwindow->width=ncols;
    newwindow->height=nlines;
    newwindow->inuse=true;
    newwindow->draw=false;
    newwindow->BG=0;

//CAT-g: make it yellow
    newwindow->FG= 11; 

    newwindow->cursorx=0;
    newwindow->cursory=0;
    newwindow->line = new curseline[nlines];

    for (j=0; j<nlines; j++)
    {
        newwindow->line[j].chars= new char[ncols];
        newwindow->line[j].FG= new char[ncols];
        newwindow->line[j].BG= new char[ncols];
        newwindow->line[j].touched=true;//Touch them all !?
        for (i=0; i<ncols; i++)
        {
          newwindow->line[j].chars[i]=0;
          newwindow->line[j].FG[i]=0;
          newwindow->line[j].BG[i]=0;
        }
    }
    //WindowCount++;
    return newwindow;
};


//Deletes the window and marks it as free. Clears it just in case.
int delwin(WINDOW *win)
{
    int j;
    win->inuse=false;
    win->draw=false;
    for (j=0; j<win->height; j++){
        delete win->line[j].chars;
        delete win->line[j].FG;
        delete win->line[j].BG;
        }
    delete win->line;
    delete win;
    return 1;
};

inline int newline(WINDOW *win){
    if (win->cursory < win->height - 1){
        win->cursory++;
        win->cursorx=0;
        return 1;
    }
return 0;
};

inline void addedchar(WINDOW *win){
    win->cursorx++;
    win->line[win->cursory].touched=true;
    if (win->cursorx > win->width)
        newline(win);
};


//Borders the window with fancy lines!
int wborder(WINDOW *win, chtype ls, chtype rs, chtype ts, chtype bs, chtype tl, chtype tr, chtype bl, chtype br)
{

    int i, j;
    int oldx=win->cursorx;//methods below move the cursor, save the value!
    int oldy=win->cursory;//methods below move the cursor, save the value!
    if (ls>0)
        for (j=1; j<win->height-1; j++)
            mvwaddch(win, j, 0, 179);
    if (rs>0)
        for (j=1; j<win->height-1; j++)
            mvwaddch(win, j, win->width-1, 179);
    if (ts>0)
        for (i=1; i<win->width-1; i++)
            mvwaddch(win, 0, i, 196);
    if (bs>0)
        for (i=1; i<win->width-1; i++)
            mvwaddch(win, win->height-1, i, 196);
    if (tl>0)
        mvwaddch(win,0, 0, 218);
    if (tr>0)
        mvwaddch(win,0, win->width-1, 191);
    if (bl>0)
        mvwaddch(win,win->height-1, 0, 192);
    if (br>0)
        mvwaddch(win,win->height-1, win->width-1, 217);
    //_windows[w].cursorx=oldx;//methods above move the cursor, put it back
    //_windows[w].cursory=oldy;//methods above move the cursor, put it back
    wmove(win,oldy,oldx);
    return 1;
};

//Refreshes a window, causing it to redraw on top.
int wrefresh(WINDOW *win)
{
    if (win==0) win=mainwin;
    if (win->draw)
        DrawWindow(win);
    return 1;
};

//Refreshes window 0 (stdscr), causing it to redraw on top.
int refresh(void)
{
    return wrefresh(mainwin);
};


//CAT-g:
int getch(void)
{

  lastchar=ERR;
  do{
    CheckMessages();

//low cpu wait?
    MsgWaitForMultipleObjects(0, NULL, FALSE, 10, QS_ALLEVENTS); 
  }while (lastchar==ERR);


/* attempt to fix keyboard buffering problems on Linux...
int out= 0;
asm
(
	"xor %%ah, %%ah;"
	"int $0x16;"
	: "=a" (out)
	: "a" (out)
	: 
);
*/
  return lastchar;
};

//The core printing function, prints characters to the array, and sets colors
inline int printstring(WINDOW *win, char *fmt)
{
 int size = strlen(fmt);
 int j;
 for (j=0; j<size; j++){
  if (!(fmt[j]==10)){//check that this isnt a newline char
   if (win->cursorx <= win->width - 1 && win->cursory <= win->height - 1) {
    win->line[win->cursory].chars[win->cursorx]=fmt[j];
    win->line[win->cursory].FG[win->cursorx]=win->FG;
    win->line[win->cursory].BG[win->cursorx]=win->BG;
    win->line[win->cursory].touched=true;
    addedchar(win);
   } else
   return 0; //if we try and write anything outside the window, abort completely
} else // if the character is a newline, make sure to move down a line
  if (newline(win)==0){
      return 0;
      }
 }
 win->draw=true;
 return 1;
}

//Prints a formatted string to a window at the current cursor, base function
int wprintw(WINDOW *win, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char printbuf[2048];
    vsnprintf(printbuf, 2047, fmt, args);
    va_end(args);
    return printstring(win,printbuf);
};

//Prints a formatted string to a window, moves the cursor
int mvwprintw(WINDOW *win, int y, int x, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char printbuf[2048];
    vsnprintf(printbuf, 2047, fmt, args);
    va_end(args);
    if (wmove(win,y,x)==0) return 0;
    return printstring(win,printbuf);
};

//Prints a formatted string to window 0 (stdscr), moves the cursor
int mvprintw(int y, int x, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char printbuf[2048];
    vsnprintf(printbuf, 2047, fmt, args);
    va_end(args);
    if (move(y,x)==0) return 0;
    return printstring(mainwin,printbuf);
};

//Prints a formatted string to window 0 (stdscr) at the current cursor
int printw(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char printbuf[2078];
    vsnprintf(printbuf, 2047, fmt, args);
    va_end(args);
    return printstring(mainwin,printbuf);
};

//erases a window of all text and attributes
int werase(WINDOW *win)
{
    int j,i;
    for (j=0; j<win->height; j++)
    {
     for (i=0; i<win->width; i++)   {
     win->line[j].chars[i]=0;
     win->line[j].FG[i]=0;
     win->line[j].BG[i]=0;
     }
        win->line[j].touched=true;
    }

//CAT-g:
//    win->draw=true;
//    wmove(win,0,0);
//    wrefresh(win);

    return 1;
};

//erases window 0 (stdscr) of all text and attributes
int erase(void)
{
    return werase(mainwin);
};

//pairs up a foreground and background color and puts it into the array of pairs
int init_pair(short pair, short f, short b)
{
    colorpairs[pair].FG=f;
    colorpairs[pair].BG=b;
    return 1;
};

//moves the cursor in a window
int wmove(WINDOW *win, int y, int x)
{
    if (x>=win->width)
     {return 0;}//FIXES MAP CRASH -> >= vs > only
    if (y>=win->height)
     {return 0;}// > crashes?
    if (y<0)
     {return 0;}
    if (x<0)
     {return 0;}
    win->cursorx=x;
    win->cursory=y;
    return 1;
};

//Clears windows 0 (stdscr)     I'm not sure if its suppose to do this?
int clear(void)
{
    return wclear(mainwin);
};

//Ends the terminal, destroy everything
int endwin(void)
{
    DeleteObject(font);
    WinDestroy();
    RemoveFontResourceExA("data\\termfont",FR_PRIVATE,NULL);//Unload it
    return 1;
};

//adds a character to the window
int mvwaddch(WINDOW *win, int y, int x, const chtype ch)
{
   if (wmove(win,y,x)==0) return 0;
   return waddch(win, ch);
};

//clears a window
int wclear(WINDOW *win)
{
    werase(win);
    wrefresh(win);
    return 1;
};

//gets the max x of a window (the width)
int getmaxx(WINDOW *win)
{
    if (win==0) return mainwin->width;     //StdScr
    return win->width;
};

//gets the max y of a window (the height)
int getmaxy(WINDOW *win)
{
    if (win==0) return mainwin->height;     //StdScr
    return win->height;
};

//gets the beginning x of a window (the x pos)
int getbegx(WINDOW *win)
{
    if (win==0) return mainwin->x;     //StdScr
    return win->x;
};

//gets the beginning y of a window (the y pos)
int getbegy(WINDOW *win)
{
    if (win==0) return mainwin->y;     //StdScr
    return win->y;
};

inline RGBQUAD BGR(int b, int g, int r)
{
    RGBQUAD result;
    result.rgbBlue=b;    //Blue
    result.rgbGreen=g;    //Green
    result.rgbRed=r;    //Red
    result.rgbReserved=0;//The Alpha, isnt used, so just set it to 0
    return result;
};

//CAT-g:
int start_color(void)
{
 colorpairs=new pairs[50];
 windowsPalette=new RGBQUAD[16]; //Colors in the struct are BGR!! not RGB!!
 windowsPalette[0]= BGR(0,0,0); // Black
 windowsPalette[1]= BGR(0, 0, 200); // Red
 windowsPalette[2]= BGR(0,150,0); // Green
 windowsPalette[3]= BGR(50,90,170); // Brown???
 windowsPalette[4]= BGR(200, 0, 0); // Blue
 windowsPalette[5]= BGR(98, 58, 139); // Purple
 windowsPalette[6]= BGR(180, 150, 0); // Cyan
 windowsPalette[7]= BGR(196, 196, 196);// Gray
 windowsPalette[8]= BGR(90, 90, 90);// Dark Gray
 windowsPalette[9]= BGR(0, 80, 255); // Light Red
 windowsPalette[10]= BGR(0, 255, 0); // Bright Green
 windowsPalette[11]= BGR(0, 255, 255); // Yellow
 windowsPalette[12]= BGR(255, 100, 100); // Light Blue
 windowsPalette[13]= BGR(240, 0, 255); // Pink
 windowsPalette[14]= BGR(255, 240, 0); // Light Cyan?
 windowsPalette[15]= BGR(255, 255, 255); // White
 return SetDIBColorTable(backbuffer, 0, 16, windowsPalette);
};

int keypad(WINDOW *faux, bool bf)
{
return 1;
};

int noecho(void)
{
    return 1;
};
int cbreak(void)
{
    return 1;
};
int keypad(int faux, bool bf)
{
    return 1;
};
int curs_set(int visibility)
{
    return 1;
};

int mvaddch(int y, int x, const chtype ch)
{
    return mvwaddch(mainwin,y,x,ch);
};

int wattron(WINDOW *win, int attrs)
{
    bool isBold = !!(attrs & A_BOLD);
    bool isBlink = !!(attrs & A_BLINK);
    int pairNumber = (attrs & A_COLOR) >> 17;
    win->FG=colorpairs[pairNumber].FG;
    win->BG=colorpairs[pairNumber].BG;
    if (isBold) win->FG += 8;
    if (isBlink) win->BG += 8;
    return 1;
};

int wattroff(WINDOW *win, int attrs)
{
//CAT-g: make it yellow
     win->FG= 11;		//reset to white (gray actually)
     win->BG= 0;		//reset to black
    return 1;
};
int attron(int attrs)
{
    return wattron(mainwin, attrs);
};
int attroff(int attrs)
{
    return wattroff(mainwin,attrs);
};
int waddch(WINDOW *win, const chtype ch)
{
    char charcode;
    charcode=ch;

    switch (ch){        //LINE_NESW  - X for on, O for off
        case 4194424:   //#define LINE_XOXO 4194424
            charcode=179;
            break;
        case 4194417:   //#define LINE_OXOX 4194417
            charcode=196;
            break;
        case 4194413:   //#define LINE_XXOO 4194413
            charcode=192;
            break;
        case 4194412:   //#define LINE_OXXO 4194412
            charcode=218;
            break;
        case 4194411:   //#define LINE_OOXX 4194411
            charcode=191;
            break;
        case 4194410:   //#define LINE_XOOX 4194410
            charcode=217;
            break;
        case 4194422:   //#define LINE_XXOX 4194422
            charcode=193;
            break;
        case 4194420:   //#define LINE_XXXO 4194420
            charcode=195;
            break;
        case 4194421:   //#define LINE_XOXX 4194421
            charcode=180;
            break;
        case 4194423:   //#define LINE_OXXX 4194423
            charcode=194;
            break;
        case 4194414:   //#define LINE_XXXX 4194414
            charcode=197;
            break;
        default:
            charcode = (char)ch;
            break;
        }


int curx=win->cursorx;
int cury=win->cursory;

//if (win2 > -1){
   win->line[cury].chars[curx]=charcode;
   win->line[cury].FG[curx]=win->FG;
   win->line[cury].BG[curx]=win->BG;


    win->draw=true;
    addedchar(win);
    return 1;
  //  else{
  //  win2=win2+1;

};


//Move the cursor of windows 0 (stdscr)
int move(int y, int x)
{
    return wmove(mainwin,y,x);
};

//Set the amount of time getch waits for input
void timeout(int delay)
{
    inputdelay=delay;
};
void set_escdelay(int delay) { } //PORTABILITY, DUMMY FUNCTION

#endif
