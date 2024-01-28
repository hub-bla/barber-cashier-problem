cmake_minimum_required(VERSION 3.27)
project(fryzjerzy_kasjerzy C)

set(CMAKE_C_STANDARD 11)

add_executable(fryzjerzy_kasjerzy main.c
        barber.c
        client.c
        semaphore_ops.c
        semaphore_ops.h)
