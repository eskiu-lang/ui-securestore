// <securestore> on macOS: generic passwords in the Keychain under the app's bundle id, never
// synchronized to other devices. A signed app with a keychain access group gets the data
// protection keychain (this device only, readable after the first unlock, as on iOS); an app
// without that entitlement (the ui tool signs ad hoc) gets the user's login keychain, which
// macOS keeps encrypted and opens only to the app that made the item.
#import <Foundation/Foundation.h>
#import <Security/Security.h>
#include <stdlib.h>
#include <string.h>

// Whether the app uses the login keychain: it has neither a keychain access group nor an
// application identifier, the entitlements the data protection keychain asks for. A search
// there without them finds nothing rather than failing, so it is decided up front (and
// again when an add is refused for the entitlement).
static BOOL legacy = NO;

static BOOL entitled(CFStringRef name) {
    SecTaskRef task = SecTaskCreateFromSelf(NULL);
    if (!task) return NO;
    CFTypeRef value = SecTaskCopyValueForEntitlement(task, name, NULL);
    CFRelease(task);
    if (!value) return NO;
    CFRelease(value);
    return YES;
}

__attribute__((constructor)) static void securestore_mode(void) {
    legacy = !entitled(CFSTR("keychain-access-groups")) && !entitled(CFSTR("com.apple.application-identifier"));
}

static NSMutableDictionary *query(const char *key) {
    NSMutableDictionary *q = [@{(__bridge id)kSecClass: (__bridge id)kSecClassGenericPassword,
                                (__bridge id)kSecAttrService: NSBundle.mainBundle.bundleIdentifier ?: @"eskiu.securestore",
                                (__bridge id)kSecAttrAccount: @(key),
                                (__bridge id)kSecAttrSynchronizable: @NO} mutableCopy];
    if (!legacy) q[(__bridge id)kSecUseDataProtectionKeychain] = @YES;
    return q;
}

// Runs `call` on the data protection keychain, and again on the login keychain when the app
// lacks the entitlement for the first.
static OSStatus keychain(OSStatus (^call)(NSMutableDictionary *q), const char *key) {
    OSStatus status = call(query(key));
    if (status == errSecMissingEntitlement && !legacy) {
        legacy = YES;
        status = call(query(key));
    }
    return status;
}

char *secure_native_get(const char *key) {
    __block CFTypeRef found = NULL;
    OSStatus status = keychain(^OSStatus(NSMutableDictionary *q) {
        q[(__bridge id)kSecReturnData] = @YES;
        q[(__bridge id)kSecMatchLimit] = (__bridge id)kSecMatchLimitOne;
        return SecItemCopyMatching((__bridge CFDictionaryRef)q, &found);
    }, key);
    if (status != errSecSuccess || !found) return NULL;
    NSData *data = (__bridge_transfer NSData *)found;
    char *out = malloc(data.length + 1);
    if (!out) return NULL;
    memcpy(out, data.bytes, data.length);
    out[data.length] = 0;
    return out;
}

int secure_native_set(const char *key, const char *value) {
    NSData *data = [NSData dataWithBytes:value length:strlen(value)];
    OSStatus status = keychain(^OSStatus(NSMutableDictionary *q) {
        NSMutableDictionary *update = [@{(__bridge id)kSecValueData: data} mutableCopy];
        if (!legacy) update[(__bridge id)kSecAttrAccessible] = (__bridge id)kSecAttrAccessibleAfterFirstUnlockThisDeviceOnly;
        // An update to empty data leaves the old value, so an empty value replaces the item.
        OSStatus s = data.length ? SecItemUpdate((__bridge CFDictionaryRef)q, (__bridge CFDictionaryRef)update)
                                 : SecItemDelete((__bridge CFDictionaryRef)q);
        if (s == errSecItemNotFound || (s == errSecSuccess && !data.length)) {
            [q addEntriesFromDictionary:update];
            s = SecItemAdd((__bridge CFDictionaryRef)q, NULL);
        }
        return s;
    }, key);
    return status == errSecSuccess;
}

int secure_native_remove(const char *key) {
    OSStatus status = keychain(^OSStatus(NSMutableDictionary *q) {
        return SecItemDelete((__bridge CFDictionaryRef)q);
    }, key);
    return status == errSecSuccess || status == errSecItemNotFound;
}

int secure_native_is_secure(void) { return 1; }
