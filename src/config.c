#include "tvi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*** configuration management ***/

// Configuration setting definitions
typedef struct {
    char *name;
    char *short_name;
    int *value_ptr;
    int default_value;
    int min_value;
    int max_value;
} ConfigSetting;

// Configuration table - central definition of all settings
static ConfigSetting config_table[] = {
    {"tabstop",         "ts", &E.tab_stop,         KILO_TAB_STOP, 1, 16},
    {"autoindent",      "ai", &E.autoindent,             1, 0, 1},
    {"ignorecase",      "ic", &E.ignore_case,            0, 0, 1},
    {"wrapsearch",      "ws", &E.wrap_search,            1, 0, 1},
    {"highlightsearch", "hl", &E.highlight_search,       1, 0, 1},
    {NULL, NULL, NULL, 0, 0, 0}  // Terminator
};

// Find a configuration setting by name or short name
static ConfigSetting* findConfigSetting(char *name) {
    ConfigSetting *setting = config_table;
    while (setting->name) {
        int ofs = (setting->max_value==1 && strncmp(name,"no",2)==0) ? 2 : 0;
        if (strcmp(setting->name, &name[ofs]) == 0 
                  || strcmp(setting->short_name, &name[ofs]) == 0) return setting;
        setting++;
    }
    return NULL;
}

int editorSetSetting(char *key, char *value) {
    if (!key) return 0;
   
    ConfigSetting *setting = findConfigSetting(key);
    if (!setting) {
        editorSetStatusMessage("Unknown setting: %s", key);
        return 0;
    }
    int int_value = strncmp(key,"no",2)==0 ? 0 : 1;
    if (setting->max_value == 1) {
       *(setting->value_ptr) = int_value;
    } else if (value) {
       int_value = atoi(value);
       // Validate range
       if (int_value < setting->min_value || int_value > setting->max_value) {
           editorSetStatusMessage("Invalid value (must be %d-%d)", setting->min_value, setting->max_value);
           return 0;
       }
       // Special handling for tabstop
       if (strcmp(key, "tabstop") == 0 || strcmp(key, "ts") == 0) {
           int old_value = *(setting->value_ptr);
           if (old_value != int_value && E.numrows > 0) {
               for (int i = 0; i < E.numrows; i++) editorUpdateRow(&E.row[i]);
           }
       }
       *(setting->value_ptr) = int_value;
    }
#if DEBUG
    fprintf(stderr,"- %d key=%s\n",int_value,key);
#endif
    return 1;
}

int editorConfigLine(void *p, int swv, char *key, char *value) {
    int rc=editorSetSetting(key, value);
#if DEBUG
    fprintf(stderr,"- %d %s = %s\n",rc,key,value);
#endif
    // get rid if the unused parameter warning when compiling
    (void)p;
    (void)swv;
    if (rc == 0) {
        editorSetStatusMessage("");
        return 0;
    }
    return 1;
}

void editorConfigLoad() {
    char fn[255];
    char *home = getenv("HOME");
    fn[0]='\0';
    if (home) {
        snprintf(fn, 255, "%s/.config/tvi/tvi.conf", home);
        if (access(fn,R_OK)!=0) fn[0]='\0';
    }
    if (fn[0] == '\0')  strcpy(fn,"/usr/local/share/tvi/tvi.conf");
#if DEBUG
    fprintf(stderr,"# config %s\n",fn);
#endif
    int rc=editorReadFile(fn,'=',editorConfigLine,NULL);
    if (rc == 0) return;
}

void editorInitConfig() {
    // Set all settings to their defaults
    ConfigSetting *setting = config_table;
    while (setting->name) {
        *(setting->value_ptr) = setting->default_value;
        setting++;
    }
    editorConfigLoad();
}

void editorShowAllSettings() {
  char status[512] = "Settings:";
  char temp[32];
  ConfigSetting *setting = config_table;

  while (setting->name) {
    int current_value = *(setting->value_ptr);
    if (setting->max_value == 1) {
        snprintf(temp, sizeof(temp), " %s%s(%s)", (current_value) ? "" : "no", setting->name,setting->short_name);
    } else {
        snprintf(temp, sizeof(temp), " %s(%s)=%d", setting->name, setting->short_name,current_value);
    }
    strcat(status, temp);
    setting++;
  }
  editorSetStatusMessage(":%s", status);
}

void editorProcessSetCommand(char *cmd) {
  if (!cmd || !*cmd) {
    editorShowAllSettings();
    return;
  }
  // Skip leading whitespace
  while (*cmd == ' ' || *cmd == '\t') cmd++;

  // Show all settings
  if (*cmd=='\0' || strcmp(cmd, "all") == 0) {
    editorShowAllSettings();
    return;
  }

  // Find equals sign
  char *value=NULL;
  char *sep = strchr(cmd, '=');
  if (sep) {
    *sep++ = '\0';
    value = sep;
  }
  editorSetSetting(cmd, value);
}
