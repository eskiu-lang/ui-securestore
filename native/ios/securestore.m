// <securestore> on iOS: generic passwords in the Keychain under the app's service, this
// device only and readable after the first unlock (a background refresh can read them).
#import <Foundation/Foundation.h>
#import <Security/Security.h>
#include <stdlib.h>
#include <string.h>

static NSMutableDictionary *query(const char *key) {
    return [@{(__bridge id)kSecClass: (__bridge id)kSecClassGenericPassword,
              (__bridge id)kSecAttrService: NSBundle.mainBundle.bundleIdentifier ?: @"eskiu.securestore",
              (__bridge id)kSecAttrAccount: @(key)} mutableCopy];
}

char *secure_native_get(const char *key) {
    NSMutableDictionary *q = query(key);
    q[(__bridge id)kSecReturnData] = @YES;
    q[(__bridge id)kSecMatchLimit] = (__bridge id)kSecMatchLimitOne;
    CFTypeRef found = NULL;
    if (SecItemCopyMatching((__bridge CFDictionaryRef)q, &found) != errSecSuccess || !found) return NULL;
    NSData *data = (__bridge_transfer NSData *)found;
    char *out = malloc(data.length + 1);
    memcpy(out, data.bytes, data.length);
    out[data.length] = 0;
    return out;
}

int secure_native_set(const char *key, const char *value) {
    NSData *data = [NSData dataWithBytes:value length:strlen(value)];
    NSMutableDictionary *q = query(key);
    NSDictionary *update = @{(__bridge id)kSecValueData: data,
                             (__bridge id)kSecAttrAccessible: (__bridge id)kSecAttrAccessibleAfterFirstUnlockThisDeviceOnly};
    OSStatus status = SecItemUpdate((__bridge CFDictionaryRef)q, (__bridge CFDictionaryRef)update);
    if (status == errSecItemNotFound) {
        [q addEntriesFromDictionary:update];
        status = SecItemAdd((__bridge CFDictionaryRef)q, NULL);
    }
    return status == errSecSuccess;
}

int secure_native_remove(const char *key) {
    OSStatus status = SecItemDelete((__bridge CFDictionaryRef)query(key));
    return status == errSecSuccess || status == errSecItemNotFound;
}

int secure_native_is_secure(void) { return 1; }
