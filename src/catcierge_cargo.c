//
// This file is part of the Catcierge project.
//
// Copyright (c) Joakim Soderberg 2013-2015
//
//    Catcierge is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 2 of the License, or
//    (at your option) any later version.
//
//    Catcierge is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Catcierge.  If not, see <http://www.gnu.org/licenses/>.
//
#include "alini.h"
#include "cargo.h"
#include "cargo_ini.h"
#include "catcierge_args.h"

static int add_lockout_options(cargo_t cargo, catcierge_args_t *args)
{
	int ret = 0;

	ret |= cargo_add_group(cargo, 0, "lockout", "Lockout settings", NULL);

	// TODO: Verify value
	ret |= cargo_add_option(cargo, 0,
			"<lockout> --lockout_method",
			"Defines the method used to decide when to unlock:\n"
			"[1: Only use the timer, don't care about clear frame.]\n"
			"2: Wait for clear frame or that the timer has timed out.\n"
			"3: Wait for clear frame and then start unlock timer.\n",
			"i", &args->lockout_method);

	ret |= cargo_add_option(cargo, 0,
			"<lockout> --lockout",
			NULL,
			"i", &args->lockout_time);
	ret |= cargo_set_option_description(cargo, "--lockout",
			"The time in seconds a lockout takes. Default %d seconds.",
			DEFAULT_LOCKOUT_TIME);

	ret |= cargo_add_option(cargo, 0,
			"<lockout> --lockout_error",
			"Number of lockouts in a row that's allowed before we "
			"consider it an error and quit the program. "
			"Default is to never do this.",
			"i", &args->max_consecutive_lockout_count);

	ret |= cargo_add_option(cargo, 0,
			"<lockout> --lockout_error_delay",
			NULL,
			"i", &args->max_consecutive_lockout_count);
	ret |= cargo_set_option_description(cargo, "--lockout_error_delay",
			"The delay in seconds between lockouts that should be "
			"counted as a consecutive lockout. Default %0.1f.",
			DEFAULT_CONSECUTIVE_LOCKOUT_DELAY);

	ret |= cargo_add_option(cargo, 0,
			"<lockout> --lockout_dummy",
			"Do everything as normal, but don't actually "
			"lock the door. This is useful for testing.",
			"i", &args->max_consecutive_lockout_count);

	return ret;
}

static int add_presentation_options(cargo_t cargo, catcierge_args_t *args)
{
	int ret = 0;

	ret |= cargo_add_group(cargo, 0, "pres", "Presentation settings", NULL);

	ret |= cargo_add_option(cargo, 0,
			"<pres> --show",
			"Show GUI of the camera feed (X11 only).",
			"b", &args->show);

	ret |= cargo_add_option(cargo, 0,
			"<pres> --highlight",
			"Highlight the best match on saved images. "
			"(Only ever use for debugging purposes, "
			"since it writes on the original image)",
			"b", &args->highlight_match);

	ret |= cargo_add_option(cargo, 0,
			"<pres> --nocolor",
			"Turn off all color output in the console.",
			"b", &args->nocolor);

	ret |= cargo_add_option(cargo, 0,
			"<pres> --noanim",
			"Turn off animations in the console.",
			"b", &args->noanim);

	return ret;
}

static int add_matcher_options(cargo_t cargo, catcierge_args_t *args)
{
	//
	// Adds options meant for the matcher algorithm chosen.
	//
	int ret = 0;

	// TODO: Add functions for matcher arguments

	ret |= cargo_add_group(cargo, 0, "matcher_opts", "Matcher settings", NULL);

	// TODO: Verify between 0 and MATCH_MAX_COUNT
	ret |= cargo_add_option(cargo, 0,
			"<matcher_opts> --ok_matches_needed", NULL,
			"b", &args->ok_matches_needed);
	ret |= cargo_set_option_description(cargo,
			"--ok_matches_needed",
			"The number of matches out of %d matches "
			"that need to be OK for the match to be considered "
			"an over all OK match", MATCH_MAX_COUNT);

	return ret;
}

static int parse_CvRect(cargo_t ctx, void *user, const char *optname,
                                 int argc, char **argv)
{
	CvRect *rect = (CvRect *)user;

	if (argc != 4)
	{
		cargo_set_error(ctx, 0,
			"Not enough arguments for %s, expected (x, y, width, height) "
			"but only got %d values",
			optname, argc);
		return -1;
	}

	*rect = cvRect(atoi(argv[0]), atoi(argv[1]), atoi(argv[2]), atoi(argv[3]));

	return argc;
}

static int add_options(cargo_t cargo, catcierge_args_t *args)
{
	int ret = 0;

	ret |= cargo_add_option(cargo, 0,
			"--config -c",
			"Path to config file",
			"s", &args->config_path);

	ret |= cargo_add_option(cargo, 0,
			"--chuid",
			"Run the process under this user when dropping root privileges "
			"which are needed for setting GPIO pins on the Raspberry Pi.",
			"s", &args->chuid);

	ret |= cargo_add_option(cargo, 0,
			"--startup_delay",
			"Number of seconds to wait after starting before starting "
			"to capture anything. This is so that if you have a back light "
			"that is turned on at startup, it has time to turn on, otherwise "
			"the program will think something is obstructing the image and "
			"start trying to match.",
			"i", &args->startup_delay);

	ret |= cargo_add_option(cargo, 0,
			"--roi",
			"Crop all input image to this region of interest.",
			"[c]#", parse_CvRect, &args->roi, NULL, 4);
	ret |= cargo_set_metavar(cargo,
			"--roi",
			"X Y WIDTH HEIGHT");

	ret |= cargo_add_mutex_group(cargo,
			CARGO_MUTEXGRP_ONE_REQUIRED |
			CARGO_MUTEXGRP_GROUP_USAGE, 
			"matcher", 
			"Matcher",
			"The algorithm to use when matching for the cat profile image");

	ret |= cargo_add_option(cargo, 0,
			"<!matcher> --template_matcher --templ",
			"Template based matching algorithm",
			"b=", &args->matcher_type, MATCHER_TEMPLATE);

	ret |= cargo_add_option(cargo, 0,
			"<!matcher> --haar_matcher --haar",
			"Haar feature based matching algorithm (recommended)",
			"b=", &args->matcher_type, MATCHER_HAAR);

	ret |= add_matcher_options(cargo, args);

	ret |= add_lockout_options(cargo, args);

	ret |= add_presentation_options(cargo, args);

	return ret;
}

int parse_cmdargs(int argc, char **argv)
{
	int i;
	int ret = 0;
	cargo_t cargo;
	catcierge_args_t args;

	memset(&args, 0, sizeof(args));

	if (cargo_init(&cargo, 0, argv[0]))
	{
		fprintf(stderr, "Failed to init command line parsing\n");
		return -1;
	}

	cargo_set_description(cargo,
		"Catcierge saves you from cleaning the floor!");

	ret = add_options(cargo, &args);

	assert(ret == 0);

	// Parse once to get --config value.
	if (cargo_parse(cargo, 0, 1, argc, argv))
	{
		goto fail;
	}

	// Read ini file and translate that into an argv that cargo can parse.
	printf("Config path: %s\n", args.config_path);
	if (args.config_path
		&& parse_config(cargo, args.config_path, &args.ini_args))
	{
		goto fail;
	}

	// And finally parse the commandline to override config settings.
	if (cargo_parse(cargo, 0, 1, argc, argv))
	{
		goto fail;
	}

fail:
	cargo_destroy(&cargo);
	ini_args_destroy(&args.ini_args);

	return 0;
}