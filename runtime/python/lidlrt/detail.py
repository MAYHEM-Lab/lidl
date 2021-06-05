def self_or_val(e):
    if hasattr(e, "value"):
        return e.value
    return e
