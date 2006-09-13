/* -*- c++ -*- */
/*
 * Copyright 2006 Free Software Foundation, Inc.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <decoder_viterbi.h>
#include <assert.h>
#include <iostream>

const int g_max_block_size_bits = 10000000;
const int g_max_num_streams = 10;
const int g_num_bits_per_byte = 8;

#define DO_TIME_THOUGHPUT 0
#define DO_PRINT_DEBUG_INST 0
#define DO_PRINT_DEBUG_INST_0 0
#define DO_PRINT_DEBUG_INST_1 0
#define DO_PRINT_DEBUG_INST_2 0
#define DO_PRINT_DEBUG_FSM 0
#define DO_PRINT_DEBUG_INIT 0
#define DO_PRINT_DEBUG_UP 0
#define DO_PRINT_DEBUG_UP_0 0
#define DO_PRINT_DEBUG_UP_1 0
#define DO_PRINT_DEBUG_MIDDLE 0
#define DO_PRINT_DEBUG_MIDDLE_0 0
#define DO_PRINT_DEBUG_MIDDLE_1 0
#define DO_PRINT_DEBUG_TERM 0
#define DO_PRINT_DEBUG_TERM_1 0
#define DO_PRINT_DEBUG_OUTPUT 0
#define DO_PRINT_DEBUG_OUTPUT_0 0
#define DO_PRINT_DEBUG_EXIT 0
#define DO_PRINT_DEBUG 0

#include <mld/mld_timer.h>
#include <mld/n2bs.h>

decoder_viterbi::decoder_viterbi
(int sample_precision,
 const encoder_convolutional* l_encoder)
{
  // make sure that the encoder is "valid"

  if (! l_encoder) {
    std::cerr << "decoder_viterbi: Error: Encoder is NULL.\n";
    assert (0);
  }

  // make the metrics converter

  //  d_code_metrics = new code_metrics (

  if ((sample_precision < 0) | (sample_precision > 32)) {
    std::cerr << "decoder_viterbi: "
      "Requested sample_precision (" << sample_precision <<
      ") must be between 0 and 32.\n";
    assert (0);
  }

  // get the trellis

  d_encoder = (encoder_convolutional*) l_encoder;
  d_trellis = (code_convolutional_trellis*) d_encoder->trellis ();

  // fill the class variables

  d_block_size_bits = d_encoder->block_size_bits ();
  d_n_code_inputs = d_encoder->n_code_inputs ();
  d_n_code_outputs = d_encoder->n_code_outputs ();
  d_do_termination = d_encoder->do_termination ();
  d_total_n_delays = d_encoder->total_n_delays ();
  d_n_states = d_trellis->n_states ();
  d_n_input_combinations = d_trellis->n_input_combinations ();

  // really nothing else to do here, since this class doesn't "know"
  // how to process streaming versus block decoding, or partial
  // trellis versus full-trellis.  Those will be handled by
  // sub-classes which inherit from this one.

  if (DO_PRINT_DEBUG_INST_0) {
    std::cout << "Init:\n" <<
      "d_block_size_bits          = " << d_block_size_bits << "\n" <<
      "d_n_code_inputs            = " << d_n_code_inputs << "\n" <<
      "d_n_code_outputs           = " << d_n_code_outputs << "\n" <<
      "d_do_termination           = " <<
      ((d_do_termination == true) ? "true" : "false") << "\n" <<
      "d_total_n_delays           = " << d_total_n_delays << "\n" <<
      "d_n_states                 = " << d_n_states << "\n" <<
      "d_n_input_combinations     = " << d_n_input_combinations << "\n";
  }

  // always start the fsm in the "init" state

  d_fsm_state = fsm_dec_viterbi_init;

  // create a vector of indexes to states when doing "up" or "termination";

  d_up_term_states_ndx[0] = new size_t [d_n_states];
  d_up_term_states_ndx[1] = new size_t [d_n_states];

  // get the total number of out bits per stream

  d_n_total_inputs_per_stream = d_block_size_bits;
  if (d_do_termination == true)
    d_n_total_inputs_per_stream += d_total_n_delays;
}

decoder_viterbi::~decoder_viterbi
()
{
  // reverse over from allocation
#if 0
  delete [] d_up_term_states_ndx[0];
  delete [] d_up_term_states_ndx[1];

  // delete the state's structures

  state_t_ptr t_state = d_states[0];
  for (size_t n = d_n_states; n > 0; n--, t_state++) {
    connection_t_ptr t_connection = t_state->d_connections;
    for (size_t m = d_n_input_combinations; m > 0; m--, t_connection++) {
      delete [] t_connection->d_output_bits;
    }
    delete [] t_state->d_connections;
  }
  delete [] d_states[0];

  t_state = d_states[1];
  for (size_t n = d_n_states; n > 0; n--, t_state++) {
    connection_t_ptr t_connection = t_state->d_connections;
    for (size_t m = d_n_input_combinations; m > 0; m--, t_connection++) {
      delete [] t_connection->d_output_bits;
    }
    delete [] t_state->d_connections;
  }
  delete [] d_states[1];

  // delete the save buffer

  char** t_save_buffer = d_save_buffer;
  for (size_t n = 0; n < d_n_code_inputs; n++) {
    delete [] (*t_save_buffer++);
  }
  delete [] d_save_buffer;
#endif
}

void
decoder_viterbi::reset_metrics
(u_char which)
{
  state_t_ptr t_state = d_states[which&1];
  for (size_t n = d_n_states; n > 0; n--, t_state++) {
    t_state->d_max_metric = -1e10;
#if 0
// probably don't need to do these, try removing them later
    t_state->d_max_state_ndx = 0;
    t_state->d_max_input = -1;
#endif
  }
}

void
decoder_viterbi::zero_metrics
(u_char which)
{
  state_t_ptr t_state = d_states[which&1];
  for (size_t n = d_n_states; n > 0; n--, t_state++) {
    t_state->d_max_metric = 0;
#if 0
// probably don't need to do these, try removing them later
    t_state->d_max_state_ndx = -1;
    t_state->d_max_input = -1;
#endif
  }
}

void
decoder_viterbi::decode_private
()
{
#if 0

#if DO_TIME_THOUGHPUT
  struct timeval t_tp;
  start_timer (&t_tp);
#endif
#if DO_PRINT_DEBUG
  size_t t_state_print_bits = d_total_n_delays;
  size_t t_mem_print_bits = d_total_n_delays;
#endif
// setup variables for quicker access
  const char **in_buf = (const char **) &input_items[0];
  char **out_buf = (char **) &output_items[0];
  int t_in_buf_ndx = 0, t_out_buf_ndx = 0;
  int t_out_bit_shift = 0;
  int t_ninput_items = compute_n_input_metrics (noutput_items);
  int t_noutput_bits = noutput_items;

#if DO_PRINT_DEBUG_INST
  std::cout << "# output items = " << noutput_items << " (" <<
    t_noutput_bytes << " Bytes, " << (t_noutput_bytes * g_num_bits_per_byte) <<
    " bits), # input items = " << t_ninput_items << " Symbols\n";
  for (size_t n = 0; n < ninput_items.size(); n++) {
    std::cout << "# input items [" << n << "] = " << ninput_items[n] << "\n";
  }
#endif

// put any leftover bits into the output
  if (d_n_saved_bits != 0) {
// copy the leftover from the save buffer
// check to make sure it will all fit
    size_t t_noutput_bits = t_noutput_bytes * g_num_bits_per_byte;
    size_t t_n_copy;
    if (t_noutput_bits < d_n_saved_bits) {
// asking for fewer bits than available; set to copy
// just what's being asked for
      t_n_copy = t_noutput_bytes;
    } else {
// asking for at least as many bits as available; set to copy all
      t_out_buf_ndx = d_n_saved_bits / g_num_bits_per_byte;
      t_out_bit_shift = d_n_saved_bits % g_num_bits_per_byte;
      t_n_copy = t_out_buf_ndx + (t_out_bit_shift != 0 ? 1 : 0);
    }
// do the copy for all output streams (code inputs)
// copy starting at save buffer index "start"
    for (size_t n = 0; n < d_n_code_inputs; n++)
      bcopy (&(d_save_buffer[n][d_n_saved_bits_start_ndx]),
	     out_buf[n], t_n_copy);
#if DO_PRINT_DEBUG_INST
    std::cout << "Copied " << t_n_copy << " Byte" <<
      (t_n_copy != 1 ? "s" : "") << ": s_b[][" <<
      d_n_saved_bits_start_ndx << "] => o_b[][0]\n" <<
      "# saved bits = " << d_n_saved_bits <<
      ", o_b_ndx = " << t_out_buf_ndx <<
      ", bit shift = " << t_out_bit_shift << "\n";
#endif
//  update the number of saved bits and start
    if (t_noutput_bits < d_n_saved_bits) {
// asking for fewer bit than available: update
// the number of saved bits and their starting index
      d_n_saved_bits_start_ndx += t_noutput_bytes;
      d_n_saved_bits -= (t_noutput_bytes * g_num_bits_per_byte);
#if DO_PRINT_DEBUG_INST
      std::cout << "Updated # saved bits = " << d_n_saved_bits <<
	", index = " << d_n_saved_bits_start_ndx << "\n";
#endif
// nothing to consume; return the desired number of output items
      return (noutput_items);
    } else {
      d_n_saved_bits_start_ndx = d_n_saved_bits = 0;
    }
  }
#if DO_PRINT_DEBUG_INST
  std::cout << "Starting FSM in state: " <<
    (d_fsm_state == fsm_dec_viterbi_init ? "init" :
     (d_fsm_state == fsm_dec_viterbi_doing_up ? "up" :
      (d_fsm_state == fsm_dec_viterbi_doing_middle ? "middle" :
       (d_fsm_state == fsm_dec_viterbi_doing_term ? "term" :
	(d_fsm_state == fsm_dec_viterbi_output ? "output" : "unknown"))))) << "\n";
#endif

// while there are input items to create
  while (t_out_buf_ndx < t_noutput_bytes) {
#if DO_PRINT_DEBUG_FMS
    std::cout << "Starting 'while': processing " << t_in_buf_ndx << " of " <<
      t_ninput_items << " items.\nJumping to state '" <<
      (d_fsm_state == fsm_dec_viterbi_init ? "init" :
       (d_fsm_state == fsm_dec_viterbi_doing_up ? "up" :
	(d_fsm_state == fsm_dec_viterbi_doing_middle ? "middle" :
	 (d_fsm_state == fsm_dec_viterbi_doing_term ? "term" :
	  (d_fsm_state == fsm_dec_viterbi_output ? "output" : "unknown"))))) << "'\n";
#endif
// jump to the correct state in the fsm
    switch (d_fsm_state) {
    case fsm_dec_viterbi_doing_up:
#if 0
      encode_loop_up ();
#endif
      break;
    case (fsm_dec_viterbi_doing_middle):
#if DO_PRINT_DEBUG_FSM
      std::cout << "Entered fsm_dec_viterbi_doing_middle\n";
#endif
// stay in this state until the correct number of input symbols is
// reached (if doing block coding), or until there are no more input
// symbols to parse
      while (((d_block_size_bits != 0) &
	      (d_time_count < d_block_size_bits) &
	      (t_in_buf_ndx < t_ninput_items)) |
	     ((d_block_size_bits == 0) &
	      (t_in_buf_ndx < t_ninput_items))) {
// use all states, loop over all inputs, compute the metric for
// each; compare the current metric with all previous stored metric in the
// endpoint of the connection to find the highest one.

// reset the "to" state's metrics
        reset_metrics (d_states_ndx ^ 1);
// get the state's index for each set of new inputs
	state_t_ptr t_state = d_states[d_states_ndx];
#if DO_PRINT_DEBUG_MIDDLE
	std::cout << "Time Count " << (d_time_count+1) << " of " <<
	  d_block_size_bits << "\n" <<
	  "d_states_ndx = " << d_states_ndx << "\n";
#endif
// loop over all current states
	for (size_t n = 0; n < d_n_states; n++, t_state++) {
// loop over all possible input values, 1 bit per input stream
	  connection_t_ptr t_connection = t_state->d_connections;
#if DO_PRINT_DEBUG_MIDDLE_0
	  std::cout << "Looping over all 'middle' states: " <<
	    n << " of " << d_n_states << "\n";
#endif
	  for (size_t q = 0; q < d_n_input_combinations; q++, t_connection++) {
#if DO_PRINT_DEBUG_MIDDLE_0
	    std::cout << "Input = " << n2bs(q, d_n_code_inputs+1) << "\n";
#endif
// start with this state's metric
	    float t_metric = t_state->d_max_metric;
// get the "to" state
            state_t_ptr t_to_state = t_connection->d_to;
// get that state's metric
            float t_to_metric = t_to_state->d_max_metric;
// see if the computation is even needed;
// maximum change is d_n_code_outputs if all bits match exactly
            if ((t_to_metric - t_metric) > ((float) 2*d_n_code_outputs)) {
#if DO_PRINT_DEBUG_MIDDLE_0
	      std::cout << "!not computing metric!\n";
#endif
              continue;
	    }
// metrics are close enough; do the computation and the compare
// get the output bits for this connection
	    float* t_output_bit = t_connection->d_output_bits;
	    if (d_do_mux_inputs == true) {
// if using mux'ed input streams, handle differently
              const float* t_in_buf = &(in_buf[0][t_in_buf_ndx]);
// loop over all encoder-output values
	      for (size_t r = d_n_code_outputs; r > 0; r--) {
#if DO_PRINT_DEBUG_MIDDLE_0
		std::cout << "in_sym = " << *t_in_buf << ", code_out_bit = " <<
		  *t_output_bit << " ==> metric -> ";
#endif
		t_metric += ((*t_in_buf++) * (*t_output_bit++));
#if DO_PRINT_DEBUG_MIDDLE_0
		std::cout << t_metric << "\n";
#endif
	      }
	    } else {
// loop over all encoder-output values
	      for (size_t r = 0; r < d_n_code_outputs; r++) {
#if DO_PRINT_DEBUG_MIDDLE_0
		std::cout << "in_sym = " << in_buf[r][t_in_buf_ndx] <<
		  ", code_out_bit = " << *t_output_bit << " ==> metric -> ";
#endif
		t_metric += (in_buf[r][t_in_buf_ndx] * (*t_output_bit++));
#if DO_PRINT_DEBUG_MIDDLE_0
		std::cout << t_metric << "\n";
#endif
	      }
	    }
// done with this input value; compare old and new metrics
            if (t_metric > t_to_metric) {
#if DO_PRINT_DEBUG_MIDDLE
	      std::cout << "New Metric to s[" <<
		n2bs (t_connection->d_to_ndx,t_state_print_bits) << "]: ";
	      if (t_to_state->d_max_metric == -1e10) {
		std::cout << "setting to ";
	      } else {
		std::cout << "was s[" <<
		  n2bs(t_to_state->d_max_state_ndx,t_state_print_bits) <<
		  "]i[" << n2bs (t_to_state->d_max_input, d_n_code_inputs+1) <<
		  "]@ " << t_to_state->d_max_metric << "  now ";
	      }
	      std::cout << "s[" << n2bs(n,t_state_print_bits) << "] i[" <<
		n2bs (q, d_n_code_inputs+1) << "]@ " << t_metric << "\n";
#endif
// new metric is better; copy that info into the "to" state
              t_to_state->d_max_metric = t_metric;
              t_to_state->d_max_state_ndx = n;
              t_to_state->d_max_input = q;
            }
// finished (for) this input value
	  }
// finished (for) this state
	}
// done with all states and all inputs; now update the traceback structure
// change which d_states' index to use as starting
        d_states_ndx ^= 1;
// get the state's index for the "to" set of new inputs to get the
// "max" stuff from
	t_state = d_states[d_states_ndx];
// update the traceback structure
// loop over all current states
	traceback_t_ptr t_prev_out_bufs = d_out_buf[d_time_count];
	traceback_t_ptr t_out_bufs = d_out_buf[d_time_count+1];
#if DO_PRINT_DEBUG_MIDDLE_1
	std::cout << "d_o_b[" << d_time_count+1 << "] => d_o_b prev\n";
#endif
	for (size_t n = d_n_states; n > 0; n--, t_state++) {
// simple: get the current state & output state
// and connect output to the current, set inputs on output
	  traceback_t_ptr t_out_buf = t_out_bufs++;
	  traceback_t_ptr t_prev_out_buf =
	    &(t_prev_out_bufs[t_state->d_max_state_ndx]);
#if DO_PRINT_DEBUG_MIDDLE_1
	  std::cout << "s[" << n2bs(d_n_states-n,t_state_print_bits) <<
	    "]: max_ndx = " <<
	    n2bs(t_state->d_max_state_ndx,t_state_print_bits) <<
	    ", input = " << n2bs(t_state->d_max_input, d_n_code_inputs+1) <<
	    ": " << t_out_buf << " => " << t_prev_out_buf << "\n";
#endif
	  t_out_buf->d_prev = t_prev_out_buf;
	  t_out_buf->d_inputs = t_state->d_max_input;
// finished doing this state
	}
// increment the in_buf index, depending on mux'ing or not
	t_in_buf_ndx += (d_do_mux_inputs == false) ? 1 : d_n_code_outputs;
// increment the time counter
        d_time_count++;
// finished (while) staying in this fsm state or not
      }
      if ((d_block_size_bits != 0) &
          (d_time_count == d_block_size_bits)) {
#if DO_PRINT_DEBUG_FSM
	std::cout << "Setting FSM to fsm_dec_viterbi_doing_term\n";
#endif
        d_fsm_state = fsm_dec_viterbi_doing_term;
      }
      break;
#if DO_PRINT_DEBUG_FSM
      std::cout << "Exited fsm_dec_viterbi_doing_middle\n";
#endif
    case (fsm_dec_viterbi_doing_term):
#if DO_PRINT_DEBUG_FSM
      std::cout << "Entered fsm_dec_viterbi_doing_term\n";
#endif
// set the "next" up_down index to the end of their states
      size_t t_time_count = d_total_n_delays - (d_time_count - d_block_size_bits);
      t_n_up_down_ndx = 1 << (d_n_code_inputs * t_time_count);
// stay in this state until the correct number of input symbols are
// reached; exit also if we run out of input symbols to process
      while ((t_time_count > 0) &
	     (t_in_buf_ndx < t_ninput_items)) {
#if DO_PRINT_DEBUG_TERM
	std::cout << "Doing time " << (d_total_n_delays - t_time_count + 1) <<
	  " of " << d_total_n_delays << "; starting buf[" << t_in_buf_ndx <<
	  "] of [" << t_ninput_items << "]\n";
#endif
// use the "to" states,
// FIXME: loop over just the "0" inputs
// and compute the metric for
// each, compare & store it in the "to" state at the end of the connection.

// reset the "to" state's metrics
        reset_metrics (d_states_ndx ^ 1);
// reset the state's index for each set of new inputs
	size_t* t_state_ndx_ptr = d_up_term_states_ndx[d_up_term_ndx];
	size_t* t_next_state_ndx_ptr = d_up_term_states_ndx[d_up_term_ndx ^ 1];
// loop over all current stored "term" state indeces
	for (size_t n = 0; n < t_n_up_down_ndx; n++) {
	  size_t t_state_ndx = *t_state_ndx_ptr++;
	  state_t_ptr t_state = &(d_states[d_states_ndx][t_state_ndx]);
// loop over just the all 0 input value (d_connections[0])
	  connection_t_ptr t_connection = t_state->d_connections;
// get the "to" state
          state_t_ptr t_to_state = t_connection->d_to;
// start with this state's metric
          float t_metric = t_state->d_max_metric;
// get that state's metric
          float t_to_metric = t_to_state->d_max_metric;
#if DO_PRINT_DEBUG_TERM
	  std::cout << "Term state " << (n+1) << " of " << t_n_up_down_ndx <<
	    "; using d_s[" << d_states_ndx << "][" <<
	    n2bs(t_state_ndx,t_state_print_bits) << "]\n";
#endif
// see if the computation is even needed;
// maximum change is d_n_code_outputs if all bits match exactly
          if ((t_to_metric - t_metric) > ((float) 2*d_n_code_outputs)) {
#if DO_PRINT_DEBUG_TERM
	    std::cout << "!not computing metric!\n";
#endif
	    continue;
	  }
// metrics are close enough; do the computation and the compare
// get the output bits for this connection
          float* t_output_bit = t_connection->d_output_bits;
	  if (d_do_mux_inputs == true) {
// if using mux'ed input streams, handle differently
            const float* t_in_buf = &(in_buf[0][t_in_buf_ndx]);
// loop over all encoder-output values
	    for (size_t r = d_n_code_outputs; r > 0; r--) {
	      t_metric += ((*t_in_buf++) * (*t_output_bit++));
	    }
	  } else {
// loop over all encoder-output values
	    for (size_t r = 0; r < d_n_code_outputs; r++) {
              t_metric += (in_buf[r][t_in_buf_ndx] * (*t_output_bit++));
	    }
	  }
// done with this input value; compare old and new metrics
// see if it's the best metric
          if (t_metric > t_to_metric) {
#if DO_PRINT_DEBUG_TERM
	    std::cout << "New Metric to s[" <<
	      n2bs (t_connection->d_to_ndx,t_state_print_bits) << "]: ";
	    if (t_to_state->d_max_metric == -1e10) {
	      std::cout << "setting to ";
	    } else {
	      std::cout << "was s[" <<
		n2bs(t_to_state->d_max_state_ndx,t_state_print_bits) <<
		"]i[" << n2bs (t_to_state->d_max_input, d_n_code_inputs+1) <<
		"]@ " << t_to_state->d_max_metric << "  now ";
	    }
	    std::cout << "s[" << n2bs(t_state_ndx,t_state_print_bits) <<
	      "] i[" << n2bs (0, d_n_code_inputs+1) << "]@ " << t_metric << "\n";
#endif
            t_to_state->d_max_metric = t_metric;
            t_to_state->d_max_state_ndx = t_state_ndx;
            t_to_state->d_max_input = 0;
	  }
// add the "to" state's index to the "next" set of state's indices list
          *t_next_state_ndx_ptr++ = t_connection->d_to_ndx;
// finished (for) this state
	}
// done with all states and all inputs; now update the traceback structure
// change which d_states' index to use as starting
        d_states_ndx ^= 1;
// update the number of "up_term" states
        d_up_term_ndx ^= 1;
// reduce the number of using states
        t_n_up_down_ndx >>= d_n_code_inputs;
// reset the state's index for each set of new inputs
	t_state_ndx_ptr = d_up_term_states_ndx[d_up_term_ndx];
// update the traceback structure
// loop over all current states
	traceback_t_ptr t_prev_out_bufs = d_out_buf[d_time_count];
	traceback_t_ptr t_out_bufs = d_out_buf[d_time_count+1];
#if DO_PRINT_DEBUG_TERM_1
	std::cout << "d_o_b[" << d_time_count+1 << "] => d_o_b prev\n";
#endif
// loop over all current stored "term" state indices
	for (size_t n = 0; n < t_n_up_down_ndx; n++) {
// get the start index and then pointer to each of the "to" states,
// which hold the newest "max" stuff
	  size_t t_state_ndx = *t_state_ndx_ptr++;
	  state_t_ptr t_state = &(d_states[d_states_ndx][t_state_ndx]);
// simple: get the current state & output state
// and connect output to the current, set inputs on output
	  traceback_t_ptr t_out_buf = &(t_out_bufs[t_state_ndx]);
	  traceback_t_ptr t_prev_out_buf =
	    &(t_prev_out_bufs[t_state->d_max_state_ndx]);
#if DO_PRINT_DEBUG_TERM_1
	  std::cout << "ndx[" << n << "] == " << t_state_ndx <<
	    ", s[" << n2bs(t_state_ndx,t_state_print_bits) <<
	    "]: max_ndx = " <<
	    n2bs(t_state->d_max_state_ndx,t_state_print_bits) <<
	    ", input = " << n2bs(t_state->d_max_input, d_n_code_inputs+1) <<
	    ": " << t_out_buf << " => " << t_prev_out_buf << "\n";
#endif
	  t_out_buf->d_prev = t_prev_out_buf;
	  t_out_buf->d_inputs = t_state->d_max_input;
// finished (for) this state
	}
// increment the in_buf index, depending on mux'ing or not
	t_in_buf_ndx += (d_do_mux_inputs == false) ? 1 : d_n_code_outputs;
// increment the time counters
        t_time_count--;
        d_time_count++;
// finished (while) staying in this fsm state or not
      }
      if (t_time_count == 0) {
// finished this trellis, now move as much of the decoded input to the
// output buffer as possible
#if DO_PRINT_DEBUG_FSM
	std::cout << "Setting FSM to fsm_dec_viterbi_output\n";
#endif
	d_fsm_state = fsm_dec_viterbi_output;
      }
#if DO_PRINT_DEBUG_FSM
      std::cout << "Exited fsm_dec_viterbi_doing_term\n";
#endif
      break;
    case (fsm_dec_viterbi_output):
#if DO_PRINT_DEBUG_FSM
      std::cout << "Entered fsm_dec_viterbi_output.\n";
#endif
// this is done in reverse bit order (last time to first time)
// using the traceback structure in d_out_buf, starting with
// the maximum valued one in the last time slot, then using
// the traceback's "d_prev" link to trace the trellis back

// see where the output data will terminate
      int t_next_out_buf_ndx = (t_out_buf_ndx +
				(d_block_size_bits / g_num_bits_per_byte));
      int t_next_out_bit_shift = (t_out_bit_shift +
				  (d_block_size_bits % g_num_bits_per_byte));
      if (t_next_out_bit_shift >= g_num_bits_per_byte) {
	t_next_out_bit_shift -= g_num_bits_per_byte;
	t_next_out_buf_ndx++;
      }
#if DO_PRINT_DEBUG_OUTPUT
      std::cout << "First bit in at out[][" << t_out_buf_ndx << "].[" <<
	t_out_bit_shift << "] -> " << d_block_size_bits << " bits == " <<
	(d_block_size_bits / g_num_bits_per_byte) << " Byte" <<
	((d_block_size_bits / g_num_bits_per_byte) != 1 ? "s" : "") <<
	" + " << (d_block_size_bits % g_num_bits_per_byte) << " bit" <<
	((d_block_size_bits % g_num_bits_per_byte) != 1 ? "s" : "") <<
	"\nNext set of bits start at out[][" << t_next_out_buf_ndx <<
	"].[" << t_next_out_bit_shift << "]\n";
#endif
// find the starting traceback structure
      traceback_t_ptr t_out_buf;
      if (d_do_termination == true) {
// FIXME: assume termination state == 0
#if DO_PRINT_DEBUG_OUTPUT_0
	std::cout << "Using termination; going through trellis for " <<
	  d_total_n_delays << " bit" <<
	  ((d_total_n_delays != 1) ? "s" : "") << "\n";
#endif
	t_out_buf = &(d_out_buf[d_time_count][0]);
#if DO_PRINT_DEBUG_OUTPUT_0
	std::cout << "Starting traceback ptr " << t_out_buf << "\n";
#endif
// skip over the termination bits
	for (size_t n = d_total_n_delays; n > 0; n--) {
	  t_out_buf = t_out_buf->d_prev;
#if DO_PRINT_DEBUG_OUTPUT_0
	  std::cout << "Next traceback ptr " << t_out_buf << "\n";
#endif
	}
      } else {
// no termination but doing block coding;
// FIXME: use the state with the bext metric
// get the state's index for each set of new inputs
	state_t_ptr t_state = d_states[d_states_ndx];
	float t_max_metric = t_state->d_max_metric;
	size_t t_max_state_ndx = 0;
	t_state++;
// loop over all current states
	for (size_t n = 1; n < d_n_states; n++, t_state++) {
// start with this state's metric
	  float t_metric = t_state->d_max_metric;
// compare with current max
	  if (t_metric > t_max_metric) {
	    t_max_metric = t_metric;
	    t_max_state_ndx = n;
	  }
	}
// get the correct out_buf reference to start with
	t_out_buf = &(d_out_buf[d_time_count][t_max_state_ndx]);
      }
#if DO_PRINT_DEBUG_OUTPUT
      std::cout << "Found starting traceback ptr " << t_out_buf <<
	"; getting all " << d_block_size_bits << " bits per stream.\n";
#endif
// now we've got the starting traceback structure address;
// check to make sure there is enough space in each output buffer
      size_t t_block_bit_ndx = d_block_size_bits;
      if ((t_next_out_buf_ndx > t_noutput_bytes) |
	  ((t_next_out_buf_ndx == t_noutput_bytes) &
	   (t_next_out_bit_shift != 0))) {
// not enough space for all output bits;
// find the number of extra bits to save
	size_t t_save_buf_ndx = t_next_out_buf_ndx - t_noutput_bytes;
	size_t t_n_extra_bits = ((t_save_buf_ndx * g_num_bits_per_byte) +
				 t_next_out_bit_shift);
// set the number of saved bits
	d_n_saved_bits = t_n_extra_bits;
// remove the extra bits from the number to copy to the output buffer
	t_block_bit_ndx -= t_n_extra_bits;
// find the actual output buf index, once we get there
	t_next_out_buf_ndx -= t_save_buf_ndx;
#if DO_PRINT_DEBUG_OUTPUT
	std::cout << "Not enough output buffer space:\n" <<
	  "len (o_b) = " << t_noutput_bytes << " ==> " <<
	  t_n_extra_bits << " extra bit" <<
	  (t_n_extra_bits != 1 ? "s" : "") << " == " <<
	  t_save_buf_ndx << " Byte" <<
	  (t_save_buf_ndx != 1 ? "s" : "") << ", " <<
	  t_next_out_bit_shift << " bit" <<
	  (t_next_out_bit_shift != 1 ? "s" : "") << "\n";
#endif
// zero each output buffers' bytes
	size_t t_n_zero = t_save_buf_ndx + (t_next_out_bit_shift ? 1 : 0);
#if DO_PRINT_DEBUG_OUTPUT
	size_t t_n_out_bytes = ((d_block_size_bits / g_num_bits_per_byte) +
				((d_block_size_bits % g_num_bits_per_byte) ? 1 : 0));
	std::cout << "Zeroing save buffer from for " << t_n_zero <<
	  " Byte" << (t_n_zero != 1 ? "s" : "") << " (of " <<
	  d_block_size_bits << " bit" <<
	  (d_block_size_bits != 1 ? "s" : "") << " == " << t_n_out_bytes <<
	  " Byte" << (t_n_out_bytes != 1 ? "s" : "") << ")\n";
#endif
	for (size_t m = 0; m < d_n_code_inputs; m++)
	  bzero (d_save_buffer[m], t_n_zero);
// loop over all extra bits
	for (size_t n = t_n_extra_bits; n > 0; n--) {
// first decrement the output bit & byte as necessary
	  if (--t_next_out_bit_shift < 0) {
	    t_next_out_bit_shift += g_num_bits_per_byte;
	    t_save_buf_ndx--;
	  }
// get the encoder inputs to output
	  size_t t_inputs = t_out_buf->d_inputs;
// loop over all code inputs (decoder-outputs), 1 bit per stream
	  for (size_t m = 0; m < d_n_code_inputs; m++) {
	    d_save_buffer[m][t_save_buf_ndx] |=
	      ((t_inputs & 1) << t_next_out_bit_shift);
	    t_inputs >>= 1;
	  }
	  t_out_buf = t_out_buf->d_prev;
#if DO_PRINT_DEBUG_OUTPUT_0
	  std::cout << "Next traceback ptr " << t_out_buf << "\n";
#endif
	}
// at exit, "t_out_buf_ndx" should be == t_noutput_bytes, and
// "t_out_bit_shift" should be 0; check these!
#if DO_PRINT_DEBUG_OUTPUT
	std::cout << "n_o_b_ndx (" << t_next_out_buf_ndx << ") ?=? " <<
	  "#o_B (" << t_noutput_bytes << ") & t_o_b_sh (" <<
	  t_next_out_bit_shift << ") ?=? 0\n";
	assert (t_next_out_buf_ndx == t_noutput_bytes);
	assert (t_next_out_bit_shift == 0);
#endif
      }
// set the correct output buffer index and bit shift
      t_out_bit_shift = t_next_out_bit_shift;
      t_out_buf_ndx = t_next_out_buf_ndx;
// copy any remaining bits looping backwards in bit-time
// through the traceback trellis
      for (size_t n = t_block_bit_ndx; n > 0; n--) {
// first decrement the output bit & byte as necessary
	if (--t_out_bit_shift < 0) {
	  t_out_bit_shift += g_num_bits_per_byte;
	  t_out_buf_ndx--;
	}
// get the encoder inputs to output
	size_t t_inputs = t_out_buf->d_inputs;
// loop over all code inputs (decoder-outputs), 1 bit per stream
	for (size_t m = 0; m < d_n_code_inputs; m++) {
	  out_buf[m][t_out_buf_ndx] |= ((t_inputs & 1) << t_out_bit_shift);
	  t_inputs >>= 1;
	}
	t_out_buf = t_out_buf->d_prev;
#if DO_PRINT_DEBUG_OUTPUT_0
	std::cout << "Next traceback ptr " << t_out_buf << "\n";
#endif
      }
// set the next output byte and bit-shift
      t_out_bit_shift = t_next_out_bit_shift;
      t_out_buf_ndx = t_next_out_buf_ndx;
#if DO_PRINT_DEBUG_FSM
      std::cout << "Set FSM to fsm_dec_viterbi_init\n";
#endif
      d_fsm_state = fsm_dec_viterbi_init;
#if DO_PRINT_DEBUG_FSM
      std::cout << "Exited fsm_dec_viterbi_output\n";
#endif
      break;
    case (fsm_dec_viterbi_init):
#if DO_PRINT_DEBUG_FSM
      std::cout << "Entered fsm_dec_viterbi_init\n";
#endif
// this is called immediately (first input bit upon startup),
// or after termination of a trellis.

// reset states to the 0'th one
      d_up_term_ndx = d_states_ndx = 0;
// zero the metrics for those states
      zero_metrics (0);
#if 0
// might not need to do this; check and see after it works
// reset up_term states and number so that there is 1 item, the "0" state.
      bzero (d_up_term_states_ndx[0], sizeof (size_t) * d_n_states);
      bzero (d_up_term_states_ndx[1], sizeof (size_t) * d_n_states);
#else
      d_up_term_states_ndx[0][0] = 0;
#endif
// reset time count back to the start
      d_time_count = 0;
#if 0
// reset the traceback structures
// might not need to do this; check and see after it works
      traceback_t_hdl t_out_bufs = d_out_buf;
      for (size_t n = d_n_traceback_els; n > 0; n--) {
	traceback_t_ptr t_out_buf = (*t_out_bufs++);
	for (size_t m = d_n_states; m > 0; m--, t_out_buf++) {
	  t_out_buf->d_prev = NULL;
	  t_out_buf->d_inputs = -1;
	}
      }
#endif
// set the fsm to "doing up"
#if DO_PRINT_DEBUG_FSM
      std::cout << "Set FSM to fsm_dec_viterbi_doing_up\n";
#endif
      d_fsm_state = fsm_dec_viterbi_doing_up;
#if DO_PRINT_DEBUG_FSM
      std::cout << "Exited fsm_dec_viterbi_init\n";
#endif
      break;
// should never get here!
    default:
      assert (0);
// done (switch) with FSM
    }
// done (while) there are inputs
  }

// consume all of the input items on all input streams
// "ninput_items[]" doesn't seem to be reliable,
// so compute this from the actual number of blocks processed
#if DO_PRINT_DEBUG_EXIT
  std::cout << "Exiting FSM in state: " <<
    (d_fsm_state == fsm_dec_viterbi_init ? "init" :
     (d_fsm_state == fsm_dec_viterbi_doing_up ? "up" :
      (d_fsm_state == fsm_dec_viterbi_doing_middle ? "middle" :
       (d_fsm_state == fsm_dec_viterbi_doing_term ? "term" :
	(d_fsm_state == fsm_dec_viterbi_output ? "output" : "unknown"))))) << "\n" <<
    "Consuming " << t_in_buf_ndx <<
    " input items on each input stream (of " << t_ninput_items << ").\n";
#endif
  consume_each (t_in_buf_ndx);

// make sure the number of output items makes sense
// t_out_buf_ndx always points to the current index
// t_out_bit_shift always points to the next bit position to be written
  int t_leftover_bytes = t_out_buf_ndx % d_out_stream_el_size_bytes;
  int t_leftover_bits = ((t_leftover_bytes * g_num_bits_per_byte) +
			 t_out_bit_shift);
  int t_noutput_items = noutput_items;
#if DO_PRINT_DEBUG_EXIT
  std::cout << "Final o_b[" << t_out_buf_ndx << "][" <<
    t_out_bit_shift << "] of " << t_noutput_bytes <<
    ", el_size = " << d_out_stream_el_size_bytes <<
    ", lo_Bytes = " << t_leftover_bytes <<
    ", t_lo_bits = " << t_leftover_bits << "\n" <<
    "Desired # output items = " << noutput_items << "\n";
#endif
  if (t_leftover_bits != 0) {
    // should never get here!
#if 1
    assert (0);
#else
    int t_ndx = t_out_buf_ndx - t_leftover_bytes;
    size_t t_n_copy = t_leftover_bytes + ((t_out_bit_shift != 0) ? 1 : 0);
    assert (t_n_copy <= d_out_stream_el_size_bytes);
// copy the leftover into the save buffer
    for (size_t n = 0; n < d_n_code_inputs; n++) {
      bcopy (&(out_buf[n][t_ndx]), d_save_buffer[n], t_n_copy);
    }
    t_noutput_items = t_ndx / d_out_stream_el_size_bytes;
    d_n_saved_bits = t_leftover_bits;
#if DO_PRINT_DEBUG_EXIT
    std::cout << "Copied " << t_n_copy << " Byte" <<
      (t_n_copy != 1 ? "s" : "") << " from o_b[][" << t_ndx <<
      "] into each save buffer.\n" <<
      "Actual #output items = " << t_noutput_items <<
      ", # saved bit(s) = " << d_n_saved_bits << "\n";
#endif
#endif
  }

#endif

#if DO_TIME_THOUGHPUT
  u_long d_t = end_timer (&t_tp);

  std::cout << "dec_blk_conv_soft_full: Completed " << t_ninput_items <<
    " bits in " << d_t << " usec => " <<
    1e6*(((double)(t_ninput_items))/((double) d_t)) <<
    " b/s\n";
#endif
}

#if 0

void
decoder_viterbi::encode_loop_up ()
{
#if DO_PRINT_DEBUG_FSM
  std::cout << "Starting fsm_dec_viterbi_doing_up\n";
#endif

  // set the number of up_down indices

  size_t t_n_up_down_ndx = 1 << (d_n_code_inputs *
				 d_time_count);

// stay in this state until the correct number of input symbols are
// reached; exit also if we run out of input symbols to process
      while ((d_time_count < d_total_n_delays) &
	     (t_in_buf_ndx < t_ninput_items)) {
#if DO_PRINT_DEBUG_UP_0
	std::cout << "Doing 'while' loop:\n" <<
	  "t_n_up_down_ndx    = " << t_n_up_down_ndx << "\n" <<
	  "d_time_count       = " << d_time_count << "\n" <<
	  "d_total_n_delays   = " << d_total_n_delays << "\n" <<
	  "t_in_buf_ndx       = " << t_in_buf_ndx << "\n" <<
	  "t_ninput_items     = " << t_ninput_items << "\n";
#endif
// use the "from" states, loop over all inputs and compute the metric for
// each & store it in the "to" state at the end of the connection.
// no need to compare metrics yet, since none join into any given state

#if 0
// reset the "to" state's metrics
// probably don't need to do this; try removing later
        reset_metrics (d_states_ndx ^ 1);
#if DO_PRINT_DEBUG_UP
	std::cout << "Reset Metrics\n";
#endif
#endif

// reset the state's index for each set of new inputs
	size_t* t_state_ndx_ptr = d_up_term_states_ndx[d_up_term_ndx];
	size_t* t_next_state_ndx_ptr = d_up_term_states_ndx[d_up_term_ndx ^ 1];
// loop over all current stored "up" states
	for (size_t n = 0; n < t_n_up_down_ndx; n++) {
	  size_t t_state_ndx = *t_state_ndx_ptr++;
// get a pointer to this state's structure
	  state_t_ptr t_state = &(d_states[d_states_ndx][t_state_ndx]);
// get the connections for all inputs
	  connection_t_ptr t_connection = t_state->d_connections;
#if DO_PRINT_DEBUG_UP_0
	  std::cout << "Looping over all 'up' states:\n" <<
	    "n                  = " << n << "\n" <<
	    "t_n_up_down_ndx    = " << t_n_up_down_ndx << "\n" <<
	    "d_states_ndx       = " << d_states_ndx << "\n" <<
	    "t_state_ndx        = " << t_state_ndx << "\n" <<
	    "d_n_input_combs    = " << d_n_input_combinations << "\n" <<
	    "t_state            = " << t_state << "\n" <<
	    "t_connection       = " << t_connection << "\n";
#endif
// loop over all possible input values, 1 bit per input stream
	  for (size_t q = 0; q < d_n_input_combinations; q++, t_connection++) {
// find the "to" state for this connection
	    state_t_ptr t_to_state = t_connection->d_to;
// get the output bits for this connection
	    float* t_output_bit = t_connection->d_output_bits;
// start with this state's metric
	    float t_metric = t_state->d_max_metric;
#if DO_PRINT_DEBUG_UP_0
	    std::cout <<
	      "to state index     = " << t_connection->d_to_ndx << "\n" <<
	      "current metric     = " << t_metric << "\n";
#endif
	    if (d_do_mux_inputs == true) {
// if using mux'ed input streams, handle differently
              const float* t_in_buf = &(in_buf[0][t_in_buf_ndx]);
// loop over all encoder-output values
	      for (size_t r = d_n_code_outputs; r > 0; r--) {
#if DO_PRINT_DEBUG_UP
		std::cout << "in_sym = " << *t_in_buf << ", code_out_bit = " <<
		  *t_output_bit << " ==> metric -> ";
#endif
  	        t_metric += ((*t_in_buf++) * (*t_output_bit++));
#if DO_PRINT_DEBUG_UP
		std::cout << t_metric << "\n";
#endif
	      }
	    } else {
// loop over all encoder-output values
	      for (size_t r = 0; r < d_n_code_outputs; r++) {
#if DO_PRINT_DEBUG_UP
		std::cout << "in_sym = " << in_buf[r][t_in_buf_ndx] <<
		  ", code_out_bit = " << *t_output_bit << " ==> metric -> ";
#endif
		t_metric += (in_buf[r][t_in_buf_ndx] * (*t_output_bit++));
#if DO_PRINT_DEBUG_UP
		std::cout << t_metric << "\n";
#endif
	      }
	    }
// get the "to" state index
	    size_t t_to_ndx = t_connection->d_to_ndx;
// store the metric in the "to" state; should not have been used before
	    t_to_state->d_max_metric = t_metric;
// add the "to" state index to the "up" state list
	    *t_next_state_ndx_ptr++ = t_to_ndx;
// update the traceback structure, depending on which variety it is
// doing full trellis before decoding; use d_out_buf
// simple: get the current state & output state
	    traceback_t_ptr t_out_buf = &(d_out_buf[d_time_count]
					  [t_state_ndx]);
	    traceback_t_ptr t_next_out_buf = &(d_out_buf[d_time_count+1]
					       [t_to_ndx]);
#if DO_PRINT_DEBUG_UP_1
	    std::cout << "d_o_b[" << d_time_count+1 << "] => d_o_b prev\n" <<
	      "ndx[" << n << "] == " << t_state_ndx <<
	      ", s[" << n2bs(t_state_ndx,t_state_print_bits) <<
	      "]: max_ndx = " <<
	      n2bs(t_state->d_max_state_ndx,t_state_print_bits) <<
	      ", input = " << n2bs(q, d_n_code_inputs+1) <<
	      ": " << t_next_out_buf << " => " << t_out_buf << "\n";
#endif
// and connect output to the current, set inputs on output
	    t_next_out_buf->d_prev = t_out_buf;
	    t_next_out_buf->d_inputs = q;
// finished (for) this input value
	  }
// finished (for) this "up_term" state
	}
// increment the in_buf index, depending on mux'ing or not
	t_in_buf_ndx += (d_do_mux_inputs == false) ? 1 : d_n_code_outputs;
// increment the time counter
        d_time_count++;
// update the number of "up_term" states
        d_up_term_ndx ^= 1;
// increase the number of using states
        t_n_up_down_ndx <<= d_n_code_inputs;
// change which d_states' index to use as starting
        d_states_ndx ^= 1;
// finished (while) staying in this fsm state or not
      }
// if reached the end of doing the "up" part of the trellis,
// switch states into the middle
      if (d_time_count == d_total_n_delays) {
#if DO_PRINT_DEBUG_FSM
	std::cout << "Setting FSM to fsm_dec_viterbi_doing_middle\n";
#endif
        d_fsm_state = fsm_dec_viterbi_doing_middle;
      }
#if DO_PRINT_DEBUG_FSM
      std::cout << "Exited fsm_dec_viterbi_doing_up\n";
#endif
}
#endif
