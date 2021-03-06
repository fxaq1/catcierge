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
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include "cargo_ini.h"

static void alini_cb(alini_parser_t *parser,
					 char *section, char *key, char *value)
{
	conf_arg_t *it = NULL;
	conf_ini_args_t *args = (conf_ini_args_t *)alini_parser_get_context(parser);

	// Either find an existing config argument with this key.
	HASH_FIND_STR(args->config_args, key, it);

	if (!it)
	{
		// Or add a new config argument.
		if (!(it = calloc(1, sizeof(conf_arg_t))))
		{
			fprintf(stderr, "Out of memory\n"); exit(-1);
		}

		it->linenumber = alini_parser_get_linenumber(parser);
		strncpy(it->key, key, MAX_CONFIG_KEY - 1);
		HASH_ADD_STR(args->config_args, key, it);
	}

	it->key_count++;
	
	if (!(it->vals[it->val_count++] = strdup(value)))
	{
		fprintf(stderr, "Out of memory\n"); exit(-1);
	}
}

void ini_args_destroy(conf_ini_args_t *args)
{
	size_t i;
	conf_arg_t *it = NULL;
	conf_arg_t *tmp = NULL;

	// free the hash table contents.
	HASH_ITER(hh, args->config_args, it, tmp)
	{
		HASH_DEL(args->config_args, it);
		for (i = 0; i < it->val_count; i++)
		{
			free(it->vals[i]);
			it->vals[i] = NULL;
		}

		free(it);
	}

	cargo_free_commandline(&args->config_argv, args->config_argc);
}

#if 0
void print_hash(conf_arg_t *config_args)
{
	size_t i = 0;
	conf_arg_t *it = NULL;
	conf_arg_t *tmp = NULL;

	// free the hash table contents.
	HASH_ITER(hh, config_args, it, tmp)
	{
		printf("(%lu) %s = ", it->val_count, it->key);

		for (i = 0; i < it->val_count; i++)
		{
			printf("%s", it->vals[i]);
			if (i != (it->val_count - 1))
			{
				printf(",");
			}
		}

		printf("\n");
	}
}

void print_commandline(conf_ini_args_t *args)
{
	int i;
	for (i = 0; i < args->config_argc; i++)
	{
		printf("%s ", args->config_argv[i]);
	}

	printf("\n");
}
#endif

cargo_type_t guess_expanded_name(cargo_t cargo, conf_arg_t *it,
								 char *tmpkey, size_t tmpkey_len)
{
	cargo_type_t type;
	int i = 1;

	// TODO: Maybe cargo should simply have a function that can find this for us...

	// Hack to figure out what prefix to use, "-", "--", and so on...
	// In the ini file, we are given the option name without a prefix "delta"
	// Take this keyname and try "-delta", "--delta"
	// and so on until a match is found in a hackish way...
	do
	{
		snprintf(tmpkey, tmpkey_len, "%*.*s%s", i, i, "------", it->key);
		type = cargo_get_option_type(cargo, tmpkey);
		i++;
	}
	while (((int)type == -1) && (i < CARGO_NAME_COUNT));

	return (i < CARGO_NAME_COUNT) ? type : -1;
}

int build_config_commandline(cargo_t cargo, const char *config_path, conf_ini_args_t *args)
{
	size_t j = 0;
	int i = 0;
	conf_arg_t *it = NULL;
	conf_arg_t *tmp = NULL;

	args->config_argc = 0;

	// The ini file has been parsed, and each key=value has been put
	// in the hash table. We know want to create a command line from
	// this, that is "argv" that then cargo can parse.

	// Count how many args there will be (argc).
	HASH_ITER(hh, args->config_args, it, tmp)
	{
		it->type = guess_expanded_name(cargo, it,
						it->expanded_key, sizeof(it->expanded_key) - 1);

		// Booleans can be repeated so we parse them in the form
		// key=integer
		if (it->type == CARGO_BOOL)
		{
			if (it->key_count != 1)
			{
				fprintf(stderr,
					"%s:%d Error:\n"
					"Multiple occurances of '%s' found. "
					"To repeat a flag please use '%s=%lu' instead.\n",
					config_path, it->linenumber,
					it->key, it->key, it->key_count);
				return -1;
			}

			if (strlen(it->vals[0]) == 0)
			{
				fprintf(stderr,
					"%s:%d Error:\n"
					"Please supply an integer value to the '%s' flag\n",
					config_path, it->linenumber, it->key);
				return -1;
			}
			else
			{
				char *end = NULL;
				errno = 0;
				it->args_count = strtol(it->vals[0], &end, 10);

				if (errno == ERANGE)
				{
					fprintf(stderr,
						"%s:%d Error:\n"
						"Integer out of range \"%s\"\n",
						config_path, it->linenumber, it->vals[0]);
					return -1;
				}

				if (end == it->vals[0])
				{
					fprintf(stderr,
						"%s:%d Error:\n"
						"Not a valid integer \"%s\"\n",
						config_path, it->linenumber, it->vals[0]);
				}
			}
		}
		else
		{
			it->args_count = 1 + it->val_count;
		}

		args->config_argc += it->args_count;
	}

	if (!(args->config_argv = calloc(args->config_argc, sizeof(char *))))
	{
		fprintf(stderr, "Out of memory!\n"); exit(-1);
	}

	// Now populate the argv.
	HASH_ITER(hh, args->config_args, it, tmp)
	{
		if (it->type == CARGO_BOOL)
		{
			for (j = 0; j < it->args_count; j++)
			{
				if (!(args->config_argv[i++] = strdup(it->expanded_key)))
				{
					fprintf(stderr, "Out of memory!\n"); exit(-1);
				}
			}
		}
		else
		{
			if (!(args->config_argv[i++] = strdup(it->expanded_key)))
			{
				fprintf(stderr, "Out of memory!\n"); exit(-1);
			}

			for (j = 0; j < it->val_count; j++)
			{
				if (!(args->config_argv[i++] = strdup(it->vals[j])))
				{
					fprintf(stderr, "Out of memory!\n"); exit(-1);
				}
			}
		}
	}

	return 0;
}

static int perform_config_parse(cargo_t cargo, const char *config_path,
								conf_ini_args_t *args)
{
	int ret = 0;
	int cfg_err = 0;

	cfg_err = alini_parser_create(&args->parser, config_path);

	if (cfg_err < 0)
	{
		cargo_print_usage(cargo, CARGO_USAGE_SHORT);
		fprintf(stderr, "\nFailed to create config parser for: %s\n", config_path);
		return -1;
	}
	else if (cfg_err > 0)
	{
		// Missing file, positive return value.
		ret = 1; goto fail;
	}

	alini_parser_setcallback_foundkvpair(args->parser, alini_cb);
	alini_parser_set_context(args->parser, args);

	if (alini_parser_start(args->parser))
	{
		fprintf(stderr, "\nFailed to parse %s\n", config_path);
		ret = -1; goto fail;
	}

fail:
	alini_parser_dispose(args->parser);
	args->parser = NULL;

	return ret;
}

int parse_config(cargo_t cargo, const char *config_path, conf_ini_args_t *args)
{
	int ret;
	cargo_parse_result_t err = 0;

	// No config file to parse.
	if (!config_path)
	{
		return 0;
	}

	// Parse the ini-file and store contents in a hash table.
	ret = perform_config_parse(cargo, config_path, args);
	
	if (ret < 0)
	{
		return -1;
	}
	else if (ret == 1)
	{
		// File does not exist.
		return 1;
	}

	// Build an "argv" string from the hashtable contents:
	//   key1 = 123
	//   key1 = 456
	//   key2 = 789
	// Becomes:
	//   --key1 123 456 --key2 789
	if (build_config_commandline(cargo, config_path, args))
	{
		return -1;
	}

	#if 0
	if (args->debug)
	{
		print_hash(args->ini_args.config_args);
		print_commandline(&args->ini_args);
	}
	#endif

	// Parse the "fake" command line using cargo. We turn off the
	// internal error output, so the errors are more in the context
	// of a config file.
	if ((err = cargo_parse(cargo, CARGO_NOERR_USAGE, 0, args->config_argc, args->config_argv)))
	{
		size_t k = 0;
		fprintf(stderr, "Failed to parse config: %d\n", err);
		
		switch (err)
		{
			case CARGO_PARSE_UNKNOWN_OPTS:
			{
				const char *opt = NULL;
				size_t unknown_count = 0;
				const char **unknowns = cargo_get_unknown(cargo, &unknown_count);

				fprintf(stderr, "Unknown config options:\n");

				for (k = 0; k < unknown_count; k++)
				{
					opt = unknowns[k] + strspn("--", unknowns[k]);
					fprintf(stderr, " %s\n", opt);
				}
				break;
			}
			default: break;
		}

		return -1;
	}

	return 0;
}


