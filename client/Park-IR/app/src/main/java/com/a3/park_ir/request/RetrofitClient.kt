package com.a3.park_ir.request

import CustomSSLSocketFactory
import android.content.Context
import com.google.gson.GsonBuilder
import okhttp3.OkHttpClient
import okhttp3.Request
import retrofit2.Retrofit
import retrofit2.converter.gson.GsonConverterFactory


object RetrofitClient {
    private var retrofit: Retrofit? = null

    var gson = GsonBuilder()
        .setLenient()
        .create()
    fun getClient(baseUrl: String?, context: Context): Retrofit? {
        if (retrofit == null) {
            retrofit = Retrofit.Builder()
                .client(okHttpClient(context))
                .baseUrl(baseUrl)
                .addConverterFactory(GsonConverterFactory.create(gson))
                .build()
        }
        return retrofit
    }

    private fun okHttpClient(context: Context): OkHttpClient {
        return OkHttpClient.Builder()
            .addNetworkInterceptor { chain ->
                val originalRequest: Request = chain.request()
                val newRequest: Request =
                    originalRequest.newBuilder() //ganti value header di bawah ini dengan nama kalian
                        .addHeader("Client-Name", "PARK-IR-CLIENT")
                        .build()
                chain.proceed(newRequest)
            }
            .sslSocketFactory(CustomSSLSocketFactory().getSSLSocketFactory(context))
            .build()
    }
}