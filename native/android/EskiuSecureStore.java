// <securestore> on Android: each value encrypted with AES-GCM under a key the Android Keystore
// makes and keeps (it never leaves the Keystore), stored as base64 IV + ciphertext in the
// app's private preferences. securestore.c calls get, set and remove on any thread.
package org.eskiu.securestore;

import android.content.Context;
import android.content.SharedPreferences;
import android.security.keystore.KeyGenParameterSpec;
import android.security.keystore.KeyProperties;
import android.util.Base64;
import java.nio.charset.StandardCharsets;
import java.security.KeyStore;
import javax.crypto.Cipher;
import javax.crypto.KeyGenerator;
import javax.crypto.SecretKey;
import javax.crypto.spec.GCMParameterSpec;
import org.eskiu.ui.UiActivity;

public final class EskiuSecureStore {
    private static final String ALIAS = "eskiu.securestore";

    public static void setup() {}

    private static SharedPreferences store() {
        Context context = UiActivity.current();
        return context == null ? null : context.getSharedPreferences("eskiu.securestore", Context.MODE_PRIVATE);
    }

    private static synchronized SecretKey key() throws Exception {
        KeyStore keys = KeyStore.getInstance("AndroidKeyStore");
        keys.load(null);
        if (keys.containsAlias(ALIAS)) return ((KeyStore.SecretKeyEntry)keys.getEntry(ALIAS, null)).getSecretKey();
        KeyGenerator generator = KeyGenerator.getInstance(KeyProperties.KEY_ALGORITHM_AES, "AndroidKeyStore");
        generator.init(new KeyGenParameterSpec.Builder(ALIAS, KeyProperties.PURPOSE_ENCRYPT | KeyProperties.PURPOSE_DECRYPT)
            .setBlockModes(KeyProperties.BLOCK_MODE_GCM).setEncryptionPaddings(KeyProperties.ENCRYPTION_PADDING_NONE)
            .setKeySize(256).build());
        return generator.generateKey();
    }

    public static String get(String name) {
        SharedPreferences prefs = store();
        String sealed = prefs == null ? null : prefs.getString(name, null);
        if (sealed == null) return null;
        try {
            byte[] all = Base64.decode(sealed, Base64.NO_WRAP);
            Cipher cipher = Cipher.getInstance("AES/GCM/NoPadding");
            cipher.init(Cipher.DECRYPT_MODE, key(), new GCMParameterSpec(128, all, 0, 12));
            return new String(cipher.doFinal(all, 12, all.length - 12), StandardCharsets.UTF_8);
        } catch (Exception error) {
            // A value the key cannot open (the key was lost with a restore to a new device).
            return null;
        }
    }

    public static boolean set(String name, String value) {
        SharedPreferences prefs = store();
        if (prefs == null) return false;
        try {
            Cipher cipher = Cipher.getInstance("AES/GCM/NoPadding");
            cipher.init(Cipher.ENCRYPT_MODE, key());
            byte[] iv = cipher.getIV();
            byte[] body = cipher.doFinal(value.getBytes(StandardCharsets.UTF_8));
            byte[] all = new byte[iv.length + body.length];
            System.arraycopy(iv, 0, all, 0, iv.length);
            System.arraycopy(body, 0, all, iv.length, body.length);
            return prefs.edit().putString(name, Base64.encodeToString(all, Base64.NO_WRAP)).commit();
        } catch (Exception error) {
            return false;
        }
    }

    public static boolean remove(String name) {
        SharedPreferences prefs = store();
        return prefs != null && prefs.edit().remove(name).commit();
    }
}
