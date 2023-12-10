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
import java.util.Objects

interface BaseApiServices {
    @POST("auth/login")
    @Headers("Content-Type: application/x-www-form-urlencoded")
    @FormUrlEncoded
    fun login(
        @Field("email") email: String?,
        @Field("password") password: String?
    ): Call<DataClass.LoginResponse>

    @POST("auth/signup")
    @Headers("Content-Type: application/x-www-form-urlencoded")
    @FormUrlEncoded
    fun register (
        @Field("username") name: String?,
        @Field("email") email: String?,
        @Field("password") password: String?
    ): Call<DataClass.LoginResponse>

    @GET("/plate/getID")
    fun getPlate(@Header("Authorization") token : String?): Call<DataClass.PlateResponse>

    @GET("/wallet/balance")
    fun getBalance(@Header("Authorization") token: String): Call<Int>

    @POST("/wallet/topUp")
    @Headers("Content-Type: application/x-www-form-urlencoded")
    @FormUrlEncoded
    fun topUp(@Header("Authorization") token: String, @Field("amount") amount: Int): Call<DataClass.Wallet>

    @POST("/plate/create")
    @Headers("Content-Type: application/x-www-form-urlencoded")
    @FormUrlEncoded
    fun createVID(@Header("Authorization") token: String, @Field("plate_number") plate_number: String): Call<Object>
}