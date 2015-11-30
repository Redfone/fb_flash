/*
   fb_flash_util - foneBRIDGE flash utility
   (C) 2005-2007 Redfone Communications, LLC.
   www.red-fone.com

   by Brett Carrington <brettcar@gmail.com>

   seed.c: Random numbers and feature protection routines
*/

#if HAVE_CONFIG_H
# include <config.h>
#define SW_VER FBFLASH_VERSION
#endif

#if STDC_HEADERS || HAVE_STDLIB_H
# include <stdlib.h>
#endif

#if HAVE_TIME_H
# include <time.h>
#endif

#if HAVE_STRING_H
# include <string.h>
#endif

#if HAVE_LIBFB_FB_LIB_H
# include <libfb/fb_lib.h>
# include <libfb/sha1.h>
#endif


#define LOCAL_SEED_SZ 54

/* True if the random number generator has been initalized */
static int rng_ready = 0;

/* Initalize the random number generator using the current time as the
   seed. */
void
init_rng (void)
{

  unsigned int curtime = (unsigned int) time (NULL);

  srandom (curtime);
  rng_ready = 1;
}

/* Write a seed into the memory location pointed to by `dest'. The
   seed will be `n' bytes long. */
static void
generate_preseed (void *dest, size_t n)
{

  /* The definition of random() in the Linux manpage is a bit
   * tricky. It returns a long int, yes, but only up to a value of
   * RAND_MAX. On 64-bit architecture where long int might be 8 bytes
   * it still only returns 4 bytes since RAND_MAX is that size. We
   * handle that here.
   */

  u_int8_t seed[n];

  int num_chunks = n / sizeof (RAND_MAX);
  int i;

  memset (seed, 0, n);

  for (i = 0; i < num_chunks; i++)
    {
      long int rnd_chunk = random ();
      memcpy (seed + i * sizeof (RAND_MAX), &rnd_chunk, sizeof (RAND_MAX));
    }

  memcpy (dest, seed, n);
}

/* Generate the "seed" which is the preseed function with the serial
   number and MAC address appended. */
static void
generate_seed (u_int8_t * dest, SERIAL * ser, MAC_ADDR * mac)
{
  const int preseed_sz = 32;

  generate_preseed (dest, preseed_sz);
  memcpy (dest + preseed_sz, ser, SERIAL_SZ);
  memcpy (dest + preseed_sz + SERIAL_SZ, mac, MAC_SZ);
}

static void
hash_seed (u_int8_t * hash, u_int8_t * seed)
{
  SHA1Context sha;
  int err;

  err = SHA1Reset (&sha);
  if (err)
    {
      fprintf (stderr, "SHA1Reset Error %d.\n", err);
      exit (EXIT_FAILURE);
    }

  err = SHA1Input (&sha, seed, LOCAL_SEED_SZ);
  if (err)
    {
      fprintf (stderr, "SHA1Input Error %d.\n", err);
      exit (EXIT_FAILURE);
    }

  err = SHA1Result (&sha, hash);
  if (err)
    {
      fprintf (stderr,
	       "SHA1Result Error %d, could not compute message digest.\n",
	       err);
      exit (EXIT_FAILURE);
    }
}

/* Program the key buffer to 'address' which is the concatenation of `length' KEY_ENTRY structures */
void
program_seeds (libfb_t * fb, MAC_ADDR * mac, unsigned int addr,
	       u_int8_t * keybuffer, unsigned count)
{

  if (write_to_blk
      (fb, *mac, 0, count * sizeof (KEY_ENTRY),
       (unsigned char *) keybuffer) == 0)
    start_blk_write (fb, *mac, addr);
  else
    {
      fprintf (stderr, "Failed to write seed into flash!\n");
      exit (EXIT_FAILURE);
    }
}



/* Generate a new KEY_ENTRY structure and set the corresponding seed pointer */
void
generate_key_entry (KEY_ENTRY * entry, SERIAL * ser, MAC_ADDR * mac,
		    u_int8_t * seedptr)
{
  u_int8_t seed[LOCAL_SEED_SZ];
  u_int8_t hash[HASH_KEY_SZ];

  if (!rng_ready)
    {
      fprintf (stderr, "generate_key_entry: Crypto subsystem not ready!\n");
      exit (EXIT_FAILURE);
    }

  memset (entry, 0, sizeof (KEY_ENTRY));
  /* Generate a new seed. */
  generate_seed (seed, ser, mac);
  /* Generate the hash */
  hash_seed (hash, seed);

  /* Copy hash key to entry struct */
  memcpy (entry->hash_key, hash, HASH_KEY_SZ);


  entry->crc16 = crc_16 ((unsigned char *) entry, sizeof (KEY_ENTRY) - 2);

  /* Copy seed to user */
  memcpy (seedptr, seed, LOCAL_SEED_SZ);
}

#if 0
void
check_rng (void)
{
  int i;

  printf ("RAND_MAX: %i\n", RAND_MAX);
  printf ("sizeof long int: %li\n", sizeof (long int));
  printf ("Example call: %li\n", random ());
  SERIAL ser;
  memset (ser, 0xAB, sizeof (ser));
  MAC_ADDR mac;
  memset (mac, 0xDD, sizeof (mac));

  u_int8_t newseed[200];
  u_int8_t hash[20];
  generate_seed (newseed, &ser, &mac);
  hash_seed (hash, newseed);
  for (i = 0; i < SEED_SIZE; i++)
    printf ("%0X ", newseed[i]);
  printf ("\n");
  printf ("Hash: ");
  for (i = 0; i < 20; i++)
    printf ("%0X ", hash[i]);
  printf ("\n");

}
#endif
