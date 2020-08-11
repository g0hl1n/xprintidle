/* SPDX-License-Identifier: GPL-2.0-only
 *
 * This program prints the "idle time" of the user to stdout.  The "idle time"
 * is the number of milliseconds since input was received on any input device.
 * If unsuccessful, the program prints a message to stderr and exits with a
 * non-zero exit code.
 *
 * Copyright (c) 2005, 2008 Magnus Henoch <henoch@dtek.chalmers.se>
 * Copyright (c) 2006, 2007 by Danny Kukawka <dkukawka@suse.de>,
 *                                           <danny.kukawka@web.de>
 * Copyright (c) 2008 Eivind Magnus Hvidevold <hvidevold@gmail.com>
 * Copyright (c) 2014-2020 Richard Leitner <dev@g0hl1n.net>
 *
 * This file is part of xprintidle.
 *
 * xprintidle is free software; you can redistribute it and/or modify it under
 * the terms of version 2 of the GNU General Public License as published by the
 * Free Software Foundation.
 *
 * xprintidle is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
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
#include <stdio.h>

void usage(char *name);
unsigned long workaroundCreepyXServer(Display *dpy, unsigned long idleTime);

int main(int argc, char *argv[]) {
  XScreenSaverInfo *ssi;
  Display *dpy;
  int event_basep, error_basep, vendrel;
  unsigned long idle;

  if (argc != 1) {
    usage(argv[0]);
    return 1;
  }

  dpy = XOpenDisplay(NULL);
  if (dpy == NULL) {
    fprintf(stderr, "couldn't open display\n");
    return 1;
  }

  if (!XScreenSaverQueryExtension(dpy, &event_basep, &error_basep)) {
    fprintf(stderr, "screen saver extension not supported\n");
    return 1;
  }

  ssi = XScreenSaverAllocInfo();
  if (ssi == NULL) {
    fprintf(stderr, "couldn't allocate screen saver info\n");
    return 1;
  }

  if (!XScreenSaverQueryInfo(dpy, DefaultRootWindow(dpy), ssi)) {
    fprintf(stderr, "couldn't query screen saver info\n");
    return 1;
  }

  /* xorg fixed the reset of the idle time in some (unknown) release. We now it
   * is fixed in v1.20.00, therefore don't do the workaround for this version.
   * If anybody finds the commit and therefore xorg release which fixes this
   * issue please send a patch or raise an issue ;-) */
  vendrel = VendorRelease(dpy);
  if (vendrel < 12000000) {
    idle = workaroundCreepyXServer(dpy, ssi->idle);
  } else {
    idle = ssi->idle;
  }

  printf("%lu\n", idle);

  XFree(ssi);
  XCloseDisplay(dpy);
  return 0;
}

void usage(char *name) {
  fprintf(stderr,
          "Usage:\n"
          "%s\n"
          "That is, no command line arguments.  The user's idle time\n"
          "in milliseconds is printed on stdout.\n",
          name);
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

  if (DPMSQueryExtension(dpy, &dummy, &dummy)) {
    if (DPMSCapable(dpy)) {
      DPMSGetTimeouts(dpy, &standby, &suspend, &off);
      DPMSInfo(dpy, &state, &onoff);

      if (onoff) {
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
      }
    }
  }

  return idleTime;
}
