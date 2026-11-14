#include "src/file_parser.h"

Config config;

if (!read_config(config_filename, &config)) {
    return 1;
};

//in progress