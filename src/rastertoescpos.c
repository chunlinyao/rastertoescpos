/*
 *   ESC/POS printer filter for the Common UNIX Printing System (CUPS).
 *
 *   Copyright 2001-2007 by Easy Software Products.
 *   Copyright 2009 by Patrick Kong
 *   Copyright 2010 by Sam Lown
 *   Copyright 2016 by Chunlin Yao
 *
 *   Based on Source from CUPS printing system and rastertotpcl filter.
 * 
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *
 *  Structure based on CUPS rastertolabel by Easy Software Products, 2007.
 *  Base version by Patrick Kong, 2009-07-21
 *  TOPIX Compression added by Sam Lown, 2010-05-24
 *
 * Contents:
 *
 *   Setup()        - Prepare the printer for printing.
 *   StartPage()    - Start a page of graphics.
 *   EndPage()      - Finish a page of graphics.
 *   CancelJob()    - Cancel the current job...
 *   OutputSlice()   - Output a slice of graphics.
 *   main()         - Main entry and processing of driver.
 *
 *
 * This driver should support all Toshiba TEC Label Printers with support for TPCL (TEC
 * Printer Command Language) and TOPIX Compression for graphics. 
 *
 */

#include <cups/cups.h>
#include <cups/raster.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <math.h>


/*
 * Model number constants...
 */
#define INTSIZE		20 			/* MAXIMUM CHARACTERS INTEGER */

/*
 * Globals...
 */
static unsigned char	*Buffer;		     /* Output buffer */
static unsigned char  *CompBuffer;     /* Byte array for transpose */
int   Page,           /* Current page */
      Canceled;		    /* Non-zero if job is canceled */

int		ModelNumber; 		/* cupsModelNumber attribute (not currently in use) */


struct settings_
{
  int cashDrawer1;
  int cashDrawer2;
  int cutter;
  int reducationPaper;
};

struct settings_ settings;

/*
 * Prototypes...
 */
void Setup(ppd_file_t *ppd);
void StartPage(ppd_file_t *ppd, cups_page_header2_t *header);
void EndPage(ppd_file_t *ppd, cups_page_header2_t *header);
void CancelJob(int sig);
void OutputSlice(ppd_file_t *ppd, cups_page_header2_t *header, int y);


inline int lo (int val)
{
  return val & 0xFF;
}

inline int hi (int val)
{
  return lo (val >> 8);
}

inline void initPrinter()
{
  printf("\x1b@");
}

inline void cutPaper()
{
  printf("\x1dV\x42");
  putchar('\0');
}

inline void cashDrawer(int drawer)
{
  printf("\x1bp%d\x40\x50", drawer-1);
  fflush(stdout);
}

inline void setLineHeight(int h)
{
  //Set line height to 24dot 
  char* cmd = (char[3]) {0x1b, 0x33, h};
  fwrite(cmd, 1, 3, stdout);
}
/*
 * 'Setup()' - Prepare the printer for printing.
 */
void Setup(ppd_file_t *ppd)			/* I - PPD file */
{
  ppd_choice_t	*choice;		/* Marked choice */

  /*
   * Get the model number from the PPD file.
   * This is not yet used for anything.
   */
  ModelNumber = ppd->model_number;
  
  /*
   * Always send a reset command. Helps with reliability on failed jobs.
   */
  initPrinter();

  choice = ppdFindMarkedChoice(ppd, "escCutter");
  settings.cutter = atoi(choice->choice);

  choice = ppdFindMarkedChoice(ppd, "escReducationPaper");
  settings.reducationPaper = atoi(choice->choice);
  
  choice = ppdFindMarkedChoice(ppd, "escCashDrawer1");
  settings.cashDrawer1 = atoi(choice->choice);

  choice = ppdFindMarkedChoice(ppd, "escCashDrawer2");
  settings.cashDrawer2 = atoi(choice->choice);

  
  if (settings.cashDrawer1 == 1)
  {
    cashDrawer(1);
  }

  if (settings.cashDrawer2 == 1)
  {
    cashDrawer(2);
  }
  setLineHeight(24);
}


/*
 * 'StartPage()' - Start a page of graphics.
 */
void
StartPage(ppd_file_t         *ppd,	/* I - PPD file */
          cups_page_header2_t *header)	/* I - Page header */
{
  
#if defined(HAVE_SIGACTION) && !defined(HAVE_SIGSET)
  struct sigaction action;		/* Actions for POSIX signals */
#endif /* HAVE_SIGACTION && !HAVE_SIGSET */

  /*
   * Show page device dictionary...
   */
  fprintf(stderr, "DEBUG: StartPage...\n");
  fprintf(stderr, "DEBUG: MediaClass = \"%s\"\n", header->MediaClass);
  fprintf(stderr, "DEBUG: MediaColor = \"%s\"\n", header->MediaColor);
  fprintf(stderr, "DEBUG: MediaType = \"%s\"\n", header->MediaType);
  fprintf(stderr, "DEBUG: OutputType = \"%s\"\n", header->OutputType);

  fprintf(stderr, "DEBUG: AdvanceDistance = %d\n", header->AdvanceDistance);
  fprintf(stderr, "DEBUG: AdvanceMedia = %d\n", header->AdvanceMedia);
  fprintf(stderr, "DEBUG: Collate = %d\n", header->Collate);
  fprintf(stderr, "DEBUG: CutMedia = %d\n", header->CutMedia);
  fprintf(stderr, "DEBUG: Duplex = %d\n", header->Duplex);
  fprintf(stderr, "DEBUG: HWResolution = [ %d %d ]\n", header->HWResolution[0],
          header->HWResolution[1]);
  fprintf(stderr, "DEBUG: ImagingBoundingBox = [ %d %d %d %d ]\n",
          header->ImagingBoundingBox[0], header->ImagingBoundingBox[1],
          header->ImagingBoundingBox[2], header->ImagingBoundingBox[3]);
  fprintf(stderr, "DEBUG: InsertSheet = %d\n", header->InsertSheet);
  fprintf(stderr, "DEBUG: Jog = %d\n", header->Jog);
  fprintf(stderr, "DEBUG: LeadingEdge = %d\n", header->LeadingEdge);
  fprintf(stderr, "DEBUG: Margins = [ %d %d ]\n", header->Margins[0],
          header->Margins[1]);
  fprintf(stderr, "DEBUG: ManualFeed = %d\n", header->ManualFeed);
  fprintf(stderr, "DEBUG: MediaPosition = %d\n", header->MediaPosition);
  fprintf(stderr, "DEBUG: MediaWeight = %d\n", header->MediaWeight);
  fprintf(stderr, "DEBUG: MirrorPrint = %d\n", header->MirrorPrint);
  fprintf(stderr, "DEBUG: NegativePrint = %d\n", header->NegativePrint);
  fprintf(stderr, "DEBUG: NumCopies = %d\n", header->NumCopies);
  fprintf(stderr, "DEBUG: Orientation = %d\n", header->Orientation);
  fprintf(stderr, "DEBUG: OutputFaceUp = %d\n", header->OutputFaceUp);
  fprintf(stderr, "DEBUG: cupsPageSize = [ %f %f ]\n", header->cupsPageSize[0],
          header->cupsPageSize[1]);
  fprintf(stderr, "DEBUG: Separations = %d\n", header->Separations);
  fprintf(stderr, "DEBUG: TraySwitch = %d\n", header->TraySwitch);
  fprintf(stderr, "DEBUG: Tumble = %d\n", header->Tumble);
  fprintf(stderr, "DEBUG: cupsWidth = %d\n", header->cupsWidth);
  fprintf(stderr, "DEBUG: cupsHeight = %d\n", header->cupsHeight);
  fprintf(stderr, "DEBUG: cupsMediaType = %d\n", header->cupsMediaType);
  fprintf(stderr, "DEBUG: cupsBitsPerColor = %d\n", header->cupsBitsPerColor);
  fprintf(stderr, "DEBUG: cupsBitsPerPixel = %d\n", header->cupsBitsPerPixel);
  fprintf(stderr, "DEBUG: cupsBytesPerLine = %d\n", header->cupsBytesPerLine);
  fprintf(stderr, "DEBUG: cupsColorOrder = %d\n", header->cupsColorOrder);
  fprintf(stderr, "DEBUG: cupsColorSpace = %d\n", header->cupsColorSpace);
  fprintf(stderr, "DEBUG: cupsCompression = %d\n", header->cupsCompression);

  /*
   * Register a signal handler to eject the current page if the
   * job is canceled.
   */
#ifdef HAVE_SIGSET /* Use System V signals over POSIX to avoid bugs */
  sigset(SIGTERM, CancelJob);
#elif defined(HAVE_SIGACTION)
  memset(&action, 0, sizeof(action));

  sigemptyset(&action.sa_mask);
  action.sa_handler = CancelJob;
  sigaction(SIGTERM, &action, NULL);
#else
  signal(SIGTERM, CancelJob);
#endif /* HAVE_SIGSET */
  Buffer = malloc(header->cupsBytesPerLine*24);
  CompBuffer = malloc(header->cupsWidth*3);
}


/*
 * 'EndPage()' - Finish a page of graphics.
 */
void
EndPage(ppd_file_t *ppd,		/* I - PPD file */
        cups_page_header2_t *header)	/* I - Page header */
{

#if defined(HAVE_SIGACTION) && !defined(HAVE_SIGSET)
  struct sigaction action;		/* Actions for POSIX signals */
#endif /* HAVE_SIGACTION && !HAVE_SIGSET */

  free(Buffer);
  Buffer = NULL;
  free(CompBuffer);
  CompBuffer = NULL;

  if (Canceled)
  {
    /*
     * Ramclear in case of error
     */
    initPrinter();

  } else {
    if (settings.cutter == 2) // Cut per page
    {
      cutPaper();
    }

  } // Not Cancelled

  /*
   * Unregister the signal handler...
   */

#ifdef HAVE_SIGSET /* Use System V signals over POSIX to avoid bugs */
  sigset(SIGTERM, SIG_IGN);
#elif defined(HAVE_SIGACTION)

  memset(&action, 0, sizeof(action));

  sigemptyset(&action.sa_mask);
  action.sa_handler = SIG_IGN;
  sigaction(SIGTERM, &action, NULL);
#else
  signal(SIGTERM, SIG_IGN);
#endif /* HAVE_SIGSET */

  /*
   * Free memory...
   */
  free(Buffer);
}


/*
 * 'CancelJob()' - Cancel the current job...
 */
void
CancelJob(int sig)			/* I - Signal */
{
 /*
  * Tell the main loop to stop...
  */
  (void)sig;
  Canceled = 1;
}


/*
 * 'OutputSlice()' - Output a line of graphics.
 * 
 * Some versions of this method check to see if the Buffer has data, this doesn't.
 * Empty lines can often be skipped if the buffer is checked.
 */
void
OutputSlice(ppd_file_t           *ppd,	    /* I - PPD file */
           cups_page_header2_t  *header,	/* I - Page header */
           int                  y)	      /* I - Line number */
{
    int width = header->cupsWidth; 
    int bytesPerLine = header->cupsBytesPerLine;
    // Hex Output
    char* cmd = (char[5]) {0x1b, 0x2a, 33, lo(width), hi(width)};
    fwrite(cmd, 1, 5, stdout); 

    memset(CompBuffer, 0, sizeof(CompBuffer));    

    int x,k,b,i,j;
    for (x=0; x < width; x++)
    {
       for (k=0; k< 3; k++)
       {
          unsigned char slice = 0;
          unsigned char mask = 1 << (7 - x % 8);
          i = (k * 8 * bytesPerLine ) + (x / 8);
          for (b=0; b<8; b++) 
          {
             if (Buffer[i+bytesPerLine*b] & mask )  slice |= 1<<(7-b);
          }
          CompBuffer[x*3+k] = slice;
       }
    }
    fwrite(CompBuffer, 1, width*3, stdout);
    putchar(10); //Line feed
}


/*
 * 'main()' - Main entry and processing of driver.
 */

int					/* O - Exit status */
main(int  argc,				/* I - Number of command-line arguments */
     char *argv[])			/* I - Command-line arguments */
{
  int           			fd;		  /* File descriptor */
  cups_raster_t		    *ras;		/* Raster stream for printing */
  cups_page_header2_t	header;	/* Page header from file */
  int                 y;      /* Current line */
  ppd_file_t          *ppd;   /* PPD file */
  int                 num_options;	/* Number of options */
  cups_option_t       *options;	/* Options */


  /*
   * Make sure status messages are not buffered...
   */
  setbuf(stderr, NULL);

  /*
   * Check command-line...
   */
  if (argc < 6 || argc > 7)
  {
    /*
     * We don't have the correct number of arguments; write an error message
     * and return.
     */
    fputs("ERROR: rastertotec job-id user title copies options [file]\n", stderr);
    return (1);
  }

 /*
  * Open the page stream...
  */
  if (argc == 7)
  {
    if ((fd = open(argv[6], O_RDONLY)) == -1)
    {
      perror("ERROR: Unable to open raster file - ");
      sleep(1);
      return (1);
    }
  }
  else
    fd = 0;

  ras = cupsRasterOpen(fd, CUPS_RASTER_READ);

 /*
  * Open the PPD file and apply options...
  */
  num_options = cupsParseOptions(argv[5], 0, &options);

  if ((ppd = ppdOpenFile(getenv("PPD"))) != NULL)
  {
    ppdMarkDefaults(ppd);
    cupsMarkOptions(ppd, num_options, options);
  }
  else
  {
    fputs("ERROR: Missing PPD file required for defaults!", stderr);
    return(1);
  }

  /*
   * Initialize the print device...
   */
  Setup(ppd);

  /*
   * Process pages as needed...
   */
  Page     = 0;
  Canceled = 0;

  while (cupsRasterReadHeader2(ras, &header))
  {
    /*
     * Write a status message with the page number and number of copies.
     */
    Page++;
    fprintf(stderr, "PAGE: %d 1\n", Page);

    /*
     * Start the page...
     */
    StartPage(ppd, &header);

    /*
     * Loop for each line on the page...
     */
    
    for (y = 0; y < header.cupsHeight && !Canceled; y+=24)
    {
      memset(Buffer, 0, sizeof(Buffer));
      /*
       * Let the user know how far we have progressed...
       */
      fprintf(stderr, "INFO: Printing page %d, %d%% complete...\n", Page,
	        100 * y / header.cupsHeight);

      int rest = header.cupsHeight - y;
      if (rest > 24)
      {
        rest = 24;
      }
      /*
       * Read 24 lines of graphics...
       */
      if (cupsRasterReadPixels(ras, Buffer, header.cupsBytesPerLine*24) < 1) 
           break;

      /*
       * Write it to the printer...
       */
      OutputSlice(ppd, &header, y);
    }

    /*
     * Eject the page...
     */
    EndPage(ppd, &header);
    if (Canceled)
      break;
  }

  //End Job
  if (settings.cutter == 1)
  {
    cutPaper();
  }

  if (settings.cashDrawer1 == 2)
  {
    cashDrawer(1);
  }

  if (settings.cashDrawer2 == 2)
  {
    cashDrawer(2);
  }

  //Set line height to 30dot 
  setLineHeight(30);
  /*
   * Close the raster stream...
   */
  cupsRasterClose(ras);
  if (fd != 0)
    close(fd);

  /*
   * Close the PPD file and free the options...
   */
  ppdClose(ppd);
  cupsFreeOptions(num_options, options);

  /*
   * If no pages were printed, send an error message...
   */
  if (Page == 0)
    fputs("ERROR: No pages found!\n", stderr);
  else
    fputs("INFO: Ready to print.\n", stderr);
  return (Page == 0);
}

