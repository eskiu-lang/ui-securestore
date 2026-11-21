// <securestore> on Linux: passwords in the Secret Service (GNOME Keyring, KWallet), through
// libsecret, loaded at run time so an app builds and starts without it. Each value is an item
// of the default collection with the attributes app (the app's id) and key. When libsecret or
// a Secret Service is missing, set and get fail and secure_native_is_secure is 0: nothing is
// ever written in the clear.
#include <dlfcn.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>

// libsecret's SecretSchema and its calls, as libsecret-1's headers declare them.
typedef struct {
    const gchar *name;
    int type;
} SchemaAttribute;

typedef struct {
    const gchar *name;
    int flags;
    SchemaAttribute attributes[32];
    gint reserved;
    gpointer reserved1, reserved2, reserved3, reserved4, reserved5, reserved6, reserved7;
} Schema;

typedef gboolean (*StoreSync)(const Schema *, const gchar *, const gchar *, const gchar *, void *, GError **, ...);
typedef gchar *(*LookupSync)(const Schema *, void *, GError **, ...);
typedef gboolean (*ClearSync)(const Schema *, void *, GError **, ...);
typedef void (*PasswordFree)(gchar *);

static const Schema schema = {
    "org.eskiu.securestore", 0, {{"app", 0}, {"key", 0}, {NULL, 0}}, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
};

static struct {
    int loaded;
    StoreSync store;
    LookupSync lookup;
    ClearSync clear;
    PasswordFree free_password;
} secret;

// libsecret's calls, or 0 when the library is not installed.
static int load(void) {
    if (secret.loaded) return secret.loaded > 0;
    secret.loaded = -1;
    void *lib = dlopen("libsecret-1.so.0", RTLD_NOW | RTLD_LOCAL);
    if (!lib) return 0;
    secret.store = (StoreSync)dlsym(lib, "secret_password_store_sync");
    secret.lookup = (LookupSync)dlsym(lib, "secret_password_lookup_sync");
    secret.clear = (ClearSync)dlsym(lib, "secret_password_clear_sync");
    secret.free_password = (PasswordFree)dlsym(lib, "secret_password_free");
    if (!secret.store || !secret.lookup || !secret.clear || !secret.free_password) return 0;
    secret.loaded = 1;
    return 1;
}

static const char *app(void) {
    const char *name = g_get_prgname();
    return name && name[0] ? name : "org.eskiu.app";
}

static void drop(GError *error) {
    if (error) g_error_free(error);
}

char *secure_native_get(const char *key) {
    if (!load()) return NULL;
    GError *error = NULL;
    gchar *found = secret.lookup(&schema, NULL, &error, "app", app(), "key", key, NULL);
    drop(error);
    if (!found) return NULL;
    size_t length = strlen(found);
    char *out = malloc(length + 1);
    if (out) memcpy(out, found, length + 1);
    secret.free_password(found);
    return out;
}

int secure_native_set(const char *key, const char *value) {
    if (!load()) return 0;
    gchar *label = g_strdup_printf("%s: %s", app(), key);
    GError *error = NULL;
    gboolean ok = secret.store(&schema, NULL, label, value, NULL, &error, "app", app(), "key", key, NULL);
    g_free(label);
    drop(error);
    return ok ? 1 : 0;
}

int secure_native_remove(const char *key) {
    if (!load()) return 0;
    GError *error = NULL;
    secret.clear(&schema, NULL, &error, "app", app(), "key", key, NULL);
    int ok = error == NULL;
    drop(error);
    return ok;
}

// Whether a Secret Service answers: libsecret is there and a lookup reaches the service
// without an error (none was found is fine). Asked once.
int secure_native_is_secure(void) {
    static int answer = -1;
    if (answer >= 0) return answer;
    answer = 0;
    if (!load()) return 0;
    GError *error = NULL;
    gchar *found = secret.lookup(&schema, NULL, &error, "app", app(), "key", "", NULL);
    if (found) secret.free_password(found);
    answer = error == NULL;
    drop(error);
    return answer;
}
