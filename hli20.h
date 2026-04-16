/*-----------------------------------------------
  HLI20.H - Include-Datei für hli20.dll
-----------------------------------------------*/

//#pragma comment (lib, "vfw32")    // kann für Demo Programm wohl raus.
#pragma comment (lib, "hli20") // muss für Demo-Programm wieder 'rein.
#include <windows.h>
#include <vfw.h>
#include <direct.h>
#include <math.h>
//#include "opendlg.h"
#include <stdio.h>
#include <commdlg.h>
#include <io.h>  // aus cone
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "resource.h"
#include <mmsystem.h>
//#include <atlimage.h>

#undef EXPORT
#ifdef  __cplusplus
#define EXPORT extern "C" __declspec (dllexport)
#else
#define EXPORT __declspec (dllexport)
#endif
#pragma warning(disable: 4995)
#pragma warning(disable: 4996)

#define PI 3.1415926535L
#define pi 3.1415926535L
#define BI_JPEG 4L


// -------- typedefs ------------
typedef struct
{
	long         Breite;
	long         Hoehe;
	double       SV;    // Seitenverhaeltnis
	BYTE         * Daten;
	BYTE		 * Filepointer;
	WORD         Farbmodus;
	double       Skalierung;
	WORD         PaletteEntries;
	long         PseudoBreite;
	BITMAPINFOHEADER   bmih;
	RGBQUAD      Fabb [256];
	long         Datenlaenge;
	BYTE		 TiffByteTausch;
	BYTE		 TiffOrientation;
} BILD;

struct Punkti
{
	short		x;
	short		y;
	short		R;		   // rot
	short		G;		   // gruen
	short		B;		   // blau
	short		Y;		   // Helligkeit
} ;
//--------------
struct Punkt
{
	double       x_re;     // x real
	double       y_re;     // y real
	double       R;		   // rot
	double       G;		   // gruen
	double       B;		   // blau
	double       Y;		   // Helligkeit
	double       I;		   // Farbe
	double       Q;		   // Farbe
	double       U;		   // Farbe
	double       V;		   // Farbe
	double       A;        // Alpha - oder benutzt für 16-bit-Kram
} ;
//--------------
struct Gerade
{
	Punkt      Ort;          // Ortsvektor
	Punkt      Richtung;     // Richtungsvektor
};
//--------------
struct Geometrie
{
	Punkt    A, B, C, D, M;
    Gerade   a, b, c, d, e, f;
	double   AC, AM, MC, BD, BM, MD;
} ;  // beschreibt die Lage eines Eichrahmens bzw. seines Bildes


// ------------- Funktionsdeklarationen ------------------
EXPORT BOOL   hli_init (HWND hwnd);
EXPORT void   Melde (char * Text, COLORREF Fabb, BOOL loeschen);
EXPORT BOOL   ShowBmp (HWND hwnd, BILD * pBild, long Nr, short Anz, BOOL Melden);
EXPORT BOOL   ShowBmpQuick (HWND hwnd, BILD * pBild);
EXPORT BOOL   ScaleBmp (HWND hwnd, BILD * pBild);
EXPORT BOOL   ShowBmpShift (HWND hwnd, BILD * pBild, short Nr, short Anz, BOOL Melden, long x_shift);
EXPORT BOOL   DateinameOeffnen (HWND hwnd, OPENFILENAME * pofn);
EXPORT BOOL   DateinameOeffnenAvi (HWND hwnd, OPENFILENAME * pofn);
EXPORT BOOL   DateinameOeffnenTxt (HWND hwnd, OPENFILENAME * pofn);
EXPORT BOOL   DateinameOeffnenIni (HWND hwnd, OPENFILENAME * pofn);
EXPORT BOOL   DateinameSpeichern (HWND hwnd, OPENFILENAME * pofn);
EXPORT BOOL   LoadBild (BILD * pBild, LPTSTR Dateiname, BOOL Seq);
EXPORT BOOL   LoadBmp  (BILD * pBild, LPTSTR BmpName, BOOL Seq);
EXPORT BOOL   LoadTiff (BILD * pBild, LPTSTR Dateiname, BOOL Seq);
EXPORT BOOL   SaveBmp  (BILD * pBild, LPTSTR Name);
EXPORT BOOL   SaveTiff  (BILD * pBild, LPTSTR Name);
EXPORT BOOL   SaveBild (BILD * pBild, LPTSTR Name, int jpegProzent);
EXPORT BOOL   LinkeTaste (HWND hwnd, BILD * pBild, LONG lParam, Punkt * P_MPunkt);
EXPORT void   Wertanzeige (Punkt P);
EXPORT Punkt  PunktHolenInt  (BILD * pBild, long xl, long yl);  // bekommt Longkoordinaten!!
EXPORT BOOL   PunktHolenQuick  (BILD * pBild, long xl, long yl, Punkt* P);  // bekommt Longkoordinaten!!
EXPORT BOOL   PunktiHolenQuick  (BILD * pBild, long xl, long yl, Punkti* P);  // bekommt Longkoordinaten!!
EXPORT BOOL   PunktHolen16     (BILD * pBild, long xl, long yl, Punkt* P);  // bekommt Longkoordinaten!!
EXPORT Punkt  PunktHolen  (BILD * pBild, double xr, double yr); // bekommt Realkoordinaten!!
EXPORT BOOL   PunktHolenNeu  (BILD * pBild, double xr, double yr, Punkt* P, BOOL kubisch); // bekommt Realkoordinaten!!
EXPORT BOOL   NextPunktHolen (BILD* pBild, Punkt* P);     // holt den rechten Nachbarpunkt
EXPORT Punkt  PunktHolenEbene  (BILD * pBild, double xr, double yr); // bekommt Realkoordinaten!!
EXPORT Punkt  PunktHolenQuadrate  (BILD * pBild, double xr, double yr); // bekommt Realkoordinaten!!
EXPORT void   Kreuzchen (HWND hwnd, int x, int y, int size, long Farbe);
EXPORT void   Puenktchen (HWND hwnd, int x, int y, long Farbe);
EXPORT BOOL   GetRGB     (BILD * pBild, long x, long y, WORD * pr, WORD * pg, WORD * pb);
EXPORT BOOL   GetRGBA    (BILD * pBild, long x, long y, WORD * pr, WORD * pg, WORD * pb, WORD* pa);
EXPORT BOOL   GetNextRGB (BILD * pBild, WORD * pr, WORD * pg, WORD * pb);
EXPORT void   PunktSetzen  (BILD * pBild, long x, long y, Punkt * pP);
EXPORT void   PunktSetzenQuick (BILD * pBild, long x, long y, int R, int G, int B);
EXPORT void   PunktSetzenYUV  (BILD * pBild, long x, long y, Punkt * pP);
EXPORT long   BildInit (BILD * pZiel, long Breite, long Hoehe, WORD Farbmodus, WORD Entries, long Aufl);   // gibt "lengthDaten" zurueck
EXPORT long   BildInitBlack (BILD * pZiel, long Breite, long Hoehe, WORD Farbmodus, WORD Entries, long Aufl);   // gibt "lengthDaten" zurueck
EXPORT BOOL   Invertieren (BILD * pQuelle, BILD * pZiel);
EXPORT UINT   CapSave (BILD * pBild);
EXPORT void   CapSaveWin (HWND hwnd, BILD* pZiel);
EXPORT BOOL   CapSaveAVI (UINT seconds);
EXPORT BOOL   Bildgroesse  (HWND hwnd, BILD * pQuelle, BILD * pZiel, double Prozent_x, double Prozent_y);
EXPORT BOOL   Graubild (BILD * pQuelle, BILD * pZiel);
EXPORT BOOL   InsertFromClipboard (HWND hwnd, BILD * pZiel);
EXPORT BOOL   InsertTextFromClipboard (HWND hwnd, char* Text);
EXPORT BOOL   Asciiout (HWND hwnd, BILD * pQuelle);
EXPORT BOOL   Differenz (HWND hwnd, BILD * pQuelle1, BILD * pQuelle2, BILD * pZiel);
EXPORT BOOL   DifferenzStrecken (HWND hwnd, BILD * pQuelle1, BILD * pQuelle2, BILD * pZiel);
EXPORT double Kontrast (HWND hwnd, BILD * pQuelle, BILD * pZiel, BOOL modus);
EXPORT BOOL   Summe (BILD * pQuelle1, BILD * pQuelle2, BILD * pZiel);
EXPORT BOOL   RotGruen (BILD * pQuelle1, BILD * pQuelle2, BILD * pZiel);
EXPORT BOOL   RotGruenPlus (HWND hwnd, BILD * pQuelle1, BILD * pQuelle2, BILD * pZiel);
EXPORT double MischInit (HWND hwnd);
EXPORT BOOL   Mischen (HWND hwnd, BILD * pQuelle1, BILD * pQuelle2, BILD * pZiel, double Prozent);
EXPORT BOOL   Drehen (HWND hwnd, BILD * pQuelle, BILD * pZiel);
EXPORT BOOL   Drehen_90_180 (HWND hwnd, BILD * pQuelle, BILD * pZiel, int modus);
EXPORT BOOL   Spiegeln (HWND hwnd, BILD * pQuelle, BILD * pZiel);
EXPORT BOOL   Raender     (BILD * pQuelle, BILD * pZiel, Punkt * pP1, Punkt * pP2);
EXPORT BOOL   Raender2Klicks (BILD * pQuelle, BILD * pZiel, Punkt * pP1, Punkt * pP2);
EXPORT BOOL   Rand1Klick (BILD * pQuelle, BILD * pZiel, Punkt * pP1);
EXPORT BOOL   DefGerade (Punkt * pP1, Punkt * pP2, Gerade * pG);
EXPORT short  Schnitt (Gerade * pGer1, Gerade * pGer2, Punkt * pSchnitt);
EXPORT BOOL   Line (BILD * pBild, Punkt * pP1, Punkt * pP2, Punkt * pP3);
EXPORT BOOL   Rechteck (BILD * pBild, Punkt * pP1, Punkt * pP2, Punkt * pP3);
EXPORT BOOL   FillRechteck (BILD * pBild, Punkt * pP1, Punkt * pP2, Punkt * pP3);
EXPORT double Dist (Punkt * pP1, Punkt * pP2);
EXPORT BOOL   Ausserhalb (Punkt * pP, BILD * pBild);
EXPORT BOOL   MausHolen (HWND hwnd, BILD * pBild, char * Text, short Anz, Punkt * P_MPunkt);
EXPORT BOOL   MausHolenRechteck (HWND hwnd, BILD * pBild, char * Text, Punkt * pP1, Punkt * pP2);
EXPORT BOOL   MausHolenRechteckSV (HWND hwnd, BILD * pBild, char * Text, Punkt * pP1, Punkt * pP2);
EXPORT HWND   CaptureIni (HWND hwnd, short Modus, BOOL sichtbar);
EXPORT BOOL   CaptureExistenztest (HWND hwnd);
EXPORT BOOL   CaptureEinstellungen (HWND hwnd);
EXPORT BOOL   CaptureEinBild (HWND hwnd, BILD * pZiel, LPVIDEOHDR lpVHdr, BOOL AufrufAusCallback);
EXPORT LRESULT CALLBACK About( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam );
EXPORT void   drucke (long Zahl);
EXPORT void   drucke_nlf (long Zahl);  // no line feed
EXPORT void   druckef (long double Zahl);
EXPORT void   druckeline ();
EXPORT void   drucketext (char* Zeile);
EXPORT void   MeldWinCenter ();
EXPORT void   MeldWinOri ();
EXPORT BOOL   Eingabebox (HWND hwndOwner, int Anzahl, LPSTR lpszMessage, char Zeilen [][50]);
EXPORT BOOL   To24 (BILD* pQuelle, BILD* pZiel);
EXPORT void   BildCopy (BILD * pQuelle, BILD * pZiel);

// ------------  Globale Variablen -----------

char     Startpfad [100];   // der Pfad, aus dem heraus gestartet wird.

BILD     Max;        // enthaelt maximale Bildabmessungen
BILD     Meld;       // Abmessungen Meldungsfenster
BILD     Leer;       // leeres Bild;
BILD     Bild_1;     // das sichtbare Bild
BILD     Bild_2;     // das zuletzt sichtbare Bild
BILD     Bild_3;     // Zwischenspeicher 
BILD     Bild_4;     // Zwischenspeicher 
BILD     Bild_5;     // Zwischenspeicher 
BILD     Bild_6;     // Zwischenspeicher 
BILD     Bild_7;     // Zwischenspeicher 

Punkt P1;
Punkt Schwarz, Weiss, Grau, Rot, Blau, Gelb, Dunkelblau;  // mal auf Verdacht definiert

int Randx;
int Randy;                 // Fensterraender
long  Xdsize, Ydsize;      // Displaysize - Bildgroesse aufm Schirm

HINSTANCE            hInstDll;

//  ---- Für Datei-Operationen ---
//static OPENFILENAME  sfn;   // save-filename
static OPENFILENAME  osn;   // open-sequenzname
static OPENFILENAME  ssn;   // save-sequenzname
static char          of_szFile [256];
static char          sf_szFile [256];
static char          os_szFile [256];
static char          ss_szFile [256];
static char          szFileTitle [256];
static char          szFilter []={"BMP-Files\0*.bmp\0"};
static char          seqszFilter []={"BMP-Files\0*000.bmp\0"};
char                 Zeile         [100];
char                 DateinameGlob [500] = {0};
char                 Zeile1        [100];
static short         TypeRef='MB';

LPVIDEOHDR			 lpv;  // Für Video-Capture

