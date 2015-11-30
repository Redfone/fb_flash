#if HAVE_CONFIG_H
# include <config.h>
#define SW_VER FBFLASH_VERSION
#endif

#if STDC_HEADERS || HAVE_STDLIB_H
# include <stdlib.h>
#endif

#if HAVE_UNISTD_H
# include <unistd.h>
#endif

#if HAVE_TIME_H
# include <time.h>
#endif

#if HAVE_LIBFB_FB_LIB_H
# include <libfb/fb_lib.h>
#endif

#include "ver.h"

/* Default chip size - 16 Mbit = 2 Mbyte */
#define EPCS_SPAN (2*(1<<20))
#define EPCS_BLK_SIZE (1<<16)

static libfb_t *f;

/* Prototypes */
int read_to_bin (unsigned char *mac, int address, int length, FILE * output);
int write_from_bin (unsigned char *mac, int address, FILE * input);
int diff_file (FILE * file1, FILE * file2);
extern void init_rng (void);
extern void generate_key_entry (KEY_ENTRY * entry, SERIAL * ser,
				MAC_ADDR * mac, u_int8_t * seedptr);
extern void program_seeds (libfb_t * fb, MAC_ADDR * mac, unsigned int addr,
			   u_int8_t * keybuffer, unsigned count);
/* Generate `num' number of keys and print them to standard output. */
void
generate_all_keys (libfb_t * fb, MAC_ADDR * mac, int num)
{
  KEY_ENTRY *entry;
  KEY_ENTRY *all;
  int i;

  entry = malloc (sizeof (KEY_ENTRY));

  if (!entry)
    {
      perror ("malloc");
      exit (EXIT_FAILURE);
    }

  all = malloc (sizeof (KEY_ENTRY) * num);

  if (!entry)
    {
      perror ("malloc");
      exit (EXIT_FAILURE);
    }

  for (i = 0; i < num; i++)
    {
      int j;
      SERIAL ser;
      memset (ser, 0xAB, sizeof (ser));
      MAC_ADDR mac;
      memset (mac, 0xDD, sizeof (mac));

      u_int8_t seed[SEED_SZ];

      generate_key_entry (entry, &ser, &mac, seed);
      printf ("{\n");
      printf ("\tSLOT_ID = %i;\n", i + 1);
      printf ("\tHASH_KEY = 0x");
      for (j = 0; j < HASH_KEY_SZ; j++)
	printf ("%02X", entry->hash_key[j]);
      printf (";\n");
      printf ("\tCUSTOMER_KEY = 0x");
      for (j = 0; j < CUSTOMER_KEY_SZ; j++)
	printf ("%02X", seed[j]);
      printf (";\n");
      printf ("}");

      printf ("\n");
      memcpy (all + i, entry, sizeof (KEY_ENTRY));
    }
  int addr = 0xDEADBEEF;	// TODO 
  program_seeds (fb, mac, addr, (u_int8_t *) all, num);
}


/********** Config options ************/

/* These are the 2 Fonebridge MACs */
#if 1
unsigned char mac_dflt[2][6] = {
#if 0
  {0, 0x50, 0xC2, 0x65, 0xD0, 2},
  {0, 0x50, 0xC2, 0x65, 0xD0, 3}
#else
  {0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
  {0, 0x50, 0xC2, 0x65, 0xD0, 3}
#endif
};
#endif

struct
{
  int verbosity;
  int addr_in_blocks;
  int start_addr;
  int len;
  int op;
#define RD_OP 0
#define WR_OP 1
#define CONFIG_OP 2
#define GPAK_OP 3
  char filename[256];
  int if_num;
} config_opts;

void
print_extra_version ()
{
  printf ("Compiled on %s at %s.\n", __DATE__, __TIME__);
  libfb_printver ();
}

void
print_usage ()
{
  printf ("\n");
  printf ("-------------------------------\n");
  printf ("Options:\n");
  printf ("-h:\tHelp\n");
  printf ("-v:\tVerbose operation\n");
  printf ("-V:\tPrint build information\n");
#ifndef PUBLIC_RELEASE
  printf ("-r <filename>:\tRead Flash to bin\n");
  printf ("-s <hex addr>:\tStart address offset\n");
  printf ("-l <hex val>:\tLength in bytes\n");
#endif
  printf ("-w <filename>:\tWrite bin file to Flash\n");
  printf ("-g <filename>:\tWrite GPAK file to Flash\n");
#ifndef PUBLIC_RELEASE
  printf ("-c:\tWrite config data\n");
#else
  printf ("-c:\tSet IP Address\n");
#endif
  printf ("-m <MAC>:\tMAC Address in AABBCCDDEEFF format\n");
  printf ("-i <eth_if>:\teth<x> interface to use (eg: 'eth0' by default)\n");
#ifndef PUBLIC_RELEASE
  printf ("-d:\tDisable CRC checking (enabled by default)\n");
  printf ("-b:\tStarting address is in blocks\n");
#endif
}

void show_warning ()
{
  printf ("**************************************************\n");
  printf ("* WARNING: Any interruption in network service\n");
  printf ("* to the target WILL cause flash corruption\n");
  printf ("* and will render the target unusable. Do NOT\n");
  printf ("* remove power or the network connection\n");
  printf ("* while updating firmware.\n");
  printf ("**************************************************\n");
  printf ("Press any key to continue...\n");

  getchar();
}



bool
acceptInputStream (char *buffer, int len, FILE * stream)
{
  char *s;
  s = fgets (buffer, len, stream);
  if (s == NULL)
    return false;
  if (s[0] == '-')
    return false;
  return true;
}

inline bool
acceptInput (char *buffer, int len)
{
  return acceptInputStream (buffer, len, stdin);
}

int
fill_config_struct (EPCS_CONFIG * epcs)
{
#define USER_STR_LEN 128
  time_t calendar_time;
  int result;
  int x;
  char userstr[USER_STR_LEN];
  unsigned char mactmp[6];
  unsigned char tmp;
  SERIAL tmpser;

  printf
    ("If you wish to keep the current setting enter only a dash '-' symbol.\n");

#ifndef PUBLIC_RELEASE
  printf ("Enter the first MAC address) ");
  if (acceptInput (userstr, USER_STR_LEN))
    result = parse_mac (userstr, mactmp);
  else
    result = -1;

  if (result == 0)
    memcpy (epcs->mac_addr, mactmp, 6);

  print_mac ((unsigned char *) epcs->mac_addr);
#endif
  printf ("Enter IP addresses in dotted quad format! (eg. 192.168.1.1)\n");

  for (x = 0; x < 2; x++)
    {
      unsigned char iptmp[4];
      printf ("New IP Address (Port %d)? ", x);
      if (acceptInput (userstr, USER_STR_LEN))
	{
	  result = sscanf (userstr, "%hhu.%hhu.%hhu.%hhu",
			   &iptmp[0], &iptmp[1], &iptmp[2], &iptmp[3]);
	}
      else
	result = 0;

      if (result == 4) {
	/* IP Addresses are stored little endian */
	epcs->ip_address[x] = grab32(iptmp);
      }

      print_ip (epcs->ip_address[x]);
    }
#ifndef PUBLIC_RELEASE
  printf ("Enter Flags: ");

  if (acceptInput (userstr, USER_STR_LEN))
    result = sscanf (userstr, "%hhi", &tmp);
  else
    result = 0;

  if (result == 1)
    epcs->cfg_flags = tmp;
  printf ("Flags set to 0x%X\n", epcs->cfg_flags);

  printf ("Enter serial no. string, no whitespace: ");
  if (acceptInput (userstr, USER_STR_LEN))
    result = sscanf (userstr, "%15s", tmpser);
  else
    result = 0;

  if (result == 1)
    strcpy ((char*)epcs->snumber, (char *)tmpser);

  x = strlen ((char *) epcs->snumber);
  if (epcs->snumber[x - 1] == '\n')
    epcs->snumber[x - 1] = '\0';

  printf ("Serial number set to: %s\n", epcs->snumber);

  printf ("Enter length of GPAK Binary ('0' if none): ");
  if (acceptInput (userstr, USER_STR_LEN))
    result = sscanf (userstr, "%i", &x);
  else
    result = 0;

  if (result == 1)
      epcs->gpak_len = x;

  printf ("Length set to %d\n", epcs->gpak_len);
#endif

  /* Date stuff */
  /* calendar_time will hold current time */
  calendar_time = time (NULL);
  epcs->mfg_date = calendar_time - libfb_get_ctime (f);

  epcs->crc16 = crc_16 ((unsigned char *) epcs, sizeof (EPCS_CONFIG) - 2);
  printf ("Calculated CRC (%ld bytes): 0x%X\n", sizeof (EPCS_CONFIG),
	  epcs->crc16);

  return 0;
}

int
main (int argc, char *argv[])
{
  int x, y, len = 0;
  int c;
  int fail = 0;
  extern char *optarg;
  extern int optind, optopt;
  EPCS_CONFIG config_struct;
  T_EPCS_INFO epcs_info;
  DOOF_STATIC_INFO dsi;
  char hex[5] = "0xab";
  char nicname[16];
  int crcenflag;
  char errstr[LIBFB_ERRBUF_SIZE];
  int rerror;
  FILE *binfile, *chkfile;

  printf ("Fonebridge Flash Utility, ver: %s", SW_VER);
#ifdef PUBLIC_RELEASE
  printf ("P");
#endif
  printf ("\n");
  printf ("Build Number: %d\n", BUILD_NUM);

  memset ((void *) &config_opts, 0, sizeof (config_opts));
  strcpy (nicname, "eth0");	/* Default to eth0 */
  config_opts.len = EPCS_SPAN;
  crcenflag = 1;

  init_rng ();

  /* Parse arguments */
  while ((c = getopt (argc, argv, ":hVvdbr:s:l:w:g:cm:i:")) != -1)
    {
      switch (c)
	{
#ifndef PUBLIC_RELEASE
	case 'b':
	  config_opts.addr_in_blocks = 1;
	  break;
#endif
	case 'h':
	  /* Help */
	  print_usage ();
	  exit (EXIT_SUCCESS);
	  break;

	case 'V':
	  /* Extra version info */
	  print_extra_version ();
	  exit (EXIT_SUCCESS);
	  break;
#ifndef PUBLIC_RELEASE
	case 's':
	  config_opts.start_addr = strtol (optarg, NULL, 0);
	  break;

	case 'l':
	  config_opts.len = strtol (optarg, NULL, 0);
	  break;
#endif
	case 'w':
	  config_opts.op = WR_OP;
	  strcpy (config_opts.filename, optarg);
	  break;

	case 'g':
	  config_opts.op = GPAK_OP;
	  strcpy (config_opts.filename, optarg);
	  /* Settings taken verbatim from Jai */
	  config_opts.addr_in_blocks = 1;
	  config_opts.start_addr = 10;
	  break;

#ifndef PUBLIC_RELEASE
	case 'r':
	  config_opts.op = RD_OP;
	  strcpy (config_opts.filename, optarg);
	  break;
#endif
	case 'c':
	  config_opts.op = CONFIG_OP;
	  break;

	case 'm':
	  /* Mac address */

	  for (x = 0, y = 0; x < 6; x++)
	    {
	      hex[2] = optarg[y++];
	      hex[3] = optarg[y++];

	      mac_dflt[0][x] = strtol (hex, NULL, 0);

	    }
	  break;

	case 'i':
	  //config_opts.if_num = strtol(optarg, NULL, 0);
	  strncpy (nicname, optarg, 16);
	  break;

	case 'v':
	  printf ("Setting verbosity to high\n");
	  config_opts.verbosity = 1;
	  break;
#ifndef PUBLIC_RELEASE
	case 'd':
	  crcenflag = 0;
	  break;
#endif


	}
    }

  /* Open libfb interface */
  f = libfb_init (nicname, LIBFB_ETHERNET_ON, errstr);
  if (f == NULL)
    {
      fprintf (stderr, "libfb: %s\n", errstr);
      exit (EXIT_FAILURE);
    }

  if (crcenflag)
    libfb_setcrc_on (f);
  else
    libfb_setcrc_off (f);

  printf ("-------------------------------\n");

  printf ("Source MAC: ");

  print_mac (libfb_getsrcmac (f));
  printf ("Using target MAC: ");
  print_mac (mac_dflt[0]);
  printf ("Probing target device on %s...\n", nicname);
  if (get_static_info (f, mac_dflt[0], &dsi) != 0)
    {
      printf ("Error probing target\n");
      goto exit_fail;
    }
  epcs_info.epcs_blocks = dsi.epcs_blocks;
  epcs_info.epcs_block_size = dsi.epcs_block_size;
  epcs_info.epcs_region_size = dsi.epcs_region_size;

#ifndef PUBLIC_RELEASE
  print_static_info (f, &dsi);
#else
  printf ("-------------------------------\n");
  printf ("Device found!\n");
  printf ("FB2 FW Version: %s\n", dsi.sw_ver);
  printf ("FB2 FW Compile date: %s\n", dsi.sw_compile_date);
  printf ("FB2 Build number: %d\n", grab16 ((unsigned char *) dsi.build_num));
  printf ("MAC[0]: ");
  print_mac (dsi.epcs_config.mac_addr);
  printf ("IP[0]: ");
  print_ip ((unsigned char *) dsi.epcs_config.ip_address[0]);
  printf ("IP[1]: ");
  print_ip ((unsigned char *) dsi.epcs_config.ip_address[1]);
  if (dsi.gpak_config.max_channels == 128)
    {
      x = 0;
      memcpy ((void *) &x, (void *) dsi.gpak_config.ver, 4);
      printf ("DSP Detected, FW ver: 0x%X\n", x);
    }
#endif

  if (config_opts.addr_in_blocks)
    config_opts.start_addr *= epcs_info.epcs_block_size;

  if (libfb_getcrc (f))
    printf ("CRC Checking enabled\n");
  else
    printf ("CRC Checking disabled\n");

  if (config_opts.op == CONFIG_OP)
    {
      rerror =
	read_blk (f, mac_dflt[0], (epcs_info.epcs_blocks - 2) * 65536,
		  sizeof (EPCS_CONFIG), (unsigned char *) &config_struct);

      if (rerror < 0)
	{
	  fprintf (stderr, "Error reading configuration block!\n");
	  goto exit_fail;
	}
      else if (rerror != sizeof (EPCS_CONFIG))
	{
	  fprintf (stderr, "Read configuration block but got wrong size %d\n",
		   rerror);
	  goto exit_fail;
	}

      if (fill_config_struct (&config_struct) != 0)
	goto exit_fail;

      printf ("Writing config struct to 0x%X\n",
	      (epcs_info.epcs_blocks - 2) * epcs_info.epcs_block_size);
      if (write_to_blk
	  (f, mac_dflt[0], 0, sizeof (EPCS_CONFIG),
	   (unsigned char *) &config_struct) < 0)
	{
	  printf ("Error writing config data to flash\n");
	  goto exit_fail;
	}
      if (start_blk_write (f, mac_dflt[0], epcs_info.epcs_blocks - 2))
	{
	  printf ("Error executing write blk command\n");
	  goto exit_fail;
	}

      goto exit_succ;
    }


  if (config_opts.filename[0] == 0)
    {
      printf ("No filename specified\n");
      goto exit_succ;
    }

  printf ("Summary:\n");
  printf ("Start address: 0x%X\n", config_opts.start_addr);

  if (config_opts.op == RD_OP)
    {
      printf ("Length: %d (0x%X) bytes\n", config_opts.len, config_opts.len);
      printf ("Read to file: %s\n", config_opts.filename);
    }
  else if (config_opts.op == WR_OP || config_opts.op == GPAK_OP)
    {
      printf ("Write from file: %s\n", config_opts.filename);
    }

  /* Setup file IO */

  binfile = NULL;

  if (config_opts.op == RD_OP)
    binfile = fopen (config_opts.filename, "w+");
  else if (config_opts.op == WR_OP || config_opts.op == GPAK_OP)
    binfile = fopen (config_opts.filename, "r+");

  if (!binfile)
    {
      printf ("Error opening bin file\n");
      goto exit_fail;
    }

  /* Process command */
  if (config_opts.op == WR_OP || config_opts.op == GPAK_OP)
    {
      printf ("Starting write op\n");
      len = write_from_bin (mac_dflt[0], config_opts.start_addr, binfile);
      if (!len)
	{
	  printf ("Error in writing from bin file\n");
	  goto exit_fail;
	}
      /* Read len bytes, dump file back to verify */
      chkfile = NULL;
      chkfile = fopen ("./chkfile.bin", "w+");
      if (!chkfile)
	{
	  perror ("Check file");
	  goto exit_fail;
	}
      printf ("Dumping checkfile...\n");
      if (read_to_bin (mac_dflt[0], config_opts.start_addr, len, chkfile))
	{
	  printf ("Error reading check file\n");
	  goto exit_fail;
	}
      rewind (chkfile);
      rewind (binfile);
      if (diff_file (chkfile, binfile))
	{
	  printf ("Error verifying check file\n");
	  goto exit_fail;
	}
      printf ("Done verifying.\n");
      if (config_opts.op == GPAK_OP)
	{
	  printf ("Setting GPAK file length to %d\n", len);
	  rerror = read_blk (f, mac_dflt[0], (epcs_info.epcs_blocks - 2) * 65536, sizeof (EPCS_CONFIG), (unsigned char *) &config_struct);

	  if (rerror < 0)
	    {
	      fprintf (stderr, "Error reading configuration block!\n");
	      goto exit_fail;
	    }
	  else if (rerror != sizeof (EPCS_CONFIG))
	    {
	      fprintf (stderr, "Read configuration block but got wrong size %d\n",
		       rerror);
	      goto exit_fail;
	    }


	  config_struct.gpak_len = len;
	  printf ("Writing config struct to 0x%X\n",
		  (epcs_info.epcs_blocks - 2) * epcs_info.epcs_block_size);
	  if (write_to_blk (f, mac_dflt[0], 0, sizeof (EPCS_CONFIG),
			    (unsigned char *) &config_struct) < 0)
	    {
	      printf ("Error writing config data to flash\n");
	      goto exit_fail;
	    }

	  if (start_blk_write (f, mac_dflt[0], epcs_info.epcs_blocks - 2))
	    {
	      printf ("Error executing write blk command\n");
	      goto exit_fail;
	    }
	}
    }
#ifndef PUBLIC_RELEASE
  else if (config_opts.op == RD_OP)
    {
      if (read_to_bin
	  (mac_dflt[0], config_opts.start_addr, config_opts.len, binfile))
	{
	  printf ("Error in reading to bin file\n");
	  goto exit_fail;
	}

    }
#endif

  if (config_opts.op == GPAK_OP && len != 0)
    {
      /* Read and store config data, and change gpak len */
      EPCS_CONFIG store;
      rerror =
	read_blk (f, mac_dflt[0], (epcs_info.epcs_blocks - 2) * 65536,
		  sizeof (EPCS_CONFIG), (unsigned char *) &store);
      if (rerror < 0)
	{
	  fprintf (stderr, "Error reading configuration block!\n");
	  goto exit_fail;
	}
      else if (rerror != sizeof (EPCS_CONFIG))
	{
	  fprintf (stderr, "Read configuration block but got wrong size %d\n",
		   rerror);
	  goto exit_fail;
	}


      store.gpak_len = len;
      store.crc16 =  crc_16 ((unsigned char *) &store, sizeof (EPCS_CONFIG) - 2);

      printf ("Calculated CRC (%ld bytes): 0x%X\n", sizeof (EPCS_CONFIG),
	      store.crc16);

      printf ("Re-Writing config structure\n");
      if (write_to_blk
	  (f, mac_dflt[0], 0, sizeof (EPCS_CONFIG),
	   (unsigned char *) &store) < 0)
	{
	  printf ("Error writing config data to flash\n");
	  goto exit_fail;
	}
      if (start_blk_write (f, mac_dflt[0], epcs_info.epcs_blocks - 2))
	{
	  printf ("Error executing write blk command\n");
	  goto exit_fail;
	}

      printf ("Success\n");
      goto exit_succ;
    }

exit_fail:
  fail = 1;
exit_succ:
  libfb_destroy (f);
  exit ((fail == 1) ? EXIT_FAILURE : EXIT_SUCCESS);
}

int
read_to_bin (unsigned char *mac, int address, int length, FILE * output)
{
  unsigned char buffer[1500];
  int x, addr;

  x = length;
  addr = address;
  while (x)
    {
      if (x >= 256)
	{
	  if (read_blk (f, mac, addr, 256, buffer) < 0)
	    {
	      printf ("Error, dying\n");
	      return -1;
	    }
	  fwrite (buffer, 1, 256, output);
	  x -= 256;
	  addr += 256;
	}
      else
	{
	  if (read_blk (f, mac, addr, x, buffer) < 0)
	    {
	      printf ("Error, dying\n");
	      return -1;
	    }
	  fwrite (buffer, 1, x, output);
	  x = 0;
	}


    }

  return 0;

}

int
write_from_bin (unsigned char *mac, int address, FILE * input)
{
  unsigned int x, blk, addr;
  int len = 0;

  long y;
  unsigned char file_buffer[EPCS_BLK_SIZE];

  blk = address / EPCS_BLK_SIZE;
  addr = address;

  //file_buffer = malloc(EPCS_BLK_SIZE);

  if (!file_buffer)
    {
      printf ("Error allocating room for file buffer\n");
      return 1;
    }

  show_warning ();

  while (1)
    {
      /* First clear buffer */
      memset ((void *) file_buffer, 0xff, EPCS_BLK_SIZE);
      /* Read in block from file */
      x = fread ((void *) file_buffer, 1, EPCS_BLK_SIZE, input);
      /* 'x' bytes have been read, but we are still going to issue a write for whole block */
      //addr -= x;
      if (!x)
	{
	  break;
	}
      len += x;

      printf ("Read in %d bytes, Block %d\n", x, blk);

      /* Issue write to buffer commands in multiples of 256 */
      for (y = 0; y < EPCS_BLK_SIZE; y += 256)
	{
	  //printf("Writing to Block %d Offset %d / %d\n",blk,y,EPCS_BLK_SIZE);
	  if (write_to_blk (f, mac, y, 256, file_buffer + y) < 0)
	    {
	      printf ("Error writing to Block %d, Offset: %ld (0x%lX)\n", blk,
		      y, y);
	      //free(file_buffer);
	      return 0;
	    }

	}
      //printf("Executing write cmd\n");

      /* Done writing to block buffer, execute write cmd */
      if (start_blk_write (f, mac, blk) < 0)
	{
	  printf ("Error executing write on block %d\n", blk);
	  return 0;
	}

      blk++;

      //printf("Done blk\n");

    }

  //free(file_buffer);

  printf ("Wrote %d bytes (%d KB)\n", len, len / 1024);
  return len;

}

int
diff_file (FILE * file1, FILE * file2)
{
  unsigned char *buf1, *buf2;
  int x, y, z, retval = 0;
  int blk = 0;

  buf1 = malloc (EPCS_BLK_SIZE);
  buf2 = malloc (EPCS_BLK_SIZE);

  if (!buf1 || !buf2)
    {
      perror ("Error allocating buffers");
      return 0;
    }

  printf ("Starting verify\n");

  while (1)
    {
      x = fread ((void *) buf1, 1, EPCS_BLK_SIZE, file1);
      y = fread ((void *) buf2, 1, EPCS_BLK_SIZE, file2);

      if (x != y)
	{
	  retval = 1;
	  break;
	}
      if (!x)
	break;

      printf ("Checking block %d\n", blk);

      for (z = 0; z < y; z++)
	{
	  if (buf1[z] != buf2[z])
	    {
	      printf ("Error verifying block %d, offset %d\n", blk, z);
	      retval = 1;
	      break;
	    }
	}

      blk++;

    }

  free (buf1);
  free (buf2);

  return retval;


}
