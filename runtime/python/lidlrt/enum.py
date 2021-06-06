def make_enum(description):
    ActualEnum = type(description.__name__, (description.base_type,), {})

    for name, val in description.elems.items():
        setattr(ActualEnum, name, ActualEnum(val))

    return ActualEnum
