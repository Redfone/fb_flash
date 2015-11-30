#if HAVE_CONFIG_H
# include <config.h>
#endif

#if STDC_HEADERS || HAVE_STDLIB_H
# include <stdlib.h>
#endif

#if HAVE_STRING_H
# include <string.h>
#elif HAVE_STRINGS_H
# include <strings.h>
#endif

#if !HAVE_STRCHR
# ifndef strchr
#  define strchr index
# endif
#endif

#if HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif

#if HAVE_LIBFB_FB_LIB_H
# include <libfb/fb_lib.h>
#endif

/* readline library */
#if HAVE_READLINE_READLINE_H
# include <readline/readline.h>
#endif

#ifdef HAVE_READLINE_HISTORY_H
# include <readline/history.h>
#endif


#ifdef VERSION
# define SW_VER FBFLASH_VERSION		/* from autotools */
#endif

#ifndef whitespace
#define whitespace(c) (((c) == ' ') || ((c) == '\t'))
#endif
typedef int rl_icpfunc_t (char *);

#include "ver.h"

static libfb_t *f;
int done = 0;

char **fbudp_completion __P ((const char *, int, int));

/********** COMMANDS **********/
int com_help __P ((char *));
int com_quit __P ((char *));

int com_getinfo __P ((char *));
int com_readdsp __P ((char *));
int com_writedsp __P ((char *));
int com_custom __P ((char *));
int com_ecsetchantype __P ((char *));
int com_ecsetsingle __P ((char *));
int com_ecsetrange __P ((char *));
int com_resetnios __P ((char *));
int com_readidt __P ((char *));
int com_readmem __P ((char *));
int com_dumpmem __P ((char *));
int com_readmem32 __P ((char *));
int com_readspi __P ((char *));
int com_writespi __P ((char *));
int com_tdmreg __P ((char *));
int com_tdmoectl __P ((char *));
int com_tdmoemac __P ((char *));
int com_tdmlb __P ((char *));
int com_clksel __P ((char *));
int com_lboconfig __P ((char *));
int com_idtconfig __P ((char *));
int com_dspaf __P ((char *));
int com_dspchantype __P ((char *));
int com_dspfirsegs __P ((char *));
int com_dspusage __P ((char *));
int com_dspcomp __P ((char *));
int com_dsptaplen __P ((char *));
int com_getstats __P ((char *));
int com_getgparms __P ((char *));
int com_readrbs __P ((char *));
int com_rbsspy __P ((char *));
int com_lfsrchk __P ((char *));
int com_setprio __P ((char *));
int com_writeidt __P ((char *));
int com_pmonstats __P ((char *));
int com_idtlinkstat __P ((char *));

typedef struct
{
  char *name;			/* User-printable function name */
  rl_icpfunc_t *func;		/* The actual function */
  char *doc;			/* Documentation */
} COMMAND;

COMMAND commands[] = {
  {"help", com_help, ": Display this text"},
  {"?", com_help, ": Synonym for help"},
  {"getinfo", com_getinfo, ": Get DOOF static info on device"},
  {"readdsp", com_readdsp, "<addr> <len>: Read len words at address"},
  {"writedsp", com_writedsp, "<addr> <value>: Write 4-byte value at address"},
  {"custom", com_custom, "<cmd> <param>: Send a custom command"},
  {"dumpmem", com_dumpmem, "<addr> <len> <file>: Dump memory to bin file"},
  {"ecsetchantype", com_ecsetchantype,
   "<chantype> <group1> <group2> <group3> <group4>: Set indicated channels to chantype (0=data, 1=Voice A, 2=Voice B, 3=Off)"},
  {"ecsetsingle", com_ecsetsingle,
   "<chantype> <channel>: Set channel to chantype"},
  {"ecsetrange", com_ecsetrange,
   "<chantype> <start chan> <end chan>: Set range of channels (inclusive) to chantype"},
  {"resetnios", com_resetnios, ": Reset the NIOS CPU"},
  {"readidt", com_readidt, "<span> <addr> <len>: Read len bytes from 'addr' on 'span'"},
  {"readmem", com_readmem, "<addr> <len>: Read len bytes from NIOS addr"},
  {"readmem32", com_readmem32, "<addr> <len>: Read len words from NIOS addr"},
  {"readspi", com_readspi, "<dev> <addr>: Read SPI register addr from dev"},
  {"writespi", com_writespi,
   "<dev> <addr> <val>: Write val to SPI register addr on dev"},
  {"tdmoectl", com_tdmoectl,
   "<val>: Start/stop TDMoE traffic (0=stop, 1=on, 2=reflect)"},
  {"tdmoemac", com_tdmoemac,
   "<port> <MAC>: Set the MAC address and MAC port (0 or 1) for TDMoE traffic"},
  {"lboconfig", com_lboconfig,
   "<lboval0> <1> <2> <3>: Set the LBO for each span"},
  {"idtconfig", com_idtconfig,
   "<span 0> <1> <2> <3>: Configure IDT spans 0-3 using span mode masks"},
  {"idtlinkstat", com_idtlinkstat,
	  "<span>: Read link status"},
  {"dspaf", com_dspaf, "<af>: Set the Adaptation Frequency"},
  {"dspusage", com_dspusage, ": Obtain DSP CPU usage"},
  {"dspchantype", com_dspchantype, " : Read DSP channel types"},
  {"dspcomp", com_dspcomp,
   "<type>: Set DSP Companding type (where type is either `alaw' or `ulaw')"},
  {"dspfirsegs", com_dspfirsegs, "<fir-segs>: Set DSP FIR Segments"},
  {"dsptaplen", com_dsptaplen,
   "<tap-length>: Set DSP Tap-length in mS"},
  {"getstats", com_getstats, ": Obtain statistics"},
  {"getgparms", com_getgparms, " : Get GPAK Flash parameters"},
  {"readrbs", com_readrbs,
   "<span>: Read TX and RX RBS bytes from IDT buffers"},
  {"rbsspy", com_rbsspy, "<on/off> <span>: Turn on/off spying on span"},
  {"setprio", com_setprio, "<span 1> <2> <3> <4>: Set priorities of each span (1=master, 0=slave)"},
  {"tdmlb", com_tdmlb, "<cmd> <val>: Execute cmd to TDM LB PIO"},
  {"clksel", com_clksel, "<cmd> <val>: Execute cmd to CLKSEL PIO"},
  {"tdmreg", com_tdmreg, "<cmd><core><off><mask>: Execute cmd on TDM Core according to mask"},
  {"lfsrchk", com_lfsrchk,
   "<mask>: Turn on/off LFSR checking on TDM cores via mask"},
  {"writeidt", com_writeidt, "<span> <addr> <val>: Write to IDT register"},
  {"pmonstats", com_pmonstats, "<span>: Read the PMON stats from IDT"},
  {"quit", com_quit, ": Quit fb_udp"},
  {NULL, NULL, NULL}
};

/* Strip whitespace from the start and end of STRING.  Return a pointer
   into STRING. */
char *
stripwhite (string)
     char *string;
{
  register char *s, *t;

  for (s = string; whitespace (*s); s++)
    ;

  if (*s == 0)
    return (s);

  t = s + strlen (s) - 1;
  while (t > s && whitespace (*t))
    t--;
  *++t = '\0';

  return s;
}
void
print_hex32 (uint32_t *buf, unsigned int len)
{
  int x;
  for (x = 0; x < len; x++)
    {
      if (x % 4 == 0)
	printf ("\n0x%02X: ", x);
      printf ("%08X ", buf[x]);
    }
  printf ("\n");
}

void
print_hex (const char *buf, unsigned int len)
{
  int x;
  for (x = 0; x < len; x++)
    {
      if (x % 16 == 0)
	printf ("\n0x%02X: ", x);
      printf ("%02X ", (unsigned char) buf[x]);
    }
  printf ("\n");
}

int
com_lfsrchk (word)
     char *word;
{
	uint8_t span;
  int retval;

  if (sscanf (word, "%hhi", &span) != 1)
	  return -1;
  retval = custom_cmd (f, DOOF_CMD_LFSR_CHK, span, NULL, 0);
  printf ("Return code: 0x%02X\n", retval);
  PRINT_MAPPED_ERROR_IF_FAIL (retval);
  return retval;
}
int
com_tdmlb (word)
     char *word;
{
  uint8_t param, val;
  int retval;

  if (sscanf (word, "%hhi %hhi", &param, &val) != 2)
    return -1;

  retval = custom_cmd_reply (f, DOOF_CMD_TDM_LB_SEL, param, (char *) &val, 1,
			    (char *) &val, 1);
  printf ("TDM Reg reads: 0x%X\n", val);
  PRINT_MAPPED_ERROR_IF_FAIL (retval);
  return retval;
}
int
com_clksel (word)
     char *word;
{
  uint8_t param;
  unsigned long long val;
  int x;
  int retval;

  if (sscanf (word, "%hhi %lli", &param, &val) != 2)
    return -1;

  retval = custom_cmd_reply (f, DOOF_CMD_CLKSEL_PIO, param, (char *) &val, 4,
			    (char *) &val, 4);
  printf ("CLKSEL reads: 0x%X\n", val);
  if (val & 0x1000) printf("WPLL Timing Source: FREF Buffer disabled\n");
  /* Check if WPLL_REF has 2nd-stage mux override set */
  else
  if (val & 0x40) printf("WPLL Timing Source: Internal 8.192MHz, %s\n",(val&0x80)?"2.048MHz":"1.544MHz");
  else
	 printf ("WPLL Timing Source: Span %d, %s\n",(unsigned int) val&0x3,(val&0x80)?"2.048MHz":"1.544MHz");
  for (x=0;x<4;x++)
  {
	  printf("IDT[%d]: %s (%s)\n",x,(val & (1<<(x+8)))?"Slave":"Master",
			  (val & (1<<(x+2)))?"2.048MHz":"1.544MHz");
  }
  retval = readmem (f, 0x900, 1, (char *) &param);
  printf ("WPLL is %s\n",(param&0x1)?"Locked":"Unlocked");
  PRINT_MAPPED_ERROR_IF_FAIL (retval);
  return retval;
}
int
com_writeidt (word)
     char *word;
{
  uint8_t span,buf[2];

  int retval;

  if (sscanf (word, "%hhi %hhi %hhi", &span, &buf[0], &buf[1]) != 3)
    return -1;

  retval = custom_cmd (f, DOOF_CMD_IDT_WRITE_REG, span, (char *) buf, 2);
  printf ("Return code: 0x%02X\n", retval);
  PRINT_MAPPED_ERROR_IF_FAIL (retval);
  return retval;
}
int
idt_read_pmon(unsigned char link, unsigned char off)
{
	uint8_t buf[2];
	int retval;
	/* Write the PMON IDR address */
	buf[0] = 0xe;
	buf[1] = (off & 0xf) | ((link % 2) << 5);
	retval = custom_cmd (f, DOOF_CMD_IDT_WRITE_REG, link % 2, (char *) buf, 2);
  	retval = readidt (f, link % 2, 0xf, 1, buf);
	printf("PMON[%d,%d] = 0x%X\n",link,off,buf[0]);
	return buf[0];
}
int
com_idtlinkstat (word)
	char *word;
{
	int retval;
	uint8_t span,x,buf[2],mode;
	if (sscanf (word, "%hhi", &span) != 1) return -1;
	retval = readidt (f, span, 0xb9, 1, buf);
	retval = readidt (f, span, 0x20, 1, &mode);
	printf ("IDT[%d] LSR: 0x%X (link %s, %s mode)\n",
			span,buf[0],buf[0]?"down":"up",
			(mode&0x1)?"T1":"E1");
	PRINT_MAPPED_ERROR_IF_FAIL (retval);
	return retval;
}
int
com_pmonstats (word)
	char *word;
{
	int retval;
	uint8_t span,x,buf[2];
	if (sscanf (word, "%hhi", &span) != 1)
		return -1;
	/* Update counters */
	buf[0] = 0xc2; buf[1] = 0;
	retval = custom_cmd (f, DOOF_CMD_IDT_WRITE_REG, span, (char *) buf, 2);
	buf[0] = 0xc2; buf[1] = 2;
	retval = custom_cmd (f, DOOF_CMD_IDT_WRITE_REG, span, (char *) buf, 2);
	printf("Retrieiving PMON stats from span %d\n",span);
	for (x=0;x<0xf;x++)
		idt_read_pmon(span, x);
	return retval;
}
int
tdmreg (uint8_t cmd, uint8_t core, uint8_t off, uint32_t mask)
{
  int retval;
  uint8_t buffer[10], recv[4];
  
  buffer[0] = core;
  buffer[1] = off;
  store32 (mask, buffer + 2);

  printf("TDM Reg on Core %d, Offset %d, Mask 0x%X\n",core,off,mask);
  printf("Command: %s\n",(cmd == DOOF_CMD_TDM_REGCTL_NOP) ? "No-op" :
		  (cmd == DOOF_CMD_TDM_REGCTL_SET) ? "Set bits" :
		  (cmd == DOOF_CMD_TDM_REGCTL_CLR) ? "Clear bits" :
		  (cmd == DOOF_CMD_TDM_REGCTL_FORCE) ? "Force value" :
		  "Unknown");

  retval = custom_cmd_reply (f, DOOF_CMD_TDM_REGCTL,
		       cmd, (char *) buffer, 6, (char *) recv, 4);
  PRINT_MAPPED_ERROR_IF_FAIL (retval);
  printf("Register reads: 0x%X\n", grab32(recv));
  return retval;
}

int
com_tdmreg (word)
     char *word;
{
  uint8_t core,cmd,off;
  unsigned long long mask;

  int retval;

  if (sscanf (word, "%hhi %hhi %hhi %lli",&cmd,&core,&off,&mask) != 4)
    return -1;

  retval = tdmreg(cmd,core,off,(uint32_t) mask);
  return retval;
}

int
com_rbsspy (word)
     char *word;
{
  uint8_t state, span;

  int retval;

  if (sscanf (word, "%hhu %hhu", &state, &span) != 2)
    return -1;

  printf ("RBS Spy Control (%d) to span %d\n", state, span);

  retval = custom_cmd (f, DOOF_RBS_SPY_CTL,
		       ((state & 0x0F) << 4) | (span & 0x0F), NULL, 0);
  PRINT_MAPPED_ERROR_IF_FAIL (retval);
  return retval;
}

int
com_readrbs (word)
     char *word;
{
  uint8_t span;
  char reply[64];

  int retval;

  if (sscanf (word, "%hhu", &span) != 1)
    return -1;

  retval = custom_cmd_reply (f, DOOF_GET_RBS, span, NULL, 0, reply, 64);
  if (retval == FBLIB_ESUCCESS)
    {
      printf ("TX RBS\n---------\n");
      print_hex (reply, 32);
      printf ("RX RBS\n---------\n");
      print_hex (reply + 32, 32);
    }
  else
    PRINT_MAPPED_ERROR_IF_FAIL (retval);

  return retval;
}

int
com_getstats (word)
     char *word;
{
  int retval;
  u_int64_t longval, *longptr;
  u_int64_t minutes,seconds;

  DOOF_STATS stats;
  retval = custom_cmd_reply (f, DOOF_GET_STATS, 0, NULL, 0,
			     (char *) &stats, sizeof (DOOF_STATS));
  if (retval == FBLIB_ESUCCESS)
    {
      register int i;
      longptr = (u_int64_t *) stats.ticks;
      longval = *(longptr);
      seconds = longval / 50000000;
      minutes = seconds / 60;
      seconds %= 60;
      printf ("System Ticks: %lld (0x%llX, %lldm%llds)\n",longval,longval,minutes,seconds);
      for (i = 0; i < MAC_NUM; i++)
	{
	  printf ("MAC[%d] Rx/Tx/Drop Packets: %d / %d / %d\n",
		  i, stats.mac_rxcnt[i], stats.mac_txcnt[i], stats.mac_rxdrop[i]);
	}
      for (i = 0; i < TDM_STREAM_COUNT; i++)
	{
	  float cnt;
	  if (stats.lfsr_cnt[i])
	    cnt =
	      (float) (((float) stats.lfsr_err[i]) /
		       ((float) stats.lfsr_cnt[i]) * 100);
	  else
	    cnt = 0;
	  printf ("TDM_STREAM[%d]: LFSR Check Errors: %d / %d (%.2f %%)\n", i,
		  stats.lfsr_err[i], stats.lfsr_cnt[i], cnt);
	}
    }
  else
    PRINT_MAPPED_ERROR_IF_FAIL (retval);

  return retval;
}

int
com_dspcomp (word)
     char *word;
{
  uint8_t type;

  int retval;

  if (!strcmp (word, "ulaw"))
    type = DSP_COMP_TYPE_ULAW;
  else if (!strcmp (word, "alaw"))
    type = DSP_COMP_TYPE_ALAW;
  else
    {
      printf ("Invalid companding type selected\n");
      return -1;
    }

  retval = custom_cmd (f, DOOF_CMD_EC_SETPARM, DOOF_CMD_EC_SETPARM_COMP_TYPE,
		  (char *) &type, 1);
  PRINT_MAPPED_ERROR_IF_FAIL (retval);
  return retval;
}

int
com_dspusage (word)
     char *word;
{
  int retval;
  char reply[24];
  retval =
    custom_cmd_reply (f, DOOF_CMD_GET_DSP_USAGE, 0, NULL, 0, reply, 24);

  if (retval == FBLIB_ESUCCESS)
    {
      char *ptr = reply;
      register int i;
      for (i = 0; i < 6; i++)
	{
	  int val = grab32 ((unsigned char *) ptr);
	  printf ("Usage[%d]: %d (%lf%%)\n", i, val, (double) (val / 10));
	  ptr += 4;
	}
    }
  else
    {
      PRINT_MAPPED_ERROR_IF_FAIL (retval);
      printf ("Error obtaining DSP usage\n");
    }

  return retval;
}



int
com_dspchantype (word)
     char *word;
{
  int retval;

  printf ("Reading DSP Channels...\n");
  GPAK_FLASH_PARMS *flparm = malloc (sizeof (GPAK_FLASH_PARMS));
  if (flparm)
    {
      retval = fblib_get_gpakparms (f, flparm);
      if (retval != FBLIB_ESUCCESS)
	{
	  PRINT_MAPPED_ERROR_IF_FAIL (retval);
	  printf ("Error reading GPAK Flash parameters\n");
	  return retval;
	}
      else
	{
	  print_hex ((char *) flparm->dsp_chan_type, 128);
	  free (flparm);
	  return 0;
	}
    }
  return -1;
}


int
com_getgparms (word)
     char *word;
{
  int retval;

  printf ("Reading GPAK Flash parameters\n");
  GPAK_FLASH_PARMS *flparm = malloc (sizeof (GPAK_FLASH_PARMS));
  if (flparm)
    {
      retval = fblib_get_gpakparms (f, flparm);
      if (retval != FBLIB_ESUCCESS)
	{
	  PRINT_MAPPED_ERROR_IF_FAIL (retval);
	  printf ("Error reading GPAK Flash parameters\n");
	  return retval;
	}
      else
	{
	  printf ("Comp. type: %s, Taplen: %d, AF: %d%%, FIR segs: %d, FIR seglen: %d\n",
		  flparm->dsp_companding_type == DSP_COMP_TYPE_ULAW ? "u-law":
		  flparm->dsp_companding_type == DSP_COMP_TYPE_ALAW ? "a-law":"unknown", flparm->taplen, flparm->adapt_freq, flparm->fir_segs,
		  flparm->fir_seglen);
	  free (flparm);
	  return 0;
	}
    }
  return -1;
}


int
com_dspaf (word)
     char *word;
{
  int retval;
  char af;

  if (sscanf (word, "%hhi", &af) != 1)
    return -1;

  retval =
    custom_cmd (f, DOOF_CMD_EC_SETPARM, DOOF_CMD_EC_SETPARM_ADAPT_FREQ, &af,
		1);
  PRINT_MAPPED_ERROR_IF_FAIL (retval);
  return retval;
}
int
com_dspfirsegs (word)
     char *word;
{
  int retval;
  char af;

  if (sscanf (word, "%hhi", &af) != 1)
    return -1;

  retval =
    custom_cmd (f, DOOF_CMD_EC_SETPARM, DOOF_CMD_EC_SETPARM_FIR_SEGS, &af,
                1);
  PRINT_MAPPED_ERROR_IF_FAIL (retval);
  return retval;
}

int
com_dsptaplen (word)
     char *word;
{
  int retval;
  char taplen;

  if (sscanf (word, "%hhi", &taplen) != 1)
    return -1;

  retval =
    custom_cmd (f, DOOF_CMD_EC_SETPARM, DOOF_CMD_EC_SETPARM_TAPLEN, &taplen,
		1);
  PRINT_MAPPED_ERROR_IF_FAIL (retval);
  return retval;
}

int
com_idtconfig (word)
     char *word;
{
  uint8_t smode[4];

  int retval;

  if (sscanf (word, "%hhi %hhi %hhi %hhi",
	      &smode[0], &smode[1], &smode[2], &smode[3]) != 4)
    return -1;

  retval = config_fb_udp (f, smode);
  PRINT_MAPPED_ERROR_IF_FAIL (retval);
  return retval;
}


int
com_setprio (word)
     char *word;
{
  uint8_t prio[4],reply[4];
  int retval;

  if (sscanf (word, "%hhi %hhi %hhi %hhi",
              &prio[0], &prio[1], &prio[2], &prio[3]) != 4)
    return -1;

  retval = custom_cmd_reply (f, DOOF_CMD_SET_PRIORITY, 0xf, (char *) prio, 4, (char *) reply, 4);
  PRINT_MAPPED_ERROR_IF_FAIL (retval);
  return retval;
}

int
com_lboconfig (word)
     char *word;
{

  uint8_t lbo[4];
  uint8_t smode[] = { 2, 2, 2, 2 };	/* 4 T1 */

  int retval;

  if (sscanf (word, "%hhi %hhi %hhi %hhi",
	      &lbo[0], &lbo[1], &lbo[2], &lbo[3]) != 4)
    return -1;

  retval = config_fb_udp_lbo (f, smode, lbo);
  PRINT_MAPPED_ERROR_IF_FAIL (retval);
  return retval;
}

int
com_writespi (word)
     char *word;
{
  uint8_t device, address;
  char value;

  int retval;

  if (sscanf (word, "%hhi %hhi %hhi", &device, &address, &value) != 3)
    return -1;

  printf ("Write 0x%02X to 0x%02X on device %d\n", value, address, device);
  retval = writespi (f, device, address, 1, &value);
  PRINT_MAPPED_ERROR_IF_FAIL (retval);
  return retval;
}

int
com_tdmoemac (word)
     char *word;
{
  char mac[6];
  uint8_t port;
  int num_to_skip;
  int retval;

  if (sscanf (word, "%hhu%n", &port, &num_to_skip) != 1)
    {
      fprintf (stderr, "Invalid arguments.\n");
      return -1;
    }

  word += ++num_to_skip; /* must also skip the implied white space */
  printf ("Port %d\n", port);

  if (strlen(word) < 5 || parse_mac ( word, (unsigned char*)mac ) != 0)
    {
      fprintf (stderr, "Invalid arguments.\n");
      return -1;
    }

  printf ("MAC Address: ");
  print_mac ((unsigned char *) mac);

  retval = custom_cmd (f, DOOF_CMD_TDMOE_DSTMAC, port, mac, 6);
  printf ("Return code: 0x%02X\n", retval);
  PRINT_MAPPED_ERROR_IF_FAIL (retval);
  return retval;
}

int
com_tdmoectl (word)
     char *word;
{
  uint8_t cmd;

  int retval;

  if (sscanf (word, "%hhi", &cmd) != 1)
    return -1;

  retval = custom_cmd (f, DOOF_CMD_TDMOE_TXCTL, cmd, NULL, 0);
  printf ("Return code: 0x%02X\n", retval);
  PRINT_MAPPED_ERROR_IF_FAIL (retval);
  return retval;
}

int
com_readspi (word)
     char *word;
{
#define SPI_REPLY_LEN 1
  uint8_t device, address;
  char reply[SPI_REPLY_LEN];

  int retval;

  if (sscanf (word, "%hhi %hhi", &device, &address) != 2)
    return -1;

  printf ("Read 0x%02X from device %d\n", address, device);

  retval = readspi (f, device, address, SPI_REPLY_LEN, reply);
  if (retval == FBLIB_ESUCCESS)
    printf ("Return code: 0x%02X\n", reply[0]);
  else
    PRINT_MAPPED_ERROR_IF_FAIL (retval);
  return retval;
}

int
com_readidt (word)
     char *word;
{
  unsigned long long tmplong;
  uint32_t addr, len;
  uint8_t span;
  int retval;

  /* Read in address and length */
  if (sscanf (word, "%hhu %lli %i", &span, &tmplong, &len) != 3)
    return -1;

  addr = tmplong & 0xFFFFFFFF;

  printf ("%d bytes @ 0x%08X, span %d\n", len, addr, span);

  char charbuf[len];
  retval = readidt (f, span, addr, len, charbuf);
  if (retval == FBLIB_ESUCCESS)
    print_hex (charbuf, len);
  else
    PRINT_MAPPED_ERROR_IF_FAIL (retval);

  return retval;
}
int
com_readmem32 (word)
     char *word;
{
  unsigned long long tmplong;
  uint32_t addr, len;

  int retval;

  /* Read in address and length */
  if (sscanf (word, "%lli %i", &tmplong, &len) != 2)
    return -1;

  addr = tmplong & 0xFFFFFFFF;

  printf ("%d words @ 0x%08X\n", len, addr);

  uint32_t intbuf[len];
  retval = readmem32 (f, addr, len, intbuf);
  if (retval == FBLIB_ESUCCESS)
    print_hex32 (intbuf, len);
  else
    PRINT_MAPPED_ERROR_IF_FAIL (retval);

  return retval;
}

int
com_readmem (word)
     char *word;
{
  unsigned long long tmplong;
  uint32_t addr, len;

  int retval;

  /* Read in address and length */
  if (sscanf (word, "%lli %i", &tmplong, &len) != 2)
    return -1;

  addr = tmplong & 0xFFFFFFFF;

  printf ("%d bytes @ 0x%08X\n", len, addr);

  char charbuf[len];
  retval = readmem (f, addr, len, charbuf);
  if (retval == FBLIB_ESUCCESS)
    print_hex (charbuf, len);
  else
    PRINT_MAPPED_ERROR_IF_FAIL (retval);

  return retval;
}

#define DUMPMEM_BLK_SZ 512
int
com_dumpmem (word)
     char *word;
{
  unsigned long long tmplong;
  uint32_t addr, len, writelen;
  char fname[128];
  FILE *binfile;
  char charbuf[DUMPMEM_BLK_SZ];
  int retval,x;

  /* Read in address and length */
  if (sscanf (word, "%lli %i %127s", &tmplong, &len, fname) != 3)
    return -1;
  addr = tmplong & 0xFFFFFFFF;
  printf ("%d bytes @ 0x%X to %s\n", len, addr, fname);
  binfile = fopen (fname, "w");
  if (!binfile)
    {
      perror ("Error opening binary file\n");
      return -1;
    }
  retval = x = 0;
  while (len)
    {
      if (len > DUMPMEM_BLK_SZ)
	writelen = DUMPMEM_BLK_SZ;
      else
	writelen = len;

      retval = readmem (f, addr, writelen, charbuf);

      if (retval == FBLIB_ESUCCESS)
	fwrite (charbuf, 1, writelen, binfile);
      else
	{
	  PRINT_MAPPED_ERROR_IF_FAIL (retval);
	  printf("Failed on block %d (0x%X), len: %d\n",x,addr,
		 writelen);
	  /* Retry again */
	  continue;
	}

      len -= writelen;
      addr += writelen;
      x++;
      usleep(500);
  
    }

  fclose (binfile);
  return retval;
}

int
com_resetnios (word)
     char *word;
{
  int retval;

  printf ("Resetting NIOS\n");
  retval = custom_cmd (f, DOOF_CMD_RESET, 0, NULL, 0);
  printf ("Return code: 0x%02X\n", retval);
  PRINT_MAPPED_ERROR_IF_FAIL (retval);

  return retval;
}

int
com_ecsetrange (word)
     char *word;
{
  uint8_t cmd;
  uint32_t mask[4];

  unsigned int start, end;
  register int i;

  int retval;

  if (sscanf (word, "%hhi %u %u", &cmd, &start, &end) != 3)
    return -1;

  memset (mask, 0, 4 * sizeof (uint32_t));

  /* Convert channels into bitmask */
  for (i = 0; i < 128; i++)
    {
      if ((i >= start) && (i <= end))
	mask[i / 32] |= (1 << (i % 32));
    }

  retval = ec_set_chantype (f, cmd, mask);
  PRINT_MAPPED_ERROR_IF_FAIL (retval);
  return retval;
}

int
com_ecsetsingle (word)
     char *word;
{
  uint8_t cmd;
  uint32_t mask[4];
  unsigned int chan;
  register int i = 0;

  int retval;

  if (sscanf (word, "%hhi %i", &cmd, &chan) != 2)
    return -1;

  memset (mask, 0, 4 * sizeof (uint32_t));

  /* Convert channel to bitmask */
  while (chan >= 32)
    {
      chan -= 32;
      i++;
    };
  mask[i] = (1 << chan);

  retval = ec_set_chantype (f, cmd, mask);
  PRINT_MAPPED_ERROR_IF_FAIL (retval);
  return retval;
}

int
com_ecsetchantype (word)
     char *word;
{
  uint8_t cmd;
  uint32_t group[4];
  unsigned long long int group_long[4];

  int retval;
  register int i;

  if ((sscanf (word, "%hhi %lli %lli %lli %lli", &cmd,
	       &group_long[0], &group_long[1], &group_long[2],
	       &group_long[3])) != 5)
    return -1;

  for (i = 0; i < 4; i++)
    group[i] = group_long[i] & 0xFFFFFFFF;

  printf ("Setting groups 0x%08X 0x%08X 0x%08X 0x%08X to %d\n",
	  group[0], group[1], group[2], group[3], cmd);


  retval = ec_set_chantype (f, cmd, group);
  PRINT_MAPPED_ERROR_IF_FAIL (retval);

  return retval;
}

int
com_custom (word)
     char *word;
{
  uint8_t cmd;
  uint8_t param;

  int retval;

  if (sscanf (word, "%hhi %hhi", &cmd, &param) != 2)
    return -1;

  printf ("Command: %d, Param %d\n", cmd, param);
  retval = custom_cmd (f, cmd, param, NULL, 0);
  PRINT_MAPPED_ERROR_IF_FAIL (retval);

  return retval;
}

int
com_writedsp (word)
     char *word;
{
  unsigned int addr;
  uint32_t val;

  int retval;

  if (sscanf (word, "%i %i", &addr, &val) != 2)
    return -1;

  printf ("Writing address: 0x%04X\n", addr);
  printf ("Value: 0x%08X\n", val);

  retval = writedsp (f, addr, 1, &val);
  PRINT_MAPPED_ERROR_IF_FAIL (retval);
  return retval;
}

int
com_readdsp (word)
     char *word;
{
  unsigned int addr, len;

  int retval;

  /* Read in address and length */
  if (sscanf (word, "%i %i", &addr, &len) != 2)
    return -1;

  printf ("addr: %d\nlen: %d\n", addr, len);

  unsigned int intbuf[len];
  retval = readdsp (f, addr, len, intbuf);
  if (retval == FBLIB_ESUCCESS)
    {
      print_hex32(intbuf, len);
    }
  else
    PRINT_MAPPED_ERROR_IF_FAIL (retval);

  return retval;

}

int
com_getinfo (arg)
     char *arg;
{
  fblib_err retval;
  DOOF_STATIC_INFO dsi;
  retval = udp_get_static_info (f, &dsi);
  PRINT_MAPPED_ERROR_IF_FAIL (retval);
  if (retval == 0)
    {
      /* Success, print */
      print_static_info(f,&dsi);
    }
  return 0;
}

int
com_quit (arg)
     char *arg;
{
  done = 1;
  return 0;
}

int
com_help (arg)
     char *arg;
{
  register int i;
  int printed = 0;
  printf ("Fonebridge debug tool, ver: " SW_VER "\n");
  printf ("---------------------------------\n");

  for (i = 0; commands[i].name; i++)
    {
      if (!*arg || (strcmp (arg, commands[i].name) == 0))
	{
	  printf ("%s %s\n", commands[i].name, commands[i].doc);
	  printed++;
	}
    }

  if (!printed)
    {
      printf ("No commands match `%s'. Possibilities are:\n", arg);

      for (i = 0; commands[i].name; i++)
	{
	  /* six columns */
	  if (printed == 6)
	    {
	      printed = 0;
	      printf ("\n");
	    }

	  printf ("%s\t", commands[i].name);
	  printed++;
	}

      if (printed)
	printf ("\n");
    }

  return 0;
}

/** \var line_read A static variable for holding the line. */
static char *line_read = (char *) NULL;

#if !HAVE_LIBREADLINE
char *
manual_gets ()
{
# define MAX_LINELEN 256

  if (line_read)
    {
      free (line_read);
      line_read = (char *) NULL;
    }

  printf ("> ");
  fflush (stdout);

  line_read = malloc (MAX_LINELEN);
  line_read = fgets (line_read, MAX_LINELEN, stdin);

  if (line_read)
    {
      char *endline;
      endline = strchr (line_read, '\n');
      if (endline)		/* if we got EOF there will be no '\n' */
	*endline = '\0';
      return stripwhite (line_read);
    }
  else
    return (char *) NULL;
}
#else
/** \brief Use the readline library to return a pointer to the next input line
 *
 * See GNU readline documentation, whence this routine came.
 */
char *
rl_gets ()
{
  char *s;
  if (line_read)
    {
      free (line_read);
      line_read = (char *) NULL;
    }

  /* Prompt is just a right arrow for now */
  line_read = readline (">");

  if (line_read)
    s = stripwhite (line_read);
  else
    return (char *) NULL;

  if (*s)
    add_history (line_read);

  return s;
}
#endif

/* Look up NAME as the name of a command, and return a pointer to that
   command.  Return a NULL pointer if NAME isn't a command name. */
COMMAND *
find_command (name)
     char *name;
{
  register int i;

  for (i = 0; commands[i].name; i++)
    if (strcmp (name, commands[i].name) == 0)
      return (&commands[i]);

  return ((COMMAND *) NULL);
}

int
execute_line (line)
     char *line;
{
  register int i;
  COMMAND *command;
  char *word;

  /* Isolate the command word. */
  i = 0;
  while (line[i] && whitespace (line[i]))
    i++;
  word = line + i;

  while (line[i] && !whitespace (line[i]))
    i++;

  if (line[i])
    line[i++] = '\0';

  command = find_command (word);

  if (!command)
    {
      fprintf (stderr, "%s: No such command\n", word);
      return -1;
    }

  /* find argument, if any */
  while (whitespace (line[i]))
    i++;

  word = line + i;

  return ((*(command->func)) (word));
}


#if HAVE_LIBREADLINE
int
initalize_readline (void)
{
  rl_readline_name = "fb_udp";

  /* Tell the GNU Readline library how to complete. */
  rl_attempted_completion_function = fbudp_completion;
  rl_filename_completion_desired = 0;
  return 0;
}
#endif

int
main (int argc, char *argv[])
{
  int remotePort;
  char errstr[LIBFB_ERRBUF_SIZE];
  char *remoteHost = NULL;
  fblib_err fberr;

  if (3 != argc)
    {
      fprintf (stderr, "Usage: %s <serverHost>	<serverPort>\n", argv[0]);
      exit (1);
    }

  remoteHost = argv[1];
  remotePort = atoi (argv[2]);

  f = libfb_init (NULL, LIBFB_ETHERNET_OFF, errstr);
  if (f == NULL)
    {
      fprintf (stderr, "libfb: %s", errstr);
      exit (EXIT_FAILURE);
    }

  fberr = libfb_connect (f, remoteHost, remotePort);
  switch (fberr)
    {
    case FBLIB_EERRNO:
      perror ("libfb");
      libfb_destroy (f);
      exit (EXIT_FAILURE);
      break;
    case FBLIB_EHERRNO:
      herror ("libfb");
      libfb_destroy (f);
      exit (EXIT_FAILURE);
      break;
    default:
      /* get on with it! */
      break;
    }

#if HAVE_LIBREADLINE
  /* Set up readline */
  if (initalize_readline () != 0)
    {
      fprintf (stderr, "Couldn't setup readline.\n");
      exit (EXIT_FAILURE);
    }
#endif
  printf ("FB UDP Tool, build number: %d\n",BUILD_NUM);
  printf ("Enter a command (help for usage)\n");
  while (1)
    {
      char *line;
#if HAVE_LIBREADLINE
      line = rl_gets ();
#else
      line = manual_gets ();
#endif
      if (!line || line[0] == 0)
	continue;

      execute_line (line);

      if (done)
	break;
    }

  exit (EXIT_SUCCESS);
}


/***** Completion Interface *****/
char *command_generator __P ((const char *, int));
char **fbudp_completion __P ((const char *, int, int));

/* Attempt to complete on the contents of TEXT.  START and END
   bound the region of rl_line_buffer that contains the word to
   complete.  TEXT is the word to complete.  We can use the entire
   contents of rl_line_buffer in case we want to do some simple
   parsing.  Returnthe array of matches, or NULL if there aren't any. */

#if HAVE_LIBREADLINE
char **
fbudp_completion (text, start, end)
     const char *text;
     int start, end;
{
  char **matches;

  matches = (char **) NULL;

  /* If this word is at the start of the line, then it is a command
     to complete.  Otherwise it is the name of a file in the current
     directory. */
  if (start == 0)
    matches = rl_completion_matches (text, command_generator);

  return (matches);
}
#endif
/* Generator function for command completion.  STATE lets us
   know whether to start from scratch; without any state
   (i.e. STATE == 0), then we start at the top of the list. */
char *
command_generator (text, state)
     const char *text;
     int state;
{
  static int list_index, len;
  char *name;

  /* If this is a new word to complete, initialize now.  This
     includes saving the length of TEXT for efficiency, and
     initializing the index variable to 0. */
  if (!state)
    {
      list_index = 0;
      len = strlen (text);
    }

  /* Return the next name which partially matches from the
     command list. */
  while ((name = commands[list_index].name))
    {
      list_index++;

      if (strncmp (name, text, len) == 0)
	return (strdup (name));
    }

  /* If no names matched, then return NULL. */
  return ((char *) NULL);
}
