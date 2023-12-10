package com.a3.park_ir.request

import android.content.Context

class UtilsApi {
    val BASE_URL_API = "https://172.173.157.174:4343/"

    fun getApiService(context : Context): BaseApiServices {
        return RetrofitClient.getClient(BASE_URL_API, context)!!.create(BaseApiServices::class.java)
    }
}