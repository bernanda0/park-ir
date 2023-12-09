import android.content.Context
import android.util.Log
import java.security.KeyStore
import javax.net.ssl.SSLContext
import javax.net.ssl.SSLSocketFactory
import javax.net.ssl.TrustManager
import javax.net.ssl.TrustManagerFactory
import javax.net.ssl.X509TrustManager

class CustomSSLSocketFactory {
    fun getSSLSocketFactory(context: Context): SSLSocketFactory? {
        return try {
            // Load the BKS keystore from resources
            val inputStream = context.resources.openRawResource(com.a3.park_ir.R.raw.mystore)
            val keyStore = KeyStore.getInstance("BKS")
            keyStore.load(inputStream, "some-password".toCharArray())

            // Create a custom TrustManager that trusts the CA in the BKS keystore
            val trustManagerFactory =
                TrustManagerFactory.getInstance(TrustManagerFactory.getDefaultAlgorithm())
            trustManagerFactory.init(keyStore)
            val trustManagers = trustManagerFactory.trustManagers
            val originalTrustManager = trustManagers[0] as X509TrustManager

            // Create a custom SSLSocketFactory
            val sslContext = SSLContext.getInstance("TLS")
            sslContext.init(null, arrayOf<TrustManager>(originalTrustManager), null)
            sslContext.socketFactory
        } catch (e: Exception) {
            Log.e("CustomSSLSocketFactory", "Error creating SSLSocketFactory", e)
            null
        }
    }
}