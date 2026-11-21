// <securestore> on the web, and in the desktop's webview builds: localStorage, in the clear.
// This is not secure storage, and secure_native_is_secure says so. The API is synchronous and
// WebCrypto is not (an encrypted copy could only be read after an await the caller cannot
// make), and a non-extractable key in IndexedDB would still decrypt for any script running in
// the page, which is what reads localStorage too.
mergeInto(LibraryManager.library, {
  secure_native_get__deps: ['$stringToNewUTF8'],
  secure_native_get: function(key) {
    let text = null;
    try { text = localStorage.getItem('eskiu-secure:' + UTF8ToString(key)); } catch (e) {}
    return text === null ? 0 : stringToNewUTF8(text);
  },
  secure_native_set: function(key, value) {
    try { localStorage.setItem('eskiu-secure:' + UTF8ToString(key), UTF8ToString(value)); return 1; } catch (e) { return 0; }
  },
  secure_native_remove: function(key) {
    try { localStorage.removeItem('eskiu-secure:' + UTF8ToString(key)); } catch (e) {}
    return 1;
  },
  secure_native_is_secure: function() { return 0; },
});
