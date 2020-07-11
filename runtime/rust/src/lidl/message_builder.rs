pub struct MessageBuilder<'a> {
    data: &'a mut [u8],
    ptr: u32
}

impl MessageBuilder<'_> {
    pub fn new<'a>(buf: &'a mut [u8]) -> MessageBuilder<'a> {
        MessageBuilder {data: buf, ptr: 0}
    }

    pub fn allocate<'a>(&'a mut self, size: i32, align: i32) -> &'a mut [u8] {
        while self.ptr % align as u32 != 0 {
            self.ptr += 1;
        }

        let res= &mut self.data[self.ptr as usize .. self.ptr as usize + size as usize];

        self.ptr += size as u32;

        res
    }
}