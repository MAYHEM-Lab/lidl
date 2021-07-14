from .buffer import Memory


class Builder:
    _buf = Memory
    _avail = Memory

    def __init__(self, mem: Memory):
        self._buf = mem
        self._avail = self._buf

    def allocate(self, sz: int, align: int = 1) -> Memory:
        while self._avail.get_base() % align != 0:
            self._avail = self._avail.get_slice(0, length=1)

        alloc = self._avail.get_slice(0, length=sz)
        self._avail = self._avail.get_slice(sz)
        alloc.raw_bytes()[:] = b'\xcc' * sz
        return alloc

    def create_raw(self, type):
        return type.from_memory(mem=self.allocate(type.size, 1))

    def create(self, type, **kwargs):
        return type(mem=self.allocate(type.size, 1), **kwargs)

    def get(self):
        mem = self._buf.get_slice(0, self._avail.get_base())
        return mem.raw_bytes()
