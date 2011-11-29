//
// WatchPGM.cpp - A lightweight PGM image viewer
// Written by Ted Burke - last updated 28-11-2011
//
// Copyright Ted Burke, 2011, All rights reserved.
//
// Website: http://batchloaf.wordpress.com
//
// To compile:
//
//		g++ -o WatchPGM.exe WatchPGM.cpp -lgdi32
//

// Standard Windows header file for all Win32 programs
#include <windows.h>

#include <sys/stat.h>
#include <stdio.h>

// Handle for image bitmap 
BITMAPINFO bmi;
unsigned char *image = NULL;
int w = 0, h = 0, maxval = 255;

struct FileInfo
{
	TCHAR name[MAX_PATH]; // filename
};

FILE *f = NULL;
char filename[MAX_PATH] = "";
int file_error = 0;
FileInfo *files = NULL;
int num_files = 0;
int current_file_number = 0;
char last_filename[MAX_PATH] = "";
struct _stat last_stat_buf;
char text_buf[2*MAX_PATH];

// ID for background timer
UINT_PTR timer;
const int file_check_interval = 100; // Check every 100ms

// Function prototype
int LoadPGM(const char *);

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
		
		// Allow files to be dropped into this window
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
		int result = _stat(filename, &stat_buf);
		
		if ((file_error == 0) &&
			(strlen(filename) > 0) &&
			((stat_buf.st_mtime > last_stat_buf.st_mtime) ||
				(strcmp(filename, last_filename) != 0)))
		{
			// Load new file or reload updated file
			fprintf(stderr, "Loading %s...", filename);
			if (LoadPGM(filename) == 0) printf("OK\n");
			else
			{
				file_error = 1;
				fprintf(stderr, "ERROR\n");
				sprintf(text_buf, "Invalid PGM file: %s", filename);
				MessageBox(NULL, text_buf, "Error", 0);
				InvalidateRect(hWnd, NULL, TRUE);
			}
			sprintf(text_buf, "WatchPGM %s", filename);
			SetWindowText(hWnd, text_buf);
			InvalidateRect(hWnd, NULL, FALSE);
		}
		
		break;
		
	case WM_DROPFILES:
	
		// Find out how many files were dropped
		num_files = DragQueryFile((HDROP)wParam, 0xFFFFFFFF, NULL, 0);
		if (num_files == 0) break;
		
		// Reallocate file info array for correct number of files
		if (files) delete[] files;
		files = new FileInfo[num_files];
		
		// Copy all filenames into the file info array
		for (int i=0 ; i < num_files ; ++i)
			DragQueryFile((HDROP)wParam, i, files[i].name, MAX_PATH);
		
		// Display first file in set initially
		current_file_number = 0;
		strncpy(filename, files[current_file_number].name, MAX_PATH);
		file_error = 0;
		
		break;
		
	case WM_KEYDOWN:
	
		// Process keypress
		if (wParam == 'Q')
		{
			// Q key quits WatchPGM
			DestroyWindow(hWnd);
		}
		else if (wParam == VK_LEFT || wParam == VK_DOWN)
		{
			// Left (or down) arrow key selects previous image in set
			if (--current_file_number < 0)
				current_file_number = num_files - 1;
			if (num_files > 0)
				strncpy(filename, files[current_file_number].name, MAX_PATH);
			file_error = 0;
		}
		else if (wParam == VK_RIGHT || wParam == VK_UP)
		{
			// Right (or up) arrow key selects next image in set
			if (++current_file_number >= num_files)
				current_file_number = 0;
			if (num_files > 0)
				strncpy(filename, files[current_file_number].name, MAX_PATH);
			file_error = 0;
		}
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

		// Delete file info array
		if (files)
		{
			delete[] files;
			files = NULL;
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
int LoadPGM(const char *filename)
{
	// Deallocate pixel buffer
	//fprintf(stderr, "deallocating pixel buffer\n");
	if (image != NULL)
	{
		delete[] image;
		image = NULL;
	}
	
	// Nasty hack to check if file is currently open
	// in another program. There had been a problem
	// without this because WatchPGM was already trying
	// to reload the file before the modifying program
	// had finished writing it, causing a corrupted
	// image to be loaded.
	//fprintf(stderr, "Trying to rename file %s\n", filename);
	if (rename(filename, filename) != 0) return 1;

	// Load bitmap data from file
	//fprintf(stderr, "Opening file\n");
	if ((f = fopen(filename, "r")) == NULL)
	{
		return 1;
	}
	
	// Read file header
	
	// Read and check the "P2" line
	//fprintf(stderr, "Reading file header\n");
	fgets(text_buf, 2*MAX_PATH, f);
	//fscanf(f, "%[^\n]\n", text_buf);
	
	if ((text_buf[0] != 'P') || (text_buf[1] != '2'))
	{
		// Doesn't seem to be a PGM file
		fclose(f);
		return 1;
	}
	
	// Read lines until we get to one that's not a comment (or blank)
	do
	{
		fscanf(f, "%[^\n]\n", text_buf);
	}
	while((!strlen(text_buf)) || text_buf[0] == '#');

	// Read width, height and maximum pixel value
	sscanf(text_buf, "%d %d", &w, &h);
	fscanf(f, "%[^\n]\n", text_buf);
	sscanf(text_buf, "%d", &maxval);
	
	// Allocate pixel buffer
	image = new unsigned char[3*w*h];
	
	// Read pixel data from file
	//fprintf(stderr, "Reading pixel data\n");
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
	//fprintf(stderr, "Closing file\n");
	fclose(f);

	// Fill bitmap info structure
	//fprintf(stderr, "Create bitmap structure\n");
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
	strncpy(last_filename, filename, MAX_PATH);

	// Remember modification time of file
	struct _stat stat_buf;
	int result = _stat(filename, &stat_buf);
	last_stat_buf = stat_buf;
	
	//fprintf(stderr, "Return zero\n");
	return 0;
}
