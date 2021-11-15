from .string import String
from .builder import Builder
from .buffer import Memory
from .vector import Vector
from .basic_types import U8
from .pointer import Pointer


def copy_arg_to_build_call(builder: Builder, arg):
    print(arg.__class__)
    if isinstance(arg, str):
        # copy as lidl.String
        return String.create(builder, arg)

    if isinstance(arg, bytes):
        # copy as lidl.Vector(U8)
        res = Vector(U8).create(builder, arg)
        print("vector")
        return res

    if isinstance(arg, list):
        raise NotImplementedError("Passing lists not supported yet!")

    if hasattr(arg, "is_ref"):
        space = builder.allocate(len(arg._mem))
        space.raw_bytes()[:] = bytes(arg._mem.raw_bytes())
        return arg.__class__(mem=space, from_memory_=True)

    # Must be a regular value
    return arg

def copy_res_to_return(service, procedure, res):
    return getattr(res, procedure).ret0


def copy_args_to_struct(params_struct):
    if hasattr(params_struct, "is_ptr"):
        params_struct = params_struct.pointee

    def func(builder=None, **kwargs):
        for key in kwargs.keys():
            kwargs[key] = copy_arg_to_build_call(builder, kwargs[key])
            print(kwargs[key])
        if hasattr(params_struct, "is_ref"):
            return builder.create(params_struct, **kwargs)
        return params_struct(**kwargs)

    return func


def make_call_union(params_union):
    def func(proc_name, builder=None, **kwargs):
        struct_type = params_union.members[proc_name]["type"]

        if hasattr(struct_type, "is_ptr"):
            struct_type = struct_type.pointee

        params_struct = copy_args_to_struct(struct_type)(builder, **kwargs)
        unargs = {
            proc_name: params_struct
        }

        return builder.create(params_union, **unargs)

    return func


def generate_proc(cls, proc_name: str):
    params_union = make_call_union(cls.call_union)

    def fn(instance, **kwargs):
        builder = Builder(Memory(bytearray(1024)))
        call = params_union(proc_name, builder=builder, **kwargs)
        print(call)
        res = instance.send_receive(builder.get())
        res_size = cls.return_union.size
        base = res.get_end() - res_size
        res.set_base(base)
        ret = cls.return_union.from_memory(res)
        #print(ret)
        return copy_res_to_return(cls, proc_name, ret)

    fn.__name__ = proc_name

    return fn


def Service(description):
    class Serv(description):
        pass

    for name in description.procedures:
        setattr(Serv, name, generate_proc(Serv, name))

    Serv.__name__ = description.__name__

    return Serv
