package com.a3.park_ir.request

import com.a3.park_ir.DTO.DataClass
import retrofit2.Call
import retrofit2.http.Field
import retrofit2.http.FormUrlEncoded
import retrofit2.http.GET
import retrofit2.http.Header
import retrofit2.http.Headers
import retrofit2.http.POST
import retrofit2.http.Query

interface BaseApiServices {
    @POST("auth/login")
    @Headers("Content-Type: application/x-www-form-urlencoded")
    @FormUrlEncoded
    fun login(
        @Field("email") email: String?,
        @Field("password") password: String?
    ): Call<DataClass.LoginResponse>

    @POST("auth/register")
    fun register(
        @Query("name") name: String?,
        @Query("email") email: String?,
        @Query("password") password: String?
    ): Call<DataClass.LoginResponse>

    @GET("/plate/getID")
    fun getPlate(@Header("Authorization") token : String?): Call<DataClass.PlateResponse>
}