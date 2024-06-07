require test_mod
require rtti
require fio
require ast
require daslib/cpp_bind

class DasClass{
    x : int[65536];
}
[export]
def main {
    // let a <- type_info();
    var cls = new DasClass();
    add_object(cls);
    // add_object(cls, type_info(*cls));
}