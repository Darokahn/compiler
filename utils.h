#pragma once
#define stack(type) (&(type){0})

#define stackval(type, ...) (&(type){__VA_ARGS__})
