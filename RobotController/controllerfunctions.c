#include "controllerfunctions.h"

#ifdef __linux__

int getIntFromConfig(GKeyFile* gkf, char *section, char *key, int def) {
    if (gkf == NULL) {
        return def;
    }
    GError *gerror = NULL;
    int temp = g_key_file_get_integer(gkf, section, key, &gerror);
    if (gerror != NULL) {
        fprintf(stderr, "%s, assuming joystick 0\n",(gerror->message));
        temp = def;
        g_error_free(gerror);
    }
    return temp;
}

#endif
