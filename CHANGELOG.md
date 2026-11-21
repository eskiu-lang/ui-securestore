# Changelog

All notable changes to this package are documented in this file. The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [0.1.4] - 2026-11-21

### Added

- macOS: values in the Keychain under the app's bundle id, never synchronized (the data
  protection keychain for an app with a keychain access group, the login keychain otherwise).
- Linux: values in the Secret Service through libsecret, loaded at run time. Without libsecret
  or a Secret Service, set and get fail and `secure_is_secure()` is false.
- Windows: values encrypted with DPAPI for the current user, kept in the registry.
- `tools/native_smoke.c`, a check of a desktop's native side against the real store, and CI
  jobs that build the example and run it on Linux and Windows.

### Changed

- The docs say plainly that the web and the desktop's webview builds keep values in
  `localStorage` in the clear; `secure_is_secure()` stays false there.

### Fixed

- Native desktop builds of an app using the package link (before, the externs were missing on
  macOS, Linux and Windows).
- iOS and macOS: setting a key to an empty value kept the old value (an update to empty data
  leaves the Keychain item as it was); the item is replaced now.
