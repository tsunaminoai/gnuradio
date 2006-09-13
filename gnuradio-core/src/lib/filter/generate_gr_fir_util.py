#!/bin/env python
#
# Copyright 2003 Free Software Foundation, Inc.
# 
# This file is part of GNU Radio
# 
# GNU Radio is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
# 
# GNU Radio is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with GNU Radio; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
# 

from generate_utils import *

def make_info_struct (out, sig):
    out.write (
'''
struct gr_fir_%s_info {
  const char    *name;             // implementation name, e.g., "generic", "SSE", "3DNow!"
  gr_fir_%s	*(*create)(const std::vector<%s> &taps);
};
''' % (sig, sig, tap_type(sig)))

def make_create (out, sig):
    out.write ('''  static gr_fir_%s *create_gr_fir_%s (const std::vector<%s> &taps);
''' % (sig, sig, tap_type (sig)))
    
def make_info (out, sig):
    out.write ('''  static void get_gr_fir_%s_info (std::vector<gr_fir_%s_info> *info);
''' % (sig, sig))
    

# ----------------------------------------------------------------

def make_gr_fir_util_h ():
    out = open_and_log_name ('gr_fir_util.h', 'w')
    out.write (copyright)

    out.write (
'''
/*
 * WARNING: This file is automatically generated by
 * generate_gr_fir_util.py.
 *
 * Any changes made to this file will be overwritten.
 */

#ifndef INCLUDED_GR_FIR_UTIL_H
#define INCLUDED_GR_FIR_UTIL_H

/*!
 * \\brief routines to create gr_fir_XXX's
 *
 * This class handles selecting the fastest version of the finite
 * implulse response filter available for your platform.  This
 * interface should be used by the rest of the system for creating
 * gr_fir_XXX's.
 *
 * The trailing suffix has the form _IOT where I codes the input type,
 * O codes the output type, and T codes the tap type.
 * I,O,T are elements of the set 's' (short), 'f' (float), 'c' (gr_complex), 
 * 'i' (short)
 */

#include <gr_types.h>

''')
   
    for sig in fir_signatures:
       out.write ('class gr_fir_%s;\n' % sig);

    out.write ('\n// structures returned by get_gr_fir_XXX_info methods\n\n')

    for sig in fir_signatures:
        make_info_struct (out, sig)

    out.write ('''
struct gr_fir_util {

  // create a fast version of gr_fir_XXX.

''')
    
    for sig in fir_signatures:
        make_create (out, sig)

    out.write ('''
  // Get information about all gr_fir_XXX implementations.
  // This is useful for benchmarking, testing, etc without having to
  // know a priori what's linked into this image
  //
  // The caller must pass in a valid pointer to a vector.
  // The vector will be filled with structs describing the
  // available implementations.

''')

    for sig in fir_signatures:
        make_info (out, sig)

    out.write ('''
};

#endif /* INCLUDED_GR_FIR_UTIL_H */
''')
    out.close ()


# ----------------------------------------------------------------

def make_constructor_cc (out, sig):
    out.write (
'''
gr_fir_%s *
gr_fir_util::create_gr_fir_%s (const std::vector<%s> &taps)
{
  return gr_fir_sysconfig_singleton()->create_gr_fir_%s (taps);
}
''' % (sig, sig, tap_type (sig), sig))


def make_info_cc (out, sig):
    out.write (
'''
void
gr_fir_util::get_gr_fir_%s_info (std::vector<gr_fir_%s_info> *info)
{
  gr_fir_sysconfig_singleton()->get_gr_fir_%s_info (info);
}
''' % (sig, sig, sig))

    
def make_gr_fir_util_cc ():
    out = open_and_log_name ('gr_fir_util.cc', 'w')
    out.write (copyright)
    out.write ('''

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gr_fir_util.h>
#include <gr_fir_sysconfig.h>

//
// There's no problem that can't be solved by the addition of
// another layer of indirection...
//

// --- constructors ---

''')

    for sig in fir_signatures:
        make_constructor_cc (out, sig)

    out.write ('''
// --- info gatherers ---

''')
    
    for sig in fir_signatures:
        make_info_cc (out, sig)

    out.close ()    
    

# ----------------------------------------------------------------

def generate ():
    make_gr_fir_util_h ()
    make_gr_fir_util_cc ()

if __name__ == '__main__':
    generate ()
    
