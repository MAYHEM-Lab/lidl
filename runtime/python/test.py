import enum

import lidlrt
from lidlrt.service import copy_args_to_struct

mem = lidlrt.Memory(bytearray(128))

build = lidlrt.Builder(mem)

i32 = lidlrt.I32(93)

print(i32)
print(i32.value)


@lidlrt.Union
class IntOrFloat:
    members = {
        "alternative": {
            "type": lidlrt.I8,
            "offset": 0
        },
        "i": {
            "type": lidlrt.I32,
            "offset": 4
        },
        "f": {
            "type": lidlrt.F32,
            "offset": 4
        }
    }
    size = 8


@lidlrt.Struct
class Vec3f:
    members = {
        "x": {
            "type": lidlrt.F32,
            "offset": 0
        },
        "y": {
            "type": lidlrt.F32,
            "offset": 4
        },
        "z": {
            "type": lidlrt.F32,
            "offset": 8
        }
    }
    size = 12


@lidlrt.Struct
class Foo:
    members = {
        "x": {
            "type": lidlrt.I32,
            "offset": 0
        },
        "y": {
            "type": lidlrt.U64,
            "offset": 8
        },
        "vec": {
            "type": Vec3f,
            "offset": 16
        }
    }
    size = 32


@lidlrt.Struct
class HavePtrToInt:
    members = {
        "ptr_to_int": {
            "type": lidlrt.Pointer(lidlrt.I32),
            "offset": 0
        }
    }
    size = 2
    is_ref = True

@lidlrt.Struct
class HavePtrToStr:
    members = {
        "ptr_to_str": {
            "type": lidlrt.Pointer(lidlrt.String),
            "offset": 0
        }
    }
    size = 2
    is_ref = True

@lidlrt.Struct
class arr:
    members = {
        "arr": {
            "type": lidlrt.Array(lidlrt.I32, 4),
            "offset": 0
        }
    }

    size = 16


@lidlrt.Struct
class none_t:
    members = {

    }

    size = 1


@lidlrt.Enum
class some_enum:
    base_type = lidlrt.I8
    elems = {
        "a": 0,
        "b": 1,
        "c": 2
    }


@lidlrt.Enum
class SomeEnum:
    base_type = lidlrt.I32

    elems = {
        "x": 1,
        "y": 2,
        "z": 3
    }

@lidlrt.Union
class maybe_int:
    @lidlrt.Enum
    class alternatives:
        base_type = lidlrt.I8
        elems = {
            "val": 0,
            "none": 1
        }

    members = {
        "alternative": {
            "type": alternatives,
            "offset": 0
        },
        "val": {
            "type": lidlrt.I32,
            "offset": 4
        },
        "none": {
            "type": lidlrt.I32,
            "offset": 4
        }
    }

    size = 8


strcopier = copy_args_to_struct(HavePtrToStr)
print(strcopier(builder=build, ptr_to_str="hello world!"))

copier = copy_args_to_struct(Vec3f)

print(copier(x=32, y=64, z=128, builder=build))

print(maybe_int.alternatives.val)

print(SomeEnum.x)

build.create(none_t)

my_str = lidlrt.String.create(build, "Hello")
str_ptr = lidlrt.Pointer(lidlrt.String).create_with_object(build, my_str)
print(str_ptr)

my_int = build.create_raw(lidlrt.I32)
my_int.value = 12412
x = build.create(HavePtrToInt, ptr_to_int=my_int)
print(x)

y = build.create(HavePtrToStr, ptr_to_str=lidlrt.String.create(build, "hello world!"))
print(y)

print(build.create(Foo, x=32, y=64, vec=Vec3f(x=12, y=24, z=36)))

print(lidlrt.Array(lidlrt.I32, 4)([1, 2, 3, 4]))
a = build.create_raw(arr)
a.arr.assign([1, 2, 3, 4])
a.arr = [3, 4, 5, 6]
a.arr[0] = 42
print(a)

print(Vec3f(x=10, y=20, z=30))

ut = build.create_raw(Foo)

ut.vec.x = 100.234124135
print(ut.vec.x)

ut.x = 33
print(ut.x)

ut.y = 1013521341356
print(ut.y)
print(ut)
