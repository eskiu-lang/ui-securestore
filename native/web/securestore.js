// <securestore> on the web and the desktop: localStorage, in the clear (no secure store).
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
