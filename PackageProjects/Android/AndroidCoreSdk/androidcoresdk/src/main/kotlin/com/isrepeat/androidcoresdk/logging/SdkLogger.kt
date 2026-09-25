package com.isrepeat.androidcoresdk.logging

//
// Минимальный мост SDK к журналу конкретного приложения.
//
fun interface SdkLogger {
    fun log(message: String)
}