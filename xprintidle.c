/* SPDX-License-Identifier: GPL-2.0-only
 *
 * This program prints the "idle time" of the user to stdout. The "idle time"
 * is the number of milliseconds since input was received on any input device.
 * If unsuccessful, the program prints a message to stderr and exits with a
 * non-zero exit code.
 *
 * Copyright (c) 2005, 2008 Magnus Henoch <henoch@dtek.chalmers.se>
 * Copyright (c) 2006, 2007 by Danny Kukawka <dkukawka@suse.de>,
 *                                           <danny.kukawka@web.de>
 * Copyright (c) 2008 Eivind Magnus Hvidevold <hvidevold@gmail.com>
 * Copyright (c) 2014-2022 Richard Leitner <dev@g0hl1n.net>
 *
 * This file is part of xprintidle.
 *
 * xprintidle is free software; you can redistribute it and/or modify it under
 * the terms of version 2 of the GNU General Public License as published by the
 * Free Software Foundation.
 *
 * xprintidle is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * xprintidle. If not, see <https://www.gnu.org/licenses/>.
 *
 * The function workaroundCreepyXServer was adapted from kpowersave-0.7.3 by
 * Eivind Magnus Hvidevold <hvidevold@gmail.com>. kpowersave is licensed under
 * the GNU GPL, version 2 _only_.
 */

#include <X11/Xlib.h>
#include <X11/extensions/dpms.h>
#include <X11/extensions/scrnsaver.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef XPRINTIDLE_VERSION
#define XPRINTIDLE_VERSION "n/a"
#endif

unsigned long workaroundCreepyXServer(Display *dpy, unsigned long idleTime);

void print_usage(char *name) {
  fprintf(stdout,
          "usage: %s [OPTION]\n"
          "Query the X server for the user's idle time\n"
          "\n"
          "Options:\n"
          "  -h, --help              Show this text\n"
          "  -H, --human-readable    Output the time in a human readable format\n"
          "  -v, --version           Print the program version\n"
          "\n"
          "Report bugs at: https://github.com/g0hl1n/xprintidle/issues\n"
          "Written by Magnus Henoch and others; see\n"
          "https://github.com/g0hl1n/xprintidle/blob/master/AUTHORS\n",
          name);
}

void print_version(void) {
  fprintf(stdout, "xprintidle %s\n", XPRINTIDLE_VERSION);
}

/*
 * This function gets the X idle time in milliseconds and writes it to the
 * "idle" argument.
 * On success 0 is returned.
 * On error -1 is returned.
 */
int get_x_idletime(uint64_t *idle) {
  XScreenSaverInfo *ssi;
  Display *dpy;
  int event_basep, error_basep, vendrel;

  dpy = XOpenDisplay(NULL);
  if (dpy == NULL) {
    fprintf(stderr, "couldn't open display\n");
    return -1;
  }

  if (!XScreenSaverQueryExtension(dpy, &event_basep, &error_basep)) {
    fprintf(stderr, "screen saver extension not supported\n");
    XCloseDisplay(dpy);
    return -1;
  }

  ssi = XScreenSaverAllocInfo();
  if (ssi == NULL) {
    fprintf(stderr, "couldn't allocate screen saver info\n");
    XCloseDisplay(dpy);
    return -1;
  }

  if (!XScreenSaverQueryInfo(dpy, DefaultRootWindow(dpy), ssi)) {
    fprintf(stderr, "couldn't query screen saver info\n");
    XFree(ssi);
    XCloseDisplay(dpy);
    return -1;
  }

  /* xorg fixed the reset of the idle time in some (unknown) release. We now it
   * is fixed in v1.20.00, therefore don't do the workaround for this version.
   * If anybody finds the commit and therefore xorg release which fixes this
   * issue please send a patch or raise an issue ;-) */
  vendrel = VendorRelease(dpy);
  if (vendrel < 12000000) {
    *idle = workaroundCreepyXServer(dpy, ssi->idle);
  } else {
    *idle = ssi->idle;
  }

  XFree(ssi);
  XCloseDisplay(dpy);

  return 0;
}

/* This function prints miliseconds in a human-readable format. */
void print_human_time(uint64_t time) {
  /* The C standard says that integer division round towards 0. */

  int convFacs[] = {24 * 60 * 60 * 1000, 60 * 60 * 1000, 60 * 1000, 1000, 1};
  char *names[] = {"day", "hour", "minute", "second", "millisecond"};
  size_t units = sizeof(convFacs) / sizeof(int);

  int firstPrint = 1;
  size_t i;
  for (i = 0; i < units; i++) {
    int unitMag = time / convFacs[i];
    time %= convFacs[i];

    if (!unitMag)
      continue;

    if (!firstPrint)
      printf(", ");
    printf("%d %s", unitMag, names[i]);
    if (unitMag != 1)
      printf("s");

    firstPrint = 0;
  }

  /* Smallest unit would be 0. */
  if (firstPrint)
    printf("0 %ss", names[units - 1]);

  printf("\n");
}

int main(int argc, char *argv[]) {
  uint64_t idle;
  int human = 0;

  /* TODO change this to getopts as soon as we have more options */
  if (argc != 1) {
    if (!strcmp(argv[1], "-v") || !strcmp(argv[1], "--version")) {
      print_version();
      return EXIT_SUCCESS;
    } else if (!strcmp(argv[1], "-H") || !strcmp(argv[1], "--human-readable")) {
      human = 1;
    } else if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
      print_usage(argv[0]);
      return EXIT_FAILURE;
    }

  }

  if (get_x_idletime(&idle) < 0) {
    return EXIT_FAILURE;
  }

  if (human) {
    print_human_time(idle);
    return EXIT_SUCCESS;
  }

  printf("%llu\n", idle);
  return EXIT_SUCCESS;
}

/*
 * This function works around an XServer idleTime bug in the
 * XScreenSaverExtension if dpms is running. In this case the current
 * dpms-state time is always subtracted from the current idletime.
 * This means: XScreenSaverInfo->idle is not the time since the last
 * user activity, as described in the header file of the extension.
 * This result in SUSE bug # and sf.net bug #. The bug in the XServer itself
 * is reported at https://bugs.freedesktop.org/buglist.cgi?quicksearch=6439.
 *
 * Workaround: Check if if XServer is in a dpms state, check the
 *             current timeout for this state and add this value to
 *             the current idle time and return.
 */
unsigned long workaroundCreepyXServer(Display *dpy, unsigned long idleTime) {
  int dummy;
  CARD16 standby, suspend, off;
  CARD16 state;
  BOOL onoff;

  if (!DPMSQueryExtension(dpy, &dummy, &dummy))
    return idleTime;
  else if (!DPMSCapable(dpy))
    return idleTime;

  DPMSGetTimeouts(dpy, &standby, &suspend, &off);
  DPMSInfo(dpy, &state, &onoff);

  if (!onoff)
    return idleTime;

  switch (state) {
  case DPMSModeStandby:
    /* this check is a little bit paranoid, but be sure */
    if (idleTime < (unsigned)(standby * 1000))
      idleTime += (standby * 1000);
    break;
  case DPMSModeSuspend:
    if (idleTime < (unsigned)((suspend + standby) * 1000))
      idleTime += ((suspend + standby) * 1000);
    break;
  case DPMSModeOff:
    if (idleTime < (unsigned)((off + suspend + standby) * 1000))
      idleTime += ((off + suspend + standby) * 1000);
    break;
  case DPMSModeOn:
  default:
    break;
  }

  return idleTime;
}
