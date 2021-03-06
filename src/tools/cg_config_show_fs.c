/*
 * This file is part of Nuage Labs SAS's Cloud Gateway.
 *
 * Copyright (C) 2011-2017  Nuage Labs SAS
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * In addition, for the avoidance of any doubt, permission is granted to
 * link this program with OpenSSL and to (re)distribute the binaries
 * produced as the result of such linking.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

/*
   -i --id Filesystem ID (required)

   -f --file Configuration File to update
*/

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include <cloudutils/cloudutils.h>
#include <cloudutils/cloudutils_xml.h>
#include <cloudutils/cloudutils_xml_reader.h>

static void print_usage(void)
{
    fprintf(stderr, "Usage: CloudGatewayShowFilesystem [OPTIONS]\n");
    fprintf(stderr, "Required options are:\n");
#define ITEM(longname, variable, shortname, xmlname, required, expl)     \
    if (required == true)                                       \
    {                                                           \
        fprintf(stderr, "\t-%c --%-35s %s\n", shortname, longname, expl);  \
    }
#include "cg_config_show_fs.itm"
#undef ITEM
    fprintf(stderr, "Please look at the product documentation for more information.\n");
}

static void print_filesystem(cgutils_xml_reader * const fs)
{
    static struct
    {
        char const * const print_name;
        char const * const xpath;
    }
    const values[] =
        {
            { "Id", "Id"},
            { "Type", "Type"},
            { "Cache Root", "CacheRoot"},
            { "Full Threshold", "FullThreshold"},
        };
    static size_t const values_count = sizeof values / sizeof *values;
    char * value = NULL;
    cgutils_llist * instances_list = NULL;
    int res = 0;

    assert(fs != NULL);

    for (size_t idx = 0;
         idx < values_count;
         idx++)
    {
        res = cgutils_xml_reader_get_string(fs,
                                            values[idx].xpath,
                                            &value);

        if (res == 0)
        {
            fprintf(stdout, "%s: %s\n",
                    values[idx].print_name,
                    value);
            CGUTILS_FREE(value);
        }
        else if (res != ENOENT)
        {
            fprintf(stderr, "Error getting value for %s: %s\n",
                    values[idx].print_name,
                    strerror(res));
        }
    }

    res = cgutils_xml_reader_get_all(fs,
                                     "Instances/Instance",
                                     &instances_list);

    if (res == 0)
    {
        fprintf(stdout, "\nInstances:\n");

        for (cgutils_llist_elt * elt = cgutils_llist_get_first(instances_list);
             elt != NULL;
             elt = cgutils_llist_elt_get_next(elt))
        {
            cgutils_xml_reader * inst = cgutils_llist_elt_get_object(elt);
            char * name = NULL;

            res = cgutils_xml_reader_get_string(inst,
                                                ".",
                                                &name);

            if (res == 0)
            {
                fprintf(stdout, "%s\n",
                        name);
                CGUTILS_FREE(name);
            }
        }

        cgutils_llist_free(&instances_list, &cgutils_xml_reader_delete);
    }
}

int main (int argc, char **argv)
{

#define ITEM(longname, variable, shortname, xmlname, required, expl) char * variable = NULL;
#include "cg_config_show_fs.itm"
#undef ITEM

    static struct option const long_options[] =
        {
#define ITEM(longname, variable, shortname, xmlname, required, expl) { longname, required_argument, NULL, shortname },
#include "cg_config_show_fs.itm"
#undef ITEM
            { 0, 0, 0, 0 }
        };

    int result = 0;
    int indexptr = 0;

    opterr = 1;

    while ((result = getopt_long(argc,
                                 argv,
                                 "+i:f:",
                                 long_options,
                                 &indexptr)) != -1)
    {
        switch(result)
        {
#define ITEM(longname, variable, shortname, xmlname, required, expl)  \
            case shortname:                                           \
                variable = optarg;                                    \
                break;
#include "cg_config_show_fs.itm"
#undef ITEM
        default:
            print_usage();
        }

    }

    if (optind == argc)
    {
        if (fs_id != NULL &&
            file != NULL)
        {
            cgutils_xml_reader * reader = NULL;

            cgutils_xml_init();

            result = cgutils_xml_reader_from_file(file,
                                                  &reader);

            if (result == 0)
            {
                char * xpath = NULL;

                result = cgutils_asprintf(&xpath,
                                          "/Configuration/FileSystems/FileSystem[Id='%s']",
                                          fs_id);

                if (result == 0)
                {
                    cgutils_xml_reader * fs = NULL;

                    result = cgutils_xml_reader_from_path(reader,
                                                          xpath,
                                                          &fs);

                    if (result == 0)
                    {
                        print_filesystem(fs);

                        cgutils_xml_reader_free(fs), fs = NULL;
                    }
                    else if (result == ENOENT)
                    {
                        fprintf(stderr, "There is no %s filesystem.\n",
                                fs_id);
                    }
                    else
                    {
                        fprintf(stderr, "Error looking for a filesystem whose Id is %s: %s\n",
                                fs_id,
                                strerror(result));
                    }

                    CGUTILS_FREE(xpath);
                }
                else
                {
                    fprintf(stderr, "Error allocating memory for XPath expression: %s\n",
                            strerror(result));
                }

                cgutils_xml_reader_free(reader), reader = NULL;
            }
            else
            {
                fprintf(stderr, "Error opening file %s: %s\n",
                        file,
                        strerror(result));
            }

            cgutils_xml_destroy();
        }
        else
        {
            result = EINVAL;
            fprintf(stderr, "Id and file parameters are mandatory.\n");
            print_usage();
        }
    }
    else
    {
        result = EINVAL;
        fprintf(stderr, "Too many arguments\n");
    }

    fclose(stdin);
    fclose(stdout);
    fclose(stderr);

    return result;
}
