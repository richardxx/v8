// Options

#ifndef OPTIONS_H
#define OPTIONS_H

#include <unistd.h>

static const char* input_file = NULL;
static const char* visual_file = NULL;
static int slice_sig = -1;

static int parse_options(int argc, char** argv)
{
  int c;

  while ( (c = getopt(argc, argv, "v:s:h") ) != -1 ) {
    switch(c) {
    case 'v':
      visual_file = optarg;
      break;
      
    case 's':
      sscanf( optarg, "%p", slice_sig );
      break;

    case 'h':
      printf( "Usage: %s [options] input_file\n", argv[0] );
      printf( "Options:\n");
      printf( "-v [file]            : Output graphviz file for visualization\n" );
      printf( "-s [signature]       : Output a log slice of specified function/object\n" );
      printf( "-h        : Print this help.\n" );
      break;
    }
  }

  if ( optind == argc ) {
    printf( "Missing input file. Aborted.\n" );
    return 0;
  }

  input_file = argv[optind];
  return 1;
}


#endif

