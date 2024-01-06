cmake_minimum_required(VERSION 3.27)
project(fryzjerzy_kasjerzy C)

set(CMAKE_C_STANDARD 11)

add_executable(fryzjerzy_kasjerzy fryzjerzy_main.c
        fryzjerzy_fryzjer.c
        fryzjerzy_klient.c
        fryzjerzy_semaphore_ops.c
        fryzjerzy_semaphore_ops.h)
