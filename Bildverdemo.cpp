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

// -------- Funktionsdeklarationen ---------
// ... (andere Deklarationen)
double cubicWeight(double x);
Punkt Bicubic(BILD* src, double x, double y);
Punkt bilinear(BILD* src, double x, double y); // FIX: Semikolon ergänzt

void RGBtoHSV(Punkt rgb, double& h, double& s, double& v);
BOOL ErzeugeHSVMatrix(HWND hwnd, BILD* pQuelle, BILD* pZiel);
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

		

		case ID_PRAKTIKUM_HSV_MATRIX:
		{  // <--- Diese Klammer muss da sein
			// 1. Sicherheitscheck: Gibt es überhaupt ein Bild?
			if (Bild_1.Daten == NULL) break;

			// 2. Funktion aufrufen (diese erledigt BildInit und die HSV-Berechnung)
			if (ErzeugeHSVMatrix(hwnd, &Bild_1, &Bild_2))
			{
				// 3. Das Ergebnis (die 2x2 Matrix) im Fenster anzeigen
				ShowBmp(hwnd, &Bild_2, 0, 0, TRUE);

				// 4. Die typische Bild-Rotation für die Undo-Funktion
				Bild_3 = Bild_2;
				Bild_2 = Bild_1;
				Bild_1 = Bild_3;
			}
		} // <--- Und diese auch vor dem break
		break;

		case ID_INTERPOLATION_BICUBIC:
			if (Bild_1.Daten != NULL)
			{
				// Sicherheitscheck: Wurde überhaupt schon ins Bild geklickt?
				if (g_clickX <= 0 || g_clickY <= 0) {
					Melde("Bitte zuerst mit Linksklick einen Punkt im Bild wählen!", 0x0000FF, FALSE);
					break;
				}

				int zoomSize = 512;
				double scale = 1.0 / 64.0; // 64-fache Vergrößerung laut Aufgabe

				if (BildInit(&Bild_2, zoomSize, zoomSize, 24, 0, 1000))
				{
					double startX = (double)g_clickX;
					double startY = (double)g_clickY;

					for (int y = 0; y < Bild_2.Hoehe; y++)
					{
						for (int x = 0; x < Bild_2.Breite; x++)
						{
							// Quellkoordinaten berechnen
							double src_x = startX + (double)(x - zoomSize / 2) * scale;
							double src_y = startY + (double)(y - zoomSize / 2) * scale;

							// Bikubische Interpolation
							Punkt p = Bicubic(&Bild_1, src_x, src_y);
							PunktSetzen(&Bild_2, x, y, &p);
						}
					}

					// --- WICHTIG: ERGEBNIS ANZEIGEN ---
					ShowBmp(hwnd, &Bild_2, 0, 0, TRUE);

					// Undo-Logik: Aktuelles Bild in Bild_2 sichern, altes in Bild_1 lassen oder rotieren
					// Damit das Programm stabil bleibt, empfehle ich hier:
					BildCopy(&Bild_2, &Bild_3); // Backup in Bild_3
					Melde("Bikubische Vergrößerung (64x) fertig!", 0x00FF00, FALSE);
				}
			}
			break;

		case ID_INTERPOLATION_BILINEAR:
			if (Bild_1.Daten != NULL)
			{
				int zoomSize = 512;        // Größe des Ausgabefensters
				double scale = 1.0 / 64.0; // 64-fache Vergrößerung laut Aufgabe 

				if (BildInit(&Bild_2, zoomSize, zoomSize, 24, 0, 1000))
				{
					// g_clickX/Y wurden in WM_LBUTTONDOWN gespeichert
					double startX = (double)g_clickX;
					double startY = (double)g_clickY;

					for (int y = 0; y < Bild_2.Hoehe; y++)
					{
						for (int x = 0; x < Bild_2.Breite; x++)
						{
							// Berechne Quellkoordinate (zentriert um Klickpunkt)
							double src_x = startX + (double)(x - zoomSize / 2) * scale;
							double src_y = startY + (double)(y - zoomSize / 2) * scale;

							// Bilineare Funktion aufrufen
							Punkt p = bilinear(&Bild_1, src_x, src_y);

							PunktSetzen(&Bild_2, x, y, &p);
						}
					}

					ShowBmp(hwnd, &Bild_2, 0, 0, TRUE);

					// Undo-Logik
					Bild_3 = Bild_2; Bild_2 = Bild_1; Bild_1 = Bild_3;
					Melde("Bilineare Vergrößerung (64x) fertig", 0x00FF00, FALSE);
				}
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
			{
				// Statt: P = Schwarz;
				P.R = 0; P.G = 0; P.B = 0;
			}

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

Punkt Bicubic(BILD* src, double x, double y)
{
	// ix, iy ist die obere linke Ecke der zentralen 4 Pixel
	int ix = (int)floor(x);
	int iy = (int)floor(y);

	double dx = x - ix;
	double dy = y - iy;

	Punkt result;
	result.R = result.G = result.B = 0.0;

	// Sicherheitscheck: Wir brauchen 2 Pixel in jede Richtung (n-1 bis n+2)
	if (ix < 1 || iy < 1 || ix >= src->Breite - 2 || iy >= src->Hoehe - 2)
	{
		return PunktHolenInt(src, (int)(x + 0.5), (int)(y + 0.5));
	}

	// 4x4 Nachbarschaft (n und m laufen von -1 bis 2)
	for (int m = -1; m <= 2; m++)
	{
		double wy = cubicWeight(m - dy);
		for (int n = -1; n <= 2; n++)
		{
			Punkt p = PunktHolenInt(src, ix + n, iy + m);
			double wx = cubicWeight(n - dx);

			double w = wx * wy;

			result.R += p.R * w;
			result.G += p.G * w;
			result.B += p.B * w;
		}
	}

	// Clamp: Verhindert "Overshooting" (Werte < 0 oder > 255)
	result.R = (result.R < 0) ? 0 : (result.R > 255 ? 255 : result.R);
	result.G = (result.G < 0) ? 0 : (result.G > 255 ? 255 : result.G);
	result.B = (result.B < 0) ? 0 : (result.B > 255 ? 255 : result.B);

	return result;
}

double cubicWeight(double x) {
	x = fabs(x);
	double a = -0.5; // Standard-Koeffizient
	if (x <= 1.0) {
		return (a + 2.0) * pow(x, 3) - (a + 3.0) * pow(x, 2) + 1.0;
	}
	else if (x < 2.0) {
		return a * pow(x, 3) - 5.0 * a * pow(x, 2) + 8.0 * a * x - 4.0 * a;
	}
	return 0.0;
}

// Hilfsfunktion: Wandelt einen RGB-Punkt in HSV um
// Ergebnis wird zur Visualisierung wieder auf 0-255 skaliert
void RGBtoHSV(Punkt rgb, double& h, double& s, double& v) {
	double r = rgb.R / 255.0;
	double g = rgb.G / 255.0;
	double b = rgb.B / 255.0;
	double maxVal = max(r, max(g, b));
	double minVal = min(r, min(g, b));
	double delta = maxVal - minVal;
	v = maxVal;
	s = (maxVal > 0) ? (delta / maxVal) : 0;
	if (delta == 0) h = 0;
	else {
		if (maxVal == r) h = 60.0 * fmod(((g - b) / delta), 6.0);
		else if (maxVal == g) h = 60.0 * (((b - r) / delta) + 2.0);
		else h = 60.0 * (((r - g) / delta) + 4.0);
	}
	if (h < 0) h += 360.0;
}

BOOL ErzeugeHSVMatrix(HWND hwnd, BILD* pQuelle, BILD* pZiel) {
	BildInit(pZiel, pQuelle->Breite * 2, pQuelle->Hoehe * 2, 24, 0, 1000);

	for (long y = 0; y < pQuelle->Hoehe; y++) {
		for (long x = 0; x < pQuelle->Breite; x++) {
			// WICHTIG: Hier muss der Punkt P erst aus dem Quellbild geholt werden!
			Punkt P = PunktHolenInt(pQuelle, x, y);

			double h, s, v;
			RGBtoHSV(P, h, s, v);

			// 1. Quadrant (Oben Links): Originalbild
			PunktSetzen(pZiel, x, y, &P);
			// 2. Quadrant (Oben Rechts): Farbwert H (Hue)
			BYTE h_gray = (BYTE)((h / 360.0) * 255.0);
			Punkt Ph;
			Ph.R = Ph.G = Ph.B = h_gray; // Explizite Zuweisung aller Kanäle
			PunktSetzen(pZiel, x + pQuelle->Breite, y, &Ph);

			// 3. Quadrant (Unten Links): Sättigung S (Saturation)
			BYTE s_gray = (BYTE)(s * 255.0);
			Punkt Ps;
			Ps.R = Ps.G = Ps.B = s_gray; // Explizite Zuweisung aller Kanäle
			PunktSetzen(pZiel, x, y + pQuelle->Hoehe, &Ps);

			// 4. Quadrant (Unten Rechts): Hellwert V (Value)
			BYTE v_gray = (BYTE)(v * 255.0);
			Punkt Pv;
			Pv.R = Pv.G = Pv.B = v_gray; // Explizite Zuweisung aller Kanäle
			PunktSetzen(pZiel, x + pQuelle->Breite, y + pQuelle->Hoehe, &Pv);
		}
	}
	return TRUE;
}

Punkt bilinear(BILD* src, double x, double y)
{
	int x1 = (int)floor(x);
	int y1 = (int)floor(y);
	int x2 = x1 + 1;
	int y2 = y1 + 1;

	// Grenzprüfung
	if (x1 < 0 || y1 < 0 || x2 >= src->Breite || y2 >= src->Hoehe)
		return PunktHolenInt(src, (int)(x + 0.5), (int)(y + 0.5));

	double dx = x - x1;
	double dy = y - y1;

	Punkt p11 = PunktHolenInt(src, x1, y1);
	Punkt p21 = PunktHolenInt(src, x2, y1);
	Punkt p12 = PunktHolenInt(src, x1, y2);
	Punkt p22 = PunktHolenInt(src, x2, y2);

	Punkt res;
	res.R = (1 - dx) * (1 - dy) * p11.R + dx * (1 - dy) * p21.R + (1 - dx) * dy * p12.R + dx * dy * p22.R;
	res.G = (1 - dx) * (1 - dy) * p11.G + dx * (1 - dy) * p21.G + (1 - dx) * dy * p12.G + dx * dy * p22.G;
	res.B = (1 - dx) * (1 - dy) * p11.B + dx * (1 - dy) * p21.B + (1 - dx) * dy * p12.B + dx * dy * p22.B;
	return res;
}