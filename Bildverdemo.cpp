//******************************************************
//            Bildverarbeitungs-Hauptprogramm
//******************************************************

//--------- includes -----------------------

#include "hli20.h"  // enthält weitere Includes
#include <cmath>    // für exp, M_PI
#include <algorithm>
using std::min;
using std::max;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// -------- Globale Variablen --------------

HINSTANCE  hInst;
HWND       hwndMain;
DWORD      BildZaehler;
short      Callbackfunktion;
short      CapCallbackNeuInit;
Punkt      MPunkt[5] = { '\0' };

char     Eingabezeilen[20][50];   // für Dialogboxen   (Anzahl 20, Länge 50)

BILD bild4;
BILD bild5;
int g_clickX = 0;
int g_clickY = 0;

// -------- Funktionsdeklarationen ---------

LRESULT CALLBACK WndProc(HWND, UINT, UINT, LONG);
LRESULT CALLBACK FrameCallbackProc(HWND hcwnd, LPVIDEOHDR lpVHdr); // kriegt capture window übergeben
BOOL BlueBox(BILD* pQuelle, BILD* pZiel);
BOOL Invertieren100(HWND hwnd, BILD* pQuelle, BILD* pZiel);
BOOL Gamma(HWND hwnd, BILD* pQuelle, BILD* pZiel);
BOOL HelligkeitsProfil(HWND hwnd, BILD* pQuelle, BILD* pZiel);
BOOL LineareTransformation(HWND hwnd, BILD* pQuelle, BILD* pZiel, BOOL manuell);
BOOL GaussFilter(HWND hwnd, BILD* pQuelle, BILD* pZiel);
BOOL GaussOptimiert(HWND hwnd, BILD* pQuelle, BILD* pZiel);
BOOL GaussOptimiertReichweite(HWND hwnd, BILD* pQuelle, BILD* pZiel, int radius);

Punkt Bilinear(BILD* src, double x, double y);
BOOL ZoomBicubic(HWND hwnd, BILD* src, BILD* dst, int cx, int cy);
double cubicWeight(double t);
Punkt Bicubic(BILD* src, double x, double y);
//-------------------------------------------------------------------------
//                    WinMain   Funktion
//-------------------------------------------------------------------------

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR CmdLine, int cmdShow)
{
	MSG      msg;
	WNDCLASS wndclass;
	HMENU    hMenu = NULL;

	if (!hPrevInstance)
	{
		wndclass.style = CS_HREDRAW | CS_VREDRAW;
		wndclass.lpfnWndProc = WndProc;
		wndclass.cbClsExtra = 0;
		wndclass.cbWndExtra = 0;
		wndclass.hInstance = hInstance;
		wndclass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
		wndclass.hCursor = LoadCursor((HINSTANCE)NULL, IDC_CROSS);
		wndclass.hbrBackground = (HBRUSH)GetStockObject(DKGRAY_BRUSH);
		wndclass.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
		wndclass.lpszClassName = "MAIN";

		if (!RegisterClass(&wndclass))
		{
			MessageBox(NULL, "Kann Fensterklasse nicht registrieren", "Mist", MB_ICONERROR);
			return FALSE;
		}

	}

	hInst = hInstance;

	// --- Fenster erzeugen - erstmal irgendwelche Abmessungen ---

	hwndMain = CreateWindow("MAIN",
		"Bildverarbeitung",
		WS_OVERLAPPEDWINDOW,
		0,
		0,
		500,
		100,
		(HWND)NULL,
		hMenu,
		hInst,
		(LPVOID)NULL
	);
	if (!hwndMain) return FALSE;

	ShowWindow(hwndMain, cmdShow);
	UpdateWindow(hwndMain);

	// -------------- Initialisierungen ---------------

	hli_init(hwndMain);

	// --------- Startbild ---------------

	if ((LoadBild(&Bild_1, (strlen(CmdLine) > 0) ? CmdLine : "Start.jpg", FALSE))
		|| (LoadBild(&Bild_1, (strlen(CmdLine) > 0) ? CmdLine : "..//Start.jpg", FALSE)))
	{
		ShowBmp(hwndMain, &Bild_1, 0, 0, TRUE);
		SetWindowText(hwndMain, "       Bildverarbeitung ");
		if (strlen(CmdLine) == 0)
			Melde("Willkommen in der Bildverarbeitung !", 0x000FF, TRUE);
	}
	else
		Melde("Start.jpg nicht gefunden !", 0x00FFFF, FALSE);

	while (GetMessage(&msg, NULL, 0, 0))
	{

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

//-------------------------------------------------------------------------
//                       Menu-Auswertung   
//-------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hwnd, UINT wMsg,
	UINT wParam, LONG lParam)
{
	static OPENFILENAME ofn;
	static HWND hCaptureWin;
	double Prozent_x, Prozent_y; // für Bildgröße
	static BOOL Meldungen_an = 1;

	switch (wMsg)
	{
	case WM_RBUTTONDOWN:
		if (Bild_2.Daten != NULL)
		{
			ShowBmp(hwnd, &Bild_2, 0, 0, TRUE);
			// Tauschen der Bilder...
			Bild_3 = Bild_2; Bild_2 = Bild_1; Bild_1 = Bild_3;
		}
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
		break;

	case WM_LBUTTONDOWN:    // linke Maustaste
	{
		// 🔥 Klickposition speichern (für Zoom!)
		g_clickX = LOWORD(lParam);
		g_clickY = HIWORD(lParam);

		// bestehende Funktion weiter aufrufen
		LinkeTaste(hwnd, &Bild_1, lParam, MPunkt);
	}
	break;

	case WM_PAINT:			 // neuzeichnen
		if (Bild_1.Daten != NULL)
			ShowBmp(hwnd, &Bild_1, 0, 0, TRUE);
		break;

	case WM_COMMAND:
		switch (wParam)
		{
		case IDM_EXIT:
			DestroyWindow(hwnd);
			break;

		case IDM_FILE_OPEN:    // --------- Datei laden ----------
			if (DateinameOeffnen(hwnd, &ofn))
			{
				if (Bild_1.Daten == NULL)  // erstes Bild
				{
					LoadBild(&Bild_1, ofn.lpstrFile, FALSE);
					if (Bild_1.Daten != NULL)
					{
						ShowBmp(hwnd, &Bild_1, 0, 0, TRUE);
					}
				}
				else                        // zweites Bild
				{
					LoadBild(&Bild_2, ofn.lpstrFile, FALSE);
					if (Bild_2.Daten != NULL)
						ShowBmp(hwnd, &Bild_2, 0, 0, TRUE);
					// Tauschen der Bilder...
					Bild_3 = Bild_2; Bild_2 = Bild_1; Bild_1 = Bild_3;
				}
			}
			break;

		case IDM_FILE_SAVE:     // -------- Datei speichern ----
			if (Bild_1.Daten != NULL)
			{
				if (DateinameSpeichern(hwnd, &ofn))  // überschreiben-Test wird vom System durchgeführt.
					SaveBild(&Bild_1, ofn.lpstrFile, 80);
			}
			break;


		case IDM_EDIT_UNDO:
			if (Bild_2.Daten != NULL)
			{
				ShowBmp(hwnd, &Bild_2, 0, 0, TRUE);
				// Tauschen der Bilder...
				Bild_3 = Bild_2; Bild_2 = Bild_1; Bild_1 = Bild_3;
			}
			break;

		case IDM_EDIT_INSERT:    // --------- Datei aus Zwischenablage ----------
			if (Bild_1.Daten == NULL)  // erstes Bild
			{
				if (InsertFromClipboard(hwnd, &Bild_1))
				{
					if (Bild_1.Daten != NULL)
					{
						ShowBmp(hwnd, &Bild_1, 0, 0, TRUE);
					}
				}
				else Melde("ClipboardError", 0x00FFFF, FALSE);
			}
			else                        // zweites Bild
			{
				if (InsertFromClipboard(hwnd, &Bild_2))
				{
					if (Bild_2.Daten != NULL)
						ShowBmp(hwnd, &Bild_2, 0, 0, TRUE);
					// Tauschen der Bilder...
					Bild_3 = Bild_2; Bild_2 = Bild_1; Bild_1 = Bild_3;
				}
				else Melde("ClipboardError", 0x00FFFF, FALSE);
			}
			break;



		case IDM_EDIT_INVERT:       // -------- Invertieren
			if (Bild_1.Daten != NULL)
			{
				Invertieren(&Bild_1, &Bild_2);
				if (Bild_2.Daten != NULL)
				{
					ShowBmp(hwnd, &Bild_2, 0, 0, TRUE);
					// Tauschen der Bilder...
					Bild_3 = Bild_2; Bild_2 = Bild_1; Bild_1 = Bild_3;
				}
			}
			break;



		case IDM_EDIT_INVERT_100:       // -------- Invertieren eines Bereichs 100 x 100
			if (Bild_1.Daten != NULL)
			{
				if (!Invertieren100(hwnd, &Bild_1, &Bild_2))
					break;
				if (Bild_2.Daten != NULL)
				{
					ShowBmp(hwnd, &Bild_2, 0, 0, TRUE);
					// Tauschen der Bilder...
					Bild_3 = Bild_2; Bild_2 = Bild_1; Bild_1 = Bild_3;
				}
			}
			break;


		case IDM_GAMMA:       // -------- Testfunktion
			if (Bild_1.Daten != NULL)
			{
				Gamma(hwnd, &Bild_1, &Bild_2);
				if (Bild_2.Daten != NULL)
				{
					ShowBmp(hwnd, &Bild_2, 0, 0, TRUE);
					// Tauschen der Bilder...
					Bild_3 = Bild_2; Bild_2 = Bild_1; Bild_1 = Bild_3;
				}
			}
			break;



		case IDM_EDIT_GROESSE:       // -------- Bildgröße
			if (Bild_1.Daten != NULL)
			{
				sprintf(Eingabezeilen[0], "50");  // erstmal Bildgröße erfragen
				sprintf(Eingabezeilen[1], "50");  // erstmal Bildgröße erfragen

				if (Eingabebox(hwnd, 2,
					"Neue Breite in Prozent der alten:& Neue Höhe in Prozent der alten:",
					Eingabezeilen))
				{
					sscanf(Eingabezeilen[0], "%lf", &Prozent_x);
					sscanf(Eingabezeilen[1], "%lf", &Prozent_y);
				}
				else
				{
					Melde("Abbruch", 0xFFFFFF, FALSE);
					break;
				}
				if ((min(Prozent_x, Prozent_y) < 5)
					|| (max(Prozent_x, Prozent_y) > 1000))
				{
					Melde("Versuchen Sie's mal zwischen 5 und 1000", 0x00FFFF, FALSE);
					break;
				}
				if (Bildgroesse(hwnd, &Bild_1, &Bild_2, Prozent_x, Prozent_y))  // false: nicht einfach nur halbieren
				{
					ShowBmp(hwnd, &Bild_2, 0, 0, TRUE);
					// Tauschen der Bilder...
					Bild_3 = Bild_2; Bild_2 = Bild_1; Bild_1 = Bild_3;
				}
			}
			break;


		case IDM_ABOUT:  // ------------- About-Box
			DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hwnd, (DLGPROC)About);
			break;

		case IDM_MELD_ONOFF:
			Melde(" ", Meldungen_an = 1 - Meldungen_an, FALSE);   // Farbe 0 oder 1 als Schalter
			break;

		case ID_PRAKTIKUM_HELLIGKEIT:   // <- hier deine Resource-ID
			if (Bild_1.Daten != NULL) {
				// Aufruf der Helligkeitsprofil-Funktion
				if (HelligkeitsProfil(hwndMain, &Bild_1, &Bild_2)) {
					ShowBmp(hwndMain, &Bild_2, 0, 0, TRUE);
					// Bilder tauschen für Undo-Funktion
					Bild_3 = Bild_2;
					Bild_2 = Bild_1;
					Bild_1 = Bild_3;
				}
			}
			break;

		case ID_PRAKTIKUM_LINEARETRANSFORMATION: // Automatisch
			if (Bild_1.Daten != NULL) {
				if (LineareTransformation(hwnd, &Bild_1, &Bild_2, TRUE)) {
					ShowBmp(hwnd, &Bild_2, 0, 0, TRUE);
					Bild_3 = Bild_2; Bild_2 = Bild_1; Bild_1 = Bild_3;
				}
			}
			break;

		case ID_PRAKTIKUM_LINEARETRANSFORMATION40024: // Manuell
			if (Bild_1.Daten != NULL) {
				if (LineareTransformation(hwnd, &Bild_1, &Bild_2, FALSE)) {
					ShowBmp(hwnd, &Bild_2, 0, 0, TRUE);
					Bild_3 = Bild_2; Bild_2 = Bild_1; Bild_1 = Bild_3;
				}
			}
			break;

		case ID_FILTER_GAUSS:
			if (Bild_1.Daten != NULL) {
				DWORD start = GetTickCount(); // Zeitmessung Start

				if (GaussFilter(hwnd, &Bild_1, &Bild_2)) { // Deine Funktion aus a)
					DWORD dauer = GetTickCount() - start;

					char msg[100];
					sprintf(msg, "Einfacher Filter fertig in %ld ms", dauer);
					Melde(msg, 0x00FFFF, FALSE);

					ShowBmp(hwnd, &Bild_2, 0, 0, TRUE);
					Bild_3 = Bild_2; Bild_2 = Bild_1; Bild_1 = Bild_3;
				}
			}
			break;

		case ID_FILTER_GAUSS_OPTIMIERT:
			if (Bild_1.Daten != NULL) {
				DWORD start = GetTickCount();

				if (GaussOptimiert(hwnd, &Bild_1, &Bild_2)) { // Die neue optimierte Funktion
					DWORD dauer = GetTickCount() - start;

					char msg[100];
					sprintf(msg, "Optimierter Filter fertig in %ld ms", dauer);
					Melde(msg, 0x00FF00, FALSE);

					ShowBmp(hwnd, &Bild_2, 0, 0, TRUE);
					Bild_3 = Bild_2; Bild_2 = Bild_1; Bild_1 = Bild_3;
				}
			}
			break;

		case ID_FILTER_GAUSS_OPTIMIERT_REICHWEITE: // Optimierter Filter
			if (Bild_1.Daten != NULL) {

				int radius;

				// Standardwert setzen
				sprintf(Eingabezeilen[0], "2");

				// Eingabebox anzeigen
				if (!Eingabebox(hwnd, 1, "Radius des Gaussfilters eingeben:", Eingabezeilen)) {
					Melde("Abbruch", 0xFFFFFF, FALSE);
					break;
				}

				sscanf(Eingabezeilen[0], "%d", &radius);

				// Sicherheitscheck
				if (radius < 1 || radius > 20) {
					Melde("Radius bitte zwischen 1 und 20 wählen!", 0x00FFFF, FALSE);
					break;
				}

				DWORD t1 = GetTickCount();

				GaussOptimiertReichweite(hwnd, &Bild_1, &Bild_2, radius);

				DWORD t2 = GetTickCount();

				char buf[100];
				sprintf(buf, "Optimiert (r=%d): %ld ms", radius, t2 - t1);
				Melde(buf, 0x00FF00, FALSE);

				ShowBmp(hwnd, &Bild_2, 0, 0, TRUE);
				Bild_3 = Bild_2; Bild_2 = Bild_1; Bild_1 = Bild_3;
			}
			break;

		case ID_INTERPOLATION_BICUBIC:
		{
			if (Bild_1.Daten == NULL) break;

			if (g_clickX == 0 && g_clickY == 0)
			{
				Melde("Bitte zuerst ins Bild klicken!", 0x00FFFF, FALSE);
				break;
			}

			BildInit(&Bild_2, Bild_1.Breite, Bild_1.Hoehe, 24, 0, 1000);

			ZoomBicubic(hwnd, &Bild_1, &Bild_2, g_clickX, g_clickY);

			ShowBmp(hwnd, &Bild_2, 0, 0, TRUE);
		}
		break;

		case ID_INTERPOLATION_BILINEAR:
		{
			if (Bild_1.Daten == NULL) break;

			BildInit(&Bild_2,
				Bild_1.Breite,
				Bild_1.Hoehe,
				24, 0, 1000);

			double angle = M_PI / 4.0; // 45 Grad
			double cosA = cos(angle);
			double sinA = sin(angle);

			double cx = Bild_1.Breite / 2.0;
			double cy = Bild_1.Hoehe / 2.0;

			for (int y = 0; y < Bild_2.Hoehe; y++)
			{
				for (int x = 0; x < Bild_2.Breite; x++)
				{
					// Mittelpunktverschiebung
					double dx = x - cx;
					double dy = y - cy;

					// 🔥 inverse Rotation (Ziel -> Quelle)
					double srcX = cosA * dx + sinA * dy + cx;
					double srcY = -sinA * dx + cosA * dy + cy;

					Punkt P;

					if (srcX < 0 || srcY < 0 ||
						srcX >= Bild_1.Breite - 1 ||
						srcY >= Bild_1.Hoehe - 1)
					{
						P.R = P.G = P.B = 0; // Hintergrund schwarz
					}
					else
					{
						P = Bilinear(&Bild_1, srcX, srcY);
					}

					PunktSetzen(&Bild_2, x, y, &P);
				}
			}

			ShowBmp(hwnd, &Bild_2, 0, 0, TRUE);

			Bild_3 = Bild_2;
			Bild_2 = Bild_1;
			Bild_1 = Bild_3;
		}
		break;

		} // Ende von: Windows-Command (Menü, switch wParam)

	}
	return DefWindowProc(hwnd, wMsg, wParam, lParam);
}


//************************************************************************

// FrameCallbackProc: Frame Callback Funktion 
// Aufgerufen, wenn neuer Frame im Buffer (außer bei Streaming) 
// 
LRESULT CALLBACK FrameCallbackProc(HWND hcwnd, LPVIDEOHDR lpVHdr) // kriegt capture window übergeben
{
	//	DWORD i;
	//	BYTE  lesbyte;

	if (!hcwnd)
		return FALSE;

	TCHAR Zeile[100];
	wsprintf(Zeile, TEXT("Preview frame# %ld "), BildZaehler++);

	SetWindowText(hcwnd, (LPTSTR)Zeile);

	CaptureEinBild(hcwnd, &Bild_2, lpVHdr, TRUE);  // true: Aufruf aus Callback

	switch (Callbackfunktion)
	{
	case 1:
		Invertieren(&Bild_2, &Bild_1);
		break;
	case 2:
		BlueBox(&Bild_2, &Bild_1);
		break;
	}
	CapCallbackNeuInit = FALSE;
	ShowBmp(hwndMain, &Bild_1, 0, 0, FALSE);
	//Bild_3 = Bild_2; Bild_2 = Bild_1; Bild_1 = Bild_3;

	return (LRESULT)TRUE;
}

// ---------------------------------------------------------------------
//                  Einfache BlueBox-Routine
// ---------------------------------------------------------------------

BOOL BlueBox(BILD* pQuelle, BILD* pZiel)
{
	long  x, y;
	Punkt P;


	// ------  neues Bild initialisieren !!  ------

	BildInit(pZiel, pQuelle->Breite, pQuelle->Hoehe, 24, 0, 1000);   // bitte nur 24-bit-Bilder erzeugen!

  // ------- so, nun Bearbeitung ----------------

	for (y = 0; y < pZiel->Hoehe; y++)
		for (x = 0; x < pZiel->Breite; x++)
		{
			P = PunktHolenInt(pQuelle, x, y);

			if ((P.B > P.R + 10) && (P.B > P.G + 10))
				P = Schwarz;

			PunktSetzen(pZiel, x, y, &P);
		}

	return (TRUE);
}

// -------------------------------------------------------------------------
// Invertieren eines 100 x 100 Pixel großen Bereichs rund um Mausklick
// Blöde Aufgabe, aber einfaches Beispiel für Maus-abhängige Bearbeitung
// -------------------------------------------------------------------------

BOOL Invertieren100(HWND hwnd, BILD* pQuelle, BILD* pZiel)
{
	long  x, y;
	Punkt P;

	// ------- Maus-Abfrage. Abbruch bei rechtem Mausklick

	if (MausHolen(hwnd, pQuelle, "Bitte einen Punkt anklicken", 1, MPunkt) == FALSE)
	{
		Melde("Abbruch", 0xFFFFFF, FALSE);
		return (FALSE);
	}
	// ------  neues Bild initialisieren !!  ------
	BildInit(pZiel, pQuelle->Breite, pQuelle->Hoehe, 24, 0, 1000);   // bitte nur 24-bit-Bilder erzeugen!

  // ------- so, nun Bearbeitung - Bild punktweise abrastern ------

	for (y = 0; y < pZiel->Hoehe; y++)
		for (x = 0; x < pZiel->Breite; x++)
		{
			P = PunktHolenInt(pQuelle, x, y);  // einen Punkt aus Bild holen
			if ((abs(x - (int)MPunkt[0].x_re) < 100) && (abs(y - (int)MPunkt[0].y_re) < 100))
			{
				P.R = 255.0 - P.R;    // invertieren, wenn in Bereich um Mauspunkt
				P.G = 255.0 - P.G;
				P.B = 255.0 - P.B;
			}
			PunktSetzen(pZiel, x, y, &P);
		}
	return (TRUE);
}
// ------------------------------------------------------------------
// ------------------------------------------------------------------
// ------------------------------------------------------------------

BOOL Gamma(HWND hwnd, BILD* pQuelle, BILD* pZiel)
{
	long x, y;
	Punkt P;
	int H;
	double Hneu;

	BildInit(pZiel, (pQuelle->Breite) * 2, (pQuelle->Hoehe) * 2, 24, 0, 1000);   // bitte nur 24-bit-Bilder erzeugen!

	for (y = 0; y < pQuelle->Hoehe; y++)
		for (x = 0; x < pQuelle->Breite; x++)
		{
			P = PunktHolenInt(pQuelle, x, y);
			H = (int)max(P.R, max(P.G, P.B));

			// Gamma = 0.5
			Hneu = (255 / (pow(255, 0.5))) * pow(H, 0.5);

			P.R = Hneu;
			P.G = Hneu;
			P.B = Hneu;

			PunktSetzen(pZiel, x, y + pQuelle->Hoehe, &P);

			//Gamma = 0.7
			Hneu = (255 / (pow(255, 0.7))) * pow(H, 0.7);

			P.R = Hneu;
			P.G = Hneu;
			P.B = Hneu;

			PunktSetzen(pZiel, x + pQuelle->Breite, y + pQuelle->Hoehe, &P);

			//Gamma = 1.0
			Hneu = H;

			P.R = Hneu;
			P.G = Hneu;
			P.B = Hneu;

			PunktSetzen(pZiel, x, y, &P);

			//Gamma = 2.0
			Hneu = (255 / (pow(255, 2.0))) * pow(H, 2.0);

			P.R = Hneu;
			P.G = Hneu;
			P.B = Hneu;

			PunktSetzen(pZiel, x + pQuelle->Breite, y, &P);
		}

	return(TRUE);
}

// ------------------------------------------------------------
// Funktion: HelligkeitsProfil
// Aufgabe:
//  - Wartet auf einen Mausklick im Bild
//  - Linksklick:
//        * kopiert das Bild
//        * zeichnet eine horizontale Linie an der Klickposition
//        * zeichnet das Helligkeitsprofil entlang dieser Linie
//  - Rechtsklick:
//        * beendet die Funktion
// ------------------------------------------------------------

BOOL HelligkeitsProfil(HWND hwnd, BILD* pQuelle, BILD* pZiel)
{
	long x;
	Punkt P;
	int grauwert;
	int profilY; // Deklaration außerhalb der Schleife, um C2374 zu vermeiden

	while (true)
	{
		if (MausHolen(hwnd, pQuelle, "Linksklick: Profil | Rechtsklick: Ende", 1, MPunkt) == FALSE)
			return TRUE;

		BildInit(pZiel, pQuelle->Breite, pQuelle->Hoehe, 24, 0, 1000);
		BildCopy(pQuelle, pZiel);

		int klickY = (int)MPunkt[0].y_re;
		if (klickY < 0) klickY = 0;
		if (klickY >= pQuelle->Hoehe) klickY = pQuelle->Hoehe - 1;

		for (x = 0; x < pQuelle->Breite; x++)
		{
			P = PunktHolenInt(pQuelle, x, klickY);
			grauwert = (int)((P.R + P.G + P.B) / 3.0);

			// LOGIK: Hell (255) soll nach oben ausschlagen (Y klein)
			// Dunkel (0) soll unten bleiben (Y groß)
			profilY = (int)(grauwert * 0.8);

			if (profilY < 0) profilY = 0;
			if (profilY >= pQuelle->Hoehe) profilY = pQuelle->Hoehe - 1;

			// Rote Referenzlinie
			Punkt Linie = { 255, 0, 0 };
			PunktSetzen(pZiel, x, klickY, &Linie);

			// Blaue Profilkurve
			for (int dy = -1; dy <= 1; dy++)
			{
				int yPlot = profilY + dy;
				if (yPlot >= 0 && yPlot < pQuelle->Hoehe)
				{
					Punkt Profil = { 0, 0, 255 };
					PunktSetzen(pZiel, x, yPlot, &Profil);
				}
			}
		}
		ShowBmp(hwnd, pZiel, 0, 0, TRUE);
	}
	return TRUE;
}


// -------------------------------------------------------------------------
// Aufgabe 2: Lineare Grauwerttransformation (Kontraststreckung)
// -------------------------------------------------------------------------

BOOL LineareTransformation(HWND hwnd, BILD* pQuelle, BILD* pZiel, BOOL automatisch)
{
	long x, y;
	Punkt P;
	double H_min = 255.0, H_max = 0.0;
	double H;

	// 1. Schritt: Grenzen bestimmen
	if (automatisch)
	{
		// Automatische Suche nach min und max Helligkeit im Bild
		for (y = 0; y < pQuelle->Hoehe; y++)
		{
			for (x = 0; x < pQuelle->Breite; x++)
			{
				P = PunktHolenInt(pQuelle, x, y);
				H = max(P.R, max(P.G, P.B)); // Helligkeit als Max der Kanäle
				if (H < H_min) H_min = H;
				if (H > H_max) H_max = H;
			}
		}
	}
	else
	{
		// Manuelles Anklicken von zwei Schwellwerten
		if (MausHolen(hwnd, pQuelle, "Klicken Sie auf den DUNKLEN Schwellwert (Min)", 1, MPunkt) == FALSE) return FALSE;
		P = PunktHolenInt(pQuelle, (long)MPunkt[0].x_re, (long)MPunkt[0].y_re);
		H_min = max(P.R, max(P.G, P.B));

		if (MausHolen(hwnd, pQuelle, "Klicken Sie auf den HELLEN Schwellwert (Max)", 1, MPunkt) == FALSE) return FALSE;
		P = PunktHolenInt(pQuelle, (long)MPunkt[0].x_re, (long)MPunkt[0].y_re);
		H_max = max(P.R, max(P.G, P.B));

		// Sicherstellen, dass Min < Max ist
		if (H_min >= H_max) {
			Melde("Min muss kleiner als Max sein!", 0x00FFFF, FALSE);
			return FALSE;
		}
	}

	// 2. Schritt: Bild transformieren
	BildInit(pZiel, pQuelle->Breite, pQuelle->Hoehe, 24, 0, 1000);

	double bereich = H_max - H_min;
	if (bereich <= 0) bereich = 1; // Division durch Null verhindern

	for (y = 0; y < pQuelle->Hoehe; y++)
	{
		for (x = 0; x < pQuelle->Breite; x++)
		{
			P = PunktHolenInt(pQuelle, x, y);

			// Lineare Skalierung für jeden Kanal:
			// P_neu = (P_alt - H_min) * (255 / (H_max - H_min))

			P.R = (P.R - H_min) * (255.0 / bereich);
			P.G = (P.G - H_min) * (255.0 / bereich);
			P.B = (P.B - H_min) * (255.0 / bereich);

			// Clipping (Werte auf 0-255 begrenzen)
			P.R = max(0.0, min(255.0, P.R));
			P.G = max(0.0, min(255.0, P.G));
			P.B = max(0.0, min(255.0, P.B));

			PunktSetzen(pZiel, x, y, &P);
		}
	}

	return TRUE;
}

BOOL GaussFilter(HWND hwnd, BILD* pQuelle, BILD* pZiel)
{
	long x, y, k;
	Punkt P;

	int width = pQuelle->Breite;
	int height = pQuelle->Hoehe;

	BildInit(pZiel, width, height, 24, 0, 1000);

	struct RGBBuffer {
		double R, G, B;
	};

	RGBBuffer* Zeile1 = new RGBBuffer[width]();
	RGBBuffer* Zeile2 = new RGBBuffer[width]();
	RGBBuffer* Zeile3 = new RGBBuffer[width]();

	const int w1 = 1, w2 = 2, w3 = 1;
	const double Nenner = 16.0;

	auto GrauWert = [](const Punkt& p) -> double {
		return 0.299 * p.R + 0.587 * p.G + 0.114 * p.B;
	};

	// =========================================================
	// INITIALISIERUNG (KORRIGIERT: Zeile1 war vorher uninitialisiert!)
	// =========================================================
	for (k = 0; k < width; k++)
	{
		P = PunktHolenInt(pQuelle, k, 0);
		Zeile1[k].R = Zeile1[k].G = Zeile1[k].B = GrauWert(P);
	}

	for (k = 0; k < width; k++)
	{
		P = PunktHolenInt(pQuelle, k, 0);
		Zeile2[k].R = Zeile2[k].G = Zeile2[k].B = GrauWert(P);
	}

	for (k = 0; k < width; k++)
	{
		P = PunktHolenInt(pQuelle, k, 1);
		Zeile3[k].R = Zeile3[k].G = Zeile3[k].B = GrauWert(P);
	}

	// =========================================================
	// HAUPTSCHLEIFE
	// =========================================================
	for (y = 1; y < height - 1; y++)
	{
		RGBBuffer* temp = Zeile1;
		Zeile1 = Zeile2;
		Zeile2 = Zeile3;
		Zeile3 = temp;

		for (k = 0; k < width; k++)
		{
			P = PunktHolenInt(pQuelle, k, y + 1);
			double g = GrauWert(P);
			Zeile3[k].R = Zeile3[k].G = Zeile3[k].B = g;
		}

		for (x = 1; x < width - 1; x++)
		{
			double v_l =
				Zeile1[x - 1].R * w1 + Zeile2[x - 1].R * w2 + Zeile3[x - 1].R * w1;

			double v_m =
				Zeile1[x].R * w1 + Zeile2[x].R * w2 + Zeile3[x].R * w1;

			double v_r =
				Zeile1[x + 1].R * w1 + Zeile2[x + 1].R * w2 + Zeile3[x + 1].R * w1;

			double result = (v_l * w1 + v_m * w2 + v_r * w1) / Nenner;

			unsigned char Grau = (unsigned char)(result + 0.5);

			Punkt P_neu;
			P_neu.R = P_neu.G = P_neu.B = Grau;

			PunktSetzen(pZiel, x, y, &P_neu);
		}
	}

	delete[] Zeile1;
	delete[] Zeile2;
	delete[] Zeile3;

	// =========================================================
	// SICHERN
	// =========================================================
	BildInit(&bild5, width, height, 24, 0, 1000);
	BildCopy(pZiel, &bild5);

	return TRUE;
}
BOOL GaussOptimiert(HWND hwnd, BILD* pQuelle, BILD* pZiel)
{
	DWORD start = GetTickCount();

	long x, y, k, j, xa, ya, xe, ye, xa1;
	Punkt P;

	xa = 0;
	ya = 0;
	xe = pQuelle->Breite - 1;
	ye = pQuelle->Hoehe - 1;

	// WICHTIG: KEIN BildCopy -> sonst andere Ausgangsbasis als Referenz
	BildInit(pZiel, pQuelle->Breite, pQuelle->Hoehe, 24, 0, 1000);

	double* Zeile1 = new double[pQuelle->Breite]();
	double* Zeile2 = new double[pQuelle->Breite]();
	double* Zeile3 = new double[pQuelle->Breite]();

	auto GrauWert = [](const Punkt& p) -> double {
		return 0.299 * p.R + 0.587 * p.G + 0.114 * p.B;
	};

	// ---------------------------------------------------------
	// Initialisierung (IDENTISCH zur Referenz!)
	// ---------------------------------------------------------
	for (k = xa; k <= xe; k++)
	{
		P = PunktHolenInt(pQuelle, k, 0);
		Zeile2[k] = GrauWert(P);
	}

	for (k = xa; k <= xe; k++)
	{
		P = PunktHolenInt(pQuelle, k, 1);
		Zeile3[k] = GrauWert(P);
	}

	// ---------------------------------------------------------
	// Hauptschleife
	// ---------------------------------------------------------
	for (j = ya + 1; j <= ye - 1; j++)
	{
		xa1 = xa + 1;

		double* temp = Zeile1;
		Zeile1 = Zeile2;
		Zeile2 = Zeile3;
		Zeile3 = temp;

		P = PunktHolenInt(pQuelle, xa, j + 1);
		Zeile3[xa] = GrauWert(P);

		P = PunktHolenInt(pQuelle, xa1, j + 1);
		Zeile3[xa1] = GrauWert(P);

		double s2 = Zeile1[xa] + 2.0 * Zeile2[xa] + Zeile3[xa];
		double s3 = Zeile1[xa1] + 2.0 * Zeile2[xa1] + Zeile3[xa1];

		for (k = xa + 1; k <= xe - 1; k++)
		{
			P = PunktHolenInt(pQuelle, k + 1, j + 1);
			Zeile3[k + 1] = GrauWert(P);

			double s1 = s2;
			s2 = s3;

			s3 = Zeile1[k + 1] + 2.0 * Zeile2[k + 1] + Zeile3[k + 1];

			double gjk = (s1 + 2.0 * s2 + s3) / 16.0;

			unsigned char Grau = (unsigned char)(gjk + 0.5);

			Punkt P_neu;
			P_neu.R = Grau;
			P_neu.G = Grau;
			P_neu.B = Grau;

			PunktSetzen(pZiel, k, j, &P_neu);
		}
	}

	delete[] Zeile1;
	delete[] Zeile2;
	delete[] Zeile3;

	// ---------------------------------------------------------
	// Ergebnis sichern
	// ---------------------------------------------------------
	BildInit(&bild4, pZiel->Breite, pZiel->Hoehe, 24, 0, 1000);
	BildCopy(pZiel, &bild4);

	DWORD dauer = GetTickCount() - start;

	char msg[200];
	sprintf(msg, "Optimierter Gauss fertig in %ld ms", dauer);
	Melde(msg, 0x00FF00, FALSE);

	// ---------------------------------------------------------
	// Vergleich bild4 vs bild5
	// ---------------------------------------------------------
	bool gleich = true;

	if (bild4.Breite != bild5.Breite || bild4.Hoehe != bild5.Hoehe)
	{
		gleich = false;
	}
	else
	{
		for (int y = 0; y < bild4.Hoehe && gleich; y++)
		{
			for (int x = 0; x < bild4.Breite; x++)
			{
				Punkt p4 = PunktHolenInt(&bild4, x, y);
				Punkt p5 = PunktHolenInt(&bild5, x, y);

				if (p4.R != p5.R || p4.G != p5.G || p4.B != p5.B)
				{
					gleich = false;
					break;
				}
			}
		}
	}

	if (gleich)
		Melde("Bild4 und Bild5 sind IDENTISCH", 0x00FF00, FALSE);
	else
		Melde("Bild4 und Bild5 unterscheiden sich!", 0x0000FF, FALSE);

	return TRUE;
}


BOOL GaussOptimiertReichweite(HWND hwnd, BILD* pQuelle, BILD* pZiel, int radius)
{
	DWORD start = GetTickCount();
	long x, y, k, j;
	Punkt P;

	int width = pQuelle->Breite;
	int height = pQuelle->Hoehe;

	// Arbeitsbereich (Ränder bleiben unangetastet)
	int xa = 0, ya = 0;
	int xe = width - 1;
	int ye = height - 1;

	BildInit(pZiel, width, height, 24, 0, 1000);
	BildCopy(pQuelle, pZiel); // Ränder kopieren

	// --- Gauß-Kernel berechnen ---
	int kernelSize = 2 * radius + 1;
	double* kernel = new double[kernelSize * kernelSize];
	double sigma = radius / 2.0;   // Faustregel
	double sum = 0.0;

	for (int dy = -radius; dy <= radius; dy++) {
		for (int dx = -radius; dx <= radius; dx++) {
			double value = exp(-(dx * dx + dy * dy) / (2.0 * sigma * sigma));
			kernel[(dy + radius) * kernelSize + (dx + radius)] = value;
			sum += value;
		}
	}

	// Normalisieren
	for (int i = 0; i < kernelSize * kernelSize; i++)
		kernel[i] /= sum;

	// --- Hauptschleife ---
	for (y = ya + radius; y <= ye - radius; y++) {
		for (x = xa + radius; x <= xe - radius; x++) {
			double g = 0.0;

			// Kernel über Nachbarpixel anwenden
			for (int dy = -radius; dy <= radius; dy++) {
				for (int dx = -radius; dx <= radius; dx++) {
					Punkt Ptmp = PunktHolenInt(pQuelle, x + dx, y + dy);
					double grau = (Ptmp.R + Ptmp.G + Ptmp.B) / 3.0;
					g += grau * kernel[(dy + radius) * kernelSize + (dx + radius)];
				}
			}

			Punkt Pneu;
			Pneu.R = Pneu.G = Pneu.B = (BYTE)g;
			PunktSetzen(pZiel, x, y, &Pneu);
		}
	}

	delete[] kernel;

	DWORD dauer = GetTickCount() - start;
	char msg[100];
	sprintf(msg, "Variabler Gauss fertig in %ld ms", dauer);
	Melde(msg, 0x00FF00, FALSE);

	return TRUE;
}

Punkt Bilinear(BILD* src, double x, double y)
{
	int x0 = (int)x;
	int y0 = (int)y;

	// Sicherstellen, dass wir nicht über den Rand hinauslesen
	x0 = max(0, min(x0, src->Breite - 2));
	y0 = max(0, min(y0, src->Hoehe - 2));

	int x1 = x0 + 1;
	int y1 = y0 + 1;

	double dx = x - x0;
	double dy = y - y0;

	Punkt p00 = PunktHolenInt(src, x0, y0);
	Punkt p10 = PunktHolenInt(src, x1, y0);
	Punkt p01 = PunktHolenInt(src, x0, y1);
	Punkt p11 = PunktHolenInt(src, x1, y1);

	Punkt P;

	P.R = (1 - dx) * (1 - dy) * p00.R + dx * (1 - dy) * p10.R + (1 - dx) * dy * p01.R + dx * dy * p11.R;
	P.G = (1 - dx) * (1 - dy) * p00.G + dx * (1 - dy) * p10.G + (1 - dx) * dy * p01.G + dx * dy * p11.G;
	P.B = (1 - dx) * (1 - dy) * p00.B + dx * (1 - dy) * p10.B + (1 - dx) * dy * p01.B + dx * dy * p11.B;

	return P;
}

BOOL ZoomBicubic(HWND hwnd, BILD* src, BILD* dst, int cx, int cy)
{
	double zoom = 2.0;

	int w = dst->Breite;
	int h = dst->Hoehe;

	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++)
		{
			// Rücktransformation (Ziel -> Quelle)
			double srcX = cx + (x - w / 2.0) / zoom;
			double srcY = cy + (y - h / 2.0) / zoom;

			Punkt p;

			// Randbehandlung
			if (srcX < 1 || srcY < 1 || srcX >= src->Breite - 2 || srcY >= src->Hoehe - 2)
			{
				p = PunktHolenInt(src, (int)srcX, (int)srcY);
			}
			else
			{
				p = Bicubic(src, srcX, srcY);
			}

			PunktSetzen(dst, x, y, &p);
		}
	}

	InvalidateRect(hwnd, NULL, FALSE);
	return TRUE;
}

double cubicWeight(double t)
{
	t = fabs(t);

	const double a = -0.5; // Catmull-Rom

	if (t <= 1.0)
		return (a + 2) * t * t * t - (a + 3) * t * t + 1;
	else if (t < 2.0)
		return a * t * t * t - 5 * a * t * t + 8 * a * t - 4 * a;
	else
		return 0.0;
}

Punkt Bicubic(BILD* src, double x, double y)
{
	int ix = (int)x;
	int iy = (int)y;

	double dx = x - ix;
	double dy = y - iy;

	Punkt result;
	result.R = result.G = result.B = 0.0;

	// Sicherheitscheck (Randvermeidung)
	if (ix < 1 || iy < 1 || ix >= src->Breite - 2 || iy >= src->Hoehe - 2)
	{
		return PunktHolenInt(src, ix, iy);
	}

	// 4x4 Nachbarschaft
	for (int m = -1; m <= 2; m++)
	{
		for (int n = -1; n <= 2; n++)
		{
			Punkt p = PunktHolenInt(src, ix + n, iy + m);

			double wx = cubicWeight(n - dx);
			double wy = cubicWeight(m - dy);
			double w = wx * wy;

			result.R += p.R * w;
			result.G += p.G * w;
			result.B += p.B * w;
		}
	}

	// Clamp (wichtig!)
	result.R = max(0.0, min(255.0, result.R));
	result.G = max(0.0, min(255.0, result.G));
	result.B = max(0.0, min(255.0, result.B));

	return result;
}


