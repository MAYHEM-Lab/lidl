mod lidl;

fn main() {
    let mut buf : [u8; 128] = [0; 128];
    let mut mb = lidl::MessageBuilder::new(&mut buf);

    let ostr = lidl::String::new(&mut mb, "hello rust");
    println!("Lidl string: {}", ostr.get());
}