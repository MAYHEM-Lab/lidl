from .string import String


def copy_arg_to_build_call(builder, arg):
    if isinstance(arg, str):
        # copy as lidl.String
        return String.create(builder, arg)

    if isinstance(arg, bytes):
        raise NotImplementedError("Passing bytes not supported yet!")

    # Must be a regular value
    return arg


def copy_args_to_struct(params_struct):
    if hasattr(params_struct, "is_ptr"):
        params_struct = params_struct.pointee

    def func(builder=None, **kwargs):
        print(params_struct.__name__)
        for key in kwargs.keys():
            kwargs[key] = copy_arg_to_build_call(builder, kwargs[key])
        if hasattr(params_struct, "is_ref"):
            return builder.create(params_struct, **kwargs)
        return params_struct(**kwargs)

    return func


def make_call_union(params_union):
    def func(proc_name, builder=None, **kwargs):
        struct_type = params_union.members[proc_name]["type"]
        params_struct = copy_args_to_struct(struct_type)(builder, **kwargs)
        unargs = {
            proc_name: params_struct
        }

        if hasattr(params_union, "is_ref") or hasattr(params_union, "is_ptr"):
            return builder.create(params_union, **unargs)
        return params_union(**unargs)

    return func


def Service(description):
    return description
