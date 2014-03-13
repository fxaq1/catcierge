
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <catcierge_config.h>
#include "catcierge_args.h"
#include "catcierge_log.h"
#include "catcierge_rfid.h"

#ifdef WITH_INI
#include "alini/alini.h"
#endif

static int catcierge_create_rfid_allowed_list(catcierge_args_t *args, const char *allowed)
{
	int i;
	const char *s = allowed;
	char *allowed_cpy = NULL;
	args->rfid_allowed_count = 1;

	if (!allowed || (strlen(allowed) == 0))
	{
		return -1;
	}

	while ((s = strchr(s, ',')))
	{
		args->rfid_allowed_count++;
		s++;
	}

	if (!(args->rfid_allowed = (char **)calloc(args->rfid_allowed_count, sizeof(char *))))
	{
		CATERR("Out of memory\n");
		return -1;
	}

	// strtok changes its input.
	if (!(allowed_cpy = strdup(allowed)))
	{
		free(args->rfid_allowed);
		args->rfid_allowed = NULL;
		args->rfid_allowed_count = 0;
		return -1;
	}

	s = strtok(allowed_cpy, ",");

	for (i = 0; i < args->rfid_allowed_count; i++)
	{
		args->rfid_allowed[i] = strdup(s);
		s = strtok(NULL, ",");

		if (!s)
			break;
	}

	free(allowed_cpy);

	return 0;
}

static void catcierge_free_rfid_allowed_list(catcierge_args_t *args)
{
	int i;

	for (i = 0; i < args->rfid_allowed_count; i++)
	{
		free(args->rfid_allowed[i]);
		args->rfid_allowed[i] = NULL;
	}

	free(args->rfid_allowed);
	args->rfid_allowed = NULL;
}


static int catcierge_parse_setting(catcierge_args_t *args, const char *key, char **values, size_t value_count)
{
	size_t i;

	if (!strcmp(key, "show"))
	{
		args->show = 1;
		if (value_count == 1) args->show = atoi(values[0]);
		return 0;
	}

	if (!strcmp(key, "save"))
	{
		args->saveimg = 1;
		if (value_count == 1) args->saveimg = atoi(values[0]);
		return 0;
	}

	if (!strcmp(key, "highlight"))
	{
		args->highlight_match = 1;
		if (value_count == 1) args->highlight_match = atoi(values[0]);
		return 0;
	}

	if (!strcmp(key, "show_fps"))
	{
		args->show_fps = 1;
		if (value_count == 1) args->show_fps = atoi(values[0]);
		return 0;
	}

	if (!strcmp(key, "lockout"))
	{
		if (value_count == 1)
		{
			args->lockout_time = atoi(values[0]);
		}
		else
		{
			args->lockout_time = DEFAULT_LOCKOUT_TIME;
		}

		return 0;
	}

	if (!strcmp(key, "lockout_error"))
	{
		if (value_count == 1)
		{
			args->max_consecutive_lockout_count = atoi(values[0]);
			return 0;
		}

		return -1;
	}

	if (!strcmp(key, "lockout_dummy"))
	{
		if (value_count == 1)
		{
			args->lockout_dummy = atoi(values[0]);
		}
		else
		{
			args->lockout_dummy = 1;
		}

		return 0;
	}

	if (!strcmp(key, "matchtime"))
	{
		if (value_count == 1)
		{
			args->match_time = atoi(values[0]);
		}
		else
		{
			args->match_time = DEFAULT_MATCH_WAIT;
		}

		return 0;
	}

	if (!strcmp(key, "match_flipped"))
	{
		if (value_count == 1)
		{
			args->match_flipped = atoi(values[0]);
		}
		else
		{
			args->match_flipped = 1;
		}

		return 0;
	}

	if (!strcmp(key, "output"))
	{
		if (value_count == 1)
		{
			args->output_path = values[0];
			return 0;
		}

		return -1;
	}

	#ifdef WITH_RFID
	if (!strcmp(key, "rfid_in"))
	{
		if (value_count == 1)
		{
			args->rfid_inner_path = values[0];
			return 0;
		}

		return -1;
	}

	if (!strcmp(key, "rfid_out"))
	{
		if (value_count == 1)
		{
			args->rfid_outer_path = values[0];
			return 0;
		}

		return -1;
	}

	if (!strcmp(key, "rfid_allowed"))
	{
		if (value_count == 1)
		{
			if (catcierge_create_rfid_allowed_list(args, values[0]))
			{
				CATERR("Failed to create RFID allowed list\n");
				return -1;
			}

			return 0;
		}

		return -1;
	}

	if (!strcmp(key, "rfid_time"))
	{
		if (value_count == 1)
		{
			printf("val: %s\n", values[0]);
			args->rfid_lock_time = (double)atof(values[0]);
			return 0;
		}

		return -1;
	}

	if (!strcmp(key, "rfid_lock"))
	{
		args->lock_on_invalid_rfid = 1;
		if (value_count == 1) args->lock_on_invalid_rfid = atoi(values[0]);
		return 0;
	}
	#endif // WITH_RFID

	// On the command line you can specify multiple snouts like this:
	// --snout_paths <path1> <path2> <path3>
	// or 
	// --snout_path <path1> --snout_path <path2>
	// In the config file you do
	// snout_path=<path1>
	// snout_path=<path2>
	if (!strcmp(key, "snout") || 
		!strcmp(key, "snout_paths") || 
		!strcmp(key, "snout_path"))
	{
		if (value_count == 0)
			return -1;

		for (i = 0; i < value_count; i++)
		{
			if (args->snout_count >= MAX_SNOUT_COUNT) return -1;
			args->snout_paths[args->snout_count] = values[i];
			args->snout_count++;
		}

		return 0;
	}

	if (!strcmp(key, "threshold"))
	{
		if (value_count == 1)
		{
			args->match_threshold = atof(values[0]);
		}
		else
		{
			args->match_threshold = DEFAULT_MATCH_THRESH;
		}

		return 0;
	}

	if (!strcmp(key, "log"))
	{
		if (value_count == 1)
		{
			args->log_path = values[0];
			return 0;
		}

		return -1;
	}

	if (!strcmp(key, "match_cmd"))
	{
		if (value_count == 1)
		{
			args->match_cmd = values[0];
			return 0;
		}

		return -1;
	}

	if (!strcmp(key, "save_img_cmd"))
	{
		if (value_count == 1)
		{
			args->save_img_cmd = values[0];
			return 0;
		}

		return -1;
	}

	if (!strcmp(key, "save_imgs_cmd"))
	{
		if (value_count == 1)
		{
			args->save_imgs_cmd = values[0];
			return 0;
		}

		return -1;
	}

	if (!strcmp(key, "match_done_cmd"))
	{
		if (value_count == 1)
		{
			args->match_done_cmd = values[0];
			return 0;
		}

		return -1;
	}

	if (!strcmp(key, "do_lockout_cmd"))
	{
		if (value_count == 1)
		{
			args->do_lockout_cmd = values[0];
			return 0;
		}

		return -1;
	}

	if (!strcmp(key, "do_unlock_cmd"))
	{
		if (value_count == 1)
		{
			args->do_unlock_cmd = values[0];
			return 0;
		}

		return -1;
	}

	#ifdef WITH_RFID
	if (!strcmp(key, "rfid_detect_cmd"))
	{
		if (value_count == 1)
		{
			args->rfid_detect_cmd = values[0];
			return 0;
		}

		return -1;
	}

	if (!strcmp(key, "rfid_match_cmd"))
	{
		if (value_count == 1)
		{
			args->rfid_match_cmd = values[0];
			return 0;
		}

		return -1;
	}
	#endif // WITH_RFID

	return -1;
}

#ifdef WITH_INI

int temp_config_count = 0;
#define MAX_TEMP_CONFIG_VALUES 128
char *temp_config_values[MAX_TEMP_CONFIG_VALUES];

static void alini_cb(alini_parser_t *parser, char *section, char *key, char *value)
{
	char *value_cpy = NULL;
	catcierge_args_t *args = alini_parser_get_context(parser);

	// We must make a copy of the string and keep it so that
	// it won't dissapear after the parser has done its thing.
	if (temp_config_count >= MAX_TEMP_CONFIG_VALUES)
	{
		fprintf(stderr, "Max %d config file values allowed.\n", MAX_TEMP_CONFIG_VALUES);
		return;
	}

	if (!(value_cpy = strdup(value)))
	{
		fprintf(stderr, "Out of memory\n");
		exit(1);
	}

	temp_config_values[temp_config_count] = value_cpy;

	if (catcierge_parse_setting(args, key, &temp_config_values[temp_config_count], 1) < 0)
	{
		fprintf(stderr, "Failed to parse setting in config: %s\n", key);
	}

	temp_config_count++;
}

static void catcierge_config_free_temp_strings(catcierge_args_t *args)
{
	int i;

	for (i = 0; i < args->temp_config_count; i++)
	{
		free(args->temp_config_values[i]);
		args->temp_config_values[i] = NULL;
	}
}
#endif // WITH_INI

void catcierge_show_usage(catcierge_args_t *args, const char *prog)
{
	fprintf(stderr, "Usage: %s [options]\n\n", prog);
	fprintf(stderr, " --snout <paths>        Path to the snout images to use. If more than one path is\n");
	fprintf(stderr, "                        given, the average match result is used.\n");
	fprintf(stderr, " --threshold <float>    Match threshold as a value between 0.0 and 1.0. Default %.1f\n", DEFAULT_MATCH_THRESH);
	fprintf(stderr, " --lockout <seconds>    The time in seconds a lockout takes. Default %ds\n", DEFAULT_LOCKOUT_TIME);
	fprintf(stderr, " --lockout_error <n>    Number of lockouts in a row is allowed before we\n");
	fprintf(stderr, "                        consider it an error and quit the program. \n");
	fprintf(stderr, "                        Default is to never do this.\n");
	fprintf(stderr, " --lockout_dummy        Do everything as normal, but don't actually lock the door.\n");
	fprintf(stderr, "                        This is useful for testing.\n");
	fprintf(stderr, " --matchtime <seconds>  The time to wait after a match. Default %ds\n", DEFAULT_MATCH_WAIT);
	fprintf(stderr, " --match_flipped <0|1>  Match a flipped version of the snout\n");
	fprintf(stderr, "                        (don't consider going out a failed match). Default on.\n");
	fprintf(stderr, " --show                 Show GUI of the camera feed (X11 only).\n");
	fprintf(stderr, " --show_fps <0|1>       Show FPS. Default is ON.\n");
	fprintf(stderr, " --save                 Save match images (both ok and failed).\n");
	fprintf(stderr, " --highlight            Highlight the best match on saved images.\n");
	fprintf(stderr, " --output <path>        Path to where the match images should be saved.\n");
	fprintf(stderr, " --log <path>           Log matches and rfid readings (if enabled).\n");
	#ifdef WITH_INI
	fprintf(stderr, " --config <path>        Path to config file. Default is ./catcierge.cfg or /etc/catcierge.cfg\n");
	fprintf(stderr, "                        This is parsed as an INI file. The keys/values are the same as these options.\n");
	#endif // WITH_INI
	#ifdef WITH_RFID
	fprintf(stderr, "\n");
	fprintf(stderr, "RFID:\n");
	fprintf(stderr, " --rfid_in <path>       Path to inner RFID reader. Example: /dev/ttyUSB0\n");
	fprintf(stderr, " --rfid_out <path>      Path to the outter RFID reader.\n");
	fprintf(stderr, " --rfid_lock            Lock if no RFID tag present or invalid RFID tag. Default OFF.\n");
	fprintf(stderr, " --rfid_time <seconds>  Number of seconds to wait after a valid match before the\n");
	fprintf(stderr, "                        RFID readers are checked.\n");
	fprintf(stderr, "                        (This is so that there is enough time for the cat to be read by both readers)\n");
	fprintf(stderr, " --rfid_allowed <list>  A comma separated list of allowed RFID tags. Example: %s\n", EXAMPLE_RFID_STR);
	#endif // WITH_RFID
	fprintf(stderr, "\n");
	fprintf(stderr, "Commands:\n");
	fprintf(stderr, "   (Note that %%0, %%1, %%2... will be replaced in the input, see --cmdhelp for details)\n");
	#define EPRINT_CMD_HELP(fmt, ...) if (args->show_cmd_help) fprintf(stderr, fmt, ##__VA_ARGS__);
	EPRINT_CMD_HELP("\n");
	EPRINT_CMD_HELP("   General: %%cwd will output the current working directory for this program.\n");
	EPRINT_CMD_HELP("            Any paths returned are relative to this. %%%% Produces a literal %% sign.\n");
	EPRINT_CMD_HELP("\n");
	fprintf(stderr, " --match_cmd <cmd>      Command to run after a match is made.\n");
	EPRINT_CMD_HELP("                         %%0 = [float] Match result.\n");
	EPRINT_CMD_HELP("                         %%1 = [0/1]   Success or failure.\n");
	EPRINT_CMD_HELP("                         %%2 = [path]  Path to where image will be saved (Not saved yet!)\n");
	EPRINT_CMD_HELP("\n");
	fprintf(stderr, " --save_img_cmd <cmd>   Command to run at the moment a match image is saved.\n");
	EPRINT_CMD_HELP("                         %%0 = [float]  Match result, 0.0-1.0\n");
	EPRINT_CMD_HELP("                         %%1 = [0/1]    Match success.\n");
	EPRINT_CMD_HELP("                         %%2 = [string] Image path (Image is saved to disk).\n");
	EPRINT_CMD_HELP("\n");
	fprintf(stderr, " --save_imgs_cmd <cmd>  Command that runs when all match images have been saved to disk.\n");
	fprintf(stderr, "                        (This is most likely what you want to use in most cases)\n");
	EPRINT_CMD_HELP("                         %%0 = [0/1]    Match success.\n");
	EPRINT_CMD_HELP("                         %%1 = [string] Image path for first match.\n");
	EPRINT_CMD_HELP("                         %%2 = [string] Image path for second match.\n");
	EPRINT_CMD_HELP("                         %%3 = [string] Image path for third match.\n");
	EPRINT_CMD_HELP("                         %%4 = [string] Image path for fourth match.\n");
	EPRINT_CMD_HELP("                         %%5 = [float]  First image result.\n");
	EPRINT_CMD_HELP("                         %%6 = [float]  Second image result.\n");
	EPRINT_CMD_HELP("                         %%7 = [float]  Third image result.\n");
	EPRINT_CMD_HELP("                         %%8 = [float]  Fourth image result.\n");
	EPRINT_CMD_HELP("\n");
	fprintf(stderr, " --match_done_cmd <cmd> Command to run when a match is done.\n");
	EPRINT_CMD_HELP("                         %%0 = [0/1]    Match success.\n");
	EPRINT_CMD_HELP("                         %%1 = [int]    Successful match count.\n");
	EPRINT_CMD_HELP("                         %%2 = [int]    Max matches.\n");
	EPRINT_CMD_HELP("\n");
	#ifdef WITH_RFID
	fprintf(stderr, " --rfid_detect_cmd <cmd> Command to run when one of the RFID readers detects a tag.\n");
	EPRINT_CMD_HELP("                         %%0 = [string] RFID reader name.\n");
	EPRINT_CMD_HELP("                         %%1 = [string] RFID path.\n");
	EPRINT_CMD_HELP("                         %%2 = [0/1]    Is allowed.\n");
	EPRINT_CMD_HELP("                         %%3 = [0/1]    Is data incomplete.\n");
	EPRINT_CMD_HELP("                         %%4 = [string] Tag data.\n");
	EPRINT_CMD_HELP("                         %%5 = [0/1]    Other reader triggered.\n");
	EPRINT_CMD_HELP("                         %%6 = [string] Direction (in, out or uknown).\n");
	EPRINT_CMD_HELP("\n");
	fprintf(stderr, " --rfid_match_cmd <cmd> Command that is run when a RFID match is made.\n");
	EPRINT_CMD_HELP("                         %%0 = Match success.\n");
	EPRINT_CMD_HELP("                         %%1 = RFID inner in use.\n");
	EPRINT_CMD_HELP("                         %%2 = RFID outer in use.\n");
	EPRINT_CMD_HELP("                         %%3 = RFID inner success.\n");
	EPRINT_CMD_HELP("                         %%4 = RFID outer success.\n");
	EPRINT_CMD_HELP("                         %%5 = RFID inner data.\n");
	EPRINT_CMD_HELP("                         %%6 = RFID outer data.\n");
	EPRINT_CMD_HELP("\n");
	fprintf(stderr, " --do_lockout_cmd <cmd> Command to run when the lockout should be performed.\n");
	fprintf(stderr, "                        This will override the normal lockout method.\n");
	fprintf(stderr, " --do_unlock_cmd <cmd>  Command that is run when we should unlock.\n");
	fprintf(stderr, "                        This will override the normal unlock method.\n");
	fprintf(stderr, "\n");
	#endif // WITH_RFID
	fprintf(stderr, " --help                 Show this help.\n");
	fprintf(stderr, " --cmdhelp              Show extra command help.\n");
	fprintf(stderr, "\nThe snout image refers to the image of the cat snout that is matched against.\n");
	fprintf(stderr, "This image should be based on a 320x240 resolution image taken by the rpi camera.\n");
	fprintf(stderr, "If no path is specified \"snout.png\" in the current directory is used.\n");
	fprintf(stderr, "\n");
	#ifndef _WIN32
	fprintf(stderr, "Signals:\n");
	fprintf(stderr, "The program can receive signals that can be sent using the kill command.\n");
	fprintf(stderr, " SIGUSR1 = Force the cat door to unlock\n");
	fprintf(stderr, " SIGUSR2 = Force the cat door to lock (for lock timeout)\n");
	fprintf(stderr, "\n");
	#endif // _WIN32
}

int catcierge_parse_cmdargs(catcierge_args_t *args, int argc, char **argv)
{
	int ret = 0;
	int i;
	char *key = NULL;
	char **values = (char **)calloc(argc, sizeof(char *));
	size_t value_count = 0;

	if (!values)
	{
		fprintf(stderr, "Out of memory!\n");
		return -1;
	}

	for (i = 1; i < argc; i++)
	{
		// These are command line specific.
		if (!strcmp(argv[i], "--help"))
		{
			catcierge_show_usage(args, argv[0]);
			exit(1);
		}

		if (!strcmp(argv[i], "--cmdhelp"))
		{
			args->show_cmd_help = 1;
			catcierge_show_usage(args, argv[0]);
			exit(1);
		}

		if (!strcmp(argv[i], "--config"))
		{
			if (argc >= (i + 1))
			{
				i++;
				args->config_path = argv[i];
				continue;
			}
			else
			{
				fprintf(stderr, "No config path specified\n");
				return -1;
			}
		}

		// Normal options. These can be parsed from the
		// config file as well.
		if (!strncmp(argv[i], "--", 2))
		{
			int j = i + 1;
			key = &argv[i][2];
			memset(values, 0, value_count * sizeof(char *));
			value_count = 0;

			// Look for values for the option.
			// Continue fetching values until we get another option
			// or there are no more options.
			while ((j < argc) && strncmp(argv[j], "--", 2))
			{
				values[value_count] = argv[j];
				value_count++;
				i++;
				j = i + 1;
			}

			if ((ret = catcierge_parse_setting(args, key, values, value_count)) < 0)
			{
				fprintf(stderr, "Failed to parse command line arguments for \"%s\"\n", key);
				return ret;
			}
		}
	}

	free(values);

	return 0;
}

int catcierge_parse_config(catcierge_args_t *args, int argc, char **argv)
{
	int i;
	int ret;
	assert(args);

	// If the user has supplied us with a config use that.
	for (i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "--config"))
		{
			if (argc >= (i + 1))
			{
				i++;
				args->config_path = argv[i];
				continue;
			}
			else
			{
				fprintf(stderr, "No config path specified\n");
				return -1;
			}
		}
	}

	if (args->config_path)
	{
		if (alini_parser_create(&args->parser, args->config_path) < 0)
		{
			fprintf(stderr, "Failed to load config %s\n", args->config_path);
			return -1;
		}
	}
	else
	{
		// Use default.
		if (alini_parser_create(&args->parser, "catcierge.cfg") < 0)
		{
			alini_parser_create(&args->parser, "/etc/catcierge.cfg");
		}
	}

	return 0;
}

void catcierge_print_settings(catcierge_args_t *args)
{
	int i;

	printf("--------------------------------------------------------------------------------\n");
	printf("Settings:\n");
	printf("--------------------------------------------------------------------------------\n");
	printf(" Snout image(s): %s\n", args->snout_paths[0]);
	for (i = 1; i < args->snout_count; i++)
	{
		printf("                 %s\n", args->snout_paths[i]);
	}
	printf("Match threshold: %.2f\n", args->match_threshold);
	printf("  Match flipped: %d\n", args->match_flipped);
	printf("     Show video: %d\n", args->show);
	printf("   Save matches: %d\n", args->saveimg);
	printf("Highlight match: %d\n", args->highlight_match);
	printf("      Lock time: %d seconds\n", args->lockout_time);
	printf("  Match timeout: %d seconds\n", args->match_time);
	printf("       Log file: %s\n", args->log_path ? args->log_path : "-");
	#ifdef WITH_RFID
	printf("     Inner RFID: %s\n", args->rfid_inner_path ? args->rfid_inner_path : "-");
	printf("     Outer RFID: %s\n", args->rfid_outer_path ? args->rfid_outer_path : "-");
	printf("Lock on no RFID: %d\n", args->lock_on_invalid_rfid);
	printf(" RFID lock time: %.2f seconds\n", args->rfid_lock_time);
	printf("   Allowed RFID: %s\n", (args->rfid_allowed_count <= 0) ? "-" : args->rfid_allowed[0]);
	for (i = 1; i < args->rfid_allowed_count; i++)
	{
		printf("                 %s\n", args->rfid_allowed[i]);
	}
	#endif // WITH_RFID
	printf("--------------------------------------------------------------------------------\n");

}

int catcierge_init(catcierge_args_t *args)
{
	memset(args, 0, sizeof(catcierge_args_t));

	return 0;
}

void catcierge_destroy(catcierge_args_t *args)
{
	assert(args);

	catcierge_config_free_temp_strings(args);
	catcierge_free_rfid_allowed_list(args);
}
