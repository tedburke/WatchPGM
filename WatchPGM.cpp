//
// WatchPGM.cpp - A PGM viewer that reloads if file changes
// Written by Ted Burke - last updated 27-11-2011
//
// To compile:
//
//		gcc -o WatchPGM.exe WatchPGM.cpp
//

// Standard Windows header file for all Win32 programs
#include <windows.h>

#include <sys/stat.h>
#include <stdio.h>

// Handle for image bitmap 
BITMAPINFO bmi;
unsigned char *image = NULL;
int w = 0, h = 0, maxval = 255;

FILE *f = NULL;
char filename[MAX_PATH] = "";
char last_filename[MAX_PATH] = "";
struct _stat last_stat_buf;
char text_buf[200];

// ID for background timer
UINT_PTR timer;
const int file_check_interval = 100; // Check every 100ms

// Function prototype
void LoadPGM(const char *);

// This is the application's message handler
LRESULT CALLBACK WndProc(
	HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Process the message
	switch(msg)
	{
	case WM_CREATE:

		// Start background timer
		timer = SetTimer(hWnd, 0, file_check_interval, NULL);
		
		// Register that files can be dropped into
		// this window
		DragAcceptFiles(hWnd, TRUE);
		
		break;
		
	case WM_PAINT:
	
		// Draw image
		PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
		if (image != NULL)
		{
			// Display pixel data in client area
			SetDIBitsToDevice( hdc, 0, 0, w, h, 0, 0,
				0, h, image, &bmi, DIB_RGB_COLORS);
		}
        EndPaint(hWnd, &ps);
		
		break;
		
	case WM_TIMER:

		// Check to see if file has been updated or
		// filename has changed
		struct _stat stat_buf;
		int result;
		if (strcmp(filename, "") == 0) break;
		result = _stat(filename, &stat_buf);
		if ((stat_buf.st_mtime > last_stat_buf.st_mtime) ||
			(strcmp(filename, last_filename) != 0))
		{
			// Reload updated file
			LoadPGM(filename);
			InvalidateRect(hWnd, NULL, FALSE);
		}
		
		break;
		
	case WM_DROPFILES:
	
		// Get filename (first filename if several files were dropped)
		DragQueryFile((HDROP)wParam, 0, filename, MAX_PATH );
		break;
		
	case WM_KEYDOWN:
	
		// Respond to a key stroke here
		break;
		
	case WM_CLOSE:
	
		DestroyWindow(hWnd);
		break;
	
	case WM_DESTROY:
	
		// Kill background timer
		if (timer) KillTimer(hWnd, timer);
		
		// Deallocate image buffer if necessary
		if (image != NULL)
		{
			delete[] image;
			image = NULL;
		}
		
		// Exit program
		PostQuitMessage(0);
		
		break;
		
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	
	return 0;
}

// This function is the entry point for the program
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	WNDCLASSEX wc;
	HWND hWnd;
	MSG msg;

	// Register window class
	wc.cbSize        = sizeof(WNDCLASSEX);
	wc.style         = 0;
	wc.lpfnWndProc   = WndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = "WatchPGMClass";
	wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);
	RegisterClassEx(&wc);
	
	// Create window
	hWnd = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		"WatchPGMClass",
		"WatchPGM",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		800, 600,
		NULL, NULL,
		hInstance, NULL);
		
	// Show window
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
	
	// Main message Loop
	while(GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
}

// Load or reload a PGMimage file into memory
void LoadPGM(const char *filename)
{
	// Nasty hack to check if file is currently open
	// in another program. There had been a problem
	// without this because WatchPGM was already trying
	// to reload the file before the modifying program
	// had finished writing it, causing a corrupted
	// image to be loaded.
	if (rename(filename, filename) != 0) return;

	// Deallocate pixel buffer
	if (image != NULL)
	{
		delete[] image;
		image = NULL;
	}
	
	// Load bitmap data from file
	fprintf(stderr, "Loading image file %s...", filename);
	f = fopen(filename, "r");
	if (!f) fprintf(stderr, "Error\n");
	
	// Read file header
	fscanf(f, "%[^\n]\n", text_buf);
	fscanf(f, "%[^\n]\n", text_buf);
	fscanf(f, "%d %d\n", &w, &h);
	fscanf(f, "%d\n", &maxval);
	
	// Allocate pixel buffer
	image = new unsigned char[3*w*h];
	
	// Read pixel data from file
	int n;
	for (int y=h-1 ; y>=0 ; --y)
	{
		for (int x=0 ; x<w ; ++x)
		{
			n = 3 * (y*w + x);
			fscanf(f, "%d", &image[n]);
			image[n+1] = image[n];
			image[n+2] = image[n];
		}
	}
	
	// Close file
	fclose(f);

	// Fill bitmap info structure
	bmi.bmiHeader.biSize = sizeof(BITMAPINFO);
	bmi.bmiHeader.biWidth = w;
	bmi.bmiHeader.biHeight = h;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 24;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biSizeImage = 0; // can be zero for BI_RGB images
	bmi.bmiHeader.biXPelsPerMeter = 0;
	bmi.bmiHeader.biYPelsPerMeter = 0;
	bmi.bmiHeader.biClrUsed = 0;
	bmi.bmiHeader.biClrImportant = 0;

	// Remember the name of the file that was loaded
	strcpy(last_filename, filename);

	// Remember modification time of file
	struct _stat stat_buf;
	int result;
	result = _stat(filename, &stat_buf);
	last_stat_buf = stat_buf;
	
	fprintf(stderr, "Done\n");
}
