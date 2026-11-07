<p align="center">
  <img src="https://raw.githubusercontent.com/eskiu-lang/eskiu/main/assets/logo.png" alt="Eskiu, a kitten with a ball of yarn" width="140">
</p>

<h2 align="center">securestore</h2>
<p align="center">Small secrets kept by the platform's secure storage, for Eskiu ui apps.</p>
<p align="center"><sub>Part of the <a href="https://github.com/eskiu-lang">Eskiu</a> project</sub></p>

---

```eskiu
import <securestore>;

secure_set("session", token);
string saved = secure_get("session");   // null when there is none; free it
defer free(saved);
secure_remove("session");
```

| Call | Does |
|---|---|
| `secure_get(key)` | the value kept under `key`, a copy the caller frees, or null |
| `secure_set(key, value)` | keeps `value` under `key`, replacing what was there; false when the platform refused |
| `secure_remove(key)` | forgets `key` |
| `secure_is_secure()` | whether values are kept encrypted by the platform (iOS, Android) |

| Platform | Keeps values in |
|---|---|
| iOS | the Keychain, as generic passwords under the app's bundle id, this device only and readable after the first unlock (so a background refresh can read them) |
| Android | the app's private preferences, each value encrypted with AES-GCM under a 256-bit key the Android Keystore makes and keeps; the key never leaves the Keystore, so a backup restored to another device cannot open them (`secure_get` is then null) |
| Web, desktop | `localStorage`, in the clear: there is no secure store (`secure_is_secure()` is false) |

It is for tokens and keys, not for data: a value is read whole on each call.

`examples/vault` keeps a session token across launches (`ui run ios` or `ui run android`).


## Documentation

Every public function and type, with its documentation, is in the [API reference](docs/api.md).

## Install

In an app made with `ui new`:

```bash
eskiuc add securestore --git https://github.com/eskiu-lang/ui-securestore --rev v0.1.0
```
## Tests

`eskiuc test` checks the calls against an in-memory stand-in for the platform.

## License

MIT
