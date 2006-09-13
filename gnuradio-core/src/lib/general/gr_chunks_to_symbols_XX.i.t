/* -*- c++ -*- */
/*
 * Copyright 2004 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

// @WARNING@

GR_SWIG_BLOCK_MAGIC(gr,@BASE_NAME@);

@SPTR_NAME@ gr_make_@BASE_NAME@ (const std::vector<@O_TYPE@> &symbol_table, const int D = 1);

class @NAME@ : public gr_sync_interpolator
{
private:
  @NAME@ (const std::vector<@O_TYPE@> &symbol_table, const int D = 1);

public:
  int D () const { return d_D; }
  std::vector<@O_TYPE@> symbol_table () const { return d_symbol_table; }
};
