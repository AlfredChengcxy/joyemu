/*
 * joyemu 
 *
 * Main program, reads arguments, initializes input and I/O, starts threads.
 *
 *
 * Copyright (c) 2017 Jani Halme
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list
 *    of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice, this
 *    list of conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
 * WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <getopt.h>
#include <glob.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "defaults.h"
#include "input.h"
#include "logging.h"
#include "ports.h"


// bus and address for MCP23017
int config_i2c_bus=MCP_I2C_BUS_NUMBER;
int config_i2c_base=MCP_I2C_BASE_ADDR;

// thread handles
pthread_t port_io, event_poll;

// user configurable stuff
int config_log_verbosity=LOGLEVEL_INFO;
uint8_t config_joystick_port=2;
uint8_t config_mouse_port=1;
float config_mouse_speed=1.3;


int main(int argc, char **argv) {
  int i, rc, opt;
  static const char *options="i:a:d:m:j:vqh";

  // read command line arguments and set configuration variables accordingly
  while (1) {
    opt=getopt(argc, argv, options);
    switch (opt) {
      case 'q':
      config_log_verbosity++;
      if (config_log_verbosity > LOGLEVEL_ERROR) config_log_verbosity=LOGLEVEL_ERROR;
      break;
      
      case 'v':
      config_log_verbosity--;
      if (config_log_verbosity < LOGLEVEL_EXTRADEBUG) config_log_verbosity=LOGLEVEL_EXTRADEBUG;
      break;
      
      case 'i':
      sscanf(optarg, "%d", &config_i2c_bus);
      if (config_i2c_bus<0) {
        fprintf(stderr, "Invalid I2C bus number - please enter a positive integer number, eg. 1\n");
        exit(EXIT_FAILURE);
      }        
      break;
      
      case 'a':
      sscanf(optarg, "0x%02x", &config_i2c_base);
      if (config_i2c_base<0 || config_i2c_base>0xff) {
        fprintf(stderr, "Invalid I2C base address - please enter a hexadeximal number between 0x00 and 0xff\n");
        exit(EXIT_FAILURE);
      }        
      break;
      
      case 'd':
      break;
      
      case 'm':
      sscanf(optarg, "%d", &config_mouse_port);
      if (config_mouse_port!=1 && config_mouse_port!=2) {
        fprintf(stderr, "Invalid mouse port - please enter either 1 or 2\n");
        exit(EXIT_FAILURE);
      }        
      break;
      
      case 'j':
      sscanf(optarg, "%d", &config_joystick_port);
      if (config_joystick_port!=1 && config_joystick_port!=2) {
        fprintf(stderr, "Invalid joystick port - please enter either 1 or 2\n");
        exit(EXIT_FAILURE);
      }        
      break;

      case 'h':
      default:
      fprintf(stderr, "Usage: %s [-vqh] [-i i2c_bus] [-a i2c_addr] [-d (j1|j2|m):evdev_nr] [-m mouse_port] [-j joystick_port]\n\n", argv[0]);
      fprintf(stderr, "\t-v\tadd verbosity\n\
\t-q\tadd quietness\n\
\t-i n\tset I2C bus number for I/O expander (default: 1)\n\
\t-a 0xnn\tset I2C address for I/O expander as a hexadecimal byte (default: 0x20)\n\
\t-d j1:n\tset event device number for joystick 1\n\
\t-d j2:n\tset event device number for joystick 2\n\
\t-d m:n\tset event device number for mouse\n\
\t-m n\tset mouse port: 1 (default) or 2\n\
\t-j n\tset first joystick port: 1 or 2 (default)\n\
\t-h\tdisplay this help\n");
      exit(EXIT_FAILURE);
      break;
      
      case -1:
      break;
    }
    if (opt==-1) break;
  }

  // set logging verbosity level
  debug_set_verbosity(config_log_verbosity);

  // scan the input devices for suitable gamepads and/or mice
  rc=input_scan_devices(config_mouse_port, config_joystick_port);
  if (rc==GLOB_NOMATCH) {
    debug_log(LOGLEVEL_ERROR, "Could not find any input devices - make sure your devices are powered on and paired - exiting");
    exit(-1);
  }
  else if (rc) {
    debug_log(LOGLEVEL_ERROR, "Error while scanning for input devices - make sure you have permission to access /dev/input - exiting");
    exit(-1);
  }
  if (!input_mouse_connected() && !input_joysticks_connected()) {
    debug_log(LOGLEVEL_ERROR, "No suitable input devices found for emulating either mouse or joysticks - exiting");
    exit(-1);
  } 

  // initialize the I/O expander and start the port I/O thread
  mcp_initialize(MCP_I2C_BUS_NUMBER, MCP_I2C_BASE_ADDR);
  mouse_set_port(config_mouse_port);
  rc=pthread_create(&port_io, NULL, port_io_thread, (void *)NULL);
  if (rc) {
    debug_log(LOGLEVEL_ERROR, "Failed to create port I/O thread - exiting\n");
    exit(-1);
  }

  // wait a second and then start event polling thread
  sleep(1);
  rc=pthread_create(&event_poll, NULL, input_poll_thread, (void *)NULL);
  if (rc) {
    debug_log(LOGLEVEL_ERROR, "Failed to create event poll thread - exiting\n");
    exit(-1);
  }

  // main thread sleeps
  do {
    sleep(1);
  } while (1);

  return 0;
}