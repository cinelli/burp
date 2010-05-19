/*
 *  conf.c
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "conf.h"
#include "curl.h"
#include "util.h"

struct config_t *config = NULL;

void config_free(struct config_t *config) {
  if (config->user)
    free(config->user);
  if (config->password)
    free(config->password);
  if (config->cookies)
    free(config->cookies);
  if (config->category && STRNEQ(config->category, "None"))
    free(config->category);

  free(config);
}

struct config_t *config_new(void) {
  struct config_t *config = xcalloc(1, sizeof *config);

  config->user = config->password = config->cookies = config->category = NULL;
  config->persist = FALSE;
  config->verbose = 0;
  config->catnum = 1;

  return config;
}

int read_config_file() {
  int ret;
  char *ptr;
  char config_path[PATH_MAX + 1], line[BUFSIZ + 1];

  snprintf(&config_path[0], PATH_MAX, "%s/%s", 
    getenv("XDG_CONFIG_HOME"), "burp/burp.conf");

  if (! file_exists(config_path)) {
    if (config->verbose > 1)
      printf("::DEBUG:: No config file found\n");
    return 0;
  }

  if (config->verbose > 1)
    printf("::DEBUG:: Found config file\n");

  ret = 0;
  FILE *conf_fd = fopen(config_path, "r");
  while (fgets(line, BUFSIZ, conf_fd)) {
    strtrim(line);

    if (line[0] == '#' || strlen(line) == 0)
      continue;

    if ((ptr = strchr(line, '#'))) {
      *ptr = '\0';
    }

    char *key;
    key = ptr = line;
    strsep(&ptr, "=");
    strtrim(key);
    strtrim(ptr);

    if (STREQ(key, "User")) {
      if (config->user == NULL) {
        config->user = strndup(ptr, AUR_USER_MAX);
        if (config->verbose > 1)
          printf("::DEBUG:: Using username: %s\n", config->user);
      }
    } else if (STREQ(key, "Password")) {
      if (config->password == NULL) {
        config->password = strndup(ptr, AUR_PASSWORD_MAX);
        if (config->verbose > 1)
          printf("::DEBUG:: Using password from config file.\n");
      }
    } else if (STREQ(key, "Cookies")) {
      if (config->cookies == NULL) {
        config->cookies = expand_tilde(ptr);

        if (config->verbose > 1)
          printf("::DEBUG:: Using cookie file: %s\n", config->cookies);
      }
    } else if (STREQ(key, "Persist")) {
      config->persist = TRUE;
    } else {
      fprintf(stderr, "Error parsing config file: bad option '%s'\n", key);
      ret = 1;
      break;
    }
  }

  fclose(conf_fd);

  return ret;
}
