/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* Directory for the storage files. */
#define BLUEALSA_STORAGE_DIR "/var/lib/bluealsa"

/* Define to 1 if the debugging is enabled. */
#define DEBUG 1

/* Define to 1 if the debug timing is enabled. */
/* #undef DEBUG_TIME */

/* Define to 1 if AAC is enabled. */
#define ENABLE_AAC 1

/* Define to 1 if apt-X is enabled. */
#define ENABLE_APTX 1

/* Define to 1 if apt-X HD is enabled. */
/* #undef ENABLE_APTX_HD */

/* Define to 1 if FastStream is enabled. */
/* #undef ENABLE_FASTSTREAM */

/* Define to 1 if HFP codec selection is enabled. */
/* #undef ENABLE_HFP_CODEC_SELECTION */

/* Define to 1 if LC3plus is enabled. */
/* #undef ENABLE_LC3PLUS */

/* Define to 1 if LC3-SWB is enabled. */
/* #undef ENABLE_LC3_SWB */

/* Define to 1 if LDAC is enabled. */
/* #undef ENABLE_LDAC */

/* Define to 1 if LHDC is enabled. */
/* #undef ENABLE_LHDC */

/* Define to 1 if Bluetooth LE MIDI is enabled. */
/* #undef ENABLE_MIDI */

/* Define to 1 if MP3LAME is enabled. */
/* #undef ENABLE_MP3LAME */

/* Define to 1 if MPEG is enabled. */
/* #undef ENABLE_MPEG */

/* Define to 1 if MPG123 is enabled. */
/* #undef ENABLE_MPG123 */

/* Define to 1 if mSBC is enabled. */
/* #undef ENABLE_MSBC */

/* Define to 1 if oFono is enabled. */
/* #undef ENABLE_OFONO */

/* Define to 1 if Opus is enabled. */
#define ENABLE_OPUS 1

/* Define to 1 if PAYLOADCHECK is enabled. */
#define ENABLE_PAYLOADCHECK 1

/* Define to 1 if systemd is enabled. */
/* #undef ENABLE_SYSTEMD */

/* Define to 1 if UPower is enabled. */
#define ENABLE_UPOWER 1

/* Define to 1 if you have apt-X decode library. */
#define HAVE_APTX_DECODE 1

/* Define to 1 if you have apt-X HD decode library. */
/* #undef HAVE_APTX_HD_DECODE */

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the `eventfd' function. */
#define HAVE_EVENTFD 1

/* Define to 1 if you have the <execinfo.h> header file. */
#define HAVE_EXECINFO_H 1

/* Define to 1 if you have the `gettid' function. */
/* #undef HAVE_GETTID */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <lc3plus.h> header file. */
/* #undef HAVE_LC3PLUS_H */

/* Define to 1 if you have LDAC decode module. */
/* #undef HAVE_LDAC_DECODE */

/* Define to 1 if you have the libbsd library. */
/* #undef HAVE_LIBBSD */

/* Define to 1 if you have the `SegFault' library (-lSegFault). */
/* #undef HAVE_LIBSEGFAULT */

/* Define to 1 if you have the <minix/config.h> header file. */
/* #undef HAVE_MINIX_CONFIG_H */

/* Define to 1 if you have the `pipe2' function. */
#define HAVE_PIPE2 1

/* Define to 1 if you have the sndfile library. */
/* #undef HAVE_SNDFILE */

/* Define to 1 if you have the `splice' function. */
#define HAVE_SPLICE 1

/* Define to 1 if you have the <stdatomic.h> header file. */
#define HAVE_STDATOMIC_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdio.h> header file. */
#define HAVE_STDIO_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the <wchar.h> header file. */
#define HAVE_WCHAR_H 1

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#define LT_OBJDIR ".libs/"

/* Name of package */
#define PACKAGE "bluez-alsa"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "arkadiusz.bokowy@gmail.com"

/* Define to the full name of this package. */
#define PACKAGE_NAME "BlueALSA"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "BlueALSA v4.3.1-70-g8f99300"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "bluez-alsa"

/* Define to the home page for this package. */
#define PACKAGE_URL "https://github.com/arkq/bluez-alsa"

/* Define to the version of this package. */
#define PACKAGE_VERSION "v4.3.1-70-g8f99300"

/* Define to 1 if all of the C90 standard headers exist (not just the ones
   required in a freestanding environment). This macro is provided for
   backward compatibility; new code need not use it. */
#define STDC_HEADERS 1

/* Enable extensions on AIX 3, Interix.  */
#ifndef _ALL_SOURCE
# define _ALL_SOURCE 1
#endif
/* Enable general extensions on macOS.  */
#ifndef _DARWIN_C_SOURCE
# define _DARWIN_C_SOURCE 1
#endif
/* Enable general extensions on Solaris.  */
#ifndef __EXTENSIONS__
# define __EXTENSIONS__ 1
#endif
/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE 1
#endif
/* Enable X/Open compliant socket functions that do not require linking
   with -lxnet on HP-UX 11.11.  */
#ifndef _HPUX_ALT_XOPEN_SOCKET_API
# define _HPUX_ALT_XOPEN_SOCKET_API 1
#endif
/* Identify the host operating system as Minix.
   This macro does not affect the system headers' behavior.
   A future release of Autoconf may stop defining this macro.  */
#ifndef _MINIX
/* # undef _MINIX */
#endif
/* Enable general extensions on NetBSD.
   Enable NetBSD compatibility extensions on Minix.  */
#ifndef _NETBSD_SOURCE
# define _NETBSD_SOURCE 1
#endif
/* Enable OpenBSD compatibility extensions on NetBSD.
   Oddly enough, this does nothing on OpenBSD.  */
#ifndef _OPENBSD_SOURCE
# define _OPENBSD_SOURCE 1
#endif
/* Define to 1 if needed for POSIX-compatible behavior.  */
#ifndef _POSIX_SOURCE
/* # undef _POSIX_SOURCE */
#endif
/* Define to 2 if needed for POSIX-compatible behavior.  */
#ifndef _POSIX_1_SOURCE
/* # undef _POSIX_1_SOURCE */
#endif
/* Enable POSIX-compatible threading on Solaris.  */
#ifndef _POSIX_PTHREAD_SEMANTICS
# define _POSIX_PTHREAD_SEMANTICS 1
#endif
/* Enable extensions specified by ISO/IEC TS 18661-5:2014.  */
#ifndef __STDC_WANT_IEC_60559_ATTRIBS_EXT__
# define __STDC_WANT_IEC_60559_ATTRIBS_EXT__ 1
#endif
/* Enable extensions specified by ISO/IEC TS 18661-1:2014.  */
#ifndef __STDC_WANT_IEC_60559_BFP_EXT__
# define __STDC_WANT_IEC_60559_BFP_EXT__ 1
#endif
/* Enable extensions specified by ISO/IEC TS 18661-2:2015.  */
#ifndef __STDC_WANT_IEC_60559_DFP_EXT__
# define __STDC_WANT_IEC_60559_DFP_EXT__ 1
#endif
/* Enable extensions specified by ISO/IEC TS 18661-4:2015.  */
#ifndef __STDC_WANT_IEC_60559_FUNCS_EXT__
# define __STDC_WANT_IEC_60559_FUNCS_EXT__ 1
#endif
/* Enable extensions specified by ISO/IEC TS 18661-3:2015.  */
#ifndef __STDC_WANT_IEC_60559_TYPES_EXT__
# define __STDC_WANT_IEC_60559_TYPES_EXT__ 1
#endif
/* Enable extensions specified by ISO/IEC TR 24731-2:2010.  */
#ifndef __STDC_WANT_LIB_EXT2__
# define __STDC_WANT_LIB_EXT2__ 1
#endif
/* Enable extensions specified by ISO/IEC 24747:2009.  */
#ifndef __STDC_WANT_MATH_SPEC_FUNCS__
# define __STDC_WANT_MATH_SPEC_FUNCS__ 1
#endif
/* Enable extensions on HP NonStop.  */
#ifndef _TANDEM_SOURCE
# define _TANDEM_SOURCE 1
#endif
/* Enable X/Open extensions.  Define to 500 only if necessary
   to make mbstate_t available.  */
#ifndef _XOPEN_SOURCE
/* # undef _XOPEN_SOURCE */
#endif


/* Version number of package */
#define VERSION "v4.3.1-70-g8f99300"

/* Define to 1 if libfreeaptx shall be used. */
/* #undef WITH_LIBFREEAPTX */

/* Define to 1 if libopenaptx shall be used. */
#define WITH_LIBOPENAPTX 1

/* Define to 1 if libsamplerate shall be used. */
/* #undef WITH_LIBSAMPLERATE */

/* Define to 1 if libunwind shall be used. */
/* #undef WITH_LIBUNWIND */
