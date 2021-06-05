class Memory:
    _data: bytearray
    _begin: int
    _end: int

    def __init__(self, buf: bytearray = bytearray(bytes(0)), begin: int = 0, end: int = None):
        self._data = buf
        self._begin = begin
        if end is None:
            end = len(self._data)
        self._end = end

    def get_slice(self, begin: int, length: int = None):
        if length is None:
            length = self._end - (self._begin + begin)
        
        return Memory(self._data, self._begin + begin, self._begin + begin + length)

    def __len__(self):
        return self._end - self._begin

    def raw_bytes(self) -> memoryview:
        return memoryview(self._data)[slice(self._begin, self._end)]

    def get_base(self):
        return self._begin

    def is_same_memory(self, other_mem):
        return other_mem._data == self._data
