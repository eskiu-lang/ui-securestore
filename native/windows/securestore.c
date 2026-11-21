// <securestore> on Windows: each value encrypted with DPAPI (CryptProtectData) for the user
// who runs the app, and the ciphertext kept in the registry under
// HKEY_CURRENT_USER\Software\Eskiu\SecureStore\<program name>, one binary value per key. Only
// that Windows account can decrypt it; the key's name is mixed in as entropy, so a blob
// copied under another key does not open. crypt32.dll is loaded at run time; without it set
// and get fail and nothing is written.
#include <windows.h>
#include <wincrypt.h>
#include <stdlib.h>
#include <string.h>

typedef BOOL(WINAPI *ProtectFn)(DATA_BLOB *, LPCWSTR, DATA_BLOB *, PVOID, PVOID, DWORD, DATA_BLOB *);
typedef BOOL(WINAPI *UnprotectFn)(DATA_BLOB *, LPWSTR *, DATA_BLOB *, PVOID, PVOID, DWORD, DATA_BLOB *);

static ProtectFn protect;
static UnprotectFn unprotect;

static int load(void) {
    static int loaded = 0;
    if (loaded) return loaded > 0;
    loaded = -1;
    HMODULE lib = LoadLibraryW(L"crypt32.dll");
    if (!lib) return 0;
    protect = (ProtectFn)(void *)GetProcAddress(lib, "CryptProtectData");
    unprotect = (UnprotectFn)(void *)GetProcAddress(lib, "CryptUnprotectData");
    if (!protect || !unprotect) return 0;
    loaded = 1;
    return 1;
}

// UTF-8 to a UTF-16 copy the caller frees, or NULL.
static wchar_t *wide(const char *text) {
    int n = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    if (n <= 0) return NULL;
    wchar_t *out = (wchar_t *)malloc(sizeof(wchar_t) * (size_t)n);
    if (out) MultiByteToWideChar(CP_UTF8, 0, text, -1, out, n);
    return out;
}

// The registry key for this program's values (created when `create`), or NULL.
static HKEY store(int create) {
    wchar_t path[MAX_PATH], subkey[MAX_PATH + 64];
    DWORD length = GetModuleFileNameW(NULL, path, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) return NULL;
    wchar_t *name = path;
    for (wchar_t *p = path; *p; p++) {
        if (*p == L'\\' || *p == L'/') name = p + 1;
    }
    wchar_t *dot = wcsrchr(name, L'.');
    if (dot) *dot = 0;
    wcscpy(subkey, L"Software\\Eskiu\\SecureStore\\");
    wcscat(subkey, name);
    HKEY key = NULL;
    LONG status = create ? RegCreateKeyExW(HKEY_CURRENT_USER, subkey, 0, NULL, 0, KEY_READ | KEY_WRITE, NULL, &key, NULL)
                         : RegOpenKeyExW(HKEY_CURRENT_USER, subkey, 0, KEY_READ | KEY_WRITE, &key);
    return status == ERROR_SUCCESS ? key : NULL;
}

static DATA_BLOB entropy(const char *key) {
    DATA_BLOB blob;
    blob.pbData = (BYTE *)key;
    blob.cbData = (DWORD)strlen(key);
    return blob;
}

char *secure_native_get(const char *key) {
    if (!load()) return NULL;
    HKEY reg = store(0);
    wchar_t *name = wide(key);
    char *out = NULL;
    BYTE *cipher = NULL;
    DWORD type = 0, size = 0;
    if (reg && name && RegQueryValueExW(reg, name, NULL, &type, NULL, &size) == ERROR_SUCCESS && type == REG_BINARY &&
        (cipher = (BYTE *)malloc(size ? size : 1)) &&
        RegQueryValueExW(reg, name, NULL, &type, cipher, &size) == ERROR_SUCCESS) {
        DATA_BLOB in = {size, cipher}, extra = entropy(key), plain = {0, NULL};
        if (unprotect(&in, NULL, &extra, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &plain)) {
            out = (char *)malloc(plain.cbData + 1);
            if (out) {
                memcpy(out, plain.pbData, plain.cbData);
                out[plain.cbData] = 0;
            }
            SecureZeroMemory(plain.pbData, plain.cbData);
            LocalFree(plain.pbData);
        }
    }
    free(cipher);
    free(name);
    if (reg) RegCloseKey(reg);
    return out;
}

int secure_native_set(const char *key, const char *value) {
    if (!load()) return 0;
    DATA_BLOB in = {(DWORD)strlen(value), (BYTE *)value}, extra = entropy(key), cipher = {0, NULL};
    if (!protect(&in, L"eskiu securestore", &extra, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &cipher)) return 0;
    HKEY reg = store(1);
    wchar_t *name = wide(key);
    int ok = reg && name && RegSetValueExW(reg, name, 0, REG_BINARY, cipher.pbData, cipher.cbData) == ERROR_SUCCESS;
    LocalFree(cipher.pbData);
    free(name);
    if (reg) RegCloseKey(reg);
    return ok;
}

int secure_native_remove(const char *key) {
    HKEY reg = store(0);
    if (!reg) return 1;
    wchar_t *name = wide(key);
    LONG status = name ? RegDeleteValueW(reg, name) : ERROR_OUTOFMEMORY;
    free(name);
    RegCloseKey(reg);
    return status == ERROR_SUCCESS || status == ERROR_FILE_NOT_FOUND;
}

int secure_native_is_secure(void) { return load(); }
